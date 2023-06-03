#ifndef SimpleVoxelEngine__VoxelForRendering_H
#define SimpleVoxelEngine__VoxelForRendering_H

#include <map>

#include "Additional/Vector3d.h"

#include "VoxelInfo.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! Информация для отрисовки вокселя.
        /*! В данном классе содержится полная информация для отрисовки вокселя.
        Есть возможность задания своих параметров.
        */
        class VoxelForRendering
        {
            public:
                //! Стандартный конструктор.
                VoxelForRendering();
                //! Конструктор копирования.
                VoxelForRendering(const VoxelForRendering&);
                //! Конструктор с параметрами.
                /*! \param pos Вектор позиции вокселя в глобальных мировых координатах.
                \param rot Вектор поворота вокселя в глобальных мировых координатах.
                \param scale Вектор масштабирования вокселя в глобальных мировых координатах.
                */
                VoxelForRendering(SimpleVoxelEngine::Additional::Vector3d<float> pos, SimpleVoxelEngine::Additional::Vector3d<float> rot, SimpleVoxelEngine::Additional::Vector3d<float> scale);
                //! Стандартный деструктор.
                ~VoxelForRendering();
                //! Получение позиции вокселя в глобальных мировых координатах.
                /*! \return Возвращает вектор положения вокселя в глобальных мировых координатах.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getPosition() const;
                //! Получение поворота вокселя в глобальных мировых координатах.
                /*! \return Возвращает вектор поворота вокселя в глобальных мировых координатах.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getRotation() const;
                //! Получение масштабирования вокселя в глобальных мировых координатах.
                /*! \return Возвращает вектор масштабирования вокселя в глобальных мировых координатах.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getScale() const;
                //! Установка позиции вокселя в глобальных мировых координатах.
                /*! \param vect Вектор позиции вокселя в глобальных мировых координатах.
                */
                void setPosition(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! Установка поворота вокселя в глобальных мировых координатах.
                /*! \param vect Вектор поворота вокселя в глобальных мировых координатах.
                */
                void setRotation(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! Установка масштабирования вокселя в глобальных мировых координатах.
                /*! \param vect Вектор масштабирования вокселя в глобальных мировых координатах.
                */
                void setScale(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! Получнеие информации о вокселе (VoxelInfo)
                /*! \return Возвращает указатель на VoxelInfo.
                */
                VoxelInfo* getVoxelInfo() const;
                //! Установка VoxelInfo.
                /*! \param vxlinfo Указатель на устанавливаемое VoxelInfo
                */
                void setVoxelInfo(VoxelInfo* vxlinfo);
                //! Проверка на необходимость отрисовки грани.
                /*! \param numb Номер грани, которая проверяется для отрисовки. Массив имеет следующий порядок: (x2, x1, y2, y1, z2, z1), x2 - самое большое по X положение грани, перпендикулярной X, x1 - самое маленькое по X положение грани, перпендикулярной X. С остальными осями аналогично.
                \return Возвращает true, если грань необходимо отрисовать, иначе false. Так же false возвращается в случае numb > 5.
                */
                bool isRenderFace(int numb) const;
                //! Установка грани для отрисовки.
                /*! \param numb Номер грани в массиве.
                \param value Если имеет значение true, то грань рисуется, иначе нет.
                */
                void setRenderFace(int numb, bool value);
                //! Установка массива отрисовки граней
                /*! Копирует массив faces в свой кэш. Поэтому после установки необходимо самому удалить массив faces в случае ненадобности.
                \param faces Копируемый массив.
                */
                void setRenderFaces(bool* faces);
                //! Оператор присваивания.
                VoxelForRendering& operator=(VoxelForRendering&);
            private:
                SimpleVoxelEngine::Additional::Vector3d<float> s_Position;
                SimpleVoxelEngine::Additional::Vector3d<float> s_Rotation;
                SimpleVoxelEngine::Additional::Vector3d<float> s_Scale;
                bool s_Faces[6];
                VoxelInfo* s_VoxelInfo;
        };
    }
}

#endif
