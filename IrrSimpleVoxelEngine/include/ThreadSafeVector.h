#ifndef ThreadSafeVector_H
#define ThreadSafeVector_H

#include <mutex>
#include <thread>
#include <vector>
#include <functional>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace ThreadSafeTypes
        {
            //! ������ ������ std::vector, �� � ���������� ������� � �������.
            /*! ������ ����� ��������� ��������������� ��������� ������ std::vector.
            */
            template<typename TYPE_Value> class ThreadSafeVector
            {
                public:
                    //! ����������� �����������.
                    ThreadSafeVector():
                        s_ReadKey(0), //! ���� ������ � ������� �������������� �������.
                        s_WriteKey(-1) //! ���� ������ � ������� �������������� �������.
                    {
                        //...
                    }
                    //! ����������� ����������.
                    ~ThreadSafeVector()
                    {
                        //...
                    }
                    //! ������� �������� �� ����� ������.
                    /*! \param value ����������� �������.
                    \param nextwritekey ���� ����� �������� true, �� ���� ������ ����� ������� �����, ����� �������� �������.
                    */
                    void insertBeforeWriteKey(TYPE_Value value, bool nextwritekey = false)
                    {
                        s_Mutex.lock();
                        if(s_WriteKey != -1)
                        {
                            s_Vector.insert(s_Vector.begin() + s_WriteKey - 1, value);
                            if(nextwritekey) s_WriteKey++;
                        }
                        s_Mutex.unlock();
                    }
                    //! ������� �������� ����� ����� ������.
                    /*! \param value ����������� �������.
                    \param nextwritekey ���� ����� �������� true, �� ���� ������ ����� ������� �����, ����� �������� �������.
                    */
                    void insertAfterWriteKey(TYPE_Value value, bool nextwritekey = false)
                    {
                        s_Mutex.lock();
                        if(s_WriteKey != -1)
                        {
                            s_Vector.insert(s_Vector.begin() + s_WriteKey, value);
                            if(nextwritekey) s_WriteKey++;
                        }
                        s_Mutex.unlock();
                    }
                    //! ������� �������� ����� ����� ������.
                    /*! \param value ����������� �������.
                    \param nextreadkey ���� ����� �������� true, �� ���� ������ ����� ������� �����, ����� �������� �������.
                    */
                    void insertBeforeReadKey(TYPE_Value value, bool nextreadkey = false)
                    {
                        s_Mutex.lock();
                        s_Vector.insert(s_Vector.begin() + s_ReadKey - 1, value);
                        if(nextreadkey) s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! ������� �������� �� ����� ������.
                    /*! \param value ����������� �������.
                    \param nextreadkey ���� ����� �������� true, �� ���� ������ ����� ������� �����, ����� �������� �������.
                    */
                    void insertAfterReadKey(TYPE_Value value, bool nextreadkey = false)
                    {
                        s_Mutex.lock();
                        s_Vector.insert(s_Vector.begin() + s_ReadKey, value);
                        if(nextreadkey) s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! ����� ����� ������ �����.
                    /*! �������� ���� ������ ����� �� ���� �������.
                    */
                    void nextReadKey()
                    {
                        s_Mutex.lock();
                        s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! ��������� ����� ������.
                    /*! \return ���������� ������� ���� ������.
                    */
                    int getReadKey()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_ReadKey;
                    }
                    //! ��������� ����� ������.
                    /*! \param newreadkey ��������������� ���� ������.
                    */
                    void setReadKey(int newreadkey)
                    {
                        s_Mutex.lock();
                        s_ReadKey = newreadkey;
                        s_Mutex.unlock();
                    }
                    //! ����� ����� ������ �����.
                    /*! �������� ���� ������ ����� �� ���� �������.
                    */
                    void nextWriteKey()
                    {
                        s_Mutex.lock();
                        if(s_WriteKey != -1) s_WriteKey++;
                        s_Mutex.unlock();
                    }
                    //! ��������� ����� ������.
                    /*! \return ���������� ������� ���� ������.
                    */
                    int getWriteKey()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_WriteKey;
                    }
                    //! ��������� ����� ������.
                    /*! \param newwritekey ��������������� ���� ������.
                    */
                    void setWriteKey(int newwritekey)
                    {
                        s_Mutex.lock();
                        s_WriteKey = newwritekey;
                        s_Mutex.unlock();
                    }
                    //! ��������� ����� ������ �� ���� ������.
                    /*! ���� ������ ���������� ������ ����� ������.
                    */
                    void setWriteKeyFromReadKey()
                    {
                        s_Mutex.lock();
                        s_WriteKey = s_ReadKey;
                        s_Mutex.unlock();
                    }
                    //! ����� ����� ������.
                    /*! ��������� ������ �������� � �������� ��������.
                    */
                    void resetWriteKey()
                    {
                        s_Mutex.lock();
                        s_WriteKey = -1;
                        s_Mutex.unlock();
                    }
                    //! ������� �������� � ����� ������.
                    /*! \param value ����������� �������.
                    */
                    void push_back(TYPE_Value value)
                    {
                        s_Mutex.lock();
                        s_Vector.push_back(value);
                        s_Mutex.unlock();
                    }
                    //! ��������� ������� ������.
                    /*! \return ���������� ���������� ��������� � ������.
                    */
                    unsigned int size()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Vector.size();
                    }
                    //! ������� ������.
                    /*! \param shrinktofit ���� ����� �������� true, �� ������ ������������ � �������, ����� ����� �� ���������.
                    */
                    void clear(bool shrinktofit = false)
                    {
                        s_Mutex.lock();
                        s_ReadKey = 0;
                        s_Vector.clear();
                        if(shrinktofit) s_Vector.shrink_to_fit();
                        s_Mutex.unlock();
                    }
                    //! �������� ��������� �������� vector �� �������.
                    TYPE_Value& operator[](const unsigned int i)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Vector[i];
                    }
                    //! ��������� ���������� ��������.
                    /*! �������� ���� ������ �� ������� �����.
                    \return ���������� �������, �� ������� ����� ���� ������.
                    */
                    TYPE_Value getNextElement()
                    {
                        std::function<bool()> predicat = [&]() mutable -> bool {std::lock_guard<std::mutex> locker(s_Mutex); return s_WriteKey != -1 && s_ReadKey >= s_WriteKey;};
                        while(predicat()) std::this_thread::yield();
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Vector[s_ReadKey++];
                    }
                private:
                    int s_ReadKey;
                    int s_WriteKey;
                    std::mutex s_Mutex;
                    std::vector<TYPE_Value> s_Vector;
            };
        }
    }
}

#endif
