/*! \page tutorial2 ���� 2: ��������� ���� � ������� ������� �����.
 � ������� ����� �� ������� ���, �� �� ��� ������. ��� �� �������, �� �� �����
 ��� ���-�� ���������, �����? � ��� �� ������ � ��� �������� ��������� ����.
 �������, ����������, � ������� ��� ���������. ����� ����������, ���������
 ���� ������������ �����, � ��� �� ��������� ������������ ���, ����� �� ������
 �� ������ ��� ������:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 \endcode
 �� ��� ������ � ������� ������� �����, ����� ���������� ������, �� � ������
 ������� �� ���������� �� ������ ��������. ������� ��������� ������� ������� �����,
 �� ��������� ������������ ��� Debug:
 \code
 using namespace Debug;
 \endcode
 ��� ������� � ���������� ������ - ������ �����, � ������ ������� ����������� ����.
 ��� ����� �������� �����, ����������� ��� �� WorldGenerator, ������� ��������
 ����������� �������:
 \code
 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator(): // ����������� �����������, �������� ����������� ������.
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator() // ����������� ����������, ������ �� ������.
        {
            //...
        }
 \endcode
 � ������ ������ ����� ��� ��������� ����� ������� ������� - ������� ��������� �����,
 ��� ��������� ��������� �� ����, ������� �� ����������, � ������ ����� �����. �������
 �������� ��� ���������� ����� ��������� ������ �����:
 \code
 bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
 {
     for(int x = 0; x < sizechunk; x++)
        for(int y = 0; y < sizechunk; y++)
            for(int z = 0; z < sizechunk / 2; z++) // ����� �� 2, ����� ������ �������� ����� ���������.
            {
                ARGBColorVoxel* argbcolorvoxel = new ARGBColorVoxel(ARGBColor(255,255,255,255));
 \endcode
 �� � ����� � ��� ��������� �����-�� ������ � ������� �� ��������? ������� �������� ���, � � ������
 ������ ������� ������� ����������� ��������� �� ������:
 \code
     if(!argbcolorvoxel)
     {
         chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, ARGBColorVoxel not added", true); // ��������� �������� ��������, ��� ������ ��������
                                                                                                                                                 // � ������������� ������� ���������.
         return false;
     }
     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), argbcolorvoxel, true, false); // true ��������, ��� �� ����������� ������, ���� �� ����������, � ��������� false ��������,
                                                                                       // ��� �� �� ����� �������� ������ ������������� ��������� ���������� ��� ��������, ��� ���
                                                                                       // ��� ��������� ���� ��� �� ����� � ����� � ���, ��� ������ ������� � ��� ����� ����������.
                                                                                       // ��� ���������� ������ ������ ���� �� ��������� ���������� ��� ��������� ����� �����
                                                                                       // ��������� ���� ������ ������ ����, ����� ����� ������.
 }
 \endcode
 ��� � ��, ����� true � ������� �������� ������:
 \code
            return true;
        }
 };
 \endcode
 ������� ���, �������, ������, ��� �� �������� ������ �����, ������� �� ����������? ���� � ���, ���
 � ������ ChunkVoxelObject ���� ����� getVoxelAddress(), ������� ����� ��� ����� ����� � ����, �, �����
 ������ ������ �����, �� ������ �������� ������� ������ (chunk->getVoxelAddress()->getVector(0)). ���������
 �� ��������� �������� �� ��������� � ��������� ������, � ���� ������ � ���� ������.
 �� � ���� �� ����� - ������� ������ � ���������� � ���� ��������� ��������� � ������ � ������� ����,
 �� ��� ���� �� �� ����� ������ ����������� ��������� �������, ��� �� ��� �������� ������ � ��� �����,
 ������� �������� ��, �� ��������� ��� � ������ �����:
 \code
 int main()
 {
    Logger* lgr = new Logger; // ������� ������.
    lgr->setPrintStandard(false); // ��������� ����� ����������� ���������.
    Engine* sve = new Engine(lgr); // ������� ������, ��� ��� ������.
    lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
    WorldCreatingParameters wcp;
    wcp.s_SizeChunk = 16;
    int worldid = sve->addWorld(wcp);
    World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
 \endcode
 � ��� ��� �� �������� � ��������� ��� ��������� � �������� ����, �� ����� ������������ � �������
 ������ ����������:
 \code
 MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
 wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
 wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
 \endcode
 � ������� �������� �������� ������ ����������: ������� ������� � ������������ (0, 0, 0) � �������
 ��� ���� �� ����� � ������� �������, � ��� ��� ������ ������������ ������ std::string, �� ��������
 ����������� �������, ������ � �������, ������� int ����������� � std::string.
 \code
 ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
 if(!chunk) return 1;
 SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // �������� ������.
 if(!svo || svo->getTypeVoxel() != ARGBCOLOR) return 1; // ��������� �� ������.
 ARGBColorVoxel* argbcolorvoxel = (ARGBColorVoxel*)svo; // ������� ����.
 int a = argbcolorvoxel->getARGBColor().getA(); // �������� ����� �������.
 int r = argbcolorvoxel->getARGBColor().getR();
 int g = argbcolorvoxel->getARGBColor().getG();
 int b = argbcolorvoxel->getARGBColor().getB();
 // ������ ��� WARNING, ����� ������� ���������, ��� ��� ����������� �� ���������.
 sve->registerEvent(EVENT_TYPE_WARNING, WorkFunctions::ConvertFunctions::itos(a) + ", " + WorkFunctions::ConvertFunctions::itos(r) + ", " + WorkFunctions::ConvertFunctions::itos(g) + ", " + WorkFunctions::ConvertFunctions::itos(b));
 \endcode
 �� ��� � ��, �������� ������� ������ � ��������� ���������:
 \code
    delete sve;
    return 0;
 }
 \endcode
 �� � ���������� ��� ��� ������ ���:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator(): // ����������� �����������, �������� ����������� ������.
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator() // ����������� ����������, ������ �� ������.
        {
            //...
        }
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
        {
            for(int x = 0; x < sizechunk; x++)
               for(int y = 0; y < sizechunk; y++)
                   for(int z = 0; z < sizechunk / 2; z++) // ����� �� 2, ����� ������ �������� ����� ���������.
                   {
                       ARGBColorVoxel* argbcolorvoxel = new ARGBColorVoxel(ARGBColor(255,255,255,255));
                        if(!argbcolorvoxel)
                        {
                            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, ARGBColorVoxel not added", true); // ��������� �������� ��������, ��� ������ ��������
                                                                                                                                                                    // � ������������� ������� ���������.
                            return false;
                        }
                        chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), argbcolorvoxel, true, false); // true ��������, ��� �� ����������� ������, ���� �� ����������, � ��������� false ��������,
                                                                                                          // ��� �� �� ����� �������� ������ ������������� ��������� ���������� ��� ��������, ��� ���
                                                                                                          // ��� ��������� ���� ��� �� ����� � ����� � ���, ��� ������ ������� � ��� ����� ����������.
                                                                                                          // ��� ���������� ������ ������ ���� �� ��������� ���������� ��� ��������� ����� �����
                                                                                                          // ��������� ���� ������ ������ ����, ����� ����� ������.
                    }
             return true;
        }
 };

 int main()
 {
     Logger* lgr = new Logger; // ������� ������.
     lgr->setPrintStandard(false); // ��������� ����� ����������� ���������.
     Engine* sve = new Engine(lgr); // ������� ������, ��� ��� ������.
     lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = sve->addWorld(wcp);
     World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
     wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
     if(!chunk) return 1;
     SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // �������� ������.
     if(!svo || svo->getTypeVoxel() != ARGBCOLOR) return 1; // ��������� �� ������.
     ARGBColorVoxel* argbcolorvoxel = (ARGBColorVoxel*)svo; // ������� ����.
     int a = argbcolorvoxel->getARGBColor().getA(); // �������� ����� �������.
     int r = argbcolorvoxel->getARGBColor().getR();
     int g = argbcolorvoxel->getARGBColor().getG();
     int b = argbcolorvoxel->getARGBColor().getB();
     // ������ ��� WARNING, ����� ������� ���������, ��� ��� ����������� �� ���������.
     sve->registerEvent(EVENT_TYPE_WARNING, WorkFunctions::ConvertFunctions::itos(a) + ", " + WorkFunctions::ConvertFunctions::itos(r) + ", " + WorkFunctions::ConvertFunctions::itos(g) + ", " + WorkFunctions::ConvertFunctions::itos(b));
     delete sve;
     return 0;
 }
 \endcode
 � �����, ��� ���� � �� �� ���� ���. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
