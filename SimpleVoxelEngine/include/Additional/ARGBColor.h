#ifndef SimpleVoxelEngine__ARGBColor_H
#define SimpleVoxelEngine__ARGBColor_H

#include <map>

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! Цвет ARGB.
        /*! Данный класс содержит в себе ARGB цвет. Можно использовать для цвета вокселей.
        Компоненты - unsigned char, арифметика (+, -, *) идёт по модулю 256 (переполнение "по кругу", без насыщения):
        например, 200 + 100 = 44, 0 - 1 = 255.
        */
        class ARGBColor
        {
            public:
                //! Конструктор с заданием цвета.
                /*! \param a Альфа компонента.
                \param r Красная компонента.
                \param g Зелёная компонента.
                \param b Синяя компонента.
                */
                ARGBColor(unsigned char a = 0, unsigned char r = 0, unsigned char g = 0, unsigned char b = 0);
                //! Конструктор копирования.
                ARGBColor(const ARGBColor& argbcolor);
                //! Стандартный деструктор.
                ~ARGBColor();
                //! Функция установки цвета.
                /*! \param a Альфа компонента.
                \param r Красная компонента.
                \param g Зелёная компонента.
                \param b Синяя компонента.
                */
                void setARGB(unsigned char a, unsigned char r, unsigned char g, unsigned char b);
                //! Получение альфа компоненты.
                /*! \return Возвращает альфа компоненту.*/
                unsigned char getA() const;
                //! Получение красной компоненты.
                /*! \return Возвращает красную компоненту.*/
                unsigned char getR() const;
                //! Получение зелёной компоненты.
                /*! \return Возвращает зелёную компоненту.*/
                unsigned char getG() const;
                //! Получение голубой компоненты.
                /*! \return Возвращает голубую компоненту.*/
                unsigned char getB() const;
                //! Оператор присваивания.
                ARGBColor& operator=(const ARGBColor&);
                //! Оператор добавления значения к исходному.
                ARGBColor& operator+=(const ARGBColor&); //Исправлено: const-ссылка, чтобы можно было прибавлять временный цвет.
                //! Оператор сложения.
                ARGBColor operator+(const ARGBColor&) const; //Исправлено: возвращала ссылку на локальную переменную (уже уничтоженную).
                //! Оператор умножения.
                ARGBColor operator*(unsigned char) const; //Исправлено: возвращала ссылку на локальную переменную.
                //! Оператор умножения исходного на значение.
                ARGBColor& operator*=(unsigned char);
                //! Оператор вычитания значения из исходного.
                ARGBColor& operator-=(const ARGBColor&);
                //! Оператор вычитания.
                ARGBColor operator-(const ARGBColor&) const; //Исправлено: возвращала ссылку на локальную переменную.
            private:
                unsigned char s_A;
                unsigned char s_Red;
                unsigned char s_Green;
                unsigned char s_Blue;
        };
        //! Оператор умножения на число слева.
        ARGBColor operator*(unsigned char, ARGBColor); //Исправлено: возвращала ссылку на локальную переменную.
    }
}

#endif
