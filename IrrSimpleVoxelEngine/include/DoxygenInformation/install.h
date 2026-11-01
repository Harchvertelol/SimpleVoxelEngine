/*! \page install Настройка враппера IrrSimpleVoxelEngine для работы.
 Для работы с враппером необходимо наличие двух движков: SimpleVoxelEngine
 и Irrlicht (проверено с Irrlicht 1.8.4 и MinGW-w64 GCC 8.1, 64 бита). Подключим их:<br>
 -# Добавить в линковку Debug библиотеку PATH_TO_IRRSIMPLEVOXELENGINE/lib/Debug/libIrrSimpleVoxelEngine.a
 -# Добавить в линковку Release библиотеку PATH_TO_IRRSIMPLEVOXELENGINE/lib/Release/libIrrSimpleVoxelEngine.a
 -# Добавить в линковку Debug библиотеку PATH_TO_SIMPLEVOXELENGINE/lib/Debug/libSimpleVoxelEngine.a
 -# Добавить в линковку Release библиотеку PATH_TO_SIMPLEVOXELENGINE/lib/Release/libSimpleVoxelEngine.a
 -# Добавить в линковку библиотеку Irrlicht (libIrrlicht.a) - после двух библиотек выше
 -# Добавить в пути поиска папки PATH_TO_IRRSIMPLEVOXELENGINE/include, PATH_TO_SIMPLEVOXELENGINE/include и include Irrlicht
 -# Включить C++11 (-std=c++11): враппер использует std::thread, std::mutex и std::atomic
 -# Положить рядом с программой Irrlicht.dll той же разрядности, что и программа

 Библиотеки собираются в порядке SimpleVoxelEngine, затем IrrSimpleVoxelEngine, затем ваша программа.
 После этого можете компилировать любой короткий пример.
 Простейший код:
 \code
 #include <IrrSimpleVoxelEngine.h>
 int main()
 {
     irr::IrrlichtDevice* device = irr::createDevice(irr::video::EDT_NULL);
     SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine* isve = new SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine(device);
     delete isve;
     device->drop();
     return 0;
 }
 \endcode
 Если он скомпилировался и запустился, то вы всё настроили правильно.
*/
