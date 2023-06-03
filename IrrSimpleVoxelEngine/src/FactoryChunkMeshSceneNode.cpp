#include "FactoryChunkMeshSceneNode.h"

#include "IrrSimpleVoxelEngine.h"

#include "IrrSVEDefines.h"

using namespace SimpleVoxelEngine;
using namespace Additional;
using namespace Core;
using namespace Voxels;
using namespace Defines;
using namespace IrrEngine;
using namespace IrrSVEDefines;

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

using namespace std;

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device, IrrSimpleVoxelEngine* isve):
    s_Device(device),
    s_ISVE(isve),
    s_Freeze(false),
    s_ChunksMesh(0),
    s_PositionScale(1),
    s_RotationScale(1),
    s_SScale(1),
    s_IsFaces(true),
    s_SizeBlockChunksX(0),
    s_SizeBlockChunksY(0),
    s_SizeBlockChunksZ(0),
    s_OcclusionQueries(false),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_TimerOcclusionQueriesInMilliseconds(TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS),
    s_RememberTimeForOcclusionQueriesInMilliseconds(0),
    s_NumberOfMeshesSceneNodeForChunk(1)
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device):
    s_Device(device),
    s_ISVE(0),
    s_Freeze(false),
    s_ChunksMesh(0),
    s_PositionScale(1),
    s_RotationScale(1),
    s_SScale(1),
    s_IsFaces(true),
    s_SizeBlockChunksX(0),
    s_SizeBlockChunksY(0),
    s_SizeBlockChunksZ(0),
    s_OcclusionQueries(false),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_TimerOcclusionQueriesInMilliseconds(TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS),
    s_RememberTimeForOcclusionQueriesInMilliseconds(0),
    s_NumberOfMeshesSceneNodeForChunk(1)
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::FactoryChunkMeshSceneNode():
    s_Device(0),
    s_ISVE(0),
    s_Freeze(false),
    s_ChunksMesh(0),
    s_PositionScale(1),
    s_RotationScale(1),
    s_SScale(1),
    s_IsFaces(true),
    s_SizeBlockChunksX(0),
    s_SizeBlockChunksY(0),
    s_SizeBlockChunksZ(0),
    s_OcclusionQueries(false),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_TimerOcclusionQueriesInMilliseconds(TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS),
    s_RememberTimeForOcclusionQueriesInMilliseconds(0),
    s_NumberOfMeshesSceneNodeForChunk(1)
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::~FactoryChunkMeshSceneNode()
{
    clear();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::clear()
{
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    s_HardwareMappingHint = 0;
    s_BufferType = 0;
    s_MaterialType = 0;
    updateMeshChunks();
    deleteAllChunks();
    if(s_ChunksMesh) delete[] s_ChunksMesh;
    s_ChunksMesh = 0;
    std::map< int, ChunksMeshBlocksController* >::iterator iter, iter2;
    for(iter = s_ChunksMeshBlocksControllers.begin(), iter2 = s_ChunksMeshBlocksControllers.end(); iter != iter2;)
    {
        delete iter->second;
        ++iter;
    }
    s_WorldRootNodes.clear();
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::addChunk(int id, Vector3d<int> vect, ChunkVoxelObject* cvo)
{
    s_ISVE->getAccessToTheSimpleVoxelEngine();
    VoxelsForRendering* vxlfr = cvo->getVoxelsForRenderingStatic(getPositionScale(), getRotationScale(), getSScale(), s_IsFaces);
    s_ISVE->freeAccessToTheSimpleVoxelEngine();
    if(!vxlfr)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error getting voxels for rendering.", true);
        return false;
    }
    CBatchingMesh** bm = generateIMeshForSceneNode(vxlfr);
    if(!bm)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error generate IMesh for scene node.", true);
        return false;
    }
    s_LockWrite.lock();
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        if(bm[i])
        {
            if(s_ChunksMesh[i][id][vect] != 0) s_ChunksMesh[i][id][vect]->drop();
            s_ChunksMesh[i][id][vect] = bm[i];
        }
    }
    s_LockWrite.unlock();
    delete[] bm;
    delete vxlfr;
    return true;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunks(int numbermesh, int id)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error updating mesh chunks: \"number of mesh\" is > number of meshes");
        return;
    }
    lock_guard<recursive_mutex> locker(s_LockWrite);
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* >::iterator iter, iter2;
    for (iter = s_ChunksMesh[numbermesh][id].begin(), iter2 = s_ChunksMesh[numbermesh][id].end(); iter != iter2;)
    {
        updateMeshChunk(numbermesh, id, iter->first);
        ++iter;
    }
    s_ChunksMesh[numbermesh][id].clear();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunk(int numbermesh, int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error updating mesh chunk: \"number of mesh\" is > number of meshes");
        return;
    }
    s_LockWrite.lock();
    IMesh* bm = s_ChunksMesh[numbermesh][id][vect];
    s_ChunksMesh[numbermesh][id][vect] = 0;
    s_LockWrite.unlock();
    if(bm == 0)
    {
        if(!isRenderChunk(id, vect))
        {
            s_ISVE->registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error deleting chunk from rendering: chunk does not exist. This is normal.");
            return;
        }
        s_ChunksMeshBlocksControllers[id]->deleteMesh(numbermesh, vect);
        deleteChunkForUpdate(id, vect);
        if(s_ChunksMeshBlocksControllers[id]->getNumberOfRenderMeshs(vect) == 0)
        {
            s_LockListAllChunksFlagRendering.lock();
            s_ListAllChunksFlagRendering[id].erase(vect);
            s_LockListAllChunksFlagRendering.unlock();
        }
        return;
    }
    s_ChunksMeshBlocksControllers[id]->setMesh(numbermesh, bm, vect);
    s_LockListAllChunksFlagRendering.lock();
    s_ListAllChunksFlagRendering[id][vect] = true;
    s_LockListAllChunksFlagRendering.unlock();
    if(!isMultiplePartition(id)) setMultiplePartition(id, false);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunks()
{
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) updateMeshChunks(i);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunksByWorldId(int id)
{
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) updateMeshChunks(i, id);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunksByWorldIdAndVector(int id, Vector3d<int> vect)
{
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) updateMeshChunk(i, id, vect);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunk(int id, Vector3d<int> vect)
{
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) updateMeshChunk(i, id, vect);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialFlag(int numbermesh, int id, E_MATERIAL_FLAG flag, bool value)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->setMaterialFlag(numbermesh, flag, value);
    else s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag: world does not exist, id: " + WorkFunctions::ConvertFunctions::itos(id));
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateMeshChunks(int numbermesh)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error updating mesh chunks: \"number of mesh\" is > number of meshes");
        return;
    }
    lock_guard<recursive_mutex> locker(s_LockWrite);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* > >::iterator iter, iter2;
    for (iter = s_ChunksMesh[numbermesh].begin(), iter2 = s_ChunksMesh[numbermesh].end(); iter != iter2;)
    {
        updateMeshChunks(numbermesh, iter->first);
        ++iter;
    }
    s_ChunksMesh[numbermesh].clear();
}

CBatchingMesh** SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::generateIMeshForSceneNode(VoxelsForRendering* vxlfr)
{
    int numbermesh = 0;
    IMesh* mesh = 0;
    VoxelForRendering* vxl = 0;
    CBatchingMesh** BatchingMesh = new CBatchingMesh*[s_NumberOfMeshesSceneNodeForChunk];
    if(!BatchingMesh)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh mas.", true);
        return 0;
    }
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        BatchingMesh[i] = new CBatchingMesh();
        if(!BatchingMesh[i])
        {
            s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh.", true);
            return 0;
        }
    }
    for(int i = 0; i < vxlfr->getNumberOfVoxelsForRendering(); i++)
    {
        vxl = vxlfr->getVoxelForRendering(i);
        s_LockGetMeshVoxel.lock();
        mesh = getMeshVoxel(vxl);
        s_LockGetMeshVoxel.unlock();
        if(!mesh)
        {
            s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error getting IMesh.", true);
            return 0;
        }
        numbermesh = getMeshNumber(vxl);
        if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
        {
            s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk (FCMSN): \"number of mesh\" is > number of meshes");
            return 0;
        }
        BatchingMesh[numbermesh]->addMesh(mesh, vector3df( vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ()),
                              vector3df( vxl->getRotation().getX(), vxl->getRotation().getY(), vxl->getRotation().getZ() ));
        /*BatchingMesh->addMesh(mesh, vector3df( vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ()),
                              vector3df( vxl->getRotation().getX(), vxl->getRotation().getY(), vxl->getRotation().getZ() ));*/
                              //vector3df( vxl->getScale().getX(), vxl->getScale().getY(), vxl.getScale().getZ() ) );
        mesh->drop();
    }
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        if(BatchingMesh[i] !=0)
        {
            BatchingMesh[i]->update();
            BatchingMesh[i]->finalize();
        }
    }
    return BatchingMesh;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteChunk(int id, Vector3d<int> vect)
{
    s_LockWrite.lock();
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        if(s_ChunksMesh[i][id][vect]) s_ChunksMesh[i][id][vect]->drop();
        s_ChunksMesh[i][id][vect] = 0;
    }
    s_LockWrite.unlock();
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isRenderChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator I = s_ListAllChunksFlagRendering.find(id);
    if ( I == s_ListAllChunksFlagRendering.end() ) return false;
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool >::iterator II = s_ListAllChunksFlagRendering[id].find(vect);
    if ( II == s_ListAllChunksFlagRendering[id].end() ) return false;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isForUpdateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator I = s_ChunksMeshSceneNodeForUpdate.find(id);
    if ( I == s_ChunksMeshSceneNodeForUpdate.end() ) return false;
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool >::iterator II = s_ChunksMeshSceneNodeForUpdate[id].find(vect);
    if ( II == s_ChunksMeshSceneNodeForUpdate[id].end() ) return false;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(!isRenderChunk(id, vect))
    {
        s_ISVE->registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error setting chunk for update: chunk does not exist. This is normal.");
        return false;
    }
    if(isForUpdateChunk(id, vect))
    {
        s_ISVE->registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error setting chunk for updates: chunk already setted for updates. This is normal.");
        return false;
    }
    s_LockChunksMeshSceneNodeForUpdate.lock();
    s_ChunksMeshSceneNodeForUpdate[id][vect] = true;
    s_LockChunksMeshSceneNodeForUpdate.unlock();
    return true;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateAllChunks(int id)
{
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool >::iterator iter, iter2;
    lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
    for (iter = s_ListAllChunksFlagRendering[id].begin(), iter2 = s_ListAllChunksFlagRendering[id].end(); iter != iter2;)
    {
        updateChunk(id, iter->first);
        ++iter;
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateAllChunks()
{
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator iter, iter2;
    lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
    for (iter = s_ListAllChunksFlagRendering.begin(), iter2 = s_ListAllChunksFlagRendering.end(); iter != iter2;)
    {
        updateAllChunks(iter->first);
        ++iter;
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunks(int id)
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool >::iterator iter, iter2;
    for (iter = s_ChunksMeshSceneNodeForUpdate[id].begin(), iter2 = s_ChunksMeshSceneNodeForUpdate[id].end(); iter != iter2;)
    {
        updateChunk(id, iter->first, true);
        ++iter;
    }
    s_ChunksMeshSceneNodeForUpdate[id].clear();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunks()
{
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator iter, iter2;
    for (iter = s_ChunksMeshSceneNodeForUpdate.begin(), iter2 = s_ChunksMeshSceneNodeForUpdate.end(); iter != iter2;)
    {
        updateChunks(iter->first);
        ++iter;
    }
    s_ChunksMeshSceneNodeForUpdate.clear();
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool saveplace)
{
    if(!isRenderChunk(id, vect))
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error chunk update: chunk does not exist. ID: " + WorkFunctions::ConvertFunctions::itos(id) + ", Vector: " + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER) + ".");
        return false;
    }
    TaskUpdateChunk* taskupdatechunk = new TaskUpdateChunk;
    taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
    taskupdatechunk->isCompleted = false;
    taskupdatechunk->s_WorldId = id;
    taskupdatechunk->s_VectorChunk = vect;
    s_ISVE->addMainTask(taskupdatechunk);
    if(!saveplace) deleteChunkForUpdate(id, vect);
    return true;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunks(int id)
{
    s_LockListAllChunksFlagRendering.lock();
    s_ListAllChunksFlagRendering[id].clear();
    s_LockListAllChunksFlagRendering.lock();
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->clear();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunks()
{
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator iter, iter2;
    lock_guard<recursive_mutex> locker(s_LockListAllChunksFlagRendering);
    for (iter = s_ListAllChunksFlagRendering.begin(), iter2 = s_ListAllChunksFlagRendering.end(); iter != iter2;)
    {
        deleteAllChunks(iter->first);
        ++iter;
    }
    s_ListAllChunksFlagRendering.clear();
    deleteAllChunksForUpdate();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunksForUpdate(int id)
{
    std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool >::iterator iter, iter2;
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    for (iter = s_ChunksMeshSceneNodeForUpdate[id].begin(), iter2 = s_ChunksMeshSceneNodeForUpdate[id].end(); iter != iter2;)
    {
        deleteChunkForUpdate(id, iter->first);
        ++iter;
    }
    s_ChunksMeshSceneNodeForUpdate[id].clear();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteAllChunksForUpdate()
{
    std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > >::iterator iter, iter2;
    lock_guard<recursive_mutex> locker(s_LockChunksMeshSceneNodeForUpdate);
    for (iter = s_ChunksMeshSceneNodeForUpdate.begin(), iter2 = s_ChunksMeshSceneNodeForUpdate.end(); iter != iter2;)
    {
        deleteAllChunksForUpdate(iter->first);
        ++iter;
    }
    s_ChunksMeshSceneNodeForUpdate.clear();
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::deleteChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(!isForUpdateChunk(id, vect))
    {
        s_ISVE->registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error deleting chunk for updates: chunk does exist for updates. This is normal.");
        return false;
    }
    s_LockChunksMeshSceneNodeForUpdate.lock();
    s_ChunksMeshSceneNodeForUpdate[id].erase(vect);
    s_LockChunksMeshSceneNodeForUpdate.unlock();
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::freeze()
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error freezeing factory chunk mesh scene node: this object already frozen.");
        return false;
    }
    s_Freeze = true;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isFreeze()
{
    return s_Freeze;
}

IrrSimpleVoxelEngine* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getIrrSimpleVoxelEngine()
{
    return s_ISVE;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIrrSimpleVoxelEngine(IrrSimpleVoxelEngine* isve)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrSimpleVoxelEngine: object already frozen.");
        return false;
    }
    if(s_ISVE)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrSimpleVoxelEngine: IrrSimpleVoxelEngine already setted.");
        return false;
    }
    s_ISVE = isve;
    return true;
}

IrrlichtDevice* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getIrrlichtDevice()
{
    return s_Device;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIrrlichtDevice(IrrlichtDevice* device)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrlichtDevice: object already frozen.");
        return false;
    }
    if(s_Device)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting IrrlichtDevice: IrrlichtDevice already setted.");
        return false;
    }
    s_Device = device;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setPositionScale(float ps)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting position scale: object already frozen.");
        return false;
    }
    s_PositionScale = ps;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getPositionScale()
{
    return s_PositionScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setRotationScale(float rs)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting rotation scale: object already frozen.");
        return false;
    }
    s_RotationScale = rs;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getRotationScale()
{
    return s_RotationScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSScale(float ss)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting 'scale' scale: object already frozen.");
        return false;
    }
    s_SScale = ss;
    return true;
}

float SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getSScale()
{
    return s_SScale;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setIsFaces(bool value)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting isFaces: object already frozen.");
        return false;
    }
    s_IsFaces = value;
    return true;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isFaces()
{
    return s_IsFaces;
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshCubeFaces(VoxelForRendering* vxlfr, IMesh* plane, vector3df planescale)
{
    IMesh* planemas[6];
    for(int i = 0; i < 6; i++) planemas[i] = plane;
    return getMeshCubeFaces(vxlfr, planemas, planescale);
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshCubeFaces(VoxelForRendering* vxlfr, IMesh** planes, vector3df planescale)
{
    if(!planes)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: plane mesh not setted.", true);
        return 0;
    }
    CBatchingMesh* BatchingMesh = new CBatchingMesh();
    if(!BatchingMesh)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: error adding CBatchingMesh.", true);
        return 0;
    }
    vector3df vect_move_pos;
    vector3df vect_move_rot;
    vector3df vect_scale;
    float psscale = 1;
    for(int i = 0; i < 6; i++)
    {
        if(vxlfr->isRenderFace(i))
        {
            vect_move_pos.set(0,0,0);
            vect_move_rot.set(0,0,0);
            vect_scale.set(vxlfr->getScale().getX(), vxlfr->getScale().getY(), vxlfr->getScale().getZ());
            if(!planes[i])
            {
                s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: Error getting IMesh.", true);
                return 0;
            }
            if(i == 4) { vect_move_pos.set(0, psscale/2, 0); }
            else if(i == 5) { vect_move_pos.set(0, (-1)*psscale/2, 0); vect_move_rot.set(180, 180, 0); }
            else if(i == 0) { vect_move_pos.set(psscale/2, 0, 0); vect_move_rot.set(0, 90, -90); }
            else if(i == 2) { vect_move_pos.set((-1)*psscale/2, 0, 0); vect_move_rot.set(0, -90, 90); }
            else if(i == 1) { vect_move_pos.set(0, 0, psscale/2); vect_move_rot.set(90, 0, 0); }
            else if(i == 3) { vect_move_pos.set(0, 0, (-1)*psscale/2); vect_move_rot.set(-90, 0, 180); }
            vect_move_pos.set(vect_move_pos.X * vect_scale.X, vect_move_pos.Y * vect_scale.Y, vect_move_pos.Z * vect_scale.Z);
            BatchingMesh->addMesh(planes[i], vect_move_pos, vect_move_rot, vect_scale);
        }
    }
    BatchingMesh->update();
    BatchingMesh->finalize();
    return BatchingMesh;
}

IMesh* SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getMeshVoxelByGlue(VoxelForRendering* vxlfr, IMesh** glues)
{
    if(!glues)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: plane mesh not setted.", true);
        return 0;
    }
    CBatchingMesh* BatchingMesh = new CBatchingMesh();
    if(!BatchingMesh)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: error adding CBatchingMesh.", true);
        return 0;
    }
    BatchingMesh->addMesh(glues[0]);
    for(int i = 1; i < 7; i++)
    {
        if(vxlfr->isRenderFace(i - 1))
        {
            if(!glues[i])
            {
                s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding CBatchingMesh for cube faces: Error getting IMesh.", true);
                return 0;
            }
            BatchingMesh->addMesh(glues[i]);
        }
    }
    BatchingMesh->update();
    BatchingMesh->finalize();
    return BatchingMesh;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMultiplePartition(int id, bool mp)
{
    s_ISVE->getWorld(id)->setMultiplePartition(mp);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::isMultiplePartition(int id)
{
    return s_ISVE->getWorld(id)->isMultiplePartition();
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::addWorld(int id, World3DInfo w3dinfo)
{
    if(s_ChunksMeshBlocksControllers[id])
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding world in factory chunk mesh scene node: world already added.");
        return;
    }
    s_ChunksMeshBlocksControllers[id] = new ChunksMeshBlocksController(s_ISVE, s_Device, id);
    s_ChunksMeshBlocksControllers[id]->setScale(s_SScale);
    s_ChunksMeshBlocksControllers[id]->setNumberOfMeshesSceneNodeForChunk(s_NumberOfMeshesSceneNodeForChunk);
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        s_ChunksMeshBlocksControllers[id]->setHardwareMappingHint(i, s_HardwareMappingHint[i], s_BufferType[i]);
        s_ChunksMeshBlocksControllers[id]->setMaterialType(i, s_MaterialType[i]);
    }
    if(s_SizeBlockChunksX * s_SizeBlockChunksY * s_SizeBlockChunksZ != 0) s_ChunksMeshBlocksControllers[id]->setSizeChunkMeshBlock(s_SizeBlockChunksX, s_SizeBlockChunksY, s_SizeBlockChunksZ);
    s_WorldRootNodes[id] = w3dinfo;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::update()
{
    updateMeshChunks();
    std::map< int, ChunksMeshBlocksController* >::iterator iter, iter2;
    for(iter = s_ChunksMeshBlocksControllers.begin(), iter2 = s_ChunksMeshBlocksControllers.end(); iter != iter2;)
    {
        iter->second->update(s_OcclusionQueries);
        ++iter;
    }
    if(s_OcclusionQueries && s_ISVE->getTimeSafeThread() - s_RememberTimeForOcclusionQueriesInMilliseconds >= s_TimerOcclusionQueriesInMilliseconds)
    {
        s_Device->getVideoDriver()->runAllOcclusionQueries(false);
        s_Device->getVideoDriver()->updateAllOcclusionQueries();
        for(iter = s_ChunksMeshBlocksControllers.begin(), iter2 = s_ChunksMeshBlocksControllers.end(); iter != iter2;)
        {
            iter->second->updateOcclusionQueries();
            ++iter;
        }
        s_RememberTimeForOcclusionQueriesInMilliseconds = s_ISVE->getTimeSafeThread();
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setOcclusionQueries(bool value)
{
    s_OcclusionQueries = value;
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getOcclusionQueries()
{
    return s_OcclusionQueries;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setTimerTestOcclusionQueriesInMilliseconds(unsigned int timer)
{
    s_TimerOcclusionQueriesInMilliseconds = timer;
}

unsigned int SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getTimerTestOcclusionQueriesInMilliseconds()
{
    return s_TimerOcclusionQueriesInMilliseconds;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSizeBlockChunks(int id, int xsize, int ysize, int zsize)
{
    if(s_SizeBlockChunksX * s_SizeBlockChunksY * s_SizeBlockChunksZ != 0)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks to world: size chunks setted for all worlds.");
        return;
    }
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->setSizeChunkMeshBlock(xsize, ysize, zsize);
    else s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks: world does not exist.");
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setSizeBlockChunks(int xsize, int ysize, int zsize)
{
    if(isFreeze())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size block chunks for all worlds: object already frozen.");
        return;
    }
    s_SizeBlockChunksX = xsize;
    s_SizeBlockChunksY = ysize;
    s_SizeBlockChunksZ = zsize;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setHardwareMappingHint(int numbermesh, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE buffertype)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting hardware mapping hint (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    s_HardwareMappingHint[numbermesh] = mapping;
    s_BufferType[numbermesh] = buffertype;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype)
{
    if(numbermesh >= s_NumberOfMeshesSceneNodeForChunk)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag (FCMSN): \"number of mesh\" is > number of meshes");
        return;
    }
    s_MaterialType[numbermesh] = newtype;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setHardwareMappingHint(int numbermesh, int id, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE buffertype)
{
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->setHardwareMappingHint(numbermesh, mapping, buffertype);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setMaterialType(int numbermesh, int id, irr::video::E_MATERIAL_TYPE newtype)
{
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->setMaterialType(numbermesh, newtype);
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setNumberOfMeshesSceneNodeForChunk(int nomsnfc)
{
    s_NumberOfMeshesSceneNodeForChunk = nomsnfc;
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    if(s_ChunksMesh) delete[] s_ChunksMesh;
    s_HardwareMappingHint = new E_HARDWARE_MAPPING[s_NumberOfMeshesSceneNodeForChunk];
    s_BufferType = new E_BUFFER_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_MaterialType = new E_MATERIAL_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_ChunksMesh = new std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* > >[s_NumberOfMeshesSceneNodeForChunk];
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        s_HardwareMappingHint[i] = EHM_NEVER;
        s_BufferType[i] = EBT_VERTEX_AND_INDEX;
        s_MaterialType[i] = EMT_SOLID;
    }
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::setID(int numbermesh, int id, s32 idscenenode)
{
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->setID(numbermesh, idscenenode);
}

int SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getNumberOfMeshesSceneNodeForChunk()
{
    return s_NumberOfMeshesSceneNodeForChunk;
}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::onUpdateChunkMeshSceneNode(IMeshSceneNode* node, int numbermesh, Vector3d<int> vectblock) {}

void SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::updateCameraPosition(int id, vector3df vect)
{
    if(s_ChunksMeshBlocksControllers[id]) s_ChunksMeshBlocksControllers[id]->updateCameraPosition(vect);
}

bool SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::changeWorld3DInfo(int id, World3DInfo w3dinfo)
{
    if(!s_ChunksMeshBlocksControllers[id])
    {
        s_ISVE->getLogger()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Change root node for not exists world, id = " + WorkFunctions::ConvertFunctions::itos(id) + ".");
        return false;
    }
    s_WorldRootNodes[id] = w3dinfo;
    return true;
}

World3DInfo SimpleVoxelEngine::IrrEngine::FactoryChunkMeshSceneNode::getWorld3DInfo(int id)
{
    World3DInfo w3dinfo;
    w3dinfo.s_WorldRootNode = 0;
    if(!s_ChunksMeshBlocksControllers[id]) s_ISVE->getLogger()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Getting world 3d info for not exists world, id = " + WorkFunctions::ConvertFunctions::itos(id) + ".");
    else w3dinfo = s_WorldRootNodes[id];
    return w3dinfo;
}
