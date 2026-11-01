/*! \page tutorial11 Урок 11: Сохранение мира во время игры.
 Как сохранять и загружать чанки, мы уже знаем: в уроке 3 SimpleVoxelEngine мы написали свой контроллер сохранения
 (наследник ChunksSaveLoadController) и вызывали World::saveAllChunks(). Во враппере контроллер точно такой же, и
 пишется он так же. Отличается другое - когда и откуда сохранять. В ядре всё происходит в одном потоке, и мир
 меняется только тогда, когда мы сами его меняем. Во враппере же мир живёт в нескольких потоках: поток главной
 очереди загружает и генерирует чанки, ставит и удаляет блоки, потоки построения в это время читают мир, чтобы
 построить меши. Если посреди игры просто вызвать World::saveAllChunks() из главного потока, мир может поменяться прямо
 во время сохранения - половина чанка сохранится до установки блока, половина после, а то и что похуже. Поэтому в
 этом уроке мы разберём только то, чем сохранение во враппере отличается от ядра:
 - сохранение во время игры - IrrSimpleVoxelEngine::saveAllChunks() и проверка его окончания isSaveCompleted();
 - в каком потоке работает контроллер сохранения и что из этого следует;
 - сохранение при выгрузке чанков;
 - правильный порядок действий при выходе из программы.

 Начнём с привычного: подключим враппер и пространства имён.
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cstdio>
 #include <fstream>
 #include <string>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace Defines;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;
 \endcode
 Фабрику возьмём из урока 1 без изменений: куб из видимых граней, цвет вершин - цвет вокселя ARGBColorVoxel.
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
 \section t11controller Контроллер сохранения
 Теперь контроллер. Он такой же, как в уроке 3 SimpleVoxelEngine, и пишет чанки на диск: каждый чанк - в своём
 текстовом файле data/lesson11/chunk_x.y.z (x, y, z - координаты чанка). В файле сначала число непустых вокселей,
 потом цвета всех ячеек чанка одним числом (0 - пустая ячейка). Формат простейший, без сжатия, зато его можно открыть
 и посмотреть глазами. Папки ещё может не быть - тогда создаём её (WorkFunctions::FileFunctions::createFolders()) и
 открываем файл снова. Ошибки (файл не открылся, не записался) пишем в лог и возвращаем false - тогда сохранение
 считается неудачным:
 \code
 // Контроллер сохранения и загрузки (урок 3 SimpleVoxelEngine): каждый чанк - в своём текстовом файле
 // <папка>/chunk_<x>.<y>.<z>: сначала число непустых вокселей, потом цвета всех ячеек чанка (0 - пустая ячейка).
 // Формат - простейший, как в уроке 3 SimpleVoxelEngine. Враппер вызывает контроллер в потоке главной очереди под блокировкой движка.
 class FileSaveLoadController : public ChunksSaveLoadController
 {
 public:
     FileSaveLoadController(IrrSimpleVoxelEngine* isve, std::string folder) : s_ISVE(isve), s_Folder(folder) {}
     std::string getFileName(Vector3d<int> vect) // Файл чанка: <папка>/chunk_x.y.z
     {
         return s_Folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER);
     }
     bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::string filename = getFileName(vect);
         std::ofstream file(filename.c_str()); // Файл перезаписывается с начала.
         if(!file) // Папки ещё нет - создадим её и откроем файл снова.
         {
             WorkFunctions::FileFunctions::createFolders(s_Folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
             file.open(filename.c_str(), std::ios::out | std::ios::trunc);
             if(!file)
             {
                 s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved: error opening file " + filename);
                 return false;
             }
         }
         const unsigned int notnull = cvo->getNumberOfNotNullSimpleVoxelObjects();
         file << notnull;
         if(notnull != 0) // В пустом чанке массивов вокселей может не быть - не трогаем их.
         {
             int n = cvo->getNumberOfPartitions();
             SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects();
             for(int i = 0; i < n * n * n; i++)
             {
                 u32 color = 0; // Пустая ячейка; вложенные чанки и чужие воксели тоже не сохраняем.
                 if(svos[i] && svos[i]->getTypeVoxel() == ARGBCOLOR)
                 {
                     ARGBColor c = ((ARGBColorVoxel*)svos[i])->getARGBColor();
                     color = SColor(c.getA(), c.getR(), c.getG(), c.getB()).color; // Цвет одним числом.
                 }
                 file << " " << color;
             }
         }
         if(!file) // Ошибка записи (например, кончилось место на диске).
         {
             s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved: error writing file " + filename);
             return false;
         }
         s_SavedChunks++;
         return true;
 \endcode
 Обратите внимание: в пустом чанке массивов вокселей может и не быть (ядро экономит память), поэтому сначала
 смотрим getNumberOfNotNullSimpleVoxelObjects(), а уже потом берём getSimpleVoxelObjects(). Вложенные чанки
 (маленькие блоки, урок 13) и чужие воксели наш простой формат не сохраняет - на их месте после загрузки будет пусто.
 Загрузка - как в уроке 3 SimpleVoxelEngine: читаем файл, создаём воксели прямо в массиве чанка, просим движок
 пересчитать видимость граней (мы её не сохраняли) и сами ставим число непустых вокселей. Если файл обрезан или
 испорчен, возвращаем false - мир сам удалит недозагруженный чанк. canLoad() говорит движку, есть ли сохранение
 чанка: есть файл - загружаем, нет - движок вызовет генератор:
 \code
     bool loadChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::string filename = getFileName(vect);
         std::ifstream file(filename.c_str());
         unsigned int notnull = 0;
         if(!file || !(file >> notnull))
         {
             s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not loaded: error reading file " + filename);
             return false;
         }
         if(notnull == 0) return true; // Чанк был пустым - так и оставляем.
         int n = cvo->getNumberOfPartitions();
         SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Создаёт массивы чанка.
         bool* recalculate = cvo->getReCalculateVisibleCacheSimpleVoxelObjects();
         unsigned int created = 0;
         for(int i = 0; i < n * n * n; i++)
         {
             u32 color = 0;
             if(!(file >> color)) // Файл обрезан или испорчен - мир удалит недозагруженный чанк сам.
             {
                 s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not loaded: damaged file " + filename);
                 return false;
             }
             if(!color) continue;
             SColor c(color);
             svos[i] = new ARGBColorVoxel(ARGBColor(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue()));
             recalculate[i] = true; // Видимость граней мы не сохраняли - пусть движок пересчитает.
             created++;
         }
         cvo->setNumberOfNotNullSimpleVoxelObjects(created);
         return true;
     }
     bool canLoad(World*, Vector3d<int> vect) { return WorkFunctions::FileFunctions::fileExists(getFileName(vect)); } // Есть файл - загружаем.
     int s_SavedChunks = 0; // Сколько раз сохранялся чанк (для вывода).
 private:
     IrrSimpleVoxelEngine* s_ISVE; // Для сообщений в лог.
     std::string s_Folder; // Папка мира.
 };
 \endcode
 А теперь самое важное отличие от ядра. <b>Враппер вызывает контроллер сохранения (и загрузки, и генератор) в
 потоке главной очереди, под блокировкой движка.</b> Что из этого следует:
 - пока работает saveChunk(), мир никто не меняет: другие задания очереди ждут, а потоки построения не могут взять
   блокировку - снимок мира получится целым;
 - в контроллере нельзя трогать сцену Irrlicht (ноды, текстуры, драйвер) - это можно только в главном потоке;
 - если главный поток (или любой другой) читает данные контроллера (у нас - s_SavedChunks), он должен
   взять ту же блокировку (пример - сразу после списка);
 - долгое сохранение задерживает задания очереди (установку блоков, загрузку чанков), но не рисование: update()
   каждый кадр продолжает работать, просто новые меши в это время не строятся. Поэтому сам контроллер должен
   работать быстро: большой мир лучше писать в файлы чанков (а не в один огромный файл) и без лишних преобразований.

 Так главный поток может, например, узнать, сколько раз сохранялся чанк:
 \code
 int saved = 0;
 {
     std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine()); // Та же блокировка, что у потока очереди.
     saved = saver->s_SavedChunks;
 } // Здесь блокировка отпускается.
 \endcode

 Генератор нужен, когда чанка ещё нет в сохранении (canLoad() вернула false): тогда движок вызывает генератор, а
 если генератора нет - создаёт пустой чанк (с предупреждением в логе). Наш генератор делает пол в один блок:
 \code
 // Генератор: в чанках с y = 0 - пол толщиной в один блок (по 256 вокселей на чанк). Вызывается, только если контроллер
 // сохранения не может загрузить чанк (canLoad() вернула false).
 class FloorGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         Vector3d<int> v = chunk->getVoxelAddress()->getVector(0); // Координаты чанка (в чанках).
         if(v.getY() != 0) return true; // Остальные чанки - пустые.
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
                 chunk->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new ARGBColorVoxel(ARGBColor(255, (v.getX() * size + x) * 8, 128, z * 16)), true, false);
         return true;
     }
 };
 \endcode
 И обработчик клавиш: F5 - сохранить мир, F9 - выгрузить чанк (1,0,0) с сохранением и загрузить его снова.
 Обработчик событий Irrlicht вызывается в главном потоке, поэтому здесь мы только запоминаем нажатие, а всё делаем в
 главном цикле.
 \code
 // Обработчик клавиш: F5 - сохранить мир, F9 - выгрузить чанк (1,0,0) с сохранением и загрузить его снова.
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_F5) s_Save = true;
             if(event.KeyInput.Key == KEY_F9) s_Reload = true;
         }
         return false;
     }
     bool s_Save = false;
     bool s_Reload = false;
 };
 \endcode
 \section t11setup Подключение контроллера
 В main() всё как обычно. Логгер нужен, чтобы видеть ошибки (без логгера движок их молча проглатывает), стандартные
 сообщения отключим.
 \code
 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     MyReceiver receiver;
     device->setEventReceiver(&receiver);
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
 \endcode
 Контроллер и генератор отдаём миру. До start() потоки враппера ещё не запущены, поэтому блокировка не нужна. Если
 же менять контроллер во время работы - только под блокировкой движка (getLockAccessToTheSimpleVoxelEngine()): его в
 этот момент может вызывать поток очереди. Мир сам удалит и контроллер, и генератор.
 \code
 factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
 FileSaveLoadController* saver = new FileSaveLoadController(isve, "data/lesson11"); // Мир хранится в папке data/lesson11.
 isve->getWorld(id)->setChunksSaveLoadController(saver); // До start() блокировка не нужна. Мир сам удалит контроллер.
 isve->getWorld(id)->setWorldGenerator(new FloorGenerator); // Чанка нет в сохранении - его создаст генератор.
 isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(1, 0, 0), true); // Два чанка: (0,0,0) и (1,0,0).
 ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
 camera->setPosition(vector3df(16, 12, -12));
 camera->setTarget(vector3df(16, 0, 8));
 isve->start();
 \endcode
 При первом запуске контроллер пуст, canLoad() возвращает false, и оба чанка создаёт генератор.

 \section t11save Сохранение во время игры
 Главный цикл. По F5 вызываем isve->saveAllChunks(id):
 \code
 unsigned long savenumber = 0; // Номер идущего сохранения (0 - сохранения нет).
 while(isve->runIrrDevice())
 {
     if(receiver.s_Save && savenumber == 0) // F5: сохранение ставится в очередь, игра не ждёт.
     {
         receiver.s_Save = false;
         savenumber = isve->saveAllChunks(id);
     }
     bool ok = false;
     if(savenumber != 0 && isve->isSaveCompleted(savenumber, &ok)) // Каждый кадр проверяем, закончилось ли.
     {
         printf(ok ? "World saved\n" : "World NOT saved\n");
         savenumber = 0;
     }
 \endcode
 saveAllChunks() ничего не сохраняет сама: она ставит в главную очередь отдельное задание (SAVE_CHUNKS) и сразу
 возвращает номер сохранения. Игра при этом не ждёт. Задания очереди выполняются строго по порядку, поэтому:
 - в сохранение попадёт всё, что поставлено в очередь до вызова, - блоки, поставленные по щелчку мыши за мгновение до
   F5, загрузка и выгрузка чанков, изменения из UserControllerForThread;
 - всё, что поставлено после вызова, в это сохранение не попадёт, даже если выполнится раньше, чем вы увидите
   результат;
 - пока сохранение идёт, мир никто не меняет (оно выполняется под блокировкой движка).

 Закончилось ли сохранение, узнаём по номеру: isSaveCompleted(номер, &ok) возвращает true, когда задание выполнено, и
 записывает в ok результат. Проверять можно хоть каждый кадр - это быстро. Результат false будет, если мира нет,
 миру не установлен контроллер сохранения, какой-то чанк не сохранился (saveChunk() вернула false) или движок удалили
 раньше, чем очередь дошла до задания. Враппер помнит результаты последних 1000 законченных сохранений - этого
 хватает с запасом, если не ждать результата часами.

 saveAllChunks() потокобезопасна: её можно вызывать из любого потока - например, из своего потока автосохранения раз
 в пять минут. Есть и второй параметр - wait:
 \code
 isve->saveAllChunks(id, true); // Дождаться окончания сохранения.
 \endcode
 С wait = true функция возвращается, только когда сохранение закончено. В главном потоке во время игры так делать не
 стоит - картинка встанет, пока очередь не дойдёт до сохранения и не выполнит его. И никогда не ждите сохранения,
 держа блокировку движка (getAccessToTheSimpleVoxelEngine() или lock_guard на getLockAccessToTheSimpleVoxelEngine()):
 заданию сохранения нужна та же блокировка, и программа зависнет навсегда.

 Ещё три случая, когда saveAllChunks() сохраняет сразу, прямо в вызывающем потоке (результат готов уже при возврате):
 - вызов из пользовательской функции UserControllerForThread (урок 10): она сама работает в потоке главной очереди, и
   задание ждало бы само себя. Сохранение попадает ровно в это место: всё, что функция успела изменить, сохранится;
 - движок ещё не запущен (до start()). Осторожно: задания, поставленные до start() (например, наш
   addChunksForRendering()), ещё не выполнены, и в такое сохранение не попадут;
 - поток очереди уже завершился после stop() - все задания к этому моменту выполнены. Если же stop() уже вызван (например,
   stop(false) из другого потока), а очередь ещё работает, задание сохранения встаёт перед заданием остановки - в
   сохранение попадает всё, что поставлено до вызова.

 \section t11unload Сохранение при выгрузке чанков
 Чанки сохраняются не только по saveAllChunks(), но и при выгрузке из памяти. По F9 мы выгружаем чанк (1,0,0) и тут
 же загружаем его снова:
 \code
 if(receiver.s_Reload) // F9: выгрузить чанк с сохранением и загрузить снова (пользуется тем же контроллером).
 {
     receiver.s_Reload = false;
     isve->deleteChunkForRendering(id, Vector3d<int>(1, 0, 0), true, true); // unload = true, save = true.
     isve->addChunkForRendering(id, Vector3d<int>(1, 0, 0), true); // load = true: контроллер загрузит сохранённое.
 }
 \endcode
 Параметры deleteChunkForRendering(): unload = true - не только убрать чанк из отрисовки, но и выгрузить из памяти,
 save = true - перед выгрузкой сохранить. Выгрузка выполняется в потоке главной очереди, и контроллер вызывается там
 же и так же, как при saveAllChunks(). Следующее задание - addChunkForRendering(..., load = true): теперь canLoad()
 возвращает true, и чанк загружает наш loadChunk(), а не генератор. Обратите внимание: у deleteChunk(s)ForRendering()
 save по умолчанию true, так что выгрузка без сохранения - это явный save = false (в TestProject3 так сделана клавиша Z).
 Без установленного контроллера сохранять некуда, и чанк просто выгружается.

 Стриминг (урок 6) выгружает чанки так же: ChunkStreamingPool::setSaveOnUnload(true) - и чанки, от которых ушли
 игроки, сохраняются тем же контроллером, в том же потоке. Только помните, что стриминг сохраняет лишь выгружаемые
 чанки: те, что ещё загружены, сохранит saveAllChunks() или сохранение при выходе.

 \section t11exit Правильный выход
 Осталось выйти из программы, ничего не потеряв. Ядро при удалении мира само ничего не сохраняет, поэтому сохранить
 нужно явно, и порядок здесь важен:
 \code
 isve->stop(); // Сначала - остановка: все поставленные задания выполнены, мир больше никто не меняет.
 unsigned long last = isve->saveAllChunks(id); // Очередь уже остановлена - сохранение выполняется сразу, здесь.
 bool lastok = false;
 isve->isSaveCompleted(last, &lastok);
 printf(lastok ? "World saved on exit\n" : "World NOT saved on exit\n");
 delete isve; // Удалит мир, а с ним и контроллер сохранения.
 device->drop();
 \endcode
 - сначала stop(): он выполняет все задания, поставленные до вызова (последние щелчки игрока тоже), и
   останавливает потоки. После этого мир никто не меняет и не читает;
 - потом сохранение. Очередь уже остановлена, поэтому saveAllChunks(id) выполняется сразу, здесь же, и
   isSaveCompleted() тут же даёт результат. Можно вместо неё вызвать и isve->getWorld(id)->saveAllChunks(), как в
   TestProject3: после stop() это уже безопасно;
 - и только потом delete isve: он удалит миры, а с ними контроллеры сохранения и генераторы.

 Подойдёт и обратный порядок - saveAllChunks(id), потом stop(): stop() сначала выполнит все задания очереди, в том
 числе и сохранение. А вот так делать нельзя:
 - delete isve без сохранения - всё, что не выгружалось, пропадёт;
 - World::saveAllChunks() до stop() (без блокировки движка) - мир может меняться прямо во время сохранения;
 - saveAllChunks(id, true) под блокировкой движка - зависание (см. выше).

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cstdio>
 #include <fstream>
 #include <string>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace Defines;
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

 // Контроллер сохранения и загрузки (урок 3 SimpleVoxelEngine): каждый чанк - в своём текстовом файле
 // <папка>/chunk_<x>.<y>.<z>: сначала число непустых вокселей, потом цвета всех ячеек чанка (0 - пустая ячейка).
 // Формат - простейший, как в уроке 3 SimpleVoxelEngine. Враппер вызывает контроллер в потоке главной очереди под блокировкой движка.
 class FileSaveLoadController : public ChunksSaveLoadController
 {
 public:
     FileSaveLoadController(IrrSimpleVoxelEngine* isve, std::string folder) : s_ISVE(isve), s_Folder(folder) {}
     std::string getFileName(Vector3d<int> vect) // Файл чанка: <папка>/chunk_x.y.z
     {
         return s_Folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER);
     }
     bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::string filename = getFileName(vect);
         std::ofstream file(filename.c_str()); // Файл перезаписывается с начала.
         if(!file) // Папки ещё нет - создадим её и откроем файл снова.
         {
             WorkFunctions::FileFunctions::createFolders(s_Folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
             file.open(filename.c_str(), std::ios::out | std::ios::trunc);
             if(!file)
             {
                 s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved: error opening file " + filename);
                 return false;
             }
         }
         const unsigned int notnull = cvo->getNumberOfNotNullSimpleVoxelObjects();
         file << notnull;
         if(notnull != 0) // В пустом чанке массивов вокселей может не быть - не трогаем их.
         {
             int n = cvo->getNumberOfPartitions();
             SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects();
             for(int i = 0; i < n * n * n; i++)
             {
                 u32 color = 0; // Пустая ячейка; вложенные чанки и чужие воксели тоже не сохраняем.
                 if(svos[i] && svos[i]->getTypeVoxel() == ARGBCOLOR)
                 {
                     ARGBColor c = ((ARGBColorVoxel*)svos[i])->getARGBColor();
                     color = SColor(c.getA(), c.getR(), c.getG(), c.getB()).color; // Цвет одним числом.
                 }
                 file << " " << color;
             }
         }
         if(!file) // Ошибка записи (например, кончилось место на диске).
         {
             s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved: error writing file " + filename);
             return false;
         }
         s_SavedChunks++;
         return true;
     }
     bool loadChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::string filename = getFileName(vect);
         std::ifstream file(filename.c_str());
         unsigned int notnull = 0;
         if(!file || !(file >> notnull))
         {
             s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not loaded: error reading file " + filename);
             return false;
         }
         if(notnull == 0) return true; // Чанк был пустым - так и оставляем.
         int n = cvo->getNumberOfPartitions();
         SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Создаёт массивы чанка.
         bool* recalculate = cvo->getReCalculateVisibleCacheSimpleVoxelObjects();
         unsigned int created = 0;
         for(int i = 0; i < n * n * n; i++)
         {
             u32 color = 0;
             if(!(file >> color)) // Файл обрезан или испорчен - мир удалит недозагруженный чанк сам.
             {
                 s_ISVE->registerEvent(EVENT_TYPE_ERROR, "Chunk not loaded: damaged file " + filename);
                 return false;
             }
             if(!color) continue;
             SColor c(color);
             svos[i] = new ARGBColorVoxel(ARGBColor(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue()));
             recalculate[i] = true; // Видимость граней мы не сохраняли - пусть движок пересчитает.
             created++;
         }
         cvo->setNumberOfNotNullSimpleVoxelObjects(created);
         return true;
     }
     bool canLoad(World*, Vector3d<int> vect) { return WorkFunctions::FileFunctions::fileExists(getFileName(vect)); } // Есть файл - загружаем.
     int s_SavedChunks = 0; // Сколько раз сохранялся чанк (для вывода).
 private:
     IrrSimpleVoxelEngine* s_ISVE; // Для сообщений в лог.
     std::string s_Folder; // Папка мира.
 };

 // Генератор: в чанках с y = 0 - пол толщиной в один блок (по 256 вокселей на чанк). Вызывается, только если контроллер
 // сохранения не может загрузить чанк (canLoad() вернула false).
 class FloorGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         Vector3d<int> v = chunk->getVoxelAddress()->getVector(0); // Координаты чанка (в чанках).
         if(v.getY() != 0) return true; // Остальные чанки - пустые.
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
                 chunk->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new ARGBColorVoxel(ARGBColor(255, (v.getX() * size + x) * 8, 128, z * 16)), true, false);
         return true;
     }
 };

 // Обработчик клавиш: F5 - сохранить мир, F9 - выгрузить чанк (1,0,0) с сохранением и загрузить его снова.
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_F5) s_Save = true;
             if(event.KeyInput.Key == KEY_F9) s_Reload = true;
         }
         return false;
     }
     bool s_Save = false;
     bool s_Reload = false;
 };

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     MyReceiver receiver;
     device->setEventReceiver(&receiver);
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     FileSaveLoadController* saver = new FileSaveLoadController(isve, "data/lesson11"); // Мир хранится в папке data/lesson11.
     isve->getWorld(id)->setChunksSaveLoadController(saver); // До start() блокировка не нужна. Мир сам удалит контроллер.
     isve->getWorld(id)->setWorldGenerator(new FloorGenerator); // Чанка нет в сохранении - его создаст генератор.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(1, 0, 0), true); // Два чанка: (0,0,0) и (1,0,0).
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(16, 12, -12));
     camera->setTarget(vector3df(16, 0, 8));
     isve->start();
     unsigned long savenumber = 0; // Номер идущего сохранения (0 - сохранения нет).
     while(isve->runIrrDevice())
     {
         if(receiver.s_Save && savenumber == 0) // F5: сохранение ставится в очередь, игра не ждёт.
         {
             receiver.s_Save = false;
             savenumber = isve->saveAllChunks(id);
         }
         bool ok = false;
         if(savenumber != 0 && isve->isSaveCompleted(savenumber, &ok)) // Каждый кадр проверяем, закончилось ли.
         {
             printf(ok ? "World saved\n" : "World NOT saved\n");
             savenumber = 0;
         }
         if(receiver.s_Reload) // F9: выгрузить чанк с сохранением и загрузить снова (пользуется тем же контроллером).
         {
             receiver.s_Reload = false;
             isve->deleteChunkForRendering(id, Vector3d<int>(1, 0, 0), true, true); // unload = true, save = true.
             isve->addChunkForRendering(id, Vector3d<int>(1, 0, 0), true); // load = true: контроллер загрузит сохранённое.
         }
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop(); // Сначала - остановка: все поставленные задания выполнены, мир больше никто не меняет.
     unsigned long last = isve->saveAllChunks(id); // Очередь уже остановлена - сохранение выполняется сразу, здесь.
     bool lastok = false;
     isve->isSaveCompleted(last, &lastok);
     printf(lastok ? "World saved on exit\n" : "World NOT saved on exit\n");
     delete isve; // Удалит мир, а с ним и контроллер сохранения.
     device->drop();
     return 0;
 }
 \endcode
*/
