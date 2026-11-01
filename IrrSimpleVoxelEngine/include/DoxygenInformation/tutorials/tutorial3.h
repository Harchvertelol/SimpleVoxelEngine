/*! \page tutorial3 Урок 3: Своя фабрика мешей.
 В первых двух уроках наши блоки были просто цветными кубиками. Пора сделать их красивыми: с текстурами, с разными
 текстурами на разных гранях (трава: сверху зелень, сбоку земля с полоской травы, снизу земля) и с подкрашиванием
 цветом вершин. Для этого разберём подробно главную функцию фабрики - getMeshVoxel(), - а заодно узнаем, как живёт
 фабрика: что с ней происходит после передачи врапперу и как убирать чанки из отрисовки.

 Файлы картинок нам опять не понадобятся: текстуры мы нарисуем прямо в коде.

 \section irrtut3_voxel Воксель с цветом
 Блок - как в уроке 2, только теперь кроме типа он хранит цвет, которым подкрашиваются его вершины:
 \code
 // Типы наших блоков.
 enum { ID_GRASS = 1, ID_STONE = 2, ID_BRICK = 3 };

 // Информация для отрисовки: тип блока и цвет, которым подкрасить его вершины.
 struct MyVoxelInfo : VoxelInfo
 {
     int s_Id;
     SColor s_Color;
 };

 // Свой блок (как в уроке 2) - теперь ещё и с цветом.
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(int id, SColor color = SColor(255, 255, 255, 255)) : s_Id(id), s_Color(color) { setTypeVoxel(Defines::CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo()
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         info->s_Color = s_Color;
         return info;
     }
 private:
     int s_Id;
     SColor s_Color;
 };
 \endcode
 Цвет по умолчанию - белый: белые вершины оставляют текстуру как есть.

 \section irrtut3_textures Текстуры из кода
 \code
 // Текстура 16x16, нарисованная в коде: шахматка из цветов a и b с клеткой cell, верхние toprows строк - цветом top.
 // Вызывать только в главном потоке (создание текстуры - работа драйвера).
 ITexture* makeTexture(IVideoDriver* driver, const char* name, SColor a, SColor b, u32 cell, SColor top, u32 toprows)
 {
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16));
     for(u32 y = 0; y < 16; y++)
         for(u32 x = 0; x < 16; x++)
         {
             SColor c = ((x / cell + y / cell) % 2) ? a : b;
             if(y < toprows) c = top;
             image->setPixel(x, y, c);
         }
     ITexture* texture = driver->addTexture(name, image); // Текстурой владеет драйвер.
     image->drop(); // Картинка больше не нужна.
     return texture;
 }
 \endcode
 Обычный Irrlicht: создаём картинку, рисуем в ней пиксели, делаем из неё текстуру с именем и отпускаем картинку.
 Текстурой владеет драйвер, по имени её можно потом получить через driver->getTexture(). В своей игре вы, скорее
 всего, будете грузить текстуры из файлов (driver->getTexture("media/stone.png")) - правила те же: только в главном
 потоке, то есть в конструкторе фабрики, а не в getMeshVoxel().

 \section irrtut3_factory Конструктор фабрики
 \code
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         // Конструктор - в главном потоке: здесь создаём всё, что требует Irrlicht.
         IVideoDriver* driver = device->getVideoDriver();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
         SColor green(255, 70, 170, 60), darkgreen(255, 50, 140, 40), brown(255, 130, 90, 50), darkbrown(255, 105, 70, 40);
         s_StoneTexture = makeTexture(driver, "stone", SColor(255, 150, 150, 150), SColor(255, 120, 120, 120), 4, SColor(0, 0, 0, 0), 0);
         s_BrickTexture = makeTexture(driver, "brick", SColor(255, 180, 70, 50), SColor(255, 200, 190, 170), 8, SColor(0, 0, 0, 0), 0);
         ITexture* grasstop = makeTexture(driver, "grass_top", green, darkgreen, 2, green, 0);
         ITexture* grassside = makeTexture(driver, "grass_side", brown, darkbrown, 2, green, 4); // Сверху - полоса травы.
         ITexture* dirt = makeTexture(driver, "dirt", brown, darkbrown, 2, brown, 0);
         s_Face = createFaceMesh(); // Общая грань для блоков с одной текстурой на всех гранях.
         for(int i = 0; i < 6; i++) // Шесть граней травы: порядок x+, z+, x-, z-, y+, y-.
         {
             s_GrassFaces[i] = createFaceMesh();
             ITexture* texture = grassside; // Боковые грани.
             if(i == 4) texture = grasstop; // Верх (y+).
             if(i == 5) texture = dirt; // Низ (y-).
             s_GrassFaces[i]->getMeshBuffer(0)->getMaterial().setTexture(0, texture); // Эти текстуры не меняются - ставим один раз.
         }
     }
     ~MyFactory()
     {
         s_Face->drop();
         for(int i = 0; i < 6; i++) s_GrassFaces[i]->drop();
     }
 \endcode
 Создаём пять текстур: камень, кирпич и три текстуры травы. Дальше - грани. У нас их два вида.

 s_Face - одна общая грань для блоков, у которых все шесть граней одинаковые (камень, кирпич). Текстуру на неё мы
 будем ставить прямо перед сборкой каждого куба.

 s_GrassFaces - шесть отдельных граней для травы, по одной на каждую сторону куба, и у каждой своя текстура. Порядок
 граней тот же, что и в массиве видимости граней VoxelForRendering: x+, z+, x-, z-, y+, y- (грани, смотрящие в
 направлениях (1,0,0), (0,0,1), (-1,0,0), (0,0,-1), (0,1,0) и (0,-1,0)). Эти текстуры не меняются, поэтому ставим их
 один раз, здесь.

 Как ляжет картинка? На боковых гранях куба, собранного getMeshCubeFaces(), верхняя строка картинки оказывается
 сверху - поэтому полоса травы, которую мы нарисовали в верхних строках grass_side, будет у верхнего края блока. На
 верхней грани координата v картинки растёт в сторону +Z.

 Пара слов про createFaceMesh(). У неё есть параметр texinset - отступ текстурных координат от краёв текстуры (по
 умолчанию 0.002): координаты грани идут не от 0 до 1, а от texinset до 1 - texinset. Зачем? При сглаживании текстуры
 (билинейная фильтрация) на самом краю грани видеокарта подмешивает пиксели с противоположного края текстуры, и по
 краям блоков появляются тонкие линии чужого цвета. Маленький отступ это убирает. createFaceMesh(0) даёт координаты
 ровно от 0 до 1. Особенно важен отступ для текстурного атласа - урок 5.

 \section irrtut3_getmesh getMeshVoxel() подробно
 \code
 protected:
     // Потоки построения, строго по одному вызову за раз.
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         MyVoxelInfo* info = (MyVoxelInfo*)vxl->getVoxelInfo();
         IMesh* mesh = 0;
         if(info->s_Id == ID_GRASS) mesh = getMeshCubeFaces(vxl, s_GrassFaces, vector3df(1, 1, 1)); // Шесть разных граней.
         else
         {
             // Общая грань: ставим нужную текстуру и собираем куб. Куб - копия, смена текстуры для следующего блока его не тронет.
             s_Face->getMeshBuffer(0)->getMaterial().setTexture(0, info->s_Id == ID_BRICK ? s_BrickTexture : s_StoneTexture);
             mesh = getMeshCubeFaces(vxl, s_Face, vector3df(1, 1, 1));
         }
         if(mesh) s_MeshManipulator->setVertexColors(mesh, info->s_Color); // Подкрашиваем: цвет вершин умножается на текстуру.
         return mesh; // Новый меш - враппер сам сделает ему drop().
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Face;
     IMesh* s_GrassFaces[6];
     ITexture* s_StoneTexture;
     ITexture* s_BrickTexture;
     IMeshManipulator* s_MeshManipulator;
 };
 \endcode
 У getMeshCubeFaces() два варианта. С одной гранью (как в уроках 1 и 2) все шесть сторон куба получают одинаковый
 меш, а значит, одну текстуру и все остальные параметры материала. С массивом из шести граней (трава) каждая сторона
 получает свою грань - в порядке x+, z+, x-, z-, y+, y-. В обоих случаях в куб попадают только видимые грани. Грани
 должны быть квадратами 1x1 с центром в начале координат в плоскости Oxz лицом вверх - ровно такие даёт
 createFaceMesh(), поэтому модель грани из файла не нужна.

 Последний параметр, planescale, - множитель размера куба. (1, 1, 1) - куб ровно в размер вокселя. Например, с
 (0.8, 0.8, 0.8) получится кубик чуть меньше ячейки. Задавайте его одинаковым по всем осям. Только помните, что
 видимость граней ядро считает для целой ячейки: грань соседа, прилегающая к уменьшенному кубику, считается закрытой
 и не рисуется - в щели будет видна пустота. Как с этим справиться, рассказано в уроке 4 (тестер прозрачности).

 Теперь главный вопрос: почему можно менять текстуру общей грани s_Face прямо в getMeshVoxel(), если потоков
 построения несколько? Здесь работают две вещи:
 - getMeshVoxel() и getMeshNumber() вызываются строго по одному: враппер вызывает их под общей блокировкой фабрики
   (s_LockGetMeshVoxel). Пока один поток построения собирает куб, другой не может в это время читать или менять s_Face.
 - getMeshCubeFaces() не ссылается на грань, а копирует её вершины и материал (вместе с указателем на текстуру) в
   новый меш. Поэтому, когда для следующего блока мы поставим на s_Face другую текстуру, уже собранный куб это не
   затронет.

 Отсюда и ограничение: менять общие меши фабрики можно только внутри getMeshVoxel() и getMeshNumber(). Если менять
 их, например, из главного потока по нажатию клавиши, пока движок работает, - это будет уже без блокировки, прямо во
 время сборки куба в другом потоке. Если вид блоков должен меняться, меняйте данные блоков (их VoxelInfo) и
 перестраивайте чанки (урок 8).

 Возвращаемый меш должен быть новым: враппер вклеит его в меш чанка и один раз сделает ему drop(). getMeshCubeFaces()
 и getMeshVoxelByGlue() возвращают новые меши. Если же хочется вернуть свой заранее готовый меш, сначала возьмите
 на него ссылку:
 \code
 s_MyModel->grab(); // drop() враппера отпустит эту ссылку, а наша останется (не в коде урока).
 return s_MyModel;
 \endcode
 Вернуть NULL - значит сообщить об ошибке: меш чанка не построится, а в логе появится ошибка.

 \section irrtut3_colors Цвет вершин и материалы
 setVertexColors() красит все вершины меша одним цветом. С материалом EMT_SOLID без освещения цвет вершины
 умножается на цвет текстуры: белый оставляет текстуру как есть, красноватый делает блок красноватым. У нас так
 подкрашены два камня. Цвет вершин - это ещё и самый простой способ сделать освещение: светлые и тёмные блоки,
 день и ночь (урок 15). Если цвет нужен разный на разных гранях, пройдите по мешбуферам меша и задайте цвет
 вершинам каждой грани отдельно.

 А что с производительностью? Враппер склеивает меши вокселей так, что вершины с одинаковым материалом попадают в
 один мешбуфер. Каждая текстура - это свой материал, а значит, свой мешбуфер в каждой ноде и свой вызов отрисовки.
 В нашем чанке пять текстур - пять мешбуферов. Для пяти текстур это пустяк, а вот для сотни разных блоков будет
 заметно. Решение - текстурный атлас (одна текстура на все блоки), о нём - урок 5. Цвет вершин материал не меняет,
 поэтому подкрашенные камни лежат в том же мешбуфере, что и обычные.

 \section irrtut3_main Мир
 \code
 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(1280, 720));
     Logger* lgr = new Logger;
     lgr->setPrintStandard(false);
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop();
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     factory->setMaterialFlag(0, id, EMF_BILINEAR_FILTER, false); // Чёткие пиксели маленьких текстур.
 \endcode
 Всё знакомо по прошлым урокам. Новое - выключенная билинейная фильтрация: наши текстуры всего 16x16 пикселей, и со
 сглаживанием они расплылись бы. Без неё получаются чёткие "пиксельные" блоки.

 Дальше ставим один чанк и блоки: пол из травы, каменный столб, кирпичную стенку и два подкрашенных камня:
 \code
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new MyBlock(ID_GRASS)); // Пол из травы.
     for(int y = 1; y < 4; y++)
         isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(3, y, 3), new MyBlock(ID_STONE)); // Каменный столб.
     for(int x = 6; x < 10; x++)
         for(int y = 1; y < 3; y++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, 8), new MyBlock(ID_BRICK)); // Кирпичная стенка.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(12, 1, 3), new MyBlock(ID_STONE, SColor(255, 255, 90, 90))); // Красноватый камень.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(12, 1, 6), new MyBlock(ID_STONE, SColor(255, 90, 120, 255))); // Синеватый камень.
     isve->addTaskUpdateChunks();
 \endcode
 Камера, запуск, главный цикл и остановка - как в уроке 2 (см. полный код ниже).

 \section irrtut3_settings Ещё настройки фабрики
 Есть ещё несколько настроек, которые задаются, как и масштабы, только до setFactoryChunkMeshSceneNode():
 - setIsFaces(false) - не передавать фабрике информацию о видимых гранях. Тогда у каждого VoxelForRendering
   isRenderFace() возвращает true для всех шести граней, и getMeshCubeFaces() строит все грани куба, даже закрытые
   соседями. Невидимые воксели (закрытые со всех сторон) враппер всё равно не передаёт. Нужно это редко - разве что
   ваша фабрика рисует свои модели и граней не учитывает. По умолчанию true; узнать значение - isFaces().
 - setRotationScale() - масштаб поворота вокселей, как setPositionScale() для позиции. Сейчас ядро отдаёт всем
   вокселям нулевой поворот, поэтому на результат этот масштаб не влияет. Если вашим блокам нужен поворот (лестница,
   смотрящая в одну из четырёх сторон), храните направление в самом блоке, передавайте его в VoxelInfo и
   поворачивайте меш в getMeshVoxel() сами (урок 4).
 - getPositionScale(), getSScale(), getRotationScale() возвращают текущие масштабы.

 \section irrtut3_life Жизнь фабрики
 Что происходит с фабрикой после isve->setFactoryChunkMeshSceneNode(factory)?
 - Враппер становится её владельцем и удалит её в delete isve - сами её не удаляйте.
 - Фабрика замораживается (freeze()): узнать это можно через isFreeze(). Число уровней, масштабы, setIsFaces() и общий
   размер блока чанков больше не меняются - вызовы вернут false или ничего не сделают, а в логе будет нарушение
   логики. Замороженную фабрику не примет другой движок. Вызывать freeze() самим обычно незачем: это делает
   setFactoryChunkMeshSceneNode().
 - getIrrSimpleVoxelEngine() и getIrrlichtDevice() возвращают движок и устройство фабрики.

 Фабрику можно создать и без движка: у FactoryChunkMeshSceneNode есть конструкторы только с устройством и вообще без
 параметров. Недостающее подставит setFactoryChunkMeshSceneNode() (движок - этот, устройство - устройство движка),
 либо можно задать самим: setIrrSimpleVoxelEngine() и setIrrlichtDevice() - каждое по одному разу и только до
 заморозки. Фабрику, уже привязанную к другому движку или устройству, движок не примет. Например:
 \code
 class MyFactory2 : public FactoryChunkMeshSceneNode // (не в коде урока)
 {
 public:
     MyFactory2(IrrlichtDevice* device) : FactoryChunkMeshSceneNode(device) {} // Движок подставит setFactoryChunkMeshSceneNode().
     ...
 };
 \endcode
 Одна тонкость: пока фабрика не знает свой движок, ей некуда писать сообщения, и её ошибки (например, повторный
 setIrrlichtDevice()) молча пропадают.

 \section irrtut3_chunks Чанки фабрики
 Фабрика знает, какие чанки сейчас рисуются, и позволяет убирать их из отрисовки:
 - isRenderChunk(id, вектор чанка) - зарегистрирован ли чанк для отрисовки. Чанк регистрируется ещё до построения
   меша, поэтому true не значит, что нода уже на экране.
 - deleteChunk(id, вектор чанка) - убрать чанк из отрисовки: его нода исчезнет в ближайшем update(), незаконченные
   построения его меша отменяются. Данные мира не трогаются - чанк остаётся в памяти, и его можно снова показать
   через addChunksForRendering() (уже без загрузки).
 - deleteAllChunks(id) - убрать из отрисовки все чанки мира, deleteAllChunks() - все чанки всех миров. Можно вызывать
   из любого потока: в главном ноды удаляются сразу, в других - чанки убираются из отрисовки, а их ноды удалит
   ближайший update().

 Обычно удобнее функции движка deleteChunkForRendering() и deleteChunksForRendering(): они ещё и умеют выгружать
 чанки из памяти с сохранением (урок 6).

 И последнее - clear(). Она полностью очищает фабрику: убирает все чанки, удаляет миры фабрики и её уровни. Её
 вызывает деструктор фабрики, то есть delete isve. Самим вызывать её нужно, только если фабрика больше не нужна, а
 движок вы удалять не собираетесь, - в главном потоке и после остановки движка. Пользоваться такой фабрикой дальше
 нельзя. А функцию update() фабрики вызывает IrrSimpleVoxelEngine::update(), отдельно её вызывать не нужно.

 \section irrtut3_errors Типичные ошибки
 - Текстуры грузятся или создаются в getMeshVoxel() - это поток построения, а не главный.
 - Общие грани фабрики меняются вне getMeshVoxel() (например, из обработчика клавиш) во время работы движка.
 - Из getMeshVoxel() возвращается свой меш без grab() - после первого же чанка он удалён.
 - Перепутан порядок граней в массиве из шести: правильно x+, z+, x-, z-, y+, y-.
 - На краях блоков тонкие линии чужого цвета: грань создана с texinset = 0, а фильтрация включена.
 - Каждый блок со своей текстурой - сотни мешбуферов и тормоза: пора делать атлас (урок 5).
 - Фабрика удалена вручную после передачи врапперу - двойное удаление.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Типы наших блоков.
 enum { ID_GRASS = 1, ID_STONE = 2, ID_BRICK = 3 };

 // Информация для отрисовки: тип блока и цвет, которым подкрасить его вершины.
 struct MyVoxelInfo : VoxelInfo
 {
     int s_Id;
     SColor s_Color;
 };

 // Свой блок (как в уроке 2) - теперь ещё и с цветом.
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(int id, SColor color = SColor(255, 255, 255, 255)) : s_Id(id), s_Color(color) { setTypeVoxel(Defines::CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo()
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         info->s_Color = s_Color;
         return info;
     }
 private:
     int s_Id;
     SColor s_Color;
 };

 // Текстура 16x16, нарисованная в коде: шахматка из цветов a и b с клеткой cell, верхние toprows строк - цветом top.
 // Вызывать только в главном потоке (создание текстуры - работа драйвера).
 ITexture* makeTexture(IVideoDriver* driver, const char* name, SColor a, SColor b, u32 cell, SColor top, u32 toprows)
 {
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16));
     for(u32 y = 0; y < 16; y++)
         for(u32 x = 0; x < 16; x++)
         {
             SColor c = ((x / cell + y / cell) % 2) ? a : b;
             if(y < toprows) c = top;
             image->setPixel(x, y, c);
         }
     ITexture* texture = driver->addTexture(name, image); // Текстурой владеет драйвер.
     image->drop(); // Картинка больше не нужна.
     return texture;
 }

 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         // Конструктор - в главном потоке: здесь создаём всё, что требует Irrlicht.
         IVideoDriver* driver = device->getVideoDriver();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
         SColor green(255, 70, 170, 60), darkgreen(255, 50, 140, 40), brown(255, 130, 90, 50), darkbrown(255, 105, 70, 40);
         s_StoneTexture = makeTexture(driver, "stone", SColor(255, 150, 150, 150), SColor(255, 120, 120, 120), 4, SColor(0, 0, 0, 0), 0);
         s_BrickTexture = makeTexture(driver, "brick", SColor(255, 180, 70, 50), SColor(255, 200, 190, 170), 8, SColor(0, 0, 0, 0), 0);
         ITexture* grasstop = makeTexture(driver, "grass_top", green, darkgreen, 2, green, 0);
         ITexture* grassside = makeTexture(driver, "grass_side", brown, darkbrown, 2, green, 4); // Сверху - полоса травы.
         ITexture* dirt = makeTexture(driver, "dirt", brown, darkbrown, 2, brown, 0);
         s_Face = createFaceMesh(); // Общая грань для блоков с одной текстурой на всех гранях.
         for(int i = 0; i < 6; i++) // Шесть граней травы: порядок x+, z+, x-, z-, y+, y-.
         {
             s_GrassFaces[i] = createFaceMesh();
             ITexture* texture = grassside; // Боковые грани.
             if(i == 4) texture = grasstop; // Верх (y+).
             if(i == 5) texture = dirt; // Низ (y-).
             s_GrassFaces[i]->getMeshBuffer(0)->getMaterial().setTexture(0, texture); // Эти текстуры не меняются - ставим один раз.
         }
     }
     ~MyFactory()
     {
         s_Face->drop();
         for(int i = 0; i < 6; i++) s_GrassFaces[i]->drop();
     }
 protected:
     // Потоки построения, строго по одному вызову за раз.
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         MyVoxelInfo* info = (MyVoxelInfo*)vxl->getVoxelInfo();
         IMesh* mesh = 0;
         if(info->s_Id == ID_GRASS) mesh = getMeshCubeFaces(vxl, s_GrassFaces, vector3df(1, 1, 1)); // Шесть разных граней.
         else
         {
             // Общая грань: ставим нужную текстуру и собираем куб. Куб - копия, смена текстуры для следующего блока его не тронет.
             s_Face->getMeshBuffer(0)->getMaterial().setTexture(0, info->s_Id == ID_BRICK ? s_BrickTexture : s_StoneTexture);
             mesh = getMeshCubeFaces(vxl, s_Face, vector3df(1, 1, 1));
         }
         if(mesh) s_MeshManipulator->setVertexColors(mesh, info->s_Color); // Подкрашиваем: цвет вершин умножается на текстуру.
         return mesh; // Новый меш - враппер сам сделает ему drop().
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Face;
     IMesh* s_GrassFaces[6];
     ITexture* s_StoneTexture;
     ITexture* s_BrickTexture;
     IMeshManipulator* s_MeshManipulator;
 };


 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(1280, 720));
     Logger* lgr = new Logger;
     lgr->setPrintStandard(false);
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop();
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     factory->setMaterialFlag(0, id, EMF_BILINEAR_FILTER, false); // Чёткие пиксели маленьких текстур.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new MyBlock(ID_GRASS)); // Пол из травы.
     for(int y = 1; y < 4; y++)
         isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(3, y, 3), new MyBlock(ID_STONE)); // Каменный столб.
     for(int x = 6; x < 10; x++)
         for(int y = 1; y < 3; y++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, 8), new MyBlock(ID_BRICK)); // Кирпичная стенка.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(12, 1, 3), new MyBlock(ID_STONE, SColor(255, 255, 90, 90))); // Красноватый камень.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(12, 1, 6), new MyBlock(ID_STONE, SColor(255, 90, 120, 255))); // Синеватый камень.
     isve->addTaskUpdateChunks();
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(8, 8, -8));
     camera->setTarget(vector3df(8, 0, 8));
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 120, 160, 220));
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
