/*! \page tutorial13 Урок 13: Вложенные чанки.
 Вспомним одну из главных идей SimpleVoxelEngine (раздел "Общие идеи работы движка" его документации): чанк - это
 тоже воксель. Значит, в ячейку чанка можно положить не блок, а ещё один чанк со своим разбиением - и получить внутри
 блока маленькие блоки. Целый блок храним одним объектом, а разбитый, повреждённый, "выгрызенный" взрывом - вложенным
 чанком. Ядро поддерживает вложенные чанки любой глубины и с любым разбиением во всех своих функциях (соседи, видимость
 граней, адреса, поиск по точке), а враппер их рисует. В этом уроке мы:
 - разобьём блок на 4x4x4 маленьких блока и поставим и удалим маленький блок по адресу из трёх векторов;
 - посмотрим, как маленькие блоки приходят в фабрику и как подогнать им текстуру;
 - научимся ломать и ставить маленькие блоки под курсором (getAddressSimpleVoxelObjectByPoint());
 - сделаем взрыв, край воронки которого режется по маленьким блокам, и свет, который освещает каждый маленький блок
   по отдельности.
 Всё это есть и в TestProject3 (клавиша C включает режим маленьких блоков: левый щелчок разбивает блок, правый ставит
 маленький блок, B и L - взрыв и свет по маленьким блокам), здесь - то же самое в коротком примере. Адреса вокселей
 VoxelAddress разобраны в уроке 4 SimpleVoxelEngine.

 Начнём с подключений и двух констант: размер чанка и на сколько частей по каждой оси разбивается блок.
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <vector>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace Defines;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 const int SIZE_CHUNK = 16; // Ребро чанка в блоках.
 const int PARTS = 4; // На сколько частей по каждой оси разбивается блок: 4x4x4 = 64 маленьких блока.
 \endcode
 Нам часто понадобится адрес блока первого уровня по его глобальным координатам: вектор [0] - чанк, вектор [1] - блок
 в чанке. Делить нужно с округлением вниз: блок с координатой -1 лежит в чанке -1 на месте 15, а обычное деление в C++
 округляет к нулю и дало бы чанк 0.
 \code
 // Адрес блока первого уровня по его глобальным координатам: [0] - чанк, [1] - блок в чанке. Деление - с округлением
 // вниз (блок -1 лежит в чанке -1 на месте 15). Адрес забирает тот, кому его отдали (setSimpleVoxelObject() и др.).
 VoxelAddress* createAddressOfBlock(IrrSimpleVoxelEngine* isve, Vector3d<int> block)
 {
     const int g[3] = { block.getX(), block.getY(), block.getZ() };
     int c[3];
     for(int k = 0; k < 3; k++) c[k] = g[k] >= 0 ? g[k] / SIZE_CHUNK : -((-g[k] + SIZE_CHUNK - 1) / SIZE_CHUNK);
     VoxelAddress* addr = new VoxelAddress(isve);
     addr->addNextVector(Vector3d<int>(c[0], c[1], c[2]));
     addr->addNextVector(Vector3d<int>(g[0] - c[0] * SIZE_CHUNK, g[1] - c[1] * SIZE_CHUNK, g[2] - c[2] * SIZE_CHUNK));
     return addr;
 }
 \endcode
 Генератор делает землю из четырёх слоёв (y = 0..3) в чанке (0,0,0):
 \code
 // Генератор: в чанке (0,0,0) - земля из слоёв y = 0..3, остальные чанки пустые.
 class GroundGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         if(!(chunk->getVoxelAddress()->getVector(0) == Vector3d<int>(0, 0, 0))) return true;
         for(int x = 0; x < size; x++)
             for(int y = 0; y < 4; y++)
                 for(int z = 0; z < size; z++)
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 90, 170 - y * 20, 70)), true, false);
         return true;
     }
 };
 \endcode
 \section t13factory Маленькие блоки в фабрике
 Для фабрики маленький блок ничем не отличается от обычного: getMeshVoxel() вызывается для каждого видимого вокселя на
 любой глубине вложенности, а VoxelForRendering уже содержит его позицию и размер. Размер - vxl->getScale(): при
 масштабах фабрики, равных размеру чанка, целый блок - 1, маленький из разбиения на 4 - 0.25 (из разбиения 4 внутри
 разбиения 4 - 1/16 и так далее). getMeshCubeFaces() сама строит куб в размер вокселя, так что маленькие блоки
 нарисуются правильно без единой строчки кода.

 С текстурой сложнее. Грань createFaceMesh() растягивает текстуру целиком (от 0 до 1) на любую грань, и маленький блок
 получит всю текстуру блока, уменьшенную в 4 раза: разбитый блок будет выглядеть как мозаика из 64 маленьких копий.
 Чтобы разбитый блок выглядел так же, как целый, маленькому блоку нужно взять текстурные координаты с того места грани
 целого блока, где он лежит. Чтобы это было хорошо видно, сделаем текстуру прямо в коде - светлую с тёмной рамкой
 (текстура без рамки на маленьком блоке - это четверть рамки на целом блоке):
 \code
 // Фабрика: куб из видимых граней с текстурой-рамкой; цвет вершин - цвет вокселя. Маленьким блокам текстурные
 // координаты берутся с того места грани целого блока, где маленький блок лежит.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         IVideoDriver* driver = device->getVideoDriver();
         IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16)); // Текстура 16x16: светлая с тёмной рамкой.
         for(u32 x = 0; x < 16; x++)
             for(u32 y = 0; y < 16; y++)
                 image->setPixel(x, y, (x == 0 || y == 0 || x == 15 || y == 15) ? SColor(255, 60, 60, 60) : SColor(255, 230, 230, 230));
         ITexture* texture = driver->addTexture("block_frame", image);
         image->drop();
         s_Plane = createFaceMesh();
         s_Plane->getMeshBuffer(0)->getMaterial().setTexture(0, texture);
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения. Маленький блок приходит сюда так же, как обычный.
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1)); // Размер куба - getScale() вокселя (маленький - 1/4).
         if(!mesh) return 0;
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         if(vxl->getScale().getX() < 0.999f) fitTexCoordsToWholeBlock(mesh, vxl); // Маленький блок.
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
 \endcode
 Маленький блок узнаём по размеру меньше 1 и подгоняем ему текстурные координаты функцией из TestProject3. Она не
 знает, как повёрнута текстура на каждой грани (это задаёт модель грани), и поэтому для каждого треугольника находит
 линейную зависимость текстурных координат от положения на грани, а потом применяет ту же зависимость в масштабе
 целого блока. Центр целого блока - ближайший целый узел, потому что центры блоков первого уровня лежат в целых
 координатах. Функция работает в потоке построения (как и весь getMeshVoxel()) и меняет только меш, который мы сами
 только что получили, - это безопасно.
 \code
 // Текстурные координаты маленького блока - с того же места грани целого блока (как в TestProject3): тогда разбитый
 // блок выглядит так же, как целый. Для каждого треугольника находится линейная зависимость текстурных координат от
 // положения на грани, и та же зависимость применяется в масштабе целого блока (размер 1, центр - ближайший целый узел).
 void fitTexCoordsToWholeBlock(IMesh* mesh, VoxelForRendering* vxl)
 {
     const float size = vxl->getScale().getX(); // Размер маленького блока (целый - 1).
     const float center[3] = { vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ() };
     float offset[3]; // Центр маленького блока относительно центра целого.
     for(int k = 0; k < 3; k++) offset[k] = center[k] - floorf(center[k] + 0.5f);
     for(u32 b = 0; b < mesh->getMeshBufferCount(); b++)
     {
         IMeshBuffer* mb = mesh->getMeshBuffer(b);
         if(mb->getIndexType() != EIT_16BIT) continue;
         const u16* indices = mb->getIndices();
         const u32 vcount = mb->getVertexCount();
         std::vector<vector2df> oldtc(vcount);
         std::vector<bool> done(vcount, false);
         for(u32 i = 0; i < vcount; i++) oldtc[i] = mb->getTCoords(i);
         for(u32 t = 0; t + 2 < mb->getIndexCount(); t += 3)
         {
             const u16 id[3] = { indices[t], indices[t + 1], indices[t + 2] };
             float p[3][3]; // Позиции вершин относительно центра маленького блока.
             for(int j = 0; j < 3; j++) { const vector3df& v = mb->getPosition(id[j]); p[j][0] = v.X; p[j][1] = v.Y; p[j][2] = v.Z; }
             int axis = -1; // Ось нормали грани: по ней у всех вершин одна координата.
             for(int k = 0; k < 3 && axis < 0; k++) if(fabsf(p[1][k] - p[0][k]) < 1e-5f && fabsf(p[2][k] - p[0][k]) < 1e-5f) axis = k;
             if(axis < 0) continue;
             const int t1 = (axis + 1) % 3, t2 = (axis + 2) % 3;
             // tc = A * (положение на грани) + c; A находим по двум сторонам треугольника.
             const float d1x = p[1][t1] - p[0][t1], d1y = p[1][t2] - p[0][t2];
             const float d2x = p[2][t1] - p[0][t1], d2y = p[2][t2] - p[0][t2];
             const float det = d1x * d2y - d1y * d2x;
             if(fabsf(det) < 1e-12f) continue;
             const vector2df e1 = oldtc[id[1]] - oldtc[id[0]], e2 = oldtc[id[2]] - oldtc[id[0]];
             const vector2df a1 = (e1 * d2y - e2 * d1y) / det; // Столбец A при оси t1.
             const vector2df a2 = (e2 * d1x - e1 * d2x) / det; // Столбец A при оси t2.
             const vector2df c = oldtc[id[0]] - a1 * p[0][t1] - a2 * p[0][t2];
             for(int j = 0; j < 3; j++)
             {
                 if(done[id[j]]) continue;
                 // Тот же шаблон грани, растянутый до целого блока: A уменьшается в size раз.
                 mb->getTCoords(id[j]) = a1 * (size * (p[j][t1] + offset[t1])) + a2 * (size * (p[j][t2] + offset[t2])) + c;
                 done[id[j]] = true;
             }
         }
         mb->setDirty(EBT_VERTEX);
     }
 }
 \endcode
 Ну и последнее в фабрике - селектор треугольников каждой ноде чанков, чтобы луч из камеры находил точку под
 курсором (подробно про селекторы - в уроке 7). Здесь самый простой вариант: селектор принадлежит ноде и удаляется
 вместе с ней.
 \code
     // Ноде чанков - селектор треугольников, чтобы луч из камеры находил точку под курсором (урок 7). Селектор
     // принадлежит ноде и удаляется вместе с ней.
     void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int, Vector3d<int>)
     {
         if(!node) return;
         ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createTriangleSelector(node->getMesh(), node);
         node->setTriangleSelector(selector);
         selector->drop();
     }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };
 \endcode
 \section t13split Разбиение блока и адрес из трёх векторов
 Теперь main(). Создаём движок, фабрику и мир как обычно (контроллер UserControllerForThread понадобится для взрыва и
 света, он описан ниже):
 \code
 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     ISceneManager* smgr = device->getSceneManager();
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     isve->setUserControllerForThread(new MyController(isve));
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(SIZE_CHUNK);
     factory->setSScale(SIZE_CHUNK);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMultiplePartition(id, true); // Чанки верхнего уровня разбиты одинаково - вложенным чанкам это не мешает.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     isve->getWorld(id)->setWorldGenerator(new GroundGenerator);
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
 \endcode
 setMultiplePartition(id, true) обещает движку, что все чанки <b>верхнего уровня</b> разбиты одинаково (на
 getSizeChunk()), - вложенным чанкам это не мешает: видимость граней у разбитых блоков и их соседей движок считает
 точно, просто медленнее (подробнее - в уроке 14).

 Разобьём блок (8,3,8). Создаём вложенный чанк ChunkVoxelObject, задаём ему разбиение setNumberOfPartitions() и
 заполняем маленькими блоками - прямо так, до вставки в мир: адрес и мир движок выдаст ему сам при вставке. Потом
 ставим его в мир на место блока, как обычный воксель:
 \code
 // 1) Разбить блок (8,3,8): на его место - вложенный чанк из 4x4x4 маленьких блоков того же цвета.
 ChunkVoxelObject* split = new ChunkVoxelObject();
 split->setNumberOfPartitions(PARTS);
 for(int i = 0; i < PARTS; i++)
     for(int j = 0; j < PARTS; j++)
         for(int k = 0; k < PARTS; k++)
             split->setSimpleVoxelObject(Vector3d<int>(i, j, k), new ARGBColorVoxel(ARGBColor(255, 90, 110, 70)), true, false);
 isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(8, 3, 8), split); // Старый блок удалит движок.
 \endcode
 Несколько важных деталей:
 - разбиение задаётся только пустому чанку (пока в нём нет вокселей), иначе setNumberOfPartitions() вернёт false. Оно
   может быть любым от 1 до 255 и у разных вложенных чанков - разным;
 - последний параметр ChunkVoxelObject::setSimpleVoxelObject() - false: чанк ещё не в мире, соседей помечать на
   пересчёт видимости незачем, при вставке движок всё посчитает сам;
 - старый блок (8,3,8) движок удалит сам, а вложенный чанк вместе с маленькими блоками теперь принадлежит миру;
 - внутрь вложенного чанка можно положить ещё один вложенный чанк - глубина не ограничена.

 Адрес маленького блока - из трёх векторов: чанк, блок в чанке, маленький блок во вложенном чанке. Таким адресом
 можно и поставить (заменить) маленький блок, и удалить его - функциями враппера setSimpleVoxelObject() и
 deleteSimpleVoxelObject() (адрес они забирают):
 \code
 // 2) Маленький блок по адресу из трёх векторов: чанк (0,0,0), блок (8,3,8) в чанке, маленький блок (3,3,3) в нём.
 VoxelAddress* addr = new VoxelAddress(isve);
 addr->addNextVector(Vector3d<int>(0, 0, 0));
 addr->addNextVector(Vector3d<int>(8, 3, 8));
 addr->addNextVector(Vector3d<int>(3, 3, 3));
 isve->setSimpleVoxelObject(id, addr, new ARGBColorVoxel(ARGBColor(255, 220, 40, 40)), true); // true - заменить старый.

 // 3) Удалить маленький блок (0,3,0) того же разбитого блока - в углу верхнего слоя будет ямка.
 VoxelAddress* hole = createAddressOfBlock(isve, Vector3d<int>(8, 3, 8));
 hole->addNextVector(Vector3d<int>(0, 3, 0));
 isve->deleteSimpleVoxelObject(id, hole);
 isve->addTaskUpdateChunks();
 \endcode
 Третий вектор работает, только если в ячейке уже лежит вложенный чанк. Поставить маленький блок в пустую ячейку
 или в целый блок напрямую нельзя - сначала в ячейку кладётся вложенный чанк (как мы сделали выше, а ниже - в
 placeSmallBlock()).

 <b>Осторожно с чтением мира.</b> После разбиения getSimpleVoxelObjectByGlobalVector(id, (8,3,8)) вернёт уже не ваш
 блок, а вложенный чанк - объект с типом CHUNK_VOXEL_OBJECT. Любой код, который читает блоки и приводит их к своему
 типу, должен сначала проверять getTypeVoxel() - иначе он прочитает мусор (поэтому и наш свет, и взрыв ниже, и свет
 в TestProject3 разбирают разбитые блоки отдельно). И помните про сохранение: контроллер из урока 11 и TestProject3 вложенные чанки не
 сохраняют. Если маленькие блоки должны переживать перезапуск, ваш формат сохранения должен уметь записывать
 вложенный чанк (его разбиение и воксели, рекурсивно).

 \section t13cursor Маленькие блоки под курсором
 Чтобы ломать и ставить маленькие блоки мышью, нужен адрес ячейки ровно под курсором.
 getAddressSimpleVoxelObjectByThreePoints() (урок 8) по треугольнику, в который попал луч, находит ячейку у центра
 этого треугольника - для целых блоков этого достаточно, а для маленьких нет: большая грань целого блока закрывает
 сразу 16 маленьких ячеек перед собой, и блок встал бы не там, куда смотрит игрок. Для этого есть
 getAddressSimpleVoxelObjectByPoint(): она берёт саму точку под курсором, сдвигает её вдоль lookvect на 0.001 блока и
 возвращает ячейку, в которой точка оказалась, - на любой глубине вложенных чанков. Направление от камеры даёт сам
 блок под курсором, к камере (lookvect * -1) - пустую ячейку перед гранью, куда можно ставить.

 Точку под курсором каждый кадр находит главный цикл - лучом из камеры по селекторам нод:
 \code
 // Точка под курсором (центр экрана): луч из камеры по селекторам нод чанков.
 vector3df campos = camera->getAbsolutePosition();
 vector3df look = (camera->getTarget() - campos).normalize();
 line3df ray(campos, campos + look * 50.0f);
 vector3df hit;
 triangle3df triangle;
 receiver.s_HasTarget = collision->getSceneNodeAndCollisionPointFromRay(ray, hit, triangle) != 0;
 receiver.s_Point = Vector3d<float>(hit.X, hit.Y, hit.Z);
 receiver.s_Look = Vector3d<float>(look.X, look.Y, look.Z);
 receiver.s_CamPos = Vector3d<float>(campos.X, campos.Y, campos.Z);
 \endcode
 А обработчик событий по щелчкам ломает и ставит блоки. Ломать просто: адрес из двух векторов - целый блок, из
 трёх - маленький, deleteSimpleVoxelObject() удалит и то, и другое.
 \code
 // Обработчик: левая кнопка - сломать блок (или маленький блок) под курсором, правая - поставить маленький блок,
 // B - взрыв, L - свет в блоке под курсором. Точку под курсором каждый кадр заполняет главный цикл.
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver(IrrSimpleVoxelEngine* isve, int id) : s_ISVE(isve), s_WorldId(id) {}
     bool OnEvent(const SEvent& event)
     {
         if(!s_HasTarget) return false;
         if(event.EventType == EET_MOUSE_INPUT_EVENT)
         {
             if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) breakUnderCursor();
             if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) placeSmallBlock();
         }
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_B) addTask(BOOM, 2);
             if(event.KeyInput.Key == KEY_KEY_L) addTask(LIGHT, 3);
         }
         return false;
     }
     // Сломать то, что под курсором: точку чуть сдвигаем от камеры - внутрь блока. На любой глубине вложенных чанков:
     // адрес из двух векторов - целый блок, из трёх - маленький блок.
     bool breakUnderCursor()
     {
         VoxelAddress* addr = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look, s_CamPos);
         if(!addr) return false;
         if(addr->getLastVectInd() < 2) { delete addr; return false; } // Чанк не загружен.
         s_ISVE->deleteSimpleVoxelObject(s_WorldId, addr); // Адрес забирает движок.
         s_ISVE->addTaskUpdateChunks();
         return true;
     }
 \endcode
 Ставить сложнее. Если ячейка к камере - пустая маленькая ячейка разбитого блока (адрес из трёх векторов), ставим
 туда маленький блок. Если же это пустая ячейка первого уровня (адрес из двух векторов), кладём в неё вложенный чанк с
 одним маленьким блоком - тем, что под курсором. Какой это из 4x4x4, считаем по той же точке в координатах мира
 (getWorldCoordinates(), урок 12): дробная часть координаты (с учётом того, что блок k занимает от k - 0.5 до k + 0.5),
 умноженная на PARTS. Мир здесь читаем напрямую (World::getSimpleVoxelObject()) - значит, под блокировкой движка:
 потоки построения в это время могут читать мир, а поток очереди - менять.
 \code
 // Поставить маленький блок ровно под курсором: точку сдвигаем к камере - в пустую ячейку перед гранью.
 bool placeSmallBlock()
 {
     VoxelAddress* target = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look * (-1), s_CamPos);
     if(!target) return false;
     bool empty = false;
     if(target->getLastVectInd() >= 2)
     {
         std::lock_guard<std::recursive_mutex> lock(s_ISVE->getLockAccessToTheSimpleVoxelEngine()); // Читаем мир напрямую.
         World* wrld = s_ISVE->getWorld(s_WorldId);
         empty = wrld && !wrld->getSimpleVoxelObject(target, false);
     }
     if(!empty) { delete target; return false; }
     if(target->getLastVectInd() >= 3) // Пустая маленькая ячейка уже разбитого блока - ставим маленький блок.
     {
         s_ISVE->setSimpleVoxelObject(s_WorldId, target, new ARGBColorVoxel(ARGBColor(255, 80, 80, 255)));
         s_ISVE->addTaskUpdateChunks();
         return true;
     }
     // Пустая ячейка первого уровня: ставим вложенный чанк с одним маленьким блоком. Какой из PARTS^3 - по той же
     // точке в координатах мира (блок k занимает [k - 0.5, k + 0.5)).
     Vector3d<float> look = s_ISVE->getWorldCoordinates(s_WorldId, s_CamPos) - s_ISVE->getWorldCoordinates(s_WorldId, s_CamPos + s_Look);
     Vector3d<float> w = s_ISVE->getWorldCoordinates(s_WorldId, s_Point) + look.getNormalizeVector() * 0.001f;
     const float q[3] = { w.getX() + 0.5f, w.getY() + 0.5f, w.getZ() + 0.5f };
     int sub[3];
     for(int k = 0; k < 3; k++)
     {
         sub[k] = (int)floorf((q[k] - floorf(q[k])) * PARTS);
         if(sub[k] < 0) sub[k] = 0;
         if(sub[k] >= PARTS) sub[k] = PARTS - 1;
     }
     ChunkVoxelObject* nested = new ChunkVoxelObject(); // Вложенный чанк заполняется до вставки в мир.
     nested->setNumberOfPartitions(PARTS);
     nested->setSimpleVoxelObject(Vector3d<int>(sub[0], sub[1], sub[2]), new ARGBColorVoxel(ARGBColor(255, 80, 80, 255)), true, false);
     s_ISVE->setSimpleVoxelObject(s_WorldId, target, nested); // Адрес и вложенный чанк забирает движок.
     s_ISVE->addTaskUpdateChunks();
     return true;
 }
 \endcode
 \section t13boom Взрыв и свет по маленьким блокам
 Взрыв и свет меняют много блоков сразу, поэтому, как и в уроке 10, их делает пользовательская функция
 UserControllerForThread в потоке главной очереди: пока она работает, мир меняет только она, и указатели на воксели,
 полученные внутри неё, действительны до её конца. Обработчик только упаковывает параметры - блок первого уровня под
 курсором и радиус:
 \code
 // Взрыв или свет - в потоке главной очереди (UserControllerForThread). Центр - блок первого уровня под курсором.
 bool addTask(MY_TASK type, int radius)
 {
     VoxelAddress* addr = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look, s_CamPos);
     if(!addr) return false;
     bool good = addr->getLastVectInd() >= 2;
     if(good)
     {
         MyParams* p = new MyParams; // Удалит движок после выполнения.
         p->s_Type = type;
         p->s_WorldId = s_WorldId;
         p->s_Center = addr->getVector(1) + addr->getVector(0) * SIZE_CHUNK; // Чанк и блок в чанке - в глобальные координаты.
         p->s_Radius = radius;
         s_ISVE->addTaskRunUserControllerForThread(p);
     }
     delete addr;
     return good;
 }
 \endcode
 \code
 // Параметры взрыва и света для потока главной очереди.
 enum MY_TASK { BOOM, LIGHT };
 struct MyParams : ParametersForThreadFunction
 {
     MY_TASK s_Type;
     int s_WorldId;
     Vector3d<int> s_Center; // Блок первого уровня в центре (глобальные координаты).
     int s_Radius;
 };
 \endcode
 Взрыв. Обычный взрыв просто удаляет блоки внутри шара, и край воронки получается ступенчатым. Сделаем его
 "выгрызенным": каждую ячейку рядом с шаром рассматриваем как n^3 маленьких блоков (целый блок - как будто уже разбитый
 на PARTS^3, разбитый - по своим маленьким блокам) и удаляем маленькие блоки, центр которых внутри шара. Если удалены
 все - ячейка пустеет, если ни одного - не меняется, иначе на её место ставится новый вложенный чанк из оставшихся
 маленьких блоков. Проходов два: первый только считает, второй строит вложенный чанк - так целые блоки, которые взрыв не
 задел, не превращаются зря во вложенные чанки (они дороже и в памяти, и при построении мешей).
 \code
 // Взрыв и свет по маленьким блокам - в потоке главной очереди: пока функция работает, мир меняет только она.
 class MyController : public UserControllerForThread
 {
 public:
     MyController(IrrSimpleVoxelEngine* isve) : UserControllerForThread(isve) {}
     void userThreadFunction(ParametersForThreadFunction* params)
     {
         MyParams* p = (MyParams*)params;
         if(p->s_Type == BOOM) boom(p);
         else light(p);
         getIrrSimpleVoxelEngine()->addTaskUpdateChunks(); // Перестроить изменённые чанки - один раз после всех правок.
     }
 private:
     // Каждая ячейка около шара - n^3 маленьких блоков (целый блок - как будто разбитый на PARTS^3): маленькие блоки,
     // центр которых внутри шара, удаляются. Удалены все - ячейка пустеет, ни одного - не меняется, иначе на её место
     // ставится вложенный чанк из оставшихся маленьких блоков. Край воронки получается "выгрызенным".
     void boom(MyParams* p)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         const int ext = p->s_Radius + 1; // Ячейка рядом с шаром тоже может задеть его своими маленькими блоками.
         const float r2 = (float)(p->s_Radius * p->s_Radius);
         for(int x = -ext; x <= ext; x++)
             for(int y = -ext; y <= ext; y++)
                 for(int z = -ext; z <= ext; z++)
                 {
                     const Vector3d<int> cell = p->s_Center + Vector3d<int>(x, y, z);
                     if(cell.getX() < 0 || cell.getX() >= SIZE_CHUNK || cell.getY() < 0 || cell.getY() >= SIZE_CHUNK ||
                        cell.getZ() < 0 || cell.getZ() >= SIZE_CHUNK) continue; // Только в загруженном чанке (0,0,0).
                     SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, false);
                     if(!obj) continue;
                     ChunkVoxelObject* old = 0; // Уже разбитый блок.
                     int n = PARTS;
                     if(obj->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                     {
                         old = (ChunkVoxelObject*)obj;
                         n = old->getNumberOfPartitions();
                     }
                     int kept = 0, removed = 0;
                     for(int pass = 0; pass < 2; pass++) // Проход 0 - считаем, проход 1 - строим вложенный чанк.
                     {
                         ChunkVoxelObject* nested = 0;
                         if(pass == 1)
                         {
                             if(removed == 0) break; // Взрыв ячейку не задел.
                             if(kept == 0) { isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, 0); break; } // Уничтожено всё.
                             nested = new ChunkVoxelObject();
                             nested->setNumberOfPartitions(n);
                         }
                         for(int i = 0; i < n; i++)
                             for(int j = 0; j < n; j++)
                                 for(int k = 0; k < n; k++)
                                 {
                                     SimpleVoxelObject* src = old ? old->getSimpleVoxelObject(Vector3d<int>(i, j, k)) : obj;
                                     if(!src || src->getTypeVoxel() != ARGBCOLOR) continue; // Пустая маленькая ячейка.
                                     // Центр маленького блока относительно центра шара (блок занимает [-0.5, 0.5) вокруг своих координат).
                                     const float dx = x - 0.5f + (i + 0.5f) / n, dy = y - 0.5f + (j + 0.5f) / n, dz = z - 0.5f + (k + 0.5f) / n;
                                     const bool inside = dx * dx + dy * dy + dz * dz <= r2;
                                     if(pass == 0) { if(inside) removed++; else kept++; }
                                     else if(!inside) nested->setSimpleVoxelObject(Vector3d<int>(i, j, k), new ARGBColorVoxel(((ARGBColorVoxel*)src)->getARGBColor()), true, false);
                                 }
                         if(nested) isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, nested); // Старое удалит движок.
                     }
                 }
     }
 \endcode
 Обратите внимание: старый вложенный чанк (old) мы не меняем на месте, а читаем из него и строим новый - старый
 движок удалит сам при замене. Копии маленьких блоков создаём через new: один и тот же воксель не может лежать в двух
 местах.

 Свет. Блоки в SimpleVoxelEngine неизменяемые: если поменять цвет вокселя на месте (ARGBColorVoxel::setARGBColor()),
 движок не узнает, что чанк нужно перестроить. Поэтому на место каждого освещённого вокселя ставится новый, с новым
 цветом. Целый блок меняем по глобальным координатам, а маленькие блоки разбитого - каждый по своему центру и по
 адресу из трёх векторов:
 \code
     // Свет: блоки внутри шара желтеют тем сильнее, чем ближе к центру. Маленькие блоки разбитого блока - каждый по своему
     // центру, по адресу из трёх векторов. Воксели неизменяемые: на место каждого ставится новый с новым цветом.
     void light(MyParams* p)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         const int r = p->s_Radius;
         for(int x = -r; x <= r; x++)
             for(int y = -r; y <= r; y++)
                 for(int z = -r; z <= r; z++)
                 {
                     const Vector3d<int> cell = p->s_Center + Vector3d<int>(x, y, z);
                     if(cell.getX() < 0 || cell.getX() >= SIZE_CHUNK || cell.getY() < 0 || cell.getY() >= SIZE_CHUNK ||
                        cell.getZ() < 0 || cell.getZ() >= SIZE_CHUNK) continue;
                     SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, false);
                     if(!obj) continue;
                     if(obj->getTypeVoxel() == ARGBCOLOR) // Целый блок.
                     {
                         float d = sqrtf((float)(x * x + y * y + z * z));
                         if(d <= r) isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, new ARGBColorVoxel(lit(((ARGBColorVoxel*)obj)->getARGBColor(), d, r)));
                         continue;
                     }
                     if(obj->getTypeVoxel() != CHUNK_VOXEL_OBJECT) continue;
                     ChunkVoxelObject* nested = (ChunkVoxelObject*)obj; // Разбитый блок: каждый маленький блок отдельно.
                     const int n = nested->getNumberOfPartitions();
                     for(int i = 0; i < n; i++)
                         for(int j = 0; j < n; j++)
                             for(int k = 0; k < n; k++)
                             {
                                 SimpleVoxelObject* piece = nested->getSimpleVoxelObject(Vector3d<int>(i, j, k));
                                 if(!piece || piece->getTypeVoxel() != ARGBCOLOR) continue;
                                 const float dx = x - 0.5f + (i + 0.5f) / n, dy = y - 0.5f + (j + 0.5f) / n, dz = z - 0.5f + (k + 0.5f) / n;
                                 const float d = sqrtf(dx * dx + dy * dy + dz * dz);
                                 if(d > r) continue;
                                 VoxelAddress* addr = createAddressOfBlock(isve, cell); // Чанк и блок в чанке...
                                 addr->addNextVector(Vector3d<int>(i, j, k)); // ...и маленький блок во вложенном чанке.
                                 isve->setSimpleVoxelObject(p->s_WorldId, addr, new ARGBColorVoxel(lit(((ARGBColorVoxel*)piece)->getARGBColor(), d, r)), true);
                             }
                 }
     }
     // Цвет c, подсвеченный жёлтым: в центре света - сильнее, на расстоянии r - никак.
     static ARGBColor lit(ARGBColor c, float d, int r)
     {
         const float power = r > 0 ? 1 - d / r : 1;
         return ARGBColor(c.getA(), (unsigned char)(c.getR() + (255 - c.getR()) * power), (unsigned char)(c.getG() + (255 - c.getG()) * power), c.getB());
     }
 };
 \endcode
 В конце userThreadFunction() один раз вызывается addTaskUpdateChunks() - все изменённые чанки перестроятся один раз
 после всех правок (а не после каждой из сотен).

 Осталось запустить движок и главный цикл (точку под курсором он находит после drawAll(), как показано выше):
 \code
 ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
 camera->setPosition(vector3df(8, 7, 3));
 camera->setTarget(vector3df(8, 3.5f, 8));
 MyReceiver receiver(isve, id);
 device->setEventReceiver(&receiver);
 ISceneCollisionManager* collision = smgr->getSceneCollisionManager();
 isve->start();
 \endcode
 \section t13cost Цена маленьких блоков
 Вложенные чанки - не бесплатные:
 - разбитый блок 4x4x4 - это 64 воксели и массивы вложенного чанка вместо одного блока;
 - у каждого маленького блока - свои видимые грани: на месте одной грани целого блока может оказаться 16;
 - видимость граней у вложенных чанков и их соседей считается медленным путём, а не быстрым (как у чанков с
   одинаковым разбиением).
 Поэтому разбивайте блоки, только когда это действительно нужно (повреждение, мелкие детали), а не "на всякий
 случай". Хорошая идея - собирать вложенный чанк обратно в целый блок, если все маленькие блоки снова одинаковые.
 Для небольших фигурных блоков, у которых форма заранее известна (полублоки, лестницы), чаще выгоднее склейка вокселя
 из частей - getMeshVoxelByGlue() (урок 4).

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <vector>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace Defines;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 const int SIZE_CHUNK = 16; // Ребро чанка в блоках.
 const int PARTS = 4; // На сколько частей по каждой оси разбивается блок: 4x4x4 = 64 маленьких блока.

 // Адрес блока первого уровня по его глобальным координатам: [0] - чанк, [1] - блок в чанке. Деление - с округлением
 // вниз (блок -1 лежит в чанке -1 на месте 15). Адрес забирает тот, кому его отдали (setSimpleVoxelObject() и др.).
 VoxelAddress* createAddressOfBlock(IrrSimpleVoxelEngine* isve, Vector3d<int> block)
 {
     const int g[3] = { block.getX(), block.getY(), block.getZ() };
     int c[3];
     for(int k = 0; k < 3; k++) c[k] = g[k] >= 0 ? g[k] / SIZE_CHUNK : -((-g[k] + SIZE_CHUNK - 1) / SIZE_CHUNK);
     VoxelAddress* addr = new VoxelAddress(isve);
     addr->addNextVector(Vector3d<int>(c[0], c[1], c[2]));
     addr->addNextVector(Vector3d<int>(g[0] - c[0] * SIZE_CHUNK, g[1] - c[1] * SIZE_CHUNK, g[2] - c[2] * SIZE_CHUNK));
     return addr;
 }

 // Генератор: в чанке (0,0,0) - земля из слоёв y = 0..3, остальные чанки пустые.
 class GroundGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         if(!(chunk->getVoxelAddress()->getVector(0) == Vector3d<int>(0, 0, 0))) return true;
         for(int x = 0; x < size; x++)
             for(int y = 0; y < 4; y++)
                 for(int z = 0; z < size; z++)
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 90, 170 - y * 20, 70)), true, false);
         return true;
     }
 };

 // Фабрика: куб из видимых граней с текстурой-рамкой; цвет вершин - цвет вокселя. Маленьким блокам текстурные
 // координаты берутся с того места грани целого блока, где маленький блок лежит.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         IVideoDriver* driver = device->getVideoDriver();
         IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16)); // Текстура 16x16: светлая с тёмной рамкой.
         for(u32 x = 0; x < 16; x++)
             for(u32 y = 0; y < 16; y++)
                 image->setPixel(x, y, (x == 0 || y == 0 || x == 15 || y == 15) ? SColor(255, 60, 60, 60) : SColor(255, 230, 230, 230));
         ITexture* texture = driver->addTexture("block_frame", image);
         image->drop();
         s_Plane = createFaceMesh();
         s_Plane->getMeshBuffer(0)->getMaterial().setTexture(0, texture);
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения. Маленький блок приходит сюда так же, как обычный.
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1)); // Размер куба - getScale() вокселя (маленький - 1/4).
         if(!mesh) return 0;
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         if(vxl->getScale().getX() < 0.999f) fitTexCoordsToWholeBlock(mesh, vxl); // Маленький блок.
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     // Текстурные координаты маленького блока - с того же места грани целого блока (как в TestProject3): тогда разбитый
     // блок выглядит так же, как целый. Для каждого треугольника находится линейная зависимость текстурных координат от
     // положения на грани, и та же зависимость применяется в масштабе целого блока (размер 1, центр - ближайший целый узел).
     void fitTexCoordsToWholeBlock(IMesh* mesh, VoxelForRendering* vxl)
     {
         const float size = vxl->getScale().getX(); // Размер маленького блока (целый - 1).
         const float center[3] = { vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ() };
         float offset[3]; // Центр маленького блока относительно центра целого.
         for(int k = 0; k < 3; k++) offset[k] = center[k] - floorf(center[k] + 0.5f);
         for(u32 b = 0; b < mesh->getMeshBufferCount(); b++)
         {
             IMeshBuffer* mb = mesh->getMeshBuffer(b);
             if(mb->getIndexType() != EIT_16BIT) continue;
             const u16* indices = mb->getIndices();
             const u32 vcount = mb->getVertexCount();
             std::vector<vector2df> oldtc(vcount);
             std::vector<bool> done(vcount, false);
             for(u32 i = 0; i < vcount; i++) oldtc[i] = mb->getTCoords(i);
             for(u32 t = 0; t + 2 < mb->getIndexCount(); t += 3)
             {
                 const u16 id[3] = { indices[t], indices[t + 1], indices[t + 2] };
                 float p[3][3]; // Позиции вершин относительно центра маленького блока.
                 for(int j = 0; j < 3; j++) { const vector3df& v = mb->getPosition(id[j]); p[j][0] = v.X; p[j][1] = v.Y; p[j][2] = v.Z; }
                 int axis = -1; // Ось нормали грани: по ней у всех вершин одна координата.
                 for(int k = 0; k < 3 && axis < 0; k++) if(fabsf(p[1][k] - p[0][k]) < 1e-5f && fabsf(p[2][k] - p[0][k]) < 1e-5f) axis = k;
                 if(axis < 0) continue;
                 const int t1 = (axis + 1) % 3, t2 = (axis + 2) % 3;
                 // tc = A * (положение на грани) + c; A находим по двум сторонам треугольника.
                 const float d1x = p[1][t1] - p[0][t1], d1y = p[1][t2] - p[0][t2];
                 const float d2x = p[2][t1] - p[0][t1], d2y = p[2][t2] - p[0][t2];
                 const float det = d1x * d2y - d1y * d2x;
                 if(fabsf(det) < 1e-12f) continue;
                 const vector2df e1 = oldtc[id[1]] - oldtc[id[0]], e2 = oldtc[id[2]] - oldtc[id[0]];
                 const vector2df a1 = (e1 * d2y - e2 * d1y) / det; // Столбец A при оси t1.
                 const vector2df a2 = (e2 * d1x - e1 * d2x) / det; // Столбец A при оси t2.
                 const vector2df c = oldtc[id[0]] - a1 * p[0][t1] - a2 * p[0][t2];
                 for(int j = 0; j < 3; j++)
                 {
                     if(done[id[j]]) continue;
                     // Тот же шаблон грани, растянутый до целого блока: A уменьшается в size раз.
                     mb->getTCoords(id[j]) = a1 * (size * (p[j][t1] + offset[t1])) + a2 * (size * (p[j][t2] + offset[t2])) + c;
                     done[id[j]] = true;
                 }
             }
             mb->setDirty(EBT_VERTEX);
         }
     }
     // Ноде чанков - селектор треугольников, чтобы луч из камеры находил точку под курсором (урок 7). Селектор
     // принадлежит ноде и удаляется вместе с ней.
     void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int, Vector3d<int>)
     {
         if(!node) return;
         ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createTriangleSelector(node->getMesh(), node);
         node->setTriangleSelector(selector);
         selector->drop();
     }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };

 // Параметры взрыва и света для потока главной очереди.
 enum MY_TASK { BOOM, LIGHT };
 struct MyParams : ParametersForThreadFunction
 {
     MY_TASK s_Type;
     int s_WorldId;
     Vector3d<int> s_Center; // Блок первого уровня в центре (глобальные координаты).
     int s_Radius;
 };

 // Взрыв и свет по маленьким блокам - в потоке главной очереди: пока функция работает, мир меняет только она.
 class MyController : public UserControllerForThread
 {
 public:
     MyController(IrrSimpleVoxelEngine* isve) : UserControllerForThread(isve) {}
     void userThreadFunction(ParametersForThreadFunction* params)
     {
         MyParams* p = (MyParams*)params;
         if(p->s_Type == BOOM) boom(p);
         else light(p);
         getIrrSimpleVoxelEngine()->addTaskUpdateChunks(); // Перестроить изменённые чанки - один раз после всех правок.
     }
 private:
     // Каждая ячейка около шара - n^3 маленьких блоков (целый блок - как будто разбитый на PARTS^3): маленькие блоки,
     // центр которых внутри шара, удаляются. Удалены все - ячейка пустеет, ни одного - не меняется, иначе на её место
     // ставится вложенный чанк из оставшихся маленьких блоков. Край воронки получается "выгрызенным".
     void boom(MyParams* p)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         const int ext = p->s_Radius + 1; // Ячейка рядом с шаром тоже может задеть его своими маленькими блоками.
         const float r2 = (float)(p->s_Radius * p->s_Radius);
         for(int x = -ext; x <= ext; x++)
             for(int y = -ext; y <= ext; y++)
                 for(int z = -ext; z <= ext; z++)
                 {
                     const Vector3d<int> cell = p->s_Center + Vector3d<int>(x, y, z);
                     if(cell.getX() < 0 || cell.getX() >= SIZE_CHUNK || cell.getY() < 0 || cell.getY() >= SIZE_CHUNK ||
                        cell.getZ() < 0 || cell.getZ() >= SIZE_CHUNK) continue; // Только в загруженном чанке (0,0,0).
                     SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, false);
                     if(!obj) continue;
                     ChunkVoxelObject* old = 0; // Уже разбитый блок.
                     int n = PARTS;
                     if(obj->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                     {
                         old = (ChunkVoxelObject*)obj;
                         n = old->getNumberOfPartitions();
                     }
                     int kept = 0, removed = 0;
                     for(int pass = 0; pass < 2; pass++) // Проход 0 - считаем, проход 1 - строим вложенный чанк.
                     {
                         ChunkVoxelObject* nested = 0;
                         if(pass == 1)
                         {
                             if(removed == 0) break; // Взрыв ячейку не задел.
                             if(kept == 0) { isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, 0); break; } // Уничтожено всё.
                             nested = new ChunkVoxelObject();
                             nested->setNumberOfPartitions(n);
                         }
                         for(int i = 0; i < n; i++)
                             for(int j = 0; j < n; j++)
                                 for(int k = 0; k < n; k++)
                                 {
                                     SimpleVoxelObject* src = old ? old->getSimpleVoxelObject(Vector3d<int>(i, j, k)) : obj;
                                     if(!src || src->getTypeVoxel() != ARGBCOLOR) continue; // Пустая маленькая ячейка.
                                     // Центр маленького блока относительно центра шара (блок занимает [-0.5, 0.5) вокруг своих координат).
                                     const float dx = x - 0.5f + (i + 0.5f) / n, dy = y - 0.5f + (j + 0.5f) / n, dz = z - 0.5f + (k + 0.5f) / n;
                                     const bool inside = dx * dx + dy * dy + dz * dz <= r2;
                                     if(pass == 0) { if(inside) removed++; else kept++; }
                                     else if(!inside) nested->setSimpleVoxelObject(Vector3d<int>(i, j, k), new ARGBColorVoxel(((ARGBColorVoxel*)src)->getARGBColor()), true, false);
                                 }
                         if(nested) isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, nested); // Старое удалит движок.
                     }
                 }
     }
     // Свет: блоки внутри шара желтеют тем сильнее, чем ближе к центру. Маленькие блоки разбитого блока - каждый по своему
     // центру, по адресу из трёх векторов. Воксели неизменяемые: на место каждого ставится новый с новым цветом.
     void light(MyParams* p)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         const int r = p->s_Radius;
         for(int x = -r; x <= r; x++)
             for(int y = -r; y <= r; y++)
                 for(int z = -r; z <= r; z++)
                 {
                     const Vector3d<int> cell = p->s_Center + Vector3d<int>(x, y, z);
                     if(cell.getX() < 0 || cell.getX() >= SIZE_CHUNK || cell.getY() < 0 || cell.getY() >= SIZE_CHUNK ||
                        cell.getZ() < 0 || cell.getZ() >= SIZE_CHUNK) continue;
                     SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, false);
                     if(!obj) continue;
                     if(obj->getTypeVoxel() == ARGBCOLOR) // Целый блок.
                     {
                         float d = sqrtf((float)(x * x + y * y + z * z));
                         if(d <= r) isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, cell, new ARGBColorVoxel(lit(((ARGBColorVoxel*)obj)->getARGBColor(), d, r)));
                         continue;
                     }
                     if(obj->getTypeVoxel() != CHUNK_VOXEL_OBJECT) continue;
                     ChunkVoxelObject* nested = (ChunkVoxelObject*)obj; // Разбитый блок: каждый маленький блок отдельно.
                     const int n = nested->getNumberOfPartitions();
                     for(int i = 0; i < n; i++)
                         for(int j = 0; j < n; j++)
                             for(int k = 0; k < n; k++)
                             {
                                 SimpleVoxelObject* piece = nested->getSimpleVoxelObject(Vector3d<int>(i, j, k));
                                 if(!piece || piece->getTypeVoxel() != ARGBCOLOR) continue;
                                 const float dx = x - 0.5f + (i + 0.5f) / n, dy = y - 0.5f + (j + 0.5f) / n, dz = z - 0.5f + (k + 0.5f) / n;
                                 const float d = sqrtf(dx * dx + dy * dy + dz * dz);
                                 if(d > r) continue;
                                 VoxelAddress* addr = createAddressOfBlock(isve, cell); // Чанк и блок в чанке...
                                 addr->addNextVector(Vector3d<int>(i, j, k)); // ...и маленький блок во вложенном чанке.
                                 isve->setSimpleVoxelObject(p->s_WorldId, addr, new ARGBColorVoxel(lit(((ARGBColorVoxel*)piece)->getARGBColor(), d, r)), true);
                             }
                 }
     }
     // Цвет c, подсвеченный жёлтым: в центре света - сильнее, на расстоянии r - никак.
     static ARGBColor lit(ARGBColor c, float d, int r)
     {
         const float power = r > 0 ? 1 - d / r : 1;
         return ARGBColor(c.getA(), (unsigned char)(c.getR() + (255 - c.getR()) * power), (unsigned char)(c.getG() + (255 - c.getG()) * power), c.getB());
     }
 };

 // Обработчик: левая кнопка - сломать блок (или маленький блок) под курсором, правая - поставить маленький блок,
 // B - взрыв, L - свет в блоке под курсором. Точку под курсором каждый кадр заполняет главный цикл.
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver(IrrSimpleVoxelEngine* isve, int id) : s_ISVE(isve), s_WorldId(id) {}
     bool OnEvent(const SEvent& event)
     {
         if(!s_HasTarget) return false;
         if(event.EventType == EET_MOUSE_INPUT_EVENT)
         {
             if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) breakUnderCursor();
             if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) placeSmallBlock();
         }
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_B) addTask(BOOM, 2);
             if(event.KeyInput.Key == KEY_KEY_L) addTask(LIGHT, 3);
         }
         return false;
     }
     // Сломать то, что под курсором: точку чуть сдвигаем от камеры - внутрь блока. На любой глубине вложенных чанков:
     // адрес из двух векторов - целый блок, из трёх - маленький блок.
     bool breakUnderCursor()
     {
         VoxelAddress* addr = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look, s_CamPos);
         if(!addr) return false;
         if(addr->getLastVectInd() < 2) { delete addr; return false; } // Чанк не загружен.
         s_ISVE->deleteSimpleVoxelObject(s_WorldId, addr); // Адрес забирает движок.
         s_ISVE->addTaskUpdateChunks();
         return true;
     }
     // Поставить маленький блок ровно под курсором: точку сдвигаем к камере - в пустую ячейку перед гранью.
     bool placeSmallBlock()
     {
         VoxelAddress* target = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look * (-1), s_CamPos);
         if(!target) return false;
         bool empty = false;
         if(target->getLastVectInd() >= 2)
         {
             std::lock_guard<std::recursive_mutex> lock(s_ISVE->getLockAccessToTheSimpleVoxelEngine()); // Читаем мир напрямую.
             World* wrld = s_ISVE->getWorld(s_WorldId);
             empty = wrld && !wrld->getSimpleVoxelObject(target, false);
         }
         if(!empty) { delete target; return false; }
         if(target->getLastVectInd() >= 3) // Пустая маленькая ячейка уже разбитого блока - ставим маленький блок.
         {
             s_ISVE->setSimpleVoxelObject(s_WorldId, target, new ARGBColorVoxel(ARGBColor(255, 80, 80, 255)));
             s_ISVE->addTaskUpdateChunks();
             return true;
         }
         // Пустая ячейка первого уровня: ставим вложенный чанк с одним маленьким блоком. Какой из PARTS^3 - по той же
         // точке в координатах мира (блок k занимает [k - 0.5, k + 0.5)).
         Vector3d<float> look = s_ISVE->getWorldCoordinates(s_WorldId, s_CamPos) - s_ISVE->getWorldCoordinates(s_WorldId, s_CamPos + s_Look);
         Vector3d<float> w = s_ISVE->getWorldCoordinates(s_WorldId, s_Point) + look.getNormalizeVector() * 0.001f;
         const float q[3] = { w.getX() + 0.5f, w.getY() + 0.5f, w.getZ() + 0.5f };
         int sub[3];
         for(int k = 0; k < 3; k++)
         {
             sub[k] = (int)floorf((q[k] - floorf(q[k])) * PARTS);
             if(sub[k] < 0) sub[k] = 0;
             if(sub[k] >= PARTS) sub[k] = PARTS - 1;
         }
         ChunkVoxelObject* nested = new ChunkVoxelObject(); // Вложенный чанк заполняется до вставки в мир.
         nested->setNumberOfPartitions(PARTS);
         nested->setSimpleVoxelObject(Vector3d<int>(sub[0], sub[1], sub[2]), new ARGBColorVoxel(ARGBColor(255, 80, 80, 255)), true, false);
         s_ISVE->setSimpleVoxelObject(s_WorldId, target, nested); // Адрес и вложенный чанк забирает движок.
         s_ISVE->addTaskUpdateChunks();
         return true;
     }
     // Взрыв или свет - в потоке главной очереди (UserControllerForThread). Центр - блок первого уровня под курсором.
     bool addTask(MY_TASK type, int radius)
     {
         VoxelAddress* addr = s_ISVE->getAddressSimpleVoxelObjectByPoint(s_WorldId, s_Point, s_Look, s_CamPos);
         if(!addr) return false;
         bool good = addr->getLastVectInd() >= 2;
         if(good)
         {
             MyParams* p = new MyParams; // Удалит движок после выполнения.
             p->s_Type = type;
             p->s_WorldId = s_WorldId;
             p->s_Center = addr->getVector(1) + addr->getVector(0) * SIZE_CHUNK; // Чанк и блок в чанке - в глобальные координаты.
             p->s_Radius = radius;
             s_ISVE->addTaskRunUserControllerForThread(p);
         }
         delete addr;
         return good;
     }
     bool s_HasTarget = false; // Смотрит ли камера на блок.
     Vector3d<float> s_Point; // Точка под курсором (координаты сцены).
     Vector3d<float> s_Look; // Направление взгляда.
     Vector3d<float> s_CamPos; // Позиция камеры.
 private:
     IrrSimpleVoxelEngine* s_ISVE;
     int s_WorldId;
 };

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     ISceneManager* smgr = device->getSceneManager();
     Logger* lgr = new Logger; // Логгер: без него движок молча проглатывает и ошибки.
     lgr->setPrintStandard(false); // Обычные сообщения не нужны - только предупреждения и ошибки.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Движок держит свою ссылку.
     isve->setUserControllerForThread(new MyController(isve));
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(SIZE_CHUNK);
     factory->setSScale(SIZE_CHUNK);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     factory->setMultiplePartition(id, true); // Чанки верхнего уровня разбиты одинаково - вложенным чанкам это не мешает.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     isve->getWorld(id)->setWorldGenerator(new GroundGenerator);
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);

     // 1) Разбить блок (8,3,8): на его место - вложенный чанк из 4x4x4 маленьких блоков того же цвета.
     ChunkVoxelObject* split = new ChunkVoxelObject();
     split->setNumberOfPartitions(PARTS);
     for(int i = 0; i < PARTS; i++)
         for(int j = 0; j < PARTS; j++)
             for(int k = 0; k < PARTS; k++)
                 split->setSimpleVoxelObject(Vector3d<int>(i, j, k), new ARGBColorVoxel(ARGBColor(255, 90, 110, 70)), true, false);
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(8, 3, 8), split); // Старый блок удалит движок.

     // 2) Маленький блок по адресу из трёх векторов: чанк (0,0,0), блок (8,3,8) в чанке, маленький блок (3,3,3) в нём.
     VoxelAddress* addr = new VoxelAddress(isve);
     addr->addNextVector(Vector3d<int>(0, 0, 0));
     addr->addNextVector(Vector3d<int>(8, 3, 8));
     addr->addNextVector(Vector3d<int>(3, 3, 3));
     isve->setSimpleVoxelObject(id, addr, new ARGBColorVoxel(ARGBColor(255, 220, 40, 40)), true); // true - заменить старый.

     // 3) Удалить маленький блок (0,3,0) того же разбитого блока - в углу верхнего слоя будет ямка.
     VoxelAddress* hole = createAddressOfBlock(isve, Vector3d<int>(8, 3, 8));
     hole->addNextVector(Vector3d<int>(0, 3, 0));
     isve->deleteSimpleVoxelObject(id, hole);
     isve->addTaskUpdateChunks();

     ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(8, 7, 3));
     camera->setTarget(vector3df(8, 3.5f, 8));
     MyReceiver receiver(isve, id);
     device->setEventReceiver(&receiver);
     ISceneCollisionManager* collision = smgr->getSceneCollisionManager();
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update();
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         smgr->drawAll();
         // Точка под курсором (центр экрана): луч из камеры по селекторам нод чанков.
         vector3df campos = camera->getAbsolutePosition();
         vector3df look = (camera->getTarget() - campos).normalize();
         line3df ray(campos, campos + look * 50.0f);
         vector3df hit;
         triangle3df triangle;
         receiver.s_HasTarget = collision->getSceneNodeAndCollisionPointFromRay(ray, hit, triangle) != 0;
         receiver.s_Point = Vector3d<float>(hit.X, hit.Y, hit.Z);
         receiver.s_Look = Vector3d<float>(look.X, look.Y, look.Z);
         receiver.s_CamPos = Vector3d<float>(campos.X, campos.Y, campos.Z);
         device->getVideoDriver()->endScene();
     }
     isve->stop();
     delete isve;
     device->drop();
     return 0;
 }
 \endcode
*/
