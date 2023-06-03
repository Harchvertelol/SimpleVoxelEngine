#include "ChunksSaveLoadController.h"

#include "World.h"

#include "WorkFunctions.h"

#include <map>

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Additional;
using namespace Defines;
using namespace STRING_CONSTANTS;

using namespace WorkFunctions;
using namespace ParserFunctions;

using namespace std;

SimpleVoxelEngine::Core::ChunksSaveLoadController::ChunksSaveLoadController():
    s_World(0)
{
    //...
}

SimpleVoxelEngine::Core::ChunksSaveLoadController::~ChunksSaveLoadController()
{
    //...
}

void SimpleVoxelEngine::Core::ChunksSaveLoadController::setWorld(World* wrld)
{
    s_World = wrld;
}

World* SimpleVoxelEngine::Core::ChunksSaveLoadController::getWorld()
{
    return s_World;
}
