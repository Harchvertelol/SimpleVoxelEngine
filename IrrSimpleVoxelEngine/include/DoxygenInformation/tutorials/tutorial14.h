/*! \page tutorial14 Урок 14: Производительность и отладка.
 Мир работает, блоки ставятся, всё рисуется. Теперь пора разобраться, от каких настроек зависит скорость и как
 ловить ошибки. В этом уроке:
 - склейка чанков в ноды (setSizeBlockChunks()) - главная настройка скорости рисования;
 - hardware mapping - где хранятся меши нод;
 - число потоков построения и setMultiplePartition();
 - замер: FPS, число треугольников и нод;
 - Occlusion Queries - что это, как включить, сколько стоят и когда окупаются;
 - отладочные библиотеки (lib/Debug, SVE_DEBUG_CHECKS);
 - заморозка и очистка фабрики, правильное завершение, константы IrrSVEDefines.
 Пример - два одинаковых холмистых мира по 4x2x4 чанка рядом: у первого каждый чанк - своя нода, у второго все 32
 чанка склеены в одну. Подключения и фабрика - как в уроке 1:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <cstdio>
 #include <thread>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;
 \endcode
 \code
 // Фабрика - как в уроке 1: куб из видимых граней, цвет вершин - цвет вокселя.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         if(mesh) s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };
 \endcode
 Генератор холмов (высота от 4 до 20 блоков, поэтому заняты чанки с y = 0 и y = 1), обработчик клавиши O и
 маленькая функция подсчёта нод мира - ноды чанков являются детьми родительской ноды мира (урок 12):
 \code
 // Генератор холмов высотой от 4 до 20 блоков: заняты чанки с y = 0 и y = 1. Вызывается в потоке главной очереди.
 class HillsGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         Vector3d<int> v = chunk->getVoxelAddress()->getVector(0); // Координаты чанка (в чанках).
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
             {
                 const int gx = v.getX() * size + x, gz = v.getZ() * size + z; // Глобальные координаты столбца.
                 const int h = (int)(12 + 8 * sinf(gx * 0.2f) * cosf(gz * 0.15f));
                 for(int y = 0; y < size; y++)
                 {
                     const int gy = v.getY() * size + y;
                     if(gy > h) break;
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 60 + gy * 8, 150, 60)), true, false);
                 }
             }
         return true;
     }
 };
 \endcode
 \code
 // Обработчик: O - включить или выключить Occlusion Queries.
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown && event.KeyInput.Key == KEY_KEY_O) s_Toggle = true;
         return false;
     }
     bool s_Toggle = false;
 };
 \endcode
 \code
 // Сколько нод чанков у мира: ноды чанков - дети его родительской ноды (урок 12).
 int countChunkNodes(ISceneNode* root)
 {
     int nodes = 0;
     for(core::list<ISceneNode*>::ConstIterator it = root->getChildren().begin(); it != root->getChildren().end(); ++it)
         if((*it)->getType() == ESNT_MESH) nodes++;
     return nodes;
 }
 \endcode
 \section t14threads Потоки построения
 Меши чанков строят два пула потоков: потоки "создания" строят только что добавленные чанки (загрузка мира), потоки
 "обновления" в первую очередь перестраивают изменённые чанки (чтобы правки игрока появлялись сразу, даже пока
 грузится большой мир), а когда таких нет - помогают потокам создания. По умолчанию в каждом пуле по одному потоку
 (IrrSVEDefines::DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS и DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS). Число потоков задаётся
 только до start() (getMaxNumberOfThreadAddChunks() и getMaxNumberOfThreadUpdateChunks() вернут текущие значения):
 \code
 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     MyReceiver receiver;
     device->setEventReceiver(&receiver);
     ISceneManager* smgr = device->getSceneManager();
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     // Потоки построения - до start(). Два ядра оставляем главному потоку и потоку главной очереди.
     unsigned int cores = std::thread::hardware_concurrency();
     isve->setMaxNumberOfThreadAddChunks(cores > 3 ? cores - 2 : 1);
     isve->setMaxNumberOfThreadUpdateChunks(1);
 \endcode
 Два ядра оставляем главному потоку (рисование) и потоку главной очереди (генерация, загрузка, правки). Больше потоков
 - быстрее загрузка, но не бесконечно: снимок данных чанка берётся под блокировкой движка, а ваши getMeshVoxel() и
 getMeshNumber() вызываются строго по одному (под общей блокировкой фабрики, урок 3) - параллельно идёт только
 остальная работа (склейка мешей вокселей в меш чанка). Чем быстрее ваш getMeshVoxel(), тем больше пользы от
 потоков: ничего не загружайте и не создавайте в нём лишнего.

 \section t14glue Склейка чанков в ноды
 Враппер не создаёт по ноде на каждый чанк: соседние чанки склеиваются в блок, и у блока одна нода Irrlicht на каждый
 уровень мешей. Размер блока в чанках задаёт setSizeBlockChunks(). Это главный выбор между скоростью рисования и
 скоростью изменений:
 - <b>меньше нод - быстрее рисование.</b> Каждая нода - это обход сцены, проверка видимости камерой, смена материала и
   вызовы отрисовки (по одному на мешбуфер - на каждую текстуру уровня). Тысяча маленьких нод рисуется заметно
   медленнее, чем сотня больших с тем же числом треугольников;
 - <b>больше нод - быстрее изменения.</b> При правке одного блока перестраивается только его чанк (в потоке
   построения), но склеивается весь блок чанков - копированием готовых мешей всех его чанков, в главном потоке, внутри
   update(). Блок 4x4x4 - это 64 чанка на каждую правку, и игра начинает подтормаживать при каждом щелчке;
 - большие ноды хуже отсекаются камерой (нода рисуется целиком, если видна хоть её часть), а у полупрозрачных уровней
   сортировка между нодами грубее и каждая пересортировка дороже (урок 9).
 Обычно хорош небольшой столбик или кубик: в TestProject3 - 1x4x1 (четыре чанка по высоте). Лучший размер для вашей
 игры покажет только замер.

 Задать размер можно двумя способами - выбрать нужно один:
 - общий для всех миров: factory->setSizeBlockChunks(x, y, z) - до setFactoryChunkMeshSceneNode() (потом фабрика
   заморожена);
 - для каждого мира свой: factory->setSizeBlockChunks(id, x, y, z) - после addWorld(), пока у мира нет ни одного
   готового меша (проще всего - до start()): первый меш замораживает размер. Если общий размер задан, такой вызов -
   ошибка в логе.
 Если не задать ничего, каждый чанк - своя нода (1x1x1).

 \section t14hm Hardware mapping
 По умолчанию Irrlicht каждый кадр передаёт видеокарте вершины и индексы всех мешей из оперативной памяти. Меши нод
 чанков меняются редко, поэтому выгоднее хранить их в видеопамяти (VBO) - EHM_STATIC: меш передаётся один раз и
 заново - только когда нода пересклеена. Это setHardwareMappingHint(уровень, режим, буфер): без номера мира - для всех
 будущих миров (вызывать после setNumberOfMeshesSceneNodeForChunk(), который сбрасывает настройки уровней, и до
 addWorld()), с номером мира - для одного мира (после addWorld(), применяется и к уже склеенным мешам). У
 полупрозрачных уровней индексы пересортировываются при движении камеры, там можно попробовать EHM_DYNAMIC для
 индексов - и замерить.

 Настроим фабрику: масштабы, один уровень, hardware mapping, таймер Occlusion Queries (о нём ниже), и отдадим её
 движку:
 \code
 MyFactory* factory = new MyFactory(device, isve);
 factory->setPositionScale(16);
 factory->setSScale(16);
 factory->setNumberOfMeshesSceneNodeForChunk(1); // Первым из настроек уровней: сбрасывает материалы и hardware mapping.
 factory->setHardwareMappingHint(0, EHM_STATIC, EBT_VERTEX_AND_INDEX); // Меши нод - в видеопамяти (для всех будущих миров).
 factory->setTimerTestOcclusionQueriesInMilliseconds(2 * IrrSVEDefines::TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS); // Раз в 200 мс.
 isve->setFactoryChunkMeshSceneNode(factory); // Здесь фабрика замораживается: масштабы и общая склейка больше не меняются.
 \endcode
 Два мира рядом, у каждого своя родительская нода (урок 12). setMultiplePartition(id, true) обещает движку, что все
 чанки верхнего уровня разбиты одинаково (на getSizeChunk()), - тогда видимость граней считается быстрым путём, прямо
 по массивам чанка и шести соседей (около 0.2 мс на чанк 16x16x16). Результат тот же, что медленным путём, а если
 у какого-то чанка всё же другое разбиение, для него быстрый путь просто не используется (текущее значение -
 isMultiplePartition(id)). Рамка (loadframe = true)
 загружает чанки вокруг области: сама не рисуется, но без неё на краях области рисовались бы лишние грани.
 \code
 // Два одинаковых мира 4x2x4 чанка рядом: у первого каждый чанк - своя нода, у второго все 32 чанка склеены в одну.
 ISceneNode* roots[2];
 int ids[2];
 for(int i = 0; i < 2; i++)
 {
     roots[i] = smgr->addEmptySceneNode();
     roots[i]->setPosition(vector3df(i * 80.0f, 0, 0));
     World3DInfo info;
     info.s_WorldRootNode = roots[i];
     ids[i] = isve->addWorld(info);
     isve->getWorld(ids[i])->setWorldGenerator(new HillsGenerator);
     factory->setMultiplePartition(ids[i], true); // Все чанки разбиты одинаково - быстрый расчёт видимости граней.
     factory->setMaterialFlag(0, ids[i], EMF_LIGHTING, false);
     isve->addChunksForRendering(ids[i], Vector3d<int>(0, 0, 0), Vector3d<int>(3, 1, 3), true, true); // С рамкой: края без лишних граней.
 }
 factory->setSizeBlockChunks(ids[0], 1, 1, 1); // Склейка - для каждого мира (общую до этого не задавали), до первого меша.
 factory->setSizeBlockChunks(ids[1], 4, 2, 4);
 \endcode
 \section t14measure Замер
 Камера, запуск и главный цикл. Раз в 2 секунды печатаем FPS, число нарисованных треугольников и число нод каждого
 мира:
 \code
 ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
 camera->setPosition(vector3df(70, 40, -40));
 camera->setTarget(vector3df(70, 0, 32));
 isve->start();
 u32 lastprint = 0;
 while(isve->runIrrDevice())
 {
     if(receiver.s_Toggle) // O: Occlusion Queries включаются и выключаются и у уже созданных нод.
     {
         receiver.s_Toggle = false;
         factory->setOcclusionQueries(!factory->getOcclusionQueries());
     }
     isve->update();
     device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
     smgr->drawAll();
     isve->runOcclusionQueries(); // Между drawAll() и endScene(); без включённых Occlusion Queries ничего не делает.
     device->getVideoDriver()->endScene();
     if(isve->getTimeSafeThread() - lastprint > 2000) // Раз в 2 секунды - замер.
     {
         lastprint = isve->getTimeSafeThread();
         printf("FPS %d, triangles %u, nodes: world 1 (1x1x1) %d, world 2 (4x2x4) %d, occlusion queries %s\n",
                device->getVideoDriver()->getFPS(), device->getVideoDriver()->getPrimitiveCountDrawn(),
                countChunkNodes(roots[0]), countChunkNodes(roots[1]), factory->getOcclusionQueries() ? "on" : "off");
     }
 }
 \endcode
 У первого мира получится нода на каждый непустой чанк (у нас 28), у второго - одна. Треугольников у них поровну, а
 вызовов отрисовки - в 28 раз меньше у второго. Попробуйте поставить у обоих по 50x2x50 чанков и сравнить FPS - а
 потом поставьте блок в каждом мире и сравните, насколько дольше появляется изменение во втором.

 \section t14oq Occlusion Queries
 Occlusion Query - это вопрос к видеокарте: "сколько пикселей этой ноды прошло бы проверку глубины?". Если ноль - нода
 целиком закрыта другими (например, горой перед камерой), и её можно не рисовать. Включаются они у фабрики:
 setOcclusionQueries(true) (только в главном потоке; применяется и к уже созданным нодам, при выключении скрытые ноды
 снова показываются). Сами проверки выполняет isve->runOcclusionQueries() - её нужно вызывать каждый кадр <b>между
 drawAll() и endScene()</b>: проверка рисует ноды в уже заполненный кадром Z-буфер. В другом месте (например, до
 beginScene()) результат неверный: ноды мигают или не скрываются никогда. Раз в
 getTimerTestOcclusionQueriesInMilliseconds() миллисекунд (по умолчанию IrrSVEDefines::TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS
 = 100, у нас 200) выполняются все Occlusion Queries драйвера - и ваши собственные тоже, - и закрытые ноды скрываются
 до следующей проверки. Без включённых Occlusion Queries runOcclusionQueries() ничего не делает, поэтому её можно
 вызывать всегда.

 Окупаются ли они - зависит от сцены, поэтому сначала разберём, что они стоят и что дают.

 <b>Цена на видеокарте</b> небольшая. Проверка рисует каждую ноду ещё раз, но без записи цвета и глубины, и враппер
 по умолчанию рисует для проверки не полный меш ноды, а её ограничивающий кубоид - 12 треугольников вместо тысяч. В кадр
 кубоид не попадает. Видимую ноду кубоид не скроет никогда, а закрытую иногда сочтёт видимой (виден угол кубоида над
 пустым местом) - это безопасно, просто выигрыш чуть меньше. Изнутри у кубоида не видно ни одной лицевой грани,
 поэтому ноду, внутри кубоида которой стоит камера (блок чанков вокруг игрока), враппер не скрывает никогда. Полный меш
 точнее, но на проверке нода рисуется второй раз целиком: setOcclusionQueriesBoundingBox(false).

 <b>Цена на процессоре.</b> Irrlicht 1.8 хранит запросы в простом массиве и ищет запрос ноды перебором (linear_search) - при
 запуске проверки, при получении результата, при обновлении; на N нод это порядка N^2 сравнений за проверку. При сотнях
 нод это незаметно, при многих тысячах - уже да; поэтому с Occlusion Queries лучше крупнее склейка
 (setSizeBlockChunks() побольше - меньше нод). Кроме того, updateAllOcclusionQueries() ждёт результатов от видеокарты -
 раз в таймер процессор стоит, пока видеокарта не дорисует проверку; таймер побольше - ждать реже.

 <b>Выигрыш</b> - каждая скрытая нода не рисуется целиком, со всеми её тысячами треугольников и текстурами. Чем больше нод
 закрыто, тем больше выигрыш. Где закрыто много: пещеры и подземелья, помещения, плотный город, горы и холмы перед
 камерой, вид с земли в холмистом мире. Где почти ничего: открытая равнина и вид сверху - меши чанков и так состоят только
 из видимых граней, а ноды вне поля зрения камеры Irrlicht и так не рисует.

 <b>Почему по умолчанию выключены.</b> Результат применяется с задержкой - до следующей проверки. Быстро повернул камеру -
 нода, которая только что была закрыта, ещё скрыта, и на её месте на мгновение мелькает дыра (меньше таймер - короче
 мелькание, но чаще проверки). Решайте сами, включать ли их, - лучше с замером FPS на своей сцене и нужных видеокартах.

 Учтите ещё одно: Irrlicht
 сам удаляет запрос, который не запускался больше 1000 кадров. Поэтому проверка выполняется не только по таймеру, но и
 не реже, чем раз в getMaxFramesBetweenOcclusionQueries() кадров (вызовов runOcclusionQueries(); по умолчанию
 IrrSVEDefines::MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES = 900, можно задать от 1 до 1000 функцией
 setMaxFramesBetweenOcclusionQueries()) - большой таймер при высоком FPS запросы не потеряет. В нашем примере их включает и выключает
 клавиша O, а в цикле стоит вызов:
 \code
 isve->update();
 device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
 smgr->drawAll();
 isve->runOcclusionQueries(); // Между drawAll() и endScene(); без включённых Occlusion Queries ничего не делает.
 device->getVideoDriver()->endScene();
 \endcode
 \section t14debug Отладочные библиотеки и SVE_DEBUG_CHECKS
 У ядра и враппера есть две сборки: Release (lib/Release) и Debug (lib/Debug). Отладочная собрана с флагом
 -DSVE_DEBUG_CHECKS - в ней включены проверки ошибок программиста, которые в релизе ничего не проверяют ради скорости
 (в релизной сборке этих проверок нет совсем). Чтобы отлаживать свою программу с проверками, просто подключите
 библиотеки из lib/Debug (как в Debug-цели TestProject3) - свою программу собирать с этим флагом не нужно: заголовки
 от него не зависят. При ошибке в лог пишется сообщение с "Debug check (SVE_DEBUG_CHECKS)", а функция не выполняется
 (или возвращает ошибку) вместо порчи памяти и падения где-то далеко от места ошибки. Проверяется:
 - в ядре: координаты вектора внутри чанка, svo == NULL, чанк вне мира, отрицательный индекс VoxelAddress::addVector();
 - во враппере: update(), addWorld(), setFactoryChunkMeshSceneNode() и настройки фабрики, которые трогают сцену
   (FactoryChunkMeshSceneNode::update(), setMaterialFlag(), setMaterialType(), setHardwareMappingHint(), setID(),
   changeWorld3DInfo(), setOcclusionQueries()), а также runOcclusionQueries() и функции пула стриминга вызываются из
   главного потока - того, где создан движок (узнать поток - isMainThread());
 - stop() не вызывается из потока построения (из функций фабрики) - иначе зависание;
 - getWorldCoordinates() и поиск адреса по трём точкам и по точке: есть фабрика и мир (без проверки - молча неверный
   результат или NULL);
 - add/deleteChunksForRendering(): углы области не перепутаны (иначе область пустая);
 - set/deleteSimpleVoxelObject(): адрес создан этим же движком (предупреждение);
 - addMainTask(): задание STOP (его ставит только stop()) отклоняется.
 Например, вызов setOcclusionQueries() из другого потока в отладочной сборке не выполнится, а в логе будет:
 \code
 Logic violation: IrrSVE: Debug check (SVE_DEBUG_CHECKS): FactoryChunkMeshSceneNode::setOcclusionQueries must be called from the main thread (the thread where the engine was created).
 \endcode
 Два важных замечания. Первое: сообщения видны, только если у движка есть логгер, - без логгера движок молча
 проглатывает и ошибки, и предупреждения. Поэтому в примере движок создан с логгером (отключены только стандартные
 сообщения):
 \code
 Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
 lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
 IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
 lgr->drop(); // Движок держит свою ссылку.
 \endcode
 Второе: отладочные проверки ловят только ошибки программиста. Данные извне - файлы сохранений, сеть, ввод
 игрока - проверяйте в своём коде, и в релизе тоже. Хорошая привычка - запускать игру с отладочными библиотеками
 после каждой заметной правки и смотреть, не появилось ли в логе "Debug check".

 \section t14exit Заморозка, очистка и завершение
 setFactoryChunkMeshSceneNode() замораживает фабрику (freeze(), проверить - isFreeze()): после этого не меняются
 масштабы (setPositionScale(), setRotationScale(), setSScale()), setIsFaces(), общий setSizeBlockChunks(), число уровней
 (setNumberOfMeshesSceneNodeForChunk()) и привязка к движку и устройству. Так враппер гарантирует, что потоки
 построения и уже созданные блоки чанков не увидят смены настроек на ходу. Сами вы freeze() вызывать не должны -
 замороженную фабрику движок не примет.

 clear() полностью очищает фабрику: убирает все чанки из отрисовки, удаляет неприменённые меши, миры фабрики (с их
 настройками и World3DInfo) и уровни мешей. Её вызывает деструктор фабрики, а фабрику удаляет движок, так что
 вызывать clear() самому обычно не нужно: замороженную фабрику после clear() дальше использовать нельзя. Если нужно
 убрать из отрисовки часть чанков, не трогая данные мира, есть deleteChunk(id, вектор чанка), deleteAllChunks(id) и
 deleteAllChunks() (потокобезопасны; нода исчезает не позже ближайшего update()), а проверить,
 отрисовывается ли чанк, - isRenderChunk(id, вектор чанка). Обычно же удобнее deleteChunk(s)ForRendering() движка
 (урок 6) - они умеют ещё и выгрузить данные. Правильное
 завершение - как в уроке 11:
 \code
 isve->stop(); // Остановить потоки, потом удалить движок: он удалит ноды чанков, фабрику (её деструктор вызовет clear()) и миры.
 delete isve;
 device->drop();
 return 0;
 \endcode
 delete isve сначала убирает ноды чанков (onUpdateChunkMeshSceneNode(0, ...) по каждой ноде, пока ваша фабрика ещё
 цела), потом удаляет фабрику и миры. Устройство Irrlicht удаляется последним - движку оно нужно до конца.

 \section t14defines Константы IrrSVEDefines
 В пространстве имён IrrSVEDefines (IrrSVEDefines.h) - значения по умолчанию и пороги враппера:
 - DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS = 1 и DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS = 1 - число потоков построения по
   умолчанию;
 - TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS = 100 - период проверок Occlusion Queries по умолчанию;
 - MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES = 900 - проверки Occlusion Queries не реже, чем раз в столько кадров, и
   MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES_LIMIT = 1000 - наибольшее значение для setMaxFramesBetweenOcclusionQueries();
 - MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS = 1000 - сколько максимум склейка блока чанков ждёт, пока построятся
   его новые чанки. При загрузке чанки одного блока приходят в разных кадрах, и без ожидания блок склеивался бы
   заново на каждый пришедший чанк. Правки уже построенных чанков не ждут;
 - TRANSPARENT_SORT_CAMERA_THRESHOLD = 0.25 - на сколько единиц сцены должна сместиться камера, чтобы полупрозрачные
   треугольники блока были пересортированы (сортировать каждый кадр дорого, а при маленьком сдвиге порядок почти не
   меняется).

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <cstdio>
 #include <thread>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Фабрика - как в уроке 1: куб из видимых граней, цвет вершин - цвет вокселя.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         if(mesh) s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };

 // Генератор холмов высотой от 4 до 20 блоков: заняты чанки с y = 0 и y = 1. Вызывается в потоке главной очереди.
 class HillsGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         Vector3d<int> v = chunk->getVoxelAddress()->getVector(0); // Координаты чанка (в чанках).
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
             {
                 const int gx = v.getX() * size + x, gz = v.getZ() * size + z; // Глобальные координаты столбца.
                 const int h = (int)(12 + 8 * sinf(gx * 0.2f) * cosf(gz * 0.15f));
                 for(int y = 0; y < size; y++)
                 {
                     const int gy = v.getY() * size + y;
                     if(gy > h) break;
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 60 + gy * 8, 150, 60)), true, false);
                 }
             }
         return true;
     }
 };

 // Обработчик: O - включить или выключить Occlusion Queries.
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown && event.KeyInput.Key == KEY_KEY_O) s_Toggle = true;
         return false;
     }
     bool s_Toggle = false;
 };

 // Сколько нод чанков у мира: ноды чанков - дети его родительской ноды (урок 12).
 int countChunkNodes(ISceneNode* root)
 {
     int nodes = 0;
     for(core::list<ISceneNode*>::ConstIterator it = root->getChildren().begin(); it != root->getChildren().end(); ++it)
         if((*it)->getType() == ESNT_MESH) nodes++;
     return nodes;
 }

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     MyReceiver receiver;
     device->setEventReceiver(&receiver);
     ISceneManager* smgr = device->getSceneManager();
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     // Потоки построения - до start(). Два ядра оставляем главному потоку и потоку главной очереди.
     unsigned int cores = std::thread::hardware_concurrency();
     isve->setMaxNumberOfThreadAddChunks(cores > 3 ? cores - 2 : 1);
     isve->setMaxNumberOfThreadUpdateChunks(1);
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     factory->setNumberOfMeshesSceneNodeForChunk(1); // Первым из настроек уровней: сбрасывает материалы и hardware mapping.
     factory->setHardwareMappingHint(0, EHM_STATIC, EBT_VERTEX_AND_INDEX); // Меши нод - в видеопамяти (для всех будущих миров).
     factory->setTimerTestOcclusionQueriesInMilliseconds(2 * IrrSVEDefines::TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS); // Раз в 200 мс.
     isve->setFactoryChunkMeshSceneNode(factory); // Здесь фабрика замораживается: масштабы и общая склейка больше не меняются.

     // Два одинаковых мира 4x2x4 чанка рядом: у первого каждый чанк - своя нода, у второго все 32 чанка склеены в одну.
     ISceneNode* roots[2];
     int ids[2];
     for(int i = 0; i < 2; i++)
     {
         roots[i] = smgr->addEmptySceneNode();
         roots[i]->setPosition(vector3df(i * 80.0f, 0, 0));
         World3DInfo info;
         info.s_WorldRootNode = roots[i];
         ids[i] = isve->addWorld(info);
         isve->getWorld(ids[i])->setWorldGenerator(new HillsGenerator);
         factory->setMultiplePartition(ids[i], true); // Все чанки разбиты одинаково - быстрый расчёт видимости граней.
         factory->setMaterialFlag(0, ids[i], EMF_LIGHTING, false);
         isve->addChunksForRendering(ids[i], Vector3d<int>(0, 0, 0), Vector3d<int>(3, 1, 3), true, true); // С рамкой: края без лишних граней.
     }
     factory->setSizeBlockChunks(ids[0], 1, 1, 1); // Склейка - для каждого мира (общую до этого не задавали), до первого меша.
     factory->setSizeBlockChunks(ids[1], 4, 2, 4);

     ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(70, 40, -40));
     camera->setTarget(vector3df(70, 0, 32));
     isve->start();
     u32 lastprint = 0;
     while(isve->runIrrDevice())
     {
         if(receiver.s_Toggle) // O: Occlusion Queries включаются и выключаются и у уже созданных нод.
         {
             receiver.s_Toggle = false;
             factory->setOcclusionQueries(!factory->getOcclusionQueries());
         }
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         smgr->drawAll();
         isve->runOcclusionQueries(); // Между drawAll() и endScene(); без включённых Occlusion Queries ничего не делает.
         device->getVideoDriver()->endScene();
         if(isve->getTimeSafeThread() - lastprint > 2000) // Раз в 2 секунды - замер.
         {
             lastprint = isve->getTimeSafeThread();
             printf("FPS %d, triangles %u, nodes: world 1 (1x1x1) %d, world 2 (4x2x4) %d, occlusion queries %s\n",
                    device->getVideoDriver()->getFPS(), device->getVideoDriver()->getPrimitiveCountDrawn(),
                    countChunkNodes(roots[0]), countChunkNodes(roots[1]), factory->getOcclusionQueries() ? "on" : "off");
         }
     }
     isve->stop(); // Остановить потоки, потом удалить движок: он удалит ноды чанков, фабрику (её деструктор вызовет clear()) и миры.
     delete isve;
     device->drop();
     return 0;
 }
 \endcode
*/
