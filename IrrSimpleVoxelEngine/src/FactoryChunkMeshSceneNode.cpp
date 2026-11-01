#include "FactoryChunkMeshSceneNode.h"

#include "IrrSimpleVoxelEngine.h"

#include "IrrSVEDefines.h"

using namespace SimpleVoxelEngine;
using namespace Additional;
using namespace Core;
using namespace Voxels;
using namespace Defines;
using namespace IrrEngine;
using namespace IrrSVEDefines;

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

using namespace std;

/*
    Потокобезопасность фабрики:
    - s_LockWrite защищает s_ChunksMesh (готовые, но ещё не применённые меши) и s_ChunkVersions;
    - s_LockListAllChunksFlagRendering защищает список чанков, зарегистрированных для отрисовки;
    - s_LockChunksMeshSceneNodeForUpdate защищает список чанков, помеченных для обновления;
    - s_LockGetMeshVoxel сериализует вызовы пользовательской getMeshVoxel() из потоков построения;
    - контроллеры блоков (ChunksMeshBlocksController) и Irrlicht-ноды трогаются только в главном потоке.
    Версия чанка: каждый снимок данных чанка (под блокировкой движка) и каждое удаление чанка
    получают новый номер. Готовый меш принимается, только если его номер всё ещё последний,
    поэтому устаревший меш (построенный по старым данным или для уже удалённого чанка) никогда
    не перезапишет более новый.
*/

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device, IrrSimpleVoxelEngine* isve):
    s_ChunkVersionCounter(0),
    s_Device(device),
    s_ISVE(isve),
    s_Freeze(false),
    s_ChunksMesh(0),
    s_PositionScale(1),
    s_RotationScale(1),
    s_SScale(1),
    s_IsFaces(true),
    s_SizeBlockChunksX(0),
    s_SizeBlockChunksY(0),
    s_SizeBlockChunksZ(0),
    s_OcclusionQueries(false),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_TimerOcclusionQueriesInMilliseconds(TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS),
    s_RememberTimeForOcclusionQueriesInMilliseconds(0),
    s_MaxFramesBetweenOcclusionQueries(MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES),
    s_FramesSinceOcclusionQueries(0),
    s_OcclusionQueriesBoundingBox(true),
    s_NumberOfMeshesSceneNodeForChunk(1)
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

//Два конструктора ниже раньше дублировали весь список инициализации (и s_ChunkVersionCounter в нём не было) -
//теперь делегируют основному.
SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device):
    FactoryChunkMeshSceneNode(device, 0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode():
    FactoryChunkMeshSceneNode(0, 0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::~FactoryChunkMeshSceneNode()
{
    clear();
}

//Раньше clear() сначала применял ожидающие меши (updateMeshChunks() - создание нод в деструкторе, в т.ч. не из главного
//потока), а сами ожидающие меши не освобождал (утечка). Теперь: убрать все чанки, освободить неприменённые меши, удалить контроллеры.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::clear()
{
    deleteAllChunks();
    {
        //Меши, которые ещё не применены, просто освобождаем.
        lock_guard<recursive_mutex> locker(s_LockWrite);
        if(s_ChunksMesh)
        {
            for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
            {
                std::map< int, std::map< Vector3d<int>, IMesh* > >::iterator iter;
                for(iter = s_ChunksMesh[i].begin(); iter != s_ChunksMesh[i].end(); ++iter)
                {
                    std::map< Vector3d<int>, IMesh* >::iterator iter2;
                    for(iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
                        if(iter2->second) iter2->second->drop();
                }
            }
            delete[] s_ChunksMesh;
        }
        s_ChunksMesh = 0;
        s_ChunkVersions.clear();
        s_ChunksWaitingFirstMesh.clear();
        //Добавлено: уровней больше нет - иначе функции уровней (setMaterialType() и др.) писали бы в удалённые массивы.
        s_NumberOfMeshesSceneNodeForChunk = 0;
    }
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    s_HardwareMappingHint = 0;
    s_BufferType = 0;
    s_MaterialType = 0;
    std::map< int, ChunksMeshBlocksController* >::iterator iter;
    for(iter = s_ChunksMeshBlocksControllers.begin(); iter != s_ChunksMeshBlocksControllers.end(); ++iter)
        if(iter->second) delete iter->second;
    s_ChunksMeshBlocksControllers.clear();
    //Изменено: под блокировкой, ссылки на родительские ноды миров отпускаются.
    lock_guard<mutex> rootlocker(s_LockWorldRootNodes);
    for(std::map<int, World3DInfo>::iterator iterroot = s_WorldRootNodes.begin(); iterroot != s_WorldRootNodes.end(); ++iterroot)
        if(iterroot->second.s_WorldRootNode) iterroot->second.s_WorldRootNode->drop();
    s_WorldRootNodes.clear();
}

//Добавлено: все вызовы s_ISVE->registerEvent() в этом файле заменены на неё - фабрика, ещё не отданная
//движку (s_ISVE == NULL), в ветках ошибок (например, при повторном setIrrlichtDevice()) падала на разыменовании NULL.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::registerFactoryEvent(EventType type, const std::string& text, bool fatal)
{
    if(s_ISVE) s_ISVE->registerEvent(type, text, fatal);
}

//Новая функция: все обращения к s_ChunksMeshBlocksControllers[id] заменены на неё.
ChunksMeshBlocksController* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getChunksMeshBlocksController(int id)
{
    //Не operator[]: он вставил бы в карту NULL-контроллер, на котором потом упал бы update().
    std::map< int, ChunksMeshBlocksController* >::iterator iter = s_ChunksMeshBlocksControllers.find(id);
    if(iter == s_ChunksMeshBlocksControllers.end()) return 0;
    return iter->second;
}

//Новая функция: версия чанка (см. описание в начале файла).
unsigned long SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::newChunkVersion(int id, Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockWrite);
    s_ChunkVersionCounter++;
    s_ChunkVersions[id][vect] = s_ChunkVersionCounter;
    return s_ChunkVersionCounter;
}

//Новая функция. Чанк регистрируется "отрисовываемым" в потоке главной очереди ДО постановки на построение.
//Раньше это делал updateMeshChunk() в главном потоке уже после построения, и проверки isRenderChunk() в других
//потоках видели чанк то отрисовываемым, то нет.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::registerChunkForRendering(int id, Vector3d<int> vect)
{
    //Исправлено: под блокировкой движка, как и deleteChunk() - иначе удаление в главном потоке могло вклиниться
    //между пометкой "отрисовывается" и пометкой "ждёт первого меша", и чанк навсегда оставался "ждущим".
    lock_guard<recursive_mutex> enginelocker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        s_ListAllChunksFlagRendering[id][vect] = true;
    }
#ifdef SVE_DEBUG_CHECKS
    {
        //Добавлено (отладочная сборка): чанк снова отрисовывается - запись "убран из отрисовки" больше не нужна.
        std::map< int, std::map< Vector3d<int>, unsigned int > >::iterator recent = s_DebugRecentlyUnrendered.find(id);
        if(recent != s_DebugRecentlyUnrendered.end()) recent->second.erase(vect);
    }
#endif
    //Пока чанк не построен, склейка его блока откладывается (см. ChunksMeshBlocksController::update()).
    lock_guard<recursive_mutex> locker(s_LockWrite);
    s_ChunksWaitingFirstMesh[id].insert(vect);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::finishWaitingFirstMesh(int id, Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockWrite);
    std::map< int, std::set< Vector3d<int> > >::iterator iter = s_ChunksWaitingFirstMesh.find(id);
    if(iter != s_ChunksWaitingFirstMesh.end()) iter->second.erase(vect);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isChunkWaitingForMesh(int id, Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockWrite);
    std::map< int, std::set< Vector3d<int> > >::iterator iter = s_ChunksWaitingFirstMesh.find(id);
    if(iter != s_ChunksWaitingFirstMesh.end() && iter->second.find(vect) != iter->second.end()) return true;
    //Меш уже построен, но главный поток его ещё не забрал (готов после applyPendingMeshes() этого кадра).
    if(!s_ChunksMesh) return false;
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        std::map< int, std::map< Vector3d<int>, IMesh* > >::iterator iterworld = s_ChunksMesh[i].find(id);
        if(iterworld != s_ChunksMesh[i].end() && iterworld->second.find(vect) != iterworld->second.end()) return true;
    }
    return false;
}

//Новая функция: построение меша чанка в потоке построения.
//1) Под блокировкой движка: проверка, что чанк ещё отрисовывается, снимок его данных, новая версия.
//2) Без блокировок: построение меша (самая долгая часть, идёт параллельно в нескольких потоках).
//3) Под s_LockWrite: меш принимается, только если версия чанка всё ещё та же (иначе он устарел).
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::buildChunk(int id, Vector3d<int> vect)
{
    VoxelsForRendering* vxlfr = 0;
    unsigned long version = 0;
    {
        //Снимок данных чанка. Пока держится блокировка движка, мир никто не меняет.
        lock_guard<recursive_mutex> locker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
        if(!isRenderChunk(id, vect))
        {
            finishWaitingFirstMesh(id, vect); //Исправлено: иначе чанк мог навсегда остаться "ждущим" (склейка блока ждала бы таймаут).
            return false; //Чанк успели убрать из отрисовки.
        }
        World* wrld = s_ISVE->getWorld(id);
        if(!wrld)
        {
            finishWaitingFirstMesh(id, vect);
            return false;
        }
        ChunkVoxelObject* cvo = wrld->getChunk(vect);
        if(cvo) vxlfr = cvo->getVoxelsForRenderingStatic(getPositionScale(), getRotationScale(), getSScale(), s_IsFaces);
        else vxlfr = new VoxelsForRendering(); //Чанка нет в памяти - рисовать нечего.
        version = newChunkVersion(id, vect);
    }
    //При ошибке меш не появится, поэтому чанк больше не "ждёт" - иначе склейка его блока откладывалась бы до таймаута.
    if(!vxlfr)
    {
        registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error getting voxels for rendering.", true);
        finishWaitingFirstMesh(id, vect);
        return false;
    }
    //Построение меша идёт без блокировки движка, параллельно в нескольких потоках.
    int levels = 0; //Сколько уровней в bm - по нему, а не по текущему s_NumberOfMeshesSceneNodeForChunk, освобождается bm.
    CBatchingMesh** bm = generateIMeshForSceneNode(vxlfr, levels);
    delete vxlfr;
    if(!bm)
    {
        registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error generate IMesh for scene node.", true);
        finishWaitingFirstMesh(id, vect);
        return false;
    }
    bool accepted = false;
    {
        lock_guard<recursive_mutex> locker(s_LockWrite);
        std::map< int, std::map< Vector3d<int>, unsigned long > >::iterator iterworld = s_ChunkVersions.find(id);
        if(iterworld != s_ChunkVersions.end())
        {
            std::map< Vector3d<int>, unsigned long >::iterator iterchunk = iterworld->second.find(vect);
            accepted = (iterchunk != iterworld->second.end() && iterchunk->second == version);
        }
        //Исправлено: меш не принимается, если фабрику уже очистили (s_ChunksMesh == NULL - было разыменование NULL)
        //или число уровней изменилось за время построения (индексация bm за пределами массива).
        if(accepted && (!s_ChunksMesh || levels != s_NumberOfMeshesSceneNodeForChunk)) accepted = false;
        if(accepted)
        {
            for(int i = 0; i < levels; i++)
            {
                std::map< Vector3d<int>, IMesh* >& pending = s_ChunksMesh[i][id];
                std::map< Vector3d<int>, IMesh* >::iterator iter = pending.find(vect);
                if(iter != pending.end() && iter->second) iter->second->drop();
                pending[vect] = bm[i];
                bm[i] = 0;
            }
            //Под той же блокировкой: главный поток увидит либо "ждёт", либо уже готовый меш, но не пропуск между ними.
            finishWaitingFirstMesh(id, vect);
        }
        //Если версия не совпала, чанк дождётся меша от более нового построения (или его уже удалили).
    }
    for(int i = 0; i < levels; i++) if(bm[i]) bm[i]->drop();
    delete[] bm;
    return accepted;
}

//Новая функция: одна реализация вместо пяти почти одинаковых updateMeshChunk(s)() (сами они удалены). Главный поток забирает
//готовые меши и раздаёт их контроллерам блоков. Раньше updateMeshChunk() через operator[] вставлял в карты
//пустые записи, а при NULL-меше ещё и правил список отрисовываемых чанков - отсюда гонки с потоками построения.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::applyPendingMeshes(int numbermesh, bool allworlds, int id, bool allchunks, Vector3d<int> vect)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //numbermesh = -1 - все уровни (поэтому без проверки < 0).
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error updating mesh chunks: \"number of mesh\" is > number of meshes");
        return;
    }
    struct PendingMesh
    {
        int s_NumberMesh;
        int s_WorldId;
        Vector3d<int> s_Vector;
        IMesh* s_Mesh;
    };
    std::vector<PendingMesh> meshes;
    {
        //Под блокировкой только забираем готовые меши, сами ноды создаём уже без неё,
        //чтобы потоки построения не ждали главный поток.
        lock_guard<recursive_mutex> locker(s_LockWrite);
        if(!s_ChunksMesh) return;
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        {
            if(numbermesh >= 0 && i != numbermesh) continue;
            std::map< int, std::map< Vector3d<int>, IMesh* > >::iterator iterworld;
            for(iterworld = s_ChunksMesh[i].begin(); iterworld != s_ChunksMesh[i].end(); ++iterworld)
            {
                if(!allworlds && iterworld->first != id) continue;
                std::map< Vector3d<int>, IMesh* >& chunks = iterworld->second;
                std::map< Vector3d<int>, IMesh* >::iterator iterchunk = chunks.begin();
                while(iterchunk != chunks.end())
                {
                    if(!allchunks && !(iterchunk->first == vect))
                    {
                        ++iterchunk;
                        continue;
                    }
                    PendingMesh pm;
                    pm.s_NumberMesh = i;
                    pm.s_WorldId = iterworld->first;
                    pm.s_Vector = iterchunk->first;
                    pm.s_Mesh = iterchunk->second;
                    meshes.push_back(pm);
                    chunks.erase(iterchunk++);
                }
            }
        }
    }
    for(unsigned int i = 0; i < meshes.size(); i++)
    {
        ChunksMeshBlocksController* controller = getChunksMeshBlocksController(meshes[i].s_WorldId);
        if(!controller)
        {
            if(meshes[i].s_Mesh) meshes[i].s_Mesh->drop();
            continue;
        }
        if(meshes[i].s_Mesh) controller->setMesh(meshes[i].s_NumberMesh, meshes[i].s_Mesh, meshes[i].s_Vector);
        else controller->deleteMesh(meshes[i].s_NumberMesh, meshes[i].s_Vector);
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialFlag(int numbermesh, int id, E_MATERIAL_FLAG flag, bool value)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setMaterialFlag")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->setMaterialFlag(numbermesh, flag, value);
    else registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag: world does not exist, id: " + WorkFunctions::ConvertFunctions::itos(id));
}

//Изменено: getMeshNumber() вызывается под той же блокировкой, что и getMeshVoxel() (раньше - без неё);
//при ошибке освобождаются уже созданные меши и массив (раньше - утечка); проверяется и отрицательный номер уровня.
CBatchingMesh** SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::generateIMeshForSceneNode(VoxelsForRendering* vxlfr, int& levels)
{
    int numbermesh = 0;
    IMesh* mesh = 0;
    VoxelForRendering* vxl = 0;
    levels = s_NumberOfMeshesSceneNodeForChunk; //Число уровней читается один раз: дальше используется только оно.
    CBatchingMesh** BatchingMesh = new CBatchingMesh*[levels];
    for(int i = 0; i < levels; i++) BatchingMesh[i] = new CBatchingMesh();
    bool good = true;
    for(int i = 0; i < vxlfr->getNumberOfVoxelsForRendering() && good; i++)
    {
        vxl = vxlfr->getVoxelForRendering(i);
        {
            //Пользовательские getMeshVoxel()/getMeshNumber() обычно используют общие временные меши,
            //поэтому вызываются строго по одному.
            lock_guard<recursive_mutex> locker(s_LockGetMeshVoxel);
            mesh = getMeshVoxel(vxl);
            numbermesh = getMeshNumber(vxl);
        }
        if(!mesh)
        {
            registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error getting IMesh.", true);
            good = false;
            break;
        }
        if(numbermesh < 0 || numbermesh >= levels) //Исправлено: по levels - числу мешей в BatchingMesh (раньше по текущему числу уровней).
        {
            registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk (FCMSN): \"number of mesh\" is > number of meshes");
            mesh->drop();
            good = false;
            break;
        }
        BatchingMesh[numbermesh]->addMesh(mesh, vector3df( vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ()),
                              vector3df( vxl->getRotation().getX(), vxl->getRotation().getY(), vxl->getRotation().getZ() ));
        mesh->drop();
    }
    if(!good)
    {
        for(int i = 0; i < levels; i++) BatchingMesh[i]->drop(); //Исправлено: по levels, как и создавались.
        delete[] BatchingMesh;
        return 0;
    }
    for(int i = 0; i < levels; i++) //Исправлено: по levels - если число уровней изменилось за время построения, раньше здесь был выход за массив.
    {
        BatchingMesh[i]->update();
        BatchingMesh[i]->finalize();
    }
    return BatchingMesh;
}

//Раньше deleteChunk() только клал NULL в очередь мешей, а из списка отрисовываемых чанк убирал главный поток
//позже (в updateMeshChunk) - всё это время потоки построения считали чанк живым. Теперь чанк убирается сразу,
//его версия сбрасывается (незаконченные построения отбрасываются), а главный поток только удалит меш из блока.
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteChunk(int id, Vector3d<int> vect)
{
    //Исправлено: под блокировкой движка. Поток построения проверяет "чанк отрисовывается" и берёт новую версию
    //под этой блокировкой; без неё удаление из главного потока могло вклиниться между ними, и меш удалённого
    //чанка принимался - на экране оставался "призрак", который больше никто не убирал.
    lock_guard<recursive_mutex> enginelocker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iter = s_ListAllChunksFlagRendering.find(id);
#ifdef SVE_DEBUG_CHECKS
        //Добавлено (отладочная сборка): когда чанк убран из отрисовки - для сообщения о вероятной гонке в setChunkForUpdate().
        if(iter != s_ListAllChunksFlagRendering.end() && iter->second.erase(vect) > 0)
        {
            const unsigned int now = s_ISVE->getTimeSafeThread();
            std::map< Vector3d<int>, unsigned int >& recent = s_DebugRecentlyUnrendered[id];
            if(recent.size() >= 4096) //Записи старше секунды не нужны: удаляются, чтобы карта не росла.
            {
                for(std::map< Vector3d<int>, unsigned int >::iterator r = recent.begin(); r != recent.end(); )
                {
                    if(now - r->second > 1000) recent.erase(r++); //Исправлено: скобки - без них неоднозначный else (-Wdangling-else).
                    else ++r;
                }
            }
            recent[vect] = now;
        }
#else
        if(iter != s_ListAllChunksFlagRendering.end()) iter->second.erase(vect);
#endif
    }
    deleteChunkForUpdate(id, vect);
    lock_guard<recursive_mutex> locker(s_LockWrite);
    finishWaitingFirstMesh(id, vect);
    if(!s_ChunksMesh) return true;
    //Удаление тоже меняет версию: все незаконченные построения этого чанка будут отброшены.
    std::map< int, std::map< Vector3d<int>, unsigned long > >::iterator iterversion = s_ChunkVersions.find(id);
    if(iterversion != s_ChunkVersions.end()) iterversion->second.erase(vect);
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        std::map< Vector3d<int>, IMesh* >& pending = s_ChunksMesh[i][id];
        std::map< Vector3d<int>, IMesh* >::iterator iter = pending.find(vect);
        if(iter != pending.end() && iter->second) iter->second->drop();
        pending[vect] = 0; //NULL - пометка "удалить меш из блока" для главного потока.
    }
    return true;
}

//Упрощено: повторный поиск через operator[] заменён уже найденным итератором (логика та же).
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isRenderChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator I = s_ListAllChunksFlagRendering.find(id);
    if ( I == s_ListAllChunksFlagRendering.end() ) return false;
    return I->second.find(vect) != I->second.end();
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isForUpdateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator I = s_ChunksMeshSceneNodeForUpdate.find(id);
    if ( I == s_ChunksMeshSceneNodeForUpdate.end() ) return false;
    return I->second.find(vect) != I->second.end();
}

//Раньше блокировка бралась после проверки isForUpdateChunk(), и один чанк мог попасть в список дважды.
//Изменено: проверка и пометка - под блокировкой движка (чанк не убирается из отрисовки между ними); чанк, который не
//отрисовывается, - нарушение логики; уже помеченный чанк - без сообщения (враппер помечает чанк на каждую правку вокселя).
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    std::unique_lock<recursive_mutex> enginelocker;
    if(s_ISVE) enginelocker = std::unique_lock<recursive_mutex>(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    if(!isRenderChunk(id, vect))
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting chunk for update: chunk is not rendered (Vector: " + vect.toString(", ") + ").");
#ifdef SVE_DEBUG_CHECKS
        //Добавлено: отладочная проверка - чанк убрали из отрисовки только что, а вызов не из потока главной очереди:
        //вероятно, гонка (проверка isRenderChunk() или изменение мира были не под той же блокировкой движка).
        if(s_ISVE && !s_ISVE->isMainQueueThread())
        {
            std::map< int, std::map< Vector3d<int>, unsigned int > >::iterator recentworld = s_DebugRecentlyUnrendered.find(id);
            if(recentworld != s_DebugRecentlyUnrendered.end())
            {
                std::map< Vector3d<int>, unsigned int >::iterator recent = recentworld->second.find(vect);
                const unsigned int now = s_ISVE->getTimeSafeThread();
                if(recent != recentworld->second.end() && now - recent->second <= 1000)
                    registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Debug check (SVE_DEBUG_CHECKS): setChunkForUpdate: the chunk was removed from rendering "
                                         + std::to_string(now - recent->second) + " ms ago - probably a race: call isRenderChunk()/change the world and setChunkForUpdate() under one engine lock or in the main queue thread.");
            }
        }
#endif
        return false;
    }
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    if(isForUpdateChunk(id, vect)) return false;
    s_ChunksMeshSceneNodeForUpdate[id][vect] = true;
    return true;
}

//Списки копируются под блокировкой и обходятся без неё (в этой и следующих функциях): раньше задания ставились,
//пока блокировка списка была захвачена, и цикл шёл по карте, которую в это время меняли.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateAllChunks(int id)
{
    //Список копируется, чтобы не держать блокировку списка, пока ставятся задания
    //(иначе возможна взаимная блокировка со списком чанков для обновления).
    std::vector< Vector3d<int> > chunks;
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iterworld = s_ListAllChunksFlagRendering.find(id);
        if(iterworld == s_ListAllChunksFlagRendering.end()) return;
        std::map< Vector3d<int>, bool >::iterator iter;
        for(iter = iterworld->second.begin(); iter != iterworld->second.end(); ++iter) chunks.push_back(iter->first);
    }
    for(unsigned int i = 0; i < chunks.size(); i++) updateChunk(id, chunks[i]);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateAllChunks()
{
    std::vector<int> worlds;
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iter;
        for(iter = s_ListAllChunksFlagRendering.begin(); iter != s_ListAllChunksFlagRendering.end(); ++iter) worlds.push_back(iter->first);
    }
    for(unsigned int i = 0; i < worlds.size(); i++) updateAllChunks(worlds[i]);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunks(int id)
{
    std::vector< Vector3d<int> > chunks;
    {
        lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iterworld = s_ChunksMeshSceneNodeForUpdate.find(id);
        if(iterworld == s_ChunksMeshSceneNodeForUpdate.end()) return;
        std::map< Vector3d<int>, bool >::iterator iter;
        for(iter = iterworld->second.begin(); iter != iterworld->second.end(); ++iter) chunks.push_back(iter->first);
        s_ChunksMeshSceneNodeForUpdate.erase(iterworld);
    }
    for(unsigned int i = 0; i < chunks.size(); i++) updateChunk(id, chunks[i], true);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunks()
{
    std::vector<int> worlds;
    {
        lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iter;
        for(iter = s_ChunksMeshSceneNodeForUpdate.begin(); iter != s_ChunksMeshSceneNodeForUpdate.end(); ++iter) worlds.push_back(iter->first);
    }
    for(unsigned int i = 0; i < worlds.size(); i++) updateChunks(worlds[i]);
}

//Раньше на каждый чанк создавалось задание UPDATE_CHUNKS в главную очередь, а оттуда - ещё одно в очередь обновления.
//Теперь чанк сразу ставится в очередь построения (повторно не ставится, если уже ждёт).
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool saveplace)
{
    if(!isRenderChunk(id, vect))
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error chunk update: chunk does not exist. ID: " + WorkFunctions::ConvertFunctions::itos(id) + ", Vector: " + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER) + ".");
        return false;
    }
    s_ISVE->requestBuildChunk(id, vect, true);
    if(!saveplace) deleteChunkForUpdate(id, vect);
    return true;
}

//Раньше здесь было два lock() подряд вместо lock()/unlock(): мьютекс рекурсивный, поэтому сам вызвавший поток
//не зависал, но мьютекс оставался захваченным навсегда, и любой другой поток, обратившийся к списку
//отрисовываемых чанков (isRenderChunk() в потоках построения), зависал. Кроме того, чанки не удалялись
//по одному (оставались ожидающие меши и версии), а clear() контроллера стирал настройки уровней.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunks(int id)
{
    std::vector< Vector3d<int> > chunks;
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iterworld = s_ListAllChunksFlagRendering.find(id);
        if(iterworld != s_ListAllChunksFlagRendering.end())
        {
            std::map< Vector3d<int>, bool >::iterator iter;
            for(iter = iterworld->second.begin(); iter != iterworld->second.end(); ++iter) chunks.push_back(iter->first);
        }
    }
    for(unsigned int i = 0; i < chunks.size(); i++) deleteChunk(id, chunks[i]);
    deleteAllChunksForUpdate(id);
    //Исправлено: ноды сцены трогаются только в главном потоке. Из другого потока чанки только убраны из
    //отрисовки (в очереди мешей - пометки "удалить"), их ноды удалит ближайший update(). Раньше ноды удалялись (и вызывался
    //onUpdateChunkMeshSceneNode()) прямо в вызвавшем потоке, параллельно с рисованием в главном.
    if(s_ISVE && !s_ISVE->isMainThread()) return;
    applyPendingMeshes(-1, false, id, true, Vector3d<int>());
    //Раньше здесь вызывался clear() контроллера, который удалял и его настройки (материалы, флаги),
    //после чего мир нельзя было рисовать снова. Теперь удаляются только блоки.
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->clearBlocks();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunks()
{
    std::vector<int> worlds;
    {
        lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
        std::map< int, std::map< Vector3d<int>, bool > >::iterator iter;
        for(iter = s_ListAllChunksFlagRendering.begin(); iter != s_ListAllChunksFlagRendering.end(); ++iter) worlds.push_back(iter->first);
    }
    for(unsigned int i = 0; i < worlds.size(); i++) deleteAllChunks(worlds[i]);
    //Убрана общая очистка s_ListAllChunksFlagRendering: каждый чанк уже удалён deleteChunk(), а общая очистка
    //заодно стирала чанки, зарегистрированные между этими шагами, не снимая с них пометку "ждёт первого меша".
    deleteAllChunksForUpdate();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunksForUpdate(int id)
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    s_ChunksMeshSceneNodeForUpdate.erase(id);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunksForUpdate()
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    s_ChunksMeshSceneNodeForUpdate.clear();
}

//Проверка и удаление под одной блокировкой (раньше проверка шла без неё). Сообщение "This is normal" убрано - это не ошибка.
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    std::map< int, std::map< Vector3d<int>, bool > >::iterator iter = s_ChunksMeshSceneNodeForUpdate.find(id);
    if(iter == s_ChunksMeshSceneNodeForUpdate.end() || iter->second.find(vect) == iter->second.end()) return false;
    iter->second.erase(vect);
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::freeze()
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error freezeing factory chunk mesh scene node: this object already frozen.");
        return false;
    }
    s_Freeze = true;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isFreeze()
{
    return s_Freeze;
}

IrrSimpleVoxelEngine* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getIrrSimpleVoxelEngine()
{
    return s_ISVE;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIrrSimpleVoxelEngine(IrrSimpleVoxelEngine* isve)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrSimpleVoxelEngine: object already frozen.");
        return false;
    }
    if(s_ISVE)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrSimpleVoxelEngine: IrrSimpleVoxelEngine already setted.");
        return false;
    }
    s_ISVE = isve;
    return true;
}

IrrlichtDevice* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getIrrlichtDevice()
{
    return s_Device;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIrrlichtDevice(IrrlichtDevice* device)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrlichtDevice: object already frozen.");
        return false;
    }
    if(s_Device)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrlichtDevice: IrrlichtDevice already setted.");
        return false;
    }
    s_Device = device;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setPositionScale(float ps)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting position scale: object already frozen.");
        return false;
    }
    s_PositionScale = ps;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getPositionScale()
{
    return s_PositionScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setRotationScale(float rs)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting rotation scale: object already frozen.");
        return false;
    }
    s_RotationScale = rs;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getRotationScale()
{
    return s_RotationScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSScale(float ss)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting 'scale' scale: object already frozen.");
        return false;
    }
    s_SScale = ss;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getSScale()
{
    return s_SScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIsFaces(bool value)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting isFaces: object already frozen.");
        return false;
    }
    s_IsFaces = value;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isFaces()
{
    return s_IsFaces;
}

//Добавлено: меш грани для getMeshCubeFaces() - раньше его приходилось загружать из файла модели
//(TestProject3 - models/plane3.obj), а неверная плоскость или размер давали неверный куб. Вершины, индексы и
//текстурные координаты - как у plane3.obj после загрузчика Irrlicht (он отражает X и V).
IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::createFaceMesh(f32 texinset)
{
    if(texinset < 0.f) texinset = 0.f;
    if(texinset > 0.5f) texinset = 0.5f;
    const f32 a = texinset, b = 1.f - texinset;
    const SColor white(255, 255, 255, 255);
    SMeshBuffer* mb = new SMeshBuffer;
    mb->Vertices.push_back(S3DVertex( 0.5f, 0.f, -0.5f, 0.f, 1.f, 0.f, white, a, a));
    mb->Vertices.push_back(S3DVertex( 0.5f, 0.f,  0.5f, 0.f, 1.f, 0.f, white, a, b));
    mb->Vertices.push_back(S3DVertex(-0.5f, 0.f,  0.5f, 0.f, 1.f, 0.f, white, b, b));
    mb->Vertices.push_back(S3DVertex(-0.5f, 0.f, -0.5f, 0.f, 1.f, 0.f, white, b, a));
    const u16 indices[6] = { 2, 1, 0, 0, 3, 2 }; //Лицевая сторона - вверх (+Y), как у plane3.obj
    for(int i = 0; i < 6; i++) mb->Indices.push_back(indices[i]);
    mb->recalculateBoundingBox();
    SMesh* mesh = new SMesh;
    mesh->addMeshBuffer(mb);
    mb->drop(); //Мешбуфером теперь владеет меш
    mesh->recalculateBoundingBox();
    return mesh;
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshCubeFaces(VoxelForRendering* vxlfr, IMesh* plane, vector3df planescale)
{
    IMesh* planemas[6];
    for(int i = 0; i < 6; i++) planemas[i] = plane;
    return getMeshCubeFaces(vxlfr, planemas, planescale);
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshCubeFaces(VoxelForRendering* vxlfr, IMesh** planes, vector3df planescale)
{
    if(!planes)
    {
        registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: plane mesh not setted.", true);
        return 0;
    }
    CBatchingMesh* BatchingMesh = new CBatchingMesh();
    if(!BatchingMesh)
    {
        registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: error adding CBatchingMesh.", true);
        return 0;
    }
    vector3df vect_move_pos;
    vector3df vect_move_rot;
    vector3df vect_scale;
    float psscale = 1;
    for(int i = 0; i < 6; i++)
    {
        if(vxlfr->isRenderFace(i))
        {
            vect_move_pos.set(0,0,0);
            vect_move_rot.set(0,0,0);
            vect_scale.set(vxlfr->getScale().getX() * planescale.X, vxlfr->getScale().getY() * planescale.Y, vxlfr->getScale().getZ() * planescale.Z); //Исправлено: planescale не использовался - теперь множитель к масштабу вокселя.
            if(!planes[i])
            {
                registerFactoryEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: Error getting IMesh.", true);
                BatchingMesh->drop();
                return 0;
            }
            if(i == 4) { vect_move_pos.set(0, psscale/2, 0); }
            else if(i == 5) { vect_move_pos.set(0, (-1)*psscale/2, 0); vect_move_rot.set(180, 180, 0); }
            else if(i == 0) { vect_move_pos.set(psscale/2, 0, 0); vect_move_rot.set(0, 90, -90); }
            else if(i == 2) { vect_move_pos.set((-1)*psscale/2, 0, 0); vect_move_rot.set(0, -90, 90); }
            else if(i == 1) { vect_move_pos.set(0, 0, psscale/2); vect_move_rot.set(90, 0, 0); }
            else if(i == 3) { vect_move_pos.set(0, 0, (-1)*psscale/2); vect_move_rot.set(-90, 0, 180); }
            vect_move_pos.set(vect_move_pos.X * vect_scale.X, vect_move_pos.Y * vect_scale.Y, vect_move_pos.Z * vect_scale.Z);
            BatchingMesh->addMesh(planes[i], vect_move_pos, vect_move_rot, vect_scale);
        }
    }
    BatchingMesh->update();
    BatchingMesh->finalize();
    return BatchingMesh;
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshVoxelByGlue(VoxelForRendering* vxlfr, IMesh** glues)
{
    //Изменено: сообщения - про склейку (были скопированы от getMeshCubeFaces(): "plane mesh not setted"); ошибка
    //вызывающего кода - нарушение логики, не фатальная (меш вокселя не строится, чанк - с ошибкой построения).
    if(!glues)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error getting mesh voxel by glue: glues array is NULL.");
        return 0;
    }
    CBatchingMesh* BatchingMesh = new CBatchingMesh(); //Изменено: убрана проверка на NULL - new при нехватке памяти бросает исключение, а не возвращает NULL.
    BatchingMesh->addMesh(glues[0]);
    for(int i = 1; i < 7; i++)
    {
        if(vxlfr->isRenderFace(i - 1))
        {
            if(!glues[i])
            {
                registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error getting mesh voxel by glue: glues[" + std::to_string(i) + "] is NULL, but face " + std::to_string(i - 1) + " is visible.");
                BatchingMesh->drop();
                return 0;
            }
            BatchingMesh->addMesh(glues[i]);
        }
    }
    BatchingMesh->update();
    BatchingMesh->finalize();
    return BatchingMesh;
}

//Добавлены блокировка движка и проверка мира (раньше - разыменование NULL для несуществующего мира).
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMultiplePartition(int id, bool mp)
{
    //Добавлено: у фабрики, ещё не отданной движку, миров нет (было обращение к NULL).
    if(!s_ISVE) return;
    lock_guard<recursive_mutex> locker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    World* wrld = s_ISVE->getWorld(id);
    if(wrld) wrld->setMultiplePartition(mp);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isMultiplePartition(int id)
{
    if(!s_ISVE) return false; //Добавлено: у фабрики, ещё не отданной движку, миров нет (было обращение к NULL).
    lock_guard<recursive_mutex> locker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    World* wrld = s_ISVE->getWorld(id);
    if(!wrld) return false;
    return wrld->isMultiplePartition();
}

//Контроллер сначала полностью настраивается и только потом кладётся в карту (раньше - через многократный operator[]).
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::addWorld(int id, World3DInfo w3dinfo)
{
    if(getChunksMeshBlocksController(id))
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world in factory chunk mesh scene node: world already added.");
        return;
    }
    //Добавлено: после clear() уровней нет (массивы настроек удалены) - мир добавить нельзя.
    if(!s_ChunksMesh || s_NumberOfMeshesSceneNodeForChunk < 1)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world in factory chunk mesh scene node: factory is cleared (clear()).");
        return;
    }
    ChunksMeshBlocksController* controller = new ChunksMeshBlocksController(s_ISVE, s_Device, id);
    controller->setScale(s_SScale);
    {
        //Размер чанка мира запоминается один раз (он не меняется), чтобы блоки не читали карту миров из главного потока.
        lock_guard<recursive_mutex> locker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
        World* wrld = s_ISVE->getWorld(id);
        controller->setSizeChunk(wrld ? wrld->getSizeChunk() : 0);
    }
    controller->setNumberOfMeshesSceneNodeForChunk(s_NumberOfMeshesSceneNodeForChunk);
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        controller->setHardwareMappingHint(i, s_HardwareMappingHint[i], s_BufferType[i]);
        controller->setMaterialType(i, s_MaterialType[i]);
    }
    if(s_SizeBlockChunksX * s_SizeBlockChunksY * s_SizeBlockChunksZ != 0) controller->setSizeChunkMeshBlock(s_SizeBlockChunksX, s_SizeBlockChunksY, s_SizeBlockChunksZ);
    s_ChunksMeshBlocksControllers[id] = controller;
    //Изменено: под блокировкой (s_WorldRootNodes читают и другие потоки), на родительскую ноду берётся ссылка.
    if(w3dinfo.s_WorldRootNode) w3dinfo.s_WorldRootNode->grab();
    lock_guard<mutex> rootlocker(s_LockWorldRootNodes);
    s_WorldRootNodes[id] = w3dinfo;
}

//Изменено: применение мешей через applyPendingMeshes(), передача позиции активной камеры контроллерам
//(для сортировки прозрачности) и пропуск NULL-контроллеров.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::update()
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::update")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    applyPendingMeshes(-1, true, 0, true, Vector3d<int>());
    //Позиция активной камеры нужна для сортировки полупрозрачных треугольников.
    bool camera = false;
    vector3df cameraposition;
    ICameraSceneNode* activecamera = s_Device ? s_Device->getSceneManager()->getActiveCamera() : 0;
    if(activecamera)
    {
        camera = true;
        cameraposition = activecamera->getAbsolutePosition();
    }
    std::map< int, ChunksMeshBlocksController* >::iterator iter;
    for(iter = s_ChunksMeshBlocksControllers.begin(); iter != s_ChunksMeshBlocksControllers.end(); ++iter)
    {
        if(!iter->second) continue;
        if(camera) iter->second->setActiveCameraPosition(cameraposition);
        iter->second->update(s_OcclusionQueries);
    }
    //Перенесено: Occlusion Queries выполняются в runOcclusionQueries() - между drawAll() и endScene(). Здесь
    //(у пользователя - до beginScene()) проверка рисовалась не в Z-буфер текущего кадра и давала неверную видимость.
}

//Добавлено: бывшая часть update() - проверка видимости нод чанков.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::runOcclusionQueries()
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::runOcclusionQueries")) return; //Отладочная проверка: только главный поток.
#endif
    if(!s_OcclusionQueries || !s_ISVE || !s_Device) return;
    //Изменено: проверка и по числу кадров - Irrlicht удаляет Occlusion Query, которую не запускали больше 1000 кадров
    //(при большом таймере и высоком FPS ноды переставали проверяться).
    s_FramesSinceOcclusionQueries++;
    if(s_ISVE->getTimeSafeThread() - s_RememberTimeForOcclusionQueriesInMilliseconds < s_TimerOcclusionQueriesInMilliseconds
       && s_FramesSinceOcclusionQueries < s_MaxFramesBetweenOcclusionQueries) return;
    s_FramesSinceOcclusionQueries = 0;
    s_Device->getVideoDriver()->runAllOcclusionQueries(false);
    s_Device->getVideoDriver()->updateAllOcclusionQueries();
    std::map< int, ChunksMeshBlocksController* >::iterator iter;
    for(iter = s_ChunksMeshBlocksControllers.begin(); iter != s_ChunksMeshBlocksControllers.end(); ++iter)
        if(iter->second) iter->second->updateOcclusionQueries();
    s_RememberTimeForOcclusionQueriesInMilliseconds = s_ISVE->getTimeSafeThread();
}

//Изменено: включение/выключение применяется и к уже созданным нодам. Раньше при выключении ноды, скрытые
//проверкой, так и оставались невидимыми (а их проверки - в драйвере), а при включении уже созданные ноды не проверялись.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setOcclusionQueries(bool value)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setOcclusionQueries")) return; //Отладочная проверка: только главный поток (трогает ноды).
#endif
    if(s_OcclusionQueries == value) return;
    s_OcclusionQueries = value;
    std::map< int, ChunksMeshBlocksController* >::iterator iter;
    for(iter = s_ChunksMeshBlocksControllers.begin(); iter != s_ChunksMeshBlocksControllers.end(); ++iter)
        if(iter->second) iter->second->setOcclusionQueries(value);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getOcclusionQueries()
{
    return s_OcclusionQueries;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setTimerTestOcclusionQueriesInMilliseconds(unsigned int timer)
{
    s_TimerOcclusionQueriesInMilliseconds = timer;
}

unsigned int SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getTimerTestOcclusionQueriesInMilliseconds()
{
    return s_TimerOcclusionQueriesInMilliseconds;
}

//Добавлено: проверка Occlusion Queries не реже чем раз в frames кадров.
bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaxFramesBetweenOcclusionQueries(unsigned int frames)
{
    if(frames < 1 || frames > MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES_LIMIT)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max frames between occlusion queries: value must be from 1 to 1000 (Irrlicht removes a query not run for more than 1000 frames).");
        return false;
    }
    s_MaxFramesBetweenOcclusionQueries = frames;
    return true;
}

unsigned int SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMaxFramesBetweenOcclusionQueries()
{
    return s_MaxFramesBetweenOcclusionQueries;
}

//Добавлено: проверка нод по ограничивающему кубоиду или по полному мешу; применяется и к уже созданным нодам.
void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setOcclusionQueriesBoundingBox(bool value)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setOcclusionQueriesBoundingBox")) return; //Отладочная проверка: только главный поток (трогает ноды).
#endif
    if(s_OcclusionQueriesBoundingBox == value) return;
    s_OcclusionQueriesBoundingBox = value;
    if(!s_OcclusionQueries) return;
    //Проверки уже созданных нод добавляются заново - с новой формой (Irrlicht заменяет меш у существующей проверки).
    std::map< int, ChunksMeshBlocksController* >::iterator iter;
    for(iter = s_ChunksMeshBlocksControllers.begin(); iter != s_ChunksMeshBlocksControllers.end(); ++iter)
        if(iter->second) iter->second->setOcclusionQueries(true);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getOcclusionQueriesBoundingBox()
{
    return s_OcclusionQueriesBoundingBox;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSizeBlockChunks(int id, int xsize, int ysize, int zsize)
{
    if(s_SizeBlockChunksX * s_SizeBlockChunksY * s_SizeBlockChunksZ != 0)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks to world: size chunks setted for all worlds.");
        return;
    }
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->setSizeChunkMeshBlock(xsize, ysize, zsize);
    else registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks: world does not exist.");
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSizeBlockChunks(int xsize, int ysize, int zsize)
{
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks for all worlds: object already frozen.");
        return;
    }
    s_SizeBlockChunksX = xsize;
    s_SizeBlockChunksY = ysize;
    s_SizeBlockChunksZ = zsize;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setHardwareMappingHint(int numbermesh, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE buffertype)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setHardwareMappingHint")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting hardware mapping hint (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    s_HardwareMappingHint[numbermesh] = mapping;
    s_BufferType[numbermesh] = buffertype;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setMaterialType")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    s_MaterialType[numbermesh] = newtype;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setHardwareMappingHint(int numbermesh, int id, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE buffertype)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setHardwareMappingHint")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->setHardwareMappingHint(numbermesh, mapping, buffertype);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialType(int numbermesh, int id, irr::video::E_MATERIAL_TYPE newtype)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setMaterialType")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->setMaterialType(numbermesh, newtype);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setNumberOfMeshesSceneNodeForChunk(int nomsnfc)
{
    //Добавлено: у замороженной (отданной движку) фабрики число уровней не меняется - у миров уже есть блоки
    //со старым числом уровней, а потоки построения раскладывают меши по уровням.
    if(isFreeze())
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting number of meshes scene node for chunk: object already frozen.");
        return;
    }
    if(nomsnfc < 1) nomsnfc = 1; //Добавлено: 0 уровней давал массивы нулевой длины и падение при первом меше.
    lock_guard<recursive_mutex> locker(s_LockWrite);
    if(s_ChunksMesh)
    {
        //Ещё не применённые меши освобождаем, иначе утечка.
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        {
            std::map< int, std::map< Vector3d<int>, IMesh* > >::iterator iter;
            for(iter = s_ChunksMesh[i].begin(); iter != s_ChunksMesh[i].end(); ++iter)
            {
                std::map< Vector3d<int>, IMesh* >::iterator iter2;
                for(iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
                    if(iter2->second) iter2->second->drop();
            }
        }
        delete[] s_ChunksMesh;
    }
    s_NumberOfMeshesSceneNodeForChunk = nomsnfc;
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    s_HardwareMappingHint = new E_HARDWARE_MAPPING[s_NumberOfMeshesSceneNodeForChunk];
    s_BufferType = new E_BUFFER_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_MaterialType = new E_MATERIAL_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_ChunksMesh = new std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* > >[s_NumberOfMeshesSceneNodeForChunk];
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        s_HardwareMappingHint[i] = EHM_NEVER;
        s_BufferType[i] = EBT_VERTEX_AND_INDEX;
        s_MaterialType[i] = EMT_SOLID;
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setID(int numbermesh, int id, s32 idscenenode)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::setID")) return; //Отладочная проверка (добавлено): только главный поток.
#endif
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->setID(numbermesh, idscenenode);
}

int SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getNumberOfMeshesSceneNodeForChunk()
{
    return s_NumberOfMeshesSceneNodeForChunk;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int numbermesh, Vector3d<int> vectblock) {}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateCameraPosition(int id, vector3df vect)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::updateCameraPosition")) return; //Добавлено: отладочная проверка - только главный поток.
#endif
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(controller) controller->updateCameraPosition(vect);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::changeWorld3DInfo(int id, World3DInfo w3dinfo)
{
#ifdef SVE_DEBUG_CHECKS
    if(s_ISVE && !s_ISVE->debugCheckMainThread("FactoryChunkMeshSceneNode::changeWorld3DInfo")) return false; //Отладочная проверка (добавлено): только главный поток.
#endif
    ChunksMeshBlocksController* controller = getChunksMeshBlocksController(id);
    if(!controller)
    {
        registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "Change root node for not exists world, id = " + WorkFunctions::ConvertFunctions::itos(id) + ".");
        return false;
    }
    {
        //Изменено: под блокировкой, ссылка переходит на новую родительскую ноду.
        if(w3dinfo.s_WorldRootNode) w3dinfo.s_WorldRootNode->grab();
        lock_guard<mutex> rootlocker(s_LockWorldRootNodes);
        World3DInfo& old = s_WorldRootNodes[id];
        if(old.s_WorldRootNode) old.s_WorldRootNode->drop();
        old = w3dinfo;
    }
    //Добавлено: ноды мира пересклеиваются с новой родительской нодой и сдвигом (раньше новые данные получали
    //только ноды, склеенные позже), пересчёт координат сразу учитывает новые данные.
    controller->requestUpdateLevel(-1);
    if(s_ISVE) s_ISVE->updateWorldTransform(id);
    return true;
}

//find вместо operator[]; ошибки пишутся через registerEvent() движка (getLogger() мог вернуть NULL, если логгер не задан).
World3DInfo SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getWorld3DInfo(int id)
{
    World3DInfo w3dinfo;
    bool found = false;
    {
        lock_guard<mutex> rootlocker(s_LockWorldRootNodes); //Добавлено: s_WorldRootNodes меняет главный поток, читают и другие.
        std::map<int, World3DInfo>::iterator iter = s_WorldRootNodes.find(id);
        if(iter != s_WorldRootNodes.end())
        {
            w3dinfo = iter->second;
            found = true;
        }
    }
    if(!found) registerFactoryEvent(EVENT_TYPE_LOGIC_VIOLATION, "Getting world 3d info for not exists world, id = " + WorkFunctions::ConvertFunctions::itos(id) + ".");
    return w3dinfo;
}
