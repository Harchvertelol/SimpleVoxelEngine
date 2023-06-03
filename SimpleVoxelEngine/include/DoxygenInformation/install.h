/*! \page install Настройка движка SimpleVoxelEngine для работы.
 Для работы с движокм необходимо выполнить следующие действия:<br>
 -# Добавить в линковку Debug библиотеку /lib/Debug/libSimpleVoxelEngine.a
 -# Добавить в линковку Release библиотеку /lib/Release/libSimpleVoxelEngine.a
 -# Добавить в пути поиска папку /include

 После этого можете компилировать любой короткий пример.
 Простейший код:
 \code
 #include <SimpleVoxelEngine.h>
 int main()
 {
    SimpleVoxelEngine::Core::Engine* eng = new SimpleVoxelEngine::Core::Engine;
    eng->addWorld("testworldfolder", 16);
    delete eng;
    return 0;
 }
 \endcode
 Если он скомпилировался, то вы всё настроили правильно.
*/
