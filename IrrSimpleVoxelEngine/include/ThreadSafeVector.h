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
            //! Аналог класса std::vector, но с безопасной работой в потоках.
            /*! Данный класс реализует потокобезопасно некоторые методы std::vector.
            */
            template<typename TYPE_Value> class ThreadSafeVector
            {
                public:
                    //! Стандартный конструктор.
                    ThreadSafeVector():
                        s_ReadKey(0), //! Ключ чтения с помощью дополнительных функций.
                        s_WriteKey(-1) //! Ключ записи с помощью дополнительных функций.
                    {
                        //...
                    }
                    //! Стандартный деструктор.
                    ~ThreadSafeVector()
                    {
                        //...
                    }
                    //! Вставка элемента до ключа записи.
                    /*! \param value Вставляемый элемент.
                    \param nextwritekey Если имеет значение true, то ключ записи будет сдвинут вперёд, иначе оставлен прежним.
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
                    //! Вставка элемента после ключа записи.
                    /*! \param value Вставляемый элемент.
                    \param nextwritekey Если имеет значение true, то ключ записи будет сдвинут вперёд, иначе оставлен прежним.
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
                    //! Вставка элемента после ключа чтения.
                    /*! \param value Вставляемый элемент.
                    \param nextreadkey Если имеет значение true, то ключ чтения будет сдвинут вперёд, иначе оставлен прежним.
                    */
                    void insertBeforeReadKey(TYPE_Value value, bool nextreadkey = false)
                    {
                        s_Mutex.lock();
                        s_Vector.insert(s_Vector.begin() + s_ReadKey - 1, value);
                        if(nextreadkey) s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! Вставка элемента до ключа чтения.
                    /*! \param value Вставляемый элемент.
                    \param nextreadkey Если имеет значение true, то ключ чтения будет сдвинут вперёд, иначе оставлен прежним.
                    */
                    void insertAfterReadKey(TYPE_Value value, bool nextreadkey = false)
                    {
                        s_Mutex.lock();
                        s_Vector.insert(s_Vector.begin() + s_ReadKey, value);
                        if(nextreadkey) s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! Сдвиг ключа чтения вперёд.
                    /*! Сдвигает ключ чтения вперёд на одну позицию.
                    */
                    void nextReadKey()
                    {
                        s_Mutex.lock();
                        s_ReadKey++;
                        s_Mutex.unlock();
                    }
                    //! Получение ключа чтения.
                    /*! \return Возвращает текущий ключ чтения.
                    */
                    int getReadKey()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_ReadKey;
                    }
                    //! Установка ключа чтения.
                    /*! \param newreadkey Устанавливаемый ключ чтения.
                    */
                    void setReadKey(int newreadkey)
                    {
                        s_Mutex.lock();
                        s_ReadKey = newreadkey;
                        s_Mutex.unlock();
                    }
                    //! Сдвиг ключа записи вперёд.
                    /*! Сдвигает ключ записи вперёд на одну позицию.
                    */
                    void nextWriteKey()
                    {
                        s_Mutex.lock();
                        if(s_WriteKey != -1) s_WriteKey++;
                        s_Mutex.unlock();
                    }
                    //! Получение ключа записи.
                    /*! \return Возвращает текущий ключ записи.
                    */
                    int getWriteKey()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_WriteKey;
                    }
                    //! Установка ключа записи.
                    /*! \param newwritekey Устанавливаемый ключ записи.
                    */
                    void setWriteKey(int newwritekey)
                    {
                        s_Mutex.lock();
                        s_WriteKey = newwritekey;
                        s_Mutex.unlock();
                    }
                    //! Установка ключа записи на ключ чтения.
                    /*! Ключ записи становится равным ключу чтения.
                    */
                    void setWriteKeyFromReadKey()
                    {
                        s_Mutex.lock();
                        s_WriteKey = s_ReadKey;
                        s_Mutex.unlock();
                    }
                    //! Сброс ключа записи.
                    /*! Следующая запись начнутся с нулевого элемента.
                    */
                    void resetWriteKey()
                    {
                        s_Mutex.lock();
                        s_WriteKey = -1;
                        s_Mutex.unlock();
                    }
                    //! Вставка элемента в конец списка.
                    /*! \param value Вставляемый элемент.
                    */
                    void push_back(TYPE_Value value)
                    {
                        s_Mutex.lock();
                        s_Vector.push_back(value);
                        s_Mutex.unlock();
                    }
                    //! Получение размера списка.
                    /*! \return Возвращает количество элементов в списке.
                    */
                    unsigned int size()
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Vector.size();
                    }
                    //! Очистка списка.
                    /*! \param shrinktofit Если имеет значение true, то память возвращается в систему, иначе может не вернуться.
                    */
                    void clear(bool shrinktofit = false)
                    {
                        s_Mutex.lock();
                        s_ReadKey = 0;
                        s_Vector.clear();
                        if(shrinktofit) s_Vector.shrink_to_fit();
                        s_Mutex.unlock();
                    }
                    //! Оператор получения элемента vector по индексу.
                    TYPE_Value& operator[](const unsigned int i)
                    {
                        std::lock_guard<std::mutex> locker(s_Mutex);
                        return s_Vector[i];
                    }
                    //! Получение следующего элемента.
                    /*! Сдвигает ключ чтения на единицу вперёд.
                    \return Возвращает элемент, на котором стоит ключ чтения.
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
