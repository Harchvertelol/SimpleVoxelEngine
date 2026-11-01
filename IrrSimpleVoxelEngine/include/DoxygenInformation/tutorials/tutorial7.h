/*! \page tutorial7 Урок 7: Столкновения и выбор блоков.
 В этом уроке камера перестанет проходить сквозь блоки и начнёт стоять на полу, а мы научимся узнавать, на какой блок
 она смотрит. Сцена простая: каменный пол, лужа воды и каменный столб.

 \section tut7_idea Как это устроено в Irrlicht
 И столкновения, и выбор лучом в Irrlicht работают через селекторы треугольников (ITriangleSelector): селектор знает
 треугольники меша ноды и умеет быстро найти те, что рядом с точкой или пересекаются с лучом.
 - Аниматор столкновений камеры (createCollisionResponseAnimator()) работает с одним селектором. Нод чанков много,
   поэтому все их селекторы складываются в один мета-селектор (IMetaTriangleSelector).
 - Луч (ISceneCollisionManager::getSceneNodeAndCollisionPointFromRay()) перебирает ноды сцены и проверяет те, у которых
   задан свой селектор (node->setTriangleSelector()) и ID подходит под маску.

 Сложность в том, что ноды чанков создаёт и удаляет враппер, и делает это постоянно: каждая перестройка блока чанков
 (поставили блок, загрузился соседний чанк) создаёт новую ноду, а старую удаляет. Селектор старой ноды надо убрать, а
 для новой - создать. Для этого у фабрики есть onUpdateChunkMeshSceneNode().

 \section tut7_callback onUpdateChunkMeshSceneNode()
 \code
 void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int numbermesh, Vector3d<int> vectblock)
 \endcode
 Враппер вызывает её в главном потоке, внутри isve->update():
 - node != 0 - нода блока чанков vectblock на уровне numbermesh создана заново (или впервые);
 - node == 0 - ноды больше нет (уровень блока стал пустым или чанки убраны из отрисовки).
 Вызывается она и при delete isve - по каждой ноде, с node == 0: так селекторы убираются и при выходе.

 Главное правило: к моменту вызова старая нода этого блока уже удалена. Поэтому селекторы храним не по указателю на
 ноду, а по ключу "уровень + координаты блока чанков", и старую ноду нигде не трогаем: её указатель уже недействителен.
 А селектор Irrlicht хранит указатель на свою ноду без ссылки (grab()) - если оставить его в мета-селекторе, первое же
 столкновение обратится к удалённой ноде, и программа упадёт. Удалять ноды чанков самим тоже нельзя - ими владеет враппер.
 \code
 std::map<Vector3d<int>, ITriangleSelector*> s_Selectors[NUMBER_OF_LEVELS]; // Селектор каждой ноды: [уровень][координаты блока чанков].
 \endcode
 Сама функция: сначала убираем селектор старой ноды, потом, если нода есть, создаём новый.
 \code
 std::map<Vector3d<int>, ITriangleSelector*>::iterator it = s_Selectors[numbermesh].find(vectblock);
 if(it != s_Selectors[numbermesh].end()) // Старой ноды уже нет - убираем её селектор.
 {
     s_Meta->removeTriangleSelector(it->second); // Селектора воды там нет - тогда просто ничего не произойдёт.
     it->second->drop();
     s_Selectors[numbermesh].erase(it);
 }
 if(!node) return;
 ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createOctreeTriangleSelector(node->getMesh(), node);
 if(!selector) return;
 node->setTriangleSelector(selector); // Для луча (getSceneNodeAndCollisionPointFromRay()).
 if(numbermesh == LEVEL_SOLID) s_Meta->addTriangleSelector(selector); // Для столкновений камеры; сквозь воду можно ходить.
 s_Selectors[numbermesh][vectblock] = selector;
 \endcode
 Счётчик ссылок: createOctreeTriangleSelector() даёт селектор со счётчиком 1 (это наша ссылка, её мы и отпускаем
 drop() при удалении), а setTriangleSelector() и addTriangleSelector() берут свои.

 Селектор-октодерево (createOctreeTriangleSelector()) быстрее простого (createTriangleSelector()) на больших мешах, а меш
 блока чанков большой. Создавая его, Irrlicht пишет в лог "Needed Xms to create OctreeTriangleSelector" - на каждую
 новую ноду. Если строки мешают, поднимите уровень лога Irrlicht (device->getLogger()->setLogLevel(ELL_WARNING)) или
 отфильтруйте их в обработчике событий, как это сделано в TestProject3/main.cpp.

 У воды селектор тоже есть (через него можно, например, узнать лучом, смотрит ли камера на воду), но в мета-селектор он
 не идёт - сквозь воду можно ходить. Мета-селектор создаём в main(), до start(), и отдаём фабрике:
 \code
 IMetaTriangleSelector* meta = smgr->createMetaTriangleSelector(); // Все селекторы нод чанков - в одном.
 MyFactory* factory = new MyFactory(device, isve, meta);
 \endcode

 \section tut7_ids ID нод
 ID ноды в Irrlicht - число, и луч сравнивает его с маской побитово: нода проверяется, если ID & маска != 0 (маска 0 -
 проверяются все ноды). Поэтому ID задают битовыми флагами:
 \code
 enum NODE_IDS { ID_PICKABLE = 1, ID_WATER = 2 };
 \endcode
 ID нод чанков задаёт фабрика для уровня мешей мира - и уже созданным нодам, и всем будущим (главный поток, после addWorld()):
 \code
 factory->setID(LEVEL_SOLID, id, ID_PICKABLE);
 factory->setID(LEVEL_WATER, id, ID_WATER); // Луч с маской ID_PICKABLE проходит сквозь воду.
 \endcode
 Луч с маской ID_PICKABLE видит только твёрдые блоки: курсор "проходит" сквозь воду и стекло и находит блок за ними.
 А луч с маской ID_WATER найдёт только воду:
 \code
 collman->getSceneNodeAndCollisionPointFromRay(ray, point, triangle, ID_WATER, 0); // (пример, не из полного кода)
 \endcode
 Кстати, у нод Irrlicht ID по умолчанию -1 - то есть все биты. Если у других нод сцены (модели, биллборды) есть свои
 селекторы, а луч не должен их ловить, дайте им ID без бита ID_PICKABLE (в TestProject3 так сделано у огонька-биллборда).

 Вода у нас полупрозрачная (EMT_TRANSPARENT_VERTEX_ALPHA на своём уровне мешей), а тестер прозрачности говорит
 движку, что вода не закрывает соседние блоки, - иначе верхней грани камня под водой не было бы в меше, и лучу
 нечего было бы найти. Подробно о прозрачности - в уроке 9.

 \section tut7_collision Столкновения камеры
 Камера - обычная FPS-камера Irrlicht. Аниматор столкновений создаём, когда появились первые селекторы (то есть пол):
 \code
 if(!collision && meta->getSelectorCount() > 0) // Пол появился - теперь можно включать столкновения и гравитацию.
 {
     // Эллипсоид камеры 0.3 x 0.8 x 0.3, гравитация вниз, глаза на 0.5 выше центра эллипсоида.
     collision = smgr->createCollisionResponseAnimator(meta, camera, vector3df(0.3f, 0.8f, 0.3f), vector3df(0, -0.1f, 0), vector3df(0, 0.5f, 0));
     camera->addAnimator(collision);
     collision->drop(); // Аниматором владеет камера.
 }
 \endcode
 Почему не сразу? Чанки строятся в фоне, и в первые кадры нод ещё нет. Аниматор с гравитацией, созданный раньше,
 уронил бы камеру сквозь ещё не построенный пол. В большом мире со стримингом ждите isAreaReady() (урок 6) и
 появления нод.

 Параметры аниматора: мета-селектор, камера, радиусы эллипсоида игрока (здесь - 0.6 блока в ширину и 1.6 в высоту),
 гравитация и сдвиг: эллипсоид - тело игрока, а камера (глаза) на 0.5 выше его центра. Гравитация в Irrlicht 1.8
 считается так: скорость падения (в единицах за кадр) растёт на её величину каждую секунду, поэтому значение
 подбирается на глаз.

 Создаётся аниматор один раз: мета-селектор живой, и всё, что фабрика в него добавляет и убирает, аниматор видит сразу.
 Если камеру нужно перенести (телепорт), после camera->setPosition() вызовите collision->setTargetNode(camera) - иначе
 аниматор посчитает перенос движением и "протащит" камеру от старого места через стены.

 \section tut7_ray Луч и блок под курсором
 Луч пускаем каждый кадр после drawAll() (к этому моменту положения нод обновлены):
 \code
 line3df ray; // Луч на 30 блоков из камеры по направлению взгляда.
 ray.start = camera->getPosition();
 const vector3df dir = (camera->getTarget() - ray.start).normalize();
 ray.end = ray.start + dir * 30.0f;
 vector3df point;
 triangle3df triangle;
 if(collman->getSceneNodeAndCollisionPointFromRay(ray, point, triangle, ID_PICKABLE, 0))
 \endcode
 Функция возвращает ноду, в которую попал луч, точку попадания и треугольник - в координатах сцены. Нода чанка нам
 ничего не скажет (в ней сотни блоков), а вот по треугольнику враппер найдёт блок:
 \code
 target = isve->getAddressSimpleVoxelObjectByThreePoints(id,
     Vector3d<float>(triangle.pointA.X, triangle.pointA.Y, triangle.pointA.Z),
     Vector3d<float>(triangle.pointB.X, triangle.pointB.Y, triangle.pointB.Z),
     Vector3d<float>(triangle.pointC.X, triangle.pointC.Y, triangle.pointC.Z),
     Vector3d<float>(dir.X, dir.Y, dir.Z), Vector3d<float>(ray.start.X, ray.start.Y, ray.start.Z));
 if(target && target->getLastVectInd() < 2) { delete target; target = 0; } // Только чанк - блок не найден.
 \endcode
 Параметры: мир, три вершины треугольника, направление взгляда (вектор, "торчащий" в блок) и позиция камеры. Враппер
 сам переводит точки из координат сцены в координаты мира (с учётом родительской ноды мира и сдвига - урок 12), так что
 мир можно двигать и вращать. Функция потокобезопасна.

 Результат - новый VoxelAddress, его нужно удалить самим (или отдать в setSimpleVoxelObject()/deleteSimpleVoxelObject() -
 тогда его забирает движок). Адрес - цепочка векторов: getVector(0) - чанк, getVector(1) - блок в чанке, дальше -
 блоки во вложенных чанках (урок 13). getLastVectInd() - число векторов; если он меньше 2, блок не найден (например,
 чанк не загружен). У нас адрес живёт один кадр: в начале следующего кадра мы его удаляем и ищем заново.

 Если передать направление наоборот (dir * -1), найдётся не сам блок, а соседняя с гранью ячейка - туда ставят новый
 блок. Установка и удаление блоков, а также getAddressSimpleVoxelObjectByPoint() (ячейка точно под точкой попадания,
 нужна для маленьких блоков) - в уроке 8.

 Найденный треугольник для наглядности обводим красным - в координатах сцены, поэтому матрица мира - единичная:
 \code
 driver->setTransform(ETS_WORLD, matrix4());
 driver->setMaterial(wire);
 driver->draw3DTriangle(triangle, SColor(255, 255, 0, 0)); // Треугольник под курсором.
 \endcode
 А в заголовок окна выводим координаты чанка и блока.

 \section tut7_end Завершение
 \code
 if(target) delete target;
 isve->stop();
 delete isve; // Фабрика получит onUpdateChunkMeshSceneNode(0, ...) по каждой ноде - селекторы уберутся из meta.
 \endcode
 Мета-селектор создали мы - мы его и удаляем, но только после delete isve: при удалении движка фабрика ещё убирает из
 него свои селекторы. Устройство Irrlicht - последним.
 \code
 meta->drop(); // Мета-селектор создали мы - мы его и удаляем (камера уже держит свою ссылку через аниматор).
 device->drop();
 \endcode

 \section tut7_errors Типичные ошибки
 - В onUpdateChunkMeshSceneNode() не обработан node == 0 или селектор ищется по указателю ноды - в мета-селекторе
   остаётся селектор удалённой ноды, и программа падает при столкновении (обычно сразу после правки блока).
 - Попытка что-то сделать со старой нодой (remove(), drop(), getTriangleSelector()) - её уже нет.
 - meta->drop() до delete isve - фабрика при удалении движка обратится к удалённому мета-селектору.
 - Аниматор с гравитацией до появления пола - камера проваливается.
 - Луч ловит воду или чужие ноды - не задан ID или маска (у нод по умолчанию ID -1, все биты).
 - Забыли удалить адрес от getAddressSimpleVoxelObjectByThreePoints() - утечка памяти каждый кадр.
 - Перенос камеры без setTargetNode() - камера "протаскивается" сквозь стены или застревает.

 \section tut7_check Как проверить
 Проверка урока (AI/AITests/tutorials/diag_tut7.cpp) работает без окна (EDT_NULL) и ставит камеру сама: у каждой ноды
 чанка есть свой селектор, а в мета-селекторе - только селекторы твёрдых нод; луч сверху находит блок (12, 2, 12),
 сквозь воду - камень (3, 2, 3) под ней, а луч с маской ID_WATER - поверхность воды; после удаления блока нода
 пересоздаётся, селекторы снова совпадают с нодами, и луч находит блок под удалённым; камера упирается в столб и стоит
 на полу; после delete isve в мета-селекторе не остаётся ни одного селектора.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <map>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 enum BLOCKS { BLOCK_STONE = 0, BLOCK_WATER = 1 };
 enum LEVELS { LEVEL_SOLID = 0, LEVEL_WATER = 1, NUMBER_OF_LEVELS = 2 }; // Уровни мешей: твёрдые блоки и вода.
 // ID нод - битовые флаги: луч с маской ID_PICKABLE ищет только ноды, у которых ID & ID_PICKABLE != 0.
 enum NODE_IDS { ID_PICKABLE = 1, ID_WATER = 2 };

 struct BlockInfo : VoxelInfo
 {
     unsigned int s_Id;
 };

 class Block : public SimpleVoxelObject
 {
 public:
     Block(unsigned int id) : s_Id(id) { setTypeVoxel(CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo()
     {
         BlockInfo* info = new BlockInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         return info;
     }
     unsigned int getId() { return s_Id; }
 private:
     unsigned int s_Id;
 };

 // Вода прозрачна для соседей-не-воды: под водой видна (и есть в меше) верхняя грань камня.
 class MyTransparentTester : public SimpleVoxelObjectTransparentTester
 {
 public:
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent)
     {
         if(((Block*)svo)->getId() != BLOCK_WATER) return false;
         return !parent || ((Block*)parent)->getId() != BLOCK_WATER;
     }
 };

 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve, IMetaTriangleSelector* meta) :
         FactoryChunkMeshSceneNode(device, isve),
         s_Meta(meta)
     {
         s_Plane = createFaceMesh();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
     std::map<Vector3d<int>, ITriangleSelector*> s_Selectors[NUMBER_OF_LEVELS]; // Селектор каждой ноды: [уровень][координаты блока чанков].
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         const bool water = ((BlockInfo*)vxl->getVoxelInfo())->s_Id == BLOCK_WATER;
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         if(mesh) s_MeshManipulator->setVertexColors(mesh, water ? SColor(128, 40, 80, 220) : SColor(255, 130, 130, 130));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering* vxl)
     {
         return ((BlockInfo*)vxl->getVoxelInfo())->s_Id == BLOCK_WATER ? LEVEL_WATER : LEVEL_SOLID;
     }
     // Главный поток, внутри isve->update() (и в delete isve): нода блока чанков создана заново (node != 0) или исчезла (node == 0).
     void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int numbermesh, Vector3d<int> vectblock)
     {
         std::map<Vector3d<int>, ITriangleSelector*>::iterator it = s_Selectors[numbermesh].find(vectblock);
         if(it != s_Selectors[numbermesh].end()) // Старой ноды уже нет - убираем её селектор.
         {
             s_Meta->removeTriangleSelector(it->second); // Селектора воды там нет - тогда просто ничего не произойдёт.
             it->second->drop();
             s_Selectors[numbermesh].erase(it);
         }
         if(!node) return;
         ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createOctreeTriangleSelector(node->getMesh(), node);
         if(!selector) return;
         node->setTriangleSelector(selector); // Для луча (getSceneNodeAndCollisionPointFromRay()).
         if(numbermesh == LEVEL_SOLID) s_Meta->addTriangleSelector(selector); // Для столкновений камеры; сквозь воду можно ходить.
         s_Selectors[numbermesh][vectblock] = selector;
     }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
     IMetaTriangleSelector* s_Meta;
 };

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600));
     ISceneManager* smgr = device->getSceneManager();
     IVideoDriver* driver = device->getVideoDriver();
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device);
     IMetaTriangleSelector* meta = smgr->createMetaTriangleSelector(); // Все селекторы нод чанков - в одном.
     MyFactory* factory = new MyFactory(device, isve, meta);
     factory->setNumberOfMeshesSceneNodeForChunk(NUMBER_OF_LEVELS);
     factory->setMaterialType(LEVEL_WATER, EMT_TRANSPARENT_VERTEX_ALPHA); // Вода - полупрозрачная (прозрачность - урок 9).
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MyTransparentTester); // Его удалит мир.
     for(int i = 0; i < NUMBER_OF_LEVELS; i++) factory->setMaterialFlag(i, id, EMF_LIGHTING, false);
     factory->setID(LEVEL_SOLID, id, ID_PICKABLE);
     factory->setID(LEVEL_WATER, id, ID_WATER); // Луч с маской ID_PICKABLE проходит сквозь воду.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++)
         {
             for(int y = 0; y < 3; y++) // Пол толщиной 3 блока.
                 isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, z), new Block(BLOCK_STONE));
             if(x >= 2 && x <= 5 && z >= 2 && z <= 5) // Лужа на полу.
                 isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 3, z), new Block(BLOCK_WATER));
         }
     for(int y = 3; y < 7; y++) // Столб, в который можно упереться.
         isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(10, y, 8), new Block(BLOCK_STONE));
     isve->addTaskUpdateChunks();

     ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS(0, 100, 0.005f);
     camera->setPosition(vector3df(7, 4, 2));
     camera->setTarget(vector3df(10, 3, 8));
     camera->setNearValue(0.1f); // Вплотную к блоку он не "пропадает".
     ISceneNodeAnimatorCollisionResponse* collision = 0; // Аниматор столкновений создадим, когда появится пол.
     ISceneCollisionManager* collman = smgr->getSceneCollisionManager();
     SMaterial wire; // Материал для красного каркаса треугольника под курсором.
     wire.Wireframe = true;
     wire.Lighting = false;
     VoxelAddress* target = 0; // Адрес блока под курсором (наш - удаляем сами).
     isve->start();
     while(isve->runIrrDevice())
     {
         isve->update();
         if(!collision && meta->getSelectorCount() > 0) // Пол появился - теперь можно включать столкновения и гравитацию.
         {
             // Эллипсоид камеры 0.3 x 0.8 x 0.3, гравитация вниз, глаза на 0.5 выше центра эллипсоида.
             collision = smgr->createCollisionResponseAnimator(meta, camera, vector3df(0.3f, 0.8f, 0.3f), vector3df(0, -0.1f, 0), vector3df(0, 0.5f, 0));
             camera->addAnimator(collision);
             collision->drop(); // Аниматором владеет камера.
         }
         driver->beginScene(true, true, SColor(255, 100, 100, 140));
         smgr->drawAll();
         if(target) { delete target; target = 0; }
         line3df ray; // Луч на 30 блоков из камеры по направлению взгляда.
         ray.start = camera->getPosition();
         const vector3df dir = (camera->getTarget() - ray.start).normalize();
         ray.end = ray.start + dir * 30.0f;
         vector3df point;
         triangle3df triangle;
         if(collman->getSceneNodeAndCollisionPointFromRay(ray, point, triangle, ID_PICKABLE, 0))
         {
             // Блок по треугольнику: три вершины, направление взгляда и позиция камеры.
             target = isve->getAddressSimpleVoxelObjectByThreePoints(id,
                 Vector3d<float>(triangle.pointA.X, triangle.pointA.Y, triangle.pointA.Z),
                 Vector3d<float>(triangle.pointB.X, triangle.pointB.Y, triangle.pointB.Z),
                 Vector3d<float>(triangle.pointC.X, triangle.pointC.Y, triangle.pointC.Z),
                 Vector3d<float>(dir.X, dir.Y, dir.Z), Vector3d<float>(ray.start.X, ray.start.Y, ray.start.Z));
             if(target && target->getLastVectInd() < 2) { delete target; target = 0; } // Только чанк - блок не найден.
             if(target)
             {
                 driver->setTransform(ETS_WORLD, matrix4());
                 driver->setMaterial(wire);
                 driver->draw3DTriangle(triangle, SColor(255, 255, 0, 0)); // Треугольник под курсором.
                 const Vector3d<int> c = target->getVector(0), b = target->getVector(1); // Чанк и блок в чанке.
                 stringw caption = L"Chunk (";
                 caption += c.getX(); caption += L", "; caption += c.getY(); caption += L", "; caption += c.getZ();
                 caption += L"), block ("; caption += b.getX(); caption += L", "; caption += b.getY(); caption += L", "; caption += b.getZ();
                 caption += L")";
                 device->setWindowCaption(caption.c_str());
             }
         }
         driver->endScene();
     }
     if(target) delete target;
     isve->stop();
     delete isve; // Фабрика получит onUpdateChunkMeshSceneNode(0, ...) по каждой ноде - селекторы уберутся из meta.
     meta->drop(); // Мета-селектор создали мы - мы его и удаляем (камера уже держит свою ссылку через аниматор).
     device->drop();
     return 0;
 }
 \endcode
*/
