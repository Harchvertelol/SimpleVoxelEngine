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

//Добавлено: наследник, переопределивший registerEvent(), может сначала узнать, выведется ли сообщение (и не делать лишней работы).
bool SimpleVoxelEngine::Core::Engine::isEventPrinted(SimpleVoxelEngine::Defines::EventType TypeEvent) const
{
    return s_Logger && s_Logger->isPrinted(TypeEvent);
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
    if(wrld->getEngine() != this)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world to engine. World already setted in other engine.");
        return -1;
    }
    //Исправлено: было getId() != 0. Новый мир имеет id -1 (так его создают оба конструктора World), поэтому
    //любой новый мир отклонялся, а уже зарегистрированный мир с id 0 можно было добавить второй раз
    //(потом деструктор движка удалял его дважды).
    if(wrld->getId() != -1)
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
    //find вместо operator[]: operator[] вставлял бы пустую запись, т.е. менял карту при простом чтении
    //(а getWorld() может вызываться из нескольких потоков одновременно).
    map< int, World* >::iterator iter = s_Worlds.find(id);
    World* wrld = (iter == s_Worlds.end()) ? 0 : iter->second;
    if(!wrld) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting world with ID = " + WorkFunctions::ConvertFunctions::itos(id) + ". The world does not exist", true);
    return wrld;
}

Logger* SimpleVoxelEngine::Core::Engine::getLogger()
{
    if(s_Logger) s_Logger->grab();
    return s_Logger;
}
