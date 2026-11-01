/*! \page tutorial2 Урок 2: Свой воксель, генератор мира и настройка враппера.
 В прошлом уроке мы поставили пол вручную, вокселями встроенного типа. Для игры этого мало: нужны свои типы блоков
 (трава, земля, камень...), мир, который создаётся сам, и сообщения движка, чтобы видеть ошибки. Этим и займёмся:
 напишем свой воксель, генератор холмистого мира, подключим логгер и разберём основные настройки враппера - потоки,
 уровни мешей, склейку чанков в ноды и материалы. В итоге получится остров 112x112 блоков с холмами.

 Многое здесь - это ядро SimpleVoxelEngine, и оно уже подробно описано в его уроках: свой тип вокселя - в уроке 3
 SimpleVoxelEngine, информация для отрисовки (VoxelInfo) - в уроке 5, генератор мира и логгер - в уроке 2. Здесь мы
 сосредоточимся на том, что добавляет враппер: в каком потоке что вызывается и как это всё настроить.

 \section irrtut2_include Подключаем
 \code
 #include <cmath>
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core; // World, WorldGenerator, VoxelInfo, VoxelForRendering...
 using namespace Voxels; // SimpleVoxelObject, ChunkVoxelObject.
 using namespace Additional; // Vector3d.
 using namespace Debug; // Logger.
 using namespace IrrEngine; // Враппер.
 using namespace irr; using namespace scene; using namespace video; using namespace core; // Irrlicht.
 \endcode
 Пространство имён Debug нужно для логгера, cmath - для синусов генератора.

 \section irrtut2_voxel Свой воксель
 Заведём три типа блоков и сам блок:
 \code
 // Типы наших блоков.
 enum { ID_GRASS = 1, ID_EARTH = 2, ID_STONE = 3 };

 // Информация для отрисовки: копия того, что фабрике нужно знать о блоке.
 struct MyVoxelInfo : VoxelInfo
 {
     int s_Id;
 };

 // Свой блок. Хранит только свой тип - положение и видимость граней хранит движок.
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(int id) : s_Id(id) { setTypeVoxel(Defines::CUSTOM_VOXEL); } // Все свои блоки - CUSTOM_VOXEL.
     // Вызывается под блокировкой движка, когда строится меш чанка. Вернуть НОВЫЙ объект - его удалит движок.
     VoxelInfo* getVoxelForRenderingInfo()
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // Это наша собственная информация.
         info->s_Id = s_Id;
         return info;
     }
     int getId() { return s_Id; }
 private:
     int s_Id;
 };
 \endcode
 Блок наследуется от SimpleVoxelObject и получает тип CUSTOM_VOXEL - так ядро понимает, что это пользовательский
 воксель (подробно - в уроке 3 SimpleVoxelEngine). Хранить в блоке положение не нужно: его знает ядро по адресу вокселя.

 Самое интересное - getVoxelForRenderingInfo(). Зачем вообще отдельная структура MyVoxelInfo, почему бы фабрике не
 читать сам блок? Дело в потоках. Когда враппер строит меш чанка, поток построения под блокировкой движка делает
 "снимок" чанка: для каждого видимого вокселя - позицию, видимость граней и результат getVoxelForRenderingInfo().
 Потом блокировка снимается, и меш строится уже без неё - параллельно с тем, как главная очередь меняет мир. Блок к
 этому моменту может быть уже заменён или удалён, поэтому фабрика получает не сам блок, а копию нужных ей данных.
 Отсюда правила: возвращайте новый объект (его удалит движок), кладите в него копии данных, а не указатели на блок,
 и держите его маленьким - функция вызывается для каждого видимого вокселя при каждой перестройке чанка.

 \section irrtut2_generator Генератор мира
 \code
 // Генератор мира: холмы из камня, земли и травы. Вызывается в потоке главной очереди под блокировкой движка.
 class MyWorldGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
     {
         Vector3d<int> vc = chunk->getVoxelAddress()->getVector(0); // Координаты чанка в мире.
         for(int x = 0; x < sizechunk; x++)
             for(int z = 0; z < sizechunk; z++)
             {
                 int gx = vc.getX() * sizechunk + x; // Глобальные координаты столбца.
                 int gz = vc.getZ() * sizechunk + z;
                 int height = 6 + (int)floor(3 * sin(gx * 0.3) + 3 * cos(gz * 0.25)); // Высота земли: от 0 до 12.
                 for(int y = 0; y < sizechunk; y++)
                 {
                     int gy = vc.getY() * sizechunk + y;
                     if(gy > height) break; // Выше земли - воздух (пустые ячейки).
                     int id = ID_STONE;
                     if(gy == height) id = ID_GRASS;
                     else if(gy > height - 3) id = ID_EARTH;
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new MyBlock(id), true, false);
                 }
             }
         return true;
     }
 };
 \endcode
 Как устроен генератор, подробно рассказано в уроке 2 SimpleVoxelEngine: generateChunk() получает пустой чанк и
 заполняет его, нулевой вектор адреса чанка - его координаты в мире. Мы считаем для каждого столбца высоту земли по
 глобальным координатам (поэтому холмы плавно продолжаются из чанка в чанк), сверху кладём траву, под ней три слоя
 земли, ниже - камень. Последнее false в setSimpleVoxelObject() говорит, что пересчитывать видимость соседей не нужно:
 при генерации весь чанк и так будет пересчитан.

 Что важно знать о генераторе во враппере:
 - Он вызывается в потоке главной очереди под блокировкой движка, когда addChunksForRendering() с load = true (или
   стриминг - урок 6) просит чанк, которого нет ни в памяти, ни в сохранении. Пока генератор работает, мир никто не
   меняет и меши не снимаются, так что с чанком можно работать напрямую.
 - Поток главной очереди один, поэтому чанки генерируются по одному, а все остальные задания (установка блоков,
   клики игрока) ждут. Медленный генератор - медленная загрузка мира. Потоки построения мешей тут не помогают.
 - Из генератора нельзя вызывать то, что ждёт другие потоки враппера (stop(), saveAllChunks(id, true)): поток ждал бы сам себя.
 - Если мир нужен не бесконечным (например, высотой 16 чанков), у ядра есть границы мира - о них тоже в уроке 2
   SimpleVoxelEngine. За границей генератор даже не вызывается.

 \section irrtut2_factory Фабрика
 Фабрика почти как в уроке 1, только цвет теперь зависит от типа блока:
 \code
 // Фабрика: цвет блока - по его типу.
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
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения.
     {
         MyVoxelInfo* info = (MyVoxelInfo*)vxl->getVoxelInfo(); // То, что вернула MyBlock::getVoxelForRenderingInfo().
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         SColor color(255, 128, 128, 128); // Камень - серый.
         if(info->s_Id == ID_GRASS) color.set(255, 60, 170, 60);
         else if(info->s_Id == ID_EARTH) color.set(255, 130, 90, 50);
         if(mesh) s_MeshManipulator->setVertexColors(mesh, color);
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };
 \endcode
 vxl->getVoxelInfo() - это как раз тот объект, который вернула MyBlock::getVoxelForRenderingInfo(). Текстуры и разные
 грани - в следующем уроке.

 \section irrtut2_logger Логгер
 \code
     Logger* lgr = new Logger; // Логгер выводит сообщения движка в консоль.
     lgr->setPrintStandard(false); // Обычных сообщений (загрузка чанков и т.п.) очень много - выключим их.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Логгер теперь держит движок.
 \endcode
 Логгер передаётся в конструктор враппера. Движок берёт на него ссылку, поэтому свою мы сразу отпускаем (drop()).
 Сообщения бывают пяти типов: ошибки (Error), нарушения логики - то есть ошибки программиста, например настройка не
 вовремя (Logic violation), предупреждения (Warning), обычные (Standard) и информационные (Info). Каждый тип можно
 выключить: setPrintError(), setPrintLogicViolation(), setPrintWarning(), setPrintStandard(), setPrintInfo(). Обычных
 сообщений очень много (о каждом загруженном чанке, о каждом чанке, уже помеченном для обновления), поэтому их обычно
 выключают. А вот ошибки и нарушения логики выключать не советуем - это ваши главные подсказки.

 Особенность враппера: сообщения идут из разных потоков сразу (главного, главной очереди, потоков построения), и,
 чтобы строки не перемешивались, враппер переопределяет Engine::registerEvent() и выводит сообщения строго по одному.
 Выключенные типы сообщений отбрасываются сразу, без блокировки, так что выключенный вывод ничего не стоит. Подробнее
 (и как выводить лог, например, в файл) - в уроке 10.

 \section irrtut2_threads Потоки построения
 \code
     isve->setMaxNumberOfThreadAddChunks(4); // Потоки построения новых чанков - до start().
     isve->setMaxNumberOfThreadUpdateChunks(1); // Потоки перестройки изменённых чанков - до start().
 \endcode
 У враппера два пула потоков построения мешей. Потоки "создания" строят меши только что добавленных чанков. Потоки
 "обновления" в первую очередь перестраивают изменённые чанки (чтобы поставленный блок появлялся сразу, даже пока
 грузится большой мир), а когда таких нет - помогают потокам создания. По умолчанию в каждом пуле по одному потоку.
 Менять число потоков можно только до start(): после запуска функции вернут false (с ошибкой в логе). Узнать текущие
 значения - getMaxNumberOfThreadAddChunks() и getMaxNumberOfThreadUpdateChunks().

 Сколько ставить? Обычно на один-два меньше числа ядер процессора: главному потоку тоже нужно время на рисование.
 Помните, что генерация мира идёт в одном потоке главной очереди, и потоки построения её не ускоряют.
 Подробнее о производительности - в уроке 14.

 \section irrtut2_settings Настройки фабрики
 \code
     MyFactory* factory = new MyFactory(device, isve);
     factory->setNumberOfMeshesSceneNodeForChunk(1); // Число уровней мешей - ПЕРВЫМ из настроек уровней.
     factory->setPositionScale(16); // Масштабы - равны размеру чанка.
     factory->setSScale(16);
     factory->setSizeBlockChunks(2, 2, 2); // Склеивать чанки в ноды по 2x2x2.
     factory->setMaterialType(0, EMT_SOLID); // Материал уровня 0 (EMT_SOLID и так стоит по умолчанию).
     isve->setFactoryChunkMeshSceneNode(factory); // Фабрику - до addWorld(). Теперь фабрика заморожена.
 \endcode
 Порядок здесь важен.

 setNumberOfMeshesSceneNodeForChunk() задаёт число уровней мешей. Уровень - часть меша чанка со своим материалом:
 например, 0 - непрозрачные блоки, 1 - стекло, 2 - вода (урок 9). Номер уровня каждого вокселя возвращает
 getMeshNumber() фабрики. Эта функция сбрасывает материалы и hardware mapping всех уровней, поэтому вызывайте её
 первой, до setMaterialType() и setHardwareMappingHint(). И только до setFactoryChunkMeshSceneNode(): у замороженной
 фабрики число уровней не меняется. По умолчанию уровень один.

 setPositionScale() и setSScale() - масштабы позиции и размера вокселей, о них мы говорили в уроке 1: они должны быть
 равны размеру чанка.

 setSizeBlockChunks(2, 2, 2) говорит, сколько соседних чанков склеивать в одну ноду Irrlicht (блок чанков). Зачем?
 Каждая нода - это отдельные вызовы отрисовки, и сотни маленьких нод рисуются медленнее, чем десятки больших. Но у
 больших блоков есть цена: при любом изменении чанка меш всего блока склеивается заново (меши самих чанков при этом
 не перестраиваются, только копируются), а прозрачные уровни сортируются точнее в маленьких блоках. По умолчанию
 блок - один чанк. Эта общая настройка - для всех миров и только до setFactoryChunkMeshSceneNode(). Если нужно разное
 для разных миров, общий размер не задавайте, а после addWorld() вызовите setSizeBlockChunks(id, x, y, z) для
 каждого мира (один раз, до первых мешей).

 setMaterialType(0, EMT_SOLID) - материал уровня 0 для миров, которые будут созданы. EMT_SOLID и так стоит по
 умолчанию, строка - для примера. Чтобы сменить материал уже созданного мира, есть setMaterialType(уровень, id, тип)
 (о прозрачных материалах - в уроке 9).

 Рядом с материалом стоят ещё две настройки уровня, которые в этом уроке нам не нужны. setHardwareMappingHint(уровень,
 способ, буфер) говорит Irrlicht, держать ли меши нод уровня в видеопамяти (например, EHM_STATIC), - так же, как
 setMaterialType(): без id - для будущих миров, с id - для созданного мира (об этом - в уроке 14). setID(уровень, id,
 номер) задаёт ID нодам уровня в мире - по нему удобно отличать ноды чанков, например, при выборе блока лучом
 (урок 7). Сколько уровней у фабрики, скажет getNumberOfMeshesSceneNodeForChunk().

 После setFactoryChunkMeshSceneNode() фабрика заморожена: число уровней, масштабы и общий размер блока больше не
 меняются (вызовы вернут false или просто ничего не сделают, а в лог попадёт нарушение логики).

 \section irrtut2_world Мир
 \code
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16; // Ребро чанка в вокселях (16 - и так по умолчанию).
     int id = isve->addWorld(wcp);
     isve->getWorld(id)->setWorldGenerator(new MyWorldGenerator); // Генератор удалит мир.
     factory->setMultiplePartition(id, true); // Все чанки разбиты одинаково - видимость граней считается быстрее.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false); // Без света - виден цвет вершин.
 \endcode
 У addWorld() три варианта: без параметров (как в уроке 1), с параметрами WorldCreatingParameters (размер чанка и
 одинаковость разбиения) и с готовым объектом World, созданным для этого движка, - его враппер добавит к себе. Все
 три - только из главного потока и только после установки фабрики. Если меняете размер чанка, не забудьте, что
 масштабы фабрики должны быть равны ему.

 Генератор ставим миру сразу после создания. Мир становится его владельцем и удалит его сам. Обратите внимание:
 мы работаем с объектом World напрямую, в обход враппера. До start() так можно: потоков ещё нет. Во время работы
 прямой доступ к объектам ядра - только под блокировкой движка (урок 10).

 setMultiplePartition(id, true) сообщает, что все чанки мира разбиты одинаково (на getSizeChunk() ячеек по каждой
 оси) - тогда видимость граней на стыках чанков считается быстрым путём. Результат тот же, что и без неё, меняется
 только скорость. Наш генератор так и делает. Если у какого-то чанка окажется другое разбиение, для него просто
 будет использован медленный путь. Проверить значение можно функцией isMultiplePartition(id).

 setMaterialFlag() - флаги материала нод уровня в этом мире, как в уроке 1. Кроме освещения часто пригождаются
 EMF_BILINEAR_FILTER (сглаживание текстур), EMF_FOG_ENABLE (туман) и EMF_WIREFRAME (каркас - удобно при отладке:
 сразу видно, какие грани враппер нарисовал). Флаги ставятся из главного потока и сразу применяются и к уже
 созданным нодам.

 \section irrtut2_area Область чанков и запуск
 \code
     isve->addChunksForRendering(id, Vector3d<int>(-3, 0, -3), Vector3d<int>(3, 1, 3), true); // 7x2x7 чанков.
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(0, 30, -50));
     camera->setTarget(vector3df(0, 5, 0));
     isve->start();
 \endcode
 Отрисовываем кубоид из 7x2x7 чанков. Чанки верхнего слоя (Y = 1) пустые - над землёй только воздух, но они загружены:
 когда мы будем ставить блоки (урок 8), им будет куда встать. Пустой чанк почти не занимает памяти, и меш его пуст.

 На краю области у соседних чанков нет данных в памяти, а грань, за которой незагруженный чанк, считается видимой.
 Поэтому остров окружён "стенками" из боковых граней - для острова это как раз то, что нужно. Если же область -
 часть большого мира, передайте loadframe = true (пятый параметр): тогда вокруг области загрузится рамка из чанков
 толщиной sizeframe (шестой параметр, по умолчанию 1). Рамка не рисуется, но благодаря ей грани на краю считаются
 правильно. Подгружать и выгружать чанки вокруг игрока удобнее стримингом - урок 6.

 Главный цикл - как в уроке 1:
 \code
     while(isve->runIrrDevice())
     {
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 120, 160, 220));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
 \endcode

 \section irrtut2_stop Остановка и сохранение
 \code
     isve->stop(); // Дождались выполнения всех заданий и остановили потоки.
     // Здесь мир можно сохранить: isve->getWorld(id)->saveAllChunks() (нужен контроллер сохранения - урок 3 SimpleVoxelEngine).
     delete isve; // Удалит фабрику, миры (и их генераторы).
     device->drop();
     return 0;
 }
 \endcode
 После stop() потоки враппера миром не пользуются, и с ним можно работать напрямую - например, сохранить все чанки
 через World::saveAllChunks(). Для этого миру нужен контроллер загрузки и сохранения (урок 3 SimpleVoxelEngine),
 у нас его нет, поэтому строка - только комментарий. Сохранять мир во время игры, не останавливая движок, можно
 функцией isve->saveAllChunks(id) - об этом в уроке 11.

 \section irrtut2_errors Типичные ошибки
 - setMaterialType() или setHardwareMappingHint() вызваны до setNumberOfMeshesSceneNodeForChunk() - настройки
   уровней сброшены.
 - Настройки фабрики (масштабы, уровни, размер блока) заданы после setFactoryChunkMeshSceneNode(), число потоков -
   после start(): они не применились, в логе нарушение логики.
 - Размер чанка в WorldCreatingParameters изменён, а масштабы фабрики - нет.
 - getVoxelForRenderingInfo() отдаёт указатель на сам блок или его данные: фабрика читает их без блокировки, когда
   блок может быть уже удалён. Отдавайте копию.
 - Генератор долгий: пока он работает, ждут все задания главной очереди.
 - Прямой доступ к World во время работы движка без блокировки - случайные падения (урок 10).

 Ну и напоследок ещё раз полный код:
 \code
 #include <cmath>
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core; // World, WorldGenerator, VoxelInfo, VoxelForRendering...
 using namespace Voxels; // SimpleVoxelObject, ChunkVoxelObject.
 using namespace Additional; // Vector3d.
 using namespace Debug; // Logger.
 using namespace IrrEngine; // Враппер.
 using namespace irr; using namespace scene; using namespace video; using namespace core; // Irrlicht.

 // Типы наших блоков.
 enum { ID_GRASS = 1, ID_EARTH = 2, ID_STONE = 3 };

 // Информация для отрисовки: копия того, что фабрике нужно знать о блоке.
 struct MyVoxelInfo : VoxelInfo
 {
     int s_Id;
 };

 // Свой блок. Хранит только свой тип - положение и видимость граней хранит движок.
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(int id) : s_Id(id) { setTypeVoxel(Defines::CUSTOM_VOXEL); } // Все свои блоки - CUSTOM_VOXEL.
     // Вызывается под блокировкой движка, когда строится меш чанка. Вернуть НОВЫЙ объект - его удалит движок.
     VoxelInfo* getVoxelForRenderingInfo()
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // Это наша собственная информация.
         info->s_Id = s_Id;
         return info;
     }
     int getId() { return s_Id; }
 private:
     int s_Id;
 };

 // Генератор мира: холмы из камня, земли и травы. Вызывается в потоке главной очереди под блокировкой движка.
 class MyWorldGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
     {
         Vector3d<int> vc = chunk->getVoxelAddress()->getVector(0); // Координаты чанка в мире.
         for(int x = 0; x < sizechunk; x++)
             for(int z = 0; z < sizechunk; z++)
             {
                 int gx = vc.getX() * sizechunk + x; // Глобальные координаты столбца.
                 int gz = vc.getZ() * sizechunk + z;
                 int height = 6 + (int)floor(3 * sin(gx * 0.3) + 3 * cos(gz * 0.25)); // Высота земли: от 0 до 12.
                 for(int y = 0; y < sizechunk; y++)
                 {
                     int gy = vc.getY() * sizechunk + y;
                     if(gy > height) break; // Выше земли - воздух (пустые ячейки).
                     int id = ID_STONE;
                     if(gy == height) id = ID_GRASS;
                     else if(gy > height - 3) id = ID_EARTH;
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new MyBlock(id), true, false);
                 }
             }
         return true;
     }
 };

 // Фабрика: цвет блока - по его типу.
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
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения.
     {
         MyVoxelInfo* info = (MyVoxelInfo*)vxl->getVoxelInfo(); // То, что вернула MyBlock::getVoxelForRenderingInfo().
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         SColor color(255, 128, 128, 128); // Камень - серый.
         if(info->s_Id == ID_GRASS) color.set(255, 60, 170, 60);
         else if(info->s_Id == ID_EARTH) color.set(255, 130, 90, 50);
         if(mesh) s_MeshManipulator->setVertexColors(mesh, color);
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };


 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(1280, 720));
     Logger* lgr = new Logger; // Логгер выводит сообщения движка в консоль.
     lgr->setPrintStandard(false); // Обычных сообщений (загрузка чанков и т.п.) очень много - выключим их.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Логгер теперь держит движок.
     isve->setMaxNumberOfThreadAddChunks(4); // Потоки построения новых чанков - до start().
     isve->setMaxNumberOfThreadUpdateChunks(1); // Потоки перестройки изменённых чанков - до start().

     MyFactory* factory = new MyFactory(device, isve);
     factory->setNumberOfMeshesSceneNodeForChunk(1); // Число уровней мешей - ПЕРВЫМ из настроек уровней.
     factory->setPositionScale(16); // Масштабы - равны размеру чанка.
     factory->setSScale(16);
     factory->setSizeBlockChunks(2, 2, 2); // Склеивать чанки в ноды по 2x2x2.
     factory->setMaterialType(0, EMT_SOLID); // Материал уровня 0 (EMT_SOLID и так стоит по умолчанию).
     isve->setFactoryChunkMeshSceneNode(factory); // Фабрику - до addWorld(). Теперь фабрика заморожена.

     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16; // Ребро чанка в вокселях (16 - и так по умолчанию).
     int id = isve->addWorld(wcp);
     isve->getWorld(id)->setWorldGenerator(new MyWorldGenerator); // Генератор удалит мир.
     factory->setMultiplePartition(id, true); // Все чанки разбиты одинаково - видимость граней считается быстрее.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false); // Без света - виден цвет вершин.

     isve->addChunksForRendering(id, Vector3d<int>(-3, 0, -3), Vector3d<int>(3, 1, 3), true); // 7x2x7 чанков.
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(0, 30, -50));
     camera->setTarget(vector3df(0, 5, 0));
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 120, 160, 220));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop(); // Дождались выполнения всех заданий и остановили потоки.
     // Здесь мир можно сохранить: isve->getWorld(id)->saveAllChunks() (нужен контроллер сохранения - урок 3 SimpleVoxelEngine).
     delete isve; // Удалит фабрику, миры (и их генераторы).
     device->drop();
     return 0;
 }
 \endcode
*/
