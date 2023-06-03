#include "VoxelAddress.h"

#include "Engine.h"
#include "Defines.h"

using namespace std;

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Additional;
using namespace Defines;


SimpleVoxelEngine::Core::VoxelAddress::VoxelAddress(Engine* eng):
    s_Engine(eng),
    s_LevelShift(0),
    s_Up(true),
    s_LastVectInd(0)
{
    //...
}

SimpleVoxelEngine::Core::VoxelAddress::VoxelAddress(VoxelAddress& vxladdr):
    s_Engine(vxladdr.getEngine()),
    s_LevelShift(vxladdr.getLevelShift()),
    s_Up(vxladdr.isUp()),
    s_LastVectInd(vxladdr.getLastVectInd())
{
    for(int i = 0; i < vxladdr.getLastVectInd(); i++) s_Address[i] = vxladdr.getVector(i);
}

SimpleVoxelEngine::Core::VoxelAddress::~VoxelAddress()
{
    s_Address.clear();
}

VoxelAddress& SimpleVoxelEngine::Core::VoxelAddress::operator=(VoxelAddress& vxladdr)
{
    if(&vxladdr != this)
    {
        s_Address.clear();
        s_Engine = vxladdr.getEngine();
        s_LevelShift = vxladdr.getLevelShift();
        s_Up = vxladdr.isUp();
        s_LastVectInd = vxladdr.getLastVectInd();
        for(int i = 0; i < vxladdr.getLastVectInd(); i++) s_Address[i] = vxladdr.getVector(i);
    }
    return (*this);
}

Engine* SimpleVoxelEngine::Core::VoxelAddress::getEngine() const
{
    return s_Engine;
}

void SimpleVoxelEngine::Core::VoxelAddress::setLevelShift(int shift, bool up)
{
    s_LevelShift = shift;
}

void SimpleVoxelEngine::Core::VoxelAddress::addNextVector(Vector3d<int> vect)
{
    s_Address[s_LastVectInd] = vect;
    s_LastVectInd++;
}

bool SimpleVoxelEngine::Core::VoxelAddress::addVector(int ind, Vector3d<int> vect)
{
    if(ind > s_LastVectInd)
    {
        s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Vector to address not added. Index adding vector is more than last index.");
        return false;
    }
    if(ind == s_LastVectInd)
    {
        addNextVector(vect);
        return true;
    }
    s_Address[ind] = vect;
    return true;
}

SimpleVoxelEngine::Additional::Vector3d<int> SimpleVoxelEngine::Core::VoxelAddress::getVector(int ind)
{
    return s_Address[ind];
}

std::map< int, SimpleVoxelEngine::Additional::Vector3d<int> > SimpleVoxelEngine::Core::VoxelAddress::getAddress() const
{
    return s_Address;
}

int SimpleVoxelEngine::Core::VoxelAddress::getLevelShift() const
{
    return s_LevelShift;
}

bool SimpleVoxelEngine::Core::VoxelAddress::isUp() const
{
    return s_Up;
}

int SimpleVoxelEngine::Core::VoxelAddress::getLastVectInd() const
{
    return s_LastVectInd;
}

Vector3d<int> SimpleVoxelEngine::Core::VoxelAddress::getLastVector()
{
    if(getLastVectInd() > 0) return s_Address[getLastVectInd() - 1];
    return 0;
}

bool SimpleVoxelEngine::Core::VoxelAddress::deleteLastVector()
{
    if(s_LastVectInd == 0) return false;
    map< int, SimpleVoxelEngine::Additional::Vector3d<int> >::iterator I = s_Address.find(s_LastVectInd - 1);
    if ( I == s_Address.end() )
    {
        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Removal is not an existing vector from address.");
        return false;
    }
    s_LastVectInd--;
    s_Address.erase(s_LastVectInd);
    return true;
}
