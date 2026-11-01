/*! \page tutorial12 Урок 12: Положение мира в сцене.
 До сих пор наш мир стоял в начале координат сцены: блок с глобальными координатами (x, y, z) рисовался ровно в
 точке (x, y, z) сцены (при масштабах фабрики, равных размеру чанка). Но мир можно поставить куда угодно, повернуть,
 уменьшить, двигать прямо во время игры - и даже показывать несколько миров сразу: например, основной мир и летающий
 остров, корабль или вагон, который едет по своему пути. В этом уроке мы разберём:
 - структуру World3DInfo: родительскую ноду мира и сдвиг его начала координат;
 - как двигать, поворачивать и масштабировать мир через родительскую ноду;
 - как пересчитать точку сцены в координаты мира - getWorldCoordinates() (в главном потоке и в других);
 - как поменять положение мира после создания - changeWorld3DInfo() и getWorld3DInfo();
 - несколько миров одновременно.

 Подключения и фабрика - как в уроке 1 (одна фабрика рисует все миры движка):
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <cstdio>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;
 \endcode
 \code
 // Фабрика - как в уроке 1: куб из видимых граней, цвет вершин - цвет вокселя. Одна фабрика рисует все миры.
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
 Обработчик клавиш: P - поставить блок-метку в мир 2 под камерой, M - поднять мир 1 на два блока.
 \code
 // Обработчик клавиш: P - поставить блок-метку в мир 2 под камерой, M - поднять мир 1 на два блока (changeWorld3DInfo()).
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_P) s_Mark = true;
             if(event.KeyInput.Key == KEY_KEY_M) s_Move = true;
         }
         return false;
     }
     bool s_Mark = false;
     bool s_Move = false;
 };
 \endcode
 И генератор пола - у каждого мира свой генератор со своим цветом:
 \code
 // Генератор: в чанке (0,0,0) - пол 16x16 из вокселей одного цвета (у каждого мира - свой генератор и свой цвет).
 class FloorGenerator : public WorldGenerator
 {
 public:
     FloorGenerator(ARGBColor color) : s_Color(color) {}
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         if(!(chunk->getVoxelAddress()->getVector(0) == Vector3d<int>(0, 0, 0))) return true; // Остальные чанки - пустые.
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
                 chunk->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new ARGBColorVoxel(s_Color), true, false);
         return true;
     }
 private:
     ARGBColor s_Color;
 };
 \endcode
 \section t12info World3DInfo: родительская нода и сдвиг
 Положение мира задаёт структура World3DInfo из двух полей:
 - s_WorldRootNode - родительская нода мира. Все ноды чанков мира враппер делает её детьми, поэтому всё, что вы делаете
   с этой нодой (сдвиг, поворот, масштаб, привязка к другой ноде), делается со всем миром сразу. 0 (по умолчанию) -
   ноды чанков враппер вешает прямо на корень сцены;
 - s_AmendmentPositionWorld - сдвиг начала координат мира относительно родительской ноды (в её локальных координатах).
   Ноды чанков ставятся со сдвигом на этот вектор - удобно, например, чтобы мир вращался вокруг своего центра, а не
   вокруг угла.

 Важно помнить, где лежат блоки: блок с глобальными координатами k занимает в координатах мира отрезок от k - 0.5 до
 k + 0.5 (центр блока - в целой точке). Пол 16x16 из блоков 0..15 занимает от -0.5 до 15.5, его центр - 7.5. Поэтому
 сдвиг (-7.5, 0, -7.5) ставит центр пола ровно под родительскую ноду.

 World3DInfo передаётся в addWorld() - у всех трёх её вариантов это последний параметр. Начнём main() как обычно:
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
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
 \endcode
 Мир 1 - родительская нода в начале координат, центр пола - под ней:
 \code
 // Мир 1: родительская нода в начале координат, начало координат мира сдвинуто так, что центр пола - под нодой.
 ISceneNode* root1 = smgr->addEmptySceneNode();
 World3DInfo info1;
 info1.s_WorldRootNode = root1;
 info1.s_AmendmentPositionWorld = vector3df(-7.5f, 0, -7.5f);
 int id1 = isve->addWorld(info1);
 \endcode
 Мир 2 - "летающий остров": его нода сдвинута, повёрнута на 30 градусов и уменьшена вдвое (блоки острова - по
 половине единицы сцены). Этот мир создадим через addWorld() с параметрами мира WorldCreatingParameters (урок 2
 SimpleVoxelEngine), а World3DInfo передадим вторым параметром:
 \code
 // Мир 2: "летающий остров" - нода сдвинута, повёрнута и уменьшена вдвое; мир создаётся со своими параметрами.
 ISceneNode* root2 = smgr->addEmptySceneNode();
 root2->setPosition(vector3df(30, 5, 0));
 root2->setRotation(vector3df(0, 30, 0));
 root2->setScale(vector3df(0.5f, 0.5f, 0.5f));
 World3DInfo info2;
 info2.s_WorldRootNode = root2;
 info2.s_AmendmentPositionWorld = vector3df(-7.5f, 0, -7.5f); // Вращаться остров будет вокруг центра пола.
 WorldCreatingParameters wcp;
 wcp.s_SizeChunk = 16;
 int id2 = isve->addWorld(wcp, info2);
 \endcode
 Пока нода задана миру, фабрика держит на неё ссылку (grab()), поэтому, если удалить ноду из сцены (remove()),
 ничего не упадёт - мир просто перестанет быть виден. Сами ноды чанков принадлежат врапперу: их не удаляйте и не
 перевешивайте на другого родителя - двигайте родительскую ноду.

 \section t12many Несколько миров
 У движка одна фабрика, и она рисует все миры. Но почти все настройки фабрики задаются для каждого мира отдельно - у
 таких функций есть параметр "идентификатор мира": setMaterialType(), setMaterialFlag(), setID(), setHardwareMappingHint(),
 setSizeBlockChunks(), setMultiplePartition(), updateCameraPosition(), changeWorld3DInfo(). Генератор, контроллер
 сохранения и правило прозрачности - тоже у каждого мира свои (их задают через World):
 \code
 // Настройки фабрики для каждого мира - свои (номер уровня, номер мира, ...).
 factory->setMaterialFlag(0, id1, EMF_LIGHTING, false);
 factory->setMaterialFlag(0, id2, EMF_LIGHTING, false);
 isve->getWorld(id1)->setWorldGenerator(new FloorGenerator(ARGBColor(255, 60, 160, 60)));
 isve->getWorld(id2)->setWorldGenerator(new FloorGenerator(ARGBColor(255, 160, 120, 60)));
 isve->addChunksForRendering(id1, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
 isve->addChunksForRendering(id2, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
 \endcode
 Общими для всех миров остаются число уровней мешей (setNumberOfMeshesSceneNodeForChunk()) и масштабы
 (setPositionScale(), setSScale()). Из-за общих масштабов держите у всех миров одинаковый размер чанка, равный
 масштабам: ноды чанков враппер ставит с шагом в размер чанка мира, а воксели внутри - по масштабам фабрики, и мир с
 другим размером чанка рассыплется на перекрывающиеся или разъехавшиеся чанки. Нужен мир с другим размером блоков -
 масштабируйте его родительскую ноду, как наш остров.

 Камера и запуск:
 \code
 ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
 camera->setPosition(vector3df(30, 12, -2));
 camera->setTarget(vector3df(15, 0, 0));
 isve->start();
 \endcode
 \section t12move Движение мира
 Двигать мир - значит двигать его родительскую ноду. Остров вращается каждый кадр:
 \code
 while(isve->runIrrDevice())
 {
     root2->setRotation(root2->getRotation() + vector3df(0, 0.2f, 0)); // Остров вращается - вместе со всеми нодами его чанков.
 \endcode
 Ноды чанков - дети root2, поэтому вращаются вместе с ней, и враппер при этом ничего не перестраивает: меши чанков
 заданы в локальных координатах родителя. Так же работают setPosition() и setScale(), а можно и привязать родительскую
 ноду мира к другой ноде (setParent()) - например, к движущемуся кораблю. Трогать ноды сцены можно только в главном
 потоке - как и всё в Irrlicht.

 Поиск блока под курсором (getAddressSimpleVoxelObjectByThreePoints() и getAddressSimpleVoxelObjectByPoint(), уроки 7
 и 8) принимает точки в координатах сцены и сам учитывает положение, поворот и масштаб родительской ноды (с её
 родителями) и сдвиг - так что щелчок по повёрнутому острову попадёт в нужный блок без всяких пересчётов с вашей
 стороны. В TestProject3 мир можно двигать и вращать клавишами U, I, J, K, Y, A, O и проверить, что установка и
 удаление блоков работают в любом положении мира.

 \section t12coords Координаты сцены и координаты мира
 А вот если вам самим нужны координаты мира - на каком блоке стоит игрок, сколько блоков до точки взрыва, - есть
 getWorldCoordinates(id, точка сцены). Она пересчитывает точку сцены в координаты мира id с учётом родительской ноды
 (вместе с её родителями) и сдвига, в том числе у мира без родительской ноды. По P узнаем, над каким блоком острова
 висит камера, и поставим туда красный блок-метку:
 \code
 if(receiver.s_Mark)
 {
     // Какой блок мира 2 под камерой? Пересчитываем позицию камеры в координаты мира 2 (здесь, в главном потоке,
     // - по текущему положению ноды). Блок с координатами k занимает от k - 0.5 до k + 0.5.
     receiver.s_Mark = false;
     vector3df cam = camera->getAbsolutePosition();
     Vector3d<float> w = isve->getWorldCoordinates(id2, Vector3d<float>(cam.X, cam.Y, cam.Z));
     Vector3d<int> block((int)floorf(w.getX() + 0.5f), 1, (int)floorf(w.getZ() + 0.5f)); // Над полом (y = 1).
     printf("camera in world 2: (%.2f, %.2f, %.2f), mark block (%d, %d, %d)\n", w.getX(), w.getY(), w.getZ(), block.getX(), block.getY(), block.getZ());
     if(block.getX() >= 0 && block.getX() < 16 && block.getZ() >= 0 && block.getZ() < 16) // Только внутри загруженного чанка.
     {
         isve->setSimpleVoxelObjectByGlobalVector(id2, block, new ARGBColorVoxel(ARGBColor(255, 255, 0, 0)));
         isve->addTaskUpdateChunks();
     }
 }
 \endcode
 Блок k занимает от k - 0.5 до k + 0.5, поэтому номер блока - floorf(координата + 0.5) (не просто (int): приведение
 к int округляет к нулю, и у отрицательных координат номер был бы неверным). Метку ставим, только если блок внутри
 загруженного чанка: за его пределами чанка нет, и враппер такую установку не выполнит (предупреждение в логе).
 Направление пересчитывается как разность двух пересчитанных точек:
 \code
 Vector3d<float> look = isve->getWorldCoordinates(id, campos + lookvect) - isve->getWorldCoordinates(id, campos);
 \endcode

 <b>getWorldCoordinates() и потоки.</b> Функция потокобезопасна, но считает по-разному:
 - в главном потоке - по текущему положению нод (даже если вы сдвинули ноду в этом же кадре);
 - в остальных потоках (пользовательская функция UserControllerForThread, свой поток) - по положению, которое
   запомнил последний update() (а также addWorld() и changeWorld3DInfo()). Ноды сцены меняет главный поток, и читать их
   из других потоков нельзя, поэтому враппер раз в кадр запоминает их положение.

 Отсюда правило: если мир движется, пересчитывайте координаты в главном потоке в момент события (щелчка, нажатия),
 а в другие потоки передавайте уже координаты мира. Иначе поток очереди выполнит задание через несколько кадров, когда
 остров уже повернётся, и действие попадёт не туда. Для неподвижного мира разницы нет.

 \section t12change Смена положения после создания
 По M поднимем мир 1 на два блока. Положение мира после создания меняет changeWorld3DInfo(), текущее можно узнать
 через getWorld3DInfo():
 \code
 if(receiver.s_Move)
 {
     // Поднять мир 1 на два блока: меняем сдвиг его начала координат. Ноды чанков пересклеятся в ближайшем update().
     receiver.s_Move = false;
     World3DInfo info = factory->getWorld3DInfo(id1);
     info.s_AmendmentPositionWorld.Y += 2;
     factory->changeWorld3DInfo(id1, info);
 }
 \endcode
 Что важно знать про changeWorld3DInfo():
 - вызывать только из главного потока;
 - ноды чанков мира пересклеиваются в ближайшем update() - с новой родительской нодой и новым сдвигом. Это новые
   ноды: onUpdateChunkMeshSceneNode() получит их, так что селекторы столкновений (урок 7) обновятся сами, если вы
   обрабатываете их так, как в уроке 7;
 - getWorldCoordinates() учитывает новые данные сразу, не дожидаясь update();
 - s_WorldRootNode = 0 отвязывает мир от родительской ноды (фабрика отпускает свою ссылку на неё);
 - функция возвращает false, если мира с таким идентификатором нет.

 getWorld3DInfo() можно вызывать из любого потока, но полученная нода - объект сцены: трогать её можно только в
 главном потоке. Для простого движения мира changeWorld3DInfo() не нужна - достаточно двигать саму родительскую ноду
 (это и дешевле: ничего не пересклеивается).

 Остаток цикла - обычный: update() каждый кадр (заодно запоминает положение родительских нод для других потоков) и
 рисование:
 \code
     isve->update(); // Заодно запоминает положение родительских нод - для getWorldCoordinates() из других потоков.
     device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
     smgr->drawAll();
     device->getVideoDriver()->endScene();
 }
 isve->stop();
 delete isve;
 device->drop();
 return 0;
 \endcode
 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <cmath>
 #include <cstdio>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Фабрика - как в уроке 1: куб из видимых граней, цвет вершин - цвет вокселя. Одна фабрика рисует все миры.
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

 // Обработчик клавиш: P - поставить блок-метку в мир 2 под камерой, M - поднять мир 1 на два блока (changeWorld3DInfo()).
 class MyReceiver : public IEventReceiver
 {
 public:
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_P) s_Mark = true;
             if(event.KeyInput.Key == KEY_KEY_M) s_Move = true;
         }
         return false;
     }
     bool s_Mark = false;
     bool s_Move = false;
 };

 // Генератор: в чанке (0,0,0) - пол 16x16 из вокселей одного цвета (у каждого мира - свой генератор и свой цвет).
 class FloorGenerator : public WorldGenerator
 {
 public:
     FloorGenerator(ARGBColor color) : s_Color(color) {}
     bool generateChunk(ChunkVoxelObject* chunk, int size)
     {
         if(!(chunk->getVoxelAddress()->getVector(0) == Vector3d<int>(0, 0, 0))) return true; // Остальные чанки - пустые.
         for(int x = 0; x < size; x++)
             for(int z = 0; z < size; z++)
                 chunk->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new ARGBColorVoxel(s_Color), true, false);
         return true;
     }
 private:
     ARGBColor s_Color;
 };

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
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);

     // Мир 1: родительская нода в начале координат, начало координат мира сдвинуто так, что центр пола - под нодой.
     ISceneNode* root1 = smgr->addEmptySceneNode();
     World3DInfo info1;
     info1.s_WorldRootNode = root1;
     info1.s_AmendmentPositionWorld = vector3df(-7.5f, 0, -7.5f);
     int id1 = isve->addWorld(info1);

     // Мир 2: "летающий остров" - нода сдвинута, повёрнута и уменьшена вдвое; мир создаётся со своими параметрами.
     ISceneNode* root2 = smgr->addEmptySceneNode();
     root2->setPosition(vector3df(30, 5, 0));
     root2->setRotation(vector3df(0, 30, 0));
     root2->setScale(vector3df(0.5f, 0.5f, 0.5f));
     World3DInfo info2;
     info2.s_WorldRootNode = root2;
     info2.s_AmendmentPositionWorld = vector3df(-7.5f, 0, -7.5f); // Вращаться остров будет вокруг центра пола.
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int id2 = isve->addWorld(wcp, info2);

     // Настройки фабрики для каждого мира - свои (номер уровня, номер мира, ...).
     factory->setMaterialFlag(0, id1, EMF_LIGHTING, false);
     factory->setMaterialFlag(0, id2, EMF_LIGHTING, false);
     isve->getWorld(id1)->setWorldGenerator(new FloorGenerator(ARGBColor(255, 60, 160, 60)));
     isve->getWorld(id2)->setWorldGenerator(new FloorGenerator(ARGBColor(255, 160, 120, 60)));
     isve->addChunksForRendering(id1, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     isve->addChunksForRendering(id2, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);

     ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(30, 12, -2));
     camera->setTarget(vector3df(15, 0, 0));
     isve->start();
     while(isve->runIrrDevice())
     {
         root2->setRotation(root2->getRotation() + vector3df(0, 0.2f, 0)); // Остров вращается - вместе со всеми нодами его чанков.
         if(receiver.s_Mark)
         {
             // Какой блок мира 2 под камерой? Пересчитываем позицию камеры в координаты мира 2 (здесь, в главном потоке,
             // - по текущему положению ноды). Блок с координатами k занимает от k - 0.5 до k + 0.5.
             receiver.s_Mark = false;
             vector3df cam = camera->getAbsolutePosition();
             Vector3d<float> w = isve->getWorldCoordinates(id2, Vector3d<float>(cam.X, cam.Y, cam.Z));
             Vector3d<int> block((int)floorf(w.getX() + 0.5f), 1, (int)floorf(w.getZ() + 0.5f)); // Над полом (y = 1).
             printf("camera in world 2: (%.2f, %.2f, %.2f), mark block (%d, %d, %d)\n", w.getX(), w.getY(), w.getZ(), block.getX(), block.getY(), block.getZ());
             if(block.getX() >= 0 && block.getX() < 16 && block.getZ() >= 0 && block.getZ() < 16) // Только внутри загруженного чанка.
             {
                 isve->setSimpleVoxelObjectByGlobalVector(id2, block, new ARGBColorVoxel(ARGBColor(255, 255, 0, 0)));
                 isve->addTaskUpdateChunks();
             }
         }
         if(receiver.s_Move)
         {
             // Поднять мир 1 на два блока: меняем сдвиг его начала координат. Ноды чанков пересклеятся в ближайшем update().
             receiver.s_Move = false;
             World3DInfo info = factory->getWorld3DInfo(id1);
             info.s_AmendmentPositionWorld.Y += 2;
             factory->changeWorld3DInfo(id1, info);
         }
         isve->update(); // Заодно запоминает положение родительских нод - для getWorldCoordinates() из других потоков.
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         smgr->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop();
     delete isve;
     device->drop();
     return 0;
 }
 \endcode
*/
