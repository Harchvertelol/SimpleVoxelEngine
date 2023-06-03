#include "VoxelForRendering.h"

using namespace SimpleVoxelEngine;
using namespace Additional;
using namespace Core;

SimpleVoxelEngine::Core::VoxelForRendering::VoxelForRendering(Vector3d<float> pos, Vector3d<float> rot, Vector3d<float> scale):
    s_Position(pos),
    s_Rotation(rot),
    s_Scale(scale),
    s_VoxelInfo(0)
{
    //...
}

SimpleVoxelEngine::Core::VoxelForRendering::VoxelForRendering():
    s_Position(Vector3d<float>(0,0,0)),
    s_Rotation(Vector3d<float>(0,0,0)),
    s_Scale(Vector3d<float>(0,0,0)),
    s_VoxelInfo(0)
{
    //...
}

SimpleVoxelEngine::Core::VoxelForRendering::VoxelForRendering(const VoxelForRendering& vfr)
{
    if(this != &vfr)
    {
        s_Position = vfr.getPosition();
        s_Rotation = vfr.getRotation();
        s_Scale = vfr.getScale();
        for(int i = 0; i < 6; i++) setRenderFace(i, vfr.isRenderFace(i));
        if(s_VoxelInfo) delete s_VoxelInfo;
        s_VoxelInfo = vfr.getVoxelInfo();
    }
}

SimpleVoxelEngine::Core::VoxelForRendering::~VoxelForRendering()
{
    if(s_VoxelInfo) delete s_VoxelInfo;
}

VoxelInfo* SimpleVoxelEngine::Core::VoxelForRendering::getVoxelInfo() const
{
    return s_VoxelInfo;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setVoxelInfo(VoxelInfo* vxlinfo)
{
    if(s_VoxelInfo) delete s_VoxelInfo;
    s_VoxelInfo = vxlinfo;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getPosition() const
{
    return s_Position;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getRotation() const
{
    return s_Rotation;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getScale() const
{
    return s_Scale;
}

VoxelForRendering& SimpleVoxelEngine::Core::VoxelForRendering::operator=(VoxelForRendering& vfr)
{
    if(this != &vfr)
    {
        s_Position = vfr.getPosition();
        s_Rotation = vfr.getRotation();
        s_Scale = vfr.getScale();
        for(int i = 0; i < 6; i++) setRenderFace(i, vfr.isRenderFace(i));
        if(s_VoxelInfo) delete s_VoxelInfo;
        s_VoxelInfo = vfr.getVoxelInfo();
    }
    return *this;
}

bool SimpleVoxelEngine::Core::VoxelForRendering::isRenderFace(int numb) const
{
    if(numb > 5) return false;
    return s_Faces[numb];
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRenderFace(int numb, bool value)
{
    if(numb > 5) return;
    s_Faces[numb] = value;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRenderFaces(bool* faces)
{
    for(int i = 0; i < 6; i++) s_Faces[i] = faces[i];
}

void SimpleVoxelEngine::Core::VoxelForRendering::setPosition(Vector3d<float> newposition)
{
    s_Position = newposition;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRotation(Vector3d<float> newrotation)
{
    s_Rotation = newrotation;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setScale(Vector3d<float> newscale)
{
    s_Scale = newscale;
}
