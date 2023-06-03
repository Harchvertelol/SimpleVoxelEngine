#ifndef SimpleVoxelEngine__VoxelInfo_H
#define SimpleVoxelEngine__VoxelInfo_H

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! Типы информации для вокселей.
        enum TYPE_VOXEL_INFO
        {
            ARGBCOLOR_VOXEL_INFO, //!< Информация для ARGBColorVoxel.
            CHUNK_VOXEL_OBJECT_VOXEL_INFO, //!< Информация для чанка.
            CUSTOM_VOXEL_INFO //!< Пользовательская информация.
        };
        //! Структура хранения информации о вокселе для отрисовки. Для введения своей информации необходимо наследоваться от него и установить тип CUSTOM_VOXEL_INFO.
        struct VoxelInfo
        {
            TYPE_VOXEL_INFO s_TypeVoxelInfo; //!< Тип информации для отрисовки вокселя.
        };
    }
}

#endif
