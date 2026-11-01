#ifndef SimpleVoxelEngine__SimpleVoxelObject_H
#define SimpleVoxelEngine__SimpleVoxelObject_H

#include <fstream>

#include "Defines.h"
#include "VoxelAddress.h"
#include "VoxelInfo.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        class World;
    }
    namespace Voxels
    {
        //! Класс-родитель всех воксельных объектов.
        /*! Любой воксель, чанк, что ещё либо придуманное пользователем (воксель), должно наследоваться от данного класса.
        */
        class SimpleVoxelObject
        {
            public:
                //! Стандартный конструктор.
                SimpleVoxelObject();
                //! Стандартный деструктор. Создан виртуальным для наследования.
                virtual ~SimpleVoxelObject();
                //! Получение типа SimpleVoxelObject.
                /*! \return Возвращает тип SimpleVoxelObject.
                */
                SimpleVoxelEngine::Defines::TYPE_VOXEL getTypeVoxel();
                //! Установка типа SimpleVoxelObject.
                /*! \param tv Новый тип SimpleVoxelObject.
                */
                void setTypeVoxel(SimpleVoxelEngine::Defines::TYPE_VOXEL tv);
                //! Получение информации для рендеринга.
                /*! Данная функция вызывается при получении от SimpleVoxelObject информации, которая будет отдана пользователю для отрисовки.
                Переопределите её в своём вокселе: нужно возвращать НОВЫЙ объект (new VoxelInfo или наследника) - его заберёт
                VoxelForRendering и удалит сам. Базовая реализация возвращает NULL (информации нет).
                \return Возвращает указатель на новый VoxelInfo (или NULL).
                */
                virtual SimpleVoxelEngine::Core::VoxelInfo* getVoxelForRenderingInfo();
            private:
                SimpleVoxelEngine::Defines::TYPE_VOXEL s_TypeVoxel;
        };
    }
}

#endif
