#ifndef ThreadSafeCounter_H
#define ThreadSafeCounter_H

#include <mutex>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace ThreadSafeTypes
        {
            //! ���������������� �������.
            /*! ���� ����� ������������� ���������������� �������.
            */
            template<typename TYPE_CounterValue> class ThreadSafeCounter
            {
                public:
                    //! ����������� �����������.
                    ThreadSafeCounter():
                        s_Value(0)
                    {
                        //...
                    }
                    //! ����������� ����������.
                    ~ThreadSafeCounter()
                    {
                        //...
                    }
                    //! ����������� �����������.
                    ThreadSafeCounter(const ThreadSafeCounter<TYPE_CounterValue>& elem):
                        s_Value(elem.getValue())
                    {
                        //...
                    }
                    //! ����������� �� ��������� ��������.
                    /*! \param value ���������� ��������.
                    */
                    ThreadSafeCounter(const TYPE_CounterValue value):
                        s_Value(value)
                    {
                        //...
                    }
                    //! ���������� ���������� �������� ��������.
                    /*! \param value ��������, �� ������� �����������.
                    \return ���������� �������� ��������, ������� ���� �� ����������.
                    */
                    inline TYPE_CounterValue increasePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_add( &s_Value, value );
                        return retvalue;
                    }
                    //! ���������� ���������� �������� ��������.
                    /*! \param value ��������, �� ������� ���������.
                    \return ���������� �������� ��������, ������� ���� �� ����������.
                    */
                    inline TYPE_CounterValue reducePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_add( &s_Value, -value );
                        return retvalue;
                    }
                    //! ����������� ���������� �������� ��������.
                    /*! \param value ��������, �� ������� �����������.
                    \return ���������� �������� ��������, ������� ����� ����� ����������.
                    */
                    inline TYPE_CounterValue increasePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_add_and_fetch( &s_Value, value );
                        return retvalue;
                    }
                    //! ����������� ���������� �������� ��������.
                    /*! \param value ��������, �� ������� ���������.
                    \return ���������� �������� ��������, ������� ����� ����� ����������.
                    */
                    inline TYPE_CounterValue reducePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_add_and_fetch( &s_Value, -value );
                        return retvalue;
                    }
                    //! ��������� �������� ��������.
                    /*! \return ���������� ������� �������� ��������.
                    */
                    inline TYPE_CounterValue getValue() const
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Value;
                    }
                    //! ����������� ���������.
                    /*! \param value ��������, �� ������� ��������.
                    \return ���������� �������� ��������, ������� ����� ����� ���������.
                    */
                    inline TYPE_CounterValue subValuePostfix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_sub_and_fetch( &s_Value, value );
                        return retvalue;
                    }
                    //! ���������� ���������.
                    /*! \param value ��������, �� ������� ��������.
                    \return ���������� �������� ��������, ������� ���� �� ���������.
                    */
                    inline TYPE_CounterValue subValuePrefix(TYPE_CounterValue value)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        TYPE_CounterValue retvalue = __sync_fetch_and_sub( &s_Value, value );
                        return retvalue;
                    }
                    //! �������� �������� �������� �� �������� "����".
                    /*! \return ���������� true, ���� ������� ����� �������� NULL, ����� false.
                    */
                    inline bool isNull() const
                    {
                        return (getValue() == 0);
                    }
                    //! �������� ������������ ������� ��������.
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
                    //! �������� ������������ ������� ��������.
                    ThreadSafeCounter<TYPE_CounterValue>& operator=(const TYPE_CounterValue value)
                    {
                        s_Mutex.lock();
                        s_Value = value;
                        s_Mutex.unlock();
                        return *this;
                    }
                    //! �������� ��������� "������" � ������ ���������.
                    bool operator<(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        return getValue() < elem.getValue();
                    }
                    //! �������� ��������� "������" � ������ ���������.
                    bool operator>(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        return getValue() > elem.getValue();
                    }
                    //! �������� ��������� "������ ��� �����" � ������ ���������.
                    bool operator<=(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        bool test = getValue() < elem.getValue();
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == elem.getValue());
                        return retvalue;
                    }
                    //! �������� ��������� "������ ��� �����" � ������ ���������.
                    bool operator>=(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        bool test = getValue() > elem.getValue();
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == elem.getValue());
                        return retvalue;
                    }
                    //! �������� ��������� "������" � ������ ���������.
                    bool operator<(const TYPE_CounterValue value) const
                    {
                        return getValue() < value;
                    }
                    //! �������� ��������� "������" � ������ ���������.
                    bool operator>(const TYPE_CounterValue value) const
                    {
                        return getValue() > value;
                    }
                    //! �������� ��������� "������ ��� �����" � ������ ���������.
                    bool operator<=(const TYPE_CounterValue value) const
                    {
                        bool test = getValue() < value;
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == value);
                        return retvalue;
                    }
                    //! �������� ��������� "������ ��� �����" � ������ ���������.
                    bool operator>=(const TYPE_CounterValue value) const
                    {
                        bool test = getValue() > value;
                        bool retvalue = __sync_or_and_fetch(&test, getValue() == value);
                        return retvalue;
                    }
                    //! �������� ���������� �������� �� �������� ������� ��������.
                    ThreadSafeCounter<TYPE_CounterValue>& operator+=(const ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        increasePostfix(elem.getValue());
                        return *this;
                    }
                    //! �������� ���������� �������� �� �������� ������� ��������.
                    ThreadSafeCounter<TYPE_CounterValue>& operator-=(const ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        reducePostfix(elem.getValue());
                        return *this;
                    }
                    //! �������� �������� �������� � ������ ���������.
                    ThreadSafeCounter<TYPE_CounterValue> operator+(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(*this);
                        return newcounter += elem;
                    }
                    //! �������� ��������� �������� �� ������� ��������.
                    ThreadSafeCounter<TYPE_CounterValue> operator-(const ThreadSafeCounter<TYPE_CounterValue>& elem) const
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(*this);
                        return newcounter -= elem;
                    }
                    //! ���������� �������� ����������.
                    friend const ThreadSafeCounter<TYPE_CounterValue> operator++(ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(elem.increasePrefix(1));
                        return newcounter;
                    }
                    //! ����������� �������� ����������.
                    friend const ThreadSafeCounter<TYPE_CounterValue>& operator++(ThreadSafeCounter<TYPE_CounterValue>& elem, int)
                    {
                        elem.increasePostfix(1);
                        return elem;
                    }
                    //! ���������� �������� ����������.
                    friend const ThreadSafeCounter<TYPE_CounterValue> operator--(ThreadSafeCounter<TYPE_CounterValue>& elem)
                    {
                        ThreadSafeCounter<TYPE_CounterValue> newcounter(elem.reducePrefix(1));
                        return newcounter;
                    }
                    //! ����������� �������� ����������.
                    friend const ThreadSafeCounter<TYPE_CounterValue>& operator--(ThreadSafeCounter<TYPE_CounterValue>& elem, int)
                    {
                        elem.reducePostfix(1);
                        return elem;
                    }
                private:
                    mutable std::mutex s_Mutex;
                    TYPE_CounterValue s_Value;
            };
            //! �������� ��������� "�����" ���� ��������� ��� ����������� ������������� �������� ��� ����������� std::map.
            template<class TYPE_CounterValue> bool operator==(const ThreadSafeCounter<TYPE_CounterValue>& elem1, const ThreadSafeCounter<TYPE_CounterValue>& elem2)
            {
                return elem1.getValue() == elem2.getValue();
            }
            //! �������� ��������� "�� �����" ���� ��������� ��� ����������� ������������� �������� ��� ����������� std::map.
            template<class TYPE_CounterValue> bool operator!=(const ThreadSafeCounter<TYPE_CounterValue>& elem1, const ThreadSafeCounter<TYPE_CounterValue>& elem2)
            {
                return !(elem1 == elem2);
            }
            //! �������� ��������� "�����" �������� �� ���������.
            template<class TYPE_CounterValue> bool operator==(const ThreadSafeCounter<TYPE_CounterValue>& elem, const TYPE_CounterValue value)
            {
                return elem.getValue() == value;
            }
            //! �������� ��������� "�� �����" �������� �� ���������.
            template<class TYPE_CounterValue> bool operator!=(const ThreadSafeCounter<TYPE_CounterValue>& elem, const TYPE_CounterValue value)
            {
                return !(elem == value);
            }
            //! �������� ��������� "������" �������� � �������� �����.
            template<class TYPE_CounterValue> bool operator<(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value < elem.getValue();
            }
            //! �������� ��������� "������" �������� � �������� �����.
            template<class TYPE_CounterValue> bool operator>(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value > elem.getValue();
            }
            //! �������� ��������� "������ ��� �����" �������� � �������� �����.
            template<class TYPE_CounterValue> bool operator<=(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value <= elem.getValue();
            }
            //! �������� ��������� "������ ��� �����" �������� � �������� �����.
            template<class TYPE_CounterValue> bool operator>=(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value >= elem.getValue();
            }
            //! �������� ��������� �������� �� ��������.
            template<class TYPE_CounterValue> TYPE_CounterValue operator-(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value - elem.getValue();
            }
            //! �������� �������� �������� �� ��������� �����.
            template<class TYPE_CounterValue> TYPE_CounterValue operator+(const TYPE_CounterValue value, const ThreadSafeCounter<TYPE_CounterValue>& elem)
            {
                return value + elem.getValue();
            }
        }
    }
}

#endif
