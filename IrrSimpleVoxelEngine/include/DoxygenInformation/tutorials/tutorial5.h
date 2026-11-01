/*! \page tutorial5 Урок 5: Текстурный атлас.
 В уроке 3 у каждого вида блоков была своя текстура. Это просто, но не быстро. Давайте разберёмся, почему, и
 сделаем по-другому: одна текстура (атлас) на все блоки, а каждая грань берёт из неё свою клетку.

 \section tut5_why Почему атлас быстрее
 Враппер склеивает меши всех вокселей чанка, а потом и всех чанков блока чанков (setSizeBlockChunks()) в один меш ноды.
 Склейка (CBatchingMesh) кладёт в один мешбуфер только грани с одинаковым материалом, а текстура - часть материала
 (SMaterial). Поэтому, если в блоке чанков видно 13 разных текстур, у его ноды 13 мешбуферов на уровень, и каждый
 мешбуфер - это отдельный вызов отрисовки со сменой текстуры. Именно об этом комментарий у getMeshVoxel() в
 TestProject3/main.cpp: "текстурного атласа нет, поэтому каждая текстура даёт свой мешбуфер, а значит больше лагов".
 Вызовы отрисовки дороги сами по себе (работа драйвера на процессоре), и при сотнях нод на экране их число быстро растёт.

 С атласом у всех граней уровня один материал - и у ноды ровно один мешбуфер на уровень, сколько бы видов блоков в
 ней ни было. Число вызовов отрисовки равно числу нод. Кстати, цвет вершин в материал не входит: освещение через цвет
 вершин (как в TestProject3) мешбуферы не размножает.

 Одна оговорка: индексы в мешбуфере 16-битные, поэтому в одном мешбуфере не больше 65535 вершин (у грани их 4). Если блок
 чанков очень большой и в нём много видимых граней, склейка начнёт второй мешбуфер с тем же материалом. Это нормально:
 мешбуферов всё равно будет в разы меньше, чем с отдельными текстурами.

 \section tut5_code Пишем пример
 Начало - как в уроке 3: подключаем враппер и пространства имён. Атлас - 4x4 клетки по 16x16 пикселей. Простые
 блоки берут клетки с 0 по 11, а "трава" - три клетки: 12 сверху, 13 по бокам и 14 снизу.
 \code
 const int ATLAS_TILES = 4; // Атлас - 4x4 клетки.
 const int TILE_SIZE = 16; // Клетка - 16x16 пикселей, весь атлас - 64x64.
 const unsigned int BLOCK_GRASS = 100; // Блок "трава": сверху, сбоку и снизу - разные клетки атласа.
 \endcode
 Блок и его информация для отрисовки - самые простые: только номер (свой воксель с VoxelInfo разобран в уроке 2):
 \code
 struct BlockInfo : VoxelInfo
 {
     unsigned int s_Id;
 };
 \endcode
 Файлы уроку не нужны, поэтому атлас нарисуем прямо в коде: картинка 64x64, в каждой клетке свой цвет и тёмная рамка в
 один пиксель (чтобы было видно, где кончается блок). В настоящей игре атлас обычно загружают из файла
 (driver->getTexture("atlas.png")) - всё остальное в уроке от этого не меняется.
 \code
 ITexture* createAtlas(IVideoDriver* driver)
 {
     const int size = ATLAS_TILES * TILE_SIZE;
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(size, size));
     ...
 \endcode
 Самое интересное - конец функции. По умолчанию Irrlicht создаёт для каждой текстуры mip-уровни (уменьшенные копии).
 Для атласа они вредны (подробнее - ниже, в разделе о фильтрации), поэтому на время создания атласа выключаем флаг
 ETCF_CREATE_MIP_MAPS, а потом возвращаем, как было, чтобы не задеть остальные текстуры программы:
 \code
     const bool mipmaps = driver->getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
     driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false); // Атласу mip-уровни не нужны (см. текст урока).
     ITexture* atlas = driver->addTexture("atlas", image);
     driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, mipmaps); // Вернули флаг для остальных текстур.
     image->drop();
     return atlas;
 }
 \endcode

 \section tut5_uv Текстурные координаты граней
 Теперь главное: как дать грани нужную клетку атласа. Грань из createFaceMesh() - квадрат с текстурными координатами
 от 0 до 1 (то есть вся текстура). Нам нужна только клетка: для клетки в столбце col и строке row координата u должна
 идти от col / 4 до (col + 1) / 4, а v - от row / 4 до (row + 1) / 4.

 Поменять координаты можно в двух местах:
 - <b>до</b> getMeshCubeFaces() - в самой грани, которую мы ему отдаём (getMeshCubeFaces() копирует грани, и копии
   "запоминают" координаты);
 - <b>после</b> - в меше, который вернул getMeshCubeFaces(). Но в нём от 1 до 6 граней, уже повёрнутых и склеенных
   в один мешбуфер, и сколько их и в каком порядке - зависит от того, какие грани видимы. Разбираться в этом в каждом
   вызове getMeshVoxel() - лишняя работа и источник ошибок.

 Выбираем первый способ, причём в самом надёжном виде: заранее, в конструкторе фабрики, готовим по отдельной грани на
 каждую клетку атласа. Тогда getMeshVoxel() ничего не меняет, а только выбирает готовые грани. Можно было бы и менять
 одну общую грань перед каждым вызовом getMeshCubeFaces() (так TestProject3 меняет текстуру грани): вызовы
 getMeshVoxel() идут строго по одному, под общей блокировкой фабрики, так что это тоже работает. Но готовые грани
 проще, и их не нужно пересчитывать для каждого вокселя.
 \code
 MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
 {
     ITexture* atlas = createAtlas(device->getVideoDriver()); // Текстуры - только в главном потоке.
     for(int tile = 0; tile < ATLAS_TILES * ATLAS_TILES; tile++)
     {
         s_Faces[tile] = createFaceMesh(0); // Отступ 0: координаты ровно от 0 до 1, отступ сделаем сами.
         IMeshBuffer* mb = s_Faces[tile]->getMeshBuffer(0);
         const float col = tile % ATLAS_TILES, row = tile / ATLAS_TILES; // Где клетка в атласе.
         const float inset = 0.5f / TILE_SIZE; // Отступ в полпикселя клетки - от проступания соседних клеток.
         for(u32 i = 0; i < mb->getVertexCount(); i++)
         {
             vector2df& tc = mb->getTCoords(i); // 0 или 1 - угол грани.
             tc.X = (col + inset + tc.X * (1 - 2 * inset)) / ATLAS_TILES;
             tc.Y = (row + inset + tc.Y * (1 - 2 * inset)) / ATLAS_TILES;
         }
         mb->getMaterial().setTexture(0, atlas); // У всех граней один и тот же материал - одна текстура.
     }
 }
 ~MyFactory() { for(int tile = 0; tile < ATLAS_TILES * ATLAS_TILES; tile++) s_Faces[tile]->drop(); }
 \endcode
 Обратите внимание: атлас создаётся в конструкторе фабрики. Конструктор работает в главном потоке, а getMeshVoxel() -
 в потоках построения, где трогать драйвер (создавать или загружать текстуры) нельзя.

 Что за отступ inset? У createFaceMesh() есть свой параметр texinset (по умолчанию 0.002): координаты идут от texinset
 до 1 - texinset, чтобы при фильтрации на краю грани не проступал противоположный край текстуры. Но этот отступ
 считается от всей текстуры, а в атласе соседи у клетки - другие клетки. Поэтому берём грань с отступом 0 и делаем
 отступ сами, в долях клетки: полпикселя (0.5 / TILE_SIZE). Тогда крайние точки грани попадают в центры крайних
 пикселей клетки, и ни при выборке ближайшего пикселя (ошибки округления), ни при билинейной фильтрации (она смешивает
 соседние пиксели) цвет соседней клетки на грань не попадёт. Без отступа на стыках блоков видны тонкие линии чужого цвета.

 Функция меша вокселя теперь совсем короткая. Порядок граней в массиве - как в массиве видимости: x+, z+, x-, z-, y+, y-:
 \code
 IMesh* getMeshVoxel(VoxelForRendering* vxl)
 {
     const unsigned int id = ((BlockInfo*)vxl->getVoxelInfo())->s_Id;
     if(id == BLOCK_GRASS) // Порядок граней: x+, z+, x-, z-, y+, y-.
     {
         IMesh* faces[6] = { s_Faces[13], s_Faces[13], s_Faces[13], s_Faces[13], s_Faces[12], s_Faces[14] }; // Бока, верх, низ.
         return getMeshCubeFaces(vxl, faces, vector3df(1, 1, 1));
     }
     return getMeshCubeFaces(vxl, s_Faces[id % 12], vector3df(1, 1, 1)); // Простые блоки - клетки 0..11.
 }
 \endcode
 У "травы" три разные клетки, но материал у всех граней один и тот же - значит, и мешбуфер один.

 \section tut5_filter Фильтрация и mip-уровни
 Откуда у ноды чанка материал? Текстура приходит из материала граней (мы задали её в конструкторе фабрики), тип
 материала задаёт setMaterialType() уровня, а флаги - setMaterialFlag(). Фабрика запоминает флаги уровня и ставит их
 каждой новой ноде (ноды пересоздаются при каждой перестройке блока чанков), а также сразу применяет к уже созданным.
 Вызывается setMaterialFlag() в главном потоке, после addWorld() - флаги задаются для мира.
 \code
 factory->setMaterialFlag(0, id, EMF_BILINEAR_FILTER, false); // Чёткие пиксели и никакого смешивания с соседними клетками.
 factory->setMaterialFlag(0, id, EMF_TRILINEAR_FILTER, false);
 factory->setMaterialFlag(0, id, EMF_ANISOTROPIC_FILTER, false);
 factory->setMaterialFlag(0, id, EMF_USE_MIP_MAPS, false); // Даже если у текстуры есть mip-уровни - не использовать.
 \endcode
 Зачем всё это?
 - Билинейная фильтрация (EMF_BILINEAR_FILTER, в Irrlicht включена по умолчанию) смешивает четыре соседних пикселя.
   С отступом в полпикселя на краях клетки это безопасно, но картинка размыта. Для "пиксельных" текстур (как в
   Minecraft) её выключают - берётся ближайший пиксель. Трилинейная и анизотропная фильтрации тоже смешивают пиксели,
   их выключаем заодно.
 - Mip-уровни - главная беда атласа. На уровне k пиксель в 2^k раз больше: клетка 16x16 на четвёртом уровне - один
   пиксель, а дальше клетки смешиваются друг с другом. Вдали грани получают цвет соседних клеток, на стыках появляются
   цветные полосы, и никакой отступ в полпикселя от этого не спасает. Поэтому атласу mip-уровни не создаём
   (ETCF_CREATE_MIP_MAPS при создании текстуры) и не используем (EMF_USE_MIP_MAPS для уровня мешей). Цена - вдали
   мелкий узор текстуры "мерцает".

 Если мерцание мешает, есть компромиссы: делать клетки с запасом (поле в несколько пикселей вокруг клетки, повторяющее
 её край, - тогда несколько первых mip-уровней не смешивают соседей) или вместо атласа использовать массивы текстур
 (texture arrays) в своём шейдере - но это уже за рамками Irrlicht 1.8.

 Ещё одно ограничение атласа: текстуру нельзя "повторить" по большой грани (TextureWrap), ведь за краем клетки - другая
 клетка. Для кубиков-вокселей, где каждая грань - одна клетка, это не мешает.

 \section tut5_errors Типичные ошибки
 - Разные флаги или тип материала у разных граней одного уровня (например, у одной грани выключили освещение в её
   материале) - материалы снова разные, и мешбуферы снова размножаются. Всё, кроме текстуры, лучше задавать через
   setMaterialType()/setMaterialFlag() уровня, а не в гранях.
 - Загрузка текстуры в getMeshVoxel() (driver->getTexture()) - это поток построения, драйвер там трогать нельзя.
 - Атлас с mip-уровнями или без отступа - цветные линии на стыках блоков, особенно вдали.
 - createFaceMesh() с отступом по умолчанию и пересчёт координат поверх него - отступ получится не тот, что нужен
   (в долях всего атласа, а не клетки). Берите createFaceMesh(0) и делайте отступ сами.

 \section tut5_check Как проверить
 Проверка урока (AI/AITests/tutorials/diag_tut5.cpp) после построения чанка смотрит ноду: getMesh()->getMeshBufferCount()
 должен быть равен 1, текстура материала - атлас, а координаты каждой грани - внутри одной клетки и не ближе
 полупикселя к её краю. Так же можно проверить и свою программу: посчитайте мешбуферы у нод чанков.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines; // CUSTOM_VOXEL и другие константы ядра.
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 const int ATLAS_TILES = 4; // Атлас - 4x4 клетки.
 const int TILE_SIZE = 16; // Клетка - 16x16 пикселей, весь атлас - 64x64.
 const unsigned int BLOCK_GRASS = 100; // Блок "трава": сверху, сбоку и снизу - разные клетки атласа.

 // Информация для отрисовки: номер блока (для простых блоков - номер клетки атласа).
 struct BlockInfo : VoxelInfo
 {
     unsigned int s_Id;
 };

 // Наш блок - только номер.
 class Block : public SimpleVoxelObject
 {
 public:
     Block(unsigned int id) : s_Id(id) { setTypeVoxel(CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo() // Новый объект - его удалит движок.
     {
         BlockInfo* info = new BlockInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         return info;
     }
 private:
     unsigned int s_Id;
 };

 // Создание атласа в коде: 16 клеток разных цветов, у каждой - тёмная рамка в 1 пиксель.
 ITexture* createAtlas(IVideoDriver* driver)
 {
     const int size = ATLAS_TILES * TILE_SIZE;
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(size, size));
     for(int tile = 0; tile < ATLAS_TILES * ATLAS_TILES; tile++)
     {
         const SColor color(255, 60 + (tile % 4) * 60, 60 + (tile / 4) * 60, 220 - tile * 12); // Свой цвет у каждой клетки.
         const int x0 = (tile % ATLAS_TILES) * TILE_SIZE, y0 = (tile / ATLAS_TILES) * TILE_SIZE; // Левый верхний угол клетки.
         for(int x = 0; x < TILE_SIZE; x++)
             for(int y = 0; y < TILE_SIZE; y++)
             {
                 const bool border = (x == 0 || y == 0 || x == TILE_SIZE - 1 || y == TILE_SIZE - 1);
                 image->setPixel(x0 + x, y0 + y, border ? SColor(255, color.getRed() / 2, color.getGreen() / 2, color.getBlue() / 2) : color);
             }
     }
     const bool mipmaps = driver->getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
     driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false); // Атласу mip-уровни не нужны (см. текст урока).
     ITexture* atlas = driver->addTexture("atlas", image);
     driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, mipmaps); // Вернули флаг для остальных текстур.
     image->drop();
     return atlas;
 }

 // Фабрика: грань на каждую клетку атласа готовится один раз, в конструкторе.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         ITexture* atlas = createAtlas(device->getVideoDriver()); // Текстуры - только в главном потоке.
         for(int tile = 0; tile < ATLAS_TILES * ATLAS_TILES; tile++)
         {
             s_Faces[tile] = createFaceMesh(0); // Отступ 0: координаты ровно от 0 до 1, отступ сделаем сами.
             IMeshBuffer* mb = s_Faces[tile]->getMeshBuffer(0);
             const float col = tile % ATLAS_TILES, row = tile / ATLAS_TILES; // Где клетка в атласе.
             const float inset = 0.5f / TILE_SIZE; // Отступ в полпикселя клетки - от проступания соседних клеток.
             for(u32 i = 0; i < mb->getVertexCount(); i++)
             {
                 vector2df& tc = mb->getTCoords(i); // 0 или 1 - угол грани.
                 tc.X = (col + inset + tc.X * (1 - 2 * inset)) / ATLAS_TILES;
                 tc.Y = (row + inset + tc.Y * (1 - 2 * inset)) / ATLAS_TILES;
             }
             mb->getMaterial().setTexture(0, atlas); // У всех граней один и тот же материал - одна текстура.
         }
     }
     ~MyFactory() { for(int tile = 0; tile < ATLAS_TILES * ATLAS_TILES; tile++) s_Faces[tile]->drop(); }
 protected:
     // Поток построения: грани только читаются, общие меши не меняются.
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         const unsigned int id = ((BlockInfo*)vxl->getVoxelInfo())->s_Id;
         if(id == BLOCK_GRASS) // Порядок граней: x+, z+, x-, z-, y+, y-.
         {
             IMesh* faces[6] = { s_Faces[13], s_Faces[13], s_Faces[13], s_Faces[13], s_Faces[12], s_Faces[14] }; // Бока, верх, низ.
             return getMeshCubeFaces(vxl, faces, vector3df(1, 1, 1));
         }
         return getMeshCubeFaces(vxl, s_Faces[id % 12], vector3df(1, 1, 1)); // Простые блоки - клетки 0..11.
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Faces[ATLAS_TILES * ATLAS_TILES];
 };

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device);
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     factory->setMaterialFlag(0, id, EMF_BILINEAR_FILTER, false); // Чёткие пиксели и никакого смешивания с соседними клетками.
     factory->setMaterialFlag(0, id, EMF_TRILINEAR_FILTER, false);
     factory->setMaterialFlag(0, id, EMF_ANISOTROPIC_FILTER, false);
     factory->setMaterialFlag(0, id, EMF_USE_MIP_MAPS, false); // Даже если у текстуры есть mip-уровни - не использовать.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++) // Пол из 12 видов блоков, квадратами 4x4.
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new Block(x / 4 + (z / 4) * 4));
     for(int x = 6; x < 10; x++)
         for(int z = 6; z < 10; z++) // Холм из травы посередине.
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 1, z), new Block(BLOCK_GRASS));
     isve->addTaskUpdateChunks();
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(8, 10, -10));
     camera->setTarget(vector3df(8, 0, 8));
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop();
     delete isve;
     device->drop();
     return 0;
 }
 \endcode
*/
