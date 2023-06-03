#include "ChunkVoxelObject.h"

#include <fstream>

#include "Engine.h"
#include "World.h"
#include "ARGBColorVoxel.h"
#include "Defines.h"

using namespace std;

using namespace SimpleVoxelEngine;
using namespace Defines;
using namespace Additional;
using namespace Core;
using namespace Voxels;
using namespace STRING_CONSTANTS;


SimpleVoxelEngine::Voxels::ChunkVoxelObject::ChunkVoxelObject():
    SimpleVoxelObject(),
    s_World(0),
    s_Vxladdr(0),
    s_SimpleVoxelObjects(0),
    s_ReCalculateVisibleCacheSimpleVoxelObjects(0),
    s_CacheVisibleSimpleVoxelObjects(0),
    s_CacheVisibleSimpleVoxelObjectsRenderFaces(0),
    s_NumberOfPartitions(0),
    s_NumberOfNotNullSimpleVoxelObjects(0)
{
    setTypeVoxel(CHUNK_VOXEL_OBJECT);
}

SimpleVoxelEngine::Voxels::ChunkVoxelObject::~ChunkVoxelObject()
{
    if(s_Vxladdr) delete s_Vxladdr;
    deleteData();
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::updateData()
{
    if(s_NumberOfNotNullSimpleVoxelObjects == 0)
    {
        deleteData();
        return;
    }
    else if(!s_SimpleVoxelObjects)
    {
        createData();
        return;
    }
}

unsigned char SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberOfPartitions()
{
    return s_NumberOfPartitions;
}

unsigned int SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberOfNotNullSimpleVoxelObjects()
{
    return s_NumberOfNotNullSimpleVoxelObjects;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setNumberOfPartitions(unsigned char nop)
{
    if(s_NumberOfPartitions && s_SimpleVoxelObjects)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Number of partitions already set.");
        return false;
    }
    s_NumberOfPartitions = nop;
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::createData()
{
    deleteData();
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numberofmas = snoptemp*snoptemp*snoptemp;
    s_SimpleVoxelObjects = new SimpleVoxelObject*[numberofmas];
    s_ReCalculateVisibleCacheSimpleVoxelObjects = new bool[numberofmas];
    s_CacheVisibleSimpleVoxelObjects = new bool[numberofmas];
    s_CacheVisibleSimpleVoxelObjectsRenderFaces = new bool*[numberofmas];
    for(int i = 0; i < numberofmas; i++)
    {
        s_SimpleVoxelObjects[i] = 0;
        s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = false;
        s_CacheVisibleSimpleVoxelObjects[i] = false;
        s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
    }
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::deleteData()
{
    if(s_SimpleVoxelObjects)
    {
        const int snoptemp = (int)s_NumberOfPartitions;
        const int numberofmas = snoptemp*snoptemp*snoptemp;
        for(int i = 0; i < numberofmas; i++)
        {
            if(s_SimpleVoxelObjects[i] != 0) delete s_SimpleVoxelObjects[i];
            if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] != 0) delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
        }
        delete[] s_SimpleVoxelObjects;
        s_SimpleVoxelObjects = 0;
        delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces;
        s_CacheVisibleSimpleVoxelObjectsRenderFaces = 0;
        delete[] s_ReCalculateVisibleCacheSimpleVoxelObjects;
        s_ReCalculateVisibleCacheSimpleVoxelObjects = 0;
        delete[] s_CacheVisibleSimpleVoxelObjects;
        s_CacheVisibleSimpleVoxelObjects = 0;
    }
}

SimpleVoxelObject** SimpleVoxelEngine::Voxels::ChunkVoxelObject::getSimpleVoxelObjects()
{
    if(!s_SimpleVoxelObjects) createData();
    return s_SimpleVoxelObjects;
}

SimpleVoxelObject* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(s_SimpleVoxelObjects) return s_SimpleVoxelObjects[getNumberInMemorySimpleVoxelObjectByVector(vect)];
    return 0;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelObject* svo, bool rewrite, bool setrecalculatearoundsvo)
{
    if(isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS))
    {
        if(rewrite == false)
        {
            if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. This partition already exist.");
            return false;
        }
        else
        {
            s_NumberOfNotNullSimpleVoxelObjects++;
            deleteSimpleVoxelObject(vect);
            s_NumberOfNotNullSimpleVoxelObjects--;
        }
    }
    if(svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
    {
        VoxelAddress* vxladdr = new VoxelAddress(*(getVoxelAddress()));
        vxladdr->addNextVector(vect);
        ((ChunkVoxelObject*)svo)->setVoxelAddress(vxladdr);
        ((ChunkVoxelObject*)svo)->setWorld(s_World);
    }
    s_NumberOfNotNullSimpleVoxelObjects++;
    updateData();
    s_SimpleVoxelObjects[getNumberInMemorySimpleVoxelObjectByVector(vect)] = svo;
    setReCalculateVisibleCache(vect);
    if(setrecalculatearoundsvo) setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(vect);
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    VoxelAddress* vxladdr = new VoxelAddress(*getVoxelAddress());
    vxladdr->addNextVector(vect);
    Vector3d<int> vectsaround[6] = {
        Vector3d<int>(1,0,0),
        Vector3d<int>(0,0,1),
        Vector3d<int>(-1,0,0),
        Vector3d<int>(0,0,-1),
        Vector3d<int>(0,1,0),
        Vector3d<int>(0,-1,0)
        };
    VoxelAddress* tt = 0;
    Vector3d<int> tempvect;
    SimpleVoxelObject* tempsvo = 0;
    const int snoptemp = (int)s_NumberOfPartitions;
    for(int i = 0; i < 6; i++)
    {
        tempvect = vect + vectsaround[i];
        if( tempvect.getX() < snoptemp && tempvect.getY() < snoptemp && tempvect.getZ() < snoptemp &&
           tempvect.getX() >= 0 && tempvect.getY() >= 0 && tempvect.getZ() >= 0 ) tempsvo = this;
        else
        {
            tt = getWorld()->getAddressSimpleVoxelObjectByShift(vxladdr, vectsaround[i]);
            if(tt)
            {
                tempvect = tt->getLastVector();
                tt->deleteLastVector();
                tempsvo = getWorld()->getSimpleVoxelObject(tt);
                delete tt;
            }
        }
        if(tempsvo && tempsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
        {
            ChunkVoxelObject* tempchunkvoxelobject = (ChunkVoxelObject*)tempsvo;
            tempchunkvoxelobject->setReCalculateVisibleCache(tempvect);
        }
        tempsvo = 0;
    }
    delete vxladdr;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if ( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Removal is not an existing SVO.");
        return false;
    }
    if(isInVisibleCacheSimpleVoxelObject(vect)) setLocationInVisibleCacheSimpleVoxelObject(vect, false);
    setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(vect);
    const int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    delete s_SimpleVoxelObjects[numberinmemory];
    s_SimpleVoxelObjects[numberinmemory] = 0;
    s_NumberOfNotNullSimpleVoxelObjects--;
    updateData();
    return true;
}

VoxelsForRendering* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVoxelsForRenderingStatic(float posscale, float rotscale, float scscale, bool faces)
{
    VoxelsForRendering* vxlfr = new VoxelsForRendering();
    addVoxelsForRenderingStatic(vxlfr, posscale, rotscale, scscale, faces);
    return vxlfr;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::addVoxelsForRenderingStatic(VoxelsForRendering* vxlfr, float posscale, float rotscale, float scscale, bool faces)
{
    if(s_NumberOfNotNullSimpleVoxelObjects == 0) return;
    VoxelAddress* tempvxladdr = 0;
    recalculateVisibleCache();
    if(getTypeVoxel() == CHUNK_VOXEL_OBJECT)
    {
        const int snoptemp = (int)s_NumberOfPartitions;
        const int numberofmas = snoptemp*snoptemp*snoptemp;
        for(int i = 0; i < numberofmas; i++)
            if(s_CacheVisibleSimpleVoxelObjects[i] == true)
            {
                SimpleVoxelObject* tempptrsvo = s_SimpleVoxelObjects[i];
                if(tempptrsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                {
                    ((ChunkVoxelObject*)tempptrsvo)->addVoxelsForRenderingStatic(vxlfr, posscale, rotscale, scscale, faces);
                }
                else if( tempptrsvo->getTypeVoxel() == CUSTOM_VOXEL || tempptrsvo->getTypeVoxel() == ARGBCOLOR )
                {
                    tempvxladdr = new VoxelAddress(*getVoxelAddress());
                    tempvxladdr->addNextVector(getVectorSimpleVoxelObjectByNumberInMemory(i));
                    VoxelForRendering* vxl = new VoxelForRendering(s_World->getPosition(tempvxladdr, posscale), s_World->getRotation(tempvxladdr, rotscale), s_World->getScale(tempvxladdr, scscale));
                    vxl->setVoxelInfo(tempptrsvo->getVoxelForRenderingInfo());
                    if(faces == true)
                    {
                        bool* renderfaces = getRenderFacesSimpleVoxelObjectInCache(tempvxladdr->getLastVector());
                        if(!renderfaces)
                        {
                            if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting render faces from SVO up", true);
                        }
                        else vxl->setRenderFaces(getRenderFacesSimpleVoxelObjectInCache(tempvxladdr->getLastVector()));
                    }
                    vxlfr->addVoxelForRendering(vxl);
                    delete tempvxladdr;
                    tempvxladdr = 0;
                }
            }
    }
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setVoxelAddress(VoxelAddress* vxladdr)
{
    if(s_Vxladdr)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting voxel address. Voxel address already setted.");
        return false;
    }
    s_Vxladdr = vxladdr;
    return true;
}

VoxelAddress* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVoxelAddress()
{
    return s_Vxladdr;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isExistsInMemory(Vector3d<int> vect, WHERE_SEARCH_IN_MEMORY tpcache)
{
    int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if(tpcache == ALL_SIMPLE_VOXEL_OBJECTS)
    {
        if(!s_SimpleVoxelObjects) return false;
        if(s_SimpleVoxelObjects[numberinmemory] == 0) return false;
        return true;
    }
    else if(tpcache == VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT)
    {
        if(!s_CacheVisibleSimpleVoxelObjects) return false;
        return s_CacheVisibleSimpleVoxelObjects[numberinmemory];
    }
    else if(tpcache == VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT)
    {
        if(!s_CacheVisibleSimpleVoxelObjectsRenderFaces) return false;
        if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[numberinmemory] == 0) return false;
        return true;
    }
    return false;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setLocationInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool vsb)
{
    if ( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_WARNING, "Setting visible/invisible not an existing SVO.");
        return false;
    }
    const int numbinmas = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if(vsb == true)
    {
        s_CacheVisibleSimpleVoxelObjects[numbinmas] = true;
        return true;
    }
    if ( !isInVisibleCacheSimpleVoxelObject(vect) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_WARNING, "This SVO already invisible.");
        return false;
    }
    s_CacheVisibleSimpleVoxelObjects[numbinmas] = false;
    if(isExistsInMemory(vect, VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT))
    {
        delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[numbinmas];
        s_CacheVisibleSimpleVoxelObjectsRenderFaces[numbinmas] = 0;
    }
    return true;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if ( !isExistsInMemory(vect, VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT) ) return false;
    return true;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setReCalculateVisibleCache(Vector3d<int> vect)
{
    const int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Recalculate visible cache null SVO. Aborting...");
        return false;
    }
    s_ReCalculateVisibleCacheSimpleVoxelObjects[numberinmemory] = true;
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::recalculateVisibleCache(bool full)
{
    if(!s_SimpleVoxelObjects) return;
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numbofmas = snoptemp*snoptemp*snoptemp;
    if(full)
    {
        for(int i = 0; i < numbofmas; i++)
        {
            s_CacheVisibleSimpleVoxelObjects[i] = false;
            if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] != 0) delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
            s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
        }
    }
    for (int i = 0; i < numbofmas; i++)
    {
        if(s_ReCalculateVisibleCacheSimpleVoxelObjects[i] == true || (full && s_SimpleVoxelObjects[i] != 0) )
        {
            Vector3d<int> tempvect = getVectorSimpleVoxelObjectByNumberInMemory(i);
            if(!isExistsInMemory(tempvect, VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT)) s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = new bool[6];
            bool visible = isVisible(tempvect, s_CacheVisibleSimpleVoxelObjectsRenderFaces[i]);
            bool inVisibleCache = isInVisibleCacheSimpleVoxelObject(tempvect);
            if(isExistsInMemory(tempvect, ALL_SIMPLE_VOXEL_OBJECTS))
            {
                if( visible && !inVisibleCache ) setLocationInVisibleCacheSimpleVoxelObject(tempvect, true);
                else if(!visible)
                {
                    if(inVisibleCache) setLocationInVisibleCacheSimpleVoxelObject(tempvect, false);
                    if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[i]) delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
                    s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
                }
            }
            if(!full) s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = false;
        }
    }
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isVisible(Vector3d<int> vect, bool* renderfaces)
{
    VoxelAddress vxladdr(*(getVoxelAddress()));
    vxladdr.addNextVector(vect);
    int numbinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    return getWorld()->isVisible(&vxladdr, renderfaces, s_SimpleVoxelObjects[numbinmemory]);
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getRenderFacesSimpleVoxelObjectInCache(Vector3d<int> vect)
{
    if(isExistsInMemory(vect, VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT)) return s_CacheVisibleSimpleVoxelObjectsRenderFaces[getNumberInMemorySimpleVoxelObjectByVector(vect)];
    return 0;
}

int SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberInMemorySimpleVoxelObjectByVector(Vector3d<int> vect)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    int numberinmemory = vect.getX() + snoptemp*vect.getY() + snoptemp*snoptemp*vect.getZ();
    if(numberinmemory < 0 || numberinmemory >= snoptemp*snoptemp*snoptemp)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error: vector is not in the chunk. Vector: " + vect.toString(", "));
        return 0;
    }
    return numberinmemory;
}

Vector3d<int> SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVectorSimpleVoxelObjectByNumberInMemory(int numb)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    int temp1 = numb % (snoptemp*snoptemp);
    int temp2 = temp1 % snoptemp;
    int z = (numb - temp1) / (snoptemp*snoptemp);
    int y = (temp1 - temp2) / snoptemp;
    int x = temp2;
    return Vector3d<int>(x, y, z);
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setWorld(World* wrld)
{
    if(s_World)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world. Voxel address already setted.");
        return false;
    }
    s_World = wrld;
    return true;
}

World* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getWorld()
{
    return s_World;
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getReCalculateVisibleCacheSimpleVoxelObjects()
{
    if(!s_ReCalculateVisibleCacheSimpleVoxelObjects) createData();
    return s_ReCalculateVisibleCacheSimpleVoxelObjects;
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getCacheVisibleSimpleVoxelObjects()
{
    if(!s_CacheVisibleSimpleVoxelObjects) createData();
    return s_CacheVisibleSimpleVoxelObjects;
}

bool** SimpleVoxelEngine::Voxels::ChunkVoxelObject::getCacheVisibleSimpleVoxelObjectsRenderFaces()
{
    if(!s_CacheVisibleSimpleVoxelObjectsRenderFaces) createData();
    return s_CacheVisibleSimpleVoxelObjectsRenderFaces;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setNumberOfNotNullSimpleVoxelObjects(unsigned int notnullsvos)
{
    s_NumberOfNotNullSimpleVoxelObjects = notnullsvos;
}
