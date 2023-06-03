#include "ChunksMeshBlock.h"

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;
using namespace Defines;
using namespace IrrSVEDefines;

using namespace irr;
using namespace scene;
using namespace core;
using namespace video;

SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::ChunksMeshBlock(IrrSimpleVoxelEngine* isve, IrrlichtDevice* device):
    s_SizeX(0),
    s_SizeY(0),
    s_SizeZ(0),
    s_IMeshes(0),
    s_AddMeshesFlags(0),
    s_UpdateIMeshesFlag(false),
    s_MeshSceneNode(0),
    s_ISVE(isve),
    s_WaitUpdate(false),
    s_Device(device),
    s_Scale(1),
    s_NumberOfMeshes(0),
    s_HardwareMappingHint(EHM_NEVER),
    s_BufferType(EBT_VERTEX_AND_INDEX)
{
    s_Mesh = new CBatchingMesh;
}

SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::~ChunksMeshBlock()
{
    if(s_IMeshes)
    {
        for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++) if(s_IMeshes[i]) s_IMeshes[i]->drop();
        delete[] s_IMeshes;
    }
    if(s_AddMeshesFlags) delete[] s_AddMeshesFlags;
    if(s_MeshSceneNode) s_MeshSceneNode->remove();
    if(s_Mesh) s_Mesh->drop();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setScale(int scale)
{
    s_Scale = scale;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setSize(int x, int y, int z)
{
    if(s_IMeshes && s_AddMeshesFlags)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting ChunkMeshBlock size: size already setted.");
        return;
    }
    s_SizeX = x;
    s_SizeY = y;
    s_SizeZ = z;
    s_IMeshes = new IMesh*[s_SizeX * s_SizeY * s_SizeZ];
    s_AddMeshesFlags = new bool[s_SizeX * s_SizeY * s_SizeZ];
    for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++)
    {
        s_IMeshes[i] = 0;
        s_AddMeshesFlags[i] = false;
    }
}

int SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getNumberInMemoryIMeshByVector(Vector3d<int> vect)
{
    int numberinmemory = vect.getX() + s_SizeX*vect.getY() + s_SizeX*s_SizeY*vect.getZ();
    if(numberinmemory < 0 || numberinmemory >= s_SizeX*s_SizeY*s_SizeZ)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error: vector is not in the block chunks. Vector: " + vect.toString(", "));
        return 0;
    }
    return numberinmemory;
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getVectorIMeshByNumberInMemory(int numb)
{
    int temp1 = numb % (s_SizeX*s_SizeY);
    int temp2 = temp1 % s_SizeX;
    int z = (numb - temp1) / (s_SizeX*s_SizeY);
    int y = (temp1 - temp2) / s_SizeX;
    int x = temp2;
    return Vector3d<int>(x, y, z);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setMesh(IMesh* mesh, Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory])
    {
        s_IMeshes[numberinmemory]->drop();
        s_UpdateIMeshesFlag = true;
        s_NumberOfMeshes--;
    }
    else s_AddMeshesFlags[numberinmemory] = true;
    s_IMeshes[numberinmemory] = mesh;
    s_NumberOfMeshes++;
    s_WaitUpdate = true;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setHardwareMappingHint(E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE typebuffer)
{
    s_HardwareMappingHint = mapping;
    s_BufferType = typebuffer;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::deleteMesh(Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory])
    {
        s_IMeshes[numberinmemory]->drop();
        s_IMeshes[numberinmemory] = 0;
        s_UpdateIMeshesFlag = true;
        s_NumberOfMeshes--;
        s_WaitUpdate = true;
    }
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::update(int id, Vector3d<int> vect, bool occlusionqueries)
{
    if(!s_WaitUpdate) return;
    Vector3d<int> tempvect;
    if(s_UpdateIMeshesFlag || s_HardwareMappingHint != EHM_NEVER)
    {
        s_Mesh->clear();
        for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++)
        {
            if(s_IMeshes[i])
            {
                tempvect = getVectorIMeshByNumberInMemory(i);
                s_Mesh->addMesh(s_IMeshes[i], vector3df(tempvect.getX() * s_ISVE->getWorld(id)->getSizeChunk(), tempvect.getY() * s_ISVE->getWorld(id)->getSizeChunk(), tempvect.getZ() * s_ISVE->getWorld(id)->getSizeChunk()));
            }
        }
    }
    else
    {
        for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++)
        {
            if(s_AddMeshesFlags[i] && s_IMeshes[i])
            {
                s_AddMeshesFlags[i] = false;
                tempvect = getVectorIMeshByNumberInMemory(i);
                s_Mesh->addMesh(s_IMeshes[i], vector3df(tempvect.getX() * s_ISVE->getWorld(id)->getSizeChunk(), tempvect.getY() * s_ISVE->getWorld(id)->getSizeChunk(), tempvect.getZ() * s_ISVE->getWorld(id)->getSizeChunk()));
            }
        }
    }
    if(s_MeshSceneNode) s_MeshSceneNode->remove();
    if(s_HardwareMappingHint != EHM_NEVER) s_Mesh->setHardwareMappingHint(s_HardwareMappingHint, s_BufferType);
    s_Mesh->update();
    s_Mesh->finalize();
    int sizechunk = s_ISVE->getWorld(id)->getSizeChunk();
    s_MeshSceneNode = s_Device->getSceneManager()->addMeshSceneNode(s_Mesh);
    if(!s_MeshSceneNode)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding IMeshBlockChunksSceneNode.", true);
        return;
    }
    World3DInfo w3dinfo = s_ISVE->getFactoryChunkMeshSceneNode()->getWorld3DInfo(id);
    ISceneNode* rootnode = w3dinfo.s_WorldRootNode;
    if(rootnode) rootnode->addChild(s_MeshSceneNode);
    s_MeshSceneNode->setPosition(vector3df(s_Scale / 2, s_Scale / 2, s_Scale / 2) + vector3df(vect.getX() * sizechunk, vect.getY() * sizechunk, vect.getZ() * sizechunk) + w3dinfo.s_AmendmentPositionWorld);
    if(occlusionqueries) s_Device->getVideoDriver()->addOcclusionQuery(s_MeshSceneNode, (IMesh*)s_Mesh);
    s_UpdateIMeshesFlag = false;
    s_WaitUpdate = false;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::testOcclusionQuerie()
{
    bool nodeVisible = s_Device->getVideoDriver()->getOcclusionQueryResult(s_MeshSceneNode) > 0;
    s_MeshSceneNode->setVisible(nodeVisible);
    return nodeVisible;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isWaitUpdate()
{
    return s_WaitUpdate;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isEmpty()
{
    if(s_NumberOfMeshes == 0) return true;
    return false;
}

unsigned int SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getNumberOfMeshes()
{
    return s_NumberOfMeshes;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory]) return true;
    return false;
}

IMeshSceneNode* SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getMeshSceneNode()
{
    return s_MeshSceneNode;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isRenderMesh(Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(!s_IMeshes[numberinmemory]) return false;
    return true;
}
