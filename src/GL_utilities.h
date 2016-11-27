#ifndef GL_UTILITIES_H
#define GL_UTILITIES_H

#ifdef WIN32
#include <Windows.h>
#endif
#include <QGLShaderProgram>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <memory>

#ifndef Q_MOC_RUN
#include <boost/optional/optional.hpp>
#endif

#include "MyOpenMesh.h"
#include "Color.h"
#include "Frame_buffer.h"

void check_gl_error();

QGLShaderProgram * init_program(QGLContext const* context, QString const& vertex_file, QString const& frag_file);
QOpenGLShaderProgram * init_program(QString const& vertex_file, QString const& frag_file);
QOpenGLShaderProgram * init_program(QString const& vertex_file, QString const& frag_file, QString const& geometry_file);
QOpenGLShaderProgram * init_program_from_code(QString const& vertex_code, QString const& frag_code);

class GL_mesh : public QOpenGLFunctions_3_3_Core
{
public:
    GL_mesh() { }

    GL_mesh(MyMesh const& mesh, QGLContext const* context) : _mesh(mesh)
    {
        initializeOpenGLFunctions();

        uint vao;

        typedef void (*_glGenVertexArrays) (GLsizei, GLuint*);
//        typedef void (APIENTRY *_glGenVertexArrays) (GLsizei, GLuint*);
        typedef void (*_glBindVertexArray) (GLuint);
//        typedef void (APIENTRY *_glBindVertexArray) (GLuint);

        _glGenVertexArrays glGenVertexArrays;
        _glBindVertexArray glBindVertexArray;

        glGenVertexArrays = (_glGenVertexArrays) context->getProcAddress("glGenVertexArrays");
        glBindVertexArray = (_glBindVertexArray) context->getProcAddress("glBindVertexArray");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &_buffer_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, _buffer_vertices);
        glBufferData(GL_ARRAY_BUFFER, _mesh.n_vertices() * 3 * sizeof(float), _mesh.points(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (_mesh.has_vertex_normals())
        {
            glGenBuffers(1, &_buffer_normals);

            glBindBuffer(GL_ARRAY_BUFFER, _buffer_normals);
            glBufferData(GL_ARRAY_BUFFER, _mesh.n_vertices() * 3 * sizeof(float), _mesh.vertex_normals(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        if (_mesh.has_vertex_texcoords2D())
        {
            glGenBuffers(1, &_buffer_texcoords);

            glBindBuffer(GL_ARRAY_BUFFER, _buffer_texcoords);
            glBufferData(GL_ARRAY_BUFFER, _mesh.n_vertices() * 2 * sizeof(float), _mesh.texcoords2D(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        if (_mesh.has_vertex_colors())
        {
            glGenBuffers(1, &_buffer_colors);

            glBindBuffer(GL_ARRAY_BUFFER, _buffer_colors);
            glBufferData(GL_ARRAY_BUFFER, _mesh.n_vertices() * 3 * sizeof(GL_UNSIGNED_BYTE), _mesh.vertex_colors(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        glGenBuffers(1, &_buffer_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffer_indices);

        std::vector<GLuint> indices;

        MyMesh::ConstFaceIter
                fIt(_mesh.faces_begin()),
                fEnd(_mesh.faces_end());

        for (; fIt != fEnd; ++fIt)
        {
            MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(*fIt);
            indices.push_back(fvIt->idx());
            ++fvIt;
            indices.push_back(fvIt->idx());
            ++fvIt;
            indices.push_back(fvIt->idx());
        }

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }

    void draw()
    {
        glDrawElements(GL_TRIANGLES, int(_mesh.n_faces()) * 3, GL_UNSIGNED_INT, 0);
    }

    void bind(GLuint const vertices_location, GLuint const normals_location, GLuint const texcoords_location)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _buffer_vertices);
        glEnableVertexAttribArray(vertices_location);
        glVertexAttribPointer(vertices_location, 3, GL_FLOAT, GL_FALSE, 0, 0);

        if (_mesh.has_vertex_normals())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _buffer_normals);
            glEnableVertexAttribArray(normals_location);
            glVertexAttribPointer(normals_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }

        if (_mesh.has_vertex_texcoords2D())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _buffer_texcoords);
            glEnableVertexAttribArray(texcoords_location);
            glVertexAttribPointer(texcoords_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        }

        if (_mesh.has_vertex_colors())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _buffer_colors);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffer_indices);
    }

    void release(GLuint const vertices_location, GLuint const normals_location, GLuint const texcoords_location)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glDisableVertexAttribArray(vertices_location);
        glDisableVertexAttribArray(normals_location);
        glDisableVertexAttribArray(texcoords_location);
        glDisableVertexAttribArray(3);
    }


private:
    MyMesh _mesh;

//    GLuint _vao_object;
    GLuint _buffer_vertices;
    GLuint _buffer_normals;
    GLuint _buffer_texcoords;
    GLuint _buffer_colors;
    GLuint _buffer_indices;
};


class GL_mesh2 : public QOpenGLFunctions_3_3_Core
{
public:
    GL_mesh2() { }

    GL_mesh2(MyMesh const& mesh, const bool store_mesh = false);

    void init(MyMesh const& mesh, const bool store_mesh = false);

    void bind_vao();
    void release_vao();

    void draw();
    void draw_without_binding();
    void draw_points();

    bool have_mesh() const;
    MyMesh const& get_mesh() const;

    void setup_vertex_colors_buffer();
    void set_vertex_colors(GLfloat * data_ptr);

private:
    GLuint _vao;
    GLuint _buffers[5];

    boost::optional<MyMesh> _mesh;

    int _n_faces;
    int _n_vertices;
};


class GL_functions : public QOpenGLFunctions_3_3_Core
{
public:
    void init();

    GLuint create_texture(Frame_buffer<Color4> const& frame, bool const use_mipmaps = true, GLint const internal_format = GL_RGBA, GLint const wrap_mode = GL_REPEAT);
    GLuint create_texture(Frame_buffer<Color> const& frame, bool const use_mipmaps = true, GLint const internal_format = GL_RGB, GLint const wrap_mode = GL_REPEAT);
    GLuint create_texture(QString const& filename, bool const with_alpha = true);
//    GLuint create_texture(int const width, int const height, GLint internal_format = GL_RGBA32F, GLenum format = GL_RGBA, GLint interpolation = GL_NEAREST);

//    GLuint create_texture(int const width, int const height, GLvoid const* data, GLint const internal_format = GL_RGBA32F,
//                          GLenum const format = GL_RGBA, GLenum const type = GL_FLOAT, GLint const interpolation = GL_NEAREST);

    template <typename T>
    GLuint create_texture(int const width, int const height, std::vector<T> const& data = std::vector<T>(), GLint const internal_format = GL_RGBA32F,
                          GLenum const format = GL_RGBA, GLenum const type = GL_FLOAT, GLint const interpolation = GL_NEAREST, GLint const wrap_mode = GL_REPEAT)
    {
        assert(data.empty() || int(data.size()) == width * height);

        GLuint texture_index;

        glGenTextures(1, &texture_index);

        glBindTexture(GL_TEXTURE_2D, texture_index);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
                     format, type, data.empty() ? nullptr : data.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

        glBindTexture(GL_TEXTURE_2D, 0);

        return texture_index;
    }

    template <typename T>
    GLuint create_texture_1D(int const width, std::vector<T> const& data, GLint const internal_format = GL_RGBA32F,
                          GLenum const format = GL_RGBA, GLenum const type = GL_FLOAT, GLint const interpolation = GL_NEAREST, GLint const wrap_mode = GL_REPEAT)
    {
        assert(data.empty() || int(data.size()) == width);

        GLuint texture_index;

        glGenTextures(1, &texture_index);

        glBindTexture(GL_TEXTURE_1D, texture_index);

        glTexImage1D(GL_TEXTURE_1D, 0, internal_format, width, 0,
                     format, type, data.empty() ? nullptr : data.data());

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, interpolation);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap_mode);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, wrap_mode);

        glBindTexture(GL_TEXTURE_1D, 0);

        return texture_index;
    }

    void delete_texture(GLuint const id);
};



#endif // GL_UTILITIES_H
