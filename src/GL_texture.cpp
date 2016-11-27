#include "GL_texture.h"

GL_texture::GL_texture() : _context(nullptr), _id(0)
{ }

GL_texture::GL_texture(QGLContext * context, GLuint const id) : _context(context), _id(id)
{ }

GL_texture::~GL_texture()
{
    _context->deleteTexture(_id);
}

void GL_texture::set_context(QGLContext * context)
{
    _context = context;
}

GL_texture & GL_texture::operator=(GL_texture const& other)
{
    _context->deleteTexture(_id);

    _context = other._context;
    _id = other._id;

    return *this;
}

void GL_texture::reset(GLuint const id)
{
    _context->deleteTexture(_id);

    _id = id;
}

int GL_texture::get_id() const
{
    return _id;
}
