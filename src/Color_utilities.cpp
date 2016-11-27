#include "Color_utilities.h"

template <>
Color convert(QColor const& data)
{
    Color c(data.redF(), data.greenF(), data.blueF());
    return c;
}

template <>
Color4 convert(QRgb const& data)
{
    Color4 c(qRed(data) / 255.0f, qGreen(data) / 255.0f, qBlue(data) / 255.0f, qAlpha(data) / 255.0f);
    return c;
}

template <>
Color4 convert(QColor const& data)
{
    Color4 c(data.redF(), data.greenF(), data.blueF(), data.alphaF());
    return c;
}

template <>
QColor convert(Color const& data)
{
    QColor c = QColor::fromRgbF(data.R, data.G, data.B);
    return c;
}

template <>
QColor convert(Color4 const& data)
{
    QColor c = QColor::fromRgbF(data.r, data.g, data.b, data.a);
    return c;
}

template <>
int convert(QColor const& data)
{
    return data.red();
}

template <>
int convert(Color const& data)
{
    return data.R;
}
