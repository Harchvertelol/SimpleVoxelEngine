/*! \page tutorial9 Урок 9: Уровни мешей и прозрачность.
 До сих пор все наши воксели рисовались одним материалом. Но в настоящем мире есть стекло, через которое видно, что
 внутри, и вода, сквозь которую видно дно. Для этого во враппере есть уровни мешей: каждый уровень - это свой меш
 чанка со своим материалом, флагами и ID. В этом уроке мы сделаем три уровня (камень, стекло и вода), научим враппер
 рисовать грани за прозрачными блоками, создадим текстуры прямо в коде и подключим простой шейдер. Никаких файлов
 урок не требует.

 <b>Как устроены уровни.</b> Число уровней задаёт setNumberOfMeshesSceneNodeForChunk(). Для каждого вокселя фабрика
 в потоке построения вызывает сначала getMeshVoxel() (какой меш нарисовать), а сразу после - getMeshNumber() (на какой
 уровень этот меш положить: от 0 до числа уровней - 1; другой номер - ошибка, меш чанка не строится). Меши вокселей
 одного уровня склеиваются вместе, и для каждого блока чанков (setSizeBlockChunks(), урок 2) на каждый непустой уровень
 враппер создаёт отдельную ноду Irrlicht со своим материалом. Пустой уровень ноды не имеет.

 Зачем разные ноды? Irrlicht рисует сначала все непрозрачные ноды, а потом прозрачные - от дальних к ближним, и решает
 это по материалу ноды. Если стекло и камень окажутся в одной ноде, то либо камень будет "прозрачным", либо стекло
 нарисуется не в свою очередь. Поэтому правило простое: один вид материала - один уровень.

 Начнём с перечислений - уровни, типы блоков и ID нод:
 \code
 enum MESH_LEVELS
 {
     LEVEL_SOLID = 0, // Камень - непрозрачный.
     LEVEL_GLASS = 1, // Стекло - "вырезанная" прозрачность (альфа-тест).
     LEVEL_WATER = 2, // Вода - полупрозрачная со смешиванием.
     NUMBER_OF_LEVELS = 3
 };
 enum BLOCK_TYPES
 {
     BLOCK_STONE = 0,
     BLOCK_GLASS = 1,
     BLOCK_WATER = 2
 };
 enum NODE_IDS
 {
     ID_PICKABLE = 1,
     ID_NOT_PICKABLE = 2
 };
 \endcode
 Блок - как в уроке 2: он хранит только свой тип и отдаёт его в информации для отрисовки:
 \code
 struct MyVoxelInfo : VoxelInfo
 {
     unsigned int s_Type;
 };
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(unsigned int type) : SimpleVoxelObject(), s_Type(type) { setTypeVoxel(CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo() // Новый объект - его удалит движок.
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Type = s_Type;
         return info;
     }
     unsigned int getType() { return s_Type; }
 private:
     unsigned int s_Type;
 };
 \endcode

 <b>Грани за стеклом и водой.</b> Ядро рисует только те грани, которые ничем не закрыты: грань между двумя блоками
 считается невидимой. Для камня это правильно, а вот сквозь стекло мы должны видеть грани блоков за ним, а сквозь воду -
 дно. Как сказать ядру, какие блоки "прозрачны"? Через SimpleVoxelObjectTransparentTester (подробно - в уроке 4
 SimpleVoxelEngine). Ядро спрашивает его: "видна ли грань блока parent, если за ней стоит сосед svo?". Сделаем стекло и
 воду прозрачными для всех, кроме таких же блоков:
 \code
 class MyTransparentTester : public SimpleVoxelObjectTransparentTester
 {
 public:
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent)
     {
         if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false;
         unsigned int type = ((MyBlock*)svo)->getType();
         if(type != BLOCK_GLASS && type != BLOCK_WATER) return false; // Камень непрозрачен.
         if(!parent || parent->getTypeVoxel() != CUSTOM_VOXEL) return true; // parent может быть NULL - проверяем.
         return ((MyBlock*)parent)->getType() != type;
     }
 };
 \endcode
 Что получается: грань камня под водой рисуется (сосед - вода, она прозрачна для камня), грань воды у камня - нет
 (камень непрозрачен), грань воды за стеклом рисуется, а грани между двумя блоками воды - нет. Последнее очень важно:
 если такие грани рисовать, внутри толщи воды будет видна "сетка" из полупрозрачных граней, и каждая лишняя грань
 добавит слой смешивания. Тестер ставится миру сразу после addWorld(), до загрузки чанков (мир удалит его сам):
 \code
 isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MyTransparentTester); // Мир удалит его сам.
 \endcode

 <b>Фабрика.</b> Текстура грани и уровень зависят от типа блока. Вызовы getMeshVoxel() и getMeshNumber() во всех потоках
 построения идут строго по одному (урок 3), поэтому общую грань s_Plane можно перед getMeshCubeFaces() перекрашивать в
 нужную текстуру: getMeshCubeFaces() делает новый меш-копию, и смена текстуры в нём запоминается.
 \code
 IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения.
 {
     unsigned int type = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Type;
     s_Plane->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[type]); // Вызовы идут по одному - общую грань менять можно.
     return getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1)); // Новый меш - копия грани вместе с текстурой.
 }
 int getMeshNumber(VoxelForRendering* vxl) // На какой уровень попадёт воксель (вызывается сразу после getMeshVoxel()).
 {
     unsigned int type = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Type;
     if(type == BLOCK_GLASS) return LEVEL_GLASS;
     if(type == BLOCK_WATER) return LEVEL_WATER;
     return LEVEL_SOLID;
 }
 \endcode
 Тип материала, записанный в мешбуфере вокселя, роли не играет: материал ноды задаёт уровень (setMaterialType()).
 А вот текстура и цвет вершин берутся из меша.

 <b>Текстуры в коде.</b> Текстуры загружаются только в главном потоке (в потоках построения драйвер трогать нельзя),
 поэтому создадим их до фабрики и передадим ей готовые указатели. Картинку рисуем по пикселям: у камня - серый узор, у
 стекла - непрозрачная рамка и блик, а в середине "дыра" с альфой 0, у воды - синий цвет с альфой 140 из 255:
 \code
 ITexture* makeTexture(IVideoDriver* driver, const char* name, unsigned int type)
 {
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16));
     for(u32 x = 0; x < 16; x++)
         for(u32 y = 0; y < 16; y++)
         {
             SColor color(140, 40, 90, 200); // Вода.
             if(type == BLOCK_STONE) color = SColor(255, 100 + (x * 7 + y * 13) % 40, 100 + (x * 7 + y * 13) % 40, 110);
             if(type == BLOCK_GLASS)
             {
                 bool frame = (x == 0 || y == 0 || x == 15 || y == 15 || (x == y && x > 3 && x < 12));
                 color = frame ? SColor(255, 200, 230, 255) : SColor(0, 255, 255, 255);
             }
             image->setPixel(x, y, color);
         }
     ITexture* texture = driver->addTexture(name, image); // Текстура - копия картинки.
     image->drop();
     return texture;
 }
 \endcode

 <b>Два вида прозрачности.</b> Это главное, что нужно понять в уроке (кратко это же есть в IrrSVEReadme.txt, раздел 7):
 - "вырезанная" прозрачность (стекло-решётка, листва, забор): пиксель либо виден полностью, либо его нет совсем.
   Материал EMT_TRANSPARENT_ALPHA_CHANNEL_REF: пиксели с альфой меньше половины отбрасываются (альфа-тест), остальные
   рисуются как непрозрачные и пишут глубину в Z-буфер. Порядок отрисовки не важен, сортировка не нужна. Наше стекло
   именно такое: рамка видна, середина - дыра;
 - полупрозрачность со смешиванием (вода, цветное стекло): EMT_TRANSPARENT_ALPHA_CHANNEL, цвет пикселя смешивается с
   тем, что уже нарисовано за ним. Здесь порядок важен: дальние грани нужно рисовать раньше ближних, а писать глубину
   в Z-буфер нельзя - иначе ближняя грань, нарисованная первой, закроет дальние.

 Для полупрозрачного уровня враппер всё это делает сам: отключает запись в Z-буфер (EMF_ZWRITE_ENABLE = false),
 сортирует треугольники каждого блока от дальних к ближним и ставит ноду блока в его центр, чтобы Irrlicht правильно
 упорядочил сами ноды между собой. Полупрозрачным враппер считает уровень, у рендерера материала которого
 IMaterialRenderer::isTransparent() == true, - поэтому это работает и для шейдеров на основе
 EMT_TRANSPARENT_ALPHA_CHANNEL (Irrlicht сам запись в Z-буфер для шейдерных материалов не отключает - это делает
 враппер). Сортировка выполняется в isve->update(), только если камера сдвинулась больше чем на
 IrrSVEDefines::TRANSPARENT_SORT_CAMERA_THRESHOLD (0.25 единицы) или блок перестроен, так что каждый кадр это почти
 ничего не стоит.

 Ограничения сортировки: она точная внутри одного мешбуфера (одна текстура - один мешбуфер), а между разными
 текстурами одного блока и между разными блоками порядок приблизительный (по центрам). Поэтому для полупрозрачных
 блоков лучше одна текстура или атлас (урок 5), а чем меньше блок чанков (setSizeBlockChunks()), тем точнее порядок
 между блоками.

 Сортирует враппер относительно активной камеры сцены. Если мир рисуется другой камерой (например, в текстуру или во
 втором окне) или сортировать нужно относительно другой точки, задайте её сами - в главном потоке, каждый кадр перед
 isve->update(), в координатах сцены:
 \code
 factory->updateCameraPosition(id, camera2->getAbsolutePosition()); // Своя камера для сортировки мира id.
 \endcode
 Стоит один раз вызвать updateCameraPosition() для мира - и активная камера для него больше не используется, поэтому
 обновляйте позицию каждый кадр, пока камера движется.

 <b>Шейдеры.</b> Шейдер - это тот же уровень со своим материалом, только материал создан через
 addHighLevelShaderMaterial(). Последний параметр - базовый материал: он решает, как результат шейдера смешивается с
 картинкой и считает ли враппер уровень полупрозрачным. Сделаем один шейдер (GLSL) на три базовых материала. Он
 умножает цвет текстуры на цвет вершины и на яркость, а альфу берёт из текстуры - её дальше обработает базовый
 материал (альфа-тест у стекла, смешивание у воды):
 \code
 const char* VERTEX_SHADER =
     "uniform float s_Brightness;\n"
     "varying vec3 s_Color;\n"
     "void main(void)\n"
     "{\n"
     "    gl_Position = ftransform();\n"
     "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
     "    s_Color = gl_Color.rgb * s_Brightness;\n"
     "}\n";
 const char* PIXEL_SHADER =
     "uniform sampler2D s_Texture;\n"
     "varying vec3 s_Color;\n"
     "void main(void)\n"
     "{\n"
     "    vec4 color = texture2D(s_Texture, gl_TexCoord[0].xy);\n"
     "    gl_FragColor = vec4(color.rgb * s_Color, color.a);\n"
     "}\n";
 \endcode
 Константы шейдера передаёт обратный вызов: Irrlicht вызывает OnSetConstants() в главном потоке перед отрисовкой
 каждого мешбуфера с этим материалом. Яркость будем менять клавишей N ("день и ночь" - так же устроено солнце в
 TestProject3):
 \code
 class MyShaderCallBack : public IShaderConstantSetCallBack
 {
 public:
     MyShaderCallBack() : s_Brightness(1.f) {}
     void OnSetConstants(IMaterialRendererServices* services, s32)
     {
         s32 layer = 0; // Текстура - из слоя 0.
         services->setPixelShaderConstant("s_Texture", &layer, 1);
         services->setVertexShaderConstant("s_Brightness", &s_Brightness, 1);
     }
     f32 s_Brightness; // Клавиша N - день и ночь.
 };
 \endcode
 Создаём материалы. Если драйвер не умеет GLSL (или шейдер не собрался), остаются встроенные материалы - и всё
 по-прежнему работает, только без яркости:
 \code
 E_MATERIAL_TYPE materials[NUMBER_OF_LEVELS] = { EMT_SOLID, EMT_TRANSPARENT_ALPHA_CHANNEL_REF, EMT_TRANSPARENT_ALPHA_CHANNEL };
 MyShaderCallBack* callback = new MyShaderCallBack;
 IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
 if(gpu && driver->queryFeature(EVDF_ARB_GLSL))
     for(int i = 0; i < NUMBER_OF_LEVELS; i++)
     {
         s32 shader = gpu->addHighLevelShaderMaterial(VERTEX_SHADER, "main", EVST_VS_1_1, PIXEL_SHADER, "main", EPST_PS_1_1, callback, materials[i]);
         if(shader >= 0) materials[i] = (E_MATERIAL_TYPE)shader; // Шейдер не собрался - остаётся встроенный материал.
     }
 \endcode
 Шейдерные материалы держат свою ссылку на callback, поэтому свою ссылку мы отпустим (drop()) в самом конце, после
 удаления враппера. Шейдеры посложнее (освещение от солнца и ламп) - в TestProject3/main.cpp и в уроке 15.

 <b>Настройка уровней.</b> Здесь важен порядок, и ошибки в нём - самые частые:
 \code
 factory->setNumberOfMeshesSceneNodeForChunk(NUMBER_OF_LEVELS); // Первым из настроек уровней: сбрасывает их материалы.
 for(int i = 0; i < NUMBER_OF_LEVELS; i++)
 {
     factory->setMaterialType(i, materials[i]); // По умолчанию для всех миров - до addWorld().
     factory->setHardwareMappingHint(i, EHM_STATIC, EBT_VERTEX_AND_INDEX); // Меши чанков - в видеопамяти.
 }
 isve->setFactoryChunkMeshSceneNode(factory); // После этого число уровней менять нельзя.
 int id = isve->addWorld();
 \endcode
 - setNumberOfMeshesSceneNodeForChunk() - первым: он сбрасывает материалы и hardware mapping всех уровней. И до
   setFactoryChunkMeshSceneNode(): отданной движку (замороженной) фабрике число уровней не меняется - ошибка в логе;
 - setMaterialType(уровень, материал) и setHardwareMappingHint(уровень, ...) без номера мира - значения по умолчанию,
   их берут миры, созданные ПОСЛЕ вызова. Вызванные после addWorld(), они на этот мир уже не подействуют;
 - для уже созданного мира есть версии с номером мира: setMaterialType(уровень, id, материал) (ноды уровня
   пересклеиваются в ближайшем update()) и setHardwareMappingHint(уровень, id, ...). Так у разных миров могут быть разные
   материалы одного уровня;
 - setMaterialFlag(уровень, id, флаг, значение) и setID(уровень, id, ID) есть только для мира - после addWorld().
   Они применяются и к уже созданным нодам.
 EHM_STATIC хранит меши чанков в видеопамяти: они меняются редко, а рисуются каждый кадр. Индексы полупрозрачного
 уровня при сортировке меняются - враппер сам помечает их для обновления в видеопамяти (подробно о hardware mapping - в
 уроке 14). Все эти функции - только из главного потока.

 Флаги и ID для мира:
 \code
 for(int i = 0; i < NUMBER_OF_LEVELS; i++) factory->setMaterialFlag(i, id, EMF_LIGHTING, false); // Флаги - для мира, после addWorld().
 factory->setMaterialFlag(LEVEL_WATER, id, EMF_BACK_FACE_CULLING, false); // Воду видно и изнутри (из-под воды).
 factory->setID(LEVEL_SOLID, id, ID_PICKABLE);
 factory->setID(LEVEL_GLASS, id, ID_PICKABLE);
 factory->setID(LEVEL_WATER, id, ID_NOT_PICKABLE);
 \endcode
 EMF_LIGHTING выключаем: источников света в сцене нет, и без этого встроенные материалы были бы чёрными. ID нод нужен
 лучу выбора блоков (урок 7): ноды воды получают ID, которого луч не ловит, - курсор проходит сквозь воду и выбирает
 дно.

 Про EMF_BACK_FACE_CULLING. Обычно Irrlicht не рисует задние стороны граней - и это правильно для всех непрозрачных
 блоков. Но если нырнуть под воду, поверхность воды и её стенки мы видим изнутри, то есть с задней стороны, и без этого
 флага граница воды из-под воды не видна совсем. Поэтому у воды (как в TestProject3) задние стороны включены. Цена -
 сверху видны и задние стороны дальних стенок воды (например, стенки за стеклом изнутри бассейна); сортировка их тоже
 учитывает, но это лишние слои смешивания. У стекла (альфа-тест) и у непрозрачных уровней флаг не трогайте; если вода в
 вашей игре не бывает "изнутри", тоже оставьте его включённым.

 Что ещё нельзя делать с полупрозрачными уровнями: включать у сцены ALLOW_ZWRITE_ON_TRANSPARENT и задавать
 MaterialTypeParam с pack_textureBlendFunc(EBF_SRC_ALPHA, EBF_DST_ALPHA) - DST_ALPHA при буфере кадра без альфа-канала
 даёт почти сложение цветов.

 <b>Сцена.</b> Один чанк: каменный пол, стеклянный домик (только стенки) с каменной колонной внутри и бассейн с каменным
 бортиком, передняя стенка которого - из стекла. Через неё видна вода, а через воду - дно:
 \code
 for(int x = 9; x <= 14; x++)
     for(int y = 1; y <= 2; y++)
         for(int z = 9; z <= 14; z++)
         {
             bool rim = (x == 9 || x == 14 || z == 9 || z == 14);
             setBlock(isve, id, x, y, z, rim ? (z == 9 ? BLOCK_GLASS : BLOCK_STONE) : BLOCK_WATER);
         }
 \endcode
 В главном цикле ничего особенного нет: сортировку враппер делает внутри isve->update(). Клавиша N переключает яркость
 в шейдере (обработчик событий меняет s_Brightness у callback, OnEvent() и OnSetConstants() вызываются в главном потоке,
 так что синхронизация не нужна).

 <b>Типичные ошибки:</b>
 - setNumberOfMeshesSceneNodeForChunk() после setMaterialType() - материалы сбросились в EMT_SOLID;
 - setMaterialType() без номера мира после addWorld() - мир его не увидел (используйте версию с id);
 - getMeshNumber() вернул номер уровня больше числа уровней - меш чанка не строится, ошибка в логе;
 - нет тестера прозрачности - за стеклом и под водой пустота (грани соседей отсечены);
 - тестер делает прозрачными одинаковые блоки друг для друга - сетка внутри воды;
 - вода в материале EMT_TRANSPARENT_ALPHA_CHANNEL_REF - край "рваный", полупрозрачности нет; стекло-решётка в
   EMT_TRANSPARENT_ALPHA_CHANNEL - лишняя сортировка и неверная глубина;
 - стекло и вода на одном уровне - один из них нарисуется неправильно.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines; // CUSTOM_VOXEL.
 using namespace Debug; // Logger.
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Уровни мешей: у каждого уровня - свой материал (и свои ноды).
 enum MESH_LEVELS
 {
     LEVEL_SOLID = 0, // Камень - непрозрачный.
     LEVEL_GLASS = 1, // Стекло - "вырезанная" прозрачность (альфа-тест).
     LEVEL_WATER = 2, // Вода - полупрозрачная со смешиванием.
     NUMBER_OF_LEVELS = 3
 };
 // Типы блоков (номер типа - номер текстуры).
 enum BLOCK_TYPES
 {
     BLOCK_STONE = 0,
     BLOCK_GLASS = 1,
     BLOCK_WATER = 2
 };
 // ID нод: луч выбора блоков (урок 7) ловит только ноды с ID_PICKABLE - вода курсор не ловит.
 enum NODE_IDS
 {
     ID_PICKABLE = 1,
     ID_NOT_PICKABLE = 2
 };

 // Информация для отрисовки и сам блок - как в уроке 2: блок хранит только свой тип.
 struct MyVoxelInfo : VoxelInfo
 {
     unsigned int s_Type;
 };
 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(unsigned int type) : SimpleVoxelObject(), s_Type(type) { setTypeVoxel(CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo() // Новый объект - его удалит движок.
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Type = s_Type;
         return info;
     }
     unsigned int getType() { return s_Type; }
 private:
     unsigned int s_Type;
 };

 // Видна ли грань блока parent, за которой стоит сосед svo. Стекло и вода прозрачны для всех, кроме таких же блоков:
 // грани между двумя блоками воды (или стекла) не рисуются, а грань камня под водой и грань воды за стеклом - рисуются.
 class MyTransparentTester : public SimpleVoxelObjectTransparentTester
 {
 public:
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent)
     {
         if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false;
         unsigned int type = ((MyBlock*)svo)->getType();
         if(type != BLOCK_GLASS && type != BLOCK_WATER) return false; // Камень непрозрачен.
         if(!parent || parent->getTypeVoxel() != CUSTOM_VOXEL) return true; // parent может быть NULL - проверяем.
         return ((MyBlock*)parent)->getType() != type;
     }
 };

 // Фабрика: текстура грани - по типу блока, уровень мешей - тоже по типу блока.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve, ITexture** textures) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh();
         for(int i = 0; i < 3; i++) s_Textures[i] = textures[i];
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения.
     {
         unsigned int type = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Type;
         s_Plane->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[type]); // Вызовы идут по одному - общую грань менять можно.
         return getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1)); // Новый меш - копия грани вместе с текстурой.
     }
     int getMeshNumber(VoxelForRendering* vxl) // На какой уровень попадёт воксель (вызывается сразу после getMeshVoxel()).
     {
         unsigned int type = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Type;
         if(type == BLOCK_GLASS) return LEVEL_GLASS;
         if(type == BLOCK_WATER) return LEVEL_WATER;
         return LEVEL_SOLID;
     }
     IMesh* s_Plane;
     ITexture* s_Textures[3];
 };

 // Текстура 16x16 прямо в коде: камень - серый с узором, стекло - непрозрачная рамка и блик, в середине дыра
 // (альфа 0), вода - полупрозрачная синяя (альфа 140).
 ITexture* makeTexture(IVideoDriver* driver, const char* name, unsigned int type)
 {
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16));
     for(u32 x = 0; x < 16; x++)
         for(u32 y = 0; y < 16; y++)
         {
             SColor color(140, 40, 90, 200); // Вода.
             if(type == BLOCK_STONE) color = SColor(255, 100 + (x * 7 + y * 13) % 40, 100 + (x * 7 + y * 13) % 40, 110);
             if(type == BLOCK_GLASS)
             {
                 bool frame = (x == 0 || y == 0 || x == 15 || y == 15 || (x == y && x > 3 && x < 12));
                 color = frame ? SColor(255, 200, 230, 255) : SColor(0, 255, 255, 255);
             }
             image->setPixel(x, y, color);
         }
     ITexture* texture = driver->addTexture(name, image); // Текстура - копия картинки.
     image->drop();
     return texture;
 }

 // Шейдер (GLSL): цвет текстуры, умноженный на цвет вершины и яркость. Альфа - из текстуры: её дальше обрабатывает
 // базовый материал (альфа-тест у стекла, смешивание у воды).
 const char* VERTEX_SHADER =
     "uniform float s_Brightness;\n"
     "varying vec3 s_Color;\n"
     "void main(void)\n"
     "{\n"
     "    gl_Position = ftransform();\n"
     "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
     "    s_Color = gl_Color.rgb * s_Brightness;\n"
     "}\n";
 const char* PIXEL_SHADER =
     "uniform sampler2D s_Texture;\n"
     "varying vec3 s_Color;\n"
     "void main(void)\n"
     "{\n"
     "    vec4 color = texture2D(s_Texture, gl_TexCoord[0].xy);\n"
     "    gl_FragColor = vec4(color.rgb * s_Color, color.a);\n"
     "}\n";

 // Константы шейдера: Irrlicht вызывает OnSetConstants() перед отрисовкой каждого мешбуфера с этим материалом.
 class MyShaderCallBack : public IShaderConstantSetCallBack
 {
 public:
     MyShaderCallBack() : s_Brightness(1.f) {}
     void OnSetConstants(IMaterialRendererServices* services, s32)
     {
         s32 layer = 0; // Текстура - из слоя 0.
         services->setPixelShaderConstant("s_Texture", &layer, 1);
         services->setVertexShaderConstant("s_Brightness", &s_Brightness, 1);
     }
     f32 s_Brightness; // Клавиша N - день и ночь.
 };

 // Клавиша N переключает яркость. OnEvent() вызывается в главном потоке.
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver() : s_CallBack(0) {}
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown && event.KeyInput.Key == KEY_KEY_N && s_CallBack)
             s_CallBack->s_Brightness = (s_CallBack->s_Brightness > 0.7f) ? 0.4f : 1.f;
         return false;
     }
     MyShaderCallBack* s_CallBack;
 };

 // Блок по глобальным координатам (до start() - задание в очереди, выполнится после запуска).
 void setBlock(IrrSimpleVoxelEngine* isve, int id, int x, int y, int z, unsigned int type)
 {
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, z), new MyBlock(type));
 }


 int main()
 {
     MyReceiver receiver;
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600), 32, false, false, false, &receiver);
     IVideoDriver* driver = device->getVideoDriver();
     // Материалы уровней: встроенные, а если есть GLSL - шейдеры на их основе (последний параметр - базовый материал).
     E_MATERIAL_TYPE materials[NUMBER_OF_LEVELS] = { EMT_SOLID, EMT_TRANSPARENT_ALPHA_CHANNEL_REF, EMT_TRANSPARENT_ALPHA_CHANNEL };
     MyShaderCallBack* callback = new MyShaderCallBack;
     IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
     if(gpu && driver->queryFeature(EVDF_ARB_GLSL))
         for(int i = 0; i < NUMBER_OF_LEVELS; i++)
         {
             s32 shader = gpu->addHighLevelShaderMaterial(VERTEX_SHADER, "main", EVST_VS_1_1, PIXEL_SHADER, "main", EPST_PS_1_1, callback, materials[i]);
             if(shader >= 0) materials[i] = (E_MATERIAL_TYPE)shader; // Шейдер не собрался - остаётся встроенный материал.
         }
     receiver.s_CallBack = callback;
     ITexture* textures[3] = { makeTexture(driver, "stone", BLOCK_STONE), makeTexture(driver, "glass", BLOCK_GLASS), makeTexture(driver, "water", BLOCK_WATER) };

     Logger* lgr = new Logger;
     lgr->setPrintStandard(false);
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop();
     MyFactory* factory = new MyFactory(device, isve, textures);
     factory->setPositionScale(16);
     factory->setSScale(16);
     factory->setNumberOfMeshesSceneNodeForChunk(NUMBER_OF_LEVELS); // Первым из настроек уровней: сбрасывает их материалы.
     for(int i = 0; i < NUMBER_OF_LEVELS; i++)
     {
         factory->setMaterialType(i, materials[i]); // По умолчанию для всех миров - до addWorld().
         factory->setHardwareMappingHint(i, EHM_STATIC, EBT_VERTEX_AND_INDEX); // Меши чанков - в видеопамяти.
     }
     isve->setFactoryChunkMeshSceneNode(factory); // После этого число уровней менять нельзя.
     int id = isve->addWorld();
     isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MyTransparentTester); // Мир удалит его сам.
     for(int i = 0; i < NUMBER_OF_LEVELS; i++) factory->setMaterialFlag(i, id, EMF_LIGHTING, false); // Флаги - для мира, после addWorld().
     factory->setMaterialFlag(LEVEL_WATER, id, EMF_BACK_FACE_CULLING, false); // Воду видно и изнутри (из-под воды).
     factory->setID(LEVEL_SOLID, id, ID_PICKABLE);
     factory->setID(LEVEL_GLASS, id, ID_PICKABLE);
     factory->setID(LEVEL_WATER, id, ID_NOT_PICKABLE);

     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++) setBlock(isve, id, x, 0, z, BLOCK_STONE); // Пол.
     // Стеклянный домик 5x5x5 (только стенки) с каменной колонной внутри.
     for(int x = 2; x <= 6; x++)
         for(int y = 1; y <= 5; y++)
             for(int z = 2; z <= 6; z++)
                 if(x == 2 || x == 6 || y == 5 || z == 2 || z == 6) setBlock(isve, id, x, y, z, BLOCK_GLASS);
     for(int y = 1; y <= 3; y++) setBlock(isve, id, 4, y, 4, BLOCK_STONE);
     // Бассейн: бортик из камня, передняя стенка (к камере) - из стекла, внутри вода 4x2x4.
     for(int x = 9; x <= 14; x++)
         for(int y = 1; y <= 2; y++)
             for(int z = 9; z <= 14; z++)
             {
                 bool rim = (x == 9 || x == 14 || z == 9 || z == 14);
                 setBlock(isve, id, x, y, z, rim ? (z == 9 ? BLOCK_GLASS : BLOCK_STONE) : BLOCK_WATER);
             }
     isve->addTaskUpdateChunks();

     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS(0, 100.f, 0.01f);
     camera->setPosition(vector3df(8, 7, -8));
     camera->setTarget(vector3df(8, 1, 8));
     device->getCursorControl()->setVisible(false);
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update(); // Здесь же враппер сортирует треугольники полупрозрачных уровней.
         driver->beginScene(true, true, SColor(255, 150, 190, 230));
         device->getSceneManager()->drawAll();
         driver->endScene();
     }
     isve->stop();
     delete isve;
     callback->drop(); // Шейдеры держат свою ссылку на callback.
     device->drop();
     return 0;
 }
 \endcode
*/
