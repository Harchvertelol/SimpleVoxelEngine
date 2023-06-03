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

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator+=(ARGBColor& argbcolor)
{
    s_A += argbcolor.getA();
    s_Red += argbcolor.getR();
    s_Green += argbcolor.getG();
    s_Blue += argbcolor.getB();
    return *this;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator+(ARGBColor& argbcolor)
{
    ARGBColor newargbcolor(*this);
    return newargbcolor += argbcolor;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator*=(unsigned char what)
{
    setARGB(getA()*what, getR()*what, getG()*what, getB()*what);
    return *this;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator*(unsigned char what)
{
    ARGBColor newargbcolor(*this);
    return newargbcolor *= what;
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator-=(ARGBColor& argbcolor)
{
    return *this += argbcolor*(-1);
}

ARGBColor& SimpleVoxelEngine::Additional::ARGBColor::operator-(ARGBColor& argbcolor)
{
    ARGBColor newargbcolor( *this );
    return newargbcolor -= argbcolor;
}

ARGBColor& SimpleVoxelEngine::Additional::operator*(unsigned char what, ARGBColor argbcolor)
{
    return argbcolor*what;
}
