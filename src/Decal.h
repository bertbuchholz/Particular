#ifndef DECAL_H
#define DECAL_H

#include <QOpenGLFunctions>
#include <QFont>
#include <Eigen/Core>

struct Decal
{
    Decal() : _texture_id(0), _tint({1.0f, 1.0f, 1.0f, 1.0f}), _bg_alpha(0.0f)
    { }

    enum Aspect_handling { Dont_keep, Keep_height, Keep_width };

    Eigen::Vector2f _size; // 0..1 of screen
    Eigen::Vector2f _screen_pos; // 0..1
    Eigen::Vector2i _tex_size;
    GLuint _texture_id;
    Aspect_handling _aspect;
    Eigen::Vector4f _tint;
    float _bg_alpha;
};

struct Info_label : public Decal
{
    std::string _text;

    void set_text(std::string const& text, QFont const& font);
};


//GLuint generate_info_label_texture(Info_label const& label, QSize const& screen_size, int const text_alignment_flags = Qt::AlignCenter, QFont font = QFont());


#endif // DECAL_H
