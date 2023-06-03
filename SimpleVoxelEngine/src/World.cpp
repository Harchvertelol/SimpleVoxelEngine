#include "World.h"

#include <cstdlib>

#include "Engine.h"
#include "WorkFunctions.h"
#include "IniParser/ParserInfoFile.h"
#include "Defines.h"

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Additional;
using namespace Voxels;
using namespace Defines;
using namespace IniParser;
using namespace STRING_CONSTANTS;

using namespace std;

SimpleVoxelEngine::Core::World::World(Engine* eng, WorldCreatingParameters& wcp):
    s_Engine(eng),
    s_WorldCreatingParameters(wcp),
    s_Id(-1),
    s_WorldGenerator(0),
    s_SimpleVoxelObjectTransparentTester(0),
    s_ChunksSaveLoadController(0)
{
    //...
}

SimpleVoxelEngine::Core::World::World(Engine* eng):
    s_Engine(eng),
    s_Id(-1),
    s_WorldGenerator(0),
    s_SimpleVoxelObjectTransparentTester(0),
    s_ChunksSaveLoadController(0)
{
    //...
}

SimpleVoxelEngine::Core::World::~World()
{
    clear(false);
    if(s_ChunksSaveLoadController) delete s_ChunksSaveLoadController;
    if(s_WorldGenerator) delete s_WorldGenerator;
    if(s_SimpleVoxelObjectTransparentTester) delete s_SimpleVoxelObjectTransparentTester;
}

void SimpleVoxelEngine::Core::World::clear(bool save)
{
    map< Vector3d<int>, ChunkVoxelObject* >::iterator iter, iter2;
    for(iter = s_Chunks.begin(), iter2 = s_Chunks.end(); iter != iter2;)
    {
        unloadChunk(iter->first, save, true);
        ++iter;
    }
    s_Chunks.clear();
}

bool SimpleVoxelEngine::Core::World::saveAllChunks()
{
    map< Vector3d<int>, ChunkVoxelObject* >::iterator iter, iter2;
    for(iter = s_Chunks.begin(), iter2 = s_Chunks.end(); iter != iter2;)
    {
        if(!saveChunk(iter->first))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved and not unloaded (Vector: " + (iter->first).toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
            return false;
        }
        ++iter;
    }
    return true;
}

bool SimpleVoxelEngine::Core::World::loadChunk(Vector3d<int> vect)
{
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk loading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
    if(!createChunkInMemory(vect))
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Chunk not loaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk already loaded.");
        return false;
    }
    if(s_ChunksSaveLoadController && s_ChunksSaveLoadController->canLoad(this, vect)) return s_ChunksSaveLoadController->loadChunk(s_Chunks[vect], vect);
    else return generateChunk(vect, false);
    return false;
}

bool SimpleVoxelEngine::Core::World::generateChunk(Vector3d<int> vect, bool createchunkinmemory)
{
    if(!s_WorldGenerator)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Error chunk generation: world generator is not set. Generating clear chunk.");
        return false;
    }
    if(createchunkinmemory) createChunkInMemory(vect);
    return s_WorldGenerator->generateChunk(getChunk(vect), getSizeChunk());
}

bool SimpleVoxelEngine::Core::World::createChunkInMemory(Vector3d<int> vect)
{
    if(isExistsChunkInMemory(vect)) return false;
    s_Chunks[vect] = new ChunkVoxelObject();
    s_Chunks[vect]->setWorld(this);
    s_Chunks[vect]->setNumberOfPartitions(s_WorldCreatingParameters.s_SizeChunk);
    VoxelAddress* vxladdr = new VoxelAddress(s_Engine);
    vxladdr->addNextVector(vect);
    s_Chunks[vect]->setVoxelAddress(vxladdr);
    return true;
}

bool SimpleVoxelEngine::Core::World::loadChunks(Vector3d<int> vect, int cube_radius)
{
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks loading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "), radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius));
    if(cube_radius < 1)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: cube radius < 1");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                if(!loadChunk(vect + vect_move)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::progressiveLoadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect_old, int cube_radius_old, SimpleVoxelEngine::Additional::Vector3d<int> vect_new, int cube_radius_new, bool save)
{
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Progressive chunks loading. Vector old: " + vect_old.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "), radius old: "
                            + WorkFunctions::ConvertFunctions::itos(cube_radius_old) + ", " + "Vector new: " + vect_new.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER)
                            + "), radius new: " + WorkFunctions::ConvertFunctions::itos(cube_radius_new) + ".");
    if(cube_radius_old < 1)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: old cube radius < 1");
        return false;
    }
    if(cube_radius_new < 1)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: new cube radius < 1");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius_old; x <= cube_radius_old; x++)
        for(int y = (-1)*cube_radius_old; y <= cube_radius_old; y++)
            for(int z = (-1)*cube_radius_old; z <= cube_radius_old; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                Vector3d<int> temp_vect_1 = (vect_old - vect_move).getAbs();
                Vector3d<int> temp_vect_2 = (vect_new - vect_move).getAbs();
                if(temp_vect_1.getX() <= cube_radius_old && temp_vect_1.getY() <= cube_radius_old && temp_vect_1.getZ() <= cube_radius_old &&
                   temp_vect_2.getX() > cube_radius_new && temp_vect_1.getY() > cube_radius_new && temp_vect_1.getZ() > cube_radius_new)
                        if(!unloadChunk(vect_old + vect_move, save)) good = false;
            }
    for(int x = (-1)*cube_radius_new; x <= cube_radius_new; x++)
        for(int y = (-1)*cube_radius_new; y <= cube_radius_new; y++)
            for(int z = (-1)*cube_radius_new; z <= cube_radius_new; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                Vector3d<int> temp_vect_1 = (vect_old - vect_move).getAbs();
                Vector3d<int> temp_vect_2 = (vect_new - vect_move).getAbs();
                if(temp_vect_1.getX() > cube_radius_old && temp_vect_1.getY() > cube_radius_old && temp_vect_1.getZ() > cube_radius_old &&
                   temp_vect_2.getX() <= cube_radius_new && temp_vect_1.getY() <= cube_radius_new && temp_vect_1.getZ() <= cube_radius_new)
                        if(!loadChunk(vect_new + vect_move)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::unloadChunk(Vector3d<int> vect, bool save, bool saveplace)
{
    if(save == true)
        if(!saveChunk(vect))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved and not unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
            return false;
        }
    if(!isExistsChunkInMemory(vect))
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Chunk not unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk already unloaded.");
        return false;
    }
    delete s_Chunks[vect];
    s_Chunks[vect] = 0;
    if(!saveplace) s_Chunks.erase(vect);
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
    return true;
}

bool SimpleVoxelEngine::Core::World::unloadChunks(Vector3d<int> vect, int cube_radius, bool save)
{
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks unloading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "), radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius));
    if(cube_radius < 1)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks unloading error: cube radius < 1");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                if(!unloadChunk(vect + vect_move, save)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::saveChunk(Vector3d<int> vect)
{
    if(!s_ChunksSaveLoadController)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error save chunk. ChunksSaveLoadController not setted.");
        return false;
    }
    if(!isExistsChunkInMemory(vect))
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Chunk not saved (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk not loaded.");
        return false;
    }
    return s_ChunksSaveLoadController->saveChunk(s_Chunks[vect], vect);
}

void SimpleVoxelEngine::Core::World::setChunksSaveLoadController(ChunksSaveLoadController* cslc)
{
    s_ChunksSaveLoadController = cslc;
    s_ChunksSaveLoadController->setWorld(this);
}

ChunksSaveLoadController* SimpleVoxelEngine::Core::World::getChunksSaveLoadController()
{
    return s_ChunksSaveLoadController;
}

bool SimpleVoxelEngine::Core::World::saveChunks(Vector3d<int> vect, int cube_radius)
{
    if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks saving (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "), radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius));
    if(cube_radius < 1)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks saving error: cube radius < 1");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                if(!saveChunk(vect + vect_move)) good = false;
            }
    return good;
}

ChunkVoxelObject* SimpleVoxelEngine::Core::World::getChunk(Vector3d<int> vect)
{
    if(isExistsChunkInMemory(vect)) return s_Chunks[vect];
    return 0;
}

SimpleVoxelObject* SimpleVoxelEngine::Core::World::getSimpleVoxelObject(VoxelAddress* addr, bool isexists)
{
    if(!addr) return 0;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Getting SVO: Last vector index is NULL. Chunk getting...");
        return getChunk(addr->getLastVector());
    }
    SimpleVoxelObject* temp = getChunk(addr->getVector(0));
    if(!temp)
    {
        if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO (chunk). Chunk is not loaded.", true);
        return 0;
    }
    SimpleVoxelObject* temp1;
    for(int i = 1; i < addr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO. This partition does not exist.", true);
            return 0;
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(addr->getVector(i));
        if(!temp1)
        {
            if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting SVO. SVO does not exist.", true);
            return 0;
        }
        temp = temp1;
    }
    return temp;
}

bool SimpleVoxelEngine::Core::World::deleteSimpleVoxelObject(VoxelAddress* addr)
{
    if(!addr) return false;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error deleting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error deleting SVO. Last vector index is NULL.", true);
        return 0;
    }
    VoxelAddress addrtemp(*addr);
    Vector3d<int> dvect(addrtemp.getLastVector());
    addrtemp.deleteLastVector();
    SimpleVoxelObject* temp = getSimpleVoxelObject(&addrtemp);
    if(!temp)
    {
        s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for deleting SVO: point is NULL.");
        return false;
    }
    if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
    {
        s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for deleting SVO: getting SVO is not chunk voxel object.");
        return false;
    }
    return ((ChunkVoxelObject*)temp)->deleteSimpleVoxelObject(dvect);
}

bool SimpleVoxelEngine::Core::World::setSimpleVoxelObject(VoxelAddress* addr, SimpleVoxelObject* svo, bool rewrite)
{
    if(!addr || !svo) return false;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. Last vector index is NULL.", true);
        return 0;
    }
    VoxelAddress addrtemp(*addr);
    Vector3d<int> dvect(addrtemp.getLastVector());
    addrtemp.deleteLastVector();
    SimpleVoxelObject* temp = getSimpleVoxelObject(&addrtemp);
    if(!temp)
    {
        s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for setting SVO: point is NULL.");
        return false;
    }
    if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
    {
        s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for setting SVO: getting SVO is not chunk voxel object.");
        return false;
    }
    return ((ChunkVoxelObject*)temp)->setSimpleVoxelObject(dvect, svo, rewrite);
}

Engine* SimpleVoxelEngine::Core::World::getEngine()
{
    return s_Engine;
}

int SimpleVoxelEngine::Core::World::getId()
{
    return s_Id;
}

bool SimpleVoxelEngine::Core::World::setId(int id)
{
    if(s_Id == -1)
    {
        s_Id = id;
        return true;
    }
    return false;
}

bool SimpleVoxelEngine::Core::World::isExistsChunkInMemory(Vector3d<int> vect)
{
    map< Vector3d<int>, ChunkVoxelObject* >::iterator I = s_Chunks.find(vect);
    if ( I == s_Chunks.end() ) return false;
    return true;
}

bool SimpleVoxelEngine::Core::World::setWorldGenerator(WorldGenerator* wrldgen)
{
    if(s_WorldGenerator)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world generator: world generator always setted.");
        return false;
    }
    s_WorldGenerator = wrldgen;
    return true;
}

WorldGenerator* SimpleVoxelEngine::Core::World::getWorldGenerator()
{
    return s_WorldGenerator;
}

unsigned char SimpleVoxelEngine::Core::World::getSizeChunk()
{
    return s_WorldCreatingParameters.s_SizeChunk;
}

VoxelAddress* SimpleVoxelEngine::Core::World::getAddressSimpleVoxelObjectByShift(VoxelAddress* addr, Vector3d<int> vect)
{
    if(!addr) return 0;
    VoxelAddress* newaddr = 0;
    VoxelAddress tva(*addr);
    Vector3d<int> vct1 = tva.getLastVector();
    Vector3d<int> vct = vct1 + vect;
    tva.deleteLastVector();
    SimpleVoxelObject* tsvo = getSimpleVoxelObject(&tva);
    if( isMultiplePartition() == false || ( (tsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT) &&
                                        vct.getX() >= 0 && vct.getX() < ((ChunkVoxelObject*)tsvo)->getNumberOfPartitions() &&
                                        vct.getY() >= 0 && vct.getY() < ((ChunkVoxelObject*)tsvo)->getNumberOfPartitions() &&
                                        vct.getZ() >= 0 && vct.getZ() < ((ChunkVoxelObject*)tsvo)->getNumberOfPartitions() ) )
    {
        newaddr = new VoxelAddress( (*addr) );
        newaddr->addVector(newaddr->getLastVectInd() - 1, vct);
        return newaddr;
    }
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Last vector index is NULL.", true);
        return 0;
    }
    SimpleVoxelObject* temp = getChunk(addr->getVector(0));
    if(!temp)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Chunk is not loaded.", true);
        return 0;
    }
    SimpleVoxelObject* temp1 = 0;
    float multiplenumber = 1;
    Vector3d<float> vectshift_fl( (float)vect.getX(), (float)vect.getY(), (float)vect.getZ() );
    Vector3d<float> vectn( (float)addr->getVector(0).getX(), (float)addr->getVector(0).getY(), (float)addr->getVector(0).getZ() );
    for(int i = 1; i < addr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. This partition does not exist.", true);
            return 0;
        }
        multiplenumber = float(1 / ( (float)((ChunkVoxelObject*)temp)->getNumberOfPartitions() ) );
        vectn = vectn + Vector3d<float>( ( (float)addr->getVector(i).getX() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions(), ( (float)addr->getVector(i).getY() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions(), ( (float)addr->getVector(i).getZ() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions() );
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(addr->getVector(i));
        if(!temp1)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Error getting address SVO. SVO does not exist. This is normal.");
            return 0;
        }
        temp = temp1;
    }
    Vector3d<float> resvectsearch;
    Vector3d<float> vect2;
    vect2 = vectshift_fl * multiplenumber;
    resvectsearch = vectn + vect2;
    Vector3d<float> tempvect = resvectsearch;
    Vector3d<float> tempvect1;
    if(newaddr) delete newaddr;
    tempvect1 = getNextFloorVect(tempvect);
    temp = getChunk(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    if(!temp) return 0;
    newaddr = new VoxelAddress(addr->getEngine());
    newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    tempvect = tempvect - tempvect1;
    tempvect = tempvect*((ChunkVoxelObject*)temp)->getNumberOfPartitions();
    for(int i = 1; i < addr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT) return newaddr;
        tempvect1 = getNextFloorVect(tempvect);
        newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(newaddr->getLastVector());
        if(!temp1) return newaddr;
        tempvect = tempvect - tempvect1;
        tempvect = tempvect*((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        temp = temp1;
    }
    return newaddr;
}

Vector3d<float> SimpleVoxelEngine::Core::World::getNextFloorVect(Vector3d<float> vect)
{
    Vector3d<float> newvect(Vector3d<float>( floorf(vect.getX()), floorf(vect.getY()), floorf(vect.getZ()) ));
    return newvect;
}

bool SimpleVoxelEngine::Core::World::setSimpleVoxelObjectTransparentTester(SimpleVoxelObjectTransparentTester* svott)
{
    if(s_SimpleVoxelObjectTransparentTester)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO transparent tester: this object always setted.");
        return false;
    }
    s_SimpleVoxelObjectTransparentTester = svott;
    return true;
}

SimpleVoxelObjectTransparentTester* SimpleVoxelEngine::Core::World::getSimpleVoxelObjectTransparentTester()
{
    return s_SimpleVoxelObjectTransparentTester;
}

bool SimpleVoxelEngine::Core::World::isTransparent(VoxelAddress* vxladdr, SimpleVoxelObject* parent)
{
    SimpleVoxelObject* svo = getSimpleVoxelObject(vxladdr, false);
    if(!svo) return true;
    if(s_SimpleVoxelObjectTransparentTester) return s_SimpleVoxelObjectTransparentTester->isTransparent(svo, parent);
    return false;
}

VoxelAddress* SimpleVoxelEngine::Core::World::getAddressSimpleVoxelObjectByThreePoints(Vector3d<float> v1, Vector3d<float> v2, Vector3d<float> v3, float scale, Vector3d<float> lookvect, LevelSearchSimpleVoxelObject lssvo, int level)
{
    /*int numberlevel = 1;
    if(lssvo == NUMBERLEVEL) numberlevel = level;
    else if(lssvo == LASTLEVEL) numberlevel = -1;*/
    Vector3d<float> move_vect(0.5, 0.5, 0.5);
    Vector3d<float> vv1(v1 + move_vect);
    Vector3d<float> vv2(v2 + move_vect);
    Vector3d<float> vv3(v3 + move_vect);
    vv1 /= scale;
    vv2 /= scale;
    vv3 /= scale;
    Vector3d<float> a1(vv1 - vv2);
    Vector3d<float> a2(vv2 - vv3);
    Vector3d<float> point = (vv1 + vv2 + vv3)/3;
    if( fabs(point.getX()*scale - floor(point.getX()*scale)) < EPS || fabs(point.getY()*scale - floor(point.getY()*scale)) < EPS || fabs(point.getZ()*scale - floor(point.getZ()*scale)) < EPS )
    {
        Vector3d<float> tempvectn(a1.crossProduct(a2));
        Vector3d<float> tempvectn1(tempvectn.getNormalizeVector());
        if(tempvectn1.scalarProduct(lookvect) < 0) tempvectn1 *= (-1);
        else if(tempvectn1.scalarProduct(lookvect) == 0)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: look vector is incorrect (scalar product = 0).");
            return 0;
        }
        tempvectn1 *= sqrt(a1.getSquareNorm()) * 0.5;
        point += tempvectn1;
    }
    if( fabs(floor(point.getX()) - point.getX()) < EPS || fabs(floor(point.getY()) - point.getY()) < EPS || fabs(floor(point.getZ()) - point.getZ()) < EPS )
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: look vector is incorrect.");
        return 0;
    }
    Vector3d<float> tempvect = point;
    Vector3d<float> tempvect1;
    VoxelAddress* newaddr = new VoxelAddress(s_Engine);
    tempvect1 = getNextFloorVect(tempvect);
    SimpleVoxelObject* temp = getChunk(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    SimpleVoxelObject* temp1 = 0;
    if(!temp) return newaddr;
    newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    tempvect = tempvect - tempvect1;
    tempvect = tempvect*((ChunkVoxelObject*)temp)->getNumberOfPartitions();
    for(int i = 1; true; i++)
    {
        if(lssvo == NUMBERLEVEL && i >= level) return newaddr;
        if(lssvo == NUMBERLEVEL && temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: this object does not exist.");
            return 0;
        }
        else if(lssvo == LASTLEVEL && temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT) return newaddr;
        tempvect1 = getNextFloorVect(tempvect);
        newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(newaddr->getLastVector());
        if( (!temp1 && lssvo == LASTLEVEL) || (!temp && lssvo == NUMBERLEVEL && i >= level) ) return newaddr;
        else if(!temp)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: this object does not exist.");
            return 0;
        }
        tempvect = tempvect - tempvect1;
        tempvect = tempvect*((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        temp = temp1;
    }
    return 0;
}

bool SimpleVoxelEngine::Core::World::isVisible(VoxelAddress* vxladdr, bool* renderfaces, SimpleVoxelObject* parent)
{
    if(!renderfaces) renderfaces = new bool[6];
    for(int i = 0; i < 6; i++) renderfaces[i] = false;
    Vector3d<int> vectsaround[6] = {
        Vector3d<int>(1,0,0),
        Vector3d<int>(0,0,1),
        Vector3d<int>(-1,0,0),
        Vector3d<int>(0,0,-1),
        Vector3d<int>(0,1,0),
        Vector3d<int>(0,-1,0)
        };
    VoxelAddress* tt = 0;
    for(int i = 0; i < 6; i++)
    {
        tt = getAddressSimpleVoxelObjectByShift(vxladdr, vectsaround[i]);
        if(isTransparent(tt, parent)) renderfaces[i] = true;
        delete tt;
    }
    bool test = false;
    for(int i = 0; i < 6; i++) test = test || renderfaces[i];
    return test;
}

Vector3d<float> SimpleVoxelEngine::Core::World::getPosition(VoxelAddress* vxladdr, float posscale)
{
    SimpleVoxelObject* temp = getChunk(vxladdr->getVector(0));
    SimpleVoxelObject* temp1;
    Vector3d<float> vect( posscale*((float)vxladdr->getVector(0).getX() - 0.5), posscale*((float)vxladdr->getVector(0).getY() - 0.5), posscale*((float)vxladdr->getVector(0).getZ() - 0.5) );
    for(int i = 1; i < vxladdr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting position SVO. This partition does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(vxladdr->getVector(i));
        if(!temp1)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting position SVO. This SVO does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        vect = vect + Vector3d<float>( posscale*( (float)vxladdr->getVector(i).getX() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions(), posscale*( (float)vxladdr->getVector(i).getY() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions(), posscale*( (float)vxladdr->getVector(i).getZ() ) / ((ChunkVoxelObject*)temp)->getNumberOfPartitions() );
        temp = temp1;
    }
    return vect - Vector3d<float>((float)vxladdr->getVector(0).getX(), (float)vxladdr->getVector(0).getY(), (float)vxladdr->getVector(0).getZ()) * s_WorldCreatingParameters.s_SizeChunk;
}

Vector3d<float> SimpleVoxelEngine::Core::World::getRotation(VoxelAddress* vxladdr, float rotscale)
{
    return Vector3d<float>(0,0,0);
}

Vector3d<float> SimpleVoxelEngine::Core::World::getScale(VoxelAddress* vxladdr, float scscale)
{
    SimpleVoxelObject* temp = getChunk(vxladdr->getVector(0));
    SimpleVoxelObject* temp1;
    Vector3d<float> vect( 1, 1, 1 );
    for(int i = 1; i < vxladdr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting position SVO. This partition does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(vxladdr->getVector(i));
        if(!temp1)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting position SVO. This SVO does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        vect = vect*( scscale / ( (float)((ChunkVoxelObject*)temp)->getNumberOfPartitions() ) );
        temp = temp1;
    }
    return vect;
}

void SimpleVoxelEngine::Core::World::setMultiplePartition(bool multpart)
{
    s_WorldCreatingParameters.s_MultiplePartition = multpart;
}

bool SimpleVoxelEngine::Core::World::isMultiplePartition()
{
    return s_WorldCreatingParameters.s_MultiplePartition;
}
