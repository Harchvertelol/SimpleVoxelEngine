/*! \page tutorial6 Урок 6: Стриминг чанков: загрузка вокруг игроков и границы мира.
 До сих пор мы загружали чанки одной областью: addChunksForRendering() от одного угла до другого. Для маленькой сцены
 это удобно, но мир игры обычно намного больше памяти. Нужно, чтобы чанки загружались вокруг игрока, пока он идёт, а
 те, от которых он ушёл, выгружались (и сохранялись). Для этого во враппере есть стриминг: пул контроллеров
 (ChunkStreamingPool) и контроллеры (ChunkStreamingController) - "игроки" или "камеры", вокруг которых держится мир.

 В этом уроке камера летит над холмами туда и обратно, мир грузится вокруг неё, сзади выгружается с сохранением, а
 при возвращении загружается из сохранения - вместе с нашими изменениями. Рядом стоит второй игрок, вокруг которого мир
 загружен всегда, а в стороне - площадка, загруженная "вручную", без стриминга.

 \section tut6_world Мир: генератор, сохранение и границы
 Блоки и фабрика - как в прошлых уроках (куб из видимых граней, цвет вершин по номеру блока). Генератор строит холмы
 высотой от 14 до 26 блоков, сверху трава:
 \code
 const int height = 20 + (int)(6 * sinf(gx * 0.1f) * cosf(gz * 0.13f));
 \endcode
 Генератор вызывается в потоке главной очереди для каждого чанка, которого нет ни в памяти, ни в сохранении (подробно
 о генераторе - в уроке 2 SimpleVoxelEngine).

 Чтобы чанки при выгрузке не пропадали, нужен контроллер сохранения. Настоящий (в файлы) разобран в уроке 3
 SimpleVoxelEngine, а здесь для простоты сохраняем "в память": выгруженный чанк кладём в std::map, а при новой загрузке
 берём оттуда, а не из генератора. Мир сначала спрашивает canLoad(): если чанк сохранён - вызывается loadChunk(), иначе
 генератор:
 \code
 bool canLoad(World*, Vector3d<int> vect) // Сохранён ли чанк. Если нет - мир вызовет генератор.
 {
     std::lock_guard<std::mutex> lock(s_Lock);
     return s_Chunks.find(vect) != s_Chunks.end();
 }
 \endcode
 Контроллер сохранения, как и генератор, вызывается в потоке главной очереди под блокировкой движка. Загрузка в нём
 устроена так же, как в уроке 3 SimpleVoxelEngine: блоки кладутся прямо в массив getSimpleVoxelObjects(), для каждого
 ставится флаг "пересчитать видимость", а в конце - число непустых ячеек.

 Теперь границы мира. Бесконечным мир бывает только по горизонтали, а по высоте его обычно ограничивают. Границы задаёт
 ядро (World), подробно о них - в уроке 2 SimpleVoxelEngine; здесь хватит одной строки:
 \code
 world->setChunksBounds(WORLD_AXIS_Y, 0, 2); // Граница мира: чанки по Y только от 0 до 2 (высота 48 блоков).
 \endcode
 За границей чанков просто нет: они не загружаются и не генерируются (генератор для них не вызывается), а стриминг их
 не берёт, какой бы ни была область игрока, - без сообщений в логе (сколько таких чанков - s_OutOfBounds, о нём ниже).
 addChunksForRendering() тоже молча пропускает чанки за границей. Только одиночный World::loadChunk() для чанка за
 границей пишет в лог нарушение логики; как его выключить - в уроке 2 SimpleVoxelEngine. Границы задавайте до start()
 (или под блокировкой движка).

 \section tut6_pool Пул стриминга
 Пул один на движок, его создаёт и удаляет сам движок: getChunkStreamingPool(). Настройки пула:
 \code
 ChunkStreamingPool* pool = isve->getChunkStreamingPool();
 pool->setMaxPendingActions(64); // Сколько действий пула может ждать в главной очереди.
 pool->setMaxLoadsPerStep(16); // Сколько чанков за шаг ставится на загрузку.
 pool->setMaxUnloadsPerStep(16); // ... и на выгрузку.
 pool->setSaveOnUnload(true); // Выгружаемые чанки сохранять (через MemorySaver).
 \endcode
 Здесь все значения - по умолчанию, их можно и не писать. Зачем пределы? Пул ставит задания в главную очередь движка:
 загрузку - по одному чанку на задание, ближние к игроку чанки первыми. Очередь заданий движка ничем не ограничена, и если
 игрок летит быстро, без пределов в неё попали бы тысячи заданий (и тысячи чанков, которые к моменту загрузки уже не
 нужны). С пределами в очереди не больше setMaxPendingActions() действий пула, а между ними успевают выполняться
 остальные задания - правки мира, сохранение, ваши функции UserControllerForThread: они не ждут, пока загрузится вся
 область. Больше пределы - быстрее загрузка, но дольше ждут остальные задания.

 setSaveOnUnload(true) - выгружаемые чанки сохраняются через контроллер сохранения мира. Если контроллера у мира нет,
 чанк просто выгружается (сохранять некуда). false - выгружать без сохранения (например, если мир всегда можно заново
 сгенерировать и игрок его не меняет).

 Ещё две настройки пула (здесь не меняются): setRetryDelayMilliseconds() - через сколько миллисекунд повторить
 загрузку или выгрузку, которая не удалась (генератор или контроллер вернул false; по умолчанию 2000), и
 setRecheckIntervalMilliseconds() - как часто перепроверять, что на самом деле загружено, даже если все стоят на месте
 (так пул замечает изменения не через него, например чанк, выгруженный вручную; по умолчанию 1000). Для каждой
 настройки есть и get...().

 \section tut6_controller Контроллер: игрок-камера
 Контроллер создаёт пул: addController(имя, мир). Имя - любая строка, уникальная в пуле (например, имя игрока); если
 имя занято или мира нет, вернётся NULL. Контроллером владеет пул - не удаляйте его сами.
 \code
 ChunkStreamingController* player = pool->addController("camera", id);
 player->setShape(STREAMING_SHAPE_CYLINDER);
 player->setRadius(Vector3d<int>(3, 2, 3));
 player->setUnloadMargin(1);
 \endcode
 Форма области (setShape()):
 - STREAMING_SHAPE_CUBOID - кубоид: чанки с |dx| <= rx, |dy| <= ry, |dz| <= rz от чанка игрока (при радиусе 1 - 27 чанков);
 - STREAMING_SHAPE_CYLINDER - цилиндр с вертикальной осью (по умолчанию): круг (эллипс) по X и Z радиусом на полчанка
   больше заданного (rx + 0.5), по Y - |dy| <= ry
   (при радиусах (2, 0, 2) - 21 чанк, при (3, 0, 3) - 37);
 - STREAMING_SHAPE_ELLIPSOID - шар или эллипсоид (тоже с запасом в полчанка; при радиусе 1 - 19 чанков).
 Для обычной игры удобен цилиндр: смотрят в основном вдаль, а не вверх и вниз.

 Радиусы (setRadius()) - в чанках по X, Y, Z, по умолчанию (4, 2, 4). У нас цилиндр радиусом 3 и высотой от -2 до +2
 чанков от чанка камеры: по 37 чанков в слое. Камера на высоте 40 - в чанке Y = 2, область по Y - от 0 до 4, но слои
 3 и 4 за границей мира, так что в отрисовке 3 слоя - 111 чанков. Помните, что число чанков растёт как квадрат (а для
 шара - как куб) радиуса: цилиндр радиусом 8 и высотой 9 слоёв - это уже около 2000 чанков.

 У области есть два "кольца":
 - <b>кольцо данных</b>: чанки на один дальше радиуса загружаются в память, но не рисуются. Зачем? Видимость граней на
   краю чанка зависит от соседнего чанка. Если соседа нет в памяти, движок не знает, закрыта ли грань, и рисует её - на
   краю области появилась бы "стенка", которая исчезнет, когда сосед загрузится, и чанк пришлось бы перестраивать. Поэтому
   чанк области ставится в отрисовку, только когда загружены его соседи;
 - <b>запас выгрузки</b> (setUnloadMargin(), по умолчанию 1): уже взятый чанк отпускается, только когда он дальше
   радиуса плюс запас. Без запаса игрок, который ходит туда-сюда по границе чанка, заставлял бы пул на каждом шаге
   загружать и выгружать целый слой чанков.

 Для каждой настройки есть get...(): getShape(), getRadius(), getUnloadMargin(), а также getName() и getWorldId().
 Проверить, попадает ли чанк в область, можно функцией isInArea(центр, чанк, extra): extra = 0 - сама область,
 1 - с кольцом данных (например, чтобы не давать игроку ставить блоки там, где мир ещё не загружен).

 \section tut6_more Несколько игроков
 Контроллеров может быть сколько угодно. Второй игрок у нас стоит на месте - его положение задано чанком:
 \code
 ChunkStreamingController* friendplayer = pool->addController("friend", id);
 friendplayer->setShape(STREAMING_SHAPE_CUBOID);
 friendplayer->setRadius(Vector3d<int>(1, 1, 1));
 friendplayer->setChunkPosition(Vector3d<int>(8, 1, 0));
 \endcode
 Когда камера пролетает мимо, их области пересекаются. Общий чанк нужен обоим - у него два владельца, и выгрузится он,
 только когда не нужен ни одному. Владельцев чанка показывает getChunkOwners(мир, чанк) - набор имён контроллеров:
 \code
 std::set<std::string> owners = pool->getChunkOwners(id, Vector3d<int>(8, 1, 0)); // (пример, не из полного кода)
 if(owners.count("friend")) { ... } // Чанк держит "друг".
 \endcode
 Остальное управление контроллерами: getController(имя) - контроллер по имени (или NULL), getControllerNames() - имена
 всех, removeController(имя) - удалить (указатель после этого недействителен; чанки, которые больше никому не нужны,
 выгрузятся на следующих шагах), removeAllControllers() - удалить все.

 \section tut6_manual Чанки "вручную", без стриминга
 Старый способ никуда не делся: чанки можно загружать и убирать самому.
 \code
 isve->addChunksForRendering(id, Vector3d<int>(-8, 0, -1), Vector3d<int>(-7, 1, 1), true, true);
 \endcode
 addChunksForRendering(мир, угол, угол, load, loadframe, sizeframe) ставит в отрисовку кубоид чанков. load = true -
 загрузить (или сгенерировать) чанки, которых нет в памяти; с load = false чанки, которых нет в памяти, не появятся.
 loadframe = true - загрузить в память ещё и рамку толщиной sizeframe (по умолчанию 1) вокруг кубоида - это то же самое
 кольцо данных: без него на краях кубоида рисуются лишние грани. У нас рамка снизу (Y = -1) за границей мира - такие чанки
 просто пропускаются, без сообщений в логе. Для одного чанка есть addChunkForRendering(мир, чанк, load,
 loadframe, sizeframe).

 Убрать из отрисовки: deleteChunksForRendering(мир, угол, угол, unload, save, unloadframe, sizeframe) и
 deleteChunkForRendering(мир, чанк, unload, save, unloadframe, sizeframe): unload = true - ещё и выгрузить из памяти,
 save = true - перед выгрузкой сохранить (контроллером сохранения мира), unloadframe - выгрузить и рамку.
 \code
 isve->deleteChunksForRendering(id, Vector3d<int>(-8, 0, -1), Vector3d<int>(-7, 1, 1), true, true, true); // (пример, не из полного кода)
 \endcode
 Все эти функции только ставят задания в главную очередь, выполнятся они после start().

 Пул и ручные чанки уживаются: чанки, загруженные или поставленные в отрисовку не пулом, пул не выгружает и не убирает
 из отрисовки - даже если игрок пришёл к ним и ушёл. А если чанк пула кто-то выгрузил вручную, а он ещё нужен игроку,
 пул загрузит его снова. Поэтому площадка в нашем примере остаётся на месте всё время.

 \section tut6_loop Главный цикл и экран загрузки
 Камера у нас - обычная (не FPS): её ведёт сама программа.
 \code
 ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNode();
 \endcode
 Каждый кадр, в главном потоке, говорим контроллеру, где камера, и делаем шаг пула:
 \code
 player->setScenePosition(camera->getAbsolutePosition()); // Где сейчас камера.
 pool->step(); // Шаг стриминга: какие чанки загрузить, а какие выгрузить.
 isve->update();
 \endcode
 setScenePosition() принимает точку в координатах сцены Irrlicht, а пул сам переводит её в координаты мира через
 getWorldCoordinates() - с учётом родительской ноды мира и сдвига (урок 12). Есть и другие способы задать положение:
 setWorldPosition() - точка в координатах мира (как у getWorldCoordinates()), setChunkPosition() - сразу чанк.
 Каким способом задано положение, говорит getPositionMode(): STREAMING_POSITION_SCENE, STREAMING_POSITION_WORLD,
 STREAMING_POSITION_CHUNK или STREAMING_POSITION_NONE - ещё никак (такой контроллер ничего не загружает). Чанк, вокруг которого пул загружал область на последнем шаге, - getCenterChunk() (а hasCenterChunk()
 говорит, был ли уже такой шаг).

 step() - сердце стриминга. Шаг дешёвый: если ни один контроллер не перешёл в другой чанк и ничего не изменилось, он
 сразу возвращается. Если перешёл - пул сравнивает, какие чанки нужны, с тем, что на самом деле загружено и
 отрисовывается, и ставит задания в главную очередь. Все функции пула и контроллеров - только из главного потока
 (в отладочной сборке SVE_DEBUG_CHECKS это проверяется). Загружает чанки поток главной очереди, поэтому до start() задания
 только ждут.

 Пока область вокруг камеры не готова, показываем "экран загрузки" - здесь просто заголовок окна со счётчиком:
 \code
 ChunkStreamingState st = pool->getState("camera");
 ...
 if(pool->isAreaReady("camera")) // Можно играть.
 \endcode
 getState() возвращает ChunkStreamingState:
 - s_RenderWanted и s_Rendered - сколько чанков в области и сколько из них уже загружено и отрисовывается;
 - s_MemoryWanted и s_Loaded - то же для области вместе с кольцом данных;
 - s_Failed - сколько не загрузилось (пул повторит позже), s_OutOfBounds - сколько чанков области за границей мира
   (их нет и не будет; у нас - 74 чанка в слоях 3 и 4);
 - s_PendingActions - сколько действий пула (всех контроллеров) ещё ждёт в очереди; то же число отдаёт
   pool->getPendingActions().
 isAreaReady() - true, когда все чанки области загружены и отрисовываются. Меши при этом могут ещё строиться в фоне
 (ноды появятся через несколько кадров). Если игрок с гравитацией появится до готовности области, он провалится сквозь
 ещё не загруженный пол - поэтому и нужен экран загрузки.

 Когда область готова, ставим над холмом красную башню - она покажет, что изменения переживают выгрузку - и начинаем
 полёт:
 \code
 for(int y = 27; y < 32; y++) // Красная башня над холмом - проверим, что она переживёт выгрузку.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(8, y, 8), new Block(BLOCK_TOWER));
 isve->addTaskUpdateChunks();
 \endcode
 Камера летит по X туда и обратно, от чанка 0 до чанка 16, за минуту:
 \code
 const float seconds = (device->getTimer()->getTime() - flightstart) / 1000.f;
 const float x = 8 + 128 * (1 - cosf(seconds * 0.1f)); // Туда-обратно от 8 до 264 (чанки от 0 до 16) за минуту.
 camera->setPosition(vector3df(x, 40, 8));
 camera->setTarget(vector3df(x + 20, 20, 8));
 \endcode
 Впереди мир загружается, позади - выгружается и сохраняется в MemorySaver. Когда камера вернётся, чанки старта
 загрузятся уже из сохранения - вместе с башней. Чанки "друга" всё это время остаются на месте, площадка - тоже.

 \section tut6_end Завершение
 \code
 isve->stop();
 delete isve; // Удаляет и пул с контроллерами, и мир (с генератором и MemorySaver).
 \endcode
 Пул с контроллерами удаляется вместе с движком (чанки при этом не трогаются). Если мир сохраняется в файлы, перед
 удалением движка сохраните и то, что ещё загружено (урок 11: saveAllChunks()).

 \section tut6_errors Типичные ошибки
 - Забыли pool->step() в цикле - ничего не грузится. Вызов step() из другого потока - ошибка (в отладочной сборке -
   сообщение в лог).
 - Масштабы фабрики setPositionScale()/setSScale() не равны размеру чанка - точка камеры переводится не в тот чанк,
   и мир грузится "не там".
 - Слишком большие радиусы - память и время загрузки растут как квадрат радиуса (у шара - как куб).
 - Указатель на контроллер после removeController() или удаление контроллера через delete - контроллером владеет пул.
 - Игрок с гравитацией до isAreaReady() - проваливается.
 - Чанки, загруженные вручную, "почему-то не выгружаются" - пул не трогает чужие чанки, выгружайте их сами
   (deleteChunksForRendering()).

 \section tut6_check Как проверить
 Проверка урока (AI/AITests/tutorials/diag_tut6.cpp) двигает камеру не по времени, а по сценарию: ждёт готовности
 области, летит к "другу" (чанк (8, 1, 0) - два владельца), дальше до чанка 16 (старт выгружен и сохранён, чанки
 "друга" и площадка на месте) и обратно (башня загружена из сохранения).

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <atomic>
 #include <cmath>
 #include <map>
 #include <mutex>
 #include <vector>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 enum BLOCKS { BLOCK_STONE = 1, BLOCK_GRASS = 2, BLOCK_TOWER = 3 }; // Номера блоков (0 - "пусто" в сохранении).

 struct BlockInfo : VoxelInfo
 {
     unsigned char s_Id;
 };

 class Block : public SimpleVoxelObject
 {
 public:
     Block(unsigned char id) : s_Id(id) { setTypeVoxel(CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo()
     {
         BlockInfo* info = new BlockInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         return info;
     }
     unsigned char getId() { return s_Id; }
 private:
     unsigned char s_Id;
 };

 // Фабрика: куб из видимых граней, цвет вершин - по номеру блока.
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
         const SColor colors[4] = { SColor(255, 255, 255, 255), SColor(255, 120, 120, 120), SColor(255, 60, 170, 60), SColor(255, 200, 40, 40) };
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         if(mesh) s_MeshManipulator->setVertexColors(mesh, colors[((BlockInfo*)vxl->getVoxelInfo())->s_Id % 4]);
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };

 // Генератор: холмы высотой от 14 до 26 блоков (чанки по Y - 0 и 1), сверху трава.
 // Вызывается в потоке главной очереди для каждого чанка, которого нет ни в памяти, ни в сохранении.
 class MyGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
     {
         const Vector3d<int> c = chunk->getVoxelAddress()->getVector(0); // Координаты чанка (в чанках).
         for(int x = 0; x < sizechunk; x++)
             for(int z = 0; z < sizechunk; z++)
             {
                 const float gx = c.getX() * sizechunk + x, gz = c.getZ() * sizechunk + z; // Глобальные координаты столбца.
                 const int height = 20 + (int)(6 * sinf(gx * 0.1f) * cosf(gz * 0.13f));
                 for(int y = 0; y < sizechunk; y++)
                 {
                     const int gy = c.getY() * sizechunk + y;
                     if(gy <= height) chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new Block(gy == height ? BLOCK_GRASS : BLOCK_STONE), true, false);
                 }
             }
         return true;
     }
 };

 // Контроллер сохранения "в память": выгруженный чанк хранится в std::map, при новой загрузке берётся оттуда,
 // а не из генератора - так изменения мира не пропадают при выгрузке. Настоящий контроллер (в файлы) - в уроке 3
 // SimpleVoxelEngine. Вызывается в потоке главной очереди под блокировкой движка.
 class MemorySaver : public ChunksSaveLoadController
 {
 public:
     MemorySaver() : s_Saved(0), s_Loaded(0) {}
     bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::lock_guard<std::mutex> lock(s_Lock);
         std::vector<unsigned char>& data = s_Chunks[vect];
         data.clear(); // Пустой вектор - пустой чанк.
         if(cvo->getNumberOfNotNullSimpleVoxelObjects() > 0)
         {
             const int n = cvo->getNumberOfPartitions();
             SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects();
             data.resize(n * n * n, 0);
             for(int i = 0; i < n * n * n; i++)
                 if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) data[i] = ((Block*)svos[i])->getId();
         }
         s_Saved++;
         return true;
     }
     bool loadChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
     {
         std::lock_guard<std::mutex> lock(s_Lock);
         const std::vector<unsigned char>& data = s_Chunks[vect];
         s_Loaded++;
         if(data.empty()) return true;
         SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects();
         bool* recalculate = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Видимость граней посчитает движок.
         unsigned int created = 0;
         for(unsigned int i = 0; i < data.size(); i++)
             if(data[i])
             {
                 svos[i] = new Block(data[i]);
                 recalculate[i] = true;
                 created++;
             }
         cvo->setNumberOfNotNullSimpleVoxelObjects(created);
         return true;
     }
     bool canLoad(World*, Vector3d<int> vect) // Сохранён ли чанк. Если нет - мир вызовет генератор.
     {
         std::lock_guard<std::mutex> lock(s_Lock);
         return s_Chunks.find(vect) != s_Chunks.end();
     }
     std::mutex s_Lock; // Сохранённое можно смотреть и из главного потока (например, для статистики); счётчики - атомарные.
     std::map< Vector3d<int>, std::vector<unsigned char> > s_Chunks;
     std::atomic<int> s_Saved; // Сколько раз чанки сохранялись.
     std::atomic<int> s_Loaded; // Сколько раз загружались из сохранения.
 };

     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device);
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     World* world = isve->getWorld(id);
     world->setWorldGenerator(new MyGenerator); // Генератор и контроллер сохранения удалит мир.
     MemorySaver* saver = new MemorySaver;
     world->setChunksSaveLoadController(saver);
     world->setChunksBounds(WORLD_AXIS_Y, 0, 2); // Граница мира: чанки по Y только от 0 до 2 (высота 48 блоков).

     // Пул стриминга и его настройки (здесь - значения по умолчанию).
     ChunkStreamingPool* pool = isve->getChunkStreamingPool();
     pool->setMaxPendingActions(64); // Сколько действий пула может ждать в главной очереди.
     pool->setMaxLoadsPerStep(16); // Сколько чанков за шаг ставится на загрузку.
     pool->setMaxUnloadsPerStep(16); // ... и на выгрузку.
     pool->setSaveOnUnload(true); // Выгружаемые чанки сохранять (через MemorySaver).

     // Игрок-камера: цилиндр радиусом 3 чанка, по высоте - 2 чанка вверх и вниз.
     ChunkStreamingController* player = pool->addController("camera", id);
     player->setShape(STREAMING_SHAPE_CYLINDER);
     player->setRadius(Vector3d<int>(3, 2, 3));
     player->setUnloadMargin(1);

     // Второй игрок ("друг") стоит на месте в чанке (8, 1, 0): вокруг него мир загружен всегда.
     ChunkStreamingController* friendplayer = pool->addController("friend", id);
     friendplayer->setShape(STREAMING_SHAPE_CUBOID);
     friendplayer->setRadius(Vector3d<int>(1, 1, 1));
     friendplayer->setChunkPosition(Vector3d<int>(8, 1, 0));

     // Площадка, загруженная вручную (без стриминга): пул её не выгружает, даже когда рядом никого нет.
     isve->addChunksForRendering(id, Vector3d<int>(-8, 0, -1), Vector3d<int>(-7, 1, 1), true, true);

     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNode();
     camera->setPosition(vector3df(8, 40, 8));
     camera->setTarget(vector3df(28, 20, 8));
     isve->start();
     bool ready = false; // Загружена ли область вокруг камеры в первый раз (до этого - "экран загрузки").
     u32 flightstart = 0; // Когда началась прогулка камеры.
     while(isve->runIrrDevice())
     {
         player->setScenePosition(camera->getAbsolutePosition()); // Где сейчас камера.
         pool->step(); // Шаг стриминга: какие чанки загрузить, а какие выгрузить.
         isve->update();
         if(!ready)
         {
             ChunkStreamingState st = pool->getState("camera");
             stringw caption = L"Loading: ";
             caption += st.s_Rendered;
             caption += L" / ";
             caption += st.s_RenderWanted;
             device->setWindowCaption(caption.c_str());
             if(pool->isAreaReady("camera")) // Можно играть.
             {
                 ready = true;
                 flightstart = device->getTimer()->getTime();
                 for(int y = 27; y < 32; y++) // Красная башня над холмом - проверим, что она переживёт выгрузку.
                     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(8, y, 8), new Block(BLOCK_TOWER));
                 isve->addTaskUpdateChunks();
                 device->setWindowCaption(L"Streaming");
             }
         }
         else
         {
             const float seconds = (device->getTimer()->getTime() - flightstart) / 1000.f;
             const float x = 8 + 128 * (1 - cosf(seconds * 0.1f)); // Туда-обратно от 8 до 264 (чанки от 0 до 16) за минуту.
             camera->setPosition(vector3df(x, 40, 8));
             camera->setTarget(vector3df(x + 20, 20, 8));
         }
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop();
     delete isve; // Удаляет и пул с контроллерами, и мир (с генератором и MemorySaver).
     device->drop();
     return 0;
 }
 \endcode
*/
