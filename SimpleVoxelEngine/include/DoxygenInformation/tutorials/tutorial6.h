/*! \page tutorial6 ���� 6: ������ � ���������� ���������� �������� ini-������.
 ������ � ������ ������������ ���������� ������ ini-������ ���������� �������:
 \code
 [mainvariable1]
 secondvar1=value
 secondvar2=value
 secondvar3=value
 secondvar4=value
 secondvar5=value

 [mainvariable2]
 secondvar1=value
 secondvar2=value
 secondvar3=value
 secondvar4=value
 secondvar5=value
 ...
 \endcode
 � ���� ����� �� �������� � ��� ��������. � ������ �� ����������: ���������
 ������ � ������������ ���, ��� ���� ��������� ������������� ������������
 ��� ������ �������:
 \code
 #include <SimpleVoxelEngine.h>

 #include <iostream> // ��� ������������� std::cout.

 #include <cstdlib> // ��� ������� atoi.

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 using namespace IniParser; // ��������� ������������ ��� ������ �������.

 using namespace std; // ��� ������ �� �����.
 \endcode
 ������� ������� main � ��������� ������ ������� ��� ��������� ���������
 ini-�����:
 \code
 int main()
 {
    ParserInfoFile prs; // ������� ��������� ������ ��� ��������� ini-������.
 \endcode
 ����� ������ ���� � ������ voxelgame.ini, ������� ����� ����� � exe-������ �
 ����� ��������� ����������:
 \code
 [main]
 worldname=mysuperworld
 sizeworldinchunks=25

 [gamemode]
 admin=creative
 player=survival
 defaultmodeforall=player

 [other]
 serverip=127.0.0.1
 serverport=45678
 \endcode
 �� ��� ��, ��������� ���:
 \code
 PostParsingStruct* pps = prs.getParsedFromFile("voxelgame.ini"); // ��������� � pps ��� ������ �� �����.
 if(!pps) return 1; // ���� ���� �� ������ �������, �� ������� �� ���������.
 \endcode
 ��� � ��. ������ ������ ������� ����� ������ �� �����:
 \code
 cout << pps->getValue("main", "worldname") << endl;
 cout << pps->getValue("gamemode", "player") << endl;
 cout << pps->getValue("other", "serverip") << endl;
 \endcode
 �� ��� ������, ���� �� ����� ��������, � �������, serverip? ��� ��������
 �����-�� ����� ����������? ������ ��� ��������� �������:
 \code
 pps->setValue("other", "serverip", "192.168.0.100"); // �������� ������������ ����������.
 pps->setValue("other", "secondserverip", "127.0.0.1"); // �������� ����� ����������.
 pps->setValue("client", "id", "124"); // �������� ����� ���������� � ����� ����.
 \endcode
 ������ ��������� ������ - � ��� �������� � ��������� �����������, ���� ���
 ��� ����� ��� std::string? ����� ���� ����� - ��� ����� � ������� (� ������ � STL)
 ������ �������������� ������� ��������������:
 \code
 int sizeworld = atoi(pps->getValue("main", "sizeworldinchunks").c_str()); // ������������� std::string � int, ��� ���� � std::string ������� c_str() ��� �������������� � char*, ��� ��� ������� atoi ��������� char*, ��� ������� �� STL.
 cout << sizeworld << endl;
 sizeworld = 128;
 pps->setValue("main", "sizeworldinchunks", WorkFunctions::ConvertFunctions::itos(sizeworld)); // ������������� int � std::string, ��������� �������, ������ � �������.
 cout << pps->getValue("main", "sizeworldinchunks") << endl;
 \endcode
 ��� �� ���� ����������� ������������� PostParsingStruct � std::string � ��������� ������������
 �������� �����. ��� �����, � �������, ��� ��������� ������� �� ���� � ��� �����. ��� �� ���� ��������
 �����������, std::string � PostParsingStruct. �������� ��� ��������� �������:
 \code
 string str = prs.convertPostParsingStructToString(pps, ";"); // ��������� ����� ������ � �������.
 cout << str << endl; // ���������, ��� ����������.
 PostParsingStruct* newpps = prs.getParsedFromString(str, ";"); // ������� ���������� ini-����� �� std::string, ��������� ������������ ����� � �������.
 delete pps; // ������ ������ PostParsingStruct, ��� ��� ����� ��� ��� ����������� ���� ����� ���������.
 \endcode
 �� �, ����� ������� ������������� ��������� ���� �����������, �������
 ��������� ��������� � ���� newvoxelgame.ini, ������ ��������� PostParsingStruct
 � �������� ���������:
 \code
    prs.writeParsedToFile(newpps, "newvoxelgame.ini");
    delete newpps;
    return 0;
 }
 \endcode
 ��������� ����� ���������� � ����� newvoxelgame.ini ����� exe �����. �� � ���������� ���� ���:
 \code
    #include <SimpleVoxelEngine.h>

    #include <iostream> // ��� ������������� std::cout.
    #include <cstdlib> // ��� ������� atoi.

    using namespace SimpleVoxelEngine;
    using namespace Core;
    using namespace Voxels;
    using namespace Additional;
    using namespace Defines;
    using namespace Debug;

    using namespace IniParser; // ��������� ������������ ��� ������ �������.

    using namespace std; // ��� ������ �� �����.

    int main()
    {
        ParserInfoFile prs; // ������� ��������� ������ ��� ��������� ini-������.
        PostParsingStruct* pps = prs.getParsedFromFile("voxelgame.ini"); // ��������� � pps ��� ������ �� �����.
        if(!pps) return 1; // ���� ���� �� ������ �������, �� ������� �� ���������.
        cout << pps->getValue("main", "worldname") << endl;
        cout << pps->getValue("gamemode", "player") << endl;
        cout << pps->getValue("other", "serverip") << endl;
        pps->setValue("other", "serverip", "192.168.0.100"); // �������� ������������ ����������.
        pps->setValue("other", "secondserverip", "127.0.0.1"); // �������� ����� ����������.
        pps->setValue("client", "id", "124"); // �������� ����� ���������� � ����� ����.
        int sizeworld = atoi(pps->getValue("main", "sizeworldinchunks").c_str()); // ������������� std::string � int, ��� ���� � std::string ������� c_str() ��� �������������� � char*, ��� ��� ������� atoi ��������� char*, ��� ������� �� STL.
        cout << sizeworld << endl;
        sizeworld = 128;
        pps->setValue("main", "sizeworldinchunks", WorkFunctions::ConvertFunctions::itos(sizeworld)); // ������������� int � std::string, ��������� �������, ������ � �������.
        cout << pps->getValue("main", "sizeworldinchunks") << endl;
        string str = prs.convertPostParsingStructToString(pps, ";"); // ��������� ����� ������ � �������.
        cout << str << endl; // ���������, ��� ����������.
        PostParsingStruct* newpps = prs.getParsedFromString(str, ";"); // ������� ���������� ini-����� �� std::string, ��������� ������������ ����� � �������.
        delete pps; // ������ ������ PostParsingStruct, ��� ��� ����� ��� ��� ����������� ���� ����� ���������.
        prs.writeParsedToFile(newpps, "newvoxelgame.ini");
        delete newpps;
        return 0;
    }
 \endcode
 ��� � �� �� ���� ���. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
