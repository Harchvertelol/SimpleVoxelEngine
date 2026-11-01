#include "VoxelsForRendering.h"

using namespace SimpleVoxelEngine;
using namespace Core;

SimpleVoxelEngine::Core::VoxelsForRendering::VoxelsForRendering():
    s_NumberOfVoxelsForRendering(0)
{
    //...
}

SimpleVoxelEngine::Core::VoxelsForRendering::~VoxelsForRendering()
{
    for(int i = 0; i < s_NumberOfVoxelsForRendering; i++) delete s_VFR[i];
    s_VFR.clear();
}

void SimpleVoxelEngine::Core::VoxelsForRendering::addVoxelForRendering(VoxelForRendering* vfr)
{
    s_VFR[s_NumberOfVoxelsForRendering] = vfr;
    s_NumberOfVoxelsForRendering++;
}

int SimpleVoxelEngine::Core::VoxelsForRendering::getNumberOfVoxelsForRendering()
{
    return s_NumberOfVoxelsForRendering;
}

VoxelForRendering* SimpleVoxelEngine::Core::VoxelsForRendering::getVoxelForRendering(int ind)
{
    //»справлено: operator[] при неверном номере вставл€л в карту пустую запись. “еперь просто NULL.
    std::map<int, VoxelForRendering*>::iterator iter = s_VFR.find(ind);
    return iter == s_VFR.end() ? 0 : iter->second;
}
