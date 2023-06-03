#ifndef IrrSimpleVoxelEngine_H
#define IrrSimpleVoxelEngine_H

#include <SimpleVoxelEngine.h>
#include <irrlicht.h>

#include <ThreadSafeVector.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "FactoryChunkMeshSceneNode.h"
#include "ThreadSafeCounter.h"
#include "ThreadSafeType.h"
#include "IrrSVEDefines.h"
#include "UserControllerForThread.h"

#include "Task.h"

//! Пространство имён SimpleVoxelEngine.
/*! Мы будем наш враппер описывать внутри пространства имён исходного движка.
*/
namespace SimpleVoxelEngine
{
    //! Главное пространство имён IrrSimpleVoxelEngine.
    /*! Здесь находятся все объекты и компоненты враппера.
    */
    namespace IrrEngine
    {
        //! Пространство имён с предопределённым константами для работы движка.
        namespace IrrSVEDefines{};
        //! Пространство имён с безопасными для потоковых операций типами.
        namespace ThreadSafeTypes{};
    };
    namespace IrrEngine
    {
        //! Движок.
        /*! Враппер для иррлихта, унаследован от самого движка.
        Является основным объектом для работы. Фактически вся работа и
        управление движком идёт через этот класс.
        */
        class IrrSimpleVoxelEngine : public SimpleVoxelEngine::Core::Engine
        {
            public:
                //! Стандартный конструктор.
                /*! \param device Указатель на IrrlichtDevice, используемый для отрисовки.
                */
                IrrSimpleVoxelEngine(irr::IrrlichtDevice* device);
                //! Конструктор с логгером.
                /*! \param device Указатель на IrrlichtDevice, используемый для отрисовки.
                \param lgr Указатель на логгер, который будет использовать движок.
                */
                IrrSimpleVoxelEngine(irr::IrrlichtDevice* device, SimpleVoxelEngine::Debug::Logger* lgr);
                //! Стандартный деструктор.
                ~IrrSimpleVoxelEngine();
                //! Создание мира с параметрами по умолчанию.
                /*! \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld(World3DInfo w3dinfo = World3DInfo());
                //! Создание мира с указанными параметрами.
                /*! \param wcp Параметры создания мира
                \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld(SimpleVoxelEngine::Core::WorldCreatingParameters& wcp, World3DInfo w3dinfo = World3DInfo());
                //! Добавление мира к движку.
                /*! \param wrld Добавляемый мир.
                \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор добавленного мира (wrld) или -1 в случае неудачи.
                */
                int addWorld(SimpleVoxelEngine::Core::World* wrld, World3DInfo w3dinfo = World3DInfo());
                //! Установка SimpleVoxelObject по VoxelAddress.
                /*! \param id Идентификатор мира, в котором устанавливаем SimpleVoxelObject.
                \param vxladdr Адрес вокселя.
                \param svo Устанавливаемый SimpleVoxelObject.
                \param rewrite Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования.
                \param immediatelyupdated Если имеет значение true, то чанк будет обновлён немедленно.
                */
                void setSimpleVoxelObject(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool immediatelyupdated = false);
                //! Удаление SimpleVoxelObject по VoxelAddress.
                /*! \param id Идентификатор мира, в котором удаляем SimpleVoxelObject.
                \param vxladdr Адрес вокселя, который надо удалить.
                \param immediatelyupdated Если имеет значение true, то чанк будет обновлён немедленно.
                */
                void deleteSimpleVoxelObject(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, bool immediatelyupdated = false);
                //! Добавление чанка для отрисовки.
                /*! \param id Идентификатор мира, в котором расположен чанк.
                \param vect Вектор (координаты) чанка в мире.
                \param load Если имеет значение true, то чанки будут загружены в случае отсутствия в памяти, иначе не будут.
                \param loadframe Если имеет значение true, то будет загружена рамка вокруг чанка.
                \param sizeframe Размер загружаемой кубической рамки.
                */
                void addChunkForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool load = false, bool loadframe = false, int sizeframe = 1);
                //! Добавление чанков для отрисовки.
                /*! \param id Идентификатор мира, в котором расположены чанки.
                \param vectleftdown Вектор (координаты) левого нижнего угла добавляемого кубоида чанков.
                \param vectrightup Вектор (координаты) правого верхнего угла добавляемого кубоида чанков.
                \param load Если имеет значение true, то чанки будут загружены в случае отсутствия в памяти, иначе не будут.
                \param loadframe Если имеет значение true, то будет загружена рамка вокруг кубоида чанков.
                \param sizeframe Размер загружаемой кубической рамки.
                */
                void addChunksForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vectleftdown, SimpleVoxelEngine::Additional::Vector3d<int> vectrightup, bool load = false, bool loadframe = false, int sizeframe = 1);
                //! Удаление чанка из отрисовки.
                /*! \param id Идентификатор мира, в котором расположен чанк.
                \param vect Вектор (координаты) чанка в мире.
                \param unload Если имеет значение true, то чанки будут выгружены из памяти, иначе не будут.
                \param save Если имеет значение true, то при выгрузке из памяти чанки будут сохранены, иначе не будут.
                \param unloadframe Если имеет значение true, то будет выгружена из памяти рамка вокруг чанка.
                \param sizeframe Размер выгружаемой из памяти кубической рамки.
                */
                void deleteChunkForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //! Добавление чанков для отрисовки.
                /*! \param id Идентификатор мира, в котором расположены чанки.
                \param vectleftdown Вектор (координаты) левого нижнего угла удаляемого кубоида чанков.
                \param vectrightup Вектор (координаты) правого верхнего угла удаляемого кубоида чанков.
                \param unload Если имеет значение true, то чанки будут выгружены из памяти, иначе не будут.
                \param save Если имеет значение true, то при выгрузке из памяти чанки будут сохранены, иначе не будут.
                \param unloadframe Если имеет значение true, то будет выгружена из памяти рамка вокруг кубоида чанков.
                \param sizeframe Размер выгружаемой из памяти кубической рамки.
                */
                void deleteChunksForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vectleftdown, SimpleVoxelEngine::Additional::Vector3d<int> vectrightup, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //! Получение фабрики чанков.
                /*! \return Возвращает указатель на установленную движку фабрику чанков.
                */
                FactoryChunkMeshSceneNode* getFactoryChunkMeshSceneNode();
                //! Установка фабрики чанков.
                /*! \param fcmsn Указатель на устанавливаемую фабрику.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setFactoryChunkMeshSceneNode(FactoryChunkMeshSceneNode* fcmsn);
                //! Получение контроллера для вызова пользовательских функций в другом потоке.
                /*! \return Возвращает указатель на контроллер.
                */
                UserControllerForThread* getUserControllerForThread();
                //! Установка контроллера для вызова пользовательских функций в другом потоке.
                /*! \param usrcntrlforthread Указатель на устанавливаемый контроллер.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setUserControllerForThread(UserControllerForThread* usrcntrlforthread);
                //! Обновление всех объектов движка.
                /*! Данная функция является одной из главных для работы движка.
                */
                void update();
                //! Получение состояния работы движка.
                /*! \return Возвращает текущее состояние работы движка.
                */
                MODE_MAIN_THREAD getModeMainThread();
                //! Функция для старта движка.
                /*! Данную функцию надо вызывать для запуска движка. Инициализирует потоки.
                */
                void start();
                //! Функция для остановки движка.
                /*! \param wait Если имеет значение true, то ожидается завершение работы всех потоков, иначе не ожидается.
                */
                void stop(bool wait = true);
                //! Функция проверки работы движка.
                /*! \return Возвращает true, если движок работает, иначе false,
                */
                bool isRun();
                //! Установка максимального числа потоков для обработки заданий по созданию чанков.
                /*! В связи с непонятыми ошибками данная функция пока ни на что ни влияет.
                \param newvaluemaxnumberofthread Новое значение максимального количества потоков для создания чанков.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setMaxNumberOfThreadAddChunks(unsigned int newvaluemaxnumberofthread);
                //! Получение максимального числа потоков для обработки заданий по созданию чанков.
                /*! \return Возвращает максимальное число потоков для обработки заданий по созданию чанков.
                */
                unsigned int getMaxNumberOfThreadAddChunks();
                //! Установка максимального числа потоков для обработки заданий по обновлению чанков.
                /*! В связи с непонятыми ошибками данная функция пока ни на что ни влияет.
                \param newvaluemaxnumberofthread Новое значение максимального количества потоков для обновления чанков.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setMaxNumberOfThreadUpdateChunks(unsigned int newvaluemaxnumberofthread);
                //! Получение максимального числа потоков для обработки заданий по обновлению чанков.
                /*! \return Возвращает максимальное число потоков для обработки заданий по обновлению чанков.
                */
                unsigned int getMaxNumberOfThreadUpdateChunks();
                //! Добавление задания в очередь.
                /*! \param _task Указатель на задание, которое добавляется в очередь. Память, занимаемая этим объектом, будет очищена движком.
                */
                void addMainTask(Task* _task);
                //! Добавление задания на обновление чанков.
                /*! \param all Если имеет значение true, то будут обновлены все чанки, иначе только из очереди на обновление.
                */
                void addTaskUpdateChunks(bool all = false);
                //! Добавление задания на выполнение пользовательской функции с заданными параметрами в другом потоке.
                /*! \param par Указатель на параметры, заданные пользователем.
                */
                void addTaskRunUserControllerForThread(ParametersForThreadFunction* par);
                //! Получение времени периодичного пробуждения сборщика мусора для проверки на необходимость очистки.
                /*! \return Возвращает число миллисекунд, которые составляют период пробуждения сборщика мусора для проверки на необходимость очистки.
                */
                unsigned int getWakeUpGarbageCollectorOncePerMilliseconds();
                //! Получение минимального размера очереди, при котором будет произведена очистка мусора.
                /*! \return Возвращает минимальный размер очереди, при котором будет произведена очистка мусора.
                */
                unsigned int getMaximumSizeOfQueueForStartCleaning();
                //! Получение периода, за который сборщик мусора в любом случае сделает очистку.
                /*! \return Возвращает число миллисекунд, составляющие период, раз в который сборщик мусора будет производить отработку.
                */
                unsigned int getAlwaysBeCleanedOncePerMillisecond();
                //! Установка времени периодичного пробуждения сборщика мусора для проверки на необходимость очистки.
                /*! \param value Устанавливаемое число миллисекунд, которые составляют период пробуждения сборщика мусора для проверки на необходимость очистки.
                */
                void setWakeUpGarbageCollectorOncePerMilliseconds(unsigned int value);
                //! Установка минимального размера очереди, при котором будет произведена очистка мусора.
                /*! \param value Устанавливаемый минимальный размер очереди, при котором будет произведена очистка мусора.
                */
                void setMaximumSizeOfQueueForStartCleaning(unsigned int value);
                //! Установка периода, за который сборщик мусора в любом случае сделает очистку.
                /*! \param value Устанавливаемое число миллисекунд, составляющие период, раз в который сборщик мусора будет производить отработку.
                */
                void setAlwaysBeCleanedOncePerMillisecond(unsigned int value);
                //! Получение прямого доступа к функциям и объектам движка SimpleVoxelEngine.
                /*! Данная функция выдаёт вызвавшему её потоку прямой доступ
                к функциям и объектам движка SimpleVoxelEngine. При попытке
                работы напрямую с объектами или при прямом вызове функций движка
                SimpleVoxelEngine без получения доступа возможны ошибки, так как
                движока SimpleVoxelEngine не является потокобезопасным. Исключение
                составляет пользовательская функция, а именно из UserControllerForThread.
                В ней можно работать с чем угодно, не получая доступ к движку, но при этом
                нельзя делать никаких действий в других потоках, получая этот доступ.
                Иначе доступ также требуется получить в пользовательской функции.
                Но, к примеру, этот доступ можно получить при входе в функцию, а
                отдать при выходе. Если это кажется слишком сложным, то просто все действия,
                которые требует прямого доступа к SimpleVoxelEngine, выполняйте в
                контроллере запуска пользовательских функций в другом потоке.
                */
                void getAccessToTheSimpleVoxelEngine();
                //! Возвращение полученного доступа.
                /*! Если доступ не вернуть, то движок зависнет.
                Поэтому, если функция сложная и имеет много точек выхода,
                рекомендуется получать блокировочный mutex и блокировать
                его с помощью lock_guard.
                */
                void freeAccessToTheSimpleVoxelEngine();
                //! Получение блокировочного mutex доступа к SimpleVoxelEngine по ссылке.
                /*! \return Возвращает ссылку на блокировочный mutex к SimpleVoxelEngine.
                */
                std::recursive_mutex& getLockAccessToTheSimpleVoxelEngine();
                //! Потокобезопасная функция получение времени.
                /*! \return Возвращает время аналогично getTime() в Irrlicht, но потокобезопасно. Рекомендуется использовать только её вместо getTime().
                */
                irr::u32 getTimeSafeThread();
                //! Потокобезопасная функция, являющаяся аналогом run() в IrrlichtDevice.
                /*! \return Возвращает результат отработки функции run() в IrrlichtDevice. Рекомендуется пользоваться только ей вместо run().
                */
                bool runIrrDevice();
                //! Установка SimpleVoxelObject по глобальным мировым координатам.
                /*! Вызов этой функции в другом потоке требует блокировку доступа при работе с любыми
                объектами или функциями SimpleVoxelEngine в контроллере запуска пользовательских функций в другом потоке.
                \param worldid Идентификатор мира, в котором происходит установка SimpleVoxelObject.
                \param vect Глобальный вектор (мировые координаты) устанавливаемого SimpleVoxelObject.
                \param svo Указатель на устанавливаемый SimpleVoxelObject.
                \param level Уровень глобальной мировой координатной сетки, по которому прописаны координаты. Данный параметр пока не работает. Вся установка происходит на первом уровне.
                */
                void setSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, int level = 1);
                //! Получение SimpleVoxelObject по глобальным мировым координатам.
                /*! Вызов этой функции в другом потоке требует блокировку доступа при работе с любыми
                объектами или функциями SimpleVoxelEngine в контроллере запуска пользовательских функций в другом потоке.
                \param worldid Идентификатор мира, из которого хотим получить SimpleVoxelObject.
                \param vect Глобальный вектор (мировые координаты) получаемого SimpleVoxelObject.
                \param isexists Если имеет значение true, то система логирования выдаст предупреждение при отсутствии SimpleVoxelObject по данному адресу, иначе не выдаст.
                \param level Уровень глобальной мировой координатной сетки, по которому прописаны координаты. Данный параметр пока не работает. Вся установка происходит на первом уровне.
                \return Возвращает указатель на SimpleVoxelObject, расположенный по заданным координатам или NULL в случае его отсутствия.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool isexists = true, int level = 1);
                //! Получение адреса SimpleVoxelObject по трём точкам в пространстве и вектору направления.
                /*! Вызов этой функции в другом потоке требует блокировку доступа при работе с любыми
                объектами или функциями SimpleVoxelEngine в контроллере запуска пользовательских функций в другом потоке или в главном потоке.
                Получает адрес SimpleVoxelObject по трём точкам, который он содержит, а также вектору, который в него направлен.
                \param worldid Идентификатор мира, из которого хотим получить SimpleVoxelObject.
                \param v1 Первая точка.
                \param v2 Вторая точка.
                \param v3 Третья точка.
                \param lookvect Направление взгляда (вектор, торчащий в воксель).
                \param changeposition Позиция, откуда изменяется воксель (к примеру, камеры).
                \param calibration Производить ли калибровку точек для точного попадания в решётку вокселей. По умолчанию true (если точки могут попасть на границ куба-вокселя, то необходимо значение true, например, для кубических вокселей).
                \param lssvo Тип поиска. Не используется.
                \param level Уровень глубины погружения при поиске. Не используется. Весь поиск идёт на первом уровне.
                \return Возвращает указатель на VoxelAddress объекта SimpleVoxelObject, который содержит три указанных точки, а также вектор направлен в него.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(int worldid, SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Additional::Vector3d<float> changeposition, bool calibration = true, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //Function controllers
                //! Контроллер главной очереди заданий. Запускается в другом потоке. Пользователю данная функция не нужна.
                void threadMainQueueTaskController();
                //! Контроллер очереди заданий для создания чанков. Запускается в другом потоке. Пользователю данная функция не нужна.
                void threadAddChunkTaskController();
                //! Контроллер очереди заданий для обновления чанков. Запускается в другом потоке. Пользователю данная функция не нужна.
                void threadUpdateChunkTaskController();
                //! Контроллер сборщика мусора. Запускается в другом потоке. Пользователю данная функция не нужна.
                void threadGarbageCollectorController();
                //! Контроллер для запуска пользовательских функция в другом потоке. Запускается в другом потоке. Пользователю данная функция не нужна.
                void threadUserControllerForThread();
                //! Получение IrrlichtDevice.
                /*! \return Возвращает указатель на IrrlichtDevice, используемый для отрисовки в данном враппере.
                */
                irr::IrrlichtDevice* getIrrlichtDevice();
                //! Получение блокировочного mutex для работы с любыми функциями движка, использующими таймер.
                /*! \return Возвращает mutex, который необходимо блокировать при вызове функций Irrlicht, которые используют таймер.
                */
                std::mutex& getLockGettingTimeFromDevice();
            private:
                irr::IrrlichtDevice* s_Device;
                FactoryChunkMeshSceneNode* s_FCMSN;
                void setModeMainThread(MODE_MAIN_THREAD);
                void copyFromStandbyQueueToMainQueue();
                bool testLockChunk(int, SimpleVoxelEngine::Additional::Vector3d<int>, bool waitunlock = false);
                bool testInRenderingQueueChunk(int, SimpleVoxelEngine::Additional::Vector3d<int>, bool waitunlock = false);
                bool testWaitingAllThreads(bool waitwaitingallthreads = false);
                void addTaskAddChunks(TaskUpdateChunk*);
                void addTaskUpdateChunks(TaskUpdateChunk*);
                void addTaskForUserControllerForThread(TaskRunThreadFunctionWithParameters*);
                void setUpdateChunksForChangeSimpleVoxelObject(int, SimpleVoxelEngine::Core::VoxelAddress*, SimpleVoxelEngine::Core::World*, bool);
                //Work functions
                void addTasksUpdateChunksInMainQueue(bool all = false);
                bool setSimpleVoxelObjectThread(int, SimpleVoxelEngine::Core::VoxelAddress*, SimpleVoxelEngine::Voxels::SimpleVoxelObject*, bool rewrite = false, bool immediatelyupdated = false);
                bool deleteSimpleVoxelObjectThread(int, SimpleVoxelEngine::Core::VoxelAddress*, bool immediatelyupdated = false);
                bool addChunksForRenderingThreadPreLoad(int, SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Additional::Vector3d<int>, bool load, bool loadframe, int sizeframe);
                bool deleteChunksForRenderingThreadUnload(int, SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Additional::Vector3d<int>, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //...
                ThreadSafeTypes::ThreadSafeVector< Task* > s_TaskMainQueue;
                std::mutex s_LockWorkTaskMainQueue;
                std::condition_variable s_CheckTaskMainQueue;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastMainQueue;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitMainQueue;
                std::thread::id s_IdThreadMainQueue;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskUpdateChunk* > s_TaskQueueAddChunks;
                std::mutex s_LockWorkTaskQueueAddChunks;
                std::condition_variable s_CheckTaskAddChunks;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueAddChunks;
                unsigned int s_MaxNumberOfThreadAddChunks;
                ThreadSafeTypes::ThreadSafeType<bool>* s_FlagsRunningThreadsAddChunks;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitAddChunksQueue;
                std::thread::id s_IdThreadAddChunksQueue;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskUpdateChunk* > s_TaskQueueUpdateChunks;
                std::mutex s_LockWorkTaskQueueUpdateChunks;
                std::condition_variable s_CheckTaskUpdateChunks;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueUpdateChunks;
                unsigned int s_MaxNumberOfThreadUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>* s_FlagsRunningThreadsUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>  s_FlagWaitUpdateChunksQueue;
                std::thread::id s_IdThreadUpdateChunksQueue;
                //...
                std::map< int, std::map<SimpleVoxelEngine::Additional::Vector3d<int>, ThreadSafeTypes::ThreadSafeCounter<int> > > s_LockChunks;
                std::map< int, std::map<SimpleVoxelEngine::Additional::Vector3d<int>, ThreadSafeTypes::ThreadSafeType<bool>  > > s_InRenderingQueueChunk;
                //Controllers
                //...
                std::thread s_ThreadMainQueueTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadMainQueueTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadMainQueueTaskControllerRun;
                //...
                std::thread s_ThreadAddChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadAddChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadAddChunkTaskControllerRun;
                //...
                std::thread s_ThreadUpdateChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadUpdateChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadUpdateChunkTaskControllerRun;
                //...
                std::thread s_ThreadGarbageCollectorController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadGarbageCollectorController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadGarbageCollectorControllerRun;
                std::mutex s_LockWorkGarbageCollector;
                std::condition_variable s_CheckGarbageCollector;
                unsigned int s_WakeUpGarbageCollectorOncePerMilliseconds;
                unsigned int s_MaximumSizeOfQueueForStartCleaning;
                unsigned int s_AlwaysBeCleanedOncePerMillisecond;
                ThreadSafeTypes::ThreadSafeType<bool>  s_WaitingForCleaning;
                std::thread::id s_IdThreadGarbageCollector;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskRunThreadFunctionWithParameters* > s_TaskQueueForUserControllerForThread;
                std::mutex s_LockWorkTaskQueueForUserControllerForThread;
                std::condition_variable s_CheckTaskQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitQueueForUserControllerForThread;
                std::thread::id s_IdThreadQueueForUserControllerForThread;
                std::thread s_ThreadQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadQueueForUserControllerForThreadRun;
                //...
                MODE_MAIN_THREAD s_ModeMainThread;
                ThreadSafeTypes::ThreadSafeVector< Task* > s_TaskMainQueueForStandbyMode;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StartUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>  s_AllChunksForUpdate;
                ThreadSafeTypes::ThreadSafeType<bool>  s_MainThreadReadyForCleaning;
                std::recursive_mutex s_LockAccessToTheSimpleVoxelEngine;
                std::mutex s_LockGettingTimeFromDevice;
                std::thread::id s_IdMainThread;
                UserControllerForThread* s_UserControllerForThread;
        };
    };
};

/*! \file IrrSimpleVoxelEngine.h
	\brief Главный заголовочный файл. Необходимо присоединять только его к проекту.
*/

#endif
