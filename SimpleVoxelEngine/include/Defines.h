#ifndef SimpleVoxelEngine__Defines_H
#define SimpleVoxelEngine__Defines_H

#include <string>

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
        //! Уровень поиска SimpleVoxelObject. Данная опция в данный момент недоступна.
        enum LevelSearchSimpleVoxelObject
        {
            AUTOLEVEL, //!< Автовыбор уровня.
            LASTLEVEL, //!< До самого конца.
            NUMBERLEVEL //!< Уровень отдельно указан.
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
