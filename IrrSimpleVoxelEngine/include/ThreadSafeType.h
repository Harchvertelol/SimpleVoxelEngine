#ifndef ThreadSafeType_H
#define ThreadSafeType_H

#include <mutex>
#include <vector>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace ThreadSafeTypes
        {
            //! В теории аналог любого типа, но потокобезопасный.
            /*! На практике используется как потокобезопасный аналог типа "bool".
            */
            template<typename TYPE_Value> class ThreadSafeType
            {
                public:
                    //! Стандартный конструктор.
                    ThreadSafeType():
                        s_Value(false)
                    {
                        //...
                    }
                    //! Конструктор с заданием значения.
                    /*! \param value Задаваемое значение.
                    */
                    ThreadSafeType(TYPE_Value value):
                        s_Value(value)
                    {
                        //...
                    }
                    //! Стандартный деструктор.
                    ~ThreadSafeType()
                    {
                        //...
                    }
                    //! Получение значения.
                    /*! \return Возвращает значение переменной.
                    */
                    inline bool getValue() const
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Value;
                    }
                    //! Установка значения.
                    /*! \param value Устанавливаемое значение.
                    */
                    inline void setValue(bool value)
                    {
                        s_Mutex.lock();
                        s_Value = value;
                        s_Mutex.unlock();
                    }
                    //! Оператор присваивания другому такому же типу.
                    inline ThreadSafeType<TYPE_Value>& operator=(const ThreadSafeType<TYPE_Value>& elem)
                    {
                        if(this != &elem)
                        {
                            setValue(elem.getValue());
                        }
                        return *this;
                    }
                    //! Оператор присваивания значению.
                    inline ThreadSafeType<TYPE_Value>& operator=(const TYPE_Value value)
                    {
                        setValue(value);
                        return *this;
                    }
                private:
                    TYPE_Value s_Value;
                    mutable std::mutex s_Mutex;
            };
            //! Оператор отрицания.
            template<class TYPE_Value> inline bool operator!(const ThreadSafeType<TYPE_Value>& elem)
            {
                return !(elem.getValue());
            }
            //! Оператор сравнения с другим таким же типом.
            template<class TYPE_Value> inline bool operator==(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() == elem2.getValue();
            }
            //! Оператор сравнения со значением.
            template<class TYPE_Value> inline bool operator==(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() == value;
            }
            //! Оператор сравнения "не равно" с другим таким же типом.
            template<class TYPE_Value> inline bool operator!=(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() != elem2.getValue();
            }
            //! Оператор сравнения "не равно" с другим значением.
            template<class TYPE_Value> inline bool operator!=(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() != value;
            }
            //! Оператор "логическое И" с другим таким же типом.
            template<class TYPE_Value> inline bool operator&&(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() && elem2.getValue();
            }
            //! Оператор "логическое ИЛИ" с другим таким же типом.
            template<class TYPE_Value> inline bool operator||(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() || elem2.getValue();
            }
            //! Оператор "логическое И" с другим значением.
            template<class TYPE_Value> inline bool operator&&(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() && value;
            }
            //! Оператор "логическое ИЛИ" с другим значением.
            template<class TYPE_Value> inline bool operator||(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() || value;
            }
            //! Оператор "логическое И" с другим значением слева.
            template<class TYPE_Value> inline bool operator&&(const TYPE_Value value, const ThreadSafeType<TYPE_Value>& elem)
            {
                return elem.getValue() && value;
            }
            //! Оператор "логическое ИЛИ" с другим значением слева.
            template<class TYPE_Value> inline bool operator||(const TYPE_Value value, const ThreadSafeType<TYPE_Value>& elem)
            {
                return elem.getValue() || value;
            }
        }
    }
}

#endif
