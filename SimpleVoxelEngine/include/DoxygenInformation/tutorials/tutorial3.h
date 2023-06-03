/*! \page tutorial3 ���� 3: ��������/���������� ���� � ���� ��� ��������
 � ���� ����� �� �������� ��������� � ��������� ���, � ����� ������� ���� ��� ��������,
 ����� �������� ������ � ���. ����� �� �������, ��������� ������������ ��� � ����������:
 \code
 #include <SimpleVoxelEngine.h>
 #include <cstring> // ��� ������ � std::string.
 #include <map> // ��� ������ � std::map.
 #include <fstream> // ��� ������ � ��������� ��������.

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 using namespace std; // ��� ������ � std::string � ��������� ��������.
 \endcode
 � ������� ������ ����� ������ ������ �������, ��� ����� ���������� ��� �� SimpleVoxelObject
 � ����� ��� CUSTOM_VOXEL. ��� �� ������ ��������� ��� �������? ����� ��� ����� �������������
 �����. ��� unsgined int:
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
 �������, ���, � ��������, ��. ������ ������� � �������� ������ ��� �������� � ����������
 ����. ��� ����� �������� �����, ������������� �� ChunksSaveLoadController:
 \code
 class MyChunksSaveLoadController : public ChunksSaveLoadController
 {
    private:
        map< int, string > s_Folders; // ����� ����� ������� ��� �����, ��� �������� ��� (�� ID ����)
    public:
        string getFolder(int worldid) // � ������� ���� ������� �� ����� �������� �������� ����� ���� �� ID ����.
        {
            return s_Folders[worldid];
        }
        void setFolder(int worldid, string folder) // � � ������� ���� ������� �� ����� ������������� ���� ��� ����� (����� ��, �� ID).
        {
            s_Folders[worldid] = folder;
        }
        MyChunksSaveLoadController(Engine* eng): // �����������, ��� ������ � �������� ��������� � ���� ��������� �� ������.
            ChunksSaveLoadController(),
            s_Engine(eng) // ���������� ������ � ����� ������.
            {
                //...
            }
 \endcode
 ������� ����� ������� � ������� �������� - ���������� � ��������. ����� � ����������.
 ��� �������� ��������� ������ ����� ����� ������� (� ��������� ��������� ��� �������� ���������).
 �� ����� � ��������� ���� ������ ���������� ��� ID �������� ����� ������, � ������ ���������� �������
 ����� ���������� 0. ���� �� ������������ ����������� �������, �� ������� -1. � ���� ������� ����������
 ���������� ��������� �� ���� � ��� ���������� � ������� ������������:
 \code
 bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
 {
 \endcode
 �������� �������������� ��� ����� �����. ����� �������, ��� ��� ����� ������������� ��� ������
 � ��������� ������/������, �� ���� �� �� ���� �� ������:
 \code
 string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" � ����� ������� ����� ������
 string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� ����� ������� ����. ����� ��� ����� chunk_%v, ��� %v ���� ������ �����.
 \endcode
 �������, ��� ���������� �� ����� ���� � ����. �� ��� ������, ���� ����� �� ����������, ��� � �����, � �������
 �� ������ ������? ��� ����� � ������� ������������ ����������� �������, ������� ������ ��� ����� �� ���������� ����, � � ��������:
 \code
    ofstream filechunk(filename.c_str()); // ������� ���� ��� ������.
    if(!filechunk) // ���� �� ������, ��...
    {
        WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // �������� ��� �����.
        filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // ������� ��� ���.
        if(!filechunk) // ���� �������, �� ������� �� ������� ����������.
        {
            s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
            return false;
        }
    }
 \endcode
 ��� �������� ������ � ������ �������� ���������������� ������� ��������� ������������. ������� ���� ������ ����� ����
 ��������� ������, ��� ���������� ����������� ��������. �������� ���. ��� ����� ������� ����� ��������� ��������, �������
 ���� � �����. ���� ��� ����� ������ ����, �� ���������� ����������, ���� ����� ����, �� �� ����������.
 \code
 int numberofnotnullsvos = cvo->getNumberOfNotNullSimpleVoxelObjects(); // �������� ����� ��������� �������� � �����
 filechunk << numberofnotnullsvos; // ������� ��� ����� � ��� ����
 if(numberofnotnullsvos != 0) // � ��� ���� ����� ���������� ������ � ��� ������, ���� � �� ���� �������
 {
 \endcode
 �������, ���� ������ � ����� � ������ ������ �����. ��� ������ ������� ������ ���������� �� ��� ������������ � �����
 SimpleVoxelObject (��� ������ ���������� ���������� � ��� �������� ����� ����� �����). ����� ����� ����� ������, ���
 ����� ������� ������� ������ ����� �����, � ����� ������ �������� �� ����� �������, ��������� � ���� ID:
 \code
    filechunk << " "; // � ��� �� ������� ������ ����� ����������� ��������� �������� � ����� � ��������� �����������
    SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // �������� ������ ���������� �� SimpleVoxelObjects.
    int numbofpart = cvo->getNumberOfPartitions(); // �������� ������ ����� �����.
    for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // � ��� � ��� ����.
    {
        if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) filechunk << ((Block*)svos[i])->getId() << " "; // ���� ������ ���� � ���, �� ������� ��� ID.
        else if(!svos[i]) filechunk << 0 << " "; // ���� ���� �������, �� ����� ����
        else filechunk << -1 << " "; // ���� ������ �� ���, �� ������� -1, ��, �� ����, ��� ������� ������ ���� "����", �.�. ����� ��� CUSTOM_VOXEL.
    }
 }
 \endcode
 ��� � ��, �������� �������� ������� � ����� � ��������� ������ �������, ������ ���� � ������ true:
 \code
    s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // �������� ����������� ��������� � ���������� �����.
    filechunk.close();
    return true;
 }
 \endcode
 ���� �������. ������� ����� ��������� ���. ��� ���� ������: ������� ������� ����, ����� ���������,
 ���� �� � �� ��������� �������, ���� ����, �� ��������� �� ����� ������ ID � ����� ������� �� �������� ��������� ���� ��������,
 ��� ��� �� ��������� ��� ������ (�, ������ ������, ��������� ����, ����� �������� ������ ������ �����). ��� ���� ���� �����.
 ���� � ���, ��� � ������ ������, ��� �������� �����, � �� � ������� ������������ (� � ����� ������ �� 100%) �� ������� ���������
 ��� �������� � ������������� ������������ ������. �� ����� ����� �� ������� getSimpleVoxelObjects() � ������� (��. API) ������ �����������
 ���������. ����� ��������� ����� ���������/������� ������� � ������� ������� createData()/deleteData().
 \code
 bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
 {
    string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" � ����� ������� ����� ������
    string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� ����� ������� ����.
                                                                                                                                                            //����� ��� ����� chunk_%v, ��� %v - ������ �����.
    ifstream filechunk(filename.c_str()); // ������� ���� ��� ������.
    if(!filechunk) // ���������, �������� �� ����.
    {
        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
        return false;
    }
    int numberofnotnullsvos = 0; // ���������� ��������� �������� � �����
    filechunk >> numberofnotnullsvos; // ��������� ���������� ��������� �������� � �����
    if(numberofnotnullsvos != 0) // ���� ���� ��������� �������, �� ������ ������ ����
    {
        cvo->setNumberOfNotNullSimpleVoxelObjects(numberofnotnullsvos); // ��������� ����� ���������� ��������� ��������. ��� ������ � ������� ��������
                                                                        // ���� �������� ���������� ������������� �������
        SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // �������� ������ ���������� �� SimpleVoxelObjects.
        bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // ������� ��� � ��� ���������, ����� ����� �������� ������ ��������.
        int numbofpart = cvo->getNumberOfPartitions(); // �������� ������ ����� �����.
        int id = 0; // ���� ����� ������ ID.
        Block* block; // ���������� ��� �������� ��������� �� ��� �������.
        for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // � ��� � ��� ����.
        {
            filechunk >> id;
            if(id != -1 && id != 0) // ���� ���� �������� � ���������
            {
                block = new Block(id);
                if(!block)
                {
                    s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                    return false;
                }
                svos[i] = block;
                svocachevisible[i] = true; // � ��� � ���������, ��� ���� ����������� ���������.
            }
        }
    }
    s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // �������� ����������� ��������� � �������� �����.
    filechunk.close();
    return true;
 }
 \endcode
 ������ ���� ������� ������� ��� �������� ����������� �������� �����. ��� ��������� ���������
 �� ���, � ������� ����� ���� � ������ (����������) ����� ������ �����:
 \code
 bool canLoad(World* wrld, Vector3d<int> vect)
 {
    string folder = s_Folders[wrld->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" �������� ����� ������
    string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� �������� ����.
                                                                                                                                                            // ��� ����� chunk_%v, ��� %v - ������ �����.
    if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // ���� ���� ����������, �� ����� ���������.
    return false; // ����� �� �����.
 }
 \endcode
 �� � ��������� �����: �������� s_Engine � ������� �����:
 \code
    private:
        Engine* s_Engine;
 };
 \endcode
 ��������� ����� ��� ��������� ���� �� �������� ����� � ���������� �����������, � ������
 ����� ������������� ���� �������, � � ����� ������� ��������� � ���, ��� ��������� ���������:
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
 ������ ������ ������� main:
 \code
 int main()
 {
     Logger* lgr = new Logger; // ������� ������.
     Engine* sve = new Engine(lgr); // ������� ������, ��� ��� ������.
     lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = sve->addWorld(wcp);
     World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
     wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
 \endcode
 �������� � ��������� ��� ���������� �������� � ���������� � ��������� �� ID ����� � ������������ (0,0,0):
 \code
     MyChunksSaveLoadController* mychunksslcntrl = new MyChunksSaveLoadController(sve); // ������� ��� ����������.
     mychunksslcntrl->setFolder(worldid, "dataworlds/testfolderworld"); // ��������� ����� ��� ������ ����
     wrld->setChunksSaveLoadController(mychunksslcntrl);
     wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
     if(!chunk) return 1;
     SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // �������� ������.
     if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // ��������� �� ������.
     Block* block = (Block*)svo; // ������� ����.
     sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
     wrld->saveAllChunks(); // �������� ��� ����� �� ����.
     delete sve; // ������� ������ (�������������� ���� � ������ �� ����������).
     return 0;
 }
 \endcode
 ��� � ��. �� ��������� ��������� ���� ���� �������� � ���������/��������� ���. ��� ����
 �� ��������� � ��������� ��� ��� ������ � ����������� ������������� ��������. ��� �������
 ��� �� ����� ��������� ������������ ������. � ���������� ��������� ���� ���:
 \code
    #include <SimpleVoxelEngine.h>
    #include <cstring> // ��� ������ � std::string.
    #include <map> // ��� ������ � std::map.
    #include <fstream> // ��� ������ � ��������� ��������.
    using namespace SimpleVoxelEngine;
    using namespace Core;
    using namespace Voxels;
    using namespace Additional;
    using namespace Defines;
    using namespace Debug;
    using namespace std; // ��� ������ � std::string � ��������� ��������.
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
    class MyChunksSaveLoadController : public ChunksSaveLoadController
    {
       private:
            map< int, string > s_Folders; // ����� ����� ������� ��� �����, ��� �������� ��� (�� ID ����)
       public:
           string getFolder(int worldid) // � ������� ���� ������� �� ����� �������� �������� ����� ���� �� ID ����.
           {
               return s_Folders[worldid];
           }
           void setFolder(int worldid, string folder) // � � ������� ���� ������� �� ����� ������������� ���� ��� ����� (����� ��, �� ID).
           {
               s_Folders[worldid] = folder;
           }
           MyChunksSaveLoadController(Engine* eng): // �����������, ��� ������ � �������� ��������� � ���� ��������� �� ������.
               ChunksSaveLoadController(),
               s_Engine(eng) // ���������� ������ � ����� ������.
               {
                   //...
               }
            bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
            {
                string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" � ����� ������� ����� ������
                string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� ����� ������� ����.
                                                                                                                                                                        // ����� ��� ����� chunk_%v, ��� %v - ������ �����.
                ofstream filechunk(filename.c_str()); // ������� ���� ��� ������.
                if(!filechunk) // ���� �� ������, ��...
                {
                    WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // �������� ��� �����.
                    filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // ������� ��� ���.
                    if(!filechunk) // ���� �������, �� ������� �� ������� ����������.
                    {
                        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
                        return false;
                    }
                }
                int numberofnotnullsvos = cvo->getNumberOfNotNullSimpleVoxelObjects(); // �������� ����� ��������� �������� � �����
                filechunk << numberofnotnullsvos; // ������� ��� ����� � ��� ����
                if(numberofnotnullsvos != 0) // � ��� ���� ����� ���������� ������ � ��� ������, ���� � �� ���� �������
                {
                    filechunk << " "; // � ��� �� ������� ������ ����� ����������� ��������� �������� � ����� � ��������� �����������
                    SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // �������� ������ ���������� �� SimpleVoxelObjects.
                    int numbofpart = cvo->getNumberOfPartitions(); // �������� ������ ����� �����.
                    for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // � ��� � ��� ����.
                    {
                        if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) filechunk << ((Block*)svos[i])->getId() << " "; // ���� ������ ���� � ���, �� ������� ��� ID.
                        else if(!svos[i]) filechunk << 0 << " "; // ���� ���� �������, �� ����� ����
                        else filechunk << -1 << " "; // ���� ������ �� ���, �� ������� -1, ��, �� ����, ��� ������� ������ ���� "����", �.�. ����� ��� CUSTOM_VOXEL.
                    }
                }
                s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // �������� ����������� ��������� � ���������� �����.
                filechunk.close();
                return true;
            }
            bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
            {
               string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" � ����� ������� ����� ������
               string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� ����� ������� ����.
                                                                                                                                                                       // ����� ��� ����� chunk_%v, ��� %v - ������ �����.
               ifstream filechunk(filename.c_str()); // ������� ���� ��� ������.
               if(!filechunk) // ���������, �������� �� ����.
               {
                   s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
                   return false;
               }
               int numberofnotnullsvos = 0; // ���������� ��������� �������� � �����
               filechunk >> numberofnotnullsvos; // ��������� ���������� ��������� �������� � �����
               if(numberofnotnullsvos != 0) // ���� ���� ��������� �������, �� ������ ������ ����
               {
                   cvo->setNumberOfNotNullSimpleVoxelObjects(numberofnotnullsvos); // ��������� ����� ���������� ��������� ��������. ��� ������ � ������� ��������
                                                                                   // ���� �������� ���������� ������������� �������
                   SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // �������� ������ ���������� �� SimpleVoxelObjects.
                   bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // ������� ��� � ��� ���������, ����� ����� �������� ������ ��������.
                   int numbofpart = cvo->getNumberOfPartitions(); // �������� ������ ����� �����.
                   int id = 0; // ���� ����� ������ ID.
                   Block* block; // ���������� ��� �������� ��������� �� ��� �������.
                   for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // � ��� � ��� ����.
                   {
                       filechunk >> id;
                       if(id != -1 && id != 0) // ���� ���� �������� � ���������
                       {
                           block = new Block(id);
                           if(!block)
                           {
                               s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                               return false;
                           }
                           svos[i] = block;
                           svocachevisible[i] = true; // � ��� � ���������, ��� ���� ����������� ���������.
                       }
                   }
               }
               s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // �������� ����������� ��������� � �������� �����.
               filechunk.close();
               return true;
            }
            bool canLoad(World* wrld, Vector3d<int> vect)
            {
               string folder = s_Folders[wrld->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // �������� ����� ����. � � �������� "chunks" �������� ����� ������
               string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // � ��� � �������� �����, ��� �������� ����. ��� ����� chunk_%v, ��� %v ���� ������ �����.
               if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // ���� ���� ����������, �� ����� ���������.
               return false; // ����� �� �����.
            }
        private:
            Engine* s_Engine;
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
        Engine* sve = new Engine(lgr); // ������� ������, ��� ��� ������.
        lgr->drop(); // ��� ����� ������ ������ �� �����, ������� ��� �� ����������� ������.
        WorldCreatingParameters wcp;
        wcp.s_SizeChunk = 16;
        int worldid = sve->addWorld(wcp);
        World* wrld = sve->getWorld(worldid); // �������� ��� �� ��������������.
        MyWorldGenerator* mywrldgen = new MyWorldGenerator; // ������� ��� ���������.
        wrld->setWorldGenerator(mywrldgen); // ���������� ���� ��� ���������.
        MyChunksSaveLoadController* mychunksslcntrl = new MyChunksSaveLoadController(sve); // ������� ��� ����������.
        mychunksslcntrl->setFolder(worldid, "dataworlds/testfolderworld"); // ��������� ����� ��� ������ ����
        wrld->setChunksSaveLoadController(mychunksslcntrl);
        wrld->loadChunk(Vector3d<int>(0,0,0)); // ��������� ����.
        ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // �������� ����.
        if(!chunk) return 1;
        SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // �������� ������.
        if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // ��������� �� ������.
        Block* block = (Block*)svo; // ������� ����.
        sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
        wrld->saveAllChunks(); // �������� ��� ����� �� ����.
        delete sve; // ������� ������ (�������������� ���� � ������ �� ����������).
        return 0;
    }
 \endcode
 �� ���� ��� ��. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
