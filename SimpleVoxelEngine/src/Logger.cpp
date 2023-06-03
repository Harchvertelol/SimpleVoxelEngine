#include "Logger.h"

#include <iostream>

#include "Defines.h"

using namespace SimpleVoxelEngine;
using namespace Defines;
using namespace Debug;

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

void Logger::registerEventStandartConsole(EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    if(TypeEvent == EVENT_TYPE_ERROR && s_PrintError)
    {
        std::cout<<"Error: "<<TextEvent;
        if(IsFatal == true) std::cout<<". Fatal!";
        std::cout<<std::endl;
    }
    if(TypeEvent == EVENT_TYPE_LOGIC_VIOLATION && s_PrintLogicViolation)
    {
        std::cout<<"Logic violation: "<<TextEvent;
        if(IsFatal == true) std::cout<<". Fatal!";
        std::cout<<std::endl;
    }
    if(TypeEvent == EVENT_TYPE_WARNING && s_PrintWarning)
    {
        std::cout<<"Warning: "<<TextEvent;
        if(IsFatal == true) std::cout<<". Fatal!";
        std::cout<<std::endl;
    }
    if(TypeEvent == EVENT_TYPE_STANDARD && s_PrintStandard)
    {
        std::cout<<"Standard: "<<TextEvent;
        if(IsFatal == true) std::cout<<". Fatal!";
        std::cout<<std::endl;
    }
    if(TypeEvent == EVENT_TYPE_INFO && s_PrintInfo)
    {
        std::cout<<"Information: "<<TextEvent;
        if(IsFatal == true) std::cout<<". Fatal!";
        std::cout<<std::endl;
    }
}

void Logger::registerEvent(EventType TypeEvent, std::string TextEvent, bool IsFatal)
{
    registerEventStandartConsole(TypeEvent, TextEvent, IsFatal);
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
