#ifndef COLOR_H
#define COLOR_H

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#endif

class Color
{
    friend Color operator * (const Color &a, const Color &b);
    friend Color operator * (const float f, const Color &b);
    friend Color operator * (const Color &b, const float f);
    friend Color operator / (const Color &a, const Color &b);
    friend Color operator / (const Color &b, const float f);
    friend Color operator + (const Color &a, const Color &b);
    friend Color operator - (const Color &a, const Color &b);
    friend bool operator != (const Color &a, const Color &b)
    {
        return (a.R != b.R || a.G != b.G || a.B != b.B);
    }
    friend bool operator >(const Color &a, const Color &b)
    {
        return (a.energy() > b.energy());
    }
    friend bool operator <(const Color &a, const Color &b)
    {
        return (a.energy() < b.energy());
    }

    friend float maxAbsDiff(const Color &a, const Color &b);

public:
    Color() { R=G=B=0; }
    Color(float r, float g, float b) {R=r;G=g;B=b;}
    explicit Color(float g) { R=G=B=g; }
    Color(float af[3]) { R=af[0];  G=af[1];  B=af[2]; }
    bool isBlack() const { return ((R==0) && (G==0) && (B==0)); }
    ~Color() {}
    void set(float r, float g, float b) { R=r;  G=g;  B=b; }

    Color & operator +=(const Color &c);
    Color & operator -=(const Color &c);
    Color & operator *=(const Color &c);
    Color & operator *=(float f);

    float   operator[] (int i) const { return (&R)[i]; }
    float & operator[] (int i)       { return (&R)[i]; }

    float energy() const {return (R+G+B)*0.333333f;}
    // Using ITU/Photometric values Y = 0.2126 R + 0.7152 G + 0.0722 B
    float col2bri() const { return (0.2126f*R + 0.7152f*G + 0.0722f*B); }
    float abscol2bri() const { return (0.2126f*std::abs(R) + 0.7152f*std::abs(G) + 0.0722f*std::abs(B)); }
    void gammaAdjust(float g){ R = std::pow(R, g); G = std::pow(G, g); B = std::pow(B, g); }
    void expgam_Adjust (float e, float g, bool clamp_rgb);
    float getR() const { return R; }
    float getG() const { return G; }
    float getB() const { return B; }

    // used in blendershader
    void invertRGB()
    {
        if (R!=0.f) R=1.f/R;
        if (G!=0.f) G=1.f/G;
        if (B!=0.f) B=1.f/B;
    }
    void absRGB() { R=std::fabs(R);  G=std::fabs(G);  B=std::fabs(B); }
    void darkenRGB(const Color &col)
    {
        if (R>col.R) R=col.R;
        if (G>col.G) G=col.G;
        if (B>col.B) B=col.B;
    }
    void lightenRGB(const Color &col)
    {
        if (R<col.R) R=col.R;
        if (G<col.G) G=col.G;
        if (B<col.B) B=col.B;
    }

    void black() { R=G=B=0; }
    float minimum() const { return std::min(R, std::min(G, B)); }
    float maximum() const { return std::max(R, std::max(G, B)); }
    float absmax() const { return std::max(std::fabs(R), std::max(std::fabs(G), std::fabs(B))); }
    void clampRGB0()
    {
        if (R<0.0) R=0.0;
        if (G<0.0) G=0.0;
        if (B<0.0) B=0.0;
    }

    void clampRGB01()
    {
        if (R<0.0) R=0.0; else if (R>1.0) R=1.0;
        if (G<0.0) G=0.0; else if (G>1.0) G=1.0;
        if (B<0.0) B=0.0; else if (B>1.0) B=1.0;
    }

    float * data()
    {
        return &R;
    }

    float const* data() const
    {
        return &R;
    }

    static Color from_int(int r, int g, int b)
    {
        return Color(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(R);
        ar & BOOST_SERIALIZATION_NVP(G);
        ar & BOOST_SERIALIZATION_NVP(B);
    }

    //	protected:
    float R, G, B;
};


inline Color & Color::operator +=(const Color &c)
{
    R += c.R;  G += c.G;  B += c.B;  return *this;
}

inline Color & Color::operator *=(const Color &c)
{
    R *= c.R;  G *= c.G;  B *= c.B;  return *this;
}

inline Color & Color::operator *=(float const f)
{
    R *= f;  G*= f;  B *= f;  return *this;
}

inline Color & Color::operator -=(const Color &c)
{
    R -= c.R;  G -= c.G;  B -= c.B;  return *this;
}

inline Color operator * (const Color &a,const Color &b)
{
    return Color(a.R*b.R,a.G*b.G,a.B*b.B);
}

inline Color operator / (const Color &a,const Color &b)
{
    return Color((b.R > 0.000001f) ? (a.R / b.R) : 0.0f,
                 (b.G > 0.000001f) ? (a.G / b.G) : 0.0f,
                 (b.B > 0.000001f) ? (a.B / b.B) : 0.0f);
}

inline Color operator * (const float f,const Color &b)
{
    return Color(f*b.R,f*b.G,f*b.B);
}

inline Color operator * (const Color &b,const float f)
{
    return Color(f*b.R,f*b.G,f*b.B);
}

inline Color operator / (const Color &b,float f)
{
    return Color(b.R/f,b.G/f,b.B/f);
}

inline Color operator + (const Color &a,const Color &b)
{
    return Color(a.R+b.R,a.G+b.G,a.B+b.B);
}

inline Color operator - (const Color &a, const Color &b)
{
    return Color(a.R-b.R, a.G-b.G, a.B-b.B);
}

std::ostream & operator << (std::ostream & out, Color const& c);


struct Color4
{
    Color4() { r=g=b=a=0; }
    Color4(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_)
    { }

    Color4(Color const& color, float const alpha) : r(color.R), g(color.G), b(color.B), a(alpha)
    { }

    float   operator[] (int i) const { return (&r)[i]; }
    float & operator[] (int i)       { return (&r)[i]; }

    Color rgb() const { return Color(r, g, b); }

    float const* data() const
    {
        return &r;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(r);
        ar & BOOST_SERIALIZATION_NVP(g);
        ar & BOOST_SERIALIZATION_NVP(b);
        ar & BOOST_SERIALIZATION_NVP(a);
    }

    float r, g, b, a;
};


#endif // COLOR_H
