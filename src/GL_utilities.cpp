#include "GL_utilities.h"

#include <iostream>
#include <cassert>
#include <QtDebug>

#ifdef WIN32
#include <Windows.h>
#endif

#include "Utilities.h"

void check_gl_error()
{
    GLenum error = glGetError();

    if (error != GL_NO_ERROR)
    {
        std::cout << "check_gl_error(): Error: " << error << std::endl;
        assert(false);
    }
}


QGLShaderProgram * init_program(QGLContext const* context, QString const& vertex_file, QString const& frag_file)
{
    std::cout << "Loading shader: " << vertex_file.toStdString() << " " << frag_file.toStdString() << std::endl;

    QString log;

    QGLShaderProgram * program = new QGLShaderProgram(context);

    program->addShaderFromSourceFile(QGLShader::Vertex, vertex_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << "init_program: " << vertex_file.toStdString() << " log: " << log.toStdString();
    }

    program->addShaderFromSourceFile(QGLShader::Fragment, frag_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << "init_program: " << frag_file.toStdString() << " log: " << log.toStdString();
    }

    program->link();

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << "init_program link: " << log.toStdString();
    }

    return program;
}


QOpenGLShaderProgram * init_program(QString const& vertex_file, QString const& frag_file)
{
    std::cout << "Loading shader (V, F): " << vertex_file.toStdString() << " " << frag_file.toStdString() << std::endl;

    QString log;

    QOpenGLShaderProgram * program = new QOpenGLShaderProgram();

    program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " " << vertex_file.toStdString() << " log: " << log.toStdString();
        qCritical() << "Vertex shader log: " << vertex_file << "\nlog: " << log;
    }

    program->addShaderFromSourceFile(QOpenGLShader::Fragment, frag_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " " << frag_file.toStdString() << " log: " << log.toStdString();
        qCritical() << "Fragment shader log: " << frag_file << "\nlog: " << log;
    }

    program->link();

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " link: " << log.toStdString();
        qCritical() << "Linking log: " << vertex_file << " "  << frag_file << " log: " << log;
    }

    return program;
}


QOpenGLShaderProgram *init_program(const QString &vertex_file, const QString &frag_file, const QString &geometry_file)
{
    std::cout << "Loading shader (V, F, G): " << vertex_file.toStdString() << " " << frag_file.toStdString() << std::endl;

    QString log;

    QOpenGLShaderProgram * program = new QOpenGLShaderProgram();

    program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " " << vertex_file.toStdString() << " log: " << log.toStdString();
        qCritical() << "Vertex shader log: " << vertex_file << "\nlog: " << log;
    }

    program->addShaderFromSourceFile(QOpenGLShader::Fragment, frag_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " " << frag_file.toStdString() << " log: " << log.toStdString();
        qCritical() << "Fragment shader log: " << frag_file << "\nlog: " << log;
    }

    program->addShaderFromSourceFile(QOpenGLShader::Geometry, geometry_file);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " " << geometry_file.toStdString() << " log: " << log.toStdString();
        qCritical() << "Geometry shader log: " << frag_file << "\nlog: " << log;
    }

    program->link();

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " link: " << log.toStdString();
        qCritical() << "Linking log: " << vertex_file << " "  << frag_file << " log: " << log;
    }

    return program;
}


QOpenGLShaderProgram * init_program_from_code(QString const& vertex_code, QString const& frag_code)
{
    QString log;

    QOpenGLShaderProgram * program = new QOpenGLShaderProgram();

    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_code);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " Vertex shader log: " << log.toStdString();
        qCritical() << "Vertex shader log: " << log;
    }

    program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_code);

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " Fragment shader log: " << log.toStdString();
        qCritical() << "Fragment shader log: " << log;
    }

    program->link();

    log = program->log();

    if (!log.isEmpty())
    {
        std::cout << __func__ << " link: " << log.toStdString();
        qCritical() << "Linking log: " << log;
    }

    return program;
}


GL_mesh2::GL_mesh2(MyMesh const& mesh, bool const store_mesh)
{
    init(mesh, store_mesh);
}


void GL_mesh2::init(MyMesh const& mesh, bool const store_mesh)
{
    initializeOpenGLFunctions();

    if (store_mesh)
    {
        _mesh = mesh;
    }

    _n_faces = int(mesh.n_faces());
    _n_vertices = int(mesh.n_vertices());

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(5, _buffers);

    glBindBuffer(GL_ARRAY_BUFFER, _buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh.n_vertices() * 3 * sizeof(float), mesh.points(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (mesh.has_vertex_normals())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _buffers[1]);
        glBufferData(GL_ARRAY_BUFFER, mesh.n_vertices() * 3 * sizeof(float), mesh.vertex_normals(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (mesh.has_vertex_texcoords2D())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _buffers[2]);
        glBufferData(GL_ARRAY_BUFFER, mesh.n_vertices() * 2 * sizeof(float), mesh.texcoords2D(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (mesh.has_vertex_colors())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _buffers[3]);
        glBufferData(GL_ARRAY_BUFFER, mesh.n_vertices() * 3 * sizeof(GLubyte), mesh.vertex_colors(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[4]);

    std::vector<GLuint> indices;

    MyMesh::ConstFaceIter
            fIt(mesh.faces_begin()),
            fEnd(mesh.faces_end());

    int num_faces = 0;

    for (; fIt != fEnd; ++fIt)
    {
        MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(*fIt);
        indices.push_back(fvIt->idx());
        ++fvIt;
        indices.push_back(fvIt->idx());
        ++fvIt;
        indices.push_back(fvIt->idx());

        ++num_faces;
    }

    std::cout << __func__ << " num_faces: " << num_faces << " " << indices.size() << std::endl;

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL_mesh2::bind_vao()
{
    glBindVertexArray(_vao);
}

void GL_mesh2::release_vao()
{
    glBindVertexArray(0);
}


void GL_mesh2::draw()
{
//    std::cout << __PRETTY_FUNCTION__ << " " << QOpenGLContext::currentContext() << std::endl;

    bind_vao();
    glDrawElements(GL_TRIANGLES, _n_faces * 3, GL_UNSIGNED_INT, 0);
    release_vao();
}

void GL_mesh2::draw_points()
{
    bind_vao();
    glDrawArrays(GL_POINTS, 0, _n_vertices);
    release_vao();
}

void GL_mesh2::set_vertex_colors(GLfloat *data_ptr)
{
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[3]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, _n_vertices * 3 * sizeof(float), data_ptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool GL_mesh2::have_mesh() const
{
    return _mesh.is_initialized();
}

MyMesh const& GL_mesh2::get_mesh() const
{
    return *_mesh;
}

void GL_mesh2::setup_vertex_colors_buffer()
{
    bind_vao();

    glBindBuffer(GL_ARRAY_BUFFER, _buffers[3]);
    glBufferData(GL_ARRAY_BUFFER, _n_vertices * 3 * sizeof(float), 0, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

    release_vao();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void GL_mesh2::draw_without_binding()
{
    glDrawElements(GL_TRIANGLES, _n_faces * 3, GL_UNSIGNED_INT, 0);
}


void GL_functions::init()
{
    bool const ok = initializeOpenGLFunctions();
    assert(ok);
}


GLuint GL_functions::create_texture(Frame_buffer<Color4> const& frame, bool const use_mipmaps, const GLint internal_format, GLint const wrap_mode)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

    if (use_mipmaps)
    {
        int const num_mipmaps = 4;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_mipmaps);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, frame.get_width(), frame.get_height(), 0,
                     GL_RGBA, GL_FLOAT, frame.get_raw_data());

        glGenerateMipmap(GL_TEXTURE_2D);

//        my_glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, GL_RGBA32F, frame.get_width(), frame.get_height());
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.get_width(), frame.get_height(), GL_RGBA, GL_FLOAT, frame.get_raw_data());
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, frame.get_width(), frame.get_height(), 0,
                     GL_RGBA, GL_FLOAT, frame.get_raw_data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_index;
}


GLuint GL_functions::create_texture(Frame_buffer<Color> const& frame, bool const use_mipmaps, GLint const internal_format, GLint const wrap_mode)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

    if (use_mipmaps)
    {
        int const num_mipmaps = 4;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_mipmaps);

//        my_glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, GL_RGB32F, frame.get_width(), frame.get_height());
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.get_width(), frame.get_height(), GL_RGB, GL_FLOAT, frame.get_raw_data());

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, frame.get_width(), frame.get_height(), 0,
                     GL_RGB, GL_FLOAT, frame.get_raw_data());

        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, frame.get_width(), frame.get_height(), 0,
                     GL_RGB, GL_FLOAT, frame.get_raw_data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_index;
}

GLuint GL_functions::create_texture(QString const& filename, bool const with_alpha)
{
    QImage image(filename);

    if (with_alpha)
    {
        Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(image);
        return create_texture(texture_fb);
    }
    else
    {
        Frame_buffer<Color> texture_fb = convert<QColor_to_Color_converter, Color>(image);
        return create_texture(texture_fb);
    }
}

//GLuint GL_functions::create_texture(int const width, int const height, GLint internal_format, GLenum format, GLint interpolation)
//{
//    GLuint texture_index;

//    glGenTextures(1, &texture_index);

//    glBindTexture(GL_TEXTURE_2D, texture_index);

//    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
//                 format, GL_FLOAT, nullptr);

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);

//    glBindTexture(GL_TEXTURE_2D, 0);

//    return texture_index;
//}

//GLuint GL_functions::create_texture(const int width, const int height, const GLvoid *data, GLint const internal_format,
//                                    GLenum const format, GLenum const type, GLint const interpolation)
//{
//    //        assert(data.size() <= width * height);

//    GLuint texture_index;

//    glGenTextures(1, &texture_index);

//    glBindTexture(GL_TEXTURE_2D, texture_index);

//    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
//                 format, type, data);
//    //        format, GL_FLOAT, data.data());

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);

//    glBindTexture(GL_TEXTURE_2D, 0);

//    return texture_index;
//}

void GL_functions::delete_texture(GLuint const id)
{
    glDeleteTextures(1, &id);
}
