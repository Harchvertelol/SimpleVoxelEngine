#ifndef SimpleVoxelEngine__ReferenceCounter_H
#define SimpleVoxelEngine__ReferenceCounter_H

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! Класс, отвечающий за счётчик ссылок на объект.
        /*! Данный класс автоматически производит подсчёт ссылок, оставшихся
        на объект, и удаляет его в случае, если ссылок стало равно нулю.
        */
        class ReferenceCounter
        {
            public:
                //! Стандартный конструктор. При создании объекта счётчик ссылок становится равен единице.
                ReferenceCounter();
                //! Стандартный деструктор.
                virtual ~ReferenceCounter();
                //! Увеличение числа ссылок на объект на единицу.
                /*! Увеличивает число ссылок на объект на единицу.
                */
                void grab() const;
                //! Уменьшение числа ссылок на объект на единицу.
                /*! Уменьшает число ссылок на объект на единицу. Если число ссылок стало равно нулю, то
                удаляет объект.
                \return Возвращает true, если объект был удалён.
                */
                bool drop() const;
                //! Получение числа ссылок на объект.
                /*! \return Возвращает число ссылок на объект.
                */
                int getReferenceCount() const;
            private:
                mutable int s_ReferenceCounter;
        };
    };

};

#endif
