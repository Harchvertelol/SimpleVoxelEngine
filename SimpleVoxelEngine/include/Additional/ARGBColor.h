#ifndef SimpleVoxelEngine__ARGBColor_H
#define SimpleVoxelEngine__ARGBColor_H

#include <map>

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! ���� ARGB.
        /*! ������ ����� �������� � ���� ARGB ����. ����� ������������ ��� ����� ��������.
        */
        class ARGBColor
        {
            public:
                //! ����������� � �������� �����.
                /*! \param a ����� ����������.
                \param r ������� ����������.
                \param g ������ ����������.
                \param b ����� ����������.
                */
                ARGBColor(unsigned char a = 0, unsigned char r = 0, unsigned char g = 0, unsigned char b = 0);
                //! ����������� �����������.
                ARGBColor(const ARGBColor& argbcolor);
                //! ����������� ����������.
                ~ARGBColor();
                //! ������� ��������� �����.
                /*! \param a ����� ����������.
                \param r ������� ����������.
                \param g ������ ����������.
                \param b ����� ����������.
                */
                void setARGB(unsigned char a, unsigned char r, unsigned char g, unsigned char b);
                //! ��������� ����� ����������.
                /*! \return ���������� ����� ����������.*/
                unsigned char getA() const;
                //! ��������� ������� ����������.
                /*! \return ���������� ������� ����������.*/
                unsigned char getR() const;
                //! ��������� ������ ����������.
                /*! \return ���������� ������ ����������.*/
                unsigned char getG() const;
                //! ��������� ������� ����������.
                /*! \return ���������� ������� ����������.*/
                unsigned char getB() const;
                //! �������� ������������.
                ARGBColor& operator=(const ARGBColor&);
                //! �������� ���������� �������� � ���������.
                ARGBColor& operator+=(ARGBColor&);
                //! �������� ��������.
                ARGBColor& operator+(ARGBColor&);
                //! �������� ���������.
                ARGBColor& operator*(unsigned char);
                //! �������� ��������� ��������� �� ��������.
                ARGBColor& operator*=(unsigned char);
                //! �������� ��������� �������� �� ���������.
                ARGBColor& operator-=(ARGBColor&);
                //! �������� ���������.
                ARGBColor& operator-(ARGBColor&);
            private:
                unsigned char s_A;
                unsigned char s_Red;
                unsigned char s_Green;
                unsigned char s_Blue;
        };
        //! �������� ��������� �� ����� �����.
        ARGBColor& operator*(unsigned char, ARGBColor);
    }
}

#endif
