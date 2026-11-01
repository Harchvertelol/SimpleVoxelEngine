/*! \page tutorial8 Урок 8: Изменение мира.
 В этом уроке мы научимся менять мир прямо во время игры: ставить и удалять воксели по адресу и по глобальным
 координатам, находить блок под курсором и правильно перестраивать изменённые чанки. В конце у нас будет пол из
 цветных блоков, на котором левой кнопкой мыши блоки выламываются, правой - ставятся, а клавишами строится и
 прорубается стена.

 Прежде чем писать код, давайте разберёмся, как враппер вообще меняет мир. Все изменения мира выполняет один поток -
 поток главной очереди заданий (подробно о потоках - в уроке 10). Поэтому функции setSimpleVoxelObject(),
 deleteSimpleVoxelObject() и setSimpleVoxelObjectByGlobalVector() работают так:
 - если их вызвать из главного потока (или любого другого, кроме потока очереди), они ничего не меняют сразу, а ставят
   задание в конец главной очереди. Задания выполняются строго по порядку, чуть позже (обычно в пределах кадра);
 - если их вызвать из пользовательской функции UserControllerForThread (она работает в потоке главной очереди),
   изменение выполняется сразу.

 Отсюда первое правило: в главном потоке сразу после setSimpleVoxelObject() воксель ещё не стоит в мире, и
 getSimpleVoxelObjectByGlobalVector() вернёт старое содержимое ячейки. Второе правило: адрес и воксель, переданные в эти
 функции, забирает враппер - не удаляйте их и не пользуйтесь ими после вызова (при ошибке враппер удалит их сам).
 Третье: если к моменту выполнения задания чанк уже выгружен, ничего не меняется (предупреждение в логе, воксель
 удаляется).

 Начнём, как обычно, с подключения враппера. Пространство имён Defines нужно для типов вокселей, Debug - для логгера:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <mutex> // std::lock_guard - для блокировки движка.
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines; // Типы вокселей (ARGBCOLOR).
 using namespace Debug; // Logger.
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;
 \endcode
 Фабрика почти такая же, как на главной странице, но с одним дополнением: чтобы луч из камеры мог найти грань, у каждой
 ноды чанков должен быть селектор треугольников. Враппер сообщает о каждой новой ноде через onUpdateChunkMeshSceneNode()
 (в главном потоке, внутри isve->update()) - там селектор и создадим. Подробно о селекторах, мета-селекторе и
 столкновениях камеры с миром рассказано в уроке 7, здесь нам хватит самого простого варианта: селектор, который
 хранится в самой ноде и удаляется вместе с ней.
 \code
 void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int, Vector3d<int>)
 {
     if(!node) return; // Нода удалена - её селектор удалится вместе с ней.
     ITriangleSelector* selector = s_SceneManager->createTriangleSelector(node->getMesh(), node);
     node->setTriangleSelector(selector); // Подробно о селекторах и столкновениях - в уроке 7.
     selector->drop();
 }
 \endcode
 Обработчик событий только запоминает, что сделать: OnEvent() вызывается в главном потоке (внутри runIrrDevice()), а
 сами действия мы выполним в главном цикле:
 \code
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver() : s_Action(0) {}
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_MOUSE_INPUT_EVENT)
         {
             if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) s_Action = 1; // Удалить блок под курсором.
             if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) s_Action = 2; // Поставить блок на грань под курсором.
         }
         if(event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_W) s_Action = 3; // Построить стену.
             if(event.KeyInput.Key == KEY_KEY_Q) s_Action = 4; // Прорубить в стене проход.
             if(event.KeyInput.Key == KEY_KEY_U) s_Action = 5; // Перестроить все чанки.
         }
         return false;
     }
     int s_Action; // 0 - ничего не делать.
 };
 \endcode

 <b>Адрес вокселя.</b> Воксель в мире задаётся адресом VoxelAddress (подробно - в уроке 4 SimpleVoxelEngine). Это
 цепочка векторов: [0] - координаты чанка в мире, [1] - координаты вокселя внутри чанка (от 0 до размера чанка - 1),
 [2] - координаты внутри вложенного чанка и так далее (вложенные чанки - в уроке 13). У нас вложенных чанков нет, поэтому
 адрес всегда из двух векторов. Адрес создаётся для движка - а враппер и есть движок (IrrSimpleVoxelEngine унаследован от
 Engine), поэтому пишем new VoxelAddress(isve).

 Глобальные координаты блока (те, в которых мы думаем о мире: "блок на x = -3") переводятся в адрес так: чанк - это
 координата, делённая на размер чанка с округлением ВНИЗ, место в чанке - остаток. Для отрицательных координат обычное
 деление в C++ округляет к нулю и даёт не тот чанк: блок -3 лежит в чанке -1 на месте 13 (-3 = -1 * 16 + 13), а не в
 чанке 0. Обратный перевод - чанк * 16 + место:
 \code
 Vector3d<int> getGlobalVector(VoxelAddress* addr)
 {
     return addr->getVector(0) * 16 + addr->getVector(1);
 }
 \endcode
 Поставим по адресу красную колонну из четырёх блоков на глобальных координатах (-3, 1..4, -3):
 \code
 for(int y = 1; y <= 4; y++)
 {
     VoxelAddress* addr = new VoxelAddress(isve); // Адрес создаётся для этого движка.
     addr->addNextVector(Vector3d<int>(-1, 0, -1)); // [0] - координаты чанка.
     addr->addNextVector(Vector3d<int>(13, y, 13)); // [1] - координаты вокселя внутри чанка (от 0 до 15).
     isve->setSimpleVoxelObject(id, addr, new ARGBColorVoxel(ARGBColor(255, 220, 40, 40))); // Адрес и воксель забрал враппер.
 }
 \endcode
 У setSimpleVoxelObject(id, addr, svo, rewrite, immediatelyupdated) два необязательных параметра:
 - rewrite (по умолчанию false) - что делать, если ячейка занята. false - новый воксель не ставится (в логе логическая
   ошибка "This partition already exist", новый воксель враппер удаляет); true - старый воксель удаляется, на его место
   встаёт новый;
 - immediatelyupdated (по умолчанию false) - когда перестраивать чанк. false - чанк (и соседний чанк, если воксель лежит
   на его границе: у соседа могла открыться или закрыться грань) только помечается для обновления, а перестроит его
   addTaskUpdateChunks(); true - чанк сразу, как только задание выполнено, ставится на построение.

 deleteSimpleVoxelObject(id, addr, immediatelyupdated) удаляет воксель и так же помечает (или сразу перестраивает) чанки.
 Если ячейка пуста, ничего не происходит (стандартное сообщение в логе). Адрес тоже забирает враппер.

 Ошибки в адресе враппер проверяет всегда, даже в релизной сборке: вектор за пределами разбиения чанка (например,
 (16, 0, 0) при чанке 16) - логическая ошибка в логе, мир не меняется.

 <b>Глобальные координаты.</b> Адрес удобен, когда он уже есть (например, его вернул поиск блока под курсором). Если же
 мы строим что-то по координатам, проще отдать врапперу сами координаты - он сам найдёт чанк и место в нём (с правильным
 округлением отрицательных координат). Пол 32x32 из четырёх чанков:
 \code
 isve->addChunksForRendering(id, Vector3d<int>(-1, 0, -1), Vector3d<int>(0, 0, 0), true);
 for(int x = -16; x < 16; x++)
     for(int z = -16; z < 16; z++)
         isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new ARGBColorVoxel(ARGBColor(255, 100 + (x & 1) * 60, 140, 100 + (z & 1) * 60)));
 \endcode
 Кстати, генератора мира у нас нет, поэтому при загрузке чанков в логе будут предупреждения "world generator is not
 set. Chunk is empty" - чанки просто создаются пустыми. Это нормально; свой генератор - в уроке 2.

 setSimpleVoxelObjectByGlobalVector() всегда перезаписывает ячейку (как rewrite = true) и никогда не перестраивает чанк
 сразу (как immediatelyupdated = false) - после пачки таких правок нужен addTaskUpdateChunks(). Если вместо вокселя
 передать NULL, воксель в этой ячейке удаляется - отдельной функции удаления по глобальным координатам нет. Параметр
 level пока работает только со значением 1 (блоки первого уровня). Глобальные координаты считаются по размеру чанка мира
 (World::getSizeChunk()): если какой-то чанк разбит иначе (это мог сделать генератор или контроллер загрузки), ставить в
 него по глобальным координатам нельзя - ошибка в логе, воксель удаляется.

 Прочитать воксель по глобальным координатам можно функцией getSimpleVoxelObjectByGlobalVector(id, vect, isexists).
 isexists = true - если ячейка пуста или чанк не загружен, в логе будет сообщение; false - просто вернётся NULL.
 Важно: она возвращает указатель на воксель, который живёт в мире, а мир в это время может менять поток главной очереди.
 Указатель гарантированно действителен только внутри пользовательской функции UserControllerForThread или пока захвачена
 блокировка движка (getLockAccessToTheSimpleVoxelEngine(), подробно - в уроке 10). Ниже мы читаем цвет блока из главного
 потока - поэтому под блокировкой.

 <b>Одна перестройка после пачки правок.</b> Каждый setSimpleVoxelObject() без immediatelyupdated только помечает чанк.
 Перестраивает помеченные чанки задание addTaskUpdateChunks() - оно тоже встаёт в главную очередь, после всех правок,
 поставленных раньше, поэтому увидит их все:
 \code
 isve->addTaskUpdateChunks(); // Одна перестройка изменённых чанков после всех правок.
 \endcode
 Так 1000 правок одного чанка дают одну перестройку, а не 1000. Самая частая ошибка - забыть addTaskUpdateChunks():
 изменения есть в мире, но не видны, пока этот чанк не перестроится по какой-то другой причине (например, при следующей
 правке с addTaskUpdateChunks()). immediatelyupdated = true удобен для одиночной правки (клик мышью) - не нужно ничего
 вызывать после. Для тысяч правок он плох: каждая правка снова ставит чанк на построение, и чанк, который строится прямо
 сейчас, будет построен ещё раз - лишняя работа.

 <b>Блок под курсором.</b> Луч из камеры через центр экрана находит точку на грани (через селекторы нод, которые мы
 создали в фабрике). А дальше нужен адрес блока, которому эта грань принадлежит, - его даёт
 getAddressSimpleVoxelObjectByPoint(id, point, lookvect, changeposition). Точка лежит ровно на грани, поэтому враппер
 сдвигает её на тысячную долю блока вдоль lookvect и смотрит, в какую ячейку она попала. Если lookvect направлен от
 камеры (внутрь блока) - это сам блок, который надо удалить. Если к камере - пустая ячейка перед гранью, куда надо
 поставить новый блок. changeposition - откуда смотрим (позиция камеры). Все точки - в координатах сцены: враппер сам
 пересчитывает их в координаты мира, учитывая родительскую ноду мира и сдвиг (урок 12). Тот же пересчёт доступен и
 отдельно - getWorldCoordinates(id, точка): при стандартных масштабах блок с глобальными координатами k занимает в
 координатах мира отрезок [k - 0.5, k + 0.5), то есть центры блоков - в целых точках.
 \code
 void editBlockUnderCursor(IrrSimpleVoxelEngine* isve, int id, ICameraSceneNode* camera, bool remove)
 {
     ISceneCollisionManager* collision = isve->getIrrlichtDevice()->getSceneManager()->getSceneCollisionManager();
     vector3df start = camera->getAbsolutePosition();
     vector3df dir = (camera->getTarget() - start).normalize();
     vector3df point; // Точка попадания луча в грань.
     triangle3df triangle; // Треугольник грани.
     if(!collision->getSceneNodeAndCollisionPointFromRay(line3df(start, start + dir * 100.f), point, triangle)) return;
     Vector3d<float> hit(point.X, point.Y, point.Z), look(dir.X, dir.Y, dir.Z), campos(start.X, start.Y, start.Z);
     // Блок, на грань которого смотрим: ищем от точки по направлению взгляда (внутрь блока).
     VoxelAddress* target = isve->getAddressSimpleVoxelObjectByPoint(id, hit, look, campos);
     if(!target) return;
     if(target->getLastVectInd() < 2) { delete target; return; } // Чанк не загружен - адрес без вокселя.
 \endcode
 Обратите внимание на проверки: функция может вернуть NULL (ошибка) или адрес без векторов вокселя (чанк в этом месте не
 загружен) - такой адрес в setSimpleVoxelObject() отдавать нельзя, его надо просто удалить. Возвращённый адрес -
 новый объект: либо отдайте его врапперу (set/deleteSimpleVoxelObject), либо удалите сами.

 Есть и вторая функция поиска - getAddressSimpleVoxelObjectByThreePoints(id, v1, v2, v3, lookvect, changeposition):
 ей передают три вершины треугольника, в который попал луч (наш triangle), и она находит ячейку у центра этого
 треугольника (так делает TestProject3). Для обычных кубов результат тот же, но если рядом с большой гранью стоят
 маленькие блоки (вложенные чанки), точное место под курсором находит только getAddressSimpleVoxelObjectByPoint().
 Подробно обе функции и луч - в уроке 7.

 Удаление - одна строка, с immediatelyupdated = true (это одиночная правка):
 \code
     if(remove)
     {
         isve->deleteSimpleVoxelObject(id, target, true); // true - перестроить чанк сразу. Адрес забрал враппер.
         return;
     }
 \endcode
 Для установки сделаем новый блок того же цвета, что и блок, на который смотрим. Цвет читаем из мира - под блокировкой
 движка, потому что движок запущен и поток главной очереди может удалить этот воксель прямо сейчас (например, если
 пользователь кликнул дважды). Захват блокировки через std::lock_guard освобождает её на выходе из блока сам:
 \code
     ARGBColor color(255, 255, 255, 255);
     {
         std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
         SimpleVoxelObject* svo = isve->getSimpleVoxelObjectByGlobalVector(id, getGlobalVector(target), false);
         if(svo && svo->getTypeVoxel() == ARGBCOLOR) color = ((ARGBColorVoxel*)svo)->getARGBColor();
     }
     delete target; // Этот адрес мы никому не отдали - удаляем сами.
     // Пустая ячейка перед гранью: ищем от точки в обратную сторону, к камере.
     VoxelAddress* place = isve->getAddressSimpleVoxelObjectByPoint(id, hit, Vector3d<float>(-dir.X, -dir.Y, -dir.Z), campos);
     if(!place) return;
     if(place->getLastVectInd() < 2) { delete place; return; }
     isve->setSimpleVoxelObject(id, place, new ARGBColorVoxel(color), false, true); // Не перезаписывать, перестроить сразу.
 }
 \endcode
 rewrite = false здесь - защита: если ячейка вдруг занята (например, перед гранью стекла стоит другой блок), ничего не
 перезапишется.

 <b>Главный цикл.</b> Действия выполняем в главном цикле перед isve->update(). Стена 16x8 блоков - пачка правок по
 глобальным координатам и одна перестройка после неё. Проход в стене - та же функция с NULL вместо вокселя:
 \code
 if(receiver.s_Action == 1 || receiver.s_Action == 2) editBlockUnderCursor(isve, id, camera, receiver.s_Action == 1);
 if(receiver.s_Action == 3) // Стена 16x8 блоков у края мира.
 {
     for(int x = -8; x < 8; x++)
         for(int y = 1; y <= 8; y++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, -10), new ARGBColorVoxel(ARGBColor(255, 200, 200, 90)));
     isve->addTaskUpdateChunks(); // Без этого стена появится только при следующей перестройке этих чанков.
 }
 if(receiver.s_Action == 4) // Проход 2x3 в стене: NULL вместо вокселя - удалить.
 {
     for(int x = -1; x <= 0; x++)
         for(int y = 1; y <= 3; y++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, -10), 0);
     isve->addTaskUpdateChunks();
 }
 if(receiver.s_Action == 5) isve->addTaskUpdateChunks(true); // true - перестроить все отрисовываемые чанки.
 receiver.s_Action = 0;
 \endcode
 Стена из 128 блоков - это 128 заданий в очереди, они выполнятся быстро. Но если правок десятки тысяч
 (сфера, взрыв, заливка), ставить их по одной из главного потока не стоит: лучше одним заданием
 addTaskRunUserControllerForThread() выполнить их все в пользовательской функции, где правки идут сразу, без заданий
 (урок 10). И никогда не ставьте правки в цикле без ограничения (например, каждый кадр тысячу новых блоков, не дожидаясь
 выполнения): очередь заданий ничем не ограничена и будет расти, пока не займёт всю память.

 <b>Принудительная перестройка.</b> addTaskUpdateChunks(true) перестраивает все отрисовываемые чанки, а не только
 помеченные. Кроме того, у фабрики есть функции, которые ставят чанки на построение напрямую:
 - updateChunk(id, vect) - перестроить один чанк (false и логическая ошибка в логе, если чанк не отрисовывается);
 - updateChunks(id) и updateChunks() - перестроить помеченные чанки мира id или всех миров (пометки снимаются);
 - updateAllChunks(id) и updateAllChunks() - перестроить все отрисовываемые чанки мира или всех миров;
 - setChunkForUpdate(id, vect) - пометить чанк для обновления (false, если чанк не отрисовывается или уже помечен);
 - isForUpdateChunk(id, vect) - помечен ли чанк.

 Важное отличие: эти функции фабрики выполняются сразу, в вызывающем потоке, а не через главную очередь. Поэтому сразу
 после setSimpleVoxelObject() из главного потока factory->updateChunks() бесполезен: правка ещё стоит в очереди, чанк
 ещё не помечен, и перестраивать нечего. Из главного потока после правок вызывайте isve->addTaskUpdateChunks() - оно
 встанет в очередь после них. Функции фабрики нужны в других случаях:
 - внутри пользовательской функции UserControllerForThread, где правки уже выполнены;
 - когда мир не менялся, а изменилось то, как фабрика рисует воксели (например, getMeshVoxel() стал брать другие
   текстуры) - тогда updateAllChunks();
 - когда вы изменили мир напрямую через ядро, в обход враппера. Враппер о такой правке не знает, поэтому чанк нужно
   пометить самим (и соседний, если воксель на границе чанка) и перестроить:
 \code
 {
     std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
     World* world = isve->getWorld(id);
     VoxelAddress addr(isve);
     addr.addNextVector(Vector3d<int>(0, 0, 0));
     addr.addNextVector(Vector3d<int>(5, 1, 5));
     ARGBColorVoxel* vxl = new ARGBColorVoxel(ARGBColor(255, 0, 0, 255));
     if(!world->setSimpleVoxelObject(&addr, vxl, true)) delete vxl; // При неудаче воксель остаётся нашим.
     factory->setChunkForUpdate(id, Vector3d<int>(0, 0, 0)); // Под той же блокировкой; воксель не на границе - соседей не помечаем.
 }
 factory->updateChunks(id);
 \endcode
 Прямая работа с World во время работы движка - только под блокировкой движка (потоки построения в это время читают
 мир). Пометку setChunkForUpdate() делайте под той же блокировкой: иначе между изменением и пометкой поток главной
 очереди может убрать чанк из отрисовки (например, стриминг), и пометка закончится нарушением логики (подробнее - в
 уроке 10). Обычно проще и безопаснее пользоваться функциями враппера.

 Камера в этом уроке - FPS-камера с небольшой скоростью (10 блоков в секунду), курсор скрыт, а "курсор" - центр
 экрана, куда смотрит камера. Мышь крутит камеру, стрелки двигают, левая кнопка выламывает блок, правая ставит,
 W строит стену, Q прорубает в ней проход, U перестраивает все чанки.

 <b>Типичные ошибки:</b>
 - забыли addTaskUpdateChunks() после пачки правок - изменения не видны;
 - сразу после setSimpleVoxelObject() в главном потоке прочитали воксель и удивились, что его нет - правка ещё в очереди;
 - удалили адрес или воксель после передачи врапперу (двойное удаление) или, наоборот, не удалили адрес, который
   вернул поиск и который никуда не отдали (утечка);
 - отдали в setSimpleVoxelObject() адрес без вектора вокселя (поиск в незагруженном чанке);
 - сохранили указатель от getSimpleVoxelObjectByGlobalVector() и пользовались им позже без блокировки движка;
 - ставили правки каждый кадр без ограничения - очередь растёт без предела.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <mutex> // std::lock_guard - для блокировки движка.
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines; // Типы вокселей (ARGBCOLOR).
 using namespace Debug; // Logger.
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Фабрика - как на главной странице, плюс селектор треугольников у каждой ноды (чтобы луч из камеры находил грани).
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh();
         s_SceneManager = device->getSceneManager();
         s_MeshManipulator = s_SceneManager->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl) // Потоки построения.
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         if(mesh) s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     // Главный поток (внутри isve->update()): нода блока чанков создана (node != 0) или удалена (node == 0).
     void onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int, Vector3d<int>)
     {
         if(!node) return; // Нода удалена - её селектор удалится вместе с ней.
         ITriangleSelector* selector = s_SceneManager->createTriangleSelector(node->getMesh(), node);
         node->setTriangleSelector(selector); // Подробно о селекторах и столкновениях - в уроке 7.
         selector->drop();
     }
     IMesh* s_Plane;
     ISceneManager* s_SceneManager;
     IMeshManipulator* s_MeshManipulator;
 };

 // Обработчик событий: только запоминает, что сделать. Вызывается в главном потоке (внутри runIrrDevice()).
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver() : s_Action(0) {}
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_MOUSE_INPUT_EVENT)
         {
             if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) s_Action = 1; // Удалить блок под курсором.
             if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) s_Action = 2; // Поставить блок на грань под курсором.
         }
         if(event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown)
         {
             if(event.KeyInput.Key == KEY_KEY_W) s_Action = 3; // Построить стену.
             if(event.KeyInput.Key == KEY_KEY_Q) s_Action = 4; // Прорубить в стене проход.
             if(event.KeyInput.Key == KEY_KEY_U) s_Action = 5; // Перестроить все чанки.
         }
         return false;
     }
     int s_Action; // 0 - ничего не делать.
 };

 // Глобальные координаты блока по его адресу первого уровня: чанк * 16 + место в чанке.
 Vector3d<int> getGlobalVector(VoxelAddress* addr)
 {
     return addr->getVector(0) * 16 + addr->getVector(1);
 }

 // Удалить блок под центром экрана (remove == true) или поставить новый блок на грань, в которую смотрим.
 void editBlockUnderCursor(IrrSimpleVoxelEngine* isve, int id, ICameraSceneNode* camera, bool remove)
 {
     ISceneCollisionManager* collision = isve->getIrrlichtDevice()->getSceneManager()->getSceneCollisionManager();
     vector3df start = camera->getAbsolutePosition();
     vector3df dir = (camera->getTarget() - start).normalize();
     vector3df point; // Точка попадания луча в грань.
     triangle3df triangle; // Треугольник грани.
     if(!collision->getSceneNodeAndCollisionPointFromRay(line3df(start, start + dir * 100.f), point, triangle)) return;
     Vector3d<float> hit(point.X, point.Y, point.Z), look(dir.X, dir.Y, dir.Z), campos(start.X, start.Y, start.Z);
     // Блок, на грань которого смотрим: ищем от точки по направлению взгляда (внутрь блока).
     VoxelAddress* target = isve->getAddressSimpleVoxelObjectByPoint(id, hit, look, campos);
     if(!target) return;
     if(target->getLastVectInd() < 2) { delete target; return; } // Чанк не загружен - адрес без вокселя.
     if(remove)
     {
         isve->deleteSimpleVoxelObject(id, target, true); // true - перестроить чанк сразу. Адрес забрал враппер.
         return;
     }
     // Цвет нового блока - как у того, на который смотрим. Движок запущен, поэтому указатель на воксель
     // читаем под блокировкой движка: иначе поток главной очереди может удалить этот воксель прямо сейчас.
     ARGBColor color(255, 255, 255, 255);
     {
         std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
         SimpleVoxelObject* svo = isve->getSimpleVoxelObjectByGlobalVector(id, getGlobalVector(target), false);
         if(svo && svo->getTypeVoxel() == ARGBCOLOR) color = ((ARGBColorVoxel*)svo)->getARGBColor();
     }
     delete target; // Этот адрес мы никому не отдали - удаляем сами.
     // Пустая ячейка перед гранью: ищем от точки в обратную сторону, к камере.
     VoxelAddress* place = isve->getAddressSimpleVoxelObjectByPoint(id, hit, Vector3d<float>(-dir.X, -dir.Y, -dir.Z), campos);
     if(!place) return;
     if(place->getLastVectInd() < 2) { delete place; return; }
     isve->setSimpleVoxelObject(id, place, new ARGBColorVoxel(color), false, true); // Не перезаписывать, перестроить сразу.
 }

 int main()
 {
     MyReceiver receiver;
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600), 32, false, false, false, &receiver);
     Logger* lgr = new Logger; // Логгер: ошибки и предупреждения враппера видны в консоли.
     lgr->setPrintStandard(false); // Стандартные сообщения (их много) не выводим.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop();
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld(); // Чанк 16x16x16.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     // Четыре чанка: от (-1,0,-1) до (0,0,0). Блоки в них - с глобальными координатами от -16 до 15 по X и Z.
     isve->addChunksForRendering(id, Vector3d<int>(-1, 0, -1), Vector3d<int>(0, 0, 0), true);
     // Пол 32x32 - по глобальным координатам: враппер сам найдёт чанк и место в нём.
     for(int x = -16; x < 16; x++)
         for(int z = -16; z < 16; z++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new ARGBColorVoxel(ARGBColor(255, 100 + (x & 1) * 60, 140, 100 + (z & 1) * 60)));
     // Красная колонна - по адресу. Блок (-3, y, -3) лежит в чанке (-1, 0, -1) на месте (13, y, 13): -3 = -1 * 16 + 13.
     for(int y = 1; y <= 4; y++)
     {
         VoxelAddress* addr = new VoxelAddress(isve); // Адрес создаётся для этого движка.
         addr->addNextVector(Vector3d<int>(-1, 0, -1)); // [0] - координаты чанка.
         addr->addNextVector(Vector3d<int>(13, y, 13)); // [1] - координаты вокселя внутри чанка (от 0 до 15).
         isve->setSimpleVoxelObject(id, addr, new ARGBColorVoxel(ARGBColor(255, 220, 40, 40))); // Адрес и воксель забрал враппер.
     }
     isve->addTaskUpdateChunks(); // Одна перестройка изменённых чанков после всех правок.
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS(0, 100.f, 0.01f); // Скорость - 10 блоков в секунду.
     camera->setPosition(vector3df(2, 6, -12));
     camera->setTarget(vector3df(2, 0, 0));
     device->getCursorControl()->setVisible(false);
     isve->start();
     while(isve->runIrrDevice())
     {
         if(receiver.s_Action == 1 || receiver.s_Action == 2) editBlockUnderCursor(isve, id, camera, receiver.s_Action == 1);
         if(receiver.s_Action == 3) // Стена 16x8 блоков у края мира.
         {
             for(int x = -8; x < 8; x++)
                 for(int y = 1; y <= 8; y++)
                     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, -10), new ARGBColorVoxel(ARGBColor(255, 200, 200, 90)));
             isve->addTaskUpdateChunks(); // Без этого стена появится только при следующей перестройке этих чанков.
         }
         if(receiver.s_Action == 4) // Проход 2x3 в стене: NULL вместо вокселя - удалить.
         {
             for(int x = -1; x <= 0; x++)
                 for(int y = 1; y <= 3; y++)
                     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, y, -10), 0);
             isve->addTaskUpdateChunks();
         }
         if(receiver.s_Action == 5) isve->addTaskUpdateChunks(true); // true - перестроить все отрисовываемые чанки.
         receiver.s_Action = 0;
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
