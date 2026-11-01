#include "Additional/ARGBColor.h"

using namespace SimpleVoxelEngine;
using namespace Additional;

SimpleVoxelEngine::Additional::ARGBColor::ARGBColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b):
    s_A(a),
    s_Red(r),
    s_Green(g),
    s_Blue(b)
{
    //...
}

SimpleVoxelEngine::Additional::ARGBColor::ARGBColor(const ARGBColor& argbcolor):
    s_A( argbcolor.getA() ),
    s_Red( argbcolor.getR() ),
    s_Green( argbcolor.getG() ),
    s_Blue( argbcolor.getB() )
{
    //...
}

SimpleVoxelEngine::Additional::ARGBColor::~ARGBColor()
{
    //...
}

void SimpleVoxelEngine::Additional::ARGBColor::setARGB(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
    s_A = a;
    s_Red = r;
    s_Green = g;
    s_Blue = b;
}

unsigned char SimpleVoxelEngine::Additional::ARGBColor::getA() const
{
    return s_A;
}

unsigned char SimpleVoxelEngine::Additional::ARGBColor::getR() const
{
    return s_Red;
}

unsigned char SimpleVoxelEngine::Additional::ARGBColor::getG() const
{
    return s_Green;
}

unsigned char SimpleVoxelEngine::Additional::ARGBColor::getB() const
{
    return s_Blue;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator=(const ARGBColor& argbcolor)
{
    if(&argbcolor != this)
    {
        s_A = argbcolor.getA();
        s_Red = argbcolor.getR();
        s_Green = argbcolor.getG();
        s_Blue = argbcolor.getB();
    }
    return *this;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator+=(const ARGBColor& argbcolor)
{
    s_A += argbcolor.getA();
    s_Red += argbcolor.getR();
    s_Green += argbcolor.getG();
    s_Blue += argbcolor.getB();
    return *this;
}

//Исправлено: раньше возвращалась ссылка на локальную newargbcolor - после выхода из функции она указывала на
//уничтоженный объект (неопределённое поведение, GCC предупреждал -Wreturn-local-addr). Теперь результат возвращается по значению.
ARGBColor SimpleVoxelEngine::Additional::ARGBColor::operator+(const ARGBColor& argbcolor) const
{
    ARGBColor newargbcolor(*this);
    newargbcolor += argbcolor;
    return newargbcolor;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator*=(unsigned char what)
{
    setARGB(getA()*what, getR()*what, getG()*what, getB()*what);
    return *this;
}

//Исправлено так же, как operator+: возврат по значению.
ARGBColor SimpleVoxelEngine::Additional::ARGBColor::operator*(unsigned char what) const
{
    ARGBColor newargbcolor(*this);
    newargbcolor *= what;
    return newargbcolor;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator-=(const ARGBColor& argbcolor)
{
    return *this += argbcolor*(-1);
}

//Исправлено так же, как operator+: возврат по значению.
ARGBColor SimpleVoxelEngine::Additional::ARGBColor::operator-(const ARGBColor& argbcolor) const
{
    ARGBColor newargbcolor( *this );
    newargbcolor -= argbcolor;
    return newargbcolor;
}

ARGBColor SimpleVoxelEngine::Additional::operator*(unsigned char what, ARGBColor argbcolor) //Возврат по значению (раньше - ссылка на параметр-копию).
{
    return argbcolor*what;
}
