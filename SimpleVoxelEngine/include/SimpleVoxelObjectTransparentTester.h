#ifndef SimpleVoxelEngine__SimpleVoxelObjectTransparentTester_H
#define SimpleVoxelEngine__SimpleVoxelObjectTransparentTester_H

#include "SimpleVoxelObject.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! Система проверки SimpleVoxelObject на прозрачность.
        /*! Несмотря на кажущуюся простоту, эта система даёт много возможностей.
        Например, создание граней только между определёнными типами вокселей.
        Вложенные чанки (ChunkVoxelObject) тестеру не передаются: движок сам проверяет ячейки вложенного чанка,
        которые касаются грани (World::isFaceTransparent()), и для каждого блока среди них вызывает тестер.
        Если parent - вложенный чанк, вместо него передаётся NULL.
        */
        class SimpleVoxelObjectTransparentTester
        {
            public:
                //! Стандартный конструктор.
                SimpleVoxelObjectTransparentTester();
                //! Стандартный деструктор. Создан виртуальным для наследования.
                virtual ~SimpleVoxelObjectTransparentTester();
                //! Функция проверки на прозрачность.
                /*! \param svo Какой объект проверяется на прозрачность (всегда блок, не чанк и не NULL).
                \param parent От какого объекта идёт запрос проверки на прозрачность. Может быть NULL (например, когда проверяется
                грань вложенного чанка) - проверяйте перед использованием.
                \return Возвращает true в случае прозрачности, иначе false.
                */
                virtual bool isTransparent(SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0) = 0;
            private:
                //...
        };
    }
}

#endif
