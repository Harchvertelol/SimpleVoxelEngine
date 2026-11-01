#include <irrlicht.h> //Подключим иррлихт
#include <iostream>

#include <ctime>

#include <IrrSimpleVoxelEngine.h> //Подключим наш движок

using namespace SimpleVoxelEngine; //Подключим пространства имён нашего движка
using namespace Core;
using namespace Debug;
using namespace Additional;
using namespace IrrEngine;
using namespace Defines;

using namespace std;

using namespace irr; //Подключим пространства имён иррлихта
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

enum //Создадим enum для определения столкновений (это пока костыль)
{
    IDFlag_IsPickable,
    ID_IsNotPickable
};

class MyWorldGenerator : public WorldGenerator //Создадим наш генератор мира
{
    public:
        MyWorldGenerator():
            WorldGenerator()
        {
            //...
        }
        bool generateChunk(World* world, Vector3d<int> vect) //А вот и функция генерации
        {
            int t = clock();
            SimpleVoxelObject* chunk = world->getChunk(vect);
            chunk->setTypeVoxel(SIMPLE_VOXEL_OBJECTS);
            for(int x = 0; x < world->getSizeChunk(); x++)
                for(int y = 0; y < world->getSizeChunk(); y++)
                    for(int z = 0; z < world->getSizeChunk(); z++)
                    {
                        if( y < 9 )
                        {
                            SimpleVoxelObject* newsvo = new SimpleVoxelObject(world->getEngine());
                            if(!newsvo)
                            {
                                world->getEngine()->registerEvent(EVENT_TYPE_ERROR, "Error chunk generation.", true);
                                return false;
                            }
                            newsvo->setARGBColor(ARGBColor(3,rand()%256,rand()%256,rand()%256));
                            chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), newsvo, true, false);
                        }
                    }
            world->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generated.");
            int t2 = clock();
            cout << t2 - t << endl;
            return true;
        }
    private:
        //...
};

class MyFactoryChunkMeshSceneNode : public FactoryChunkMeshSceneNode //Описываем фабрику для того, чтобы самому по вокселю "говорить" движку, что рисовать
{
    public:
        MyFactoryChunkMeshSceneNode():
            FactoryChunkMeshSceneNode(),
            s_Driver(0),
            s_TempMesh(0)
        {
            textures = new ITexture*[5];
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device, IrrSimpleVoxelEngine* eng):
            FactoryChunkMeshSceneNode(device, eng),
            s_Driver(0),
            s_TempMesh(0)
        {
            textures = new ITexture*[5];
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device):
            FactoryChunkMeshSceneNode(device),
            s_Driver(0),
            s_TempMesh(0)
        {
            textures = new ITexture*[5];
        }
        void loadTextures()
        {
            if(!s_TempMesh) s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0); //Получили меш плоскости
            if(!s_Driver) s_Driver = getIrrlichtDevice()->getVideoDriver(); //Получаем драйвер
            textures[0] = s_Driver->getTexture("media/1.jpg");
            textures[1] = s_Driver->getTexture("media/2.jpg");
            textures[2] = s_Driver->getTexture("media/3.jpg");
            textures[3] = s_Driver->getTexture("media/4.jpg");
            textures[4] = s_Driver->getTexture("media/5.jpg");
        }
    private:
        IMesh* getMeshVoxel(VoxelForRendering* vxl) //А вот и наша функция реакции на воксель
        {
            int txt = vxl->getARGBColor().getA() + 1; //Будем действовать "тупо" - по первому параметру цвета давать текстуру.
                                                        //Если что - это костыль, вытащенный из генерации мира.
            if(txt == 1) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, textures[0]);
            else if(txt == 2) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, textures[1]);
            else if(txt == 3) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, textures[2]);
            else if(txt == 4) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, textures[3]);
            else if(txt == 5) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, textures[4]);
            s_TempMeshRes = getMeshCubeFaces(vxl, s_TempMesh, vector3df(1,1,1)); //Получаем меш куба по видимым граням
            return s_TempMeshRes; //Отдаём наш меш генератору нод
        }
        IVideoDriver* s_Driver; //Драйвер
        IMesh* s_TempMesh; //Меш
        IMesh* s_TempMeshRes;
        ITexture** textures;
};

class MyReceiver : public IEventReceiver //Наш обработчик событий
{
public:
        virtual bool OnEvent(const SEvent& event)
        {
                if (event.EventType == irr::EET_KEY_INPUT_EVENT)
                        KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
                if (event.EventType == irr::EET_KEY_INPUT_EVENT)
                {
                    if(event.KeyInput.Key == KEY_KEY_G && !event.KeyInput.PressedDown) //Включаем и выключаем гравитацию
                    {
                        if(*s_Gravity == false) *s_Gravity = true;
                        else *s_Gravity = false;
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_W && !event.KeyInput.PressedDown) //Тестируем изменение множества блоков сразу другим методом
                    {
                        VoxelAddress vxladdrtemp(isve);
                        vxladdrtemp.addNextVector(Vector3d<int>(1,0,0));
                        for(int i = 0; i < 8; i++)
                            for(int j = 0; j < 8; j++)
                                for(int k = 0; k < 8; k++)
                                {
                                    vxladdrtemp.addNextVector(Vector3d<int>(i,j,k));
                                    int r = rand()%10 - 5;
                                    if(r < 0) isve->deleteSimpleVoxelObject(s_World->getId(), &vxladdrtemp);
                                    else
                                    {
                                        SimpleVoxelObject* newsvo = new SimpleVoxelObject(s_World->getEngine());
                                        if(!newsvo) cout<<"Error creating SVO!"<<endl;
                                        newsvo->setARGBColor(ARGBColor(r,rand()%256,rand()%256,rand()%256));
                                        isve->setSimpleVoxelObject(s_World->getId(), &vxladdrtemp, newsvo, true);
                                    }
                                    vxladdrtemp.deleteLastVector();
                                }
                        isUpdate = true;
                    }
                }
                if(event.KeyInput.Key == KEY_KEY_E && !event.KeyInput.PressedDown) //Создаём плоскость для творчества
                {
                    VoxelAddress vxladdrtemp(isve);
                    vxladdrtemp.addNextVector(Vector3d<int>(1,0,0));
                    for(int i = 0; i < 16; i++)
                        for(int j = 5; j < 16; j++)
                            for(int k = 0; k < 16; k++)
                            {
                                vxladdrtemp.addNextVector(Vector3d<int>(i,j,k));
                                isve->deleteSimpleVoxelObject(s_World->getId(), &vxladdrtemp);
                                vxladdrtemp.deleteLastVector();
                            }
                    isUpdate = true;
                }
                if(event.KeyInput.Key == KEY_KEY_T && !event.KeyInput.PressedDown) //Создаём максимально лагучий чанк
                {
                    VoxelAddress vxladdrtemp(isve);
                    vxladdrtemp.addNextVector(Vector3d<int>(1,0,0));
                    for(int i = 0; i < 16; i++)
                        for(int j = 0; j < 16; j++)
                            for(int k = 0; k < 16; k++)
                            {
                                vxladdrtemp.addNextVector(Vector3d<int>(i,j,k));
                                if( (i % 2 == 0 && j % 2 != 0 && k % 2 == 0) ||
                                   (i % 2 == 0 && j % 2 == 0 && k % 2 != 0) ||
                                   (i % 2 != 0 && j % 2 != 0 && k % 2 != 0) ||
                                   (i % 2 != 0 && j % 2 == 0 && k % 2 == 0) ) isve->deleteSimpleVoxelObject(s_World->getId(), &vxladdrtemp);
                                else
                                {
                                    SimpleVoxelObject* newsvo = new SimpleVoxelObject(s_World->getEngine());
                                    if(!newsvo) cout<<"Error creating SVO!"<<endl;
                                    newsvo->setARGBColor(ARGBColor(rand()%5,rand()%256,rand()%256,rand()%256));
                                    isve->setSimpleVoxelObject(s_World->getId(), &vxladdrtemp, newsvo, true);
                                }
                                vxladdrtemp.deleteLastVector();
                            }
                    isUpdate = true;
                }
                if(event.KeyInput.Key == KEY_KEY_R && !event.KeyInput.PressedDown) //Регенерация чанка
                {
                    s_World->getWorldGenerator()->generateChunk(s_World, Vector3d<int>(1,0,0));
                    isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(1,0,0));
                    isUpdate = true;
                }
                if (event.EventType == irr::EET_MOUSE_INPUT_EVENT)
                {
                    bool left = false, right = false;
                    if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) left = true; //Удаление блока
                    if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) right = true; //Установка блока
                    float mn = 1;
                    if(left || right)
                    {
                        if(right) mn = (-1);
                        Vector3d<float> a1(hitTriangle.pointA.X, hitTriangle.pointA.Y, hitTriangle.pointA.Z);
                        Vector3d<float> a2(hitTriangle.pointB.X, hitTriangle.pointB.Y, hitTriangle.pointB.Z);
                        Vector3d<float> a3(hitTriangle.pointC.X, hitTriangle.pointC.Y, hitTriangle.pointC.Z);
                        VoxelAddress* vxladdr = s_World->getAddressSimpleVoxelObjectByThreePoints(a1, a2, a3, 16, vct*mn); //Получаем блок по трём точкам и вектору камеры
                        if(left) isve->deleteSimpleVoxelObject(s_World->getId(), vxladdr);
                        else
                        {
                            SimpleVoxelObject* newsvo = new SimpleVoxelObject(s_World->getEngine());
                            newsvo->setARGBColor(ARGBColor(s_TypeVoxelSet,rand()%256,rand()%256,rand()%256));
                            isve->setSimpleVoxelObject(s_World->getId(), vxladdr, newsvo, true);
                        }
                        delete vxladdr;
                        isUpdate = true;
                    }
                }
                return false;
        }

        virtual bool IsKeyDown(EKEY_CODE keyCode) const
        {
                return KeyIsDown[keyCode];
        }

        MyReceiver(World* wrld, IrrSimpleVoxelEngine* i, bool* gr):
            isUpdate(false),
            s_TypeVoxelSet(0),
            s_World(wrld),
            isve(i),
            s_Gravity(gr)
        {
                for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
                        KeyIsDown[i] = false;
        }

        triangle3df hitTriangle; //Треугольник щелчка по блоку
        Vector3d<float> vct; //Вектор камеры
        bool isUpdate; //Надо ли обновлять чанк
        int s_TypeVoxelSet; //Какие блоки ставим

private:
        World* s_World;
        IrrSimpleVoxelEngine* isve;
        bool* s_Gravity; //Указатель на контроллер гравитации
        bool KeyIsDown[KEY_KEY_CODES_COUNT];
};

int main()
{
    srand((unsigned)time(NULL));
    cout << sizeof(SimpleVoxelObject) << endl;
    IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d< u32 >(640, 480), 16, false, false, false);
    if (device == 0) return 1;
    IVideoDriver* driver = device->getVideoDriver();
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();
    device->setWindowCaption(L"IrrSimpleVoxelEngine test program");

    //...
    Logger* lgr = new Logger; //Создали логгер, мы же хотим отлавливать баги, да?
    IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr); //Создали наш движок сразу с привязкой к иррлу
    MyFactoryChunkMeshSceneNode* mfcmsn = new MyFactoryChunkMeshSceneNode(device, isve); //Создали нашу вышенаписанную фабрику
    mfcmsn->setPositionScale(16); //Установили масштаб позиции (так надо для нашего стандартного кубика иррла, для других моделей
                                                                                        //может быть по-другому)
    mfcmsn->setSScale(16);        //Установили масштаб самого вокселя
    isve->setFactoryChunkMeshSceneNode(mfcmsn); //Установили нашу фабрику движку
    mfcmsn->loadTextures();
    int id = isve->addWorld("testworld", 16); //Создали воксельный мир, в одном чанке 16*16*16 воксельных "точек"
                                                //(а воксельная "точка" может быть разбита ещё на воксели и т. д. в бесконечность)
                                                //Папка хранения - testworld.
    isve->getFactoryChunkMeshSceneNode()->setMultiplePartition(id, true); //Установили, что разбиение на каждом уровне у каждого вокселя одинаковое.
                                                                             //Тогда будет качественнее обработка связки чанков.
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; //Создали наш генератор мира
    isve->getWorld(id)->setWorldGenerator(mywrldgen); //Установили наш генератор мира движку
    int t = clock();
    for(int x = -7; x < 8; x++)
        for(int y = 0; y < 1; y++)
            for(int z = -7; z < 8; z++) isve->getWorld(id)->loadChunk(Vector3d<int>(x,y,z));
    for(int x = -6; x < 7; x++)
        for(int y = 0; y < 1; y++)
            for(int z = -6; z < 7; z++)
            {
                isve->addChunkForRendering(id, Vector3d<int>(x,y,z));
            }
    int t2 = clock();
    cout << t2 - t << endl;
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlagAllChunkNodes(EMF_LIGHTING, false); //Установили материал всем нашим отображённым чанкам
    //Всё, мы в нашем приложении увидим прорисованный один чанк.
    //...

    ICameraSceneNode* camFPS = smgr->addCameraSceneNodeFPS(0, 70, 0.008); //Создали камеру и установили правильно
    camFPS->setPosition(vector3df(24.4111f, 8.3288f, 25.5249f));
    camFPS->setTarget(vector3df(23.2238, 7.1799, -10.7250));
    camFPS->setNearValue(0.2);

    bool gravity = false; //Контроллер гравитации, изначально - отключена

    MyReceiver myRec(isve->getWorld(id), isve, &gravity); //Наш обработчик событий
    device->setEventReceiver(&myRec);

    IMeshSceneNode* chunk = isve->getFactoryChunkMeshSceneNode()->getMeshSceneNodeChunk(id, Vector3d<int>(1,0,0)); //Вешаем аниматоры столкновений
                                                                                                                     //И создаём селектор для получения треугольника
    ISceneNodeAnimator* anim = 0;
    scene::ITriangleSelector* selector = smgr->createTriangleSelector(chunk->getMesh(), chunk);
    chunk->setTriangleSelector(selector);
    chunk->setID(IDFlag_IsPickable);
    if(selector)
    {
		anim = smgr->createCollisionResponseAnimator(
			selector, camFPS, core::vector3df(0.4,0.9,0.4),
			core::vector3df(0,0,0), core::vector3df(0,0.5,0));
		selector->drop();
		camFPS->addAnimator(anim);
    }

    scene::IBillboardSceneNode * bill = smgr->addBillboardSceneNode(); //Создаём биллбоард по аналогии из туториала про коллиизии
	bill->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR );
	bill->setMaterialTexture(0, driver->getTexture("D:/irrlicht-1.8/media/particle.bmp"));
	bill->setMaterialFlag(video::EMF_LIGHTING, false);
	bill->setMaterialFlag(video::EMF_ZBUFFER, false);
	bill->setSize(core::dimension2d<f32>(1.0f, 1.0f));
	bill->setID(ID_IsNotPickable);

    device->getCursorControl()->setVisible(false); //Выключаем курсор

    scene::ISceneCollisionManager* collMan = smgr->getSceneCollisionManager();

    video::SMaterial material;
    material.Wireframe=true;
    material.setTexture(0, 0);
	material.Lighting = false;

	smgr->addSkyBoxSceneNode( //Создаём скайбокс
    driver->getTexture("skybox/irrlicht2_up.jpg"),
    driver->getTexture("skybox/irrlicht2_dn.jpg"),
    driver->getTexture("skybox/irrlicht2_lf.jpg"),
    driver->getTexture("skybox/irrlicht2_rt.jpg"),
    driver->getTexture("skybox/irrlicht2_ft.jpg"),
    driver->getTexture("skybox/irrlicht2_bk.jpg"));

    stringw blockstr, fpsstr, gravitystr, genstr;

    while(device->run())
        if (device->isWindowActive())
        {
            isve->update(); //Это надо для оптимизированной обработки вокселей. Можно и без этого, но тогда будет больше лагов при
                            //изменении вокселей.
            if(myRec.isUpdate) //Пересоздаём аниматоры и селекторы при изменении меша чанка, всё это пока костыли
            {
                IMeshSceneNode* chunk = isve->getFactoryChunkMeshSceneNode()->getMeshSceneNodeChunk(id, Vector3d<int>(1,0,0));
                scene::ITriangleSelector* selector = smgr->createTriangleSelector(chunk->getMesh(), chunk);
                chunk->setTriangleSelector(selector);
                chunk->setID(IDFlag_IsPickable);
                isve->getFactoryChunkMeshSceneNode()->setMaterialFlagAllChunkNodes(EMF_LIGHTING, false);
                if(selector)
                {
                    camFPS->removeAnimator(anim);
                    anim->drop();
                    if(gravity == false) anim = smgr->createCollisionResponseAnimator( //Учитываем гравитацию
                        selector, camFPS, core::vector3df(0.4,0.9,0.4),
                        core::vector3df(0,0,0), core::vector3df(0,0.5,0));
                    else if(gravity == true) anim = smgr->createCollisionResponseAnimator(
                        selector, camFPS, core::vector3df(0.4,0.9,0.4),
                        core::vector3df(0,-0.01,0), core::vector3df(0,0.5,0));
                    selector->drop();
                    camFPS->addAnimator(anim);
                }
                myRec.isUpdate = false;
            }
            int fps = driver->getFPS(); //Создаём строки для вывода информации
            IGUIFont *font1=guienv->getFont("fonts/arial.xml");
            blockstr = L"Type voxel set: ";
            blockstr += myRec.s_TypeVoxelSet + 1;
            blockstr += " (keys 1-5)";
            fpsstr = L"FPS: ";
            fpsstr += fps;
            gravitystr = L"Gravity: ";
            if(gravity) gravitystr += "On";
            else gravitystr += "Off";
            gravitystr += " ('G' key for change)";
            genstr = L"W - test change 8x8x8 voxels (back), E - cut the top of the chunk, R - regenerate chunk.";
            driver->beginScene(true,true, SColor(255,100,101,140));
            smgr->drawAll();
            font1->draw(blockstr,rect<s32>(10,10,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(fpsstr,rect<s32>(10,30,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(gravitystr,rect<s32>(10,50,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(genstr,rect<s32>(10,70,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            core::line3d<f32> ray; //А вот и наш луч. Получаем треугольник.
            ray.start = camFPS->getPosition();
            ray.end = ray.start + (camFPS->getTarget() - ray.start).normalize() * 30.0f;
            core::vector3df intersection;
            core::triangle3df hitTriangle;
            scene::ISceneNode * selectedSceneNode =
                collMan->getSceneNodeAndCollisionPointFromRay(
                        ray,
                        intersection,
                        hitTriangle,
                        IDFlag_IsPickable,
                        0 );
            if(selectedSceneNode)
            {
                bill->setPosition(intersection);
                driver->setTransform(video::ETS_WORLD, core::matrix4());
                driver->setMaterial(material);
                driver->draw3DTriangle(hitTriangle, video::SColor(0,255,0,0));
                myRec.hitTriangle = hitTriangle; //Отдаём обработчику треугольник
                vector3df aa = (camFPS->getTarget() - camFPS->getPosition()).normalize();
                myRec.vct = Vector3d<float>( aa.X, aa.Y, aa.Z ); //Отдаём обработчику вектор камеры
            }
            //...
            guienv->drawAll();
            driver->endScene();
            if(myRec.IsKeyDown(KEY_KEY_1)) myRec.s_TypeVoxelSet = 0; //Для установки типа блока
            if(myRec.IsKeyDown(KEY_KEY_2)) myRec.s_TypeVoxelSet = 1;
            if(myRec.IsKeyDown(KEY_KEY_3)) myRec.s_TypeVoxelSet = 2;
            if(myRec.IsKeyDown(KEY_KEY_4)) myRec.s_TypeVoxelSet = 3;
            if(myRec.IsKeyDown(KEY_KEY_5)) myRec.s_TypeVoxelSet = 4;
        }
    device->drop();
    return 0;
}
