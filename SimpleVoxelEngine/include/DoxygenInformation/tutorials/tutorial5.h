/*! \page tutorial5 ���� 5: ���������, �������� � ��������� ���������� ��� ��������� ��������
 ��� �� � ������� � ������ �� �������� �������� ������ � ���������� �������. �������,
 �������, ����� �� �������� � ���������� �����, �� �� �������� � ������. �� ��� �� �����.
 �� ����� ������, ��� �� ��� ��������. ��� ����� ��� ���� ���������� �������. � � ����
 ����� �� �������� �������� � �������� � ����������� ��� ��������� ��������. ����� ������,
 ��� ������������ �� �� �� ����� �������, ��� ���� ������������ ������, ������ � �����
 ������ ����� ���������� ����� ����������� ������. ���� �� �� ������ ������ ������� ���
 ������-�� ������, �� ���� ���� ��� ��� �����, ������ �����, ���������� ����������. �� ����
 ��� ������ � ������� ��������� (��������, IrrSimpleVoxelEngine, ��������� ��� Irrlicht,
 ���������� ���� ������) ���� �����, ��� �������� ���������� ��� ���������, � ��� ��
 ��� � ��������. ������� ���� ���� ������������� ����, �������� �� ��������� �������������.
 �����, ��� ������, � ����������� ���������� � ����������� ���:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;
 \endcode
 � ��������� ����� ����� �� �������� �����, �� � ���������� �����������, � ������ �������
 �������, ������� ����� �������� �������������� ���������� � �������. �� ��� ����������
 ���� ���-�� �������, ������� �� �������� �������������� ���������, ������� ����������
 �� VoxelInfo:
 \code
 struct MyVoxelInfo : VoxelInfo //���������, � ������� ����� ���������� ���������� ��� ��������� �������
 {
     int s_Id; // �� ���������� � ��� ������ ID �����.
 };
 \endcode
 �� � ������ ��� ����� ����� � �������������� ��������:
 \code
 class Block : public SimpleVoxelObject
 {
    public:
        Block(unsigned int id): // ����������� � �������� ID.
            SimpleVoxelObject(),
            s_Id(id)
        {
            setTypeVoxel(CUSTOM_VOXEL); // ���������� ��� CUSTOM_VOXEL.
        }
        ~Block()
        {
            //...
        }
        unsigned int getId() // ������� ��������� ID �����.
        {
            return s_Id;
        }
        VoxelInfo* getVoxelForRenderingInfo() //��������� ���������� � ������� ��� ���������.
        {
            MyVoxelInfo* vxlnf = new MyVoxelInfo; // ������� ������ ��� �������� ������ ����������.
            vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // ���������� ���������������� ��� ��� ���������� � �������.
            vxlnf->s_Id = s_Id; // ��������� ���� ID.
            return vxlnf; // ������ ��������� �� ��������� ���������.
        }
    private:
        unsigned int s_Id;
 };
 \endcode
 � ��������, ��� ��. ������� ������ ������ ��������� ���� �� ������� ������:
 \code
 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator():
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator()
        {
            //...
        }
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
        {
            for(int x = 0; x < sizechunk; x++)
                for(int y = 0; y < sizechunk; y++)
                    for(int z = 0; z < sizechunk / 2; z++)
                    {
                         Block* block = new Block(1); // ����� ID = 1.
                         if(!block)
                         {
                             chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, Block not added", true);
                             return false;
                         }
                         chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), block, true, false);
                     }
             chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generating."); // ����� �������� �������� ����� � ����� �� ���������.
             return true;
        }
 };
 \endcode
 �� � ���������� �������� ������, ���, �������� ����:
 \code
 int main()
 {
     Logger* lgr = new Logger; // ������� ������.
     Engine* sve = new Engine(lgr); // ������� ������ � ��������.
     lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
     int worldid = sve->addWorld(); // �������� ����������� ���.
     World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
     wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
     wrld->setMultiplePartition(true); // ���������� ���� ���������� ��������� �� ���� �������. ������������ ������������� �������� ������ � ������ ������. ��� ���� ������������������.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
     if(!chunk) return 1;
 \endcode
 �������, � ������ ������� ������� �� ������ ����� ���������� ��� ���������.
 ��� ����� �������� ��������� �� ��������� ������, ������� ������ ��� ���������� �� ���������
 ����� � ������� ��������� � ������� ����������� ������� ������. ��� ������ ���� �������
 ���������� �������������� �������� ���� ������� � ��������� ������ ��������, �����
 �������� � ��� �����:
 \code
 VoxelsForRendering* vxlfrrnd = chunk->getVoxelsForRenderingStatic(); // ����������, �������� ��� ����������. �� ������������ ������� ��������������� � �������. �������� �� �� ���������.
 \endcode
 �� � ������� ��������� �� ���� �������, � ����� ������ ������ � ���������� ���� ������:
 \code
     for(int i = 0; i < vxlfrrnd->getNumberOfVoxelsForRendering(); i++) // ��������� ��� ������� � ������.
     {
        VoxelForRendering* vxlfr = vxlfrrnd->getVoxelForRendering(i); // �������� i-� �������.
        Vector3d<float> vectpos = vxlfr->getPosition(); // �������� ������ (����������) ������� ������� � ������������.
        Vector3d<float> vectrot = vxlfr->getRotation(); // �������� ������ (����������) �������� ������� � ������������.
        Vector3d<float> vectscale = vxlfr->getScale(); // �������� ������ (����������) ��������������� ������� � ������������.
        bool renderfaces[6]; // ������ ��� �������� ���������� �� ��������� ������.
        for(int j = 0; j < 6; j++) renderfaces[j] = vxlfr->isRenderFace(j); // ��������� ���������� �� ��������� ������.
        // �� ���������� �� ��� ��� ������������, �� ���� ����� ������������ ����� �������� VoxelForRendering.
        VoxelInfo* vxlnfo = vxlfr->getVoxelInfo(); // �������� ���������� ��� ���������.
        if(vxlnfo && vxlnfo->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO) // ���� ��� ����������� � �������� ����������������.
        {
            MyVoxelInfo* myvxlnfo = (MyVoxelInfo*)vxlnfo; // ������������� ���� ����������.
            if(i == 0) sve->registerEvent(EVENT_TYPE_STANDARD, "(This ID get from VoxelInfo)ID = " + WorkFunctions::ConvertFunctions::itos(myvxlnfo->s_Id)); // ������� ID � ������� ����� �� ������, ������ �� ���������� ��� ���������.
        }
     }
     delete vxlfrrnd; // ������� ���������� ������, ����� �� ���� ������ ������.
     delete sve;
     return 0;
 }
 \endcode
 ���������� ��� ������� ��������� �� ������� ����������. �� �������� �� ��� ��������.
 �� ���, ����������, � ��. ���������� ����� ��������� ���� ���:
 \code
#include <SimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Voxels;
using namespace Additional;
using namespace Defines;
using namespace Debug;

struct MyVoxelInfo : VoxelInfo //���������, � ������� ����� ���������� ���������� ��� ��������� �������
{
    int s_Id; // �� ���������� � ��� ������ ID �����.
};

class Block : public SimpleVoxelObject
{
   public:
       Block(unsigned int id): // ����������� � �������� ID.
           SimpleVoxelObject(),
           s_Id(id)
       {
           setTypeVoxel(CUSTOM_VOXEL); // ���������� ��� CUSTOM_VOXEL.
       }
       ~Block()
       {
           //...
       }
       unsigned int getId() // ������� ��������� ID �����.
       {
           return s_Id;
       }
       VoxelInfo* getVoxelForRenderingInfo() //��������� ���������� � ������� ��� ���������.
       {
           MyVoxelInfo* vxlnf = new MyVoxelInfo; // ������� ������ ��� �������� ������ ����������.
           vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // ���������� ���������������� ��� ��� ���������� � �������.
           vxlnf->s_Id = s_Id; // ��������� ���� ID.
           return vxlnf; // ������ ��������� �� ��������� ���������.
       }
   private:
       unsigned int s_Id;
};

class MyWorldGenerator : public WorldGenerator
{
   public:
       MyWorldGenerator():
           WorldGenerator()
       {
           //...
       }
       ~MyWorldGenerator()
       {
           //...
       }
       bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
       {
           for(int x = 0; x < sizechunk; x++)
               for(int y = 0; y < sizechunk; y++)
                   for(int z = 0; z < sizechunk / 2; z++)
                   {
                        Block* block = new Block(1); // ����� ID = 1.
                        if(!block)
                        {
                            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, Block not added", true);
                            return false;
                        }
                        chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), block, true, false);
                    }
            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generating."); // ����� �������� �������� ����� � ����� �� ���������.
            return true;
       }
};

int main()
{
    Logger* lgr = new Logger; // ������� ������.
    Engine* sve = new Engine(lgr); // ������� ������ � ��������.
    lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
    int worldid = sve->addWorld(); // �������� ����������� ���.
    World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
    wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
    wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
    wrld->setMultiplePartition(true); // ���������� ���� ���������� ��������� �� ���� �������. ������������ ������������� �������� ������ � ������ ������. ��� ���� ������������������.
    ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
    if(!chunk) return 1;
    VoxelsForRendering* vxlfrrnd = chunk->getVoxelsForRenderingStatic(); // ����������, �������� ��� ����������. �� ������������ ������� ��������������� � �������. �������� �� �� ���������.
    for(int i = 0; i < vxlfrrnd->getNumberOfVoxelsForRendering(); i++) // ��������� ��� ������� � ������.
    {
       VoxelForRendering* vxlfr = vxlfrrnd->getVoxelForRendering(i); // �������� i-� �������.
       Vector3d<float> vectpos = vxlfr->getPosition(); // �������� ������ (����������) ������� ������� � ������������.
       Vector3d<float> vectrot = vxlfr->getRotation(); // �������� ������ (����������) �������� ������� � ������������.
       Vector3d<float> vectscale = vxlfr->getScale(); // �������� ������ (����������) ��������������� ������� � ������������.
       bool renderfaces[6]; // ������ ��� �������� ���������� �� ��������� ������.
       for(int j = 0; j < 6; j++) renderfaces[j] = vxlfr->isRenderFace(j); // ��������� ���������� �� ��������� ������.
       // �� ���������� �� ��� ��� ������������, �� ���� ����� ������������ ����� �������� VoxelForRendering.
       VoxelInfo* vxlnfo = vxlfr->getVoxelInfo(); // �������� ���������� ��� ���������.
       if(vxlnfo && vxlnfo->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO) // ���� ��� ����������� � �������� ����������������.
       {
           MyVoxelInfo* myvxlnfo = (MyVoxelInfo*)vxlnfo; // ������������� ���� ����������.
           if(i == 0) sve->registerEvent(EVENT_TYPE_STANDARD, "(This ID get from VoxelInfo)ID = " + WorkFunctions::ConvertFunctions::itos(myvxlnfo->s_Id)); // ������� ID � ������� ����� �� ������, ������ �� ���������� ��� ���������.
       }
    }
    delete vxlfrrnd; // ������� ���������� ������, ����� �� ���� ������ ������.
    delete sve;
    return 0;
}
 \endcode
 ��� � ��. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
