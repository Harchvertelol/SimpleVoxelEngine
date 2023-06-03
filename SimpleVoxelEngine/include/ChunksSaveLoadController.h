#ifndef SimpleVoxelEngine__ChunksSaveLoadController_H
#define SimpleVoxelEngine__ChunksSaveLoadController_H

#include "ChunkVoxelObject.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class World;
        //! Контроллер сохранения и загрузки чанков.
        /*! Необходимо наследоваться от данного класса. Далее описать
        свои функции сохранения и загрузки чанков.
        */
        class ChunksSaveLoadController
        {
            friend World;
            public:
                //! Стандартный конструктор.
                ChunksSaveLoadController();
                //! Стандартный деструктор. Создан виртуальным для наследования.
                virtual ~ChunksSaveLoadController();
                //! Функция сохранения чанка.
                /*! Данный функционал будет полностью пересмотрен и изменён в будуших версиях движка.
                \param cvo Чанк для сохранения.
                \param vect Вектор (координаты) чанка в воксельном пространстве.
                \return Возвращает true в случае удачного сохранения, в противном случае возвращает false.
                */
                virtual bool saveChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
                //! Функция загрузки чанка.
                /*! Данный функционал будет полностью пересмотрен и изменён в будуших версиях движка.
                \param cvo Чанк для загрузки.
                \param vect Вектор (координаты) чанка в воксельном пространстве.
                \return Возвращает true в случае удачной загрузки, в противном случае возвращает false.
                */
                virtual bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
                //! Получения указателя на мир, к которому приписан данный контроллер.
                /*! \return Возвращает указатель на World (воксельный мир), который использует данный контроллер.
                */
                SimpleVoxelEngine::Core::World* getWorld();
                //! Проверка возможности загрузки чанка.
                /*! \param wrld Указатель на мир, в котором лежит чанк, для которого проверяется возможность загрузки.
                \param vect Вектор (координаты) проверяемого чанка на возможность загрузки.
                \return Возвращает true, если возможно, иначе false.
                */
                virtual bool canLoad(SimpleVoxelEngine::Core::World* wrld, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
            private:
                void setWorld(SimpleVoxelEngine::Core::World*);
                SimpleVoxelEngine::Core::World* s_World;
        };
    }
}

#endif
