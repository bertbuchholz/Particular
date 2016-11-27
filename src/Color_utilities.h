#ifndef COLOR_UTILITIES_H
#define COLOR_UTILITIES_H

#include <QColor>

#include "Color.h"

template <typename Data0, typename Data1>
Data0 convert(Data1 const& data);

template <> Color convert(QColor const& data);
template <> Color4 convert(QRgb const& data);
template <> Color4 convert(QColor const& data);
template <> QColor convert(Color const& data);
template <> QColor convert(Color4 const& data);
template <> int convert(QColor const& data);
template <> int convert(Color const& data);

struct QColor_to_Color_converter
{
    Color operator() (QColor const& col) const
    {
        return convert<Color>(col);
    }
};

struct QColor_to_int_converter
{
    int operator() (QColor const& col) const
    {
        return convert<int>(col);
    }
};

struct Color_to_int_converter
{
    int operator() (Color const& col) const
    {
        return convert<int>(col);
    }
};

struct QRgb_to_Color4_converter
{
    Color4 operator() (QRgb const& col) const
    {
        return convert<Color4>(col);
    }
};

struct QColor_to_Color4_converter
{
    Color4 operator() (QColor const& col) const
    {
        return convert<Color4>(col);
    }
};

struct Color_to_QColor_converter
{
    QColor operator() (Color const& col) const
    {
        return convert<QColor>(col);
    }
};

struct Color4_to_QColor_converter
{
    QColor operator() (Color4 const& col) const
    {
        return convert<QColor>(col);
    }
};

struct QColor_alpha_to_Color_gray_converter
{
    Color operator() (QColor const& col) const
    {
        float const alpha = col.alphaF();
        return Color(alpha, alpha, alpha);
    }
};

#endif // COLOR_UTILITIES_H
