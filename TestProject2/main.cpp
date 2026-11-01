#include <irrlicht.h>
#include <iostream>

#include <ctime>

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Voxels;
using namespace Debug;
using namespace Additional;
using namespace Defines;

using namespace IrrEngine;

using namespace std;

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

class MyShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
    MyShaderCallBack():
        s_StarLightIntensity(0),
        s_TextureLayerID(0)
    {
        s_StarLightColor = SColorf(1.f, 1.f, 1.f, 0.f);
    }
	virtual void OnSetConstants(video::IMaterialRendererServices* services,
			s32 userData)
	{
        services->setPixelShaderConstant("s_TextureLayerID", &s_TextureLayerID, 1);
        services->setVertexShaderConstant("s_StarLightIntensity", reinterpret_cast<f32*>(&s_StarLightIntensity), 1);
        services->setPixelShaderConstant("s_StarLightColor", reinterpret_cast<f32*>(&s_StarLightColor), 4);
	}
	f32 s_StarLightIntensity;
	SColorf s_StarLightColor;
	s32 s_TextureLayerID;
};

class MyWorldGenerator : public WorldGenerator
{
    public:
        MyWorldGenerator():
            WorldGenerator()
        {
            //...
        }
        bool generateChunk(World* world, Vector3d<int> vect)
        {
            srand((unsigned)time(NULL));
            ChunkVoxelObject* chunk = world->getChunk(vect);
            chunk->setTypeVoxel(CHUNK_VOXEL_OBJECT);
            if(vect.getY() < 1)
            for(int x = 0; x < world->getSizeChunk(); x++)
                for(int y = 0; y < world->getSizeChunk(); y++)
                    for(int z = 0; z < world->getSizeChunk(); z++)
                    {
                        if( !(abs(x - world->getSizeChunk()/2) < 3 && abs(y - world->getSizeChunk()/2) < 3 && z > world->getSizeChunk()- 5) )
                        {
                            ARGBColorVoxel* newsvo = new ARGBColorVoxel();
                            if(!newsvo)
                            {
                                world->getEngine()->registerEvent(EVENT_TYPE_ERROR, "Error chunk generation.", true);
                                return false;
                            }
                            newsvo->setARGBColor(ARGBColor(rand()%5,rand()%256,rand()%256,rand()%256));
                            chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), newsvo, true, false);
                        }
                    }
            world->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generated.");
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
            if(!s_TempMesh)
            {
                s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
                TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
                S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
                for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            }
            if(!s_Driver) s_Driver = getIrrlichtDevice()->getVideoDriver(); //Получаем драйвер
            media = s_Driver->getTexture("media/BIG.jpg");
            blank = s_Driver->getTexture("media/blank.jpg");
            mm = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device, IrrSimpleVoxelEngine* eng):
            FactoryChunkMeshSceneNode(device, eng),
            s_Driver(0),
            s_TempMesh(0)
        {
            if(!s_TempMesh)
            {
                s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
                TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
                S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
                for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            }
            if(!s_Driver) s_Driver = getIrrlichtDevice()->getVideoDriver(); //Получаем драйвер
            media = s_Driver->getTexture("media/BIG.jpg");
            blank = s_Driver->getTexture("media/blank.jpg");
            mm = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device):
            FactoryChunkMeshSceneNode(device),
            s_Driver(0),
            s_TempMesh(0)
        {
            if(!s_TempMesh)
            {
                s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
                TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
                S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
                for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            }
            if(!s_Driver) s_Driver = getIrrlichtDevice()->getVideoDriver(); //Получаем драйвер
            media = s_Driver->getTexture("media/BIG.jpg");
            blank = s_Driver->getTexture("media/blank.jpg");
            mm = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
        }
        ~MyFactoryChunkMeshSceneNode()
        {
            delete s_TempMesh;
        }
    private:
        IMesh* getMeshVoxel(VoxelForRendering* vxl) //А вот и наша функция реакции на воксель
        {
            int txt = 1;
            VoxelInfo* t = vxl->getVoxelInfo(); //Будем действовать "тупо" - по первому параметру цвета давать текстуру.
                                                        //Если что - это костыль, вытащенный из генерации мира.
            if(t->s_TypeVoxelInfo == ARGBCOLOR_VOXEL_INFO)
            {
                txt = ((VoxelInfoARGBColor*)t)->s_ARGBColor.getA() + 1;
            }
            //if(txt == 1) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Driver->getTexture("media/1.jpg"));
            //else if(txt == 2) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Driver->getTexture("media/2.jpg"));
            //else if(txt == 3) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Driver->getTexture("media/3.jpg"));
            //else if(txt == 4) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Driver->getTexture("media/4.jpg"));
            //else if(txt == 5) s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Driver->getTexture("media/5.jpg"));
            S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
            if(txt == 1)
            {
                s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, media);
                for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++)
                {
                    ver[i].TCoords = TCoordsPlane[i] * 0.5;
                }
                s_TempMeshRes = getMeshCubeFaces(vxl, s_TempMesh, vector3df(1,1,1));
            }
            else
            {
                //s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, blank);
                s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, media);
                for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++)
                {
                    ver[i].TCoords = TCoordsPlane[i] * 0.5 + vector2d<f32>(.5f, .0f);
                    //ver[i].TCoords = TCoordsPlane[i] * 0.25 + vector2d<f32>(.0078125f, .0078125f);
                }
                s_TempMeshRes = getMeshCubeFaces(vxl, s_TempMesh, vector3df(1,1,1));
                mm->setVertexColors(s_TempMeshRes, SColor( ((VoxelInfoARGBColor*)t)->s_ARGBColor.getA(), ((VoxelInfoARGBColor*)t)->s_ARGBColor.getR(), ((VoxelInfoARGBColor*)t)->s_ARGBColor.getG(), ((VoxelInfoARGBColor*)t)->s_ARGBColor.getB()));
                //mm->makePlanarTextureMapping(s_TempMeshRes, 0.5f, 0.5f, 3, vector3df(0.25f,0 ,0.25f));
                mm->setVertexColorAlpha(s_TempMeshRes, 200);
            }
            return s_TempMeshRes; //Отдаём наш меш генератору нод
        }
        IVideoDriver* s_Driver; //Драйвер
        IMesh* s_TempMesh;
        IMesh* s_TempMeshRes;
        ITexture* media;
        ITexture* blank;
        IMeshManipulator* mm;
        vector2d<f32>* TCoordsPlane;
};

class MyReceiver : public IEventReceiver
{
public:
        virtual bool OnEvent(const SEvent& event)
        {
                if (event.EventType == irr::EET_KEY_INPUT_EVENT)
                        KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
                return false;
        }

        virtual bool IsKeyDown(EKEY_CODE keyCode) const
        {
                return KeyIsDown[keyCode];
        }

        MyReceiver()
        {
                for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
                        KeyIsDown[i] = false;
        }

private:
        bool KeyIsDown[KEY_KEY_CODES_COUNT];
};

int main()
{
    IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d< u32 >(640, 480), 16, false, false, false);
    if (device == 0) return 1;
    IVideoDriver* driver = device->getVideoDriver();
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();
    device->setWindowCaption(L"IrrSimpleVoxelEngine test.");

    // Создадим шейдеры освещения
    MyShaderCallBack* mc = new MyShaderCallBack();
    s32 chunkLightMaterialType = driver->getGPUProgrammingServices()->addHighLevelShaderMaterialFromFiles(
                                "Shaders/light.vert", "vertexMain", video::EVST_VS_1_1,
                                "Shaders/light.frag", "pixelMain", video::EPST_PS_1_1,
                                mc, video::EMT_SOLID, 0, EGSL_DEFAULT);
    //...
    Logger* lgr = new Logger;
    lgr->setPrintStandard(false);
    IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
    MyFactoryChunkMeshSceneNode* mfcmsn = new MyFactoryChunkMeshSceneNode(device, isve);
    mfcmsn->setPositionScale(16);
    mfcmsn->setSScale(16);
    mfcmsn->setSizeBlockChunks(1, 8, 1);
    isve->setFactoryChunkMeshSceneNode(mfcmsn);
    isve->getFactoryChunkMeshSceneNode()->setHardwareMappingHint(EHM_STATIC, EBT_VERTEX_AND_INDEX);
    int id = isve->addWorld("testworld", 16);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(id, EMF_LIGHTING, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(id, EMF_BILINEAR_FILTER, true);
    isve->getFactoryChunkMeshSceneNode()->setMaterialType((E_MATERIAL_TYPE)chunkLightMaterialType);
    isve->getFactoryChunkMeshSceneNode()->setMultiplePartition(id, true);
    //isve->getFactoryChunkMeshSceneNode()->setSizeBlockChunks(id, 1, 16, 1);
    //isve->getFactoryChunkMeshSceneNode()->setOcclusionQueries(true);
    //isve->getFactoryChunkMeshSceneNode()->setTimerTestOcclusionQueriesInMilliseconds(100);
    MyWorldGenerator* mywrldgen = new MyWorldGenerator;
    isve->getWorld(id)->setWorldGenerator(mywrldgen);

    isve->addChunksForRendering(id, Vector3d<int>(-10,0,-10), Vector3d<int>(10, 4, 10), true);
    //isve->addChunkForRendering(id, Vector3d<int>(0,0,0), true);
    //isve->addChunkForRendering(id, Vector3d<int>(0,0,-1), true);
    //isve->addChunkForRendering(id, Vector3d<int>(0,0,3), true);

    //isve->setMaxNumberOfThreadUpdateChunks(1);
    //isve->setMaxNumberOfThreadAddChunks(1);

    isve->start();

    ISceneNode* camFPS = smgr->addCameraSceneNodeFPS(0, 70, 0.055);
    //ISceneNode* camFPS = smgr->addCameraSceneNode();
    camFPS->setPosition(vector3df(0.0f, 0.0f, -3.0f));

    MyReceiver myRec;
    device->setEventReceiver(&myRec);

    int lastFPS = -1;
    int t = isve->getTimeSafeThread();
    int t2;
    bool test = false, del = true;
    while(device->run())
        if (device->isWindowActive() || true)
        {
            //if(isve->getModeMainThread() == NORMAL) cout << "NORMAL" << endl;
            //else cout << "THREAD STANDBY" << endl;
            t2 = isve->getTimeSafeThread();
            if(false && ( t2 - t > 3000 || (test && t2 - t > 50) ) )
            {
                test = true;
                //cout << "!" << endl;
                //if(isve->getWorld(id)->getChunk(Vector3d<int>(0,0,0)))
                {
                    if(del)
                    {
                        VoxelAddress* vxladdr = new VoxelAddress(isve);
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        isve->deleteSimpleVoxelObject(id, vxladdr);
                        del = false;
                    }
                    else
                    {
                        ARGBColorVoxel* svo = new ARGBColorVoxel();
                        svo->setTypeVoxel(ARGBCOLOR);
                        svo->setARGBColor(ARGBColor(1,1,1,1));
                        VoxelAddress* vxladdr = new VoxelAddress(isve);
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        isve->setSimpleVoxelObject(id, vxladdr, svo);
                        del = true;
                    }
                }
                isve->addTaskUpdateChunks();
                //isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(id, Vector3d<int>(0,0,0));
                //isve->getFactoryChunkMeshSceneNode()->updateChunk(id, Vector3d<int>(0,0,0));
                //isve->getFactoryChunkMeshSceneNode()->updateChunks();
                //isve->addTaskUpdateChunks();
                t = isve->getTimeSafeThread();
            }
            isve->update();
            driver->beginScene(true,true, SColor(255,100,101,140));
            smgr->drawAll();
            guienv->drawAll();
            driver->endScene();
            int fps = driver->getFPS();
            if (lastFPS != fps)
            {
                stringw str = L"Trigger [FPS:";
                str += fps;
                str += "]";
                device->setWindowCaption(str.c_str());
                lastFPS = fps;
            }
        }
    isve->stop();
    delete isve;
    device->drop();
    return 0;
}

/*int main()
{
    Engine* eng = new Engine;
    int id = eng->addWorld("testworld", 16);
    MyWorldGenerator* mywrldgen = new MyWorldGenerator;
    eng->getWorld(id)->setWorldGenerator(mywrldgen);
    World* wrld = eng->getWorld(id);
    for(int i = -10; i < 11; i++)
        for(int j = 0; j < 5; j++)
            for(int k = -10; k < 11; k++)
            {
                wrld->loadChunk(Vector3d<int>(i,j,k));
            }
    SimpleVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0));
    bool del = false;
    cout << "Go" << endl;
    while(1)
    {
        //...
    }
    return 0;
}*/
