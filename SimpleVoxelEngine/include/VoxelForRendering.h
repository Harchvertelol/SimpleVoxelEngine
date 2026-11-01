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
                /*! Позиция и поворот (0,0,0), масштаб (1,1,1), видны все грани.
                */
                VoxelForRendering();
                //! Конструктор копирования запрещён: объект владеет VoxelInfo, и копия удалила бы его второй раз.
                VoxelForRendering(const VoxelForRendering&) = delete;
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
                /*! Объект забирает vxlinfo и удалит его сам (прежний VoxelInfo удаляется сразу).
                \param vxlinfo Указатель на устанавливаемое VoxelInfo
                */
                void setVoxelInfo(VoxelInfo* vxlinfo);
                //! Проверка на необходимость отрисовки грани.
                /*! \param numb Номер грани, которая проверяется для отрисовки. Порядок граней: (x+, z+, x-, z-, y+, y-) - грани в
                направлениях (1,0,0), (0,0,1), (-1,0,0), (0,0,-1), (0,1,0), (0,-1,0), как в World::isVisible(); x+ - грань с
                наибольшим X, x- - с наименьшим, с остальными осями аналогично.
                \return Возвращает true, если грань необходимо отрисовать, иначе false. Так же false возвращается в случае numb < 0 или numb > 5.
                */
                bool isRenderFace(int numb) const;
                //! Установка грани для отрисовки.
                /*! \param numb Номер грани в массиве (порядок - см. isRenderFace()).
                \param value Если имеет значение true, то грань рисуется, иначе нет.
                */
                void setRenderFace(int numb, bool value);
                //! Установка массива отрисовки граней
                /*! Копирует массив faces в свой кэш. Поэтому после установки необходимо самому удалить массив faces в случае ненадобности.
                \param faces Копируемый массив из 6 элементов (порядок - см. isRenderFace()).
                */
                void setRenderFaces(bool* faces);
                //! Оператор присваивания запрещён по той же причине, что и копирование.
                VoxelForRendering& operator=(const VoxelForRendering&) = delete;
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
