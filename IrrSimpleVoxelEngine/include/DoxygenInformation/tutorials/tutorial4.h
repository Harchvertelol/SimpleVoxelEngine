/*! \page tutorial4 Урок 4: Нестандартные блоки: полублоки, лестницы, растения.
 Мир из одних кубов быстро надоедает. Хочется полублоков, лестниц, травы и цветов. В этом уроке мы сделаем их двумя
 способами. Первый - склейка вокселя из своих мешей функцией getMeshVoxelByGlue(): воксель остаётся одним блоком, но
 рисуется любой формой. Второй - вложенные чанки: блок делится на маленькие кубики (например, 2x2x2), и полублок -
 это просто нижний слой маленьких кубиков. В конце сравним, когда какой способ лучше.

 Но сначала нужно понять, как враппер решает, какие грани рисовать, - для нестандартных блоков это главное.

 \section irrtut4_faces Как считается видимость граней
 Видимость граней считает ядро, и считает её для ячейки целиком. Для каждого вокселя есть шесть флагов - по одному
 на грань, в порядке x+, z+, x-, z-, y+, y-. Грань видна, если хотя бы одна ячейка, которая касается её с другой
 стороны, пустая или прозрачная. Прозрачными считаются:
 - пустая ячейка и незагруженный соседний чанк;
 - вложенный чанк - но ядро не останавливается на нём, а проверяет все его маленькие ячейки, которые касаются грани
   (если все они заняты непрозрачными блоками, грань закрыта);
 - блок, про который так сказал тестер прозрачности мира (SimpleVoxelObjectTransparentTester, подробно - в уроке 4
   SimpleVoxelEngine). Тестер получает соседа (svo) и блок, грань которого проверяется (parent). Без тестера любой блок
   непрозрачен.

 Воксель, у которого не видна ни одна грань, фабрике вообще не передаётся. Флаги видимости фабрика получает в
 VoxelForRendering (isRenderFace()), и getMeshCubeFaces() рисует по ним только видимые грани. (Если у фабрики
 выключена информация о гранях - setIsFaces(false), урок 3, - все шесть флагов равны true, и рисуются все грани.)

 Ядро не знает, что ваш блок - полублок. Для него это просто занятая ячейка. Отсюда две задачи:
 - собственные грани нестандартного блока нужно рисовать так, чтобы флаги видимости им подходили;
 - соседям нужно объяснить, что нестандартный блок не закрывает их грани (иначе будут дыры).

 С первой задачей помогает getMeshVoxelByGlue(), со второй - тестер прозрачности.

 \section irrtut4_glue Как работает getMeshVoxelByGlue()
 getMeshVoxelByGlue(vxl, glues) принимает массив из семи мешей и склеивает из них новый меш:
 - glues[0] приклеивается всегда (может быть NULL - тогда ничего);
 - glues[1]...glues[6] - меши граней в порядке видимости x+, z+, x-, z-, y+, y-: glues[i] приклеивается, только если
   видна грань i - 1 (vxl->isRenderFace(i - 1)). Если грань видна, а меш NULL, - это ошибка: функция вернёт NULL, меш
   чанка не построится.

 Меши приклеиваются "как есть", ничего не сдвигается и не масштабируется. Поэтому каждый меш - это уже готовая часть
 вокселя в его координатах: центр вокселя - начало координат, размер - размер вокселя. При наших масштабах (равных
 размеру чанка) воксель первого уровня имеет размер 1, то есть его меши лежат в кубе от -0.5 до 0.5. На место вокселя
 весь склеенный меш поставит сама фабрика (как и любой меш из getMeshVoxel()). А вот масштаб вокселя склейка не
 применяет: во вложенном чанке воксель меньше 1, и меш нужно уменьшить самим (мы так и сделаем). Мешбуферы
 копируются вместе с материалами, так что у каждой части может быть своя текстура. Возвращается новый меш - его можно
 сразу отдавать врапперу.

 Отсюда правило раскладки: в glues[i] (i от 1 до 6) кладём только то, что лежит на границе вокселя с этой стороны и
 полностью закрыто, если с этой стороны стоит целый непрозрачный блок. Всё остальное - части внутри вокселя или те,
 что сосед закрывает не целиком, - в glues[0]. Если с какой-то стороны на границе ничего нет, кладём туда пустой меш
 (без мешбуферов), а не NULL.

 Посмотрим на полублок (нижняя половина ячейки, по высоте от -0.5 до 0):
 - верх полублока лежит на высоте 0, внутри ячейки. Его никакой сосед не закрывает - он всегда виден: glues[0];
 - четыре боковые стенки высотой в полблока лежат на границе ячейки. Если сбоку стоит целый блок, стенка закрыта:
   glues[1]...glues[4];
 - сверху (y+) на границе ячейки у полублока ничего нет: glues[5] - пустой меш;
 - низ лежит на границе и закрыт блоком снизу: glues[6].

 \section irrtut4_tester Тестер прозрачности
 \code
 // Прозрачность для видимости граней: полублок, лестница и растение не закрывают грани соседей целиком.
 class MyTransparentTester : public SimpleVoxelObjectTransparentTester
 {
 public:
     // svo - сосед (всегда блок: не чанк и не NULL), parent - блок, грань которого проверяется (может быть NULL).
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject*)
     {
         return ((MyBlock*)svo)->getId() != ID_STONE; // Непрозрачен только целый камень.
     }
 };
 \endcode
 Представьте целый камень, рядом с которым стоит полублок. Ядро спрашивает: видна ли грань камня, обращённая к
 полублоку? Без тестера полублок считается непрозрачным, грань камня не рисуется - и над полублоком в камне зияет
 дыра, сквозь которую видно пустоту. Поэтому все неполные блоки мы объявляем прозрачными.

 У такого решения есть цена. Тестер не знает, какая именно грань проверяется, - только кто сосед и чья грань. Поэтому
 он не может сказать "полублок закрывает грань снизу, но не сбоку". В итоге рисуются и лишние грани: верх камня под
 полублоком, стенки между двумя полублоками, стоящими рядом. Их не видно, они только добавляют немного треугольников.
 Лишняя грань лучше дыры. А вот собственные грани полублока считаются правильно: его низ на камне закрыт, потому что
 камень (сосед) непрозрачен.

 parent может быть NULL (когда проверяется грань самого вложенного чанка - вместо чанка передаётся NULL), поэтому если он
 вам нужен (как для стекла в уроке 4 SimpleVoxelEngine), проверяйте его перед использованием. Вложенные чанки тестеру
 не передаются никогда - их ядро проверяет само.

 \section irrtut4_mesh Строим меши частей
 Нам понадобятся несколько помощников. Первый добавляет в мешбуфер квадрат:
 \code
 // Добавляет в мешбуфер квадрат p0-p1-p2-p3 (углы по кругу) лицом в сторону normal.
 void addQuad(SMeshBuffer* mb, vector3df p0, vector3df p1, vector3df p2, vector3df p3, vector3df normal)
 {
     if((p1 - p0).crossProduct(p2 - p0).dotProduct(normal) < 0) { vector3df t = p1; p1 = p3; p3 = t; } // Обход - лицом к normal.
     vector3df p[4] = { p0, p1, p2, p3 };
     u16 first = (u16)mb->Vertices.size();
     for(int i = 0; i < 4; i++)
     {
         // Текстурные координаты - по положению на грани: кусок грани берёт свой кусок текстуры (верх картинки - сверху).
         f32 u = p[i].X + 0.5f, v = p[i].Z + 0.5f;
         if(fabsf(normal.X) > 0.5f) { u = p[i].Z + 0.5f; v = 0.5f - p[i].Y; }
         else if(fabsf(normal.Z) > 0.5f) { v = 0.5f - p[i].Y; }
         mb->Vertices.push_back(S3DVertex(p[i], normal, SColor(255, 255, 255, 255), vector2df(u, v)));
     }
     const u16 indices[6] = { 0, 1, 2, 0, 2, 3 };
     for(int i = 0; i < 6; i++) mb->Indices.push_back(first + indices[i]);
 }
 \endcode
 Irrlicht не рисует обратную сторону треугольника (если не выключить EMF_BACK_FACE_CULLING), поэтому важно, куда он
 смотрит. Лицевая сторона треугольника (a, b, c) смотрит туда же, куда векторное произведение (b - a) x (c - a), -
 так устроена и грань createFaceMesh(). Помощник проверяет это и, если нужно, меняет обход углов, так что углы можно
 перечислять в любую сторону. Текстурные координаты берутся по положению вершины на грани: половинка стенки получит
 половинку текстуры, и полублок выглядит как обрезанный камень, а не как сжатый.

 Второй помощник добавляет одну грань коробки - в том же порядке, что и видимость граней:
 \code
 // Добавляет в мешбуфер грань face (порядок видимости: x+, z+, x-, z-, y+, y-) коробки от a до b.
 void addBoxFace(SMeshBuffer* mb, vector3df a, vector3df b, int face)
 {
     switch(face)
     {
         case 0: addQuad(mb, vector3df(b.X, a.Y, a.Z), vector3df(b.X, a.Y, b.Z), vector3df(b.X, b.Y, b.Z), vector3df(b.X, b.Y, a.Z), vector3df(1, 0, 0)); break;
         case 1: addQuad(mb, vector3df(a.X, a.Y, b.Z), vector3df(b.X, a.Y, b.Z), vector3df(b.X, b.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(0, 0, 1)); break;
         case 2: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(a.X, a.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(a.X, b.Y, a.Z), vector3df(-1, 0, 0)); break;
         case 3: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(b.X, a.Y, a.Z), vector3df(b.X, b.Y, a.Z), vector3df(a.X, b.Y, a.Z), vector3df(0, 0, -1)); break;
         case 4: addQuad(mb, vector3df(a.X, b.Y, a.Z), vector3df(b.X, b.Y, a.Z), vector3df(b.X, b.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(0, 1, 0)); break;
         case 5: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(b.X, a.Y, a.Z), vector3df(b.X, a.Y, b.Z), vector3df(a.X, a.Y, b.Z), vector3df(0, -1, 0)); break;
     }
 }
 \endcode
 И ещё два коротких: новый мешбуфер с текстурой и меш из мешбуфера (пустой мешбуфер даёт пустой меш - как раз то,
 что нужно для "ничего нет на этой стороне"):
 \code
 // Новый пустой мешбуфер с текстурой.
 SMeshBuffer* newBuffer(ITexture* texture)
 {
     SMeshBuffer* mb = new SMeshBuffer;
     mb->Material.setTexture(0, texture);
     return mb;
 }

 // Меш из мешбуфера (пустой мешбуфер - пустой меш без мешбуферов). Мешбуфер забирает меш.
 IMesh* newMesh(SMeshBuffer* mb)
 {
     SMesh* mesh = new SMesh;
     if(mb->getVertexCount() > 0)
     {
         mb->recalculateBoundingBox();
         mesh->addMeshBuffer(mb);
     }
     mb->drop();
     mesh->recalculateBoundingBox();
     return mesh;
 }
 \endcode
 Текстуры, как и в уроке 3, рисуем в коде: камень - серая шахматка, растение - зелёные стебли на прозрачном фоне
 (у прозрачных пикселей альфа равна 0). Функцию makeTexture() смотрите в полном коде.

 \section irrtut4_factory Фабрика: полублок, лестница, растение
 Все части строим один раз, в конструкторе фабрики (главный поток). Начнём с полублока:
 \code
         const vector3df low(-0.5f, -0.5f, -0.5f), high(0.5f, 0.5f, 0.5f);
         // Полублок: коробка от низа до середины вокселя.
         const vector3df slabhigh(0.5f, 0, 0.5f);
         SMeshBuffer* mb = newBuffer(stone);
         addBoxFace(mb, low, slabhigh, 4); // Верх полублока лежит внутри вокселя - виден всегда: glues[0].
         s_Slab[0] = newMesh(mb);
         for(int face = 0; face < 6; face++)
         {
             mb = newBuffer(stone);
             if(face != 4) addBoxFace(mb, low, slabhigh, face); // На границе вокселя сверху у полублока ничего нет - пустой меш.
             s_Slab[face + 1] = newMesh(mb);
         }
 \endcode
 Ровно по разобранной выше раскладке: верх коробки - в s_Slab[0], остальные грани - каждая в свою ячейку, а для y+
 (face == 4) - пустой меш.

 Лестница - нижняя половина целиком и верхняя четверть со стороны x+ (ступенька поднимается в сторону +X):
 \code
         // Лестница: нижняя половина целиком и верхняя четверть со стороны x+.
         const vector3df steplow(0, 0, -0.5f);
         mb = newBuffer(stone);
         addBoxFace(mb, low, vector3df(0, 0, 0.5f), 4); // Верх нижней ступеньки (внутри вокселя).
         addBoxFace(mb, steplow, high, 2); // Подступёнок - вертикальная стенка внутри вокселя.
         s_Stairs[0] = newMesh(mb);
         for(int face = 0; face < 6; face++)
         {
             mb = newBuffer(stone);
             if(face != 4) addBoxFace(mb, low, slabhigh, face); // Нижняя половина - все грани, кроме верха.
             if(face != 2 && face != 5) addBoxFace(mb, steplow, high, face); // Верхняя четверть - грани на границе вокселя.
             s_Stairs[face + 1] = newMesh(mb);
         }
 \endcode
 Внутри ячейки у лестницы две части: верх нижней ступеньки (на высоте 0, от x = -0.5 до 0) и подступёнок -
 вертикальная стенка на x = 0, смотрящая в -X. Они всегда видны - в s_Stairs[0]. На границе ячейки: справа (x+) - вся
 стенка целиком (низ коробки плюс верх ступеньки), спереди и сзади (z+ и z-) - "буква Г" из двух кусков, слева (x-) -
 только нижняя половина, сверху (y+) - верх ступеньки (он на высоте 0.5, на границе, и закрыт блоком сверху), снизу -
 дно. Получается, что в одной ячейке массива может лежать несколько кусков - это нормально, они просто склеиваются.

 Если лестницам нужны четыре направления, постройте четыре таких набора (повернув части вокруг оси Y) и храните
 направление в блоке, передавая его в VoxelInfo. При повороте меняется и то, в какой ячейке массива лежит какая
 стенка: стенка, которая была на x+, после поворота окажется на другой стороне и должна лечь в ячейку той стороны.

 Растение "крестом" - два пересекающихся по диагонали квадрата:
 \code
         // Растение "крестом": два пересекающихся квадрата - glues[0], граней на границе вокселя нет.
         mb = newBuffer(plant);
         addQuad(mb, vector3df(-0.45f, -0.5f, -0.45f), vector3df(0.45f, -0.5f, 0.45f), vector3df(0.45f, 0.5f, 0.45f), vector3df(-0.45f, 0.5f, -0.45f), vector3df(0.707f, 0, -0.707f));
         addQuad(mb, vector3df(-0.45f, -0.5f, 0.45f), vector3df(0.45f, -0.5f, -0.45f), vector3df(0.45f, 0.5f, -0.45f), vector3df(-0.45f, 0.5f, 0.45f), vector3df(0.707f, 0, 0.707f));
         s_Plant[0] = newMesh(mb);
         s_Empty = newMesh(newBuffer(0)); // Пустой меш: грань есть, но рисовать на ней нечего.
         for(int i = 1; i < 7; i++) s_Plant[i] = s_Empty;
 \endcode
 Обе плоскости проходят внутри ячейки, значит, обе - в glues[0], а все шесть граней - один общий пустой меш. Один и
 тот же меш можно класть в несколько ячеек: склейка его только читает.

 Теперь сама getMeshVoxel():
 \code
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         int id = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Id;
         if(id == ID_STONE) return getMeshCubeFaces(vxl, s_Face, vector3df(1, 1, 1)); // Обычный куб (и маленький - тоже).
         IMesh* mesh = getMeshVoxelByGlue(vxl, id == ID_SLAB ? s_Slab : (id == ID_STAIRS ? s_Stairs : s_Plant));
         // Склейка берёт меши "как есть": во вложенном чанке (воксель меньше 1) уменьшаем сами.
         Vector3d<float> scale = vxl->getScale();
         if(mesh && scale.getX() != 1) s_MeshManipulator->scale(mesh, vector3df(scale.getX(), scale.getY(), scale.getZ()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering* vxl)
     {
         return ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Id == ID_PLANT ? LEVEL_PLANTS : LEVEL_SOLID;
     }
 \endcode
 Камень - обычный куб из getMeshCubeFaces(): она сама учитывает размер вокселя, поэтому и маленький камень во
 вложенном чанке получится правильного размера. Остальное - склейка. Её результат - новый меш, и мы можем его
 менять: если воксель меньше 1 (он во вложенном чанке), уменьшаем меш манипулятором. Манипулятор меняет только
 вершины этого меша, поэтому вызывать его в потоке построения можно.

 Растения идут в свой уровень мешей. У их текстуры есть прозрачные пиксели, и им нужен материал
 EMT_TRANSPARENT_ALPHA_CHANNEL_REF: пиксель либо виден целиком, либо не рисуется совсем. Такой материал рисуется
 как непрозрачный и сортировки не требует. А ещё растение должно быть видно с обеих сторон, поэтому у его уровня мы
 выключим отсечение обратных сторон. Подробно об уровнях и прозрачности - в уроке 9.

 \section irrtut4_nested Вложенные чанки
 Второй способ - разделить ячейку на маленькие ячейки. Вложенный чанк - это ChunkVoxelObject, который стоит в ячейке
 мира вместо блока и сам делится на ячейки (подробно - в уроке 13):
 \code
 // Вложенный чанк 2x2x2: нижний слой из камня (полублок), с step == true - ещё и верх со стороны x+ (ступенька).
 ChunkVoxelObject* makeNestedBlock(bool step)
 {
     ChunkVoxelObject* nested = new ChunkVoxelObject();
     nested->setNumberOfPartitions(2);
     for(int x = 0; x < 2; x++)
         for(int z = 0; z < 2; z++)
         {
             nested->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new MyBlock(ID_STONE), true, false);
             if(step && x == 1) nested->setSimpleVoxelObject(Vector3d<int>(x, 1, z), new MyBlock(ID_STONE), true, false);
         }
     return nested;
 }
 \endcode
 Вложенный чанк можно заполнить до вставки в мир, а ставится он, как обычный блок, - например, через
 setSimpleVoxelObjectByGlobalVector(). Фабрике ничего особенного делать не нужно: каждый маленький кубик приходит в
 getMeshVoxel() как обычный воксель, только с размером 0.5.

 Зато видимость граней здесь точная, и без всякого тестера: ядро проверяет каждую маленькую ячейку, которая касается
 грани соседа. Под полублоком из маленьких кубиков верх пола закрыт, потому что все четыре маленькие ячейки над ним
 заняты камнем. Стенки между маленькими кубиками тоже не рисуются. А если вложенный чанк заполнен не целиком (как
 наш третий: маленький камень и маленькое растение), ядро видит пустые маленькие ячейки, и грань пола под ним видна.

 \section irrtut4_main Мир
 \code
     MyFactory* factory = new MyFactory(device, isve);
     factory->setNumberOfMeshesSceneNodeForChunk(2); // Два уровня: камень и растения.
     factory->setMaterialType(LEVEL_PLANTS, EMT_TRANSPARENT_ALPHA_CHANNEL_REF); // Прозрачные пиксели - "вырезаны".
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MyTransparentTester); // Тестер удалит мир.
     for(int level = 0; level < 2; level++) factory->setMaterialFlag(level, id, EMF_LIGHTING, false);
     factory->setMaterialFlag(LEVEL_PLANTS, id, EMF_BACK_FACE_CULLING, false); // Растение видно с обеих сторон.
 \endcode
 Два уровня мешей: число уровней - первым, затем материал уровня растений. Тестер прозрачности ставим миру сразу после
 создания (до start(), пока потоков нет). Ставится он один раз, и мир удалит его сам.

 Дальше - каменный пол и наши блоки:
 \code
     // Склейка из мешей (getMeshVoxelByGlue()).
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 2), new MyBlock(ID_SLAB));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(5, 1, 2), new MyBlock(ID_SLAB)); // Два полублока рядом.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(6, 1, 2), new MyBlock(ID_SLAB));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 6), new MyBlock(ID_STAIRS));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(9, 1, 2), new MyBlock(ID_PLANT));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(10, 1, 4), new MyBlock(ID_PLANT));
     // Вложенные чанки: полублок и ступенька из маленьких кубов, маленький камень с маленьким растением.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 10), makeNestedBlock(false));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(5, 1, 10), makeNestedBlock(true));
     ChunkVoxelObject* nested = new ChunkVoxelObject();
     nested->setNumberOfPartitions(2);
     nested->setSimpleVoxelObject(Vector3d<int>(0, 0, 0), new MyBlock(ID_STONE), true, false);
     nested->setSimpleVoxelObject(Vector3d<int>(1, 0, 1), new MyBlock(ID_PLANT), true, false);
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(9, 1, 10), nested);
     isve->addTaskUpdateChunks();
 \endcode
 Обратите внимание на маленькое растение: это склейка внутри вложенного чанка - здесь и пригождается уменьшение меша
 в getMeshVoxel().

 \section irrtut4_compare Что выбрать
 Склейка (getMeshVoxelByGlue()):
 - любая форма: диагонали, кресты растений, наклонные и повёрнутые части, модели;
 - блок остаётся одним блоком: мало памяти, мало треугольников (верх полублока - один квадрат);
 - текстура ложится естественно - как вы задали координаты;
 - но видимость граней приблизительная: соседям приходится объявлять блок прозрачным, и рисуется немного лишних
   граней; геометрию частей нужно писать самим; часть блока нельзя сломать отдельно.

 Вложенные чанки:
 - точная видимость граней сама собой, фабрика не меняется;
 - каждый маленький кубик - настоящий блок: его можно сломать или поставить отдельно, у него может быть своё
   освещение (в TestProject3 - режим маленьких блоков, клавиша C);
 - но только формы из сетки кубиков; больше памяти (вложенный чанк и его ячейки на каждый блок) и больше
   треугольников; на маленьком кубике текстура сжимается целиком, если не пересчитать текстурные координаты
   (как это сделать - в уроке 13).

 Итого: для массовых декоративных блоков (полублоки, лестницы, заборы, растения, факелы) обычно лучше склейка, а
 для разрушаемых деталей и построек из маленьких блоков - вложенные чанки. Способы можно смешивать, как в нашем
 примере.

 \section irrtut4_errors Типичные ошибки
 - В ячейку грани положен NULL вместо пустого меша: как только грань станет видна, меш чанка не построится.
 - Внутренняя часть блока (верх полублока, подступёнок, растение) положена в ячейку грани: она пропадает, когда рядом
   ставят блок.
 - Нет тестера прозрачности: у соседей полублоков и растений дыры в гранях.
 - Меши частей сделаны не в координатах вокселя (центр - начало координат, размер 1 при наших масштабах): блок
   сдвинут или огромный. Склейка ничего не сдвигает и не масштабирует.
 - Склейка во вложенном чанке не уменьшена.
 - Растение видно только с одной стороны: не выключено EMF_BACK_FACE_CULLING у его уровня (или прозрачные пиксели
   чёрные - не тот материал).

 Ну и напоследок ещё раз полный код:
 \code
 #include <cmath>
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Debug;
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 enum { ID_STONE = 1, ID_SLAB = 2, ID_STAIRS = 3, ID_PLANT = 4 }; // Типы блоков.
 enum { LEVEL_SOLID = 0, LEVEL_PLANTS = 1 }; // Уровни мешей: камень и растения (у растений - прозрачные пиксели).

 struct MyVoxelInfo : VoxelInfo
 {
     int s_Id;
 };

 class MyBlock : public SimpleVoxelObject
 {
 public:
     MyBlock(int id) : s_Id(id) { setTypeVoxel(Defines::CUSTOM_VOXEL); }
     VoxelInfo* getVoxelForRenderingInfo()
     {
         MyVoxelInfo* info = new MyVoxelInfo;
         info->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
         info->s_Id = s_Id;
         return info;
     }
     int getId() { return s_Id; }
 private:
     int s_Id;
 };

 // Прозрачность для видимости граней: полублок, лестница и растение не закрывают грани соседей целиком.
 class MyTransparentTester : public SimpleVoxelObjectTransparentTester
 {
 public:
     // svo - сосед (всегда блок: не чанк и не NULL), parent - блок, грань которого проверяется (может быть NULL).
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject*)
     {
         return ((MyBlock*)svo)->getId() != ID_STONE; // Непрозрачен только целый камень.
     }
 };

 // Добавляет в мешбуфер квадрат p0-p1-p2-p3 (углы по кругу) лицом в сторону normal.
 void addQuad(SMeshBuffer* mb, vector3df p0, vector3df p1, vector3df p2, vector3df p3, vector3df normal)
 {
     if((p1 - p0).crossProduct(p2 - p0).dotProduct(normal) < 0) { vector3df t = p1; p1 = p3; p3 = t; } // Обход - лицом к normal.
     vector3df p[4] = { p0, p1, p2, p3 };
     u16 first = (u16)mb->Vertices.size();
     for(int i = 0; i < 4; i++)
     {
         // Текстурные координаты - по положению на грани: кусок грани берёт свой кусок текстуры (верх картинки - сверху).
         f32 u = p[i].X + 0.5f, v = p[i].Z + 0.5f;
         if(fabsf(normal.X) > 0.5f) { u = p[i].Z + 0.5f; v = 0.5f - p[i].Y; }
         else if(fabsf(normal.Z) > 0.5f) { v = 0.5f - p[i].Y; }
         mb->Vertices.push_back(S3DVertex(p[i], normal, SColor(255, 255, 255, 255), vector2df(u, v)));
     }
     const u16 indices[6] = { 0, 1, 2, 0, 2, 3 };
     for(int i = 0; i < 6; i++) mb->Indices.push_back(first + indices[i]);
 }

 // Добавляет в мешбуфер грань face (порядок видимости: x+, z+, x-, z-, y+, y-) коробки от a до b.
 void addBoxFace(SMeshBuffer* mb, vector3df a, vector3df b, int face)
 {
     switch(face)
     {
         case 0: addQuad(mb, vector3df(b.X, a.Y, a.Z), vector3df(b.X, a.Y, b.Z), vector3df(b.X, b.Y, b.Z), vector3df(b.X, b.Y, a.Z), vector3df(1, 0, 0)); break;
         case 1: addQuad(mb, vector3df(a.X, a.Y, b.Z), vector3df(b.X, a.Y, b.Z), vector3df(b.X, b.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(0, 0, 1)); break;
         case 2: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(a.X, a.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(a.X, b.Y, a.Z), vector3df(-1, 0, 0)); break;
         case 3: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(b.X, a.Y, a.Z), vector3df(b.X, b.Y, a.Z), vector3df(a.X, b.Y, a.Z), vector3df(0, 0, -1)); break;
         case 4: addQuad(mb, vector3df(a.X, b.Y, a.Z), vector3df(b.X, b.Y, a.Z), vector3df(b.X, b.Y, b.Z), vector3df(a.X, b.Y, b.Z), vector3df(0, 1, 0)); break;
         case 5: addQuad(mb, vector3df(a.X, a.Y, a.Z), vector3df(b.X, a.Y, a.Z), vector3df(b.X, a.Y, b.Z), vector3df(a.X, a.Y, b.Z), vector3df(0, -1, 0)); break;
     }
 }

 // Новый пустой мешбуфер с текстурой.
 SMeshBuffer* newBuffer(ITexture* texture)
 {
     SMeshBuffer* mb = new SMeshBuffer;
     mb->Material.setTexture(0, texture);
     return mb;
 }

 // Меш из мешбуфера (пустой мешбуфер - пустой меш без мешбуферов). Мешбуфер забирает меш.
 IMesh* newMesh(SMeshBuffer* mb)
 {
     SMesh* mesh = new SMesh;
     if(mb->getVertexCount() > 0)
     {
         mb->recalculateBoundingBox();
         mesh->addMeshBuffer(mb);
     }
     mb->drop();
     mesh->recalculateBoundingBox();
     return mesh;
 }

 // Текстура 16x16 из кода: камень - серая шахматка, растение - зелёные стебли на прозрачном фоне.
 ITexture* makeTexture(IVideoDriver* driver, const char* name, bool plant)
 {
     IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(16, 16));
     for(u32 y = 0; y < 16; y++)
         for(u32 x = 0; x < 16; x++)
         {
             SColor c = ((x / 4 + y / 4) % 2) ? SColor(255, 150, 150, 150) : SColor(255, 120, 120, 120);
             if(plant) c = ((x == 7 || x == 8 || (x + y) % 6 == 0) && y > 2) ? SColor(255, 50, 160, 40) : SColor(0, 0, 0, 0);
             image->setPixel(x, y, c);
         }
     ITexture* texture = driver->addTexture(name, image);
     image->drop();
     return texture;
 }

 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         IVideoDriver* driver = device->getVideoDriver();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
         ITexture* stone = makeTexture(driver, "stone", false);
         ITexture* plant = makeTexture(driver, "plant", true);
         s_Face = createFaceMesh();
         s_Face->getMeshBuffer(0)->getMaterial().setTexture(0, stone);
         const vector3df low(-0.5f, -0.5f, -0.5f), high(0.5f, 0.5f, 0.5f);
         // Полублок: коробка от низа до середины вокселя.
         const vector3df slabhigh(0.5f, 0, 0.5f);
         SMeshBuffer* mb = newBuffer(stone);
         addBoxFace(mb, low, slabhigh, 4); // Верх полублока лежит внутри вокселя - виден всегда: glues[0].
         s_Slab[0] = newMesh(mb);
         for(int face = 0; face < 6; face++)
         {
             mb = newBuffer(stone);
             if(face != 4) addBoxFace(mb, low, slabhigh, face); // На границе вокселя сверху у полублока ничего нет - пустой меш.
             s_Slab[face + 1] = newMesh(mb);
         }
         // Лестница: нижняя половина целиком и верхняя четверть со стороны x+.
         const vector3df steplow(0, 0, -0.5f);
         mb = newBuffer(stone);
         addBoxFace(mb, low, vector3df(0, 0, 0.5f), 4); // Верх нижней ступеньки (внутри вокселя).
         addBoxFace(mb, steplow, high, 2); // Подступёнок - вертикальная стенка внутри вокселя.
         s_Stairs[0] = newMesh(mb);
         for(int face = 0; face < 6; face++)
         {
             mb = newBuffer(stone);
             if(face != 4) addBoxFace(mb, low, slabhigh, face); // Нижняя половина - все грани, кроме верха.
             if(face != 2 && face != 5) addBoxFace(mb, steplow, high, face); // Верхняя четверть - грани на границе вокселя.
             s_Stairs[face + 1] = newMesh(mb);
         }
         // Растение "крестом": два пересекающихся квадрата - glues[0], граней на границе вокселя нет.
         mb = newBuffer(plant);
         addQuad(mb, vector3df(-0.45f, -0.5f, -0.45f), vector3df(0.45f, -0.5f, 0.45f), vector3df(0.45f, 0.5f, 0.45f), vector3df(-0.45f, 0.5f, -0.45f), vector3df(0.707f, 0, -0.707f));
         addQuad(mb, vector3df(-0.45f, -0.5f, 0.45f), vector3df(0.45f, -0.5f, -0.45f), vector3df(0.45f, 0.5f, -0.45f), vector3df(-0.45f, 0.5f, 0.45f), vector3df(0.707f, 0, 0.707f));
         s_Plant[0] = newMesh(mb);
         s_Empty = newMesh(newBuffer(0)); // Пустой меш: грань есть, но рисовать на ней нечего.
         for(int i = 1; i < 7; i++) s_Plant[i] = s_Empty;
     }
     ~MyFactory()
     {
         s_Face->drop();
         for(int i = 0; i < 7; i++) { s_Slab[i]->drop(); s_Stairs[i]->drop(); }
         s_Plant[0]->drop();
         s_Empty->drop();
     }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         int id = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Id;
         if(id == ID_STONE) return getMeshCubeFaces(vxl, s_Face, vector3df(1, 1, 1)); // Обычный куб (и маленький - тоже).
         IMesh* mesh = getMeshVoxelByGlue(vxl, id == ID_SLAB ? s_Slab : (id == ID_STAIRS ? s_Stairs : s_Plant));
         // Склейка берёт меши "как есть": во вложенном чанке (воксель меньше 1) уменьшаем сами.
         Vector3d<float> scale = vxl->getScale();
         if(mesh && scale.getX() != 1) s_MeshManipulator->scale(mesh, vector3df(scale.getX(), scale.getY(), scale.getZ()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering* vxl)
     {
         return ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Id == ID_PLANT ? LEVEL_PLANTS : LEVEL_SOLID;
     }
     IMesh* s_Face;
     IMesh* s_Slab[7];
     IMesh* s_Stairs[7];
     IMesh* s_Plant[7];
     IMesh* s_Empty;
     IMeshManipulator* s_MeshManipulator;
 };

 // Вложенный чанк 2x2x2: нижний слой из камня (полублок), с step == true - ещё и верх со стороны x+ (ступенька).
 ChunkVoxelObject* makeNestedBlock(bool step)
 {
     ChunkVoxelObject* nested = new ChunkVoxelObject();
     nested->setNumberOfPartitions(2);
     for(int x = 0; x < 2; x++)
         for(int z = 0; z < 2; z++)
         {
             nested->setSimpleVoxelObject(Vector3d<int>(x, 0, z), new MyBlock(ID_STONE), true, false);
             if(step && x == 1) nested->setSimpleVoxelObject(Vector3d<int>(x, 1, z), new MyBlock(ID_STONE), true, false);
         }
     return nested;
 }


 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(1280, 720));
     Logger* lgr = new Logger;
     lgr->setPrintStandard(false);
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop();
     MyFactory* factory = new MyFactory(device, isve);
     factory->setNumberOfMeshesSceneNodeForChunk(2); // Два уровня: камень и растения.
     factory->setMaterialType(LEVEL_PLANTS, EMT_TRANSPARENT_ALPHA_CHANNEL_REF); // Прозрачные пиксели - "вырезаны".
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld();
     isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MyTransparentTester); // Тестер удалит мир.
     for(int level = 0; level < 2; level++) factory->setMaterialFlag(level, id, EMF_LIGHTING, false);
     factory->setMaterialFlag(LEVEL_PLANTS, id, EMF_BACK_FACE_CULLING, false); // Растение видно с обеих сторон.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true);
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++)
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), new MyBlock(ID_STONE)); // Каменный пол.
     // Склейка из мешей (getMeshVoxelByGlue()).
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 2), new MyBlock(ID_SLAB));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(5, 1, 2), new MyBlock(ID_SLAB)); // Два полублока рядом.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(6, 1, 2), new MyBlock(ID_SLAB));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 6), new MyBlock(ID_STAIRS));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(9, 1, 2), new MyBlock(ID_PLANT));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(10, 1, 4), new MyBlock(ID_PLANT));
     // Вложенные чанки: полублок и ступенька из маленьких кубов, маленький камень с маленьким растением.
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(2, 1, 10), makeNestedBlock(false));
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(5, 1, 10), makeNestedBlock(true));
     ChunkVoxelObject* nested = new ChunkVoxelObject();
     nested->setNumberOfPartitions(2);
     nested->setSimpleVoxelObject(Vector3d<int>(0, 0, 0), new MyBlock(ID_STONE), true, false);
     nested->setSimpleVoxelObject(Vector3d<int>(1, 0, 1), new MyBlock(ID_PLANT), true, false);
     isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(9, 1, 10), nested);
     isve->addTaskUpdateChunks();
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(6, 6, -4));
     camera->setTarget(vector3df(6, 0, 6));
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
