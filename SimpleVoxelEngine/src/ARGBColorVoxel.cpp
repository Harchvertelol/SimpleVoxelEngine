#include "ARGBColorVoxel.h"

#include "Engine.h"

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Additional;
using namespace Defines;
using namespace Voxels;

using namespace std;

SimpleVoxelEngine::Voxels::ARGBColorVoxel::ARGBColorVoxel():
    SimpleVoxelObject(),
    s_ARGBColor(ARGBColor(0,0,0,0))
{
    setTypeVoxel(ARGBCOLOR);
}

SimpleVoxelEngine::Voxels::ARGBColorVoxel::ARGBColorVoxel(ARGBColor& argbcolor):
    SimpleVoxelObject(),
    s_ARGBColor(argbcolor)
{
    setTypeVoxel(ARGBCOLOR);
}

SimpleVoxelEngine::Voxels::ARGBColorVoxel::ARGBColorVoxel(ARGBColor argbcolor):
    SimpleVoxelObject(),
    s_ARGBColor(argbcolor)
{
    setTypeVoxel(ARGBCOLOR);
}

SimpleVoxelEngine::Voxels::ARGBColorVoxel::~ARGBColorVoxel()
{
    //...
}

void SimpleVoxelEngine::Voxels::ARGBColorVoxel::setARGBColor(ARGBColor argbcolor)
{
    s_ARGBColor = argbcolor;
}

ARGBColor SimpleVoxelEngine::Voxels::ARGBColorVoxel::getARGBColor()
{
    return s_ARGBColor;
}

VoxelInfo* SimpleVoxelEngine::Voxels::ARGBColorVoxel::getVoxelForRenderingInfo()
{
    VoxelInfoARGBColor* vxlnf = new VoxelInfoARGBColor;
    vxlnf->s_TypeVoxelInfo = ARGBCOLOR_VOXEL_INFO;
    vxlnf->s_ARGBColor = s_ARGBColor;
    return vxlnf;
}
