#ifndef IrrSimpleVoxelEngine_H
#define IrrSimpleVoxelEngine_H

#include <SimpleVoxelEngine.h>
#include <irrlicht.h>

//ThreadSafeVector/ThreadSafeCounter/ThreadSafeType удалены: враппер ими не пользуется (см. changesIrrSVE.txt).
//Заголовки для новой поточной схемы (очереди std::deque, наборы для защиты от дублей, пул потоков).
#include <atomic>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "FactoryChunkMeshSceneNode.h"
#include "ChunkStreamingPool.h" //Добавлено: стриминг чанков.
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
                /*! Только из главного потока (из пользовательской функции UserControllerForThread - нельзя, вернёт -1).
                \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld(World3DInfo w3dinfo = World3DInfo());
                //! Создание мира с указанными параметрами.
                /*! Только из главного потока (из пользовательской функции UserControllerForThread - нельзя, вернёт -1).
                \param wcp Параметры создания мира
                \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld(SimpleVoxelEngine::Core::WorldCreatingParameters& wcp, World3DInfo w3dinfo = World3DInfo());
                //! Добавление мира к движку.
                /*! Только из главного потока. Мир должен быть создан для этого движка и ещё не добавлен (id == -1).
                \param wrld Добавляемый мир.
                \param w3dinfo Информация о положении мира и его чанков в пространстве. По умолчанию ничего не изменено.
                \return Возвращает идентификатор добавленного мира (wrld) или -1 в случае неудачи.
                */
                int addWorld(SimpleVoxelEngine::Core::World* wrld, World3DInfo w3dinfo = World3DInfo());
                //! Установка SimpleVoxelObject по VoxelAddress.
                /*! Потокобезопасна. Из пользовательской функции UserControllerForThread (поток главной очереди) выполняется сразу,
                из остальных потоков - ставится в главную очередь заданий. Адрес и svo забирает движок (при ошибке удаляет сам).
                Воксель не ставится (svo удаляется), если к моменту выполнения чанк не загружен (например, его уже выгрузили -
                предупреждение в логе) или вектор адреса выходит за разбиение своего чанка (ошибка в логе).
                \param id Идентификатор мира, в котором устанавливаем SimpleVoxelObject.
                \param vxladdr Адрес вокселя.
                \param svo Устанавливаемый SimpleVoxelObject.
                \param rewrite Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования.
                \param immediatelyupdated Если имеет значение true, то чанк будет обновлён немедленно.
                */
                void setSimpleVoxelObject(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool immediatelyupdated = false);
                //! Удаление SimpleVoxelObject по VoxelAddress.
                /*! Потокобезопасна, выполняется так же, как setSimpleVoxelObject() (сразу или заданием главной очереди), адрес
                забирает движок. Если к моменту выполнения чанк не загружен - предупреждение в логе, удаления нет.
                \param id Идентификатор мира, в котором удаляем SimpleVoxelObject.
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
                /*! Чанки области и рамки за границами мира (World::setChunksBound()) пропускаются без сообщений.
                \param id Идентификатор мира, в котором расположены чанки.
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
                //! Получение пула контроллеров стриминга чанков.
                /*! Пул один на движок, создаётся вместе с ним и удаляется в деструкторе. Через него чанки загружаются и
                выгружаются вокруг игроков или камер (см. ChunkStreamingPool, ChunkStreamingController); каждый кадр в главном
                потоке вызывайте getChunkStreamingPool()->step().
                \return Возвращает указатель на пул (никогда не NULL).
                */
                ChunkStreamingPool* getChunkStreamingPool();
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
                /*! По умолчанию контроллера нет: пока его не установить, функции addTaskRunUserControllerForThread() не вызываются.
                Установленный контроллер можно в любой момент заменить другим или убрать (NULL).
                Если deletebyengine == true, контроллер принадлежит движку: движок удалит его сам, когда контроллер
                заменят другим, или в деструкторе. Если false (например, свой пул контроллеров, которые подменяются
                туда-сюда), движок контроллер никогда не удаляет: удалить его можно, когда getUserControllerForThread()
                уже возвращает другой контроллер (и замена на него не стоит в очереди), или после остановки движка.
                До start() контроллер меняется сразу. Во время работы замена ставится в главную очередь: функции,
                поставленные до вызова, выполнятся со старым контроллером, после - с новым. Можно вызывать и из самой
                пользовательской функции. getUserControllerForThread() вернёт новый контроллер, когда до замены дойдёт очередь.
                \param usrcntrlforthread Указатель на устанавливаемый контроллер. NULL - убрать контроллер: функции,
                поставленные после этого addTaskRunUserControllerForThread(), не вызываются (событие об ошибке, параметры удаляются).
                \param deletebyengine true - контроллером владеет движок и удаляет его сам; false - контроллером владеет пользователь.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setUserControllerForThread(UserControllerForThread* usrcntrlforthread, bool deletebyengine = true);
                //! Обновление всех объектов движка.
                /*! Вызывать каждый кадр в главном потоке (до или после drawAll()). Готовые меши чанков становятся нодами,
                у полупрозрачных уровней сортируются треугольники, запоминаются положения родительских нод миров (для
                getWorldCoordinates() из других потоков) и время для getTimeSafeThread().
                Occlusion Queries здесь не выполняются (вне кадра они дают неверный результат) - для них есть runOcclusionQueries().
                */
                void update();
                //! Проверка видимости нод чанков через Occlusion Queries.
                /*! Нужна, только если включено FactoryChunkMeshSceneNode::setOcclusionQueries(true). Вызывать каждый кадр в главном
                потоке между smgr->drawAll() и driver->endScene(): проверка рисует ноды в уже заполненный кадром Z-буфер.
                Раз в FactoryChunkMeshSceneNode::getTimerTestOcclusionQueriesInMilliseconds() миллисекунд выполняются все
                Occlusion Queries драйвера (runAllOcclusionQueries() и updateAllOcclusionQueries() - в том числе ваши собственные),
                и ноды чанков, закрытые другими нодами, скрываются до следующей проверки.
                */
                void runOcclusionQueries();
                //! Вызвана ли функция из главного потока.
                /*! Главный поток - тот, в котором создан движок (окно Irrlicht, рисование, update()).
                \return Возвращает true, если текущий поток - главный.
                */
                bool isMainThread();
                //! Регистрация событий.
                /*! Переопределяет SimpleVoxelEngine::Core::Engine::registerEvent(): сообщения ядра и враппера из всех потоков (главного,
                главной очереди, потоков построения) выводятся по одному - перед передачей логгеру берётся блокировка вывода лога.
                Сообщения тех типов, которые логгер не выводит, отбрасываются без блокировки. Можно вызывать из любого потока.
                \param TypeEvent Тип регистрируемого события.
                \param TextEvent Текст сообщения для логгера.
                \param IsFatal Если имеет значения true, то событие является фатальным.
                */
                void registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal = false) override;
                //Удалено: getModeMainThread() - режима STANDBY больше нет, функция всегда возвращала NORMAL.
                //! Функция для старта движка.
                /*! Данную функцию надо вызывать для запуска движка. Инициализирует потоки.
                Фабрика чанков к этому моменту должна быть установлена. Задания, добавленные
                до start(), будут выполнены после запуска. После остановки (isRun() == false) движок можно запустить снова:
                задания, поставленные после stop(), выполнятся после нового start().
                Если операционная система не дала создать поток построения (нехватка ресурсов), движок работает с теми потоками,
                которые удалось создать (ошибка в логе); если не создан поток главной очереди - движок не запускается.
                */
                void start();
                //! Функция для остановки движка.
                /*! Все задания, добавленные до вызова stop(), будут выполнены.
                Можно вызывать из нескольких потоков (вызовы выполняются по очереди) и повторно: stop(true) после stop(false)
                дожидается завершения потоков. Нельзя вызывать из пользовательской функции UserControllerForThread, из функций
                фабрики (потоки построения) и под захваченной блокировкой движка - stop(true) ждал бы сам себя.
                \param wait Если имеет значение true, то ожидается завершение работы всех потоков, иначе не ожидается
                (движок и в этом случае можно сразу удалять: деструктор дождётся завершения потоков).
                */
                void stop(bool wait = true);
                //! Функция проверки работы движка.
                /*! \return Возвращает true, если движок работает, иначе false,
                */
                bool isRun();
                //! Установка максимального числа потоков для обработки заданий по созданию чанков.
                /*! Вызывать только до start(). Потоки создания строят меши только что добавленных чанков.
                \param newvaluemaxnumberofthread Новое значение максимального количества потоков для создания чанков (не меньше 1).
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setMaxNumberOfThreadAddChunks(unsigned int newvaluemaxnumberofthread);
                //! Получение максимального числа потоков для обработки заданий по созданию чанков.
                /*! \return Возвращает максимальное число потоков для обработки заданий по созданию чанков.
                */
                unsigned int getMaxNumberOfThreadAddChunks();
                //! Установка максимального числа потоков для обработки заданий по обновлению чанков.
                /*! Вызывать только до start(). Потоки обновления в первую очередь перестраивают изменённые
                чанки, а когда таких нет - помогают потокам создания.
                \param newvaluemaxnumberofthread Новое значение максимального количества потоков для обновления чанков (не меньше 1).
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setMaxNumberOfThreadUpdateChunks(unsigned int newvaluemaxnumberofthread);
                //! Получение максимального числа потоков для обработки заданий по обновлению чанков.
                /*! \return Возвращает максимальное число потоков для обработки заданий по обновлению чанков.
                */
                unsigned int getMaxNumberOfThreadUpdateChunks();
                //! Добавление задания в очередь.
                /*! Можно вызывать из любого потока. Задания выполняются строго по порядку в потоке главной очереди.
                Задания STOP и STREAM_CHUNKS сюда не передавайте: STOP ставит только stop(), STREAM_CHUNKS - только пул стриминга
                (ChunkStreamingPool). STOP в отладочной сборке (SVE_DEBUG_CHECKS) отклоняется (ошибка в логе, задание удаляется); в
                релизе проверки нет - движок остановился бы без stop(), и задания, поставленные после STOP, не выполнялись бы до
                следующего start(). STREAM_CHUNKS, поставленное вручную, сбило бы счётчик действий пула стриминга.
                \param _task Указатель на задание, которое добавляется в очередь. Память, занимаемая этим объектом, будет очищена движком.
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
                //! Сохранение всех загруженных чанков мира. Потокобезопасная, можно вызывать в любой момент игры.
                /*! Сохраняет чанки контроллер загрузки и сохранения мира (World::setChunksSaveLoadController()) - так же, как
                World::saveAllChunks(), но безопасно во время работы движка: сохранение - отдельное задание главной очереди
                (SAVE_CHUNKS). Оно выполняется под блокировкой движка по порядку после всех заданий, поставленных раньше
                (установка блоков, загрузка и выгрузка чанков), поэтому в сохранение попадают все действия до вызова, а пока
                оно идёт, мир никто не меняет. Рисование (update()) при этом не останавливается, задерживается только
                построение новых мешей. UserControllerForThread не нужен.
                Если stop() уже вызван, но очередь ещё работает, задание ставится перед STOP: в сохранение попадают все задания,
                поставленные до вызова (и до stop()).
                Сразу, в вызывающем потоке (результат готов к возврату), сохранение выполняется, если функция вызвана из
                пользовательской функции UserControllerForThread (поток главной очереди - иначе задание ждало бы само себя),
                если движок не запущен (задания, поставленные до start(), в такое сохранение не попадут) и если поток очереди
                уже завершился после stop() (все задания до stop() к этому моменту выполнены).
                При выходе можно вызвать saveAllChunks(id), а потом stop(): stop() сначала выполнит все задания очереди.
                \param worldid Идентификатор мира.
                \param wait true - дождаться окончания сохранения. Нельзя ждать, держа блокировку движка
                (getAccessToTheSimpleVoxelEngine(), getLockAccessToTheSimpleVoxelEngine()): задание её не получит - зависание.
                \return Возвращает номер сохранения (больше 0) - по нему isSaveCompleted() сообщает, закончено ли сохранение
                и успешно ли оно.
                */
                unsigned long saveAllChunks(int worldid, bool wait = false);
                //! Проверка, закончено ли сохранение.
                /*! Потокобезопасная. Помнятся результаты последних 1000 законченных сохранений.
                \param savenumber Номер сохранения, который вернула saveAllChunks().
                \param result Если не NULL и сохранение закончено, сюда записывается его результат: true - все чанки
                сохранены; false - мира нет, контроллер сохранения не установлен, какой-то чанк не сохранился или задание
                удалено невыполненным (движок удалён раньше, чем очередь до него дошла).
                \return Возвращает true, если сохранение закончено; false - ещё идёт (или номер неизвестен).
                */
                bool isSaveCompleted(unsigned long savenumber, bool* result = 0);
                //Удалены get/setWakeUpGarbageCollectorOncePerMilliseconds, get/setMaximumSizeOfQueueForStartCleaning,
                //get/setAlwaysBeCleanedOncePerMillisecond - настройки сборщика мусора, которого больше нет.
                //! Получение прямого доступа к функциям и объектам движка SimpleVoxelEngine.
                /*! Захватывает рекурсивный mutex движка. Пока он захвачен, ни один другой поток
                враппера не читает и не меняет данные SimpleVoxelEngine. Нужен, если вы напрямую
                работаете с объектами SimpleVoxelEngine (World, ChunkVoxelObject, SimpleVoxelObject...)
                или храните полученный от движка указатель на SimpleVoxelObject.
                Все функции самого враппера захватывают этот mutex сами, поэтому их можно вызывать
                как с захваченным доступом, так и без него.
                Пользовательская функция UserControllerForThread выполняется в потоке главной очереди
                заданий: пока она работает, мир никто, кроме неё, не меняет, поэтому указатели,
                полученные в ней, действительны до её завершения. Потоки построения мешей при этом
                могут читать мир, так что при прямой работе с объектами SimpleVoxelEngine (в обход
                функций враппера) доступ нужно получить и в ней.
                Внутри захваченного доступа нельзя ждать другие потоки враппера (например, вызывать stop()).
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
                /*! Возвращает время таймера Irrlicht, запомненное последним runIrrDevice() или update() (точность - один кадр).
                Время обновляет и update(), поэтому с device->run() вместо runIrrDevice() оно тоже идёт (раз в кадр).
                Можно вызывать из любого потока, в том числе из обработчика событий (OnEvent): блокировок нет.
                \return Возвращает время аналогично getTime() в Irrlicht, но потокобезопасно. Рекомендуется использовать только её вместо getTime().
                */
                irr::u32 getTimeSafeThread();
                //! Потокобезопасная функция, являющаяся аналогом run() в IrrlichtDevice.
                /*! Вызывать в главном потоке (потоке Irrlicht). Заодно запоминает время для getTimeSafeThread() (его запоминает
                и update(), поэтому с device->run() движок тоже работает - время тогда обновляется раз в кадр).
                \return Возвращает результат отработки функции run() в IrrlichtDevice. Рекомендуется пользоваться только ей вместо run().
                */
                bool runIrrDevice();
                //! Установка SimpleVoxelObject по глобальным мировым координатам.
                /*! Потокобезопасна. Из пользовательской функции UserControllerForThread выполняется сразу,
                из остальных потоков - ставится в главную очередь заданий. Чанк не обновляется сразу, а помечается
                для обновления (затем вызовите addTaskUpdateChunks()).
                Глобальные координаты считаются по сетке getSizeChunk() мира: в чанке с другим разбиением (генератор или
                контроллер загрузки задали другое число ячеек) воксель не ставится - ошибка в логе, svo удаляется.
                \param worldid Идентификатор мира, в котором происходит установка SimpleVoxelObject.
                \param vect Глобальный вектор (мировые координаты) устанавливаемого SimpleVoxelObject.
                \param svo Указатель на устанавливаемый SimpleVoxelObject.
                \param level Уровень глобальной мировой координатной сетки, по которому прописаны координаты. Данный параметр пока не работает. Вся установка происходит на первом уровне.
                */
                void setSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, int level = 1);
                //! Получение SimpleVoxelObject по глобальным мировым координатам.
                /*! Потокобезопасна. Возвращённый указатель гарантированно действителен только внутри пользовательской
                функции UserControllerForThread или пока захвачен доступ getAccessToTheSimpleVoxelEngine().
                В чанке, разбиение которого не равно getSizeChunk(), глобальные координаты не определены - возвращается NULL
                (при isexists == true - с ошибкой в логе).
                \param worldid Идентификатор мира, из которого хотим получить SimpleVoxelObject.
                \param vect Глобальный вектор (мировые координаты) получаемого SimpleVoxelObject.
                \param isexists Если имеет значение true, то система логирования выдаст предупреждение при отсутствии SimpleVoxelObject по данному адресу, иначе не выдаст.
                \param level Уровень глобальной мировой координатной сетки, по которому прописаны координаты. Данный параметр пока не работает. Вся установка происходит на первом уровне.
                \return Возвращает указатель на SimpleVoxelObject, расположенный по заданным координатам или NULL в случае его отсутствия.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool isexists = true, int level = 1);
                //! Получение адреса SimpleVoxelObject по трём точкам в пространстве и вектору направления.
                /*! Потокобезопасна. Возвращённый адрес необходимо удалить самому (или отдать в set/deleteSimpleVoxelObject).
                Может вернуть NULL, а также адрес только из одного вектора (чанка), если воксель не найден.
                Получает адрес SimpleVoxelObject по трём точкам, который он содержит, а также вектору, который в него направлен.
                \param worldid Идентификатор мира, из которого хотим получить SimpleVoxelObject.
                \param v1 Первая точка.
                \param v2 Вторая точка.
                \param v3 Третья точка.
                \param lookvect Направление взгляда (вектор, торчащий в воксель).
                \param changeposition Позиция, откуда изменяется воксель (к примеру, камеры).
                \param calibration Производить ли калибровку точек для точного попадания в решётку вокселей. По умолчанию true (если точки могут попасть на границ куба-вокселя, то необходимо значение true, например, для кубических вокселей).
                \param lssvo Тип поиска: LASTLEVEL - спуск до вокселя, который не чанк, или до пустой ячейки; NUMBERLEVEL - до уровня level.
                \param level Уровень глубины погружения при поиске (для NUMBERLEVEL).
                Найденная ячейка прилегает к треугольнику, но вдоль грани берётся место у центра треугольника. Чтобы найти
                ячейку точно под курсором (например, маленький блок рядом с большой гранью), используйте getAddressSimpleVoxelObjectByPoint().
                \return Возвращает указатель на VoxelAddress объекта SimpleVoxelObject, который содержит три указанных точки, а также вектор направлен в него.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(int worldid, SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Additional::Vector3d<float> changeposition, bool calibration = true, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! Получение адреса SimpleVoxelObject под точкой в пространстве.
                /*! Потокобезопасна. Возвращённый адрес необходимо удалить самому (или отдать в set/deleteSimpleVoxelObject).
                Точка (например, точка пересечения луча камеры с гранью) сдвигается вдоль lookvect на 0.001 размера блока
                первого уровня, и находится ячейка, в которой она оказалась: lookvect от камеры - сам блок под курсором,
                к камере - ячейка для установки блока ровно в этом месте грани (на любой глубине вложенных чанков).
                \param worldid Идентификатор мира.
                \param point Точка в координатах сцены (как у getAddressSimpleVoxelObjectByThreePoints()).
                \param lookvect Направление, в котором искать ячейку от точки.
                \param changeposition Позиция, откуда изменяется воксель (к примеру, камеры) - нужна для пересчёта направления.
                \param lssvo Тип поиска: LASTLEVEL - спуск до вокселя, который не чанк, или до пустой ячейки; NUMBERLEVEL - до уровня level.
                \param level Уровень глубины погружения при поиске (для NUMBERLEVEL).
                \return Возвращает адрес вокселя или пустой ячейки; адрес без векторов, если чанк не загружен; NULL при ошибке.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByPoint(int worldid, SimpleVoxelEngine::Additional::Vector3d<float> point, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Additional::Vector3d<float> changeposition, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! Пересчёт точки из координат сцены в координаты мира.
                /*! Учитывает положение, поворот и масштаб родительской ноды мира (вместе с её родителями) и сдвиг начала
                координат мира (World3DInfo), в том числе у мира без родительской ноды. В главном потоке считается по текущему
                положению нод, в остальных потоках - по положению, запомненному последним update() (ноды сцены меняет главный поток).
                Это те координаты, в которых ищут getAddressSimpleVoxelObjectByThreePoints() и getAddressSimpleVoxelObjectByPoint():
                при стандартных масштабах блок первого уровня с глобальными координатами k занимает [k - 0.5, k + 0.5).
                Направление пересчитывается как разность пересчитанных точек.
                \param worldid Идентификатор мира.
                \param scenepoint Точка в координатах сцены.
                \return Возвращает точку в координатах мира (без фабрики или для неизвестного мира - ту же точку).
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getWorldCoordinates(int worldid, SimpleVoxelEngine::Additional::Vector3d<float> scenepoint);
                //Точки входа потоков (threadMainQueueTaskController и др.) перенесены в private: пользователю они не нужны.
                //! Получение IrrlichtDevice.
                /*! \return Возвращает указатель на IrrlichtDevice, используемый для отрисовки в данном враппере.
                */
                irr::IrrlichtDevice* getIrrlichtDevice();
                //Удалено: getLockGettingTimeFromDevice() - мьютекс таймера больше не используется (см. getTimeSafeThread()).
            private:
                //Фабрике нужен доступ к блокировке движка и очередям построения (снимок чанка, перестройка соседей).
                friend class FactoryChunkMeshSceneNode;
                //Добавлено: пулу стриминга нужна отладочная проверка главного потока.
                friend class ChunkStreamingPool;
                //! Задание на построение меша одного чанка (для потоков построения).
                struct BuildJob
                {
                    int s_WorldId;
                    SimpleVoxelEngine::Additional::Vector3d<int> s_VectorChunk;
                };
                irr::IrrlichtDevice* s_Device;
                FactoryChunkMeshSceneNode* s_FCMSN;
                //Изменено: atomic - во время работы контроллер меняет поток главной очереди, а getUserControllerForThread() читает любой поток.
                std::atomic<UserControllerForThread*> s_UserControllerForThread;
                //Добавлено: удалять ли текущий контроллер при замене и в деструкторе (false - контроллер пользователя, например из пула).
                //Меняется там же, где s_UserControllerForThread (во время работы - только в потоке главной очереди).
                bool s_DeleteUserControllerForThread;
                //! Вызвана ли функция из потока главной очереди (тогда изменения мира выполняются сразу, без постановки в очередь).
                /*! \return Возвращает true, если текущий поток - поток главной очереди.
                */
                bool isMainQueueThread();
                //! Выполнение одного задания главной очереди (в потоке главной очереди).
                /*! \param task Задание.
                */
                void processMainTask(Task*);
                //! Удаление задания вместе с тем, чем оно владеет (адрес, воксель, параметры пользователя).
                /*! Невыполненное сохранение сообщает ждущим об ошибке (isSaveCompleted()).
                \param task Задание.
                */
                void deleteTask(Task*);
                //! Выполнение задания пула стриминга в потоке главной очереди.
                /*! Действия - по порядку. Загрузка и выгрузка - под блокировкой движка на каждый чанк, отрисовка - сначала все
                чанки задания регистрируются, потом ставятся на построение; соседи загруженного или выгруженного чанка, которые
                отрисовываются, перестраиваются (на стыке открываются или закрываются грани). Результат каждого действия
                сообщается пулу (ChunkStreamingPool::onActionDone()) уже без блокировки движка.
                \param task Задание.
                */
                void processStreamTask(TaskStreamChunks* task);
                //! Постановка чанка в очередь построения меша (чанк, который уже ждёт в этой очереди, второй раз не ставится).
                /*! \param id Идентификатор мира.
                \param vect Вектор (координаты) чанка.
                \param update true - изменённый чанк (очередь обновления), false - новый (очередь создания).
                */
                void requestBuildChunk(int, SimpleVoxelEngine::Additional::Vector3d<int>, bool update);
                //! Цикл потока построения мешей.
                /*! \param updatepool true - поток пула обновления: сначала берёт изменённые чанки, а когда их нет - новые.
                */
                void threadBuildChunksController(bool updatepool);
                //! Цикл потока главной очереди: выполняет задания строго по порядку, после STOP останавливает и потоки построения.
                void threadMainQueueTaskController();
                //! Адрес вокселя первого уровня по глобальным координатам (с округлением вниз для отрицательных координат).
                /*! \param vect Глобальный вектор вокселя.
                \param sizechunk Размер ребра чанка мира.
                \return Возвращает новый адрес из двух векторов (чанк, воксель в чанке); удаляет вызывающий.
                */
                SimpleVoxelEngine::Core::VoxelAddress* createVoxelAddressByGlobalVector(SimpleVoxelEngine::Additional::Vector3d<int>, int sizechunk);
                //! Пометка чанков на обновление после изменения вокселя: сам чанк, а если воксель у края - и соседние чанки.
                /*! \param id Идентификатор мира.
                \param vxladdr Адрес изменённого вокселя.
                \param wrld Мир.
                \param immediatelyupdated true - поставить чанки на перестройку сразу, иначе только пометить (addTaskUpdateChunks()).
                */
                void setUpdateChunksForChangeSimpleVoxelObject(int, SimpleVoxelEngine::Core::VoxelAddress*, SimpleVoxelEngine::Core::World*, bool);
                //Work functions
                //! Установка вокселя в мир под блокировкой движка (в потоке главной очереди).
                /*! Адрес проверяется до передачи ядру (чанк загружен, векторы внутри разбиения).
                \param id Идентификатор мира.
                \param vxladdr Адрес (не удаляется).
                \param svo Воксель (при ошибке удаляется).
                \param rewrite Перезаписывать ли существующий воксель.
                \param immediatelyupdated Перестроить ли чанк сразу (иначе - пометка для обновления).
                \param requiredpartition Если больше 0 - воксель ставится, только если разбиение чанка верхнего уровня равно этому
                числу (для *ByGlobalVector: глобальные координаты считаются по getSizeChunk()).
                \return Возвращает true, если воксель установлен. При false причина уже записана в лог.
                */
                bool setSimpleVoxelObjectThread(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool immediatelyupdated = false, int requiredpartition = 0);
                //! Удаление вокселя из мира под блокировкой движка (в потоке главной очереди).
                /*! Адрес проверяется так же, как в setSimpleVoxelObjectThread().
                \param id Идентификатор мира.
                \param vxladdr Адрес (не удаляется).
                \param immediatelyupdated Перестроить ли чанк сразу (иначе - пометка для обновления).
                \param requiredpartition То же, что в setSimpleVoxelObjectThread().
                \return Возвращает true, если воксель удалён.
                */
                bool deleteSimpleVoxelObjectThread(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, bool immediatelyupdated = false, int requiredpartition = 0);
                //! Установка вокселя сразу (из потока главной очереди) или заданием главной очереди.
                /*! Общая часть setSimpleVoxelObject() и setSimpleVoxelObjectByGlobalVector().
                \param id Идентификатор мира.
                \param vxladdr Адрес (забирается).
                \param svo Воксель (забирается).
                \param rewrite Перезаписывать ли существующий воксель.
                \param immediatelyupdated Перестроить ли чанк сразу.
                \param requiredpartition То же, что в setSimpleVoxelObjectThread() (0 - без проверки разбиения).
                */
                void setSimpleVoxelObjectAnyThread(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated, int requiredpartition);
                //! Удаление вокселя сразу (из потока главной очереди) или заданием главной очереди.
                /*! Общая часть deleteSimpleVoxelObject() и setSimpleVoxelObjectByGlobalVector() с svo == NULL.
                \param id Идентификатор мира.
                \param vxladdr Адрес (забирается).
                \param immediatelyupdated Перестроить ли чанк сразу.
                \param requiredpartition То же, что в setSimpleVoxelObjectThread() (0 - без проверки разбиения).
                */
                void deleteSimpleVoxelObjectAnyThread(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, bool immediatelyupdated, int requiredpartition);
                //! Загрузка/генерация чанков области (и рамки) перед построением - в потоке главной очереди.
                /*! Если область добавить нельзя совсем (нет мира или фабрики, размер рамки меньше 1), fatal становится true.
                \param id Идентификатор мира.
                \param vectleftdown Левый нижний угол области.
                \param vectrightup Правый верхний угол области.
                \param load Загружать ли отсутствующие в памяти чанки области.
                \param loadframe Загружать ли рамку.
                \param sizeframe Размер рамки.
                \param fatal Сюда пишется true, если область вообще нельзя добавить (нет мира или фабрики, неверный размер рамки) -
                тогда чанки не регистрируются для отрисовки.
                \return Возвращает true, если все чанки загружены.
                */
                bool addChunksForRenderingThreadPreLoad(int id, SimpleVoxelEngine::Additional::Vector3d<int> vectleftdown, SimpleVoxelEngine::Additional::Vector3d<int> vectrightup, bool load, bool loadframe, int sizeframe, bool& fatal);
                //! Удаление чанков области из отрисовки и (по флагам) выгрузка их и рамки из памяти - в потоке главной очереди.
                /*! Отрисованные соседи выгруженных чанков перестраиваются (на стыке открываются грани).
                \param id Идентификатор мира.
                \param vectleftdown Левый нижний угол области.
                \param vectrightup Правый верхний угол области.
                \param unload Выгружать ли чанки области из памяти.
                \param save Сохранять ли выгружаемые чанки.
                \param unloadframe Выгружать ли рамку вокруг области (чанки рамки, которые сами отрисовываются, не выгружаются).
                \param sizeframe Толщина рамки.
                \return Возвращает false, если мира или фабрики нет либо какой-то чанк не выгрузился.
                */
                bool deleteChunksForRenderingThreadUnload(int, SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Additional::Vector3d<int>, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //Главная очередь заданий (обрабатывается одним потоком строго по порядку).
                std::deque< Task* > s_TaskMainQueue;
                std::mutex s_LockMainQueue;
                std::condition_variable s_CheckMainQueue; //!< Ожидание заданий потоком очереди - с таймаутом THREAD_WAIT_INTERVAL_IN_MILLISECONDS, без notify.
                std::thread s_ThreadMainQueueTaskController;
                std::atomic<std::thread::id> s_IdThreadMainQueue; //!< atomic - пишется в start() и потоком очереди при выходе, читается любым потоком (isMainQueueThread()).
                //Удалено поле s_IdMainThread: его читала только старая проверка потока в addMainTask(). (Потом добавлено
                //снова, ниже, - для isMainThread().)
                //Очереди построения мешей чанков (обрабатываются пулами потоков).
                std::deque< BuildJob > s_QueueAddChunks;
                std::deque< BuildJob > s_QueueUpdateChunks;
                //Какие чанки уже стоят в очередях построения - чтобы 1000 правок одного чанка дали одно построение.
                std::set< std::pair< int, SimpleVoxelEngine::Additional::Vector3d<int> > > s_PendingAddChunks;
                std::set< std::pair< int, SimpleVoxelEngine::Additional::Vector3d<int> > > s_PendingUpdateChunks;
                std::mutex s_LockBuildQueues; //Защищает обе очереди построения, наборы выше и s_StopBuildThreads.
                //Изменено: потоки пулов ждут работу с таймаутом и их никто не будит (notify_one() и notify_all()
                //в winpthreads из MinGW 8.1 иногда намертво зависают).
                std::condition_variable s_CheckAddChunks; //!< Ожидание работы потоками пула создания - с таймаутом THREAD_WAIT_INTERVAL_IN_MILLISECONDS.
                std::condition_variable s_CheckUpdateChunks; //!< Ожидание работы потоками пула обновления - с таймаутом THREAD_WAIT_INTERVAL_IN_MILLISECONDS.
                std::vector< std::thread > s_ThreadsBuildChunks;
                bool s_StopBuildThreads; //Ставится потоком главной очереди после задания STOP.
                unsigned int s_MaxNumberOfThreadAddChunks;
                unsigned int s_MaxNumberOfThreadUpdateChunks;
                std::atomic<int> s_NumberOfRunningThreads; //Сколько потоков движка ещё работает (isRun() и stop(false)).
                std::atomic<bool> s_StopRequested; //STOP уже поставлен в очередь (второй не ставится). Изменено: ставится под s_LockMainQueue вместе с заданием STOP.
                bool s_MainQueueAlive; //!< Поток главной очереди запущен и ещё не взял задание STOP (под s_LockMainQueue) - поставленное задание будет выполнено.
                std::mutex s_LockStop; //!< join()/detach() потоков в stop() и start() - по очереди (stop() из двух потоков сразу присоединял бы один std::thread дважды).
                //! Присоединение потоков движка, которые ещё не присоединены и не отсоединены.
                /*! Вызывается под s_LockStop: из stop(true), start() и деструктора. Поток, который уже закончил работу,
                присоединяется сразу; объект std::thread, оставшийся joinable, при удалении или присваивании вызвал бы std::terminate.
                */
                void joinThreads();
                std::mutex s_LockLog; //!< Блокировка вывода лога (registerEvent()): одно сообщение выводится целиком.
                ChunkStreamingPool* s_ChunkStreamingPool; //!< Пул контроллеров стриминга чанков (создаётся в конструкторе).
                std::atomic<irr::u32> s_TimeFromDevice; //Время таймера Irrlicht после последнего runIrrDevice().
                std::thread::id s_IdMainThread; //!< Поток, в котором создан движок (главный поток), - для isMainThread().
                //! Отладочная проверка "вызвано из главного потока" (определена только при SVE_DEBUG_CHECKS).
                /*! Нарушение правила потоков (update(), addWorld(), настройки материалов и всё, что трогает сцену Irrlicht, -
                только из главного потока) даёт случайные падения далеко от места ошибки. С проверкой - сразу ошибка в логе.
                \param where Имя проверяющей функции - для сообщения в логе.
                \return Возвращает true, если поток главный; иначе пишет ошибку в лог и возвращает false (функция не выполняется).
                */
                bool debugCheckMainThread(const char* where);
                //! Сохранение всех чанков мира под блокировкой движка (для saveAllChunks() и задания SAVE_CHUNKS).
                /*! \param worldid Идентификатор мира.
                \return Возвращает результат World::saveAllChunks(); false, если мира нет.
                */
                bool saveAllChunksThread(int worldid);
                //! Запись результата сохранения и пробуждение ждущих.
                /*! \param savenumber Номер сохранения.
                \param result Результат сохранения.
                */
                void finishSave(unsigned long savenumber, bool result);
                std::atomic<unsigned long> s_LastSaveNumber; //!< Номер последнего сохранения (saveAllChunks()).
                std::map<unsigned long, int> s_Saves; //!< Состояние сохранений: 0 - идёт, 1 - успешно, 2 - ошибка.
                std::mutex s_LockSaves; //!< Защищает s_Saves.
                std::condition_variable s_CheckSaves; //!< Ожидание окончания сохранения (saveAllChunks(id, true)) - с таймаутом, без notify.
                //! Пересчёт координат сцены в координаты мира.
                struct WorldTransform
                {
                    bool s_HasRoot; //!< Есть ли у мира родительская нода.
                    irr::core::matrix4 s_Inverse; //!< Обратная матрица абсолютного преобразования родительской ноды (с её родителями).
                    irr::core::vector3df s_Amendment; //!< Сдвиг начала координат мира (World3DInfo::s_AmendmentPositionWorld).
                };
                //! Расчёт преобразования мира по его World3DInfo и текущему положению нод.
                /*! Только в главном потоке: читает ноды сцены.
                \param worldid Идентификатор мира.
                \param transform Сюда записывается результат.
                */
                void calculateWorldTransform(int worldid, WorldTransform& transform);
                //! Запомнить преобразование мира для других потоков.
                /*! Только в главном потоке. Вызывается из addWorld(), update() и FactoryChunkMeshSceneNode::changeWorld3DInfo().
                \param worldid Идентификатор мира.
                */
                void updateWorldTransform(int worldid);
                std::map<int, WorldTransform> s_WorldTransforms; //!< Преобразования миров, запомненные главным потоком (для getWorldCoordinates() из других потоков).
                std::mutex s_LockWorldTransforms; //!< Защищает s_WorldTransforms.
                //Блокировки.
                //Единственная блокировка данных SimpleVoxelEngine: под ней идут все чтения и изменения мира
                //(раньше часть изменений шла без неё - главная причина падений).
                std::recursive_mutex s_LockAccessToTheSimpleVoxelEngine;
        };
    };
};

/*! \file IrrSimpleVoxelEngine.h
	\brief Главный заголовочный файл. Необходимо присоединять только его к проекту.
*/

#endif
