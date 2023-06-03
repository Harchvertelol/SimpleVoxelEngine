#ifndef SimpleVoxelEngine__Vector3d_H
#define SimpleVoxelEngine__Vector3d_H

#include <cmath>

#include "WorkFunctions.h"
#include "Defines.h"

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! Шаблонный класс 3d вектора.
        /*! Данный класс содержит основные методы, необходимые для работы с векторами в 3d пространстве..
        */
        template<typename TYPE_TypeCoord> class Vector3d
        {
            public:
                //! Конструктор по умолчанию.
                /*! \param x Координата X вектора.
                \param y Координата Y вектора.
                \param z Координата Z вектора.
                */
                Vector3d(TYPE_TypeCoord x = 0, TYPE_TypeCoord y = 0, TYPE_TypeCoord z = 0): s_XCoord(x), s_YCoord(y), s_ZCoord(z) {}
                //! Конструктор копирования.
                Vector3d(const Vector3d<TYPE_TypeCoord>& vect): s_XCoord(vect.getX()), s_YCoord(vect.getY()), s_ZCoord(vect.getZ()) {}
                //! Стандартный деструктор.
                ~Vector3d() {}
                //! Установка вектора какому-либо значению.
                /*! \param x Координата X вектора.
                \param y Координата Y вектора.
                \param z Координата Z вектора.
                */
                void setVector(TYPE_TypeCoord x, TYPE_TypeCoord y, TYPE_TypeCoord z)
                {
                    s_XCoord = x;
                    s_YCoord = y;
                    s_ZCoord = z;
                }
                //! Получение X координаты.
                /*! \return Возвращает X координату вектора.
                */
                inline TYPE_TypeCoord getX() const
                {
                    return s_XCoord;
                }
                //! Получение Y координаты.
                /*! \return Возвращает Y координату вектора.
                */
                inline TYPE_TypeCoord getY() const
                {
                    return s_YCoord;
                }
                //! Получение Z координаты.
                /*! \return Возвращает Z координату вектора.
                */
                inline TYPE_TypeCoord getZ() const
                {
                    return s_ZCoord;
                }
                //! Преобразование вектора в string.
                /*! Преобразует вектор в строковый вид, используя тип float для функции преобразования.
                \param split_str Разделитель координат.
                \return Строковый вид вектора.
                */
                inline std::string toString(std::string split_str = ", ") const
                {
                    return WorkFunctions::ConvertFunctions::ftos( getX() ) + split_str + WorkFunctions::ConvertFunctions::ftos( getY() ) + split_str + WorkFunctions::ConvertFunctions::ftos( getZ() );
                }
                //! Получение квадрата стандартной нормы вектора.
                /*! \return Возвращает квадрат стандартной нормы вектора.
                */
                inline TYPE_TypeCoord getSquareNorm() const
                {
                    return s_XCoord*s_XCoord + s_YCoord*s_YCoord + s_ZCoord*s_ZCoord;
                }
                //! Получение стандартной нормы вектора.
                /*! \return Возвращает стандартную норму вектора.
                */
                inline TYPE_TypeCoord getNorm() const
                {
                    return sqrt(getSquareNorm());
                }
                //! Получение вектора-модуля.
                /*! \return Возвращает вектор, координаты которого есть модули координат исходного вектора.
                */
                inline Vector3d<TYPE_TypeCoord> getAbs() const
                {
                    Vector3d<TYPE_TypeCoord> newvect(std::abs(getX()), std::abs(getY()), std::abs(getZ()));
                    return newvect;
                }
                //! Получение нормализованного вектора.
                /*! \return Возвращает нормализованный вектор.
                */
                inline Vector3d<TYPE_TypeCoord> getNormalizeVector() const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect / getNorm();
                }
                //! Вычисление скалярного произведения.
                /*! \param vect Вектор, на который скалярно умножаем исходный.
                \return Возвращает результат скалярного произведения исходного вектора на vect.
                */
                inline TYPE_TypeCoord scalarProduct(const Vector3d<TYPE_TypeCoord> vect) const
                {
                    return s_XCoord*vect.getX() + s_YCoord*vect.getY() + s_ZCoord*vect.getZ();
                }
                //! Вычисление векторного произведения.
                /*! \param vect Вектор, на который векторно умножаем исходный.
                \return Возвращает результат векторного произведения исходного вектора на vect.
                */
                inline Vector3d<TYPE_TypeCoord> crossProduct(const Vector3d<TYPE_TypeCoord> vect) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(s_YCoord*vect.getZ() - s_ZCoord*vect.getY(), s_ZCoord*vect.getX() - s_XCoord*vect.getZ(), s_XCoord*vect.getY() - s_YCoord*vect.getX());
                    return newvect;
                }
                //! Оператор сравнения "меньше".
                bool operator<(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    if(getX() < vect.getX()) return true;
                    else if(getX() == vect.getX())
                    {
                        if(getY() < vect.getY()) return true;
                        else if(getY() == vect.getY())
                        {
                            if(getZ() < vect.getZ()) return true;
                        }
                    }
                    return false;
                }
                //! Оператор сравнения "больше".
                bool operator>(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    if(getX() > vect.getX()) return true;
                    else if(getX() == vect.getX())
                    {
                        if(getY() > vect.getY()) return true;
                        else if(getY() == vect.getY())
                        {
                            if(getZ() > vect.getZ()) return true;
                        }
                    }
                    return false;
                }
                //! Оператор сравнения "меньше или равно".
                bool operator<=(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    return (*this) < vect || (*this) == vect;
                }
                //! Оператор сравнения "больше или равно".
                bool operator>=(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    return (*this) > vect || (*this) == vect;
                }
                //! Оператор присваивания.
                Vector3d<TYPE_TypeCoord>& operator=(const Vector3d<TYPE_TypeCoord>& vect)
                {
                    if(this != &vect)
                    {
                        s_XCoord = vect.getX();
                        s_YCoord = vect.getY();
                        s_ZCoord = vect.getZ();
                    }
                    return *this;
                }
                //! Оператор добавления вектора к исходному.
                Vector3d<TYPE_TypeCoord>& operator+=(const Vector3d<TYPE_TypeCoord>& vect)
                {
                    s_XCoord += vect.getX();
                    s_YCoord += vect.getY();
                    s_ZCoord += vect.getZ();
                    return *this;
                }
                //! Оператор вычитания вектора из исходного.
                Vector3d<TYPE_TypeCoord>& operator-=(const Vector3d<TYPE_TypeCoord>& vect)
                {
                    Vector3d<TYPE_TypeCoord> newvect(vect*(-1));
                    return (*this) += newvect;
                }
                //! Оператор сложения.
                Vector3d<TYPE_TypeCoord> operator+(const Vector3d<TYPE_TypeCoord>& vect1) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect += vect1;
                }
                //! Оператор умножения на число.
                Vector3d<TYPE_TypeCoord> operator*(TYPE_TypeCoord what) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect *= what;
                }
                //! Оператор умножения исходного на число.
                Vector3d<TYPE_TypeCoord>& operator*=(TYPE_TypeCoord what)
                {
                    s_XCoord *= what;
                    s_YCoord *= what;
                    s_ZCoord *= what;
                    return *this;
                }
                //! Оператор деления исходного на число.
                Vector3d<TYPE_TypeCoord>& operator/=(TYPE_TypeCoord what)
                {
                    s_XCoord /= what;
                    s_YCoord /= what;
                    s_ZCoord /= what;
                    return *this;
                }
                //! Оператор деления.
                Vector3d<TYPE_TypeCoord> operator/(TYPE_TypeCoord what) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect /= what;
                }
                //! Оператор вычитания.
                Vector3d<TYPE_TypeCoord> operator-(const Vector3d<TYPE_TypeCoord>& vect1) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect -= vect1;
                }
            private:
                TYPE_TypeCoord s_XCoord;
                TYPE_TypeCoord s_YCoord;
                TYPE_TypeCoord s_ZCoord;
        };
        //! Оператор умножения на число слева.
        template<class TYPE_TypeCoord> Vector3d<TYPE_TypeCoord> operator*(TYPE_TypeCoord what, const Vector3d<TYPE_TypeCoord> vect)
        {
            return vect*what;
        }
        //! Оператор сравнения "меньше" для возможности использования класса как индексатора std::map.
        template<class TYPE_TypeCoord> bool operator<(const Vector3d<TYPE_TypeCoord>& vect1, const Vector3d<TYPE_TypeCoord>& vect2)
        {
            if(vect1.getX() < vect2.getX()) return true;
            else if(vect1.getX() == vect2.getX())
            {
                if(vect1.getY() < vect2.getY()) return true;
                else if(vect1.getY() == vect2.getY())
                {
                    if(vect1.getZ() < vect2.getZ()) return true;
                }
            }
            return false;
        }
        //! Оператор сравнения "равно" для возможности использования класса как индексатора std::map.
        template<class TYPE_TypeCoord> bool operator==(const Vector3d<TYPE_TypeCoord>& vect1, const Vector3d<TYPE_TypeCoord>& vect2)
        {
            return std::abs(vect1.getX() - vect2.getX()) <= SimpleVoxelEngine::Defines::EPS && std::abs(vect1.getY() - vect2.getY()) <= SimpleVoxelEngine::Defines::EPS && std::abs(vect1.getZ() - vect2.getZ()) <= SimpleVoxelEngine::Defines::EPS;
        }
        //! Оператор сравнения "не равно" для возможности использования класса как индексатора std::map.
        template<class TYPE_TypeCoord> bool operator!=(const Vector3d<TYPE_TypeCoord>& vect1, const Vector3d<TYPE_TypeCoord>& vect2)
        {
            return !(vect1 == vect2);
        }
    }
}

#endif
