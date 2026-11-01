#include <irrlicht.h>
#include "CMeshCombiner/CMeshCombiner.h"
#include "CBatchingMesh/CBatchingMesh.h"
#include <iostream>

#include <SimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Debug;
using namespace Additional;

using namespace std;

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

IMesh* generateNodeCM(VoxelsForRendering* vxlfr, IVideoDriver* driver, ISceneManager* smgr)
{
    IMeshSceneNode* temp = 0;
    irr::core::array<irr::scene::IMeshSceneNode*> nodes;
    for(int i = 0; i < vxlfr->getNumberOfVoxelsForRendering(); i++)
    {
        VoxelForRendering vxl = vxlfr->getVoxelForRendering(i);
        temp = smgr->addCubeSceneNode();
        //cout<<i<<") "<<vxl.getPosition().getX()<<" "<<vxl.getPosition().getY()<<" "<<vxl.getPosition().getZ()<<endl;
        temp->setScale(vector3df( vxl.getScale().getX(), vxl.getScale().getY(), vxl.getScale().getZ() ));
        temp->setPosition(vector3df( vxl.getPosition().getX(), vxl.getPosition().getY(), vxl.getPosition().getZ() ));
        temp->getMesh()->setBoundingBox( temp->getTransformedBoundingBox() );
        temp->setMaterialFlag(video::EMF_LIGHTING, false);
        //...
        const core::dimension2d<u32> size(128, 128);
        const irr::io::path name="rt";
        const ECOLOR_FORMAT format = ECF_UNKNOWN;
        ITexture * target = driver->addRenderTargetTexture(size, name, format);
        driver->setRenderTarget(target);
        SColor yourColor;
        yourColor.set(vxl.getARGBColor().getA(), vxl.getARGBColor().getR(), vxl.getARGBColor().getG(), vxl.getARGBColor().getB());
        driver->draw2DRectangle(yourColor, rect<s32>(position2d<s32>(0,0),position2d<s32>(128,128)));
        driver->setRenderTarget(0);
        temp->setMaterialTexture(0, target);
        //...
        /*int txt = rand()%5 + 1;
        if(txt == 1) temp->setMaterialTexture(0, driver->getTexture("1.jpg"));
        else if(txt == 2) temp->setMaterialTexture(0, driver->getTexture("2.jpg"));
        else if(txt == 3) temp->setMaterialTexture(0, driver->getTexture("3.jpg"));
        else if(txt == 4) temp->setMaterialTexture(0, driver->getTexture("4.jpg"));
        else if(txt == 5) temp->setMaterialTexture(0, driver->getTexture("5.jpg"));*/
        nodes.push_back(temp);
        //...
    }
    CMeshCombiner combiner(0.8,40,ETPT_TILE);
    IMesh* combinedMesh = combiner.combineMeshes(driver,nodes);
    return combinedMesh;
}

void updateChunkMeshCM(IVideoDriver* driver, ISceneManager* smgr, IMeshSceneNode* k, IMesh* combinedMesh, Engine* eng, SimpleVoxelObject* chunk, World* wrld)
{
    if(k) k->remove();
    if(combinedMesh) delete combinedMesh;
    VoxelAddress* vxladdr = new VoxelAddress(eng);
    vxladdr->addNextVector(Additional::Vector3d<int>(0,0,0));
    VoxelsForRendering* vxlfr = chunk->getVoxelsForRenderingStatic(wrld, vxladdr);
    combinedMesh = generateNodeCM(vxlfr, driver, smgr);
    k = smgr->addMeshSceneNode(combinedMesh);
    k->setMaterialFlag(EMF_LIGHTING,false);
    delete vxladdr;
    delete vxlfr;
}

CBatchingMesh* generateNode(VoxelsForRendering* vxlfr, IVideoDriver* driver, ISceneManager* smgr)
{
    IMeshSceneNode* temp = smgr->addCubeSceneNode();
    temp->setMaterialFlag(video::EMF_LIGHTING, false);
    IMesh* mesh = temp->getMesh();
    CBatchingMesh* BatchingMesh = new CBatchingMesh();
    for(int i = 0; i < vxlfr->getNumberOfVoxelsForRendering(); i++)
    {
        VoxelForRendering vxl = vxlfr->getVoxelForRendering(i);
        /*const core::dimension2d<u32> size(128, 128);
        const irr::io::path name="rt";
        const ECOLOR_FORMAT format = ECF_UNKNOWN;
        ITexture * target = driver->addRenderTargetTexture(size, name, format);
        driver->setRenderTarget(target);
        SColor yourColor;
        yourColor.set(vxl.getARGBColor().getA(), vxl.getARGBColor().getR(), vxl.getARGBColor().getG(), vxl.getARGBColor().getB());
        driver->draw2DRectangle(yourColor, rect<s32>(position2d<s32>(0,0),position2d<s32>(128,128)));
        driver->setRenderTarget(0);
        mesh->getMeshBuffer(0)->getMaterial().setTexture(0, target);*/
        //...
        int txt = vxl.getARGBColor().getA() + 1;
        if(txt == 1) temp->setMaterialTexture(0, driver->getTexture("media/1.jpg"));
        else if(txt == 2) temp->setMaterialTexture(0, driver->getTexture("media/2.jpg"));
        else if(txt == 3) temp->setMaterialTexture(0, driver->getTexture("media/3.jpg"));
        else if(txt == 4) temp->setMaterialTexture(0, driver->getTexture("media/4.jpg"));
        else if(txt == 5) temp->setMaterialTexture(0, driver->getTexture("media/5.jpg"));
        BatchingMesh->addMesh(mesh, vector3df( vxl.getPosition().getX(), vxl.getPosition().getY(), vxl.getPosition().getZ() ),
                              vector3df( vxl.getRotation().getX(), vxl.getRotation().getY(), vxl.getRotation().getZ() ),
                              vector3df( vxl.getScale().getX(), vxl.getScale().getY(), vxl.getScale().getZ() ));
    }
    BatchingMesh->update();
	BatchingMesh->setMaterialFlag(EMF_LIGHTING, false);
	temp->remove();
    return BatchingMesh;
}

IMeshSceneNode* updateChunkMesh(IVideoDriver* driver, ISceneManager* smgr, IMeshSceneNode* k, IMesh* combinedMesh, Engine* eng, SimpleVoxelObject* chunk, World* wrld, Additional::Vector3d<int> vect)
{
    if(k) k->remove();
    if(combinedMesh) delete combinedMesh;
    VoxelAddress* vxladdr = new VoxelAddress(eng);
    vxladdr->addNextVector(vect);
    VoxelsForRendering* vxlfr = chunk->getVoxelsForRenderingStatic(wrld, vxladdr, 10);
    CBatchingMesh* bm = generateNode(vxlfr, driver, smgr);
    IMeshSceneNode *node = smgr->addMeshSceneNode(bm);
	//node->setScale(core::vector3df(10,10,10));
	node->setDebugDataVisible(EDS_BBOX_BUFFERS);
	bm->drop();
    k = node;
    delete vxladdr;
    delete vxlfr;
    return k;
}

class MyReceiver : public IEventReceiver
{
public:
        IVideoDriver* driver;
        ISceneManager* smgr;
        IMeshSceneNode* k;
        IMesh* combinedMesh;
        Engine* eng;
        SimpleVoxelObject* chunk;
        World* wrld;
        virtual bool OnEvent(const SEvent& event)
        {
                if (event.EventType == irr::EET_KEY_INPUT_EVENT)
                        KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
                if (event.EventType == irr::EET_KEY_INPUT_EVENT && !event.KeyInput.PressedDown)
                {
                        switch (event.KeyInput.Key)
                        {
                            case irr::KEY_KEY_Z:
                                    if(chunk->isExistsSimpleVoxelObjectInMemory(Additional::Vector3d<int>(16/2, 16/2, 15))) chunk->deleteSimpleVoxelObject(Additional::Vector3d<int>(16/2, 16/2, 15));
                                    else
                                    {
                                        SimpleVoxelObject* aa = new SimpleVoxelObject(eng, Defines::ARGBCOLOR);
                                        aa->setARGBColor(ARGBColor(1,1,1,1));
                                        chunk->setSimpleVoxelObject(Additional::Vector3d<int>(16/2, 16/2, 15), aa, true);
                                    }
                                    k = updateChunkMesh(driver, smgr, k, combinedMesh, eng, chunk, wrld, Vector3d<int>(0,0,0));
                                    return true;
                            default:
                                    break;
                        }
                }
                return false;
        }

        virtual bool IsKeyDown(EKEY_CODE keyCode) const
        {
                return KeyIsDown[keyCode];
        }

        MyReceiver(IVideoDriver* dr, ISceneManager* sm, IMeshSceneNode* kk, IMesh* cm, Engine* en, SimpleVoxelObject* ch, World* wr):
            driver(dr),
            smgr(sm),
            k(kk),
            combinedMesh(cm),
            eng(en),
            chunk(ch),
            wrld(wr)
        {
                for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
                        KeyIsDown[i] = false;
        }

private:
        bool KeyIsDown[KEY_KEY_CODES_COUNT];
};

int main()
{
    Logger* lgr = new Logger;
    //Engine* eng = new Engine(lgr);
    Engine* eng = new Engine();
    int id = eng->addWorld("test", 16);
    World* wrld = eng->getWorld(id);
    //wrld->loadChunk(Additional::Vector3d<int>(0,0,0));
    wrld->loadChunks(Additional::Vector3d<int>(0,0,0), 1);
    SimpleVoxelObject* chunk[27];
    Additional::Vector3d<int> vects[27];
    chunk[0] = wrld->getChunk(Additional::Vector3d<int>(0,0,0)); vects[0] = Additional::Vector3d<int>(0,0,0);
    chunk[1] = wrld->getChunk(Additional::Vector3d<int>(1,0,0)); vects[1] = Additional::Vector3d<int>(1,0,0);
    chunk[2] = wrld->getChunk(Additional::Vector3d<int>(1,0,1)); vects[2] = Additional::Vector3d<int>(1,0,1);
    chunk[3] = wrld->getChunk(Additional::Vector3d<int>(0,0,1)); vects[3] = Additional::Vector3d<int>(0,0,1);
    chunk[4] = wrld->getChunk(Additional::Vector3d<int>(-1,0,1)); vects[4] = Additional::Vector3d<int>(-1,0,1);
    chunk[5] = wrld->getChunk(Additional::Vector3d<int>(-1,0,0)); vects[5] = Additional::Vector3d<int>(-1,0,0);
    chunk[6] = wrld->getChunk(Additional::Vector3d<int>(-1,0,-1)); vects[6] = Additional::Vector3d<int>(-1,0,-1);
    chunk[7] = wrld->getChunk(Additional::Vector3d<int>(0,0,-1)); vects[7] = Additional::Vector3d<int>(0,0,-1);
    chunk[8] = wrld->getChunk(Additional::Vector3d<int>(1,0,-1)); vects[8] = Additional::Vector3d<int>(1,0,-1);

    chunk[9] = wrld->getChunk(Additional::Vector3d<int>(0,1,0)); vects[9] = Additional::Vector3d<int>(0,1,0);
    chunk[10] = wrld->getChunk(Additional::Vector3d<int>(1,1,0)); vects[10] = Additional::Vector3d<int>(1,1,0);
    chunk[11] = wrld->getChunk(Additional::Vector3d<int>(1,1,1)); vects[11] = Additional::Vector3d<int>(1,1,1);
    chunk[12] = wrld->getChunk(Additional::Vector3d<int>(0,1,1)); vects[12] = Additional::Vector3d<int>(0,1,1);
    chunk[13] = wrld->getChunk(Additional::Vector3d<int>(-1,1,1)); vects[13] = Additional::Vector3d<int>(-1,1,1);
    chunk[14] = wrld->getChunk(Additional::Vector3d<int>(-1,1,0)); vects[14] = Additional::Vector3d<int>(-1,1,0);
    chunk[15] = wrld->getChunk(Additional::Vector3d<int>(-1,1,-1)); vects[15] = Additional::Vector3d<int>(-1,1,-1);
    chunk[16] = wrld->getChunk(Additional::Vector3d<int>(0,1,-1)); vects[16] = Additional::Vector3d<int>(0,1,-1);
    chunk[17] = wrld->getChunk(Additional::Vector3d<int>(1,1,-1)); vects[17] = Additional::Vector3d<int>(1,1,-1);

    chunk[18] = wrld->getChunk(Additional::Vector3d<int>(0,-1,0)); vects[18] = Additional::Vector3d<int>(0,-1,0);
    chunk[19] = wrld->getChunk(Additional::Vector3d<int>(1,-1,0)); vects[19] = Additional::Vector3d<int>(1,-1,0);
    chunk[20] = wrld->getChunk(Additional::Vector3d<int>(1,-1,1)); vects[20] = Additional::Vector3d<int>(1,-1,1);
    chunk[21] = wrld->getChunk(Additional::Vector3d<int>(0,-1,1)); vects[21] = Additional::Vector3d<int>(0,-1,1);
    chunk[22] = wrld->getChunk(Additional::Vector3d<int>(-1,-1,1)); vects[22] = Additional::Vector3d<int>(-1,-1,1);
    chunk[23] = wrld->getChunk(Additional::Vector3d<int>(-1,-1,0)); vects[23] = Additional::Vector3d<int>(-1,-1,0);
    chunk[24] = wrld->getChunk(Additional::Vector3d<int>(-1,-1,-1)); vects[24] = Additional::Vector3d<int>(-1,-1,-1);
    chunk[25] = wrld->getChunk(Additional::Vector3d<int>(0,-1,-1)); vects[25] = Additional::Vector3d<int>(0,-1,-1);
    chunk[26] = wrld->getChunk(Additional::Vector3d<int>(1,-1,-1)); vects[26] = Additional::Vector3d<int>(1,-1,-1);
    /*SimpleVoxelObject* chunk = wrld->getChunk(Additional::Vector3d<int>(0, 0, 0));
    SimpleVoxelObject* chunk1 = wrld->getChunk(Additional::Vector3d<int>(1, 1, 1));
    if(!chunk || !chunk1) return 0;*/

    /*Additional::Vector3d<float> v(1,1,1);
    v = v*(0.5);
    cout<<v.getX()<<" "<<v.getY()<<" "<<v.getZ()<<endl;
    int a;
    cin>>a;*/


    IrrlichtDevice* device = createDevice(video::EDT_OPENGL, core::dimension2d< u32 >(640, 480), 16, false, false, false);
    if (device == 0) return 1; // движок не создан, выходим.
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    gui::IGUIEnvironment* guienv = device->getGUIEnvironment(); //!обязательно добавляем ГУИ
    device->setWindowCaption(L"Test Game");

    //-------------


    IMeshSceneNode* k = 0;
    IMeshSceneNode* k1 = 0;
    IMesh* combinedMesh = 0;

    k = updateChunkMesh(driver, smgr, k, combinedMesh, eng, chunk[0], wrld, vects[0]);
    for(int i = 1; i < 27; i++) updateChunkMesh(driver, smgr, k1, combinedMesh, eng, chunk[i], wrld, vects[i]);
    //k = updateChunkMesh(driver, smgr, k, combinedMesh, eng, chunk, wrld, Additional::Vector3d<int>(0, 0, 0));
    //k1 = updateChunkMesh(driver, smgr, k1, combinedMesh, eng, chunk, wrld, Additional::Vector3d<int>(1, 1, 1));

    ISceneNode* camFPS = smgr->addCameraSceneNodeFPS(0, 70, 0.03);
    camFPS->setPosition(vector3df(-20.0f, 15.0f, -20.0f));

    MyReceiver myRec(driver, smgr, k, combinedMesh, eng, chunk[0], wrld);
    device->setEventReceiver(&myRec);

    int lastFPS = -1;
    while(device->run())
        if (device->isWindowActive())
        {
            driver->beginScene(true,true, video::SColor(255,100,101,140));
            smgr->drawAll();
            guienv->drawAll();
            driver->endScene();
            int fps = driver->getFPS();
            if (lastFPS != fps)
            {
                core::stringw str = L"Trigger [FPS:";
                str += fps;
                str += "]";
                device->setWindowCaption(str.c_str());
                lastFPS = fps;
            }
        }
    device->drop();
    return 0;
}
