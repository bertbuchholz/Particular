#ifndef GL_TEXTURE_H
#define GL_TEXTURE_H

#include "Draw_functions.h"

class GL_texture
{
public:
    GL_texture();
    GL_texture(QGLContext * context, GLuint const id);

    ~GL_texture();

    void set_context(QGLContext * context);

    void reset(GLuint const id);

    int get_id() const;

private:
    GL_texture & operator=(GL_texture const& other);

    GL_texture(GL_texture const& /* other */)
    { }

    QGLContext * _context;

    GLuint _id;
};

#endif // GL_TEXTURE_H
