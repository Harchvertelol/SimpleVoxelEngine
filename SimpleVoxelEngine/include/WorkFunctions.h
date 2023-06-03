#ifndef WORKFUNCTION_H
#define WORKFUNCTION_H
#include <sstream>
#include <map>

#include "Defines.h"

//! Отдельное пространство имён со специальными функциями для работы.
/*! Содержит отдельные от движка, но полезные функции.
*/

namespace WorkFunctions
{
    //! Функции для работы со словами.
    namespace WordFunctions
    {
        //! Получение первого слова.
        /*! \param str Обрабатываемая строка.
        \return Возвращает первое слово (до пробела) из строки str.
        */
        std::string FirstWord(std::string str);
        //! Удаление первого слова.
        /*! \param str Обрабатываемая строка.
        \return Возвращает str без первого слова.
        */
        std::string RemoveFirstWord(std::string str);
    };
    //! Функции для работы ParserInfoFile.
    namespace ParserFunctions
    {
        //! Получение имени блока.
        /*! \param str Обрабатываемая строка.
        \return Возвращает название блока.
        */
        std::string getNameMainVariable(std::string str);
        //! Получение имени переменной (из строки вида NAME=VALUE).
        /*! \param str Обрабатываемая строка.
        \return Возвращает имя переменной.
        */
        std::string getNameSecondaryVariable(std::string str);
        //! Получение значения переменной (из строки вида NAME=VALUE).
        /*! \param str Обрабатываемая строка.
        \return Возвращает значение переменной.
        */
        std::string getValueSecondaryVariable(std::string str);
        //! Разделение строки в массив int.
        /*! \param mas Указатель на map, в который будет записан результат.
        \param size Неиспользуемый параметр. Можно ставить его любому числу. Например, нулю.
        \param point С какой точки начинать запись в массив. Чтобы запись началась с начала, необходимо положить point = 0.
        \param str Обрабатываемая строка.
        \param splitter По какому символу производить разделение (длина splitter должна быть равна 1).
        \return Возвращает длину получившегося массива.
        */
        int splitMass(std::map<int,int>* mas, int size, int point, std::string str, std::string splitter);
        //! Разделение строки в массив string.
        /*! \param mas Указатель на map, в который будет записан результат.
        \param size Неиспользуемый параметр. Можно ставить его любому числу. Например, нулю.
        \param point С какой точки начинать запись в массив. Чтобы запись началась с начала, необходимо положить point = 0.
        \param str Обрабатываемая строка.
        \param splitter По какому символу производить разделение (длина splitter должна быть равна 1).
        \return Возвращает длину получившегося массива.
        */
        int splitMassString(std::map<int,std::string>* mas, int size, int point, std::string str, std::string splitter);
        //! Добавление в строку блока.
        /*! \param str Обрабатываемая строка.
        \param name Имя блока.
        \param splitter Разделитель в строке вместо перехода на новую строку.
        \return Возвращает строку с добавленным блоком.
        */
        std::string addMainVariableString(std::string str, std::string name, std::string splitter);
        //! Добавление в строку переменной.
        /*! \param str Обрабатываемая строка.
        \param name Имя блока.
        \param value Значение переменной.
        \param splitter Разделитель в строке вместо перехода на новую строку.
        \return Возвращает строку с добавленным блоком.
        */
        std::string addSecondaryVariableString(std::string str, std::string name, std::string value, std::string splitter);
    };
    //! Функции конвертации типов.
    namespace ConvertFunctions
    {
        //! Конвертация int в string.
        /*! Может использоваться для вывода числовых значений в систему логирования движка.
        \param number Конвертируемое число.
        \return Возвращает строку, содержащую число.
        */
        std::string itos(int number);
        //! Конвертация float в string.
        /*! Может использоваться для вывода числовых значений в систему логирования движка.
        \param number Конвертируемое число.
        \return Возвращает строку, содержащую число.
        */
        std::string ftos(float number);
    };
    //! Математические функции
    namespace MathFunctions
    {
        //! Округление числа.
        /*! Округляет целое число до необходимой делимости.
        \param number Округляемое число.
        \param divide На что должно делиться после округления.
        \param change На сколько изменять число за шаг (Для округления вверх ставим 1, для округления вниз ставим -1). Если неправильно поставить change, то программа может зависнуть.
        \return Возвращает округлённое число.
        */
        int roundNumber(int number, int divide, int change);
    };
    //! Функции работы с файлами.
    namespace FileFunctions
    {
        //! Проверка существования файла.
        /*! \param name Имя проверяемого файла.
        \return Возвращает true в случае существования файла, иначе false.
        */
        bool fileExists(std::string name);
        //! Функция создания папок.
        /*! Создаёт все необходимые папки по пути folder.
        \param folder Путь, который необходимо создать.
        \param foldersplitter Разделитель имён папок в folder.
        \return Возвращает true в случае успеха, в противном случае false.
        */
        bool createFolders(std::string folder, std::string foldersplitter = SimpleVoxelEngine::Defines::STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
    };
};
#endif
