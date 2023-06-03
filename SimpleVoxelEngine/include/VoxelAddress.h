#ifndef SimpleVoxelEngine__VoxelAddress_H
#define SimpleVoxelEngine__VoxelAddress_H

#include <map>

#include "Additional/Vector3d.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! Адрес вокселя.
        /*! Данный класс хранит последовательность векторов, которые являются адресом.
        Последний вектор является вектором в последнем чанке. Предпоследний - вектором
        последнего чанка в предпоследнем чанке. И так далее.
        Например, если у нас простейший мир, где только чанки и кубики, размер чанка 16 на 16 на 16,
        то адрес кубика с глобальными мировыми координатами (0, 0, 0) будет следующим:
        (0, 0, 0) -> (0, 0, 0), а кубика с глобальными координатами (16, 0, 0) следующим:
        (1, 0, 0) -> (0, 0, 0).
        */
        class VoxelAddress
        {
            public:
                //! Стандартный конструктор.
                /*! \param eng Движок, в котором мир, в котором действителен этот воксельный адрес.
                */
                VoxelAddress(Engine* eng);
                //! Конструктор копирования
                VoxelAddress(VoxelAddress&);
                //! Стандартный деструктор.
                ~VoxelAddress();
                //! Устаревшая функция.
                void setLevelShift(int shift, bool up = true);
                //! Устаревшая функция.
                int getLevelShift() const;
                //! Устаревшая функция.
                bool isUp() const;
                //! Добавление вектора в адрес в конец списка.
                /*! \param vect Добавляемый вектор.
                */
                void addNextVector(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Добавление вектора в адрес по индексу.
                /*! \param ind Индекс, куда идёт добавление.
                \param vect Добавляемый вектор.
                \return Возвращает true в случае успеха, иначе false. Нельзя добавить вектор на несуществующий индекс (только addNextVector()).
                */
                bool addVector(int ind, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Получение последнего индекса.
                /*! \return Возвращает индекс после последнего, т.е. пустой, последний вектор имеет индекс getLastVectInd() - 1.
                */
                int getLastVectInd() const;
                //! Получение последнего вектора в списке адреса.
                /*! \return Получает последний вектор в списке адреса, фактически вектор (координаты) в последнем чанке.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getLastVector();
                //! Получение вектора по индексу.
                /*! \param ind Индекс получаемого вектора.
                \return Возвращает вектор с индексом ind из адреса. В случае несуществования такого возвращает нулевой вектор.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getVector(int ind);
                //! Получение map с адресом.
                /*! \return Возвращает map с адресом.
                */
                std::map< int, SimpleVoxelEngine::Additional::Vector3d<int> > getAddress() const;
                //! Удаление последнего вектора.
                /*! \return Возвращает true в случае успеха, иначе false. Неудача может быть в случае, если в адресе больше не осталось векторов.
                */
                bool deleteLastVector();
                //! Оператор присваивания.
                VoxelAddress& operator=(VoxelAddress&);
                //! Получение движка.
                /*! \return Возвращает указатель на движок, в котором зарегистрирован мир, в котором этот адрес является действительным.
                */
                Engine* getEngine() const;
            private:
                Engine* s_Engine;
                std::map< int, SimpleVoxelEngine::Additional::Vector3d<int> > s_Address;
                int s_LevelShift;
                bool s_Up;
                int s_LastVectInd;
        };
    }
}

#endif
