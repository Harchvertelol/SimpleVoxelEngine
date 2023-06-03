#include "Engine.h"

using namespace SimpleVoxelEngine;
using namespace Debug;
using namespace Defines;
using namespace Core;

using namespace std;

SimpleVoxelEngine::Core::Engine::Engine():
    s_Logger(0),
    s_NumberWorlds(0)
{
    //...
}

SimpleVoxelEngine::Core::Engine::Engine(Logger* lgr):
    s_Logger(lgr),
    s_NumberWorlds(0)
{
    if(s_Logger) s_Logger->grab();
}

SimpleVoxelEngine::Core::Engine::~Engine()
{
    map< int, World* >::iterator iter, iter2;
    for(iter = s_Worlds.begin(), iter2 = s_Worlds.end(); iter != iter2;)
    {
        delete iter->second;
        ++iter;
    }
    s_Worlds.clear();
    if(s_Logger) s_Logger->drop();
}

void SimpleVoxelEngine::Core::Engine::registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    if(s_Logger) s_Logger->registerEvent(TypeEvent, TextEvent, IsFatal);
}

int SimpleVoxelEngine::Core::Engine::addWorld()
{
    WorldCreatingParameters wcp;
    return addWorld(wcp);
}

int SimpleVoxelEngine::Core::Engine::addWorld(WorldCreatingParameters& wcp)
{
    World* wrld = new World(this, wcp);
    int id = s_NumberWorlds;
    s_Worlds[id] = wrld;
    if(!s_Worlds[id])
    {
        registerEvent(EVENT_TYPE_ERROR, "Error adding world", true);
        return -1;
    }
    s_Worlds[id]->setId(id);
    s_NumberWorlds++;
    return id;
}

int SimpleVoxelEngine::Core::Engine::addWorld(World* wrld)
{
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_ERROR, "Error adding world. World pointer is null", true);
        return -1;
    }
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world to engine. World pointer is NULL.");
        return -1;
    }
    if(wrld->getEngine() != this)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world to engine. World already setted in other engine.");
        return -1;
    }
    if(wrld->getId() != 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world to engine. World id already setted.");
        return -1;
    }
    int id = s_NumberWorlds;
    s_Worlds[id] = wrld;
    if(!s_Worlds[id])
    {
        registerEvent(EVENT_TYPE_ERROR, "Error adding world.", true);
        return -1;
    }
    s_Worlds[id]->setId(id);
    s_NumberWorlds++;
    return id;
}

World* SimpleVoxelEngine::Core::Engine::getWorld(int id)
{
    World* wrld = s_Worlds[id];
    if(!wrld) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting world with ID = " + WorkFunctions::ConvertFunctions::itos(id) + ". The world does not exist", true);
    return wrld;
}

Logger* SimpleVoxelEngine::Core::Engine::getLogger()
{
    if(s_Logger) s_Logger->grab();
    return s_Logger;
}
