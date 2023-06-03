#ifndef SimpleVoxelEngine__ARGBColorVoxel_H
#define SimpleVoxelEngine__ARGBColorVoxel_H

#include "SimpleVoxelObject.h"
#include "Additional/ARGBColor.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! Структура, которая содержит информацию о вокселе типа ARGB.
        /*! Данная структура будет дана функции генерации вокселя для
        его рендеринга.
        */
        struct VoxelInfoARGBColor : VoxelInfo
        {
            SimpleVoxelEngine::Additional::ARGBColor s_ARGBColor; //!< Параметр, содержащий значение цвета вокселя.
        };
    }
    namespace Voxels
    {
        //! Воксель, который содержит только информацию о своём цвете ARGB типа.
        /*! Этот воксель удобно использовать, если вы решили составить мир
        из маленьких кубиков/вокселей.
        */
        class ARGBColorVoxel : public SimpleVoxelEngine::Voxels::SimpleVoxelObject
        {
            public:
                //! Стандартный конструктор.
                ARGBColorVoxel();
                //! Конструктор копирования.
                ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor& argbcolor);
                //! Конструктор с заданием цвета.
                /*! \param argbcolor Задаваемый цвет.
                */
                ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor argbcolor);
                //! Стандартный деструктор.
                ~ARGBColorVoxel();
                //! Установка вокселю цвета.
                /*! \param argbcolor Устанавливаемый цвет.
                */
                void setARGBColor(SimpleVoxelEngine::Additional::ARGBColor argbcolor); //!< \param argbcolor Цвет, который устанавливают вокселю.
                //! Получить цвет вокселя.
                SimpleVoxelEngine::Additional::ARGBColor getARGBColor(); //!< \return Возвращает цвет вокселя.
                //! Функция для получения информации для рендеринга вокселя.
                SimpleVoxelEngine::Core::VoxelInfo* getVoxelForRenderingInfo(); //!< \return Возвращает указатель на VoxelInfoARGBColor.
            private:
                SimpleVoxelEngine::Additional::ARGBColor s_ARGBColor;
        };
    }
}

#endif
