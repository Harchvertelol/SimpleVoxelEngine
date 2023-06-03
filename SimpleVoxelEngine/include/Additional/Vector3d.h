#ifndef SimpleVoxelEngine__Vector3d_H
#define SimpleVoxelEngine__Vector3d_H

#include <cmath>

#include "WorkFunctions.h"
#include "Defines.h"

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! ��������� ����� 3d �������.
        /*! ������ ����� �������� �������� ������, ����������� ��� ������ � ��������� � 3d ������������..
        */
        template<typename TYPE_TypeCoord> class Vector3d
        {
            public:
                //! ����������� �� ���������.
                /*! \param x ���������� X �������.
                \param y ���������� Y �������.
                \param z ���������� Z �������.
                */
                Vector3d(TYPE_TypeCoord x = 0, TYPE_TypeCoord y = 0, TYPE_TypeCoord z = 0): s_XCoord(x), s_YCoord(y), s_ZCoord(z) {}
                //! ����������� �����������.
                Vector3d(const Vector3d<TYPE_TypeCoord>& vect): s_XCoord(vect.getX()), s_YCoord(vect.getY()), s_ZCoord(vect.getZ()) {}
                //! ����������� ����������.
                ~Vector3d() {}
                //! ��������� ������� ������-���� ��������.
                /*! \param x ���������� X �������.
                \param y ���������� Y �������.
                \param z ���������� Z �������.
                */
                void setVector(TYPE_TypeCoord x, TYPE_TypeCoord y, TYPE_TypeCoord z)
                {
                    s_XCoord = x;
                    s_YCoord = y;
                    s_ZCoord = z;
                }
                //! ��������� X ����������.
                /*! \return ���������� X ���������� �������.
                */
                inline TYPE_TypeCoord getX() const
                {
                    return s_XCoord;
                }
                //! ��������� Y ����������.
                /*! \return ���������� Y ���������� �������.
                */
                inline TYPE_TypeCoord getY() const
                {
                    return s_YCoord;
                }
                //! ��������� Z ����������.
                /*! \return ���������� Z ���������� �������.
                */
                inline TYPE_TypeCoord getZ() const
                {
                    return s_ZCoord;
                }
                //! �������������� ������� � string.
                /*! ����������� ������ � ��������� ���, ��������� ��� float ��� ������� ��������������.
                \param split_str ����������� ���������.
                \return ��������� ��� �������.
                */
                inline std::string toString(std::string split_str = ", ") const
                {
                    return WorkFunctions::ConvertFunctions::ftos( getX() ) + split_str + WorkFunctions::ConvertFunctions::ftos( getY() ) + split_str + WorkFunctions::ConvertFunctions::ftos( getZ() );
                }
                //! ��������� �������� ����������� ����� �������.
                /*! \return ���������� ������� ����������� ����� �������.
                */
                inline TYPE_TypeCoord getSquareNorm() const
                {
                    return s_XCoord*s_XCoord + s_YCoord*s_YCoord + s_ZCoord*s_ZCoord;
                }
                //! ��������� ����������� ����� �������.
                /*! \return ���������� ����������� ����� �������.
                */
                inline TYPE_TypeCoord getNorm() const
                {
                    return sqrt(getSquareNorm());
                }
                //! ��������� �������-������.
                /*! \return ���������� ������, ���������� �������� ���� ������ ��������� ��������� �������.
                */
                inline Vector3d<TYPE_TypeCoord> getAbs() const
                {
                    Vector3d<TYPE_TypeCoord> newvect(std::abs(getX()), std::abs(getY()), std::abs(getZ()));
                    return newvect;
                }
                //! ��������� ���������������� �������.
                /*! \return ���������� ��������������� ������.
                */
                inline Vector3d<TYPE_TypeCoord> getNormalizeVector() const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect / getNorm();
                }
                //! ���������� ���������� ������������.
                /*! \param vect ������, �� ������� �������� �������� ��������.
                \return ���������� ��������� ���������� ������������ ��������� ������� �� vect.
                */
                inline TYPE_TypeCoord scalarProduct(const Vector3d<TYPE_TypeCoord> vect) const
                {
                    return s_XCoord*vect.getX() + s_YCoord*vect.getY() + s_ZCoord*vect.getZ();
                }
                //! ���������� ���������� ������������.
                /*! \param vect ������, �� ������� �������� �������� ��������.
                \return ���������� ��������� ���������� ������������ ��������� ������� �� vect.
                */
                inline Vector3d<TYPE_TypeCoord> crossProduct(const Vector3d<TYPE_TypeCoord> vect) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(s_YCoord*vect.getZ() - s_ZCoord*vect.getY(), s_ZCoord*vect.getX() - s_XCoord*vect.getZ(), s_XCoord*vect.getY() - s_YCoord*vect.getX());
                    return newvect;
                }
                //! �������� ��������� "������".
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
                //! �������� ��������� "������".
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
                //! �������� ��������� "������ ��� �����".
                bool operator<=(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    return (*this) < vect || (*this) == vect;
                }
                //! �������� ��������� "������ ��� �����".
                bool operator>=(const Vector3d<TYPE_TypeCoord>& vect) const
                {
                    return (*this) > vect || (*this) == vect;
                }
                //! �������� ������������.
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
                //! �������� ���������� ������� � ���������.
                Vector3d<TYPE_TypeCoord>& operator+=(const Vector3d<TYPE_TypeCoord>& vect)
                {
                    s_XCoord += vect.getX();
                    s_YCoord += vect.getY();
                    s_ZCoord += vect.getZ();
                    return *this;
                }
                //! �������� ��������� ������� �� ���������.
                Vector3d<TYPE_TypeCoord>& operator-=(const Vector3d<TYPE_TypeCoord>& vect)
                {
                    Vector3d<TYPE_TypeCoord> newvect(vect*(-1));
                    return (*this) += newvect;
                }
                //! �������� ��������.
                Vector3d<TYPE_TypeCoord> operator+(const Vector3d<TYPE_TypeCoord>& vect1) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect += vect1;
                }
                //! �������� ��������� �� �����.
                Vector3d<TYPE_TypeCoord> operator*(TYPE_TypeCoord what) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect *= what;
                }
                //! �������� ��������� ��������� �� �����.
                Vector3d<TYPE_TypeCoord>& operator*=(TYPE_TypeCoord what)
                {
                    s_XCoord *= what;
                    s_YCoord *= what;
                    s_ZCoord *= what;
                    return *this;
                }
                //! �������� ������� ��������� �� �����.
                Vector3d<TYPE_TypeCoord>& operator/=(TYPE_TypeCoord what)
                {
                    s_XCoord /= what;
                    s_YCoord /= what;
                    s_ZCoord /= what;
                    return *this;
                }
                //! �������� �������.
                Vector3d<TYPE_TypeCoord> operator/(TYPE_TypeCoord what) const
                {
                    Vector3d<TYPE_TypeCoord> newvect(*this);
                    return newvect /= what;
                }
                //! �������� ���������.
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
        //! �������� ��������� �� ����� �����.
        template<class TYPE_TypeCoord> Vector3d<TYPE_TypeCoord> operator*(TYPE_TypeCoord what, const Vector3d<TYPE_TypeCoord> vect)
        {
            return vect*what;
        }
        //! �������� ��������� "������" ��� ����������� ������������� ������ ��� ����������� std::map.
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
        //! �������� ��������� "�����" ��� ����������� ������������� ������ ��� ����������� std::map.
        template<class TYPE_TypeCoord> bool operator==(const Vector3d<TYPE_TypeCoord>& vect1, const Vector3d<TYPE_TypeCoord>& vect2)
        {
            return std::abs(vect1.getX() - vect2.getX()) <= SimpleVoxelEngine::Defines::EPS && std::abs(vect1.getY() - vect2.getY()) <= SimpleVoxelEngine::Defines::EPS && std::abs(vect1.getZ() - vect2.getZ()) <= SimpleVoxelEngine::Defines::EPS;
        }
        //! �������� ��������� "�� �����" ��� ����������� ������������� ������ ��� ����������� std::map.
        template<class TYPE_TypeCoord> bool operator!=(const Vector3d<TYPE_TypeCoord>& vect1, const Vector3d<TYPE_TypeCoord>& vect2)
        {
            return !(vect1 == vect2);
        }
    }
}

#endif
