#include "Logger.h"

#include <iostream>

#include "Defines.h"

using namespace SimpleVoxelEngine;
using namespace Defines;
using namespace Debug;

//»зменено: блокировки вывода в логгере больше нет - €дро однопоточное. ≈сли движком пользуютс€ из нескольких потоков,
//вывод сообщений по одному обеспечивает наследник движка, переопределив Engine::registerEvent() (сво€ блокировка).

Logger::Logger():
    ReferenceCounter(),
    s_PrintError(true),
    s_PrintLogicViolation(true),
    s_PrintWarning(true),
    s_PrintStandard(true),
    s_PrintInfo(true)
{}

Logger::~Logger()
{}

//»зменено: им€ - registerEventStandardConsole() (опечатка Standart).
void Logger::registerEventStandardConsole(EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    if(!isPrinted(TypeEvent)) return; //—ообщение этого типа не выводитс€
    //»зменено: один вывод вместо п€ти одинаковых веток; перед "Fatal!" точка ставитс€, только если
    //сообщение ещЄ не кончаетс€ точкой или "!" (раньше получалось "Chunk is not loaded.. Fatal!").
    const char* prefix = "Information: ";
    switch(TypeEvent)
    {
        case EVENT_TYPE_ERROR: prefix = "Error: "; break;
        case EVENT_TYPE_LOGIC_VIOLATION: prefix = "Logic violation: "; break;
        case EVENT_TYPE_WARNING: prefix = "Warning: "; break;
        case EVENT_TYPE_STANDARD: prefix = "Standard: "; break;
        case EVENT_TYPE_INFO: prefix = "Information: "; break;
    }
    const bool endsWithMark = !TextEvent.empty() && (TextEvent[TextEvent.size() - 1] == '.' || TextEvent[TextEvent.size() - 1] == '!');
    std::cout<<prefix<<TextEvent;
    if(IsFatal == true) std::cout<<(endsWithMark ? " Fatal!" : ". Fatal!");
    std::cout<<std::endl;
}

//ƒобавлено: вывод€тс€ ли сообщени€ этого типа (дл€ registerEventStandardConsole() и Engine::isEventPrinted()).
bool Logger::isPrinted(EventType TypeEvent) const
{
    switch(TypeEvent)
    {
        case EVENT_TYPE_ERROR: return s_PrintError;
        case EVENT_TYPE_LOGIC_VIOLATION: return s_PrintLogicViolation;
        case EVENT_TYPE_WARNING: return s_PrintWarning;
        case EVENT_TYPE_STANDARD: return s_PrintStandard;
        case EVENT_TYPE_INFO: return s_PrintInfo;
    }
    return false;
}

void Logger::registerEvent(EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    registerEventStandardConsole(TypeEvent, TextEvent, IsFatal);
}

void Logger::setPrintError(bool value)
{
    s_PrintError = value;
}

bool Logger::getPrintError()
{
    return s_PrintError;
}

void Logger::setPrintLogicViolation(bool value)
{
    s_PrintLogicViolation = value;
}

bool Logger::getPrintLogicViolation()
{
    return s_PrintLogicViolation;
}

void Logger::setPrintWarning(bool value)
{
    s_PrintWarning = value;
}

bool Logger::getPrintWarning()
{
    return s_PrintWarning;
}

void Logger::setPrintStandard(bool value)
{
    s_PrintStandard = value;
}
bool Logger::getPrintStandard()
{
    return s_PrintStandard;
}

void Logger::setPrintInfo(bool value)
{
    s_PrintInfo = value;
}
bool Logger::getPrintInfo()
{
    return s_PrintInfo;
}
