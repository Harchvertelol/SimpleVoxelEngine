/*! \page tutorial4 ���� 4: ���������������� ������������ � ��������� �������� � ���������� ������������
 � ���� ����� �� ������ ��� ��, ��� ���������� �����-���� ���������� ������� (����� �� ���������
 �����), ����� ������������ ������������, � ������ ����� ���������� ��������. �� ��������� ������
 ��� ��������� ��������� ������ ���������� ��, ��� ����� ������ �� �����, ����� ������� ������ ���.
 �� ���� �� ����� �������� ���� ���� ��������, ��������, ������? ��� ��� �� ������� ����� �����
 ������� � ������� ���������? �� ��� ���� ������� ������� ���������� ����� � ������, ��� ��� ���
 ������ �� �����. � ��� ��� �� ������ �������� SimpleVoxelObjectTransparentTester. ������� �������������
 ��. ����� ���������� - ��������� ������ � ����������� ������������ ���:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;
 \endcode
 ������ �� �������� ����� �������� �����:
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
    private:
        unsigned int s_Id;
 };
 \endcode
 � ������ ������ ��� ����� ��� �������� �� ������������, ���������� ��� �� SimpleVoxelObjectTransparentTester:
 \code
 class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester
 {
    public:
        MySimpleVoxelObjectTransparentTester(): // ����������� �����������.
            SimpleVoxelObjectTransparentTester()
        {
            //...
        }
 \endcode
 � ������ ������� ��� �������, ������� � �������� �� �������� �� ������������. �������� �����
 ���������: ����� � ��� ���� ��� ���� ������. ID = 1, 2, 3. ID = 1 ����� �����, ID = 2 ����� �������,
 ID = 3 ����� �����. �� �����, ����� ������ ���� ������� ������ ����� ���������, �� �� �������
 �� ������� �� ���������, ��� ��� �� ����� ������ ���� ������ �����. ������ �� ��� � ������� �����:
 \code
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent = 0)
     {
        Block* block = 0; // ��������� �� ��� ����.
        Block* parentblock = 0; // ��������� �� ����, �� �������� ��� �������� �� ������������.
        if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false; // ���� ��� ������� �� ���, �� ������ ��� ������������.
        else block = (Block*)svo; // ������������� ��� ���������.
        if(parent->getTypeVoxel() == CUSTOM_VOXEL) parentblock = (Block*)parent; // ������������� ��� ��������� ������ � ��� ������, ���� �� �������������.
        unsigned int id = block->getId(); // ��������� ID ������������ �����.
        unsigned int idparent = 0; // �������� �� ��������� ID �������� 0.
        if(parentblock) idparent = parentblock->getId(); // ���� �������� ������ ����, �� �������� ��� ID.
        if(id == 2) // ������������� ID = 2.
        {
            if(idparent == 2) return false; // ���� � �����, �� �������� �������, ID ���� 2, �� �����������.
            return true; // � ��������� ������� ���������.
        }
        if(id == 3) // ������������� ID = 3.
        {
            if(idparent == 3) return false; // ���� � �����, �� �������� �������, ID ���� 3, �� �����������.
            return true; // � ��������� ������� ���������.
        }
        return false; // ��� ��������� �����������.
     }
 };
 \endcode
 ��� � ��. ������� ������ ������ ��������� ���� �� �������� �����:
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
 �� � ������� ��������� � ������������ ����� ������������:
 \code
 int main()
 {
    Logger* lgr = new Logger; // ������� ������.
    Engine* sve = new Engine(lgr); // ������� ������ � ��������.
    lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
    int worldid = sve->addWorld(); // �������� ����������� ���.
    World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
    MySimpleVoxelObjectTransparentTester* msvott = new MySimpleVoxelObjectTransparentTester; // ������� ��� ��������� ��� �������� �� ������������.
    wrld->setSimpleVoxelObjectTransparentTester(msvott); // ���������� ��� ����� ��� �������� �� ������������ ����.
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
    wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
    wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
 \endcode
 �������, �� ��������. ������ ������� ������� �� ������ ����� ����� � ������ ��� ���������� ���������
 �������� � ���������� ������������. ��� ������ VoxelAddress. �� ������������ ����� ������������������
 ��������, ������ (������, �������) �� ������� ��������� �� ���������� �����, ������ (������, ������) - ��
 ���������� ���������� ������� � ������� � �����, ��������� - ����� � �� � ��� �����, �� ������� ������
 ������. � ��� ���� �� �� ������ ������ ����������, ������� ��� VoxelAddress ����� �������� �� ���� ��������.
 ������ ����� ������������ �����, � ������ ������������ ������� � �����. ������� ������� �� ������ �����
 ������� � ������������ (1,1,1). ��� ����� �������� VoxelAddress � ������� � ���� ��� ������� ���������:
 \code
 VoxelAddress vxladdr(sve); // ������� ��� ���������� �����, ��� ��� ��������� �� ������.
 vxladdr.addNextVector(Vector3d<int>(0,0,0)); // ���������� ������ �����.
 vxladdr.addNextVector(Vector3d<int>(1,1,1)); // ���������� ������ �������.
 \endcode
 �� � ������ �������� � ���� ������� � ������ ������� � ������� ��� ID:
 \code
 SimpleVoxelObject* svo = wrld->getSimpleVoxelObject(&vxladdr, false); // ��������� � ���� ������ �� ������ ������, �������� �������� �������������� � ������ ��� ����������.
 if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // ��������� �� ������.
 Block* block = (Block*)svo; // ������� ����.
 sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
 \endcode
 �� � ������ ������� ������ ������� ����� �� ������� ������, ����� ����� ������ ������:
 \code
    block = new Block(2); // ������� ��� ����� ����.
    wrld->setSimpleVoxelObject(&vxladdr, block, true); // ���������� ��� ���� �� ������� ������, ������ ���������� ������� � ������ �������.
    delete sve; // ������� ������.
    return 0;
 }
 \endcode
 �� � ���������� ���� ���:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

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
    private:
        unsigned int s_Id;
 };

 class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester
 {
    public:
         MySimpleVoxelObjectTransparentTester(): // ����������� �����������.
             SimpleVoxelObjectTransparentTester()
         {
             //...
         }
         bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent = 0)
         {
            Block* block = 0; // ��������� �� ��� ����.
            Block* parentblock = 0; // ��������� �� ����, �� �������� ��� �������� �� ������������.
            if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false; // ���� ��� ������� �� ���, �� ������ ��� ������������.
            else block = (Block*)svo; // ������������� ��� ���������.
            if(parent->getTypeVoxel() == CUSTOM_VOXEL) parentblock = (Block*)parent; // ������������� ��� ��������� ������ � ��� ������, ���� �� �������������.
            unsigned int id = block->getId(); // ��������� ID ������������ �����.
            unsigned int idparent = 0; // �������� �� ��������� ID �������� 0.
            if(parentblock) idparent = parentblock->getId(); // ���� �������� ������ ����, �� �������� ��� ID.
            if(id == 2) // ������������� ID = 2.
            {
                if(idparent == 2) return false; // ���� � �����, �� �������� �������, ID ���� 2, �� �����������.
                return true; // � ��������� ������� ���������.
            }
            if(id == 3) // ������������� ID = 3.
            {
                if(idparent == 3) return false; // ���� � �����, �� �������� �������, ID ���� 3, �� �����������.
               return true; // � ��������� ������� ���������.
            }
            return false; // ��� ��������� �����������.
         }
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
     MySimpleVoxelObjectTransparentTester* msvott = new MySimpleVoxelObjectTransparentTester; // ������� ��� ��������� ��� �������� �� ������������.
     wrld->setSimpleVoxelObjectTransparentTester(msvott); // ���������� ��� ����� ��� �������� �� ������������ ����.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
     wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
     VoxelAddress vxladdr(sve);
     vxladdr.addNextVector(Vector3d<int>(0,0,0)); // ���������� ������ �����.
     vxladdr.addNextVector(Vector3d<int>(1,1,1)); // ���������� ������ �������.
     SimpleVoxelObject* svo = wrld->getSimpleVoxelObject(&vxladdr, false); // ��������� � ���� ������ �� ������ ������, �������� �������� �������������� � ������ ��� ����������.
     if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // ��������� �� ������.
     Block* block = (Block*)svo; // ������� ����.
     sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
     block = new Block(2); // ������� ��� ����� ����.
     wrld->setSimpleVoxelObject(&vxladdr, block, true); // ���������� ��� ���� �� ������� ������, ������ ���������� ������� � ������ �������.
     delete sve; // ������� ������ (�������������� ���� � ������, ���� �� ������� - ����� ��������� ��� ���������� ����������, ���������� ����� ��������� � �����, ����� �� �� ���������).
     return 0;
 }
 \endcode
 ��� ���� � ��. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
