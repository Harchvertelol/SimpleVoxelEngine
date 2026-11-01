#ifndef SimpleVoxelEngine__Defines_H
#define SimpleVoxelEngine__Defines_H

#include <string>

//! \def SVE_DEBUG_CHECKS
/*! Отладочные проверки входных данных. Макрос нигде не определяется в коде - он задаётся флагом
компилятора -DSVE_DEBUG_CHECKS в Debug-цели проекта (библиотека в lib/Debug).
Без флага (Release, сборка без параметров) проверок нет совсем - ради скорости работы с вокселями.
С флагом проверяются ошибки программиста: координаты вектора внутри чанка (иначе, например, (16,0,0) в чанке 16
молча попадает в ячейку (0,1,0)), svo == NULL в ChunkVoxelObject::setSimpleVoxelObject(), чанк вне мира (без адреса
или мира) при построении меша и пересчёте видимости, отрицательный индекс в VoxelAddress::addVector(). При
ошибке пишется EVENT_TYPE_LOGIC_VIOLATION с пометкой "Debug check (SVE_DEBUG_CHECKS)", функция возвращает ошибку.
Заголовки от макроса не зависят, поэтому приложение можно собирать с любой из версий библиотек.
Данные извне (файлы сохранений, сеть) нужно проверять самому, в своём коде, - и в релизе тоже.
*/

namespace SimpleVoxelEngine
{
    namespace Defines
    {
        //! Рабочие константы для движка.
        enum WORK_CONSTANTS
        {
            FILE_READ_SIZE_STR = 5000, //!< Максимальное количество символов в строке при чтении текстового файла.
        };
        //! Типы SimpleVoxelObject.
        enum TYPE_VOXEL
        {
            TYPE_NONE, //!< Неустановленный тип.
            CHUNK_VOXEL_OBJECT, //!< Чанк.
            ARGBCOLOR, //!< ARGB цвет.
            MULTI_VOXEL, //!< Мультивоксель. Не используется.
            FILL, //!< Заливка. Не используется.
            CUSTOM_VOXEL //!< Кастомизированный SimpleVoxelObject, т.е. тип пользователя.
        };
        //! Уровень поиска SimpleVoxelObject (World::getAddressSimpleVoxelObjectByThreePoints(), getAddressSimpleVoxelObjectByPoint()).
        enum LevelSearchSimpleVoxelObject
        {
            AUTOLEVEL, //!< Автовыбор уровня (сейчас - как LASTLEVEL).
            LASTLEVEL, //!< До самого конца: до вокселя, который не чанк, или до пустой ячейки.
            NUMBERLEVEL //!< До уровня, указанного отдельно (уровень = число векторов в адресе: 1 - чанк, 2 - воксель чанка...).
        };
        //! Типы событий для регистрации движком.
        enum EventType
        {
            EVENT_TYPE_ERROR, //!< Ошибка.
            EVENT_TYPE_STANDARD, //!< Стандартное сообщение.
            EVENT_TYPE_LOGIC_VIOLATION, //!< Логическая ошибка пользователя.
            EVENT_TYPE_WARNING, //!< Предупреждение.
            EVENT_TYPE_INFO //!< Информационное сообщение.
        };
        //! Типы кэшей для поиска SimpleVoxelObject в чанке.
        enum WHERE_SEARCH_IN_MEMORY
        {
            ALL_SIMPLE_VOXEL_OBJECTS, //!< Искать среди всех SimpleVoxelObject.
            VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT, //!< Искать только среди видимых SimpleVoxelObject.
            VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT //!< Искать среди видимых с указанным кэшем видимости граней SimpleVoxelObjects.
        };
        //! Сторона границы мира (см. World::setChunksBound()).
        enum WORLD_BOUND_SIDE
        {
            WORLD_BOUND_X_MIN, //!< Наименьшая координата X чанка.
            WORLD_BOUND_X_MAX, //!< Наибольшая координата X чанка.
            WORLD_BOUND_Y_MIN, //!< Наименьшая координата Y чанка (низ мира).
            WORLD_BOUND_Y_MAX, //!< Наибольшая координата Y чанка (верх мира).
            WORLD_BOUND_Z_MIN, //!< Наименьшая координата Z чанка.
            WORLD_BOUND_Z_MAX //!< Наибольшая координата Z чанка.
        };
        //! Ось мира (см. World::setChunksBounds()).
        enum WORLD_AXIS
        {
            WORLD_AXIS_X, //!< Ось X.
            WORLD_AXIS_Y, //!< Ось Y (высота).
            WORLD_AXIS_Z //!< Ось Z.
        };
        //! Строковые константы движка.
        namespace STRING_CONSTANTS
        {
            const std::string DEFAULT_VECTOR_COORDINATES_SPLITTER = "."; //!< Стандартный разделитель для координат вектора.
            const std::string DEFAULT_FOLDER_SPLITTER = "/"; //!< Стандартный разделитель папок.
        };
        const float EPS = 0.001; //!< Эпсилон-значение для правильного сравнения дробных величин.
    };
}

#endif
