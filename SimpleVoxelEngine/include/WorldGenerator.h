#ifndef SimpleVoxelEngine__WorldGenerator_H
#define SimpleVoxelEngine__WorldGenerator_H

#include "ChunkVoxelObject.h"
#include "Additional/Vector3d.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class World;
        //! Генератор мира.
        /*! Данный класс используется для генерации воксельного мира. Описывается пользователем.
        */
        class WorldGenerator
        {
            public:
                //! Стандартный конструктор.
                WorldGenerator();
                //! Стандартный деструктор.
                virtual ~WorldGenerator();
                //! Генерация чанка.
                /*! \param chunk Указатель на генерируемый чанк.
                \param sizechunk Размер ребра чанка. Его можно получить, взяв из chunk указатель нам World, но sizechunk передаётся для лучшей возможности реализации многопоточности.
                \return Возвращает true в случае успеха, иначе false.
                */
                virtual bool generateChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk, int sizechunk) = 0;
            private:
                //...
        };
    }
}

#endif
