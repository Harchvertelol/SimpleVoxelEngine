#include "UserControllerForThread.h"

#include "IrrSimpleVoxelEngine.h"

using namespace SimpleVoxelEngine;
using namespace IrrEngine;

UserControllerForThread::UserControllerForThread(IrrSimpleVoxelEngine* isve):
    s_ISVE(isve)
{
    //...
}

UserControllerForThread::~UserControllerForThread()
{
    //...
}

void UserControllerForThread::userThreadFunction(ParametersForThreadFunction* pftf)
{
    //...
}

IrrSimpleVoxelEngine* UserControllerForThread::getIrrSimpleVoxelEngine()
{
    return s_ISVE;
}
