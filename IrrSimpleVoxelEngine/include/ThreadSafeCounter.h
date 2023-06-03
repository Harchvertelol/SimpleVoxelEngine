#ifndef ThreadSafeCounter_H
#define ThreadSafeCounter_H

#include <mutex>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace ThreadSafeTypes
        {
            //! Потокобезопасный счётчик.
            /*! Этот класс предоставляет потокобезопасный счётчик.
            */
            template<typename TYPE_CounterValue> class ThreadSafeCounter
            {
                public:
                    //! Стандартный конструктор.
                    ThreadSafeCounter():
                        s_Value(0)
                    {
                        //...
                    }
                    //! Стандартный деструктор.
                    ~ThreadSafeCounter()
                    {
                        //...
                    }
                    //! Конструктор копирования.
                    ThreadSafeCounter(const ThreadSafeCounter<TYPE_CounterValue>& elem):
                        s_Value(elem.getValue())
                    {
                        //...
                    }
                    //! Конструктор по заданному значению.
                    /*! \param value Задаваемое значение.
                    */
                    ThreadSafeCounter(const TYPE_CounterValue value):
                        s_Value(value)
                    {
                        //...
                    }
                    //! Префиксное увеличение значения счётчика.
                    /*! \param value Значение, на которое увеличиваем.
                    \return Возвращает значение счётчика, которое было до увеличения.
                    */
                    inline TYPE_CounterValue increasePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_add( &s_Value, value );
                        return retvalue;
                    }
                    //! Префиксное уменьшение значения счётчика.
                    /*! \param value Значение, на которое уменьшаем.
                    \return Возвращает значение счётчика, которое было до уменьшения.
                    */
                    inline TYPE_CounterValue reducePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_add( &s_Value, -value );
                        return retvalue;
                    }
                    //! Постфиксное увеличение значения счётчика.
                    /*! \param value Значение, на которое увеличиваем.
                    \return Возвращает значение счётчика, которое стало после увеличения.
                    */
                    inline TYPE_CounterValue increasePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_add_and_fetch( &s_Value, value );
                        return retvalue;
                    }
                    //! Постфиксное уменьшение значения счётчика.
                    /*! \param value Значение, на которое уменьшаем.
                    \return Возвращает значение счётчика, которое стало после уменьшения.
                    */
                    inline TYPE_CounterValue reducePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_add_and_fetch( &s_Value, -value );
                        return retvalue;
                    }
                    //! Получение значения счётчика.
                    /*! \return Возвращает текущее значение счётчика.
                    */
                    inline TYPE_CounterValue getValue() const
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Value;
                    }
                    //! Постфиксное умножение.
                    /*! \param value Значение, на которое умножаем.
                    \return Возвращает значение счётчика, которое стало после умножения.
                    */
                    inline TYPE_CounterValue subValuePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_sub_and_fetch( &s_Value, value );
                        return retvalue;
                    }
                    //! Префиксное умножение.
                    /*! \param value Значение, на которое умножаем.
                    \return Возвращает значение счётчика, которое было до умножения.
                    */
                    inline TYPE_CounterValue subValuePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_sub( &s_Value, value );
                        return retvalue;
                    }
                    //! Проверка значения счётчика на значение "ноль".
                    /*! \return Возвращает true, если счётчик имеет значение NULL, иначе false.
                    */
                    inline bool isNull() const
                    {
                        return (getValue() == 0);
                    }
                    //! Оператор присваивания другому счётчику.
                    ThreadSafeCounter<TYPE_CounterValue>& operator=(const ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        if(this != &elem)
                        {
                            s_Mutex.lock();
                            s_Value = elem.getValue();
                            s_Mutex.unlock();
                        }
                        return *this;
                    }
                    //! Оператор присваивания другому значению.
                    ThreadSafeCounter<TYPE_CounterValue>& operator=(const TYPE_CounterValue value)
                    {
                        s_Mutex.lock();
                        s_Value = value;
                        s_Mutex.unlock();
                        return *this;
                    }
                    //! Оператор сравнения "меньше" с другим счётчиком.
                    bool operator<(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        return getValue() < elem.getValue();
                    }
                    //! Оператор сравнения "больше" с другим счётчиком.
                    bool operator>(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        return getValue() > elem.getValue();
                    }
                    //! Оператор сравнения "меньше или равно" с другим счётчиком.
                    bool operator<=(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        bool test = getValue() < elem.getValue();
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == elem.getValue());
                        return retvalue;
                    }
                    //! Оператор сравнения "больше или равно" с другим счётчиком.
                    bool operator>=(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        bool test = getValue() > elem.getValue();
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == elem.getValue());
                        return retvalue;
                    }
                    //! Оператор сравнения "меньше" с другим значением.
                    bool operator<(const TYPE_CounterValue value) const
                    {
                        return getValue() < value;
                    }
                    //! Оператор сравнения "больше" с другим значением.
                    bool operator>(const TYPE_CounterValue value) const
                    {
                        return getValue() > value;
                    }
                    //! Оператор сравнения "меньше или равно" с другим значением.
                    bool operator<=(const TYPE_CounterValue value) const
                    {
                        bool test = getValue() < value;
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == value);
                        return retvalue;
                    }
                    //! Оператор сравнения "больше или равно" с другим значением.
                    bool operator>=(const TYPE_CounterValue value) const
                    {
                        bool test = getValue() > value;
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == value);
                        return retvalue;
                    }
                    //! Оператор увеличения счётчика на значение другого счётчика.
                    ThreadSafeCounter<TYPE_CounterValue>& operator+=(const ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        increasePostfix(elem.getValue());
                        return *this;
                    }
                    //! Оператор уменьшения счётчика на значение другого счётчика.
                    ThreadSafeCounter<TYPE_CounterValue>& operator-=(const ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        reducePostfix(elem.getValue());
                        return *this;
                    }
                    //! Оператор сложения счётчика с другим счётчиком.
                    ThreadSafeCounter<TYPE_CounterValue> operator+(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(*this);
                        return newcounter += elem;
                    }
                    //! Оператор вычитания счётчика из другого счётчика.
                    ThreadSafeCounter<TYPE_CounterValue> operator-(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(*this);
                        return newcounter -= elem;
                    }
                    //! Префиксный оператор инкремента.
                    friend const ThreadSafeCounter<TYPE_CounterValue> operator++(ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(elem.increasePrefix(1));
                        return newcounter;
                    }
                    //! Постфиксный оператор инкремента.
                    friend const ThreadSafeCounter<TYPE_CounterValue>& operator++(ThreadSafeCounter<TYPE_CounterValue>& elem, int)
                    {
                        elem.increasePostfix(1);
                        return elem;
                    }
                    //! Префиксный оператор декремента.
                    friend const ThreadSafeCounter<TYPE_CounterValue> operator--(ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(elem.reducePrefix(1));
                        return newcounter;
                    }
                    //! Постфиксный оператор декремента.
                    friend const ThreadSafeCounter<TYPE_CounterValue>& operator--(ThreadSafeCounter<TYPE_CounterValue>& elem, int)
                    {
                        elem.reducePostfix(1);
                        return elem;
                    }
                private:
                    mutable std::mutex s_Mutex;
                    TYPE_CounterValue s_Value;
            };
            //! Оператор сравнения "равно" двух счётчиков для возможности использования счётчика как индексатора std::map.
            template<class TYPE_CounterValue> bool operator==(const ThreadSafeCounter<TYPE_CounterValue>& elem1, const ThreadSafeCounter<TYPE_CounterValue>& elem2)
            {
                return elem1.getValue() == elem2.getValue();
            }
            //! Оператор сравнения "не равно" двух счётчиков для возможности использования счётчика как индексатора std::map.
            template<class TYPE_CounterValue> bool operator!=(const ThreadSafeCounter<TYPE_CounterValue>& elem1, const ThreadSafeCounter<TYPE_CounterValue>& elem2)
            {
                return !(elem1 == elem2);
            }
            //! Оператор сравнения "равно" счётчика со значением.
            template<class TYPE_CounterValue> bool operator==(const ThreadSafeCounter<TYPE_CounterValue>& elem, const TYPE_CounterValue value)
            {
                return elem.getValue() == value;
            }
            //! Оператор сравнения "не равно" счётчика со значением.
            template<class TYPE_CounterValue> bool operator!=(const ThreadSafeCounter<TYPE_CounterValue>& elem, const TYPE_CounterValue value)
            {
                return !(elem == value);
            }
            //! Оператор сравнения "меньше" счётчика и значения слева.
            template<class TYPE_CounterValue> bool operator<(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value < elem.getValue();
            }
            //! Оператор сравнения "больше" счётчика и значения слева.
            template<class TYPE_CounterValue> bool operator>(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value > elem.getValue();
            }
            //! Оператор сравнения "меньше или равно" счётчика и значения слева.
            template<class TYPE_CounterValue> bool operator<=(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value <= elem.getValue();
            }
            //! Оператор сравнения "больше или равно" счётчика и значения слева.
            template<class TYPE_CounterValue> bool operator>=(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value >= elem.getValue();
            }
            //! Оператор вычитания счётчика из значения.
            template<class TYPE_CounterValue> TYPE_CounterValue operator-(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value - elem.getValue();
            }
            //! Оператор сложения счётчика со значением слева.
            template<class TYPE_CounterValue> TYPE_CounterValue operator+(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value + elem.getValue();
            }
        }
    }
}

#endif
