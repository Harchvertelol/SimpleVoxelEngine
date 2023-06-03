/*! \page tutorial6 Урок 6: Работа с простейшим встроенным парсером ini-файлов.
 Вместе с движок поставляется простейший парсер ini-файлов следующего формата:
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
 В этом уроке мы научимся с ним работать. В начале всё стандартно: подключим
 движок и пространства имён, при этом подключим дополнительно пространство
 имён нашего парсера:
 \code
 #include <SimpleVoxelEngine.h>

 #include <iostream> // Для использования std::cout.

 #include <cstdlib> // Для функции atoi.

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 using namespace IniParser; // Подключим пространство имён нашего парсера.

 using namespace std; // Для вывода на экран.
 \endcode
 Объявим функцию main и экземпляр класса парсера для обработки заданного
 ini-файла:
 \code
 int main()
 {
    ParserInfoFile prs; // Создали экземпляр класса для обработки ini-файлов.
 \endcode
 Будем читать файл с именем voxelgame.ini, который лежит рядом с exe-файлом и
 имеет следующее содержимое:
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
 Ну что же, прочитаем его:
 \code
 PostParsingStruct* pps = prs.getParsedFromFile("voxelgame.ini"); // Прочитали в pps все данные из файла.
 if(!pps) return 1; // Если файл не смогли открыть, то выходим из программы.
 \endcode
 Вот и всё. Теперь просто выведем часть данных на экран:
 \code
 cout << pps->getValue("main", "worldname") << endl;
 cout << pps->getValue("gamemode", "player") << endl;
 cout << pps->getValue("other", "serverip") << endl;
 \endcode
 Но что делать, если мы хотим поменять, к примеру, serverip? Или добавить
 какую-то новую переменную? Делаем это следующим образом:
 \code
 pps->setValue("other", "serverip", "192.168.0.100"); // Поменяли существующую переменную.
 pps->setValue("other", "secondserverip", "127.0.0.1"); // Добавили новую переменную.
 pps->setValue("client", "id", "124"); // Добавили новую переменную в новый блок.
 \endcode
 Дальше возникает вопрос - а как работать с числовыми переменными, если они
 все имеют тип std::string? Ответ тоже прост - для этого с движком (и просто в STL)
 даются дополнительные функции преобразований:
 \code
 int sizeworld = atoi(pps->getValue("main", "sizeworldinchunks").c_str()); // Преобразовали std::string в int, при этом у std::string вызвали c_str() для преобразования в char*, так как функция atoi принимает char*, это функция из STL.
 cout << sizeworld << endl;
 sizeworld = 128;
 pps->setValue("main", "sizeworldinchunks", WorkFunctions::ConvertFunctions::itos(sizeworld)); // Преобразовали int в std::string, используя фунцкию, идущую с движком.
 cout << pps->getValue("main", "sizeworldinchunks") << endl;
 \endcode
 Так же есть возможность преобразовать PostParsingStruct в std::string с указанным разделителем
 переноса строк. Это нужно, к примеру, для пересылки пакетов по сети и так далее. Так же есть обратная
 конвертация, std::string в PostParsingStruct. Делается это следующим образом:
 \code
 string str = prs.convertPostParsingStructToString(pps, ";"); // Разделять будем точкой с запятой.
 cout << str << endl; // Посмотрим, что получилось.
 PostParsingStruct* newpps = prs.getParsedFromString(str, ";"); // Получим информацию ini-файла из std::string, используя разделителем точку с запятой.
 delete pps; // Удалим первую PostParsingStruct, так как зачем нам две дублирующие друг друга структуры.
 \endcode
 Ну и, чтобы увидеть окончательный результат всех манипуляций, запишем
 последнюю структуру в файл newvoxelgame.ini, удалим последнюю PostParsingStruct
 и завершим программу:
 \code
    prs.writeParsedToFile(newpps, "newvoxelgame.ini");
    delete newpps;
    return 0;
 }
 \endcode
 Результат можно посмотреть в файле newvoxelgame.ini около exe файла. Ну и напоследок весь код:
 \code
    #include <SimpleVoxelEngine.h>

    #include <iostream> // Для использования std::cout.
    #include <cstdlib> // Для функции atoi.

    using namespace SimpleVoxelEngine;
    using namespace Core;
    using namespace Voxels;
    using namespace Additional;
    using namespace Defines;
    using namespace Debug;

    using namespace IniParser; // Подключим пространство имён нашего парсера.

    using namespace std; // Для вывода на экран.

    int main()
    {
        ParserInfoFile prs; // Создали экземпляр класса для обработки ini-файлов.
        PostParsingStruct* pps = prs.getParsedFromFile("voxelgame.ini"); // Прочитали в pps все данные из файла.
        if(!pps) return 1; // Если файл не смогли открыть, то выходим из программы.
        cout << pps->getValue("main", "worldname") << endl;
        cout << pps->getValue("gamemode", "player") << endl;
        cout << pps->getValue("other", "serverip") << endl;
        pps->setValue("other", "serverip", "192.168.0.100"); // Поменяли существующую переменную.
        pps->setValue("other", "secondserverip", "127.0.0.1"); // Добавили новую переменную.
        pps->setValue("client", "id", "124"); // Добавили новую переменную в новый блок.
        int sizeworld = atoi(pps->getValue("main", "sizeworldinchunks").c_str()); // Преобразовали std::string в int, при этом у std::string вызвали c_str() для преобразования в char*, так как функция atoi принимает char*, это функция из STL.
        cout << sizeworld << endl;
        sizeworld = 128;
        pps->setValue("main", "sizeworldinchunks", WorkFunctions::ConvertFunctions::itos(sizeworld)); // Преобразовали int в std::string, используя фунцкию, идущую с движком.
        cout << pps->getValue("main", "sizeworldinchunks") << endl;
        string str = prs.convertPostParsingStructToString(pps, ";"); // Разделять будем точкой с запятой.
        cout << str << endl; // Посмотрим, что получилось.
        PostParsingStruct* newpps = prs.getParsedFromString(str, ";"); // Получим информацию ini-файла из std::string, используя разделителем точку с запятой.
        delete pps; // Удалим первую PostParsingStruct, так как зачем нам две дублирующие друг друга структуры.
        prs.writeParsedToFile(newpps, "newvoxelgame.ini");
        delete newpps;
        return 0;
    }
 \endcode
 Вот и всё на этот раз. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
