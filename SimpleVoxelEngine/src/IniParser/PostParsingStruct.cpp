#include "IniParser/PostParsingStruct.h"

#include <iostream>

using namespace std;

IniParser::PostParsingStruct::PostParsingStruct()
{
    //...
}

IniParser::PostParsingStruct::~PostParsingStruct()
{
    map<string, map<string, string> >::iterator iter, iter2;
    for(iter = s_Variables.begin(), iter2 = s_Variables.end(); iter != iter2;)
    {
        s_Variables[iter->first].clear();
        ++iter;
    }
    s_Variables.clear();
}

string IniParser::PostParsingStruct::getValue(string A, string B)
{
    map<string, map<string, string> >::iterator I = s_Variables.find(A);
    if ( I == s_Variables.end() )
    {
        cout<<"Error: First value no setted. ( "<<A<<" )"<<endl;
        return "";
    }
    map<string, string>::iterator II = s_Variables[A].find(B);
    if ( II == s_Variables[A].end() )
    {
        cout<<"Error: Second value no setted. ( "<<A<<", "<<B<<" )"<<endl;
        return "";
    }
    return s_Variables[A][B];
}

void IniParser::PostParsingStruct::setValue(string A, string B, string C)
{
    s_Variables[A][B] = C;
}

map<string, map<string, string> >& IniParser::PostParsingStruct::getMapVariables()
{
    return s_Variables;
}
