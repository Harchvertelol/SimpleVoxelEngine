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
            //! � ������ ������ ������ ����, �� ����������������.
            /*! �� �������� ������������ ��� ���������������� ������ ���� "bool".
            */
            template<typename TYPE_Value> class ThreadSafeType
            {
                public:
                    //! ����������� �����������.
                    ThreadSafeType():
                        s_Value(false)
                    {
                        //...
                    }
                    //! ����������� � �������� ��������.
                    /*! \param value ���������� ��������.
                    */
                    ThreadSafeType(TYPE_Value value):
                        s_Value(value)
                    {
                        //...
                    }
                    //! ����������� ����������.
                    ~ThreadSafeType()
                    {
                        //...
                    }
                    //! ��������� ��������.
                    /*! \return ���������� �������� ����������.
                    */
                    inline bool getValue() const
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Value;
                    }
                    //! ��������� ��������.
                    /*! \param value ��������������� ��������.
                    */
                    inline void setValue(bool value)
                    {
                        s_Mutex.lock();
                        s_Value = value;
                        s_Mutex.unlock();
                    }
                    //! �������� ������������ ������� ������ �� ����.
                    inline ThreadSafeType<TYPE_Value>& operator=(const ThreadSafeType<TYPE_Value>& elem)
                    {
                        if(this != &elem)
                        {
                            setValue(elem.getValue());
                        }
                        return *this;
                    }
                    //! �������� ������������ ��������.
                    inline ThreadSafeType<TYPE_Value>& operator=(const TYPE_Value value)
                    {
                        setValue(value);
                        return *this;
                    }
                private:
                    TYPE_Value s_Value;
                    mutable std::mutex s_Mutex;
            };
            //! �������� ���������.
            template<class TYPE_Value> inline bool operator!(const ThreadSafeType<TYPE_Value>& elem)
            {
                return !(elem.getValue());
            }
            //! �������� ��������� � ������ ����� �� �����.
            template<class TYPE_Value> inline bool operator==(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() == elem2.getValue();
            }
            //! �������� ��������� �� ���������.
            template<class TYPE_Value> inline bool operator==(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() == value;
            }
            //! �������� ��������� "�� �����" � ������ ����� �� �����.
            template<class TYPE_Value> inline bool operator!=(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() != elem2.getValue();
            }
            //! �������� ��������� "�� �����" � ������ ���������.
            template<class TYPE_Value> inline bool operator!=(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() != value;
            }
            //! �������� "���������� �" � ������ ����� �� �����.
            template<class TYPE_Value> inline bool operator&&(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() && elem2.getValue();
            }
            //! �������� "���������� ���" � ������ ����� �� �����.
            template<class TYPE_Value> inline bool operator||(const ThreadSafeType<TYPE_Value>& elem1, const ThreadSafeType<TYPE_Value>& elem2)
            {
                return elem1.getValue() || elem2.getValue();
            }
            //! �������� "���������� �" � ������ ���������.
            template<class TYPE_Value> inline bool operator&&(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() && value;
            }
            //! �������� "���������� ���" � ������ ���������.
            template<class TYPE_Value> inline bool operator||(const ThreadSafeType<TYPE_Value>& elem, const TYPE_Value value)
            {
                return elem.getValue() || value;
            }
            //! �������� "���������� �" � ������ ��������� �����.
            template<class TYPE_Value> inline bool operator&&(const TYPE_Value value, const ThreadSafeType<TYPE_Value>& elem)
            {
                return elem.getValue() && value;
            }
            //! �������� "���������� ���" � ������ ��������� �����.
            template<class TYPE_Value> inline bool operator||(const TYPE_Value value, const ThreadSafeType<TYPE_Value>& elem)
            {
                return elem.getValue() || value;
            }
        }
    }
}

#endif
