#include "Additional/ReferenceCounter.h"

SimpleVoxelEngine::Additional::ReferenceCounter::ReferenceCounter():
    s_ReferenceCounter(1)
{
    //...
}

SimpleVoxelEngine::Additional::ReferenceCounter::~ReferenceCounter()
{
    //...
}

void SimpleVoxelEngine::Additional::ReferenceCounter::grab() const
{
    ++s_ReferenceCounter;
}

bool SimpleVoxelEngine::Additional::ReferenceCounter::drop() const
{
    --s_ReferenceCounter;
    if (!s_ReferenceCounter)
    {
        delete this;
        return true;
    }
    return false;
}

int SimpleVoxelEngine::Additional::ReferenceCounter::getReferenceCount() const
{
    return s_ReferenceCounter;
}
