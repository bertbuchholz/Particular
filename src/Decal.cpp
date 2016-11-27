#include "Decal.h"

#include <QImage>
#include <QColor>
#include <QPainter>

#include "GL_utilities.h"

GLuint generate_info_label_texture(Info_label const& label, int const text_alignment_flags, QFont font)
{
//    QSize const pixel_size(label._size[0] * screen_size.width(), label._size[1] * screen_size.height());
    QSize const pixel_size(label._size[0] * 1000, label._size[1] * 1000);

    QImage img(pixel_size, QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));

    QPainter p;
    p.begin(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

//    font.setWeight(QFont::Bold);
//    font.setPointSizeF(10.0f);
//    p.setFont(font);

    p.setPen(QColor(255, 255, 255));
//    p.setPen(Qt::NoPen);

//    p.setBrush(QBrush(QColor(255, 255, 255)));
    p.setBrush(Qt::NoBrush);

    QSize text_size(pixel_size.width() * 0.95f, pixel_size.height() * 0.95f);

//    QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), text_alignment_flags, QString::fromStdString(label._text)).size();

//    font.setPointSizeF(10.0f * text_size.height() / float(b_size.height())); // always use height ratio to ensure same size text for buttons of same height
//    font.setPointSizeF(10.0f * get_scale(b_size, text_size));
//    font.setPointSizeF(10.0f * get_scale(pixel_size, text_size));
//    QFont font; // = _main_font;
    font.setPixelSize(text_size.height());
//    font.setBold(true);
    font.setWeight(63);
    p.setFont(font);

    p.drawText(img.rect(), text_alignment_flags, QString::fromStdString(label._text));

    p.end();

    GL_functions f;
    f.init();

    Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(img);
    return f.create_texture(texture_fb, false);
}


void Info_label::set_text(std::string const& text, QFont const& font)
{
    _text = text;
    glDeleteTextures(1, &_texture_id);
    _texture_id = generate_info_label_texture(*this, Qt::AlignLeft | Qt::AlignVCenter, font);
}
