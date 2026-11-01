#include "IrrSimpleVoxelEngine.h"

#include "IrrSVEDefines.h"

#include <cfloat> //FLT_EPSILON - шаг поиска по точке с учётом точности float (добавлено).
#include <algorithm> //std::max (добавлено).
#include <chrono> //std::chrono::milliseconds - ожидание потоков в деструкторе.

using namespace std;

using namespace irr;
using namespace scene;
using namespace core;

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Voxels;
using namespace Additional;
using namespace Debug;
using namespace Defines;

using namespace IrrEngine;
using namespace IrrSVEDefines;

/*
    Схема потоков враппера:
    - главный поток (поток Irrlicht): вызывает update(), создаёт/удаляет ноды, рисует;
    - поток главной очереди: по порядку выполняет задания (загрузка/выгрузка чанков,
      установка/удаление вокселей, пользовательские функции UserControllerForThread);
    - пулы потоков построения: делают "снимок" чанка под s_LockAccessToTheSimpleVoxelEngine,
      затем без блокировки строят меш и отдают его фабрике.
    Все обращения к данным SimpleVoxelEngine идут под s_LockAccessToTheSimpleVoxelEngine.
    Порядок захвата блокировок (чтобы не было взаимных блокировок):
    s_LockAccessToTheSimpleVoxelEngine -> блокировки фабрики -> s_LockBuildQueues / s_LockMainQueue.
*/

//Деление с округлением вниз: для отрицательных координат обычное '/' округляет к нулю и даёт не тот чанк.
//! Деление с округлением вниз.
/*! \param a Делимое.
\param b Делитель (не 0).
\return Возвращает наибольшее целое, не большее a / b.
*/
static int floorDivide(int a, int b)
{
    int q = a / b;
    if((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

#ifdef SVE_DEBUG_CHECKS
//! Движок, потоком построения которого является текущий поток (только для отладочной проверки stop()).
/*! Записывается в начале threadBuildChunksController(). Так поток построения узнаётся без перебора списка потоков
s_ThreadsBuildChunks, который stop() из другого потока может в это же время очищать.
*/
static thread_local SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine* t_BuildThreadEngine = 0;
#endif

SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::IrrSimpleVoxelEngine(IrrlichtDevice* device, Logger* lgr):
    Engine(lgr),
    s_Device(device),
    s_FCMSN(0),
    s_UserControllerForThread(0),
    s_DeleteUserControllerForThread(true),
    s_IdThreadMainQueue(std::thread::id()), //Добавлено: поле стало atomic - начальное значение "ни один поток".
    s_StopBuildThreads(false),
    s_MaxNumberOfThreadAddChunks(DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS),
    s_MaxNumberOfThreadUpdateChunks(DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS),
    s_NumberOfRunningThreads(0),
    s_StopRequested(false),
    s_MainQueueAlive(false), //Добавлено.
    s_TimeFromDevice(device ? device->getTimer()->getTime() : 0)
{
    s_IdMainThread = this_thread::get_id(); //Добавлено: главный поток - тот, в котором создан движок.
    s_LastSaveNumber = 0; //Добавлено: номера сохранений saveAllChunks() начинаются с 1.
    s_ChunkStreamingPool = new ChunkStreamingPool(this); //Добавлено: пул стриминга чанков.
}

SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::IrrSimpleVoxelEngine(IrrlichtDevice* device):
    IrrSimpleVoxelEngine(device, 0)
{
    //...
}

//Раньше деструктор удалял фабрику и контроллер, не останавливая потоки, которые ими пользовались.
//Теперь сначала останавливаем потоки (stop() выполнит оставшиеся задания), затем удаляем то, что осталось в очереди.
SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::~IrrSimpleVoxelEngine()
{
    if(isRun() && !s_StopRequested) stop(true); //Потоки ещё пользуются фабрикой и контроллером - сначала остановить.
    //Исправлено: после stop(false) потоки отсоединены и join() невозможен - ждём, пока они сами закончат
    //(раньше деструктор сразу удалял фабрику и очередь, которыми эти потоки ещё пользовались).
    while(isRun()) this_thread::sleep_for(std::chrono::milliseconds(1));
    {
        //Добавлено: завершившиеся, но не присоединённые потоки присоединяем - деструктор joinable-объекта
        //std::thread вызывает std::terminate (так было после задания STOP, поставленного через addMainTask()).
        lock_guard<mutex> stoplocker(s_LockStop);
        joinThreads();
    }
    for(unsigned int i = 0; i < s_TaskMainQueue.size(); i++) deleteTask(s_TaskMainQueue[i]);
    s_TaskMainQueue.clear();
    //Добавлено: пул стриминга - после остановки потоков (задания пула больше не выполняются). Чанки он не трогает.
    delete s_ChunkStreamingPool;
    s_ChunkStreamingPool = 0;
    //Добавлено: ноды чанков убираются, пока фабрика пользователя ещё цела, - тогда вызывается его
    //onUpdateChunkMeshSceneNode(0, ...). Раньше ноды удалял деструктор базового класса фабрики, когда часть пользователя
    //уже разрушена, и пользователь не узнавал об удалении нод (его селекторы треугольников ссылались на удалённые ноды).
    if(s_FCMSN) s_FCMSN->deleteAllChunks();
    if(s_FCMSN) delete s_FCMSN;
    s_FCMSN = 0;
    if(s_UserControllerForThread.load() && s_DeleteUserControllerForThread) delete s_UserControllerForThread.load(); //Контроллер пользователя (deletebyengine == false) не удаляем.
    s_UserControllerForThread = 0;
}

//Пользовательская функция UserControllerForThread теперь выполняется в этом же потоке (раньше у неё был свой
//поток, и он менял мир параллельно с главной очередью). Отсюда изменения мира выполняются сразу, без очереди.
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::isMainQueueThread()
{
    return this_thread::get_id() == s_IdThreadMainQueue.load(); //Изменено: поле стало atomic (его пишут другие потоки).
}

//Добавлено: проверка "текущий поток - главный" (тот, в котором создан движок).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::isMainThread()
{
    return this_thread::get_id() == s_IdMainThread;
}

#ifdef SVE_DEBUG_CHECKS
//Добавлено: отладочная проверка главного потока (компилируется только при SVE_DEBUG_CHECKS).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::debugCheckMainThread(const char* where)
{
    if(isMainThread()) return true;
    registerEvent(EVENT_TYPE_LOGIC_VIOLATION, std::string("IrrSVE: Debug check (SVE_DEBUG_CHECKS): ") + where + " must be called from the main thread (the thread where the engine was created).");
    return false;
}
#endif

//Новая функция. Раньше задания удалялись через delete указателя на базовый Task (у Task нет виртуального
//деструктора - формально неопределённое поведение), воксель из невыполненного задания SET_SIMPLE_VOXEL_OBJECT
//терялся, а задания, оставшиеся в очередях при остановке, не удалялись совсем.
//Теперь каждое задание удаляется по своему типу вместе со всем, чем оно владеет (адрес, воксель, параметры).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteTask(Task* _task)
{
    if(!_task) return;
    switch(_task->s_TypeTask)
    {
        case SET_SIMPLE_VOXEL_OBJECT:
        {
            TaskSetSimpleVoxelObject* t = (TaskSetSimpleVoxelObject*)_task;
            if(t->s_VoxelAddress) delete t->s_VoxelAddress;
            if(t->s_SimpleVoxelObject) delete t->s_SimpleVoxelObject;
            delete t;
            break;
        }
        case DELETE_SIMPLE_VOXEL_OBJECT:
        {
            TaskDeleteSimpleVoxelObject* t = (TaskDeleteSimpleVoxelObject*)_task;
            if(t->s_VoxelAddress) delete t->s_VoxelAddress;
            delete t;
            break;
        }
        case UPDATE_CHUNKS: delete (TaskUpdateChunk*)_task; break;
        case ADD_CHUNKS: delete (TaskAddChunks*)_task; break;
        case DELETE_CHUNKS: delete (TaskDeleteChunks*)_task; break;
        case ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE: delete (TaskAddTasksChunksForUpdate*)_task; break;
        case RUN_THREAD_FUNCTION_WITH_PARAMETERS:
        {
            TaskRunThreadFunctionWithParameters* t = (TaskRunThreadFunctionWithParameters*)_task;
            if(t->s_Parameters) delete t->s_Parameters;
            delete t;
            break;
        }
        case SAVE_CHUNKS:
        {
            //Добавлено: невыполненное сохранение (движок удалён раньше, чем очередь до него дошла) - сообщаем ждущим
            //об ошибке, иначе isSaveCompleted() никогда не вернула бы true.
            TaskSaveChunks* t = (TaskSaveChunks*)_task;
            if(!t->isCompleted) finishSave(t->s_SaveNumber, false);
            delete t;
            break;
        }
        case SET_USER_CONTROLLER_FOR_THREAD:
        {
            //Невыполненная замена: если новый контроллер отдан движку - удаляем его (если он не текущий).
            //Контроллер пользователя (deletebyengine == false) не трогаем.
            TaskSetUserControllerForThread* t = (TaskSetUserControllerForThread*)_task;
            if(t->s_UserControllerForThread && t->s_DeleteByEngine && t->s_UserControllerForThread != s_UserControllerForThread.load()) delete t->s_UserControllerForThread;
            delete t;
            break;
        }
        case STREAM_CHUNKS: delete (TaskStreamChunks*)_task; break; //Добавлено: у задания есть std::vector - удалять как TaskStreamChunks.
        default: delete _task; break;
    }
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskUpdateChunks(bool all)
{
    TaskAddTasksChunksForUpdate* taskaddtaskschunksforupdate = new TaskAddTasksChunksForUpdate;
    taskaddtaskschunksforupdate->s_TypeTask = ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE;
    taskaddtaskschunksforupdate->isCompleted = false;
    taskaddtaskschunksforupdate->s_All = all;
    addMainTask(taskaddtaskschunksforupdate);
}

//Из потока главной очереди (и из пользовательской функции, она работает в нём же) воксель ставится сразу,
//из любого другого потока - заданием в главную очередь. Так все изменения мира идут в одном потоке и по порядку.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObject(int id, VoxelAddress* vxladdr, SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): адрес создан другим движком (бывает, если движков несколько). Работа продолжается.
    if(vxladdr && vxladdr->getEngine() != static_cast<SimpleVoxelEngine::Core::Engine*>(this)) registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): setSimpleVoxelObject: voxel address was created by another engine.");
#endif
    setSimpleVoxelObjectAnyThread(id, vxladdr, svo, rewrite, immediatelyupdated, 0); //Изменено: тело вынесено в setSimpleVoxelObjectAnyThread() - его использует и setSimpleVoxelObjectByGlobalVector().
}

//Добавлено: бывшее тело setSimpleVoxelObject() с параметром requiredpartition. Общее сообщение
//"function returned false" убрано: причину уже пишет setSimpleVoxelObjectThread() (или ядро) - сообщение было дублем.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObjectAnyThread(int id, VoxelAddress* vxladdr, SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated, int requiredpartition)
{
    if(isMainQueueThread())
    {
        setSimpleVoxelObjectThread(id, vxladdr, svo, rewrite, immediatelyupdated, requiredpartition);
        if(vxladdr) delete vxladdr;
        return;
    }
    TaskSetSimpleVoxelObject* tasksetsimplevoxelobject = new TaskSetSimpleVoxelObject;
    tasksetsimplevoxelobject->s_TypeTask = SET_SIMPLE_VOXEL_OBJECT;
    tasksetsimplevoxelobject->isCompleted = false;
    tasksetsimplevoxelobject->s_WorldId = id;
    tasksetsimplevoxelobject->s_VoxelAddress = vxladdr;
    tasksetsimplevoxelobject->s_SimpleVoxelObject = svo;
    tasksetsimplevoxelobject->s_Rewrite = rewrite;
    tasksetsimplevoxelobject->s_Immediatelyupdated = immediatelyupdated;
    tasksetsimplevoxelobject->s_RequiredPartition = requiredpartition; //Добавлено.
    addMainTask(tasksetsimplevoxelobject);
}

//Пометка чанков для перестройки после изменения вокселя: сам чанк и соседи, если воксель на границе
//(у соседа могла открыться или закрыться грань).
//Раньше соседи помечались, только если отрисовывался сам чанк, и без проверки, отрисовываются ли они;
//при immediatelyupdated на каждый воксель создавалось до 7 отдельных заданий. Теперь каждый чанк проверяется
//отдельно, а requestBuildChunk() не ставит в очередь чанк, который уже ждёт построения.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setUpdateChunksForChangeSimpleVoxelObject(int id, VoxelAddress* vxladdr, World* wrld, bool immediatelyupdated)
{
    if(!s_FCMSN || vxladdr->getLastVectInd() < 2) return;
    const Vector3d<int> vectchunk = vxladdr->getVector(0);
    const Vector3d<int> vectinchunk = vxladdr->getVector(1);
    //Исправлено: последний индекс - по разбиению самого чанка (раньше по getSizeChunk(): в чанке с другим
    //разбиением воксель на границе не помечал соседа, и у соседа оставалась неверная грань).
    ChunkVoxelObject* chunk = wrld->getChunk(vectchunk);
    const int lastindex = (chunk ? (int)chunk->getNumberOfPartitions() : wrld->getSizeChunk()) - 1;
    std::vector< Vector3d<int> > chunks;
    chunks.push_back(vectchunk);
    if(vectinchunk.getX() == 0) chunks.push_back(vectchunk + Vector3d<int>(-1,0,0));
    if(vectinchunk.getX() == lastindex) chunks.push_back(vectchunk + Vector3d<int>(1,0,0));
    if(vectinchunk.getY() == 0) chunks.push_back(vectchunk + Vector3d<int>(0,-1,0));
    if(vectinchunk.getY() == lastindex) chunks.push_back(vectchunk + Vector3d<int>(0,1,0));
    if(vectinchunk.getZ() == 0) chunks.push_back(vectchunk + Vector3d<int>(0,0,-1));
    if(vectinchunk.getZ() == lastindex) chunks.push_back(vectchunk + Vector3d<int>(0,0,1));
    for(unsigned int i = 0; i < chunks.size(); i++)
    {
        if(!s_FCMSN->isRenderChunk(id, chunks[i])) continue;
        if(immediatelyupdated) requestBuildChunk(id, chunks[i], true);
        else s_FCMSN->setChunkForUpdate(id, chunks[i]);
    }
}

//! Проверка адреса вокселя перед установкой или удалением.
/*! Вызывается под блокировкой движка. Ядро в релизе не проверяет, что вектор адреса лежит внутри разбиения своего чанка
(такие проверки - только при SVE_DEBUG_CHECKS, ради скорости), и вектор вне чанка попадал в чужую ячейку. Здесь проверка
разовая (одна на изменение), поэтому включена всегда. Незагруженный чанк - не ошибка программиста: его могли выгрузить,
пока задание ждало в очереди, - пишется предупреждение (ядро написало бы фатальную ошибку).
\param isve Движок (для записи в лог).
\param wrld Мир.
\param vxladdr Адрес вокселя.
\param requiredpartition Если больше 0 - требуемое разбиение чанка верхнего уровня (для *ByGlobalVector).
\param action Действие для сообщения в логе ("setting" или "deleting").
\return Возвращает true, если адрес можно передать ядру (адрес короче двух векторов или со сдвигом уровня тоже передаётся -
его отклонит само ядро со своим сообщением); false - причина записана в лог.
*/
static bool checkVoxelAddressForChange(IrrSimpleVoxelEngine* isve, World* wrld, VoxelAddress* vxladdr, int requiredpartition, const std::string& action)
{
    if(vxladdr->getLastVectInd() < 2 || vxladdr->getLevelShift() != 0) return true;
    ChunkVoxelObject* chunk = wrld->getChunk(vxladdr->getVector(0));
    if(!chunk)
    {
        isve->registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Warning " + action + " simple voxel object: the chunk is not loaded (it may have been unloaded before the task was executed), nothing is changed.");
        return false;
    }
    if(requiredpartition > 0 && (int)chunk->getNumberOfPartitions() != requiredpartition)
    {
        isve->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error " + action + " simple voxel object by global vector: the partition of the chunk differs from getSizeChunk() of the world.");
        return false;
    }
    for(int i = 1; i < vxladdr->getLastVectInd(); i++)
    {
        const Vector3d<int> v = vxladdr->getVector(i);
        const int n = (int)chunk->getNumberOfPartitions();
        if(v.getX() < 0 || v.getY() < 0 || v.getZ() < 0 || v.getX() >= n || v.getY() >= n || v.getZ() >= n)
        {
            isve->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error " + action + " simple voxel object: vector " + v.toString(", ") + " of the address is outside the partition of its chunk.");
            return false;
        }
        if(i + 1 == vxladdr->getLastVectInd()) break;
        //Следующий вектор - внутри вложенного чанка. Если его нет, адрес отклонит ядро (со своим сообщением).
        SimpleVoxelObject* nested = chunk->getSimpleVoxelObject(v);
        if(!nested || nested->getTypeVoxel() != CHUNK_VOXEL_OBJECT) break;
        chunk = (ChunkVoxelObject*)nested;
    }
    return true;
}

//Раньше мир менялся без блокировки движка, параллельно с потоками построения, читавшими тот же чанк
//(а для соседних чанков ожидание вызывалось с waitunlock = false, т.е. не ждало) - основная причина падений.
//Теперь вся работа с миром - под s_LockAccessToTheSimpleVoxelEngine. Если воксель не попал в мир, он удаляется (раньше - утечка).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObjectThread(int id, VoxelAddress* vxladdr, SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated, int requiredpartition)
{
    if(!vxladdr)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: voxel address does not exist.");
        if(svo) delete svo;
        return false;
    }
    if(!svo)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: svo does not exist.");
        return false;
    }
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: world does not exist.");
        delete svo;
        return false;
    }
    //Добавлено: адрес проверяется до передачи ядру (чанк загружен, векторы внутри разбиения, для
    //*ByGlobalVector - разбиение чанка равно getSizeChunk()).
    if(!checkVoxelAddressForChange(this, wrld, vxladdr, requiredpartition, "setting"))
    {
        delete svo;
        return false;
    }
    bool ret = wrld->setSimpleVoxelObject(vxladdr, svo, rewrite);
    if(!ret)
    {
        //Объект не попал в мир - память освобождаем сами, иначе утечка.
        delete svo;
        return false;
    }
    setUpdateChunksForChangeSimpleVoxelObject(id, vxladdr, wrld, immediatelyupdated);
    return true;
}

//Так же, как setSimpleVoxelObject(): из потока главной очереди - сразу, из остальных - через очередь.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteSimpleVoxelObject(int id, VoxelAddress* vxladdr, bool immediatelyupdated)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): адрес создан другим движком. Работа продолжается.
    if(vxladdr && vxladdr->getEngine() != static_cast<SimpleVoxelEngine::Core::Engine*>(this)) registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): deleteSimpleVoxelObject: voxel address was created by another engine.");
#endif
    deleteSimpleVoxelObjectAnyThread(id, vxladdr, immediatelyupdated, 0); //Изменено: тело вынесено в deleteSimpleVoxelObjectAnyThread().
}

//Добавлено: бывшее тело deleteSimpleVoxelObject() с параметром requiredpartition.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteSimpleVoxelObjectAnyThread(int id, VoxelAddress* vxladdr, bool immediatelyupdated, int requiredpartition)
{
    if(isMainQueueThread())
    {
        //Изменено: без сообщения "function returned false" - причину (например, пустая ячейка) уже пишет ядро.
        deleteSimpleVoxelObjectThread(id, vxladdr, immediatelyupdated, requiredpartition);
        if(vxladdr) delete vxladdr;
        return;
    }
    TaskDeleteSimpleVoxelObject* taskdeletesimplevoxelobject = new TaskDeleteSimpleVoxelObject;
    taskdeletesimplevoxelobject->s_TypeTask = DELETE_SIMPLE_VOXEL_OBJECT;
    taskdeletesimplevoxelobject->isCompleted = false;
    taskdeletesimplevoxelobject->s_WorldId = id;
    taskdeletesimplevoxelobject->s_VoxelAddress = vxladdr;
    taskdeletesimplevoxelobject->s_Immediatelyupdated = immediatelyupdated;
    taskdeletesimplevoxelobject->s_RequiredPartition = requiredpartition; //Добавлено.
    addMainTask(taskdeletesimplevoxelobject);
}

//Удаление под блокировкой движка (раньше - без неё). Чанки помечаются для перестройки, только если воксель
//действительно был удалён (раньше - всегда).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteSimpleVoxelObjectThread(int id, VoxelAddress* vxladdr, bool immediatelyupdated, int requiredpartition)
{
    if(!vxladdr)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting simple voxel object: voxel address does not exist.");
        return false;
    }
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting simple voxel object: world does not exist.");
        return false;
    }
    if(!checkVoxelAddressForChange(this, wrld, vxladdr, requiredpartition, "deleting")) return false; //Добавлено: как в setSimpleVoxelObjectThread().
    bool ret = wrld->deleteSimpleVoxelObject(vxladdr);
    if(ret) setUpdateChunksForChangeSimpleVoxelObject(id, vxladdr, wrld, immediatelyupdated);
    return ret;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunkForRendering(int id, Vector3d<int> vect, bool load, bool loadframe, int sizeframe)
{
    addChunksForRendering(id, vect, vect, load, loadframe, sizeframe);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunksForRendering(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool load, bool loadframe, int sizeframe)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): при перепутанных углах область пустая, и задание молча ничего не делает.
    if(vectleftdown.getX() > vectrightup.getX() || vectleftdown.getY() > vectrightup.getY() || vectleftdown.getZ() > vectrightup.getZ())
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): addChunksForRendering: vectleftdown > vectrightup on some axis, the area is empty.");
#endif
    TaskAddChunks* taskaddchunk = new TaskAddChunks;
    taskaddchunk->s_TypeTask = ADD_CHUNKS;
    taskaddchunk->isCompleted = false;
    taskaddchunk->s_WorldId = id;
    taskaddchunk->s_VectorLeftDown = vectleftdown;
    taskaddchunk->s_VectorRightUp = vectrightup;
    taskaddchunk->s_Load = load;
    taskaddchunk->s_LoadFrame = loadframe;
    taskaddchunk->s_SizeFrame = sizeframe;
    addMainTask(taskaddchunk);
}

//Загрузка/генерация чанков области (и рамки) перед построением. Выполняется в потоке главной очереди.
//Раньше для внутренних чанков не проверялось, что чанк уже в памяти: loadChunk() для загруженного чанка
//возвращал false, и функция выходила (return false), не загрузив остальную область. Теперь такие чанки пропускаются.
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunksForRenderingThreadPreLoad(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool load, bool loadframe, int sizeframe, bool& fatal)
{
    fatal = false; //Добавлено: true - область добавить нельзя совсем (ниже), чанки тогда не регистрируются.
    int sizeframetemp = 0;
    World* wrld = 0;
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        wrld = getWorld(id);
    }
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: world does not exist.");
        fatal = true; //Добавлено.
        return false;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: factory chunk mesh scene does not setted.");
        fatal = true; //Добавлено.
        return false;
    }
    if(loadframe)
    {
        if(sizeframe < 1)
        {
            registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering (adding frame): frame size < 1.");
            fatal = true; //Добавлено.
            return false;
        }
        sizeframetemp = sizeframe;
    }
    bool good = true;
    //Добавлено: чанки за границами мира пропускаются без сообщений (их нет) - как в World::loadChunks().
    for(int x = vectleftdown.getX() - sizeframetemp; x <= vectrightup.getX() + sizeframetemp; x++)
        for(int y = vectleftdown.getY() - sizeframetemp; y <= vectrightup.getY() + sizeframetemp; y++)
            for(int z = vectleftdown.getZ() - sizeframetemp; z <= vectrightup.getZ() + sizeframetemp; z++)
            {
                Vector3d<int> vect(x, y, z);
                bool inner = !( (x < vectleftdown.getX() || y < vectleftdown.getY() || z < vectleftdown.getZ() ) || (x > vectrightup.getX() || y > vectrightup.getY() || z > vectrightup.getZ() ) );
                //Блокировка берётся на каждый чанк, чтобы потоки построения и главный поток не ждали загрузки всей области.
                lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
                if(!wrld->isChunkInBounds(vect)) continue;
                if(wrld->isExistsChunkInMemory(vect)) continue;
                if(inner && !load)
                {
                    registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: chunk does not exist.");
                    good = false;
                    continue;
                }
                if(!wrld->loadChunk(vect))
                {
                    good = false;
                    if(inner) registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding chunk for rendering: error loading.");
                    else registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding chunk for rendering (adding frame): error loading.");
                }
            }
    return good;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunkForRendering(int id, Vector3d<int> vect, bool unload, bool save, bool unloadframe, int sizeframe)
{
    deleteChunksForRendering(id, vect, vect, unload, save, unloadframe, sizeframe);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunksForRendering(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool unload, bool save, bool unloadframe, int sizeframe)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): то же, что в addChunksForRendering().
    if(vectleftdown.getX() > vectrightup.getX() || vectleftdown.getY() > vectrightup.getY() || vectleftdown.getZ() > vectrightup.getZ())
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): deleteChunksForRendering: vectleftdown > vectrightup on some axis, the area is empty.");
#endif
    TaskDeleteChunks* taskdeletechunk = new TaskDeleteChunks;
    taskdeletechunk->s_TypeTask = DELETE_CHUNKS;
    taskdeletechunk->isCompleted = false;
    taskdeletechunk->s_WorldId = id;
    taskdeletechunk->s_VectorLeftDown = vectleftdown;
    taskdeletechunk->s_VectorRightUp = vectrightup;
    taskdeletechunk->s_Unload = unload;
    taskdeletechunk->s_Save = save;
    taskdeletechunk->s_UnloadFrame = unloadframe;
    taskdeletechunk->s_SizeFrame = sizeframe;
    addMainTask(taskdeletechunk);
}

//Удаление чанков из отрисовки и (по флагам) выгрузка из памяти. Выполняется в потоке главной очереди.
//Добавлено: соседи выгруженных чанков перестраиваются (на месте выгруженного чанка у них открываются грани),
//чанк рамки, который сам отрисовывается, не выгружается.
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunksForRenderingThreadUnload(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool unload, bool save, bool unloadframe, int sizeframe)
{
    int sizeframetemp = 0;
    World* wrld = 0;
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        wrld = getWorld(id);
    }
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering: world does not exist.");
        return false;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering: factory chunk mesh scene does not setted.");
        return false;
    }
    if(unloadframe)
    {
        if(sizeframe < 1)
        {
            registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering (deleting frame): frame size < 1.");
            return false;
        }
        sizeframetemp = sizeframe;
    }
    bool good = true;
    std::set< Vector3d<int> > unloadedchunks;
    //Сначала убираем чанки из отрисовки (это отменяет и все ещё не законченные построения их мешей).
    for(int x = vectleftdown.getX(); x <= vectrightup.getX(); x++)
        for(int y = vectleftdown.getY(); y <= vectrightup.getY(); y++)
            for(int z = vectleftdown.getZ(); z <= vectrightup.getZ(); z++)
            {
                lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
                s_FCMSN->deleteChunk(id, Vector3d<int>(x, y, z));
            }
    //Затем выгружаем данные.
    for(int x = vectleftdown.getX() - sizeframetemp; x <= vectrightup.getX() + sizeframetemp; x++)
        for(int y = vectleftdown.getY() - sizeframetemp; y <= vectrightup.getY() + sizeframetemp; y++)
            for(int z = vectleftdown.getZ() - sizeframetemp; z <= vectrightup.getZ() + sizeframetemp; z++)
            {
                Vector3d<int> vect(x, y, z);
                bool inner = !( (x < vectleftdown.getX() || y < vectleftdown.getY() || z < vectleftdown.getZ() ) || (x > vectrightup.getX() || y > vectrightup.getY() || z > vectrightup.getZ() ) );
                if(!inner && !unloadframe) continue;
                if(inner && !unload) continue;
                lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
                if(!wrld->isExistsChunkInMemory(vect)) continue;
                //Чанк рамки, который сам отрисовывается, выгружать нельзя.
                if(!inner && s_FCMSN->isRenderChunk(id, vect)) continue;
                if(!wrld->unloadChunk(vect, save))
                {
                    good = false;
                    if(inner) registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error deleting chunk for rendering: error unloading.");
                    else registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error deleting chunk for rendering (deleting frame): error unloading.");
                }
                else unloadedchunks.insert(vect);
            }
    //Соседи выгруженных чанков, которые остались в отрисовке, должны показать открывшиеся грани.
    const Vector3d<int> vectsaround[6] = { Vector3d<int>(1,0,0), Vector3d<int>(-1,0,0), Vector3d<int>(0,1,0), Vector3d<int>(0,-1,0), Vector3d<int>(0,0,1), Vector3d<int>(0,0,-1) };
    std::set< Vector3d<int> > neighbours;
    for(std::set< Vector3d<int> >::iterator iter = unloadedchunks.begin(); iter != unloadedchunks.end(); ++iter)
        for(int i = 0; i < 6; i++)
        {
            Vector3d<int> n = *iter + vectsaround[i];
            if(unloadedchunks.find(n) == unloadedchunks.end() && s_FCMSN->isRenderChunk(id, n)) neighbours.insert(n);
        }
    for(std::set< Vector3d<int> >::iterator iter = neighbours.begin(); iter != neighbours.end(); ++iter) requestBuildChunk(id, *iter, true);
    return good;
}

UserControllerForThread* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getUserControllerForThread()
{
    return s_UserControllerForThread;
}

//Изменено: параметр deletebyengine - контроллер может принадлежать пользователю (пул контроллеров), тогда движок
//его не удаляет ни при замене, ни в деструкторе.
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setUserControllerForThread(UserControllerForThread* usrcntrlforthread, bool deletebyengine)
{
    //Изменено: раньше контроллер ставился только один раз и заменить его было нельзя. Теперь его можно заменить
    //в любой момент. Во время работы контроллер вызывается в потоке главной очереди, поэтому замена ставится туда же
    //заданием: старый контроллер удаляется только тогда, когда им гарантированно никто не пользуется.
    if(isRun())
    {
        TaskSetUserControllerForThread* task = new TaskSetUserControllerForThread;
        task->isCompleted = false;
        task->s_TypeTask = SET_USER_CONTROLLER_FOR_THREAD;
        task->s_UserControllerForThread = usrcntrlforthread;
        task->s_DeleteByEngine = deletebyengine;
        addMainTask(task);
        return true;
    }
    UserControllerForThread* old = s_UserControllerForThread.load();
    const bool deleteold = s_DeleteUserControllerForThread;
    s_UserControllerForThread = usrcntrlforthread;
    s_DeleteUserControllerForThread = deletebyengine; //Тот же контроллер - меняется только признак владения.
    if(old && old != usrcntrlforthread && deleteold) delete old;
    return true;
}

FactoryChunkMeshSceneNode* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getFactoryChunkMeshSceneNode()
{
    return s_FCMSN;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setFactoryChunkMeshSceneNode(FactoryChunkMeshSceneNode* fcmsn)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckMainThread("setFactoryChunkMeshSceneNode")) return false; //Отладочная проверка (добавлено): только главный поток.
#endif
    if(!fcmsn)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: pointer is NULL.");
        return false;
    }
    if(s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: factory chunk mesh scene node is already setted in this engine.");
        return false;
    }
    if(fcmsn->isFreeze())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node is already setted in other engine.");
        return false;
    }
    if(this != fcmsn->getIrrSimpleVoxelEngine() && fcmsn->getIrrSimpleVoxelEngine() != 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node is bound to another engine.");
        return false;
    }
    if(s_Device != fcmsn->getIrrlichtDevice() && fcmsn->getIrrlichtDevice() != 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node has a different IrrlichtDevice.");
        return false;
    }
    if(fcmsn->getIrrSimpleVoxelEngine() == 0) fcmsn->setIrrSimpleVoxelEngine(this);
    if(fcmsn->getIrrlichtDevice() == 0) fcmsn->setIrrlichtDevice(s_Device);
    fcmsn->freeze();
    s_FCMSN = fcmsn;
    return true;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskRunUserControllerForThread(ParametersForThreadFunction* par)
{
    TaskRunThreadFunctionWithParameters* taskrunthreadfuncwithparam = new TaskRunThreadFunctionWithParameters;
    taskrunthreadfuncwithparam->isCompleted = false;
    taskrunthreadfuncwithparam->s_TypeTask = RUN_THREAD_FUNCTION_WITH_PARAMETERS;
    taskrunthreadfuncwithparam->s_Parameters = par;
    addMainTask(taskrunthreadfuncwithparam);
}

//Раньше здесь главный поток переключал режим STANDBY, переносил задания из запасной очереди и будил потоки.
//Теперь потоки будятся сами при постановке задания, а update() только применяет готовые меши (строго в главном потоке).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::update()
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckMainThread("update")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    //Добавлено: время для getTimeSafeThread() обновляется и здесь - если вместо runIrrDevice() вызывают
    //device->run(), время стояло бы, и не срабатывали бы таймаут отложенной склейки блоков и таймер Occlusion Queries.
    if(s_Device) s_TimeFromDevice = s_Device->getTimer()->getTime();
    //Добавлено: положение родительских нод миров запоминается для getWorldCoordinates() из других потоков.
    std::vector<int> worlds;
    {
        lock_guard<mutex> locker(s_LockWorldTransforms);
        for(std::map<int, WorldTransform>::iterator iter = s_WorldTransforms.begin(); iter != s_WorldTransforms.end(); ++iter) worlds.push_back(iter->first);
    }
    for(unsigned int i = 0; i < worlds.size(); i++) updateWorldTransform(worlds[i]);
    if(s_FCMSN) s_FCMSN->update();
}

//Добавлено: Occlusion Queries - между drawAll() и endScene() (раньше выполнялись в update(), вне кадра:
//проверка рисовалась не в Z-буфер текущего кадра, а с Direct3D - вне BeginScene()/EndScene()).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::runOcclusionQueries()
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckMainThread("runOcclusionQueries")) return; //Отладочная проверка: только главный поток.
#endif
    if(s_FCMSN) s_FCMSN->runOcclusionQueries();
}

//Раньше задания принимались только из главного потока и потока пользовательской функции (из остальных - ошибка
//"invalid stream"), а в режиме STANDBY уходили в запасную очередь. Теперь - из любого потока, в одну очередь.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addMainTask(Task* _task)
{
    if(!_task) return;
#ifdef SVE_DEBUG_CHECKS
    //Добавлено: STOP ставит только stop(). STOP, поставленный сюда, останавливал потоки "тайком": stop() считал
    //движок уже остановленным и потоки не присоединял, а деструктор joinable-объекта std::thread вызывал std::terminate.
    //Изменено: проверка - только в отладочной сборке: STOP через addMainTask() - ошибка программиста. Падения при удалении
    //движка нет и без проверки: деструктор, start() и stop() присоединяют завершившиеся потоки.
    //Без проверки такой STOP останавливает движок без stop(): задания, поставленные после него, ждут следующего start().
    if(_task->s_TypeTask == STOP)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): addMainTask: task STOP can not be added by addMainTask(), use stop().");
        deleteTask(_task);
        return;
    }
#endif
    {
        lock_guard<mutex> locker(s_LockMainQueue);
        s_TaskMainQueue.push_back(_task);
    }
    //Изменено: без notify_one() - поток очереди сам проверяет очередь раз в THREAD_WAIT_INTERVAL_IN_MILLISECONDS
    //(notify_one() в winpthreads из MinGW 8.1 иногда зависает навсегда - здесь встал бы главный поток).
}

//Новая функция: постановка чанка в очередь построения меша. update = true - изменённый чанк (очередь обновления),
//false - новый (очередь создания).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::requestBuildChunk(int id, Vector3d<int> vect, bool update)
{
    BuildJob job;
    job.s_WorldId = id;
    job.s_VectorChunk = vect;
    std::pair< int, Vector3d<int> > key(id, vect);
    bool added = false;
    {
        lock_guard<mutex> locker(s_LockBuildQueues);
        //Если чанк уже ждёт построения в этой очереди, второй раз его не добавляем:
        //снимок данных будет сделан, когда до него дойдёт очередь, и учтёт все изменения.
        if(update)
        {
            added = s_PendingUpdateChunks.insert(key).second;
            if(added) s_QueueUpdateChunks.push_back(job);
        }
        else
        {
            added = s_PendingAddChunks.insert(key).second;
            if(added) s_QueueAddChunks.push_back(job);
        }
    }
    //Изменено: потоки построения не будятся через notify_one() (в winpthreads из MinGW 8.1 он иногда зависает навсегда -
    //так вставал поток главной очереди при стриминге): свободный поток сам заберёт чанк при ближайшей проверке очереди.
}

//Потоки: один поток главной очереди + пулы построения размера setMaxNumberOfThreadAddChunks()/UpdateChunks()
//(раньше эти настройки ни на что не влияли, а сборщик мусора и поток пользовательских функций были отдельными потоками).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::start()
{
    if(isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error starting engine: engine already started.");
        return;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error starting engine: factory chunk mesh scene node is not setted.");
        return;
    }
    {
        //Добавлено: потоки прошлого запуска, которые завершились, но не присоединены, присоединяем -
        //присваивание нового потока joinable-объекту std::thread вызвало бы std::terminate.
        lock_guard<mutex> stoplocker(s_LockStop);
        joinThreads();
    }
    {
        lock_guard<mutex> locker(s_LockBuildQueues);
        s_StopBuildThreads = false;
    }
    const unsigned int numberofbuildthreads = s_MaxNumberOfThreadAddChunks + s_MaxNumberOfThreadUpdateChunks;
    s_ThreadsBuildChunks.reserve(numberofbuildthreads); //Добавлено: push_back() ниже не перевыделяет память (и не бросает исключение после создания потока).
    //Пока захвачен s_LockMainQueue, поток главной очереди не возьмёт ни одного задания,
    //поэтому к моменту первого задания s_IdThreadMainQueue уже записан.
    lock_guard<mutex> locker(s_LockMainQueue);
    s_StopRequested = false; //Изменено: под блокировкой очереди, как и в stop().
    s_MainQueueAlive = true; //Добавлено: задания, поставленные с этого момента, будут выполнены.
    //Изменено: счётчик работающих потоков растёт перед созданием каждого потока (поток уменьшает его при выходе).
    //Если система не дала создать поток (std::system_error), счётчик возвращается назад. Раньше он сразу ставился на полное
    //число потоков, исключения не ловились, и при ошибке программа завершалась.
    s_NumberOfRunningThreads++;
    try
    {
        s_ThreadMainQueueTaskController = thread(&IrrSimpleVoxelEngine::threadMainQueueTaskController, this);
    }
    catch(...)
    {
        s_NumberOfRunningThreads--;
        s_MainQueueAlive = false;
        registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error starting engine: can not create the main queue thread.", true);
        return;
    }
    s_IdThreadMainQueue = s_ThreadMainQueueTaskController.get_id();
    //Изменено: первым создаётся поток пула обновления (изменённые чанки строят только его потоки, новые - потоки
    //обоих пулов), затем все потоки создания, затем остальные потоки обновления. Если создать удастся не все потоки, движок
    //работает с теми, что есть, и хотя бы один из них строит всё.
    for(unsigned int i = 0; i < numberofbuildthreads; i++)
    {
        const bool updatepool = (i == 0 || i > s_MaxNumberOfThreadAddChunks);
        s_NumberOfRunningThreads++;
        try
        {
            s_ThreadsBuildChunks.push_back(thread(&IrrSimpleVoxelEngine::threadBuildChunksController, this, updatepool)); //Раньше - через обёртки threadAdd/UpdateChunkTaskController().
        }
        catch(...)
        {
            s_NumberOfRunningThreads--;
            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error starting engine: can not create all build threads, the engine works with fewer threads.", true);
            break;
        }
    }
}

//Остановка - обычное задание STOP в конце главной очереди, поэтому все задания до него выполняются.
//Из потока главной очереди вызывать нельзя: он ждал бы сам себя.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::stop(bool wait)
{
    if(!isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error stopping engine: engine already stopped.");
        //Добавлено: завершившиеся, но не присоединённые потоки присоединяем (иначе std::terminate в деструкторе std::thread).
        lock_guard<mutex> stoplocker(s_LockStop);
        joinThreads();
        return;
    }
    if(isMainQueueThread())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error stopping engine: stop() can not be called from user controller for thread.");
        return;
    }
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): из потока построения (например, из getMeshVoxel() фабрики) stop() ждал бы
    //сам себя - программа зависала бы.
    //Изменено: поток построения узнаётся по t_BuildThreadEngine, а не перебором s_ThreadsBuildChunks без
    //блокировки (stop() из другого потока в это время очищал список - гонка данных).
    if(t_BuildThreadEngine == this)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): stop() can not be called from a build thread (factory functions).");
        return;
    }
#endif
    //Исправлено: второй STOP не ставится (раньше stop(false), а затем деструктор ставили два STOP).
    //Изменено: флаг и само задание - под одной блокировкой очереди (STOP ставит только stop(), см. addMainTask()),
    //поэтому saveAllChunks() под той же блокировкой видит STOP в очереди и ставит сохранение перед ним.
    {
        lock_guard<mutex> locker(s_LockMainQueue);
        if(!s_StopRequested.exchange(true))
        {
            Task* tsk = new Task;
            tsk->isCompleted = false;
            tsk->s_TypeTask = STOP;
            s_TaskMainQueue.push_back(tsk);
        }
    }
    //Изменено: без notify_one() (см. addMainTask()) - поток очереди увидит STOP сам.
    {
        //Добавлено: stop() из нескольких потоков сразу выполняются по очереди (join()/detach() одного
        //std::thread из двух потоков - неопределённое поведение).
        lock_guard<mutex> stoplocker(s_LockStop);
        if(wait) joinThreads();
        else
        {
            if(s_ThreadMainQueueTaskController.joinable()) s_ThreadMainQueueTaskController.detach();
            for(unsigned int i = 0; i < s_ThreadsBuildChunks.size(); i++)
                if(s_ThreadsBuildChunks[i].joinable()) s_ThreadsBuildChunks[i].detach();
            s_ThreadsBuildChunks.clear();
        }
    }
    //Исправлено: если потоки уже отсоединены (раньше был stop(false)), join() невозможен, и stop(true)
    //возвращался сразу, пока потоки ещё работали. Теперь ожидание - по счётчику работающих потоков.
    if(wait) while(isRun()) this_thread::sleep_for(std::chrono::milliseconds(1));
}

//Добавлено: присоединение потоков (вызывается под s_LockStop).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::joinThreads()
{
    if(s_ThreadMainQueueTaskController.joinable()) s_ThreadMainQueueTaskController.join();
    for(unsigned int i = 0; i < s_ThreadsBuildChunks.size(); i++)
        if(s_ThreadsBuildChunks[i].joinable()) s_ThreadsBuildChunks[i].join();
    s_ThreadsBuildChunks.clear();
}

//Выполнение одного задания главной очереди. Раньше задания разбирались в threadMainQueueTaskController и
//частично пересылались в другие очереди/потоки (добавление чанков, пользовательские функции).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::processMainTask(Task* _task)
{
    if(_task->isCompleted) return;
    if(_task->s_TypeTask == ADD_CHUNKS)
    {
        TaskAddChunks* t = (TaskAddChunks*)_task;
        bool fatal = false;
        if(!addChunksForRenderingThreadPreLoad(t->s_WorldId, t->s_VectorLeftDown, t->s_VectorRightUp, t->s_Load, t->s_LoadFrame, t->s_SizeFrame, fatal))
        {
            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error preload chunks (addChunksForRendering).");
        }
        //Исправлено: если область добавить нельзя совсем (нет мира, неверный размер рамки), чанки не регистрируются.
        //Раньше они регистрировались, и для несуществующего мира фабрика навсегда считала их отрисовываемыми.
        if(fatal) return;
        //Сначала регистрируем все чанки области (под одной блокировкой движка), потом ставим их на построение.
        //Если регистрировать и сразу строить по одному, первые чанки строятся раньше, чем зарегистрированы их
        //соседи по блоку, и блок не знает, что их надо подождать (отложенная склейка) - он склеивался лишний раз.
        World* boundsworld = 0; //Добавлено: чанки за границами мира не регистрируются и не строятся.
        {
            lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
            boundsworld = getWorld(t->s_WorldId);
            for(int x = t->s_VectorLeftDown.getX(); x <= t->s_VectorRightUp.getX(); x++)
                for(int y = t->s_VectorLeftDown.getY(); y <= t->s_VectorRightUp.getY(); y++)
                    for(int z = t->s_VectorLeftDown.getZ(); z <= t->s_VectorRightUp.getZ(); z++)
                    {
                        if(boundsworld && !boundsworld->isChunkInBounds(Vector3d<int>(x, y, z))) continue;
                        s_FCMSN->registerChunkForRendering(t->s_WorldId, Vector3d<int>(x, y, z));
                    }
        }
        for(int x = t->s_VectorLeftDown.getX(); x <= t->s_VectorRightUp.getX(); x++)
            for(int y = t->s_VectorLeftDown.getY(); y <= t->s_VectorRightUp.getY(); y++)
                for(int z = t->s_VectorLeftDown.getZ(); z <= t->s_VectorRightUp.getZ(); z++)
                {
                    if(boundsworld && !boundsworld->isChunkInBounds(Vector3d<int>(x, y, z))) continue;
                    requestBuildChunk(t->s_WorldId, Vector3d<int>(x, y, z), false);
                }
        //Уже отрисованные соседи области строились без неё и показывали грани на её месте - перестраиваем их.
        //(Раньше это приходилось делать вручную через setChunkForUpdate().)
        for(int x = t->s_VectorLeftDown.getX() - 1; x <= t->s_VectorRightUp.getX() + 1; x++)
            for(int y = t->s_VectorLeftDown.getY() - 1; y <= t->s_VectorRightUp.getY() + 1; y++)
                for(int z = t->s_VectorLeftDown.getZ() - 1; z <= t->s_VectorRightUp.getZ() + 1; z++)
                {
                    int outside = (x < t->s_VectorLeftDown.getX() || x > t->s_VectorRightUp.getX()) + (y < t->s_VectorLeftDown.getY() || y > t->s_VectorRightUp.getY()) + (z < t->s_VectorLeftDown.getZ() || z > t->s_VectorRightUp.getZ());
                    if(outside != 1) continue; //Только соседи по грани
                    Vector3d<int> vect(x, y, z);
                    if(s_FCMSN->isRenderChunk(t->s_WorldId, vect)) requestBuildChunk(t->s_WorldId, vect, true);
                }
    }
    else if(_task->s_TypeTask == UPDATE_CHUNKS)
    {
        TaskUpdateChunk* t = (TaskUpdateChunk*)_task;
        if(s_FCMSN->isRenderChunk(t->s_WorldId, t->s_VectorChunk)) requestBuildChunk(t->s_WorldId, t->s_VectorChunk, true);
    }
    else if(_task->s_TypeTask == DELETE_CHUNKS)
    {
        TaskDeleteChunks* t = (TaskDeleteChunks*)_task;
        if(!deleteChunksForRenderingThreadUnload(t->s_WorldId, t->s_VectorLeftDown, t->s_VectorRightUp, t->s_Unload, t->s_Save, t->s_UnloadFrame, t->s_SizeFrame))
        {
            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error unload chunks (deleteChunksForRendering).");
        }
    }
    else if(_task->s_TypeTask == SET_SIMPLE_VOXEL_OBJECT)
    {
        TaskSetSimpleVoxelObject* t = (TaskSetSimpleVoxelObject*)_task;
        SimpleVoxelObject* svo = t->s_SimpleVoxelObject;
        t->s_SimpleVoxelObject = 0; //Объект теперь принадлежит миру (или удалён внутри при ошибке).
        //Изменено: общее сообщение "function returned false" убрано (причину уже пишет функция или ядро - это был
        //дубль), передаётся s_RequiredPartition.
        setSimpleVoxelObjectThread(t->s_WorldId, t->s_VoxelAddress, svo, t->s_Rewrite, t->s_Immediatelyupdated, t->s_RequiredPartition);
    }
    else if(_task->s_TypeTask == DELETE_SIMPLE_VOXEL_OBJECT)
    {
        TaskDeleteSimpleVoxelObject* t = (TaskDeleteSimpleVoxelObject*)_task;
        //Изменено: передаётся s_RequiredPartition; без сообщения "function returned false" - причину уже пишет ядро.
        deleteSimpleVoxelObjectThread(t->s_WorldId, t->s_VoxelAddress, t->s_Immediatelyupdated, t->s_RequiredPartition);
    }
    else if(_task->s_TypeTask == ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE)
    {
        TaskAddTasksChunksForUpdate* t = (TaskAddTasksChunksForUpdate*)_task;
        if(t->s_All) s_FCMSN->updateAllChunks();
        else s_FCMSN->updateChunks();
    }
    else if(_task->s_TypeTask == RUN_THREAD_FUNCTION_WITH_PARAMETERS)
    {
        TaskRunThreadFunctionWithParameters* t = (TaskRunThreadFunctionWithParameters*)_task;
        UserControllerForThread* controller = s_UserControllerForThread.load();
        if(controller) controller->userThreadFunction(t->s_Parameters);
        else registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error call user controller for thread: user controller for thread is not setted.");
    }
    else if(_task->s_TypeTask == SET_USER_CONTROLLER_FOR_THREAD)
    {
        //Добавлено: замена контроллера между заданиями - старым контроллером сейчас никто не пользуется.
        TaskSetUserControllerForThread* t = (TaskSetUserControllerForThread*)_task;
        UserControllerForThread* old = s_UserControllerForThread.load();
        const bool deleteold = s_DeleteUserControllerForThread;
        s_UserControllerForThread = t->s_UserControllerForThread;
        s_DeleteUserControllerForThread = t->s_DeleteByEngine;
        if(old && old != t->s_UserControllerForThread && deleteold) delete old; //Контроллер пользователя не удаляем.
        t->s_UserControllerForThread = 0; //Контроллер установлен - deleteTask() не должен его удалять.
    }
    else if(_task->s_TypeTask == SAVE_CHUNKS)
    {
        //Добавлено: сохранение мира во время работы (saveAllChunks()).
        TaskSaveChunks* t = (TaskSaveChunks*)_task;
        finishSave(t->s_SaveNumber, saveAllChunksThread(t->s_WorldId));
        t->isCompleted = true; //deleteTask() не должен сообщать об ошибке.
    }
    else if(_task->s_TypeTask == STREAM_CHUNKS) processStreamTask((TaskStreamChunks*)_task); //Добавлено.
}

//Добавлено: действия пула стриминга. Те же шаги, что у ADD_CHUNKS и DELETE_CHUNKS, но по одному чанку.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::processStreamTask(TaskStreamChunks* task)
{
    static const Vector3d<int> around[6] = { Vector3d<int>(1,0,0), Vector3d<int>(-1,0,0), Vector3d<int>(0,1,0), Vector3d<int>(0,-1,0), Vector3d<int>(0,0,1), Vector3d<int>(0,0,-1) };
    std::vector<STREAM_CHUNK_RESULT> results(task->s_Actions.size(), STREAM_CHUNK_FAILED);
    std::vector< std::pair< int, Vector3d<int> > > builds; //Новые чанки в отрисовке - на построение
    std::set< std::pair< int, Vector3d<int> > > rebuilds; //Отрисованные соседи загруженных и выгруженных чанков - на перестройку
    for(size_t i = 0; i < task->s_Actions.size(); i++)
    {
        const StreamChunkAction& a = task->s_Actions[i];
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine); //На каждый чанк - чтобы остальные не ждали всё задание
        World* wrld = getWorld(a.s_WorldId);
        if(!wrld || !s_FCMSN) continue;
        const bool inmem = wrld->isExistsChunkInMemory(a.s_Vector);
        const bool inrender = s_FCMSN->isRenderChunk(a.s_WorldId, a.s_Vector);
        bool neighbours = false;
        if(a.s_Action == STREAM_CHUNK_LOAD)
        {
            if(inmem) results[i] = STREAM_CHUNK_ALREADY;
            else if(wrld->isChunkInBounds(a.s_Vector) && wrld->loadChunk(a.s_Vector)) { results[i] = STREAM_CHUNK_DONE; neighbours = true; }
        }
        else if(a.s_Action == STREAM_CHUNK_RENDER)
        {
            if(inrender) results[i] = STREAM_CHUNK_ALREADY;
            else if(inmem)
            {
                s_FCMSN->registerChunkForRendering(a.s_WorldId, a.s_Vector);
                builds.push_back(std::make_pair(a.s_WorldId, a.s_Vector));
                results[i] = STREAM_CHUNK_DONE;
            }
        }
        else if(a.s_Action == STREAM_CHUNK_UNRENDER)
        {
            if(!inrender) results[i] = STREAM_CHUNK_ALREADY;
            else if(s_FCMSN->deleteChunk(a.s_WorldId, a.s_Vector)) results[i] = STREAM_CHUNK_DONE;
        }
        else if(a.s_Action == STREAM_CHUNK_UNLOAD)
        {
            if(!inmem) results[i] = STREAM_CHUNK_ALREADY;
            //Без контроллера сохранения сохранять некуда - выгружаем без сохранения (иначе ядро отказывает в выгрузке).
            else if(!inrender && wrld->unloadChunk(a.s_Vector, a.s_Save && wrld->getChunksSaveLoadController() != 0)) { results[i] = STREAM_CHUNK_DONE; neighbours = true; }
        }
        if(neighbours)
            for(int k = 0; k < 6; k++)
            {
                const Vector3d<int> n = a.s_Vector + around[k];
                if(s_FCMSN->isRenderChunk(a.s_WorldId, n)) rebuilds.insert(std::make_pair(a.s_WorldId, n));
            }
    }
    for(size_t i = 0; i < builds.size(); i++) requestBuildChunk(builds[i].first, builds[i].second, false);
    for(std::set< std::pair< int, Vector3d<int> > >::iterator iter = rebuilds.begin(); iter != rebuilds.end(); ++iter)
        requestBuildChunk(iter->first, iter->second, true);
    for(size_t i = 0; i < task->s_Actions.size(); i++)
        s_ChunkStreamingPool->onActionDone(task->s_Actions[i].s_WorldId, task->s_Actions[i].s_Vector, task->s_Actions[i].s_Action, results[i]);
    task->isCompleted = true;
}

ChunkStreamingPool* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getChunkStreamingPool()
{
    return s_ChunkStreamingPool;
}

//Добавлено: блокировка вывода лога - во враппере, а не в ядре (ядро однопоточное). В лог пишут все потоки движка: ядро -
//через Engine::registerEvent(), который здесь переопределён. Сообщения, которые логгер не выводит, - без блокировки
//(иначе на каждом отключённом сообщении потоки выстраивались бы в очередь).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::registerEvent(EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    if(!isEventPrinted(TypeEvent)) return;
    lock_guard<mutex> locker(s_LockLog);
    Engine::registerEvent(TypeEvent, TextEvent, IsFatal);
}

//Добавлено: сохранение всех чанков мира во время работы движка - отдельным заданием главной очереди.
unsigned long SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::saveAllChunks(int worldid, bool wait)
{
    const unsigned long number = ++s_LastSaveNumber;
    {
        lock_guard<mutex> locker(s_LockSaves);
        s_Saves[number] = 0;
        //Помним не больше 1000 законченных сохранений: старые законченные забываются.
        if(s_Saves.size() > 1000)
            for(std::map<unsigned long, int>::iterator iter = s_Saves.begin(); iter != s_Saves.end() && s_Saves.size() > 1000;)
            {
                if(iter->second != 0) s_Saves.erase(iter++);
                else ++iter;
            }
    }
    //Из пользовательской функции (поток очереди) задание ждало бы само себя, а неработающий движок (или поток очереди,
    //уже взявший STOP) задание не выполнил бы - в этих случаях сохраняем сразу (под блокировкой движка).
    //Исправлено: проверка и постановка задания - под одной блокировкой очереди. Раньше между проверкой
    //"движок не останавливается" и постановкой задания stop() мог поставить STOP раньше него - задание не выполнялось
    //никогда, и saveAllChunks(id, true) зависал. А если stop() уже был вызван, сохранение выполнялось сразу - раньше
    //заданий, поставленных до вызова (они в сохранение не попадали). Теперь задание ставится перед STOP.
    bool queued = false;
    if(!isMainQueueThread())
    {
        TaskSaveChunks* task = new TaskSaveChunks;
        task->s_TypeTask = SAVE_CHUNKS;
        task->isCompleted = false;
        task->s_WorldId = worldid;
        task->s_SaveNumber = number;
        {
            lock_guard<mutex> locker(s_LockMainQueue);
            if(s_MainQueueAlive)
            {
                std::deque< Task* >::iterator pos = s_TaskMainQueue.end();
                //STOP в очереди один и стоит в конце или почти в конце (задания после stop() не выполняются) - ищем с конца.
                if(s_StopRequested)
                    for(std::deque< Task* >::iterator iter = s_TaskMainQueue.end(); iter != s_TaskMainQueue.begin();)
                    {
                        --iter;
                        if((*iter)->s_TypeTask == STOP)
                        {
                            pos = iter;
                            break;
                        }
                    }
                s_TaskMainQueue.insert(pos, task);
                queued = true;
            }
        }
        //Изменено: без notify_one() (см. addMainTask()) - поток очереди увидит задание сам.
        if(!queued) delete task; //Не поставлено - сохраняем сразу (ниже); deleteTask() записал бы этому сохранению ошибку.
    }
    if(!queued)
    {
        finishSave(number, saveAllChunksThread(worldid));
        return number;
    }
    if(wait)
    {
        //Ожидание с таймаутом: notify_one() и notify_all() в winpthreads из MinGW 8.1 иногда зависают (см. s_CheckAddChunks),
        //поэтому ждущих никто не будит - каждый проверяет своё сохранение по таймауту.
        unique_lock<mutex> locker(s_LockSaves);
        while(true)
        {
            std::map<unsigned long, int>::iterator iter = s_Saves.find(number);
            if(iter == s_Saves.end() || iter->second != 0) break;
            s_CheckSaves.wait_for(locker, std::chrono::milliseconds(10));
        }
    }
    return number;
}

//Добавлено: проверка окончания сохранения по его номеру.
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::isSaveCompleted(unsigned long savenumber, bool* result)
{
    lock_guard<mutex> locker(s_LockSaves);
    std::map<unsigned long, int>::iterator iter = s_Saves.find(savenumber);
    if(iter == s_Saves.end() || iter->second == 0) return false;
    if(result) *result = (iter->second == 1);
    return true;
}

//Добавлено: само сохранение - под блокировкой движка (потоки построения пишут кэш видимости, который читает контроллер).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::saveAllChunksThread(int worldid)
{
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(worldid);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error saving chunks: world does not exist.");
        return false;
    }
    return wrld->saveAllChunks();
}

//Добавлено: результат сохранения для isSaveCompleted() и ждущих saveAllChunks(id, true).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::finishSave(unsigned long savenumber, bool result)
{
    {
        lock_guard<mutex> locker(s_LockSaves);
        s_Saves[savenumber] = result ? 1 : 2;
    }
    //Изменено: без notify_one() - ждущие проверяют результат по таймауту (см. saveAllChunks()).
}

//Поток главной очереди: берёт задания строго по порядку. После STOP останавливает и потоки построения.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadMainQueueTaskController()
{
    while(true)
    {
        Task* task = 0;
        {
            unique_lock<mutex> locker(s_LockMainQueue);
            //Изменено: ожидание с таймаутом вместо пробуждения через notify_one() (он в winpthreads из MinGW 8.1 иногда
            //зависает навсегда): пока очередь пуста, поток проверяет её раз в THREAD_WAIT_INTERVAL_IN_MILLISECONDS.
            while(s_TaskMainQueue.empty()) s_CheckMainQueue.wait_for(locker, std::chrono::milliseconds(THREAD_WAIT_INTERVAL_IN_MILLISECONDS));
            task = s_TaskMainQueue.front();
            s_TaskMainQueue.pop_front();
            //Добавлено: после STOP задания очереди больше не выполняются - saveAllChunks() видит это под этой же
            //блокировкой и сохраняет сам, сразу.
            if(task->s_TypeTask == STOP) s_MainQueueAlive = false;
        }
        if(task->s_TypeTask == STOP)
        {
            deleteTask(task);
            break;
        }
        processMainTask(task);
        deleteTask(task);
    }
    {
        lock_guard<mutex> locker(s_LockBuildQueues);
        s_StopBuildThreads = true;
    }
    //Изменено: без notify_one() - потоки построения проверяют флаг остановки сами, раз в THREAD_WAIT_INTERVAL_IN_MILLISECONDS.
    //Добавлено: сбрасываем id потока очереди - иначе новый поток с тем же id считался бы потоком очереди.
    s_IdThreadMainQueue = std::thread::id();
    s_NumberOfRunningThreads--;
}

//Общий цикл потоков построения. Поток пула обновления сначала берёт изменённые чанки, а когда их нет -
//помогает строить новые. Сам меш строит фабрика (buildChunk), версия чанка не даёт старому мешу перетереть новый.
//Раньше построение запускалось как std::thread(...).detach() с заранее взятым указателем на чанк, который
//к тому времени мог быть уже выгружен, и потоки завершались в произвольном порядке.
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadBuildChunksController(bool updatepool)
{
#ifdef SVE_DEBUG_CHECKS
    t_BuildThreadEngine = this; //Добавлено: для отладочной проверки stop() из потока построения.
#endif
    while(true)
    {
        BuildJob job;
        {
            unique_lock<mutex> locker(s_LockBuildQueues);
            condition_variable& check = updatepool ? s_CheckUpdateChunks : s_CheckAddChunks;
            //Изменено: ожидание с таймаутом вместо пробуждения через notify_one() (он в winpthreads из MinGW 8.1 иногда
            //зависает навсегда): пока работы нет, поток проверяет очереди раз в THREAD_WAIT_INTERVAL_IN_MILLISECONDS.
            while(!(s_StopBuildThreads || !s_QueueAddChunks.empty() || (updatepool && !s_QueueUpdateChunks.empty())))
                check.wait_for(locker, std::chrono::milliseconds(THREAD_WAIT_INTERVAL_IN_MILLISECONDS));
            if(s_StopBuildThreads) break;
            if(updatepool && !s_QueueUpdateChunks.empty())
            {
                job = s_QueueUpdateChunks.front();
                s_QueueUpdateChunks.pop_front();
                s_PendingUpdateChunks.erase(std::make_pair(job.s_WorldId, job.s_VectorChunk));
            }
            else
            {
                job = s_QueueAddChunks.front();
                s_QueueAddChunks.pop_front();
                s_PendingAddChunks.erase(std::make_pair(job.s_WorldId, job.s_VectorChunk));
            }
        }
        s_FCMSN->buildChunk(job.s_WorldId, job.s_VectorChunk);
    }
    s_NumberOfRunningThreads--;
}

//Исправлено: раньше runIrrDevice() держал мьютекс таймера всё время device->run(), а внутри run() Irrlicht вызывает
//OnEvent() обработчика событий. Вызов getTimeSafeThread() из OnEvent() блокировал поток сам с собой, а вместе с
//блокировкой движка в другом потоке давал взаимную блокировку. Теперь время запоминается после каждого run(),
//и getTimeSafeThread() просто читает его (из любого потока, без блокировок). Точность - один кадр.
u32 SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getTimeSafeThread()
{
    return s_TimeFromDevice;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::runIrrDevice()
{
    bool ret = s_Device->run();
    s_TimeFromDevice = s_Device->getTimer()->getTime();
    return ret;
}

//Движок работает, пока жив хотя бы один его поток (счётчик уменьшает каждый поток при выходе).
bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::isRun()
{
    return s_NumberOfRunningThreads > 0;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setMaxNumberOfThreadAddChunks(unsigned int newvaluemaxnumberofthread)
{
    if(newvaluemaxnumberofthread < 1)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread add chunks: newvalue < 1.");
        return false;
    }
    if(isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread add chunks: IrrSVE is run.");
        return false;
    }
    s_MaxNumberOfThreadAddChunks = newvaluemaxnumberofthread;
    return true;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getMaxNumberOfThreadAddChunks()
{
    return s_MaxNumberOfThreadAddChunks;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setMaxNumberOfThreadUpdateChunks(unsigned int newvaluemaxnumberofthread)
{
    if(newvaluemaxnumberofthread < 1)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread update chunks: newvalue < 1.");
        return false;
    }
    if(isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread update chunks: IrrSVE is run.");
        return false;
    }
    s_MaxNumberOfThreadUpdateChunks = newvaluemaxnumberofthread;
    return true;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getMaxNumberOfThreadUpdateChunks()
{
    return s_MaxNumberOfThreadUpdateChunks;
}

//Добавлены проверка фабрики (без неё раньше было разыменование NULL) и блокировка движка: потоки
//построения в это время могут читать карту миров.
int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(WorldCreatingParameters& wcp, World3DInfo w3dinfo)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckMainThread("addWorld")) return -1; //Отладочная проверка (добавлено): только главный поток.
#endif
    //Добавлено: только главный поток - мир регистрируется и в фабрике, а её карты читает главный поток без блокировки.
    if(isMainQueueThread())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world: addWorld() must be called from the main thread.");
        return -1;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world: factory chunk mesh scene node is not setted.");
        return -1;
    }
    int id = -1;
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        id = SimpleVoxelEngine::Core::Engine::addWorld(wcp);
    }
    if(id >= 0)
    {
        s_FCMSN->addWorld(id, w3dinfo);
        updateWorldTransform(id); //Добавлено: преобразование мира для getWorldCoordinates() из других потоков.
    }
    return id;
}

int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(World3DInfo w3dinfo)
{
    WorldCreatingParameters wcp;
    return addWorld(wcp, w3dinfo);
}

int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(World* wrld, World3DInfo w3dinfo)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckMainThread("addWorld")) return -1; //Отладочная проверка (добавлено): только главный поток.
#endif
    if(isMainQueueThread()) //Добавлено: только главный поток (см. addWorld(wcp, w3dinfo)).
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world: addWorld() must be called from the main thread.");
        return -1;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world: factory chunk mesh scene node is not setted.");
        return -1;
    }
    int id = -1;
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        id = SimpleVoxelEngine::Core::Engine::addWorld(wrld);
    }
    if(id >= 0)
    {
        s_FCMSN->addWorld(id, w3dinfo);
        updateWorldTransform(id); //Добавлено: преобразование мира для getWorldCoordinates() из других потоков.
    }
    return id;
}

//Адрес вокселя (чанк + координаты в чанке) по глобальным координатам. Раньше этот расчёт был продублирован
//в двух функциях циклами while(x % sizechunk != 0) x--; - результат тот же, но теперь одной формулой.
VoxelAddress* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::createVoxelAddressByGlobalVector(Vector3d<int> vect, int sizechunk)
{
    Vector3d<int> vectchunk(floorDivide(vect.getX(), sizechunk), floorDivide(vect.getY(), sizechunk), floorDivide(vect.getZ(), sizechunk));
    VoxelAddress* vxladdr = new VoxelAddress(this);
    vxladdr->addNextVector(vectchunk);
    vxladdr->addNextVector(vect - vectchunk * sizechunk);
    return vxladdr;
}

//Раньше при несуществующем мире было разыменование NULL, а переданный воксель при ошибке не удалялся.
//Размер чанка читается под блокировкой, сама установка идёт через setSimpleVoxelObject() (сразу или через очередь).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObjectByGlobalVector(int worldid, Vector3d<int> vect, SimpleVoxelObject* svo, int level)
{
    if(level != 1)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Error using level in setting SVO: level != 1 not supported.");
        if(svo) delete svo;
        return;
    }
    int sizechunk = 0;
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        World* wrld = getWorld(worldid);
        if(wrld) sizechunk = wrld->getSizeChunk();
    }
    if(sizechunk == 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting SVO by global vector: world does not exist.");
        if(svo) delete svo;
        return;
    }
    VoxelAddress* vxladdr = createVoxelAddressByGlobalVector(vect, sizechunk);
    //Изменено: при выполнении проверяется, что разбиение чанка равно getSizeChunk(), - иначе глобальный вектор
    //указывал бы не на ту ячейку (раньше так и менялась чужая ячейка, а вектор вне чанка писал в ячейку 0).
    if(svo) setSimpleVoxelObjectAnyThread(worldid, vxladdr, svo, true, false, sizechunk);
    else deleteSimpleVoxelObjectAnyThread(worldid, vxladdr, false, sizechunk);
}

//Вся функция под блокировкой движка (раньше блокировка снималась между получением мира и чтением вокселя,
//а из пользовательского потока не бралась совсем). Добавлены проверки мира и того, что чанк загружен.
SimpleVoxelObject* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getSimpleVoxelObjectByGlobalVector(int worldid, Vector3d<int> vect, bool isexists, int level)
{
    if(level != 1)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Error using level in getting SVO: level != 1 not supported.");
        return 0;
    }
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(worldid);
    if(!wrld) return 0;
    VoxelAddress* vxladdr = createVoxelAddressByGlobalVector(vect, wrld->getSizeChunk());
    SimpleVoxelObject* svo = 0;
    ChunkVoxelObject* chunk = wrld->getChunk(vxladdr->getVector(0));
    //Исправлено: в чанке с разбиением не getSizeChunk() глобальный вектор указывал бы не на ту ячейку.
    if(chunk && (int)chunk->getNumberOfPartitions() != wrld->getSizeChunk())
    {
        if(isexists) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error getting SVO by global vector: the partition of the chunk differs from getSizeChunk() of the world.");
    }
    else if(chunk) svo = wrld->getSimpleVoxelObject(vxladdr, isexists);
    else if(isexists) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error getting SVO by global vector: chunk is not loaded.");
    delete vxladdr;
    return svo;
}

//! Притягивание координаты к узлу сетки для калибровки.
/*! Координата притягивается к целому узлу, только если она и так почти на нём (ближе 0.01): вершины граней вложенных
вокселей (например, x = 3.5 или 3.25) не должны уезжать на узел сетки вокселей первого уровня - иначе находился бы
не тот воксель.
\param value Координата.
\return Возвращает узел сетки или исходную координату.
*/
static float snapToLattice(float value)
{
    const float rounded = floorf(value + 0.5f);
    return fabsf(value - rounded) < 0.01f ? rounded : value;
}

//Пересчёт точек из координат сцены в координаты мира не менялся. Изменено: проверки фабрики и мира
//(раньше - разыменование NULL) и блокировка движка всегда, а не только вне потоков очередей.
VoxelAddress* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAddressSimpleVoxelObjectByThreePoints(int id, Vector3d<float> v1, Vector3d<float> v2, Vector3d<float> v3, Vector3d<float> lookvect, Vector3d<float> changeposition, bool calibration, LevelSearchSimpleVoxelObject lssvo, int level)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): без фабрики функция молча возвращает NULL.
    if(!s_FCMSN) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getAddressSimpleVoxelObjectByThreePoints: factory chunk mesh scene node is not set.");
#endif
    if(!s_FCMSN) return 0;
    //Изменено: пересчёт в координаты мира вынесен в getWorldCoordinates() (тот же самый).
    Vector3d<float> nv1 = getWorldCoordinates(id, v1), nv2 = getWorldCoordinates(id, v2), nv3 = getWorldCoordinates(id, v3);
    Vector3d<float> nlookvect = getWorldCoordinates(id, changeposition + lookvect) - getWorldCoordinates(id, changeposition);
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(id);
#ifdef SVE_DEBUG_CHECKS
    if(!wrld) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getAddressSimpleVoxelObjectByThreePoints: world does not exist."); //Отладочная проверка (добавлено).
#endif
    if(!wrld) return 0;
    if(calibration)
    {
        float latticeStep = (s_FCMSN->getPositionScale() + s_FCMSN->getSScale()) / (2 * wrld->getSizeChunk()) - 0.5;
        Vector3d<float> lStep(latticeStep, latticeStep, latticeStep);
        nv1 -= lStep;
        nv2 -= lStep;
        nv3 -= lStep;
        nv1 = Vector3d<float>( snapToLattice(nv1.getX()), snapToLattice(nv1.getY()), snapToLattice(nv1.getZ()) );
        nv2 = Vector3d<float>( snapToLattice(nv2.getX()), snapToLattice(nv2.getY()), snapToLattice(nv2.getZ()) );
        nv3 = Vector3d<float>( snapToLattice(nv3.getX()), snapToLattice(nv3.getY()), snapToLattice(nv3.getZ()) );
        nv1 += lStep;
        nv2 += lStep;
        nv3 += lStep;
    }
    return wrld->getAddressSimpleVoxelObjectByThreePoints(nv1, nv2, nv3, s_FCMSN->getSScale(), nlookvect, lssvo, level);
}

//Добавлено: пересчёт точки сцены в координаты мира - вынесен из getAddressSimpleVoxelObjectByThreePoints() без изменений.
Vector3d<float> SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getWorldCoordinates(int worldid, Vector3d<float> scenepoint)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): без фабрики или для несуществующего мира точка молча возвращается без пересчёта,
    //и дальше всё считается в неверных координатах.
    if(!s_FCMSN) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getWorldCoordinates: factory chunk mesh scene node is not set, the point is not converted.");
    else
    {
        lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
        if(!getWorld(worldid)) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getWorldCoordinates: world does not exist, the point is not converted.");
    }
#endif
    if(!s_FCMSN) return scenepoint;
    //Исправлено. Раньше обратное преобразование собиралось из позиции, поворота и масштаба ноды в неверном
    //порядке (при масштабе не 1 точка пересчитывалась неверно), родители ноды не учитывались, без родительской ноды не
    //вычитался сдвиг начала координат (а ноды чанков сдвинуты на него и без родителя), и ноды сцены читались из любого
    //потока, пока главный поток мог их менять. Теперь - обратная матрица абсолютного преобразования ноды: в главном
    //потоке по текущему положению нод, в остальных - запомненная главным потоком (update(), addWorld(), changeWorld3DInfo()).
    WorldTransform transform;
    if(isMainThread()) calculateWorldTransform(worldid, transform);
    else
    {
        lock_guard<mutex> locker(s_LockWorldTransforms);
        std::map<int, WorldTransform>::iterator iter = s_WorldTransforms.find(worldid);
        if(iter == s_WorldTransforms.end()) return scenepoint;
        transform = iter->second;
    }
    vector3df p(scenepoint.getX(), scenepoint.getY(), scenepoint.getZ());
    if(transform.s_HasRoot) transform.s_Inverse.transformVect(p);
    p -= transform.s_Amendment;
    return Vector3d<float>(p.X, p.Y, p.Z);
}

//Добавлено: преобразование мира по текущему положению нод (только главный поток).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::calculateWorldTransform(int worldid, WorldTransform& transform)
{
    World3DInfo w3dinfo = s_FCMSN ? s_FCMSN->getWorld3DInfo(worldid) : World3DInfo();
    transform.s_HasRoot = (w3dinfo.s_WorldRootNode != 0);
    transform.s_Amendment = w3dinfo.s_AmendmentPositionWorld;
    transform.s_Inverse.makeIdentity();
    if(!w3dinfo.s_WorldRootNode) return;
    //Абсолютное преобразование считается по относительным преобразованиям ноды и всех её родителей, а не по
    //getAbsoluteTransformation(): та обновляется только при отрисовке, и после сдвига ноды в этом кадре была бы старой.
    matrix4 absolute = w3dinfo.s_WorldRootNode->getRelativeTransformation();
    for(ISceneNode* parent = w3dinfo.s_WorldRootNode->getParent(); parent; parent = parent->getParent()) absolute = parent->getRelativeTransformation() * absolute;
    if(!absolute.getInverse(transform.s_Inverse)) transform.s_Inverse.makeIdentity(); //Вырожденная матрица (масштаб 0) - без пересчёта.
}

//Добавлено: запомнить преобразование мира для других потоков (только главный поток).
void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::updateWorldTransform(int worldid)
{
    WorldTransform transform;
    calculateWorldTransform(worldid, transform);
    lock_guard<mutex> locker(s_LockWorldTransforms);
    s_WorldTransforms[worldid] = transform;
}

//Добавлено: адрес вокселя под точкой (например, под курсором) - точное место на грани, а не центр треугольника.
VoxelAddress* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAddressSimpleVoxelObjectByPoint(int id, Vector3d<float> point, Vector3d<float> lookvect, Vector3d<float> changeposition, LevelSearchSimpleVoxelObject lssvo, int level)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): без фабрики функция молча возвращает NULL.
    if(!s_FCMSN) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getAddressSimpleVoxelObjectByPoint: factory chunk mesh scene node is not set.");
#endif
    if(!s_FCMSN) return 0;
    Vector3d<float> npoint = getWorldCoordinates(id, point);
    Vector3d<float> nlookvect = getWorldCoordinates(id, changeposition + lookvect) - getWorldCoordinates(id, changeposition);
    lock_guard<recursive_mutex> locker(s_LockAccessToTheSimpleVoxelEngine);
    World* wrld = getWorld(id);
#ifdef SVE_DEBUG_CHECKS
    if(!wrld) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): getAddressSimpleVoxelObjectByPoint: world does not exist."); //Отладочная проверка (добавлено).
#endif
    if(!wrld) return 0;
    const float length = sqrt(nlookvect.getSquareNorm());
    if(length <= 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error getting address SVO by point: look vector is zero.");
        return 0;
    }
    //Точка лежит на грани - сдвигаем её вдоль взгляда на тысячную долю блока первого уровня, чтобы она
    //оказалась внутри нужной ячейки, а не на границе.
    //Исправлено: далеко от начала координат (примерно с 8000 блоков) тысячная доля блока меньше
    //точности float, и точка не сдвигалась с грани. Теперь шаг не меньше 4 * FLT_EPSILON от координаты точки.
    const float magnitude = std::max(fabs(npoint.getX()), std::max(fabs(npoint.getY()), fabs(npoint.getZ())));
    const float step = std::max(0.001f * s_FCMSN->getSScale() / wrld->getSizeChunk(), magnitude * 4.0f * FLT_EPSILON);
    npoint += nlookvect * (step / length);
    return wrld->getAddressSimpleVoxelObjectByPoint(npoint, s_FCMSN->getSScale(), lssvo, level);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAccessToTheSimpleVoxelEngine()
{
    s_LockAccessToTheSimpleVoxelEngine.lock();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::freeAccessToTheSimpleVoxelEngine()
{
    s_LockAccessToTheSimpleVoxelEngine.unlock();
}

recursive_mutex& SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getLockAccessToTheSimpleVoxelEngine()
{
    return s_LockAccessToTheSimpleVoxelEngine;
}

irr::IrrlichtDevice* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getIrrlichtDevice()
{
    return s_Device;
}

//Удалена getLockGettingTimeFromDevice(): мьютекс таймера больше не нужен (см. getTimeSafeThread()).
