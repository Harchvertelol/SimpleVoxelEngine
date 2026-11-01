/*!
 \mainpage Документация по IrrSimpleVoxelEngine.

 \section intro Введение

 Враппер IrrSimpleVoxelEngine написан для отрисовки воксельного мира с помощью движка Irrlicht, сам же воксельный мир
 хранится и обрабатывается с помощью движка SimpleVoxelEngine. Враппер строит меши чанков в нескольких потоках,
 склеивает соседние чанки в ноды Irrlicht, сам обновляет изменённые чанки и правильно рисует прозрачные воксели.

 \section friststep С чего начать?

 <a href="../../SimpleVoxelEngine/html/index.html">SimpleVoxelEngine</a>: Уроки, документация и описание движка SimpleVoxelEngine.<br>
 <a href="ideas.html">Общие идеи работы движка</a>: Данный раздел содержит общие идеи, необходимые для понимания сути работы враппера.<br>
 <a href="install.html">Настройка движка</a>: Данный раздел содержит инструкции по настройке враппера для работы.<br>
 <a href="tutorials.html">Уроки</a>: Данный раздел содержит набор уроков по врапперу IrrSimpleVoxelEngine.<br>
 <a href="author.html">Об авторе враппера</a>: Данный раздел содержит информацию об авторе враппера и людях, помогающих с его развитием.<br>

 \section links Быстрые ссылки

 <A HREF="namespaces.html">Пространства имён</A>: Очень удобно начинать изучение общей структуры враппера отсюда.<br>
 <A HREF="annotated.html">Список классов</A>: Список всех классов враппера с описаниями.<br>
 <A HREF="functions.html">Список функций</A>: Здесь удобно искать функции, если что-то забыл.<br>
 <A HREF="changes.html">Изменения в движке</A>: Здесь прописываются изменения во враппере по версиям.<br>

 \section examples Короткий пример

 Пример простейшего приложения на IrrSimpleVoxelEngine: один чанк 16x16x16 с полом из цветных вокселей
 (встроенный тип ARGBColorVoxel), фабрика, которая собирает куб из видимых граней, и FPS-камера.
 Никаких файлов (моделей, текстур) не нужно: грань куба даёт createFaceMesh(). Полный рабочий пример (свои воксели, генератор мира, сохранение,
 прозрачность, большие изменения мира в другом потоке) - TestProject3/main.cpp.

 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h> // Подключили враппер (он подключает и SimpleVoxelEngine).
 using namespace SimpleVoxelEngine; // Главное пространство имён движка.
 using namespace Core; // Ядро: World, VoxelForRendering и т.д.
 using namespace Voxels; // Воксели, в том числе встроенный ARGBColorVoxel.
 using namespace Additional; // Vector3d и ARGBColor.
 using namespace IrrEngine; // Сам враппер.
 using namespace irr; using namespace scene; using namespace video; using namespace core; // Irrlicht.

 // Фабрика решает, как нарисовать каждый воксель. Без неё враппер не работает.
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh(); // Грань 1x1, из которой getMeshCubeFaces() собирает куб (файл модели не нужен).
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator(); // Всё, что требует Irrlicht, - здесь, в главном потоке.
     }
     ~MyFactory() { s_Plane->drop(); } // Грань создали мы - мы её и удаляем.
 protected:
     // Вызывается в потоках построения для каждого видимого вокселя: вернуть НОВЫЙ меш (враппер сам сделает ему drop()).
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1)); // Куб только из видимых граней.
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor; // Цвет нашего ARGBColorVoxel.
         if(mesh) s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; } // Все воксели - в одном уровне мешей (одном материале).
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };

 int main()
 {
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600)); // Окно Irrlicht.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device); // Создали враппер (внутри - движок SimpleVoxelEngine).
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16); // Масштабы - равны размеру чанка (16): один воксель - одна единица сцены.
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory); // Фабрику - ДО создания мира. Удалит её враппер.
     int id = isve->addWorld(); // Мир по умолчанию: чанк 16x16x16 вокселей.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false); // Без источников света: виден цвет вершин.
     isve->addChunksForRendering(id, Vector3d<int>(0, 0, 0), Vector3d<int>(0, 0, 0), true); // Загрузить (здесь - создать пустой) чанк (0,0,0).
     for(int x = 0; x < 16; x++)
         for(int z = 0; z < 16; z++) // Пол из цветных вокселей 16x16.
         {
             ARGBColorVoxel* vxl = new ARGBColorVoxel; // Встроенный воксель с одним параметром - цветом.
             vxl->setARGBColor(ARGBColor(255, x * 16, 128, z * 16));
             isve->setSimpleVoxelObjectByGlobalVector(id, Vector3d<int>(x, 0, z), vxl); // Воксель забирает враппер.
         }
     isve->addTaskUpdateChunks(); // Перестроить изменённые чанки (один раз - после всех изменений).
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS();
     camera->setPosition(vector3df(8, 10, -10));
     camera->setTarget(vector3df(8, 0, 8));
     isve->start(); // Запустили потоки враппера: дальше чанки строятся в фоне.
     while(isve->runIrrDevice()) // То же, что device->run(), но безопасно для потоков враппера.
     {
         isve->update(); // Каждый кадр в главном потоке: готовые меши чанков становятся нодами сцены.
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop(); // Дождались выполнения всех заданий и остановили потоки.
     delete isve; // Удалили враппер, мир и фабрику.
     device->drop(); // Последним - устройство Irrlicht.
     return 0;
 }
 \endcode
*/
