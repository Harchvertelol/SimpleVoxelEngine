#include <irrlicht.h> //Подключим иррлихт
#include <iostream>

#include <ctime>

#include <IrrSimpleVoxelEngine.h> //Подключим наш движок

#include <CBatchingMesh/CBatchingMesh.h>

using namespace SimpleVoxelEngine; //Подключим пространства имён нашего движка
using namespace Core;
using namespace Voxels;
using namespace Debug;
using namespace Additional;
using namespace IrrEngine;
using namespace IrrSVEDefines;
using namespace Defines;

using namespace std;

using namespace irr; //Подключим пространства имён иррлихта
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

class MyShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
    MyShaderCallBack():
        s_StarLightIntensity(1),
        s_TextureLayerID(0)
    {
        s_StarLightColor = SColorf(1.f, 1.f, 1.f, 0.f);
    }
	virtual void OnSetConstants(video::IMaterialRendererServices* services,
			s32 userData)
	{
        services->setPixelShaderConstant("s_TextureLayerID", &s_TextureLayerID, 1);
        services->setPixelShaderConstant("s_StarLightColor", reinterpret_cast<f32*>(&s_StarLightColor), 4);
        services->setVertexShaderConstant("s_StarLightIntensity", reinterpret_cast<f32*>(&s_StarLightIntensity), 1);
	}
	f32 s_StarLightIntensity;
	SColorf s_StarLightColor;
	s32 s_TextureLayerID;
};

struct MyVoxelInfo : VoxelInfo //Структура, в которой будем передавать информацию для рендеринга
{
    unsigned int s_Id;
    SColor s_LightColor;
};

class MyBlock : public SimpleVoxelObject //Опишем наш блок, не будем пользоваться встроенным типами вокселей
{
    public:
        MyBlock(unsigned int id):
            SimpleVoxelObject(),
            s_Id(id)
        {
            setTypeVoxel(CUSTOM_VOXEL); //Для всех своих типов блоков делаем тип вокселя CUSTOM_VOXEL
            s_LightColor = SColor(255, 0, 0, 0);
        }
        MyBlock(unsigned int id, SColor lightcolor):
            SimpleVoxelObject(),
            s_Id(id),
            s_LightColor(lightcolor)
        {
            setTypeVoxel(CUSTOM_VOXEL); //Для всех своих типов блоков делаем тип вокселя CUSTOM_VOXEL
        }
        VoxelInfo* getVoxelForRenderingInfo() //Получение информации о вокселе
        {
            MyVoxelInfo* vxlnf = new MyVoxelInfo;
            vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO;
            vxlnf->s_Id = s_Id;
            vxlnf->s_LightColor = s_LightColor;
            return vxlnf;
        }
        unsigned int getId()
        {
            return s_Id;
        }
        char getStarLight()
        {
            return s_LightColor.getAlpha();
        }
        char getLightR()
        {
            return s_LightColor.getRed();
        }
        char getLightG()
        {
            return s_LightColor.getGreen();
        }
        char getLightB()
        {
            return s_LightColor.getBlue();
        }
    private:
        unsigned int s_Id;
        SColor s_LightColor;
};

class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester
{
    public:
        MySimpleVoxelObjectTransparentTester():
            SimpleVoxelObjectTransparentTester()
        {
            //...
        }
        ~MySimpleVoxelObjectTransparentTester()
        {
            //...
        }
        bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent)
        {
            if(((MyBlock*)svo)->getId() == 11 && parent && ((MyBlock*)parent)->getId() != 11) return true;
            else if(((MyBlock*)svo)->getId() == 11 && !parent) return true;
            if(((MyBlock*)svo)->getId() == 12 && parent && ((MyBlock*)parent)->getId() != 12) return true;
            else if(((MyBlock*)svo)->getId() == 12 && !parent) return true;
            return false;
        }
};

class MyWorldGenerator : public WorldGenerator //Создадим наш генератор мира
{
    public:
        MyWorldGenerator():
            WorldGenerator()
        {
            //...
        }
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk) //А вот и функция генерации
        {
            Vector3d<int> vect = chunk->getVoxelAddress()->getVector(0);
            SColor ttt = SColor(255, 0, 0, 0);
            /*if(vect.getY() < 4) //Создаём мир, в котором первые 4 чанка в высоту заполнены, остальные пусты. Это отсылка к майнкрафту.
            {
                MyBlock* newsvo = 0;
                for(int x = 0; x < sizechunk; x++)
                    for(int y = 0; y < sizechunk; y++)
                        for(int z = 0; z < sizechunk; z++)
                        {
                            if(vect.getY() == 3) ttt = SColor(y*16 + 15, 0, 0, 0);
                            else ttt = SColor(15, 0, 0, 0);
                            int idd;
                            if( ( x % 2 == 0 && y % 2 == 0 && z % 2 == 0) || (x % 2 != 0 && y % 2 != 0 && z % 2 != 0) ) idd = 3;
                            else idd = 5;
                            //idd = rand() % 10 + 1;
                            //if(rand()%100 != 0)
                            {
                                newsvo = new MyBlock(idd, ttt);
                                if(newsvo) chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), newsvo, true, false);
                            }
                        }
            }
            return true;*/


            if(vect.getY() < 4) //Создаём мир, в котором первые 4 чанка в высоту заполнены, остальные пусты. Это отсылка к майнкрафту.
            {
                MyBlock* newsvo = 0;
                for(int x = 0; x < sizechunk; x++)
                    for(int y = 0; y < sizechunk; y++)
                        for(int z = 0; z < sizechunk; z++)
                        {
                            int id = 12;
                            if(vect.getX() % 2 == 0 && vect.getY() % 2 == 0 && vect.getZ() % 2 == 0) id = 11;
                            newsvo = 0;
                            if(vect.getY() == 3) ttt = SColor(y*16 + 15, 0, 0, 0);
                            else ttt = SColor(15, 0, 0, 0);
                            //ttt = SColor(255, 255, 255, 255);
                            if(abs(vect.getX()) <= 3 && abs(vect.getZ()) <= 3 && vect.getY() > 0)
                            {
                                if( vect.getY() == 3 && y < 10 ) newsvo = new MyBlock(id, ttt);
                                else if( vect.getY() < 3 ) newsvo = new MyBlock(id, ttt);
                            }
                            else
                            {
                                int idd;
                                if( ( x % 2 == 0 && y % 2 == 0 && z % 2 == 0) || (x % 2 != 0 && y % 2 != 0 && z % 2 != 0) ) idd = 3;
                                else idd = 5;
                                newsvo = new MyBlock(idd, ttt);
                            }
                            //if(vect.getY() == 3) ttt = SColor(y*16 + 15, 0, 0, 0);
                            //else ttt = SColor(15, 0, 0, 0);
                            //newsvo = new MyBlock(3, ttt);
                            if(newsvo) chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), newsvo, true, false);
                        }
            }
            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generated.");
            return true;
        }
    private:
        //...
};

enum TYPE_MY_TASK //Наши задачи для выполнения в другом потоке
{
    CREATE_SPHERE,
    BOOM,
    CREATE_LIGHT
};

struct MyParams : ParametersForThreadFunction //Наш родительский тип для передачи параметров в поток
{
    TYPE_MY_TASK s_Type;
};

struct CreateSphereParams : MyParams //Параметры для создания сферы в потоке
{
    int s_IdWorld;
    int s_Radius;
    int s_TypeVoxelSet;
};

struct BoomParams : MyParams //Параметры для взрыва в потоке
{
    World* s_World;
    int s_Radius;
    triangle3df s_HitTriangle;
    Vector3d<float> s_Vct;
    Vector3d<float> s_Campos;
};

class MyController : public UserControllerForThread //Контроллер для работы в другом потоке
{
    public:
        MyController(IrrSimpleVoxelEngine* isve):
            UserControllerForThread(isve)
        {
            //...
        }
        ~MyController()
        {
            //...
        }
        void userThreadFunction(ParametersForThreadFunction* params) //Функция, которая и будет работать в другом поткое
        {
            IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
            //isve->getAccessToTheSimpleVoxelEngine();
            MyParams* myparameters = (MyParams*)params;
            if(myparameters->s_Type == CREATE_SPHERE)
            {
                CreateSphereParams* cspar = (CreateSphereParams*)myparameters;
                int rad = cspar->s_Radius;
                for(int x = -rad; x <= rad; x++)
                    for(int y = -rad + rad*2; y <= rad + rad*2; y++)
                        for(int z = -rad; z <= rad; z++)
                        {
                            if(y >=0 && y <= 16*16 && x <= 6*16 - 1 && x >= -5*16 && z <= 6*16 - 1 && z >= -5*16 ) //Дабы не вылазить за границы не загруженной области
                            {
                                if(rad*rad - (x*x + (y - rad*2)*(y - rad*2) + z*z) >= 0 && rad*rad - (x*x + (y - rad*2)*(y - rad*2) + z*z) <= rad*2 )
                                {
                                    MyBlock* svo = new MyBlock(cspar->s_TypeVoxelSet);
                                    isve->setSimpleVoxelObjectByGlobalVector(cspar->s_IdWorld, Vector3d<int>(x, y, z), svo);
                                }
                                else
                                {
                                    isve->setSimpleVoxelObjectByGlobalVector(cspar->s_IdWorld, Vector3d<int>(x, y, z), 0);
                                }
                            }
                        }
                isve->addTaskUpdateChunks();
            }
            else if(myparameters->s_Type == BOOM)
            {
                BoomParams* bp = (BoomParams*)myparameters;
                triangle3df hitTriangle = bp->s_HitTriangle;
                World* s_World = bp->s_World;
                Vector3d<float> vct = bp->s_Vct;
                int radboom = bp->s_Radius;
                Vector3d<float> a1(hitTriangle.pointA.X, hitTriangle.pointA.Y, hitTriangle.pointA.Z);
                Vector3d<float> a2(hitTriangle.pointB.X, hitTriangle.pointB.Y, hitTriangle.pointB.Z);
                Vector3d<float> a3(hitTriangle.pointC.X, hitTriangle.pointC.Y, hitTriangle.pointC.Z);
                int mn = 1;
                VoxelAddress* vxladdr = isve->getAddressSimpleVoxelObjectByThreePoints(s_World->getId(), a1, a2, a3, vct*mn, bp->s_Campos); //Получаем блок по трём точкам и вектору камеры
                Vector3d<int> vectboom = vxladdr->getLastVector();
                vxladdr->deleteLastVector();
                vectboom = vectboom + vxladdr->getLastVector() * 16;
                delete vxladdr;
                for(int x = -radboom + vectboom.getX(); x <= radboom + vectboom.getX(); x++)
                    for(int y = -radboom + vectboom.getY(); y <= radboom + vectboom.getY(); y++)
                        for(int z = -radboom + vectboom.getZ(); z <= radboom + vectboom.getZ(); z++)
                        {
                            if(y >=0 && y <= 16*16 && x <= 6*16 - 1 && x >= -5*16 && z <= 6*16 - 1 && z >= -5*16 ) //Дабы не вылазить за границы не загруженной области
                                if(radboom*radboom - ((x - vectboom.getX())*(x - vectboom.getX()) + (y - vectboom.getY())*(y - vectboom.getY()) + (z - vectboom.getZ())*(z - vectboom.getZ())) >= 0 )
                                {
                                    isve->setSimpleVoxelObjectByGlobalVector(s_World->getId(), Vector3d<int>(x, y, z), 0);
                                }
                        }
                isve->addTaskUpdateChunks();
            }
            else if(myparameters->s_Type == CREATE_LIGHT)
            {
                BoomParams* bp = (BoomParams*)myparameters;
                triangle3df hitTriangle = bp->s_HitTriangle;
                World* s_World = bp->s_World;
                Vector3d<float> vct = bp->s_Vct;
                int radboom = bp->s_Radius;
                Vector3d<float> a1(hitTriangle.pointA.X, hitTriangle.pointA.Y, hitTriangle.pointA.Z);
                Vector3d<float> a2(hitTriangle.pointB.X, hitTriangle.pointB.Y, hitTriangle.pointB.Z);
                Vector3d<float> a3(hitTriangle.pointC.X, hitTriangle.pointC.Y, hitTriangle.pointC.Z);
                int mn = 1;
                VoxelAddress* vxladdr = isve->getAddressSimpleVoxelObjectByThreePoints(s_World->getId(), a1, a2, a3, vct*mn, bp->s_Campos); //Получаем блок по трём точкам и вектору камеры
                Vector3d<int> vectboom = vxladdr->getLastVector();
                vxladdr->deleteLastVector();
                vectboom = vectboom + vxladdr->getLastVector() * 16;
                delete vxladdr;
                int id;
                int lightcolorR = 222;
                int lightcolorG = 9;
                int lightcolorB = 255;
                for(int x = -radboom + vectboom.getX(); x <= radboom + vectboom.getX(); x++)
                    for(int y = -radboom + vectboom.getY(); y <= radboom + vectboom.getY(); y++)
                        for(int z = -radboom + vectboom.getZ(); z <= radboom + vectboom.getZ(); z++)
                        {
                            if(y >=0 && y <= 16*16 && x <= 6*16 - 1 && x >= -5*16 && z <= 6*16 - 1 && z >= -5*16 ) //Дабы не вылазить за границы не загруженной области
                                if(radboom*radboom - ((x - vectboom.getX())*(x - vectboom.getX()) + (y - vectboom.getY())*(y - vectboom.getY()) + (z - vectboom.getZ())*(z - vectboom.getZ())) >= 0 )
                                {
                                    int starlight = 0;
                                    MyBlock* svo = (MyBlock*)isve->getSimpleVoxelObjectByGlobalVector(s_World->getId(), Vector3d<int>(x, y, z), false);
                                    char r = 0, g = 0, b = 0;
                                    if(svo)
                                    {
                                        id = svo->getId();
                                        starlight = svo->getStarLight();
                                        //r = svo->getLightR();
                                        //g = svo->getLightG();
                                        //b = svo->getLightB();
                                    }
                                    else id = -1;
                                    int aa = ((x - vectboom.getX())*(x - vectboom.getX()) + (y - vectboom.getY())*(y - vectboom.getY()) + (z - vectboom.getZ())*(z - vectboom.getZ()));
                                    SColor lightcolor(starlight, r + (255 - sqrt(aa) * 255 / radboom) * lightcolorR / 255, g + (255 - sqrt(aa) * 255 / radboom) * lightcolorG / 255, b + (255 - sqrt(aa) * 255 / radboom) * lightcolorB / 255);
                                    if(id != -1) isve->setSimpleVoxelObjectByGlobalVector(s_World->getId(), Vector3d<int>(x, y, z), new MyBlock(id, lightcolor));
                                }
                        }
                isve->addTaskUpdateChunks();
            }
            //isve->freeAccessToTheSimpleVoxelEngine();
        }
    private:
        //...
};

class MyFactoryChunkMeshSceneNode : public FactoryChunkMeshSceneNode //Описываем фабрику для того, чтобы самому по вокселю "говорить" движку, что рисовать
{
    public:
        MyFactoryChunkMeshSceneNode(): //Важно! Функция получения вокселя работает в другом потоке, поэтому загрузку
                                        //всех данных производим в конструкторе.
            FactoryChunkMeshSceneNode()
        {
            s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
            TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
            S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
            for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            s_Driver = getIrrlichtDevice()->getVideoDriver();
            s_MeshManipulator = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
            s_Textures = new ITexture*[13];
            s_Textures[0] = s_Driver->getTexture("media/1.jpg");
            s_Textures[1] = s_Driver->getTexture("media/2.jpg");
            s_Textures[2] = s_Driver->getTexture("media/3.jpg");
            s_Textures[3] = s_Driver->getTexture("media/4.jpg");
            s_Textures[4] = s_Driver->getTexture("media/5.jpg");
            s_Textures[5] = s_Driver->getTexture("media/6.jpg");
            s_Textures[6] = s_Driver->getTexture("media/7.jpg");
            s_Textures[7] = s_Driver->getTexture("media/8.jpg");
            s_Textures[8] = s_Driver->getTexture("media/9.jpg");
            s_Textures[9] = s_Driver->getTexture("media/10.jpg");
            s_Textures[10] = s_Driver->getTexture("media/11.bmp");
            s_Textures[11] = s_Driver->getTexture("media/12.png");
            s_Textures[12] = s_Driver->getTexture("media/13.png");
            /*s_NormalMap = new ITexture*[13];
            s_NormalMap[0] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[1] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[2] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[3] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[4] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[5] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[6] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[7] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[8] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[9] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[10] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[11] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[12] = s_Driver->getTexture("media/7_height.bmp");
            for(int i = 0; i < 13; i++) if (s_NormalMap[i]) s_Driver->makeNormalMapTexture(s_NormalMap[i], 9.0f);*/
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device, IrrSimpleVoxelEngine* eng):
            FactoryChunkMeshSceneNode(device, eng)
        {
            s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
            TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
            S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
            for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            s_TempMeshs = new IMesh*[6];
            for(int i = 0; i < 6; i++) s_TempMeshs[i] = getIrrlichtDevice()->getSceneManager()->getMesh(("models/plane" + WorkFunctions::ConvertFunctions::itos(i) + ".obj").c_str())->getMesh(0);
            s_Driver = getIrrlichtDevice()->getVideoDriver();
            s_MeshManipulator = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
            s_Textures = new ITexture*[13];
            s_Textures[0] = s_Driver->getTexture("media/1.jpg");
            s_Textures[1] = s_Driver->getTexture("media/2.jpg");
            s_Textures[2] = s_Driver->getTexture("media/3.jpg");
            s_Textures[3] = s_Driver->getTexture("media/4.jpg");
            s_Textures[4] = s_Driver->getTexture("media/5.jpg");
            s_Textures[5] = s_Driver->getTexture("media/6.jpg");
            s_Textures[6] = s_Driver->getTexture("media/7.jpg");
            s_Textures[7] = s_Driver->getTexture("media/8.jpg");
            s_Textures[8] = s_Driver->getTexture("media/9.jpg");
            s_Textures[9] = s_Driver->getTexture("media/10.jpg");
            s_Textures[10] = s_Driver->getTexture("media/11.bmp");
            s_Textures[11] = s_Driver->getTexture("media/12.png");
            s_Textures[12] = s_Driver->getTexture("media/13.png");
            /*s_NormalMap = new ITexture*[13];
            s_NormalMap[0] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[1] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[2] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[3] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[4] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[5] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[6] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[7] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[8] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[9] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[10] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[11] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[12] = s_Driver->getTexture("media/7_height.bmp");
            for(int i = 0; i < 13; i++) if (s_NormalMap[i]) s_Driver->makeNormalMapTexture(s_NormalMap[i], 9.0f);*/
        }
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device):
            FactoryChunkMeshSceneNode(device)
        {
            s_TempMesh = getIrrlichtDevice()->getSceneManager()->getMesh("models/plane3.obj")->getMesh(0);
            TCoordsPlane = new vector2d<f32>[s_TempMesh->getMeshBuffer(0)->getVertexCount()];
            S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
            for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) TCoordsPlane[i] = ver[i].TCoords;
            s_Driver = getIrrlichtDevice()->getVideoDriver();
            s_MeshManipulator = getIrrlichtDevice()->getSceneManager()->getMeshManipulator();
            s_Textures = new ITexture*[13];
            s_Textures[0] = s_Driver->getTexture("media/1.jpg");
            s_Textures[1] = s_Driver->getTexture("media/2.jpg");
            s_Textures[2] = s_Driver->getTexture("media/3.jpg");
            s_Textures[3] = s_Driver->getTexture("media/4.jpg");
            s_Textures[4] = s_Driver->getTexture("media/5.jpg");
            s_Textures[5] = s_Driver->getTexture("media/6.jpg");
            s_Textures[6] = s_Driver->getTexture("media/7.jpg");
            s_Textures[7] = s_Driver->getTexture("media/8.jpg");
            s_Textures[8] = s_Driver->getTexture("media/9.jpg");
            s_Textures[9] = s_Driver->getTexture("media/10.jpg");
            s_Textures[10] = s_Driver->getTexture("media/11.bmp");
            s_Textures[11] = s_Driver->getTexture("media/12.png");
            s_Textures[12] = s_Driver->getTexture("media/13.png");
            /*s_NormalMap = new ITexture*[13];
            s_NormalMap[0] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[1] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[2] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[3] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[4] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[5] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[6] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[7] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[8] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[9] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[10] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[11] = s_Driver->getTexture("media/7_height.bmp");
            s_NormalMap[12] = s_Driver->getTexture("media/7_height.bmp");
            for(int i = 0; i < 13; i++) if (s_NormalMap[i]) s_Driver->makeNormalMapTexture(s_NormalMap[i], 9.0f);*/
        }
        ~MyFactoryChunkMeshSceneNode()
        {
            if(s_TempMeshs) delete[] s_TempMeshs;
            if(s_Textures) delete[] s_Textures;
        }
        ICameraSceneNode* camFPS;
        IMetaTriangleSelector* s_MTS;
    private:
        IMesh* getMeshVoxel(VoxelForRendering* vxl) //А вот и наша функция реакции на воксель
        {
            int txt = 1;
            VoxelInfo* t = vxl->getVoxelInfo(); //Получаем информацию для рендеринга
            if(t->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO)
            {
                txt = ((MyVoxelInfo*)t)->s_Id + 1; //Узнаём ID
            }
            /*S3DVertex* ver = (S3DVertex*)s_TempMesh->getMeshBuffer(0)->getVertices();
            if(txt == 13)
            {
                if(txt == 1)
                {
                    for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++)
                    {
                        if(ver[i].TCoords.Y != 0) ver[i].TCoords.Y = TCoordsPlane[i].Y * 0.3125f;
                    }
                }
            }
            else for(unsigned int i = 0; i < s_TempMesh->getMeshBuffer(0)->getVertexCount(); i++) ver[i].TCoords = TCoordsPlane[i];*/
            s_TempMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[txt - 1]); //Мне было лень делать текстурный атлас, поэтому пока так,
                                                                                            //но данная вещь даёт на каждую текстуру свой мешбуффер,
                                                                                            //а значит больше лагов.
            if(((MyVoxelInfo*)t)->s_Id == 0) for(int i = 0; i < 6; i++) s_TempMeshs[i]->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[i]);
            if(((MyVoxelInfo*)t)->s_Id == 0) s_TempMeshRes = getMeshCubeFaces(vxl, s_TempMeshs, vector3df(1,1,1)); //Получаем меш куба по видимым граням
            else s_TempMeshRes = getMeshCubeFaces(vxl, s_TempMesh, vector3df(1,1,1));
            s_MeshManipulator->setVertexColors(s_TempMeshRes, ((MyVoxelInfo*)t)->s_LightColor);
            s_TempMeshRes->getMeshBuffer(0)->getMaterial().MaterialTypeParam = pack_textureBlendFunc(EBF_SRC_ALPHA, EBF_DST_ALPHA, EMFN_MODULATE_4X);
            //if(((MyVoxelInfo*)t)->s_Id == 12) s_TempMeshRes->getMeshBuffer(0)->getMaterial().ColorMask = ECP_RGB;
            return s_TempMeshRes; //Отдаём наш меш генератору нод
        }
        int getMeshNumber(SimpleVoxelEngine::Core::VoxelForRendering* vxl)
        {
            int txt = 1;
            VoxelInfo* t = vxl->getVoxelInfo();
            if(t->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO)
            {
                txt = ((MyVoxelInfo*)t)->s_Id + 1;
            }
            if(txt == 12) return 1;
            if(txt == 13) return 2;
            return 0;
        }
        void onUpdateChunkMeshSceneNode(IMeshSceneNode* meshscenenode, int numbermesh, Vector3d<int> vect)
        {
            if(numbermesh == 2) return;
            if(s_TA[numbermesh][vect])
            {
                s_MTS->removeTriangleSelector(s_TA[numbermesh][vect]);
                s_TA[numbermesh][vect]->drop();
                s_TA[numbermesh].erase(vect);
            }
            /*if(s_Anims[numbermesh][vect])
            {
                camFPS->removeAnimator(s_Anims[numbermesh][vect]);
                s_Anims[numbermesh].erase(vect);
            }*/
            if(!meshscenenode) return;
            //ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createTriangleSelector(meshscenenode->getMesh(), meshscenenode);
            ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createOctTreeTriangleSelector(meshscenenode->getMesh(), meshscenenode);
            if(!selector) return;
            meshscenenode->setTriangleSelector(selector);
            s_MTS->addTriangleSelector(selector);
            s_TA[numbermesh][vect] = selector;
            /*ISceneNodeAnimator* anim = getIrrlichtDevice()->getSceneManager()->createCollisionResponseAnimator(
                selector, camFPS, core::vector3df(0.4,0.9,0.4),
                core::vector3df(0,-0.01,0), core::vector3df(0,0.5,0));
            camFPS->addAnimator(anim);
            s_Anims[numbermesh][vect] = anim;
            selector->drop();*/
        }
        IVideoDriver* s_Driver;
        IMesh** s_TempMeshs;
        IMesh* s_TempMesh;
        IMesh* s_TempMeshRes;
        ITexture** s_Textures;
        ITexture** s_NormalMap;
        IMeshManipulator* s_MeshManipulator;
        vector2d<f32>* TCoordsPlane;
        std::map<Vector3d<int>, ISceneNodeAnimator*> s_Anims[3];
        std::map<Vector3d<int>, ITriangleSelector*> s_TA[3];
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
                    if(!(*s_Ghost) && event.KeyInput.Key == KEY_KEY_M && !event.KeyInput.PressedDown) //Включаем и выключаем гравитацию
                    {
                        if(*s_Gravity == false) *s_Gravity = true;
                        else *s_Gravity = false;
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_N && !event.KeyInput.PressedDown) //Включаем и выключаем прохождение сквозь стены
                    {
                        if(*s_Ghost == false)
                        {
                            *s_Ghost = true;
                            //*s_Gravity = false;
                        }
                        else *s_Ghost = false;
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_W && !event.KeyInput.PressedDown) //Просто изменение блока вокселей в чанке (0,5,0)
                    {
                        VoxelAddress vxladdrtemp(isve);
                        vxladdrtemp.addNextVector(Vector3d<int>(0,5,0));
                        for(int i = 0; i < 8; i++)
                            for(int j = 0; j < 8; j++)
                                for(int k = 0; k < 8; k++)
                                {
                                    VoxelAddress* nva = new VoxelAddress(vxladdrtemp);
                                    nva->addNextVector(Vector3d<int>(i,j,k));
                                    int r = rand()%16 - 5;
                                    if(r < 0) isve->deleteSimpleVoxelObject(s_World->getId(), nva);
                                    else
                                    {
                                        MyBlock* newsvo = new MyBlock(r);
                                        if(!newsvo) cout<<"Error creating SVO!"<<endl;
                                        isve->setSimpleVoxelObject(s_World->getId(), nva, newsvo, true);
                                    }
                                }
                        isve->addTaskUpdateChunks();
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_E && !event.KeyInput.PressedDown) //Создаём плоскость для творчества из чанка (0,5,0)
                    {
                        VoxelAddress vxladdrtemp(isve);
                        vxladdrtemp.addNextVector(Vector3d<int>(0,5,0));
                        for(int i = 0; i < 16; i++)
                            for(int j = 5; j < 16; j++)
                                for(int k = 0; k < 16; k++)
                                {
                                    VoxelAddress* nva = new VoxelAddress(vxladdrtemp);
                                    nva->addNextVector(Vector3d<int>(i,j,k));
                                    isve->deleteSimpleVoxelObject(s_World->getId(), nva);
                                }
                        isve->addTaskUpdateChunks();
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_T && !event.KeyInput.PressedDown) //Создаём максимально лагучий чанк (0,5,0)
                    {
                        VoxelAddress vxladdrtemp(isve);
                        vxladdrtemp.addNextVector(Vector3d<int>(0,5,0));
                        for(int i = 0; i < 16; i++)
                            for(int j = 0; j < 16; j++)
                                for(int k = 0; k < 16; k++)
                                {
                                    VoxelAddress* nva = new VoxelAddress(vxladdrtemp);
                                    nva->addNextVector(Vector3d<int>(i,j,k));
                                    if( (i % 2 == 0 && j % 2 != 0 && k % 2 == 0) ||
                                       (i % 2 == 0 && j % 2 == 0 && k % 2 != 0) ||
                                       (i % 2 != 0 && j % 2 != 0 && k % 2 != 0) ||
                                       (i % 2 != 0 && j % 2 == 0 && k % 2 == 0) ) isve->deleteSimpleVoxelObject(s_World->getId(), nva);
                                    else
                                    {
                                        MyBlock* newsvo = new MyBlock(rand()%11);
                                        if(!newsvo) cout<<"Error creating SVO!"<<endl;
                                        isve->setSimpleVoxelObject(s_World->getId(), nva, newsvo, true);
                                    }
                                }
                        isve->addTaskUpdateChunks();
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_R && !event.KeyInput.PressedDown) //Регенерация чанка (0,5,0) (очень плохо сделана, костыль)
                    {
                        isve->deleteChunkForRendering(s_World->getId(), Vector3d<int>(0,5,0), true);
                        isve->addChunkForRendering(s_World->getId(), Vector3d<int>(0,5,0), true);
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(2,0,0));
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(0,0,0)) ;
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(1,1,0));
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(1,-1,0));
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(1,0,1));
                        isve->getFactoryChunkMeshSceneNode()->setChunkForUpdate(s_World->getId(), Vector3d<int>(1,0,-1));
                        isve->addTaskUpdateChunks();
                        isUpdate = true;
                    }
                    if(event.KeyInput.Key == KEY_KEY_S && !event.KeyInput.PressedDown) //Создание огромной полой сферы рвдиуса rad из
                                                                                        //заданного блока (с помощью цифровых клавиш),
                                                                                        //причём отдаёт это задание в другой поток
                    {
                        CreateSphereParams* csp = new CreateSphereParams;
                        csp->s_Type = CREATE_SPHERE;
                        csp->s_IdWorld = s_World->getId();
                        csp->s_Radius = rad;
                        csp->s_TypeVoxelSet = s_TypeVoxelSet;
                        isve->addTaskRunUserControllerForThread(csp);
                    }
                    if(event.KeyInput.Key == KEY_KEY_L && !event.KeyInput.PressedDown)
                    {
                        BoomParams* bp = new BoomParams;
                        bp->s_Type = CREATE_LIGHT;
                        bp->s_HitTriangle = hitTriangle;
                        bp->s_Radius = radboom;
                        bp->s_Vct = vct;
                        bp->s_World = s_World;
                        bp->s_Campos = campos;
                        isve->addTaskRunUserControllerForThread(bp);
                    }
                    if(event.KeyInput.Key == KEY_KEY_B && !event.KeyInput.PressedDown) //Взрываем! И опять в другом потоке, дабы графический поток не подвисал
                    {
                        BoomParams* bp = new BoomParams;
                        bp->s_Type = BOOM;
                        bp->s_HitTriangle = hitTriangle;
                        bp->s_Radius = radboom;
                        bp->s_Vct = vct;
                        bp->s_World = s_World;
                        bp->s_Campos = campos;
                        isve->addTaskRunUserControllerForThread(bp);
                    }
                    if(event.KeyInput.Key == KEY_KEY_Z && !event.KeyInput.PressedDown)
                    {
                        isve->deleteChunksForRendering(s_World->getId(), Vector3d<int>(-5,0,-5), Vector3d<int>(5, 15, 5), true, false);
                    }
                }
                if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) //Реакция на щелчок мыши
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
                        isve->getAccessToTheSimpleVoxelEngine();
                        VoxelAddress* vxladdr = isve->getAddressSimpleVoxelObjectByThreePoints(s_World->getId(), a1, a2, a3, vct*mn, campos); //Получаем блок по трём точкам и вектору камеры
                        isve->freeAccessToTheSimpleVoxelEngine();
                        if(left) isve->deleteSimpleVoxelObject(s_World->getId(), vxladdr);
                        else
                        {
                            MyBlock* newsvo = new MyBlock(s_TypeVoxelSet);
                            isve->setSimpleVoxelObject(s_World->getId(), vxladdr, newsvo, true);
                        }
                        isve->addTaskUpdateChunks();
                        isUpdate = true;
                    }
                }
                return false;
        }

        virtual bool IsKeyDown(EKEY_CODE keyCode) const
        {
                return KeyIsDown[keyCode];
        }

        MyReceiver(World* wrld, IrrSimpleVoxelEngine* i, bool* gr, bool* gh):
            isUpdate(false),
            s_TypeVoxelSet(0),
            s_World(wrld),
            isve(i),
            s_Gravity(gr),
            s_Ghost(gh)
        {
                for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
                        KeyIsDown[i] = false;
        }
        Vector3d<float> campos;
        triangle3df hitTriangle; //Треугольник щелчка по блоку
        Vector3d<float> vct; //Вектор камеры
        bool isUpdate; //Надо ли обновлять чанк
        int s_TypeVoxelSet; //Какие блоки ставим
        int rad; // Радиус сферы
        int radboom; // Радиус взрыва

private:
        World* s_World;
        IrrSimpleVoxelEngine* isve;
        bool* s_Gravity; //Указатель на контроллер гравитации
        bool* s_Ghost; //Указатель на контроллер прохождения сквозь стены
        bool KeyIsDown[KEY_KEY_CODES_COUNT];
};
//...
class MyChunksSaveLoadController : public ChunksSaveLoadController
{
   public:
       MyChunksSaveLoadController(IrrSimpleVoxelEngine* eng): // Конструктор, для работы с логгером передадим в него указатель на движок.
           ChunksSaveLoadController(),
           s_Engine(eng) // Переменная движка в нашем классе.
           {
               //...
           }
           bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
           {
               unsigned int nonnsvo = cvo->getNumberOfNotNullSimpleVoxelObjects();
                std::string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
                std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк. Пусть это будет chunk_%v, где %v есть вектор чанка.
                ofstream filechunk(filename.c_str()); // Открыли файл для записи.
                if(!filechunk) // Если не смогли, то...
                {
                    WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // Создадим все папки.
                    filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // Откроем ещё раз.
                    if(!filechunk) // Если неудача, то выходим из функции сохранения.
                    {
                        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
                        return false;
                    }
                }
                filechunk << nonnsvo << " ";
                if(nonnsvo != 0)
                {
                    SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                    bool* svocachevisible = cvo->getCacheVisibleSimpleVoxelObjects();
                    bool** svocachevisiblefaces = cvo->getCacheVisibleSimpleVoxelObjectsRenderFaces();
                    int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                    for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
                    {
                        if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL)
                        {
                            filechunk << ((MyBlock*)svos[i])->getId() << " " << svocachevisible[i] << " ";
                            if(svocachevisible[i])
                                for(int j = 0; j < 6; j++) filechunk << svocachevisiblefaces[i][j] << " ";
                        }
                        else filechunk << -1 << " "; // Если нет, то запишем -1, но, по идее, все объекты должны быть "наши", т.е. иметь тип CUSTOM_VOXEL.
                    }
                }
                s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // Отправим стандартное сообщение о сохранении чанка.
                filechunk.close();
                return true;
            }
            bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
            {
                unsigned int nonnsvo = 0;
               std::string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
               std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк. Пусть это будет chunk_%v, где %v есть вектор чанка.
               ifstream filechunk(filename.c_str()); // Открыли файл для чтения.
               if(!filechunk) // Проверили, открылся ли файл.
               {
                   s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
                   return false;
               }
               int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
               int id = 0; // Сюда будем читать ID.
               MyBlock* block; // Переменная для хранения указателя на наш воксель.
               int t = 0;
               filechunk >> nonnsvo;
               if(nonnsvo != 0)
               {
                   SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                   bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Получим ещё и кэш видимости, чтобы потом устроить полный пересчёт.
                   bool* svocachevisiblenow = cvo->getCacheVisibleSimpleVoxelObjects();
                   bool** svocachevisiblefaces = cvo->getCacheVisibleSimpleVoxelObjectsRenderFaces();
                   cvo->setNumberOfNotNullSimpleVoxelObjects(nonnsvo);
                   for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
                   {
                       filechunk >> id;
                       if(id != -1)
                       {
                           filechunk >> t;
                           block = new MyBlock(id);
                           if(!block)
                           {
                               s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                               return false;
                           }
                           svos[i] = block;
                           svocachevisible[i] = false; // А вот и указываем, что надо пересчитать видимость.
                           if(t == 0)
                           {
                               svocachevisiblenow[i] = false;
                           }
                           else
                           {
                               svocachevisiblenow[i] = true;
                               if(!svocachevisiblefaces[i]) svocachevisiblefaces[i] = new bool[6];
                               for(int j = 0; j < 6; j++) filechunk >> svocachevisiblefaces[i][j];
                           }
                       }
                   }
               }
               s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // Отправим стандартное сообщение о загрузке чанка.
               filechunk.close();
               return true;
            }
            bool canLoad(World* wrld, Vector3d<int> vect)
            {
               std::string folder = s_Folders[wrld->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" хранятся файлы чанков
               std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где хранится чанк. Это будет chunk_%v, где %v есть вектор чанка.
               //while(!WorkFunctions::FileFunctions::fileExists(filename));
               if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // Если файл существует, то можем загрузить.
               return false; // Иначе не можем.
            }
            void setFolder(int id, std::string folder)
            {
                s_Folders[id] = folder;
            }
    private:
       IrrSimpleVoxelEngine* s_Engine;
       std::map< int, std::string > s_Folders;
};
/*class MyChunksSaveLoadController : public ChunksSaveLoadController
{
   public:
       MyChunksSaveLoadController(IrrSimpleVoxelEngine* eng): // Конструктор, для работы с логгером передадим в него указатель на движок.
           ChunksSaveLoadController(),
           s_Engine(eng) // Переменная движка в нашем классе.
           {
               //...
           }
           bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
           {
               return true;
                SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                std::string folder = cvo->getWorld()->getNameFolder() + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
                std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк. Пусть это будет chunk_%v, где %v есть вектор чанка.
                ofstream filechunk(filename.c_str()); // Открыли файл для записи.
                if(!filechunk) // Если не смогли, то...
                {
                    WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // Создадим все папки.
                    filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // Откроем ещё раз.
                    if(!filechunk) // Если неудача, то выходим из функции сохранения.
                    {
                        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
                        return false;
                    }
                }
                int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
                {
                    if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) filechunk << ((MyBlock*)svos[i])->getId() << " "; // Если объект есть и наш, то запишем его ID.
                    else filechunk << -1 << " "; // Если нет, то запишем -1, но, по идее, все объекты должны быть "наши", т.е. иметь тип CUSTOM_VOXEL.
                }
                s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // Отправим стандартное сообщение о сохранении чанка.
                filechunk.close();
                return true;
            }
            bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
            {
                return true;
               SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
               bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Получим ещё и кэш видимости, чтобы потом устроить полный пересчёт.
               std::string folder = cvo->getWorld()->getNameFolder() + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
               std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк. Пусть это будет chunk_%v, где %v есть вектор чанка.
               ifstream filechunk(filename.c_str()); // Открыли файл для чтения.
               if(!filechunk) // Проверили, открылся ли файл.
               {
                   s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
                   return false;
               }
               int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
               int id = 0; // Сюда будем читать ID.
               MyBlock* block; // Переменная для хранения указателя на наш воксель.
               for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
               {
                   filechunk >> id;
                   if(id != -1)
                   {
                       block = new MyBlock(id);
                       if(!block)
                       {
                           s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                           return false;
                       }
                       svos[i] = block;
                       svocachevisible[i] = true; // А вот и указываем, что надо пересчитать видимость.
                   }
               }
               s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // Отправим стандартное сообщение о загрузке чанка.
               filechunk.close();
               return true;
            }
            bool canLoad(World* wrld, Vector3d<int> vect)
            {
               std::string folder = wrld->getNameFolder() + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" хранятся файлы чанков
               std::string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где хранится чанк. Это будет chunk_%v, где %v есть вектор чанка.
               //while(!WorkFunctions::FileFunctions::fileExists(filename));
               if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // Если файл существует, то можем загрузить.
               return false; // Иначе не можем.
            }
    private:
       IrrSimpleVoxelEngine* s_Engine;
};*/
//...
/*class MyChunksSaveLoadController : public ChunksSaveLoadController
{
    public:
        MyChunksSaveLoadController(IrrSimpleVoxelEngine* eng):
            ChunksSaveLoadController(),
            s_Engine(eng)
        {
            //...
        }
        ~MyChunksSaveLoadController()
        {
            //...
        }
        bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
        {
            return true;
            SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects();
            std::string folder = getFolderChunk(vect);
            std::string filename = folder + "/SVO.chunk";
            fstream filechunk(filename.c_str(), ios::out|ios::app|ios::binary);
            if(!filechunk)
            {
                WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
                filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary);
                if(!filechunk)
                {
                    s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. Folder: " + folder);
                    return false;
                }
            }
            int numbofpart = cvo->getNumberOfPartitions();
            for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++)
            {
                if(svos[i]) filechunk << ((MyBlock*)svos[i])->getId();
            }
            filechunk.close();
            s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. Folder: " + folder);
            return true;
        }
        bool loadChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
        {
            return false;
        }
    private:
        Engine* s_Engine;
};*/

enum IDS_NODES
{
    IDFlag_IsPickable = 1,
    ID_IsNotPickable
};

int main()
{
    /*Engine* a = new Engine;
    cout << "Engine added." << endl;
    getch();
    int id1 = a->addWorld("123", 16);
    cout << "World added." << endl;
    getch();
    MyChunksSaveLoadController* aaa = new MyChunksSaveLoadController(0);
    World* w = a->getWorld(id1);
    w->setChunksSaveLoadController(aaa);
    w->loadChunks(Vector3d<int>(0,0,0), 3);
    cout << "Chunks added." << endl;
    getch();
    delete w;
    cout << "Engine deleted." << endl;
    getch();
    return 0;*/
    srand((unsigned)time(NULL));
    //IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d< u32 >(1280, 720), 32, false, false, false);
    //...
    SIrrlichtCreationParameters* s_IrrlichtCreationParameters = new SIrrlichtCreationParameters;
    s_IrrlichtCreationParameters->DeviceType = EIDT_BEST;
    s_IrrlichtCreationParameters->DriverType = EDT_OPENGL;
    s_IrrlichtCreationParameters->WindowSize = dimension2d<u32>(1280, 720);
    s_IrrlichtCreationParameters->Bits = 32;
    s_IrrlichtCreationParameters->ZBufferBits = 16;
    s_IrrlichtCreationParameters->Fullscreen = false;
    s_IrrlichtCreationParameters->Stencilbuffer = false;
    s_IrrlichtCreationParameters->Vsync = false;
    s_IrrlichtCreationParameters->AntiAlias = 8;
    s_IrrlichtCreationParameters->WithAlphaChannel = false;
    s_IrrlichtCreationParameters->Doublebuffer = true;
    s_IrrlichtCreationParameters->IgnoreInput = false;
    s_IrrlichtCreationParameters->Stereobuffer = false;
    s_IrrlichtCreationParameters->HighPrecisionFPU = false;

    IrrlichtDevice* device = createDeviceEx(*s_IrrlichtCreationParameters);
    delete s_IrrlichtCreationParameters;
    //...
    if (device == 0) return 1;
    ILogger* logger = device->getLogger();
    logger->setLogLevel(ELL_ERROR);
    IVideoDriver* driver = device->getVideoDriver();
    driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();
    device->setWindowCaption(L"Test IrrSimpleVoxelEngine program");

    smgr->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);

    // Создадим шейдеры освещения
    MyShaderCallBack* mc = new MyShaderCallBack();
    s32 chunkLightMaterialType = driver->getGPUProgrammingServices()->addHighLevelShaderMaterialFromFiles(
                                "Shaders/light.vert", "vertexMain", video::EVST_VS_1_1,
                                "Shaders/light.frag", "pixelMain", video::EPST_PS_1_1,
                                mc, video::EMT_SOLID, 0, EGSL_DEFAULT);
    s32 chunkLightTransparentMaterialTypeBlend = driver->getGPUProgrammingServices()->addHighLevelShaderMaterialFromFiles(
                                "Shaders/light.vert", "vertexMain", video::EVST_VS_1_1,
                                "Shaders/light.frag", "pixelMain", video::EPST_PS_1_1,
                                //mc, video::EMT_TRANSPARENT_ALPHA_CHANNEL, 0, EGSL_DEFAULT);
                                mc, video::EMT_ONETEXTURE_BLEND, 0, EGSL_DEFAULT);
                                //mc, video::EMT_TRANSPARENT_ALPHA_CHANNEL, 0, EGSL_DEFAULT);
    s32 chunkLightTransparentMaterialTypeRef = driver->getGPUProgrammingServices()->addHighLevelShaderMaterialFromFiles(
                                "Shaders/light.vert", "vertexMain", video::EVST_VS_1_1,
                                "Shaders/light.frag", "pixelMain", video::EPST_PS_1_1,
                                mc, video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF, 0, EGSL_DEFAULT);
    mc->drop();

    //Начинаем подключать наш движок
    Logger* lgr = new Logger; //Создали логгер для получения сообщений
    lgr->setPrintStandard(false); //Не хотим читать стандартные сообщения
    IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr); //Создали движок
    lgr->drop();
    MyController* mcntrlr = new MyController(isve); //Создали контроллер для работы в другом потоке
    isve->setUserControllerForThread(mcntrlr); //Установили контроллер движку
    MyFactoryChunkMeshSceneNode* mfcmsn = new MyFactoryChunkMeshSceneNode(device, isve); //Создали нашу фабрику нод
    mfcmsn->setPositionScale(16); //Установили масштабы позиции и растяжения
    mfcmsn->setSScale(16);
    mfcmsn->setSizeBlockChunks(1, 8, 1); //Установили блок склейки чанков в меш
    mfcmsn->setNumberOfMeshesSceneNodeForChunk(3);
    mfcmsn->setHardwareMappingHint(0, EHM_STATIC, EBT_VERTEX_AND_INDEX); //Включили hardware mapping hint
    mfcmsn->setMaterialType(0, (E_MATERIAL_TYPE)chunkLightMaterialType);
    mfcmsn->setHardwareMappingHint(1, EHM_STATIC, EBT_VERTEX_AND_INDEX);
    mfcmsn->setMaterialType(1, (E_MATERIAL_TYPE)chunkLightTransparentMaterialTypeRef);
    mfcmsn->setHardwareMappingHint(2, EHM_STATIC, EBT_VERTEX_AND_INDEX);
    mfcmsn->setMaterialType(2, (E_MATERIAL_TYPE)chunkLightTransparentMaterialTypeBlend);
    isve->setFactoryChunkMeshSceneNode(mfcmsn); //Установили фабрику движку
    MyChunksSaveLoadController* mcslc = new MyChunksSaveLoadController(isve);
    //World* wrld = new World(isve, "data/testworld");
    //int id = isve->addWorld(wrld); //Создали тестовый мир, чанк 16 на 16 на 16 блоков
    ISceneNode* rn = smgr->addEmptySceneNode();
    World3DInfo w3dinfo;
    w3dinfo.s_WorldRootNode = rn;
    vector3df a1(15.5, 88.5, -3.5);
    vector3df a2(16.5, 88.5, -3.5);
    vector3df a3(16.5, 87.5, -3.5);
    //w3dinfo.s_AmendmentPositionWorld = -(a1 + a2 + a3) / 2;
    w3dinfo.s_AmendmentPositionWorld = vector3df(-16.5f, -88.5f, 1.5f);
    //w3dinfo.s_AmendmentPositionWorld = vector3df(0,-1,0);
    int id = isve->addWorld(w3dinfo); //Создали тестовый мир, чанк 16 на 16 на 16 блоков
    //mcslc->setFolder(id, "data/test");
    mcslc->setFolder(id, "data/onlyfortest");
    MySimpleVoxelObjectTransparentTester* mysvott = new MySimpleVoxelObjectTransparentTester;
    isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(mysvott);
    isve->getWorld(id)->setChunksSaveLoadController(mcslc);
    isve->getFactoryChunkMeshSceneNode()->setMultiplePartition(id, true); //Указали, что все чанки одинаково разбиты на всех уровнях
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(0, id, EMF_LIGHTING, false); //Убрали освещение
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(0, id, EMF_BILINEAR_FILTER, false);
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(0, id, EMF_WIREFRAME, true);
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(0, id, EMF_WIREFRAME, true);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_LIGHTING, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_BLEND_OPERATION, true);
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_ZWRITE_ENABLE, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_BACK_FACE_CULLING, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_USE_MIP_MAPS, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_BILINEAR_FILTER, false);
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_COLOR_MATERIAL, false);
    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(1, id, EMF_WIREFRAME, true);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_LIGHTING, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_BLEND_OPERATION, true);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_ZWRITE_ENABLE, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_BACK_FACE_CULLING, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_COLOR_MATERIAL, false);
    isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_BILINEAR_FILTER, false);

    isve->getFactoryChunkMeshSceneNode()->setID(0, id, IDFlag_IsPickable);
    isve->getFactoryChunkMeshSceneNode()->setID(1, id, IDFlag_IsPickable);
    isve->getFactoryChunkMeshSceneNode()->setID(2, id, ID_IsNotPickable);

    //isve->getFactoryChunkMeshSceneNode()->setMaterialFlag(2, id, EMF_WIREFRAME, true);
    //isve->getFactoryChunkMeshSceneNode()->setOcclusionQueries(true);
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; //Создали наш генератор
    isve->getWorld(id)->setWorldGenerator(mywrldgen); //Отдали генератор миру
    //isve->addChunksForRendering(id, Vector3d<int>(0,0,0), Vector3d<int>(0, 0, 0), true); //Сгенерировали параллелепипед чанков.
    isve->addChunksForRendering(id, Vector3d<int>(-10,0,-10), Vector3d<int>(10, 15, 10), true); //Сгенерировали параллелепипед чанков.
    //isve->addChunksForRendering(id, Vector3d<int>(0,0,0), Vector3d<int>(0,0,0), true);


    SKeyMap keyMap[5];
    keyMap[0].Action = EKA_MOVE_FORWARD;
    keyMap[0].KeyCode = KEY_UP;

    keyMap[1].Action = EKA_JUMP_UP;
    keyMap[1].KeyCode = KEY_SPACE;

    keyMap[2].Action = EKA_MOVE_BACKWARD;
    keyMap[2].KeyCode = KEY_DOWN;

    keyMap[3].Action = EKA_STRAFE_LEFT;
    keyMap[3].KeyCode = KEY_LEFT;

    keyMap[4].Action = EKA_STRAFE_RIGHT;
    keyMap[4].KeyCode = KEY_RIGHT;

    ICameraSceneNode* camFPS = smgr->addCameraSceneNodeFPS(0, 70, 0.008, -1, keyMap, 5); //Создали нашу камеру
    //ICameraSceneNode* camFPS = smgr->addCameraSceneNode();
    camFPS->setPosition(vector3df(24.4111f, 80.3288f, 25.5249f));
    //camFPS->setPosition(vector3df(-5*16, 5.3288f, -5*16));
    //camFPS->setPosition(vector3df(24.4111f, 800.3288f, 25.5249f));
    camFPS->setTarget(vector3df(23.2238, 7.1799, -10.7250));
    camFPS->setNearValue(0.2);

    mfcmsn->camFPS = camFPS;
    mfcmsn->s_MTS = smgr->createMetaTriangleSelector();

    ISceneNodeAnimator* anim = smgr->createCollisionResponseAnimator(
                mfcmsn->s_MTS, camFPS, core::vector3df(0.4,0.9,0.4),
                core::vector3df(0,0,0), core::vector3df(0,0.5,0));
    camFPS->addAnimator(anim);

    bool gravity = false, ghost = false;

    MyReceiver myRec(isve->getWorld(id), isve, &gravity, &ghost);
    device->setEventReceiver(&myRec);

    /*if(selector)
    {
		anim = smgr->createCollisionResponseAnimator(
			selector, camFPS, core::vector3df(0.4,0.9,0.4),
			core::vector3df(0,0,0), core::vector3df(0,0.5,0));
		selector->drop();
		camFPS->addAnimator(anim);
    }*/

    scene::IBillboardSceneNode * bill = smgr->addBillboardSceneNode(); //Создаём биллбоард по аналогии из туториала про коллиизии
	bill->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
	bill->setMaterialTexture(0, driver->getTexture("media/particle.bmp"));
	bill->setMaterialFlag(video::EMF_LIGHTING, false);
	bill->setMaterialFlag(video::EMF_ZBUFFER, false);
	bill->setSize(core::dimension2d<f32>(1.0f, 1.0f));
	bill->setID(ID_IsNotPickable);
	ISceneNode* skydome = smgr->addSkyDomeSceneNode(driver->getTexture("skydome/skydome.jpg"), 256, 128, 0.9, 2, 1000);
    IGUIFont *font1=guienv->getFont("fonts/arial.xml");

    device->getCursorControl()->setVisible(false); //Выключаем курсор

    scene::ISceneCollisionManager* collMan = smgr->getSceneCollisionManager();

    video::SMaterial material;
    material.Wireframe=true;
    material.setTexture(0, 0);
	material.Lighting = false;

	/*smgr->addSkyBoxSceneNode( //Создаём скайбокс
    driver->getTexture("skybox/irrlicht2_up.jpg"),
    driver->getTexture("skybox/irrlicht2_dn.jpg"),
    driver->getTexture("skybox/irrlicht2_lf.jpg"),
    driver->getTexture("skybox/irrlicht2_rt.jpg"),
    driver->getTexture("skybox/irrlicht2_ft.jpg"),
    driver->getTexture("skybox/irrlicht2_bk.jpg"));*/

    stringw blockstr, fpsstr, gravitystr, genstr, strhlp;

    isve->start(); //Стартуем движок

    myRec.rad = 20;
    myRec.radboom = 5;

    f32 changefloat = 0.01;

    /*int t = isve->getTimeSafeThread();
    int t2;
    bool test = false, del = true;*/

    while(isve->runIrrDevice())
        if(device->isWindowActive())
        {
            //...
            /*t2 = isve->getTimeSafeThread();
            if(true && ( t2 - t > 3000 || (test && t2 - t > 50) ) )
            {
                test = true;
                //cout << "!" << endl;
                {
                    if(del)
                    {
                        VoxelAddress* vxladdr = new VoxelAddress(isve);
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        vxladdr->addNextVector(Vector3d<int>(15,15,15));
                        isve->deleteSimpleVoxelObject(id, vxladdr);
                        del = false;
                    }
                    else
                    {
                        MyBlock* svo = new MyBlock(1);
                        VoxelAddress* vxladdr = new VoxelAddress(isve);
                        vxladdr->addNextVector(Vector3d<int>(0,0,0));
                        vxladdr->addNextVector(Vector3d<int>(15,15,15));
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
            }*/
            //...
            isve->update(); //Тик движка
            /*if(myRec.isUpdate)
            {
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
            }*/
            int fps = driver->getFPS(); //Создаём строки для вывода информации
            blockstr = L"Type voxel set: ";
            blockstr += myRec.s_TypeVoxelSet + 1;
            blockstr += " (keys 1,2,3, ... , 9, 0, -)";
            fpsstr = L"FPS: ";
            fpsstr += fps;
            gravitystr = L"Gravity: ";
            if(gravity) gravitystr += "On";
            else gravitystr += "Off";
            gravitystr += " ('M' key for change)";
            genstr = L"Type moving: ";
            if(!ghost) genstr += L"stone";
            else genstr += L"ghost";
            genstr += L" ('N' key for change)";
            strhlp = L"F1 for help (not working).";
            //driver->beginScene(true,true, SColor(255,100,101,140));
            driver->beginScene(true,true, SColor(0,0,0,0));
            smgr->drawAll();
            font1->draw(blockstr,rect<s32>(10,10,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(fpsstr,rect<s32>(10,30,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(gravitystr,rect<s32>(10,50,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(genstr,rect<s32>(10,70,
            driver->getScreenSize().Height,100),SColor(255,255,255,255));
            font1->draw(strhlp,rect<s32>(10,90,
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
                myRec.campos = Vector3d<float>(camFPS->getPosition().X, camFPS->getPosition().Y, camFPS->getPosition().Z);
            }
            //...
            guienv->drawAll();
            driver->endScene();
            if(myRec.IsKeyDown(KEY_KEY_1)) myRec.s_TypeVoxelSet = 0; //Для установки типа блока
            if(myRec.IsKeyDown(KEY_KEY_2)) myRec.s_TypeVoxelSet = 1;
            if(myRec.IsKeyDown(KEY_KEY_3)) myRec.s_TypeVoxelSet = 2;
            if(myRec.IsKeyDown(KEY_KEY_4)) myRec.s_TypeVoxelSet = 3;
            if(myRec.IsKeyDown(KEY_KEY_5)) myRec.s_TypeVoxelSet = 4;
            if(myRec.IsKeyDown(KEY_KEY_6)) myRec.s_TypeVoxelSet = 5;
            if(myRec.IsKeyDown(KEY_KEY_7)) myRec.s_TypeVoxelSet = 6;
            if(myRec.IsKeyDown(KEY_KEY_8)) myRec.s_TypeVoxelSet = 7;
            if(myRec.IsKeyDown(KEY_KEY_9)) myRec.s_TypeVoxelSet = 8;
            if(myRec.IsKeyDown(KEY_KEY_0)) myRec.s_TypeVoxelSet = 9;
            if(myRec.IsKeyDown(KEY_MINUS)) myRec.s_TypeVoxelSet = 10;
            if(myRec.IsKeyDown(KEY_BACK)) myRec.s_TypeVoxelSet = 11;
            if(myRec.IsKeyDown(KEY_HOME)) myRec.s_TypeVoxelSet = 12;
            if(myRec.IsKeyDown(KEY_KEY_X)) //Установим радиус сферы
            {
                cout << "Type sphere radius (<= 70!): ";
                cin >> myRec.rad;
                if(myRec.rad > 70) myRec.rad = 70;
            }
            if(myRec.IsKeyDown(KEY_KEY_G)) //Установим радиус сферы
            {
                cout << "Type boom radius (<= 70!): ";
                cin >> myRec.radboom;
                if(myRec.radboom > 70) myRec.radboom = 70;
            }
            if(myRec.IsKeyDown(KEY_KEY_P))
            {
                mc->s_StarLightIntensity += changefloat;
                if(mc->s_StarLightIntensity > 1) mc->s_StarLightIntensity = 1;
            }
            if(myRec.IsKeyDown(KEY_OEM_1))
            {
                mc->s_StarLightIntensity -= changefloat;
                if(mc->s_StarLightIntensity < 0) mc->s_StarLightIntensity = 0;
            }
            if(myRec.IsKeyDown(KEY_KEY_U))
            {
                rn->setPosition(rn->getPosition() + vector3df(0,0.01,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_I))
            {
                rn->setPosition(rn->getPosition() + vector3df(0,-0.01,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_J))
            {
                rn->setRotation(rn->getRotation() + vector3df(0,0.05,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_K))
            {
                rn->setRotation(rn->getRotation() + vector3df(0,-0.05,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_N))
            {
                rn->setRotation(vector3df(0,90,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_A))
            {
                rn->setRotation(vector3df(0,0,0));
            }
            if(myRec.IsKeyDown(KEY_KEY_O))
            {
                rn->setRotation(vector3df(rand()%360,rand()%360,rand()%360));
            }
            if(myRec.isUpdate)
            {
                if(gravity)
                {
                    if(anim)
                    {
                        camFPS->removeAnimator(anim);
                        anim->drop();
                    }
                    anim = smgr->createCollisionResponseAnimator(
                        mfcmsn->s_MTS, camFPS, core::vector3df(0.4,0.9,0.4),
                        core::vector3df(0,-0.05,0), core::vector3df(0,0.5,0));
                    camFPS->addAnimator(anim);
                    anim->drop();
                    //gravity = true;
                }
                else
                {
                    if(anim)
                    {
                        camFPS->removeAnimator(anim);
                        anim->drop();
                    }
                    anim = smgr->createCollisionResponseAnimator(
                        mfcmsn->s_MTS, camFPS, core::vector3df(0.4,0.9,0.4),
                        core::vector3df(0,0,0), core::vector3df(0,0.5,0));
                    camFPS->addAnimator(anim);
                    anim->drop();
                    //gravity = false;
                }
                if(ghost)
                {
                    camFPS->removeAnimator(anim);
                    anim->drop();
                    anim = 0;
                }
                myRec.isUpdate = false;
            }
        }
    isve->getWorld(id)->saveAllChunks();
    isve->stop(); //Останавливаем движок
    delete isve;
    device->drop();
    return 0;
}
