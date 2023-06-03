#ifndef SimpleVoxelEngine__VoxelsForRendering_H
#define SimpleVoxelEngine__VoxelsForRendering_H

#include <map>

#include "VoxelForRendering.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! Список вокселей для отрисовки.
        /*! Хранит список VoxelForRendering, которые необходимо отрисовать.
        */
        class VoxelsForRendering
        {
            public:
                //! Стандартный конструктор.
                VoxelsForRendering();
                //! Стандартный деструктор.
                ~VoxelsForRendering();
                //! Добавление VoxelForRendering в список.
                /*! \param vfr Добавляемый воксель для отрисовки.
                */
                void addVoxelForRendering(VoxelForRendering* vfr);
                //! Получение количества вокселей для отрисовки.
                /*! \return Возвращает количество вокселей в списке для отрисовки.
                */
                int getNumberOfVoxelsForRendering();
                //! Получение вокселя для отрисовки.
                /*! \param ind Индекс получаемого вокселя для отрисовки.
                \return Возвращает указатель на VoxelForRendering.
                */
                VoxelForRendering* getVoxelForRendering(int ind);
            private:
                std::map< int, VoxelForRendering* > s_VFR;
                int s_NumberOfVoxelsForRendering;
        };
    }
}

#endif
