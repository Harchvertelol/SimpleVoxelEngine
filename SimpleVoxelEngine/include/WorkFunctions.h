#ifndef WORKFUNCTION_H
#define WORKFUNCTION_H
#include <sstream>
#include <map>

#include "Defines.h"

//! ��������� ������������ ��� �� ������������ ��������� ��� ������.
/*! �������� ��������� �� ������, �� �������� �������.
*/

namespace WorkFunctions
{
    //! ������� ��� ������ �� �������.
    namespace WordFunctions
    {
        //! ��������� ������� �����.
        /*! \param str �������������� ������.
        \return ���������� ������ ����� (�� �������) �� ������ str.
        */
        std::string FirstWord(std::string str);
        //! �������� ������� �����.
        /*! \param str �������������� ������.
        \return ���������� str ��� ������� �����.
        */
        std::string RemoveFirstWord(std::string str);
    };
    //! ������� ��� ������ ParserInfoFile.
    namespace ParserFunctions
    {
        //! ��������� ����� �����.
        /*! \param str �������������� ������.
        \return ���������� �������� �����.
        */
        std::string getNameMainVariable(std::string str);
        //! ��������� ����� ���������� (�� ������ ���� NAME=VALUE).
        /*! \param str �������������� ������.
        \return ���������� ��� ����������.
        */
        std::string getNameSecondaryVariable(std::string str);
        //! ��������� �������� ���������� (�� ������ ���� NAME=VALUE).
        /*! \param str �������������� ������.
        \return ���������� �������� ����������.
        */
        std::string getValueSecondaryVariable(std::string str);
        //! ���������� ������ � ������ int.
        /*! \param mas ��������� �� map, � ������� ����� ������� ���������.
        \param size �������������� ��������. ����� ������� ��� ������ �����. ��������, ����.
        \param point � ����� ����� �������� ������ � ������. ����� ������ �������� � ������, ���������� �������� point = 0.
        \param str �������������� ������.
        \param splitter �� ������ ������� ����������� ���������� (����� splitter ������ ���� ����� 1).
        \return ���������� ����� ������������� �������.
        */
        int splitMass(std::map<int,int>* mas, int size, int point, std::string str, std::string splitter);
        //! ���������� ������ � ������ string.
        /*! \param mas ��������� �� map, � ������� ����� ������� ���������.
        \param size �������������� ��������. ����� ������� ��� ������ �����. ��������, ����.
        \param point � ����� ����� �������� ������ � ������. ����� ������ �������� � ������, ���������� �������� point = 0.
        \param str �������������� ������.
        \param splitter �� ������ ������� ����������� ���������� (����� splitter ������ ���� ����� 1).
        \return ���������� ����� ������������� �������.
        */
        int splitMassString(std::map<int,std::string>* mas, int size, int point, std::string str, std::string splitter);
        //! ���������� � ������ �����.
        /*! \param str �������������� ������.
        \param name ��� �����.
        \param splitter ����������� � ������ ������ �������� �� ����� ������.
        \return ���������� ������ � ����������� ������.
        */
        std::string addMainVariableString(std::string str, std::string name, std::string splitter);
        //! ���������� � ������ ����������.
        /*! \param str �������������� ������.
        \param name ��� �����.
        \param value �������� ����������.
        \param splitter ����������� � ������ ������ �������� �� ����� ������.
        \return ���������� ������ � ����������� ������.
        */
        std::string addSecondaryVariableString(std::string str, std::string name, std::string value, std::string splitter);
    };
    //! ������� ����������� �����.
    namespace ConvertFunctions
    {
        //! ����������� int � string.
        /*! ����� �������������� ��� ������ �������� �������� � ������� ����������� ������.
        \param number �������������� �����.
        \return ���������� ������, ���������� �����.
        */
        std::string itos(int number);
        //! ����������� float � string.
        /*! ����� �������������� ��� ������ �������� �������� � ������� ����������� ������.
        \param number �������������� �����.
        \return ���������� ������, ���������� �����.
        */
        std::string ftos(float number);
    };
    //! �������������� �������
    namespace MathFunctions
    {
        //! ���������� �����.
        /*! ��������� ����� ����� �� ����������� ���������.
        \param number ����������� �����.
        \param divide �� ��� ������ �������� ����� ����������.
        \param change �� ������� �������� ����� �� ��� (��� ���������� ����� ������ 1, ��� ���������� ���� ������ -1). ���� ����������� ��������� change, �� ��������� ����� ���������.
        \return ���������� ���������� �����.
        */
        int roundNumber(int number, int divide, int change);
    };
    //! ������� ������ � �������.
    namespace FileFunctions
    {
        //! �������� ������������� �����.
        /*! \param name ��� ������������ �����.
        \return ���������� true � ������ ������������� �����, ����� false.
        */
        bool fileExists(std::string name);
        //! ������� �������� �����.
        /*! ������ ��� ����������� ����� �� ���� folder.
        \param folder ����, ������� ���������� �������.
        \param foldersplitter ����������� ��� ����� � folder.
        \return ���������� true � ������ ������, � ��������� ������ false.
        */
        bool createFolders(std::string folder, std::string foldersplitter = SimpleVoxelEngine::Defines::STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
    };
};
#endif
