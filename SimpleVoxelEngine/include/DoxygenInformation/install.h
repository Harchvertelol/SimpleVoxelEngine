/*! \page install Настройка движка SimpleVoxelEngine для работы.
 Для работы с движокм необходимо выполнить следующие действия:<br>
 -# Добавить в линковку Debug библиотеку /lib/Debug/libSimpleVoxelEngine.a (она собирается с флагом -DSVE_DEBUG_CHECKS:
 проверяет ошибки программиста - координаты вне чанка, NULL вместо вокселя и др. - и пишет их в лог)
 -# Добавить в линковку Release библиотеку /lib/Release/libSimpleVoxelEngine.a
 -# Добавить в пути поиска папку /include

 После этого можете компилировать любой короткий пример.
 Простейший код:
 \code
 #include <SimpleVoxelEngine.h>
 int main()
 {
    SimpleVoxelEngine::Core::Engine* eng = new SimpleVoxelEngine::Core::Engine;
    SimpleVoxelEngine::Core::WorldCreatingParameters wcp; // Параметры мира.
    wcp.s_SizeChunk = 16; // Размер ребра чанка.
    eng->addWorld(wcp); // Создали мир с этими параметрами.
    delete eng;
    return 0;
 }
 \endcode
 Если он скомпилировался, то вы всё настроили правильно.
*/
