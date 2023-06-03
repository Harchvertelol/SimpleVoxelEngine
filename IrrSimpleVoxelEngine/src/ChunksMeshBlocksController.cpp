#include "ChunksMeshBlocksController.h"

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;
using namespace Defines;

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

using namespace std;

SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::ChunksMeshBlocksController(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device, int id):
    s_SizeX(1),
    s_SizeY(1),
    s_SizeZ(1),
    s_Scale(1),
    s_WorldId(id),
    s_ISVE(isve),
    s_Device(device),
    s_Freeze(false),
    s_ChunkMeshSceneMaterialFlags(0),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_IDs(0),
    s_CameraPosition(vector3df(0, 0, 0))
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::~ChunksMeshBlocksController()
{
    clear();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMaterialFlag(int numbermesh, E_MATERIAL_FLAG flag, bool value)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag: \"number of mesh\" is > number of meshes");
        return;
    }
    for(unsigned int i = 0; i < s_ChunkMeshSceneMaterialFlags[numbermesh].size(); i++)
    {
        if(s_ChunkMeshSceneMaterialFlags[numbermesh][i].s_MaterialFlag == flag)
        {
            s_ChunkMeshSceneMaterialFlags[numbermesh][i].s_Value = value;
            return;
        }
    }
    MaterialFlagDefinition newmaterialflag;
    newmaterialflag.s_MaterialFlag = flag;
    newmaterialflag.s_Value = value;
    s_ChunkMeshSceneMaterialFlags[numbermesh].push_back(newmaterialflag);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::clear()
{
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    if(s_ChunkMeshSceneMaterialFlags) delete[] s_ChunkMeshSceneMaterialFlags;
    if(s_IDs) delete[] s_IDs;
    s_HardwareMappingHint = 0;
    s_BufferType = 0;
    s_MaterialType = 0;
    s_ChunkMeshSceneMaterialFlags = 0;
    s_IDs = 0;
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iter, iter2;
    for(iter = s_ChunksMeshBlocks.begin(), iter2 = s_ChunksMeshBlocks.end(); iter != iter2;)
    {
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) delete iter->second[i];
        delete[] iter->second;
        ++iter;
    }
    s_ChunksMeshBlocks.clear();
    s_ChunksMeshBlocksUpdateFlags.clear();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setSizeChunkMeshBlock(int x, int y, int z)
{
    if(s_Freeze)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size chunkmeshblock in controller: controller already frozen.");
        return;
    }
    s_SizeX = x;
    s_SizeY = y;
    s_SizeZ = z;
    s_Freeze = true;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setScale(int scale)
{
    if(s_Freeze)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting scale chunkmeshblock in controller: controller already frozen.");
        return;
    }
    s_Scale = scale;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMesh(int numbermesh, IMesh* mesh, Vector3d<int> vect)
{
    if(!isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting mesh in chunks mesh blocks controller: controller is not frozen.");
        return;
    }
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting mesh: \"number of mesh\" is > number of meshes");
        return;
    }
    Vector3d<int> vectorchunksmeshblock = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    if(!isExistsInMemoryChunksMeshBlock(vectorchunksmeshblock))
    {
        s_ChunksMeshBlocks[vectorchunksmeshblock] = new ChunksMeshBlock*[s_NumberOfMeshesSceneNodeForChunk];
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        {
            s_ChunksMeshBlocks[vectorchunksmeshblock][i] = new ChunksMeshBlock(s_ISVE, s_Device);
            s_ChunksMeshBlocks[vectorchunksmeshblock][i]->setSize(s_SizeX, s_SizeY, s_SizeZ);
            s_ChunksMeshBlocks[vectorchunksmeshblock][i]->setScale(s_Scale);
            s_ChunksMeshBlocks[vectorchunksmeshblock][i]->setHardwareMappingHint(s_HardwareMappingHint[i], s_BufferType[i]);
        }
    }
    s_ChunksMeshBlocks[vectorchunksmeshblock][numbermesh]->setMesh(mesh, vect - vectorchunksmeshblock);
    s_ChunksMeshBlocksUpdateFlags[vectorchunksmeshblock] = true;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isExistsInMemoryChunksMeshBlock(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, ChunksMeshBlock** >::iterator I = s_ChunksMeshBlocks.find(vect);
    if ( I == s_ChunksMeshBlocks.end() ) return false;
    return true;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::deleteMesh(int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(!isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting mesh in chunks mesh blocks controller: controller is not frozen.");
        return;
    }
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting mesh: \"number of mesh\" is > number of meshes");
        return;
    }
    Vector3d<int> vectorchunksmeshblock = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    if(isExistsInMemoryChunksMeshBlock(vectorchunksmeshblock) && s_ChunksMeshBlocks[vectorchunksmeshblock] && s_ChunksMeshBlocks[vectorchunksmeshblock][numbermesh])
    {
        s_ChunksMeshBlocks[vectorchunksmeshblock][numbermesh]->deleteMesh(vect - vectorchunksmeshblock);
        s_ChunksMeshBlocksUpdateFlags[vectorchunksmeshblock] = true;
    }
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::update(bool occlusionqueries)
{
    bool testdel = false;
    std::map< Vector3d<int>, bool >::iterator iter, iter2;
    for(iter = s_ChunksMeshBlocksUpdateFlags.begin(), iter2 = s_ChunksMeshBlocksUpdateFlags.end(); iter != iter2;)
    {
        testdel = true;
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        {
            s_ChunksMeshBlocks[iter->first][i]->update(s_WorldId, iter->first, occlusionqueries);
            if(!s_ChunksMeshBlocks[iter->first][i]->isEmpty()) testdel = false;
        }
        if(testdel)
        {
            for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) delete s_ChunksMeshBlocks[iter->first][i];
            delete[] s_ChunksMeshBlocks[iter->first];
            s_ChunksMeshBlocks.erase(iter->first);
        }
        else
        {
            for(int j = 0; j < s_NumberOfMeshesSceneNodeForChunk; j++)
            {
                IMeshSceneNode* tempmeshscenenode = s_ChunksMeshBlocks[iter->first][j]->getMeshSceneNode();
                if(tempmeshscenenode)
                {
                    for(unsigned int i = 0; i < s_ChunkMeshSceneMaterialFlags[j].size(); i++) tempmeshscenenode->setMaterialFlag(s_ChunkMeshSceneMaterialFlags[j][i].s_MaterialFlag, s_ChunkMeshSceneMaterialFlags[j][i].s_Value);
                    tempmeshscenenode->setMaterialType(s_MaterialType[j]);
                    tempmeshscenenode->setID(s_IDs[j]);
                    s_ISVE->getFactoryChunkMeshSceneNode()->onUpdateChunkMeshSceneNode(tempmeshscenenode, j, iter->first);
                }
            }
        }
        ++iter;
    }
    s_ChunksMeshBlocksUpdateFlags.clear();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::updateOcclusionQueries()
{
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator _iter, _iter2;
    for(_iter = s_ChunksMeshBlocks.begin(), _iter2 = s_ChunksMeshBlocks.end(); _iter != _iter2;)
    {
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) s_ChunksMeshBlocks[_iter->first][i]->testOcclusionQuerie();
        ++_iter;
    }
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::getVectorChunksMeshBlockByWorldVectorChunk(Vector3d<int> vect)
{
    /*int xcoord = 0, ycoord = 0, zcoord = 0;
    if(vect.getX() >= 0) xcoord = vect.getX() - vect.getX() % s_SizeX;
    else xcoord = vect.getX() - (s_SizeX + vect.getX() % s_SizeX);
    if(vect.getY() >= 0) ycoord = vect.getY() - vect.getY() % s_SizeY;
    else ycoord = vect.getY() - (s_SizeY + vect.getY() % s_SizeY);
    if(vect.getZ() >= 0) zcoord = vect.getZ() - vect.getZ() % s_SizeZ;
    else zcoord = vect.getZ() - (s_SizeZ + vect.getZ() % s_SizeZ);*/
    int xcoord = vect.getX();
    int ycoord = vect.getY();
    int zcoord = vect.getZ();
    while(xcoord % s_SizeX != 0) xcoord--;
    while(ycoord % s_SizeY != 0) ycoord--;
    while(zcoord % s_SizeZ != 0) zcoord--;
    return Vector3d<int>(xcoord, ycoord, zcoord);
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isFreeze()
{
    return s_Freeze;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isRenderChunk(Vector3d<int> vect)
{
    bool test = false;
    Vector3d<int> vectorblockchunks = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator II = s_ChunksMeshBlocks.find(vectorblockchunks);
    if ( II == s_ChunksMeshBlocks.end() ) return false;
    if(s_ChunksMeshBlocks[vectorblockchunks])
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
            test = test || s_ChunksMeshBlocks[vectorblockchunks][i]->isRenderChunk(vect - vectorblockchunks);
    return test;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setHardwareMappingHint(int numbermesh, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE typebuffer)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting hardware mapping hint: \"number of mesh\" is > number of meshes");
        return;
    }
    s_HardwareMappingHint[numbermesh] = mapping;
    s_BufferType[numbermesh] = typebuffer;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material type: \"number of mesh\" is > number of meshes");
        return;
    }
    s_MaterialType[numbermesh] = newtype;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setID(int numbermesh, s32 uid)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting ID: \"number of mesh\" is > number of meshes");
        return;
    }
    s_IDs[numbermesh] = uid;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setNumberOfMeshesSceneNodeForChunk(int nomsnfc)
{
    s_NumberOfMeshesSceneNodeForChunk = nomsnfc;
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    if(s_ChunkMeshSceneMaterialFlags) delete[] s_ChunkMeshSceneMaterialFlags;
    if(s_IDs) delete[] s_IDs;
    s_HardwareMappingHint = new E_HARDWARE_MAPPING[s_NumberOfMeshesSceneNodeForChunk];
    s_BufferType = new E_BUFFER_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_MaterialType = new E_MATERIAL_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_ChunkMeshSceneMaterialFlags = new vector< MaterialFlagDefinition >[s_NumberOfMeshesSceneNodeForChunk];
    s_IDs = new s32[s_NumberOfMeshesSceneNodeForChunk];
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        s_HardwareMappingHint[i] = EHM_NEVER;
        s_BufferType[i] = EBT_VERTEX_AND_INDEX;
        s_MaterialType[i] = EMT_SOLID;
        s_IDs[i] = -1;
    }
}

int SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::getNumberOfMeshesSceneNodeForChunk()
{
    return s_NumberOfMeshesSceneNodeForChunk;
}

unsigned int SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::getNumberOfRenderMeshs(Vector3d<int> vect)
{
    Vector3d<int> vectorchunksmeshblock = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    if(!isExistsInMemoryChunksMeshBlock(vectorchunksmeshblock)) return 0;
    int counter = 0;
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        if(s_ChunksMeshBlocks[vectorchunksmeshblock][i])
            if(s_ChunksMeshBlocks[vectorchunksmeshblock][i]->isRenderMesh(vect - vectorchunksmeshblock)) counter++;
    }
    return counter;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::updateCameraPosition(vector3df vect)
{
    //...
    s_CameraPosition = vect;
}
