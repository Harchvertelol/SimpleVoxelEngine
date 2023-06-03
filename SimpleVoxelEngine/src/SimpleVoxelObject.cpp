#include "SimpleVoxelObject.h"

#include "Engine.h"
#include "World.h"

using namespace std;

using namespace SimpleVoxelEngine;
using namespace Defines;
using namespace Core;
using namespace Voxels;

SimpleVoxelEngine::Voxels::SimpleVoxelObject::SimpleVoxelObject()
{
    setTypeVoxel(TYPE_NONE);
}

SimpleVoxelEngine::Voxels::SimpleVoxelObject::~SimpleVoxelObject()
{
    //...
}

TYPE_VOXEL SimpleVoxelEngine::Voxels::SimpleVoxelObject::getTypeVoxel()
{
    return s_TypeVoxel;
}

void SimpleVoxelEngine::Voxels::SimpleVoxelObject::setTypeVoxel(TYPE_VOXEL tv)
{
    s_TypeVoxel = tv;
}

VoxelInfo* SimpleVoxelEngine::Voxels::SimpleVoxelObject::getVoxelForRenderingInfo()
{
    return 0;
}
