#ifndef GPU_FORCE_H
#define GPU_FORCE_H

#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QGLFunctions>

#include <Eigen/Core>

#include <Frame_buffer.h>
#include <GL_utilities.h>
#include <Draw_functions.h>

#include "Atom.h"
#include "Data_config.h"

// force calc using shader, each pixel == 1 atom
// textures with:
// atom positions, charge, parent id

// empty texture with resulting force on each atom

// frag shader for each atom needs to iterate over all pixels of the pos/charge texture
// calc forces for every atom which has not the same parent id

inline GLuint create_texture(Frame_buffer<Eigen::Vector3f> const& frame)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, frame.get_width(), frame.get_height(), 0,
                 GL_RGB, GL_FLOAT, frame.get_raw_data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_index;
}

// GL_R32I internalFormat, single 32bit int on red channel

inline GLuint create_single_channel_texture(Frame_buffer<int> const& frame)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, frame.get_width(), frame.get_height(), 0,
                 GL_RED, GL_FLOAT, frame.get_raw_data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_index;
}

inline GLuint create_single_channel_texture(Frame_buffer<float> const& frame)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, frame.get_width(), frame.get_height(), 0,
                 GL_RED, GL_FLOAT, frame.get_raw_data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_index;
}

class GPU_force : public QGLFunctions
{
public:
    GPU_force(QGLContext const* context)
    {
        _max_num_atoms = 100 * 100;
        _size = std::sqrt(_max_num_atoms);

        _fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(_size, _size, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA));

        _position_frame = Frame_buffer<Eigen::Vector3f>(_size, _size);
        _charge_frame = Frame_buffer<float>(_size, _size);
        _radius_frame = Frame_buffer<float>(_size, _size);
        _parent_id_frame = Frame_buffer<int>(_size, _size);

        _shader = std::unique_ptr<QGLShaderProgram>(init_program(context, Data_config::get_instance()->get_qdata_path() + "/shaders/force_calc.vert", Data_config::get_instance()->get_qdata_path() + "/shaders/force_calc.frag"));

        initializeGLFunctions(context);

        init_vertex_data();
    }

    void init_vertex_data()
    {
        std::vector<float> vertices = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f
        };

        std::vector<float> tex_coords = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        glGenBuffers(1, &_buffer_square_positions);

        glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &_buffer_square_tex_coords);

        glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_tex_coords);
        glBufferData(GL_ARRAY_BUFFER, tex_coords.size() * sizeof(float), tex_coords.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    std::vector<Eigen::Vector3f> calc_forces(std::vector<Molecule> const& molecules)
    {
        // go over all atoms (inside the molecules) and store their data in textures

        int num_atoms = 0;

        for (Molecule const& sender : molecules)
        {
            for (Atom const& sender_atom : sender._atoms)
            {
                _position_frame.set_data(num_atoms, sender_atom.get_position());
                _charge_frame.set_data(num_atoms, sender_atom._charge);
                _radius_frame.set_data(num_atoms, sender_atom._radius);
                _parent_id_frame.set_data(num_atoms, sender.get_id());

                ++num_atoms;
            }
        }

        assert(num_atoms < _max_num_atoms);

//        GLuint position_texture  = create_texture(_position_frame);
//        GLuint charge_texture    = create_single_channel_texture(_charge_frame);
//        GLuint radius_texture    = create_single_channel_texture(_radius_frame);
//        GLuint parent_id_texture = create_single_channel_texture(_parent_id_frame);

        glPushAttrib(GL_COLOR_BUFFER_BIT);

        _fbo->bind();
//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fbo_tex, 0); // FIXME: possibly needs a nearest neighbor texture


        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        _shader->bind();
        _shader->setUniformValue("scale", 1.0f);
        _shader->setUniformValue("offset", QVector2D(0.0f, 0.0f));

        glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_tex_coords);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        _shader->release();

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);




//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);

//        glViewport(0, 0, _size, _size);

//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glMatrixMode(GL_MODELVIEW);
//        glLoadIdentity();

//        glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

//        // set up shader

//        _shader->bind();

//        // draw plane

//        draw_quad_with_tex_coords();


//        _shader->release();

        _fbo->toImage().save(QString("/tmp/force_calc.png"));
        _fbo->release();


        std::vector<Eigen::Vector3f> resulting_forces;
        resulting_forces.reserve(num_atoms);

        glPopAttrib();

        return resulting_forces;
    }

private:
    std::unique_ptr<QGLFramebufferObject> _fbo;
    std::unique_ptr<QGLShaderProgram> _shader;

    int _max_num_atoms;
    int _size;

    Frame_buffer<Eigen::Vector3f> _position_frame;
    Frame_buffer<float> _charge_frame;
    Frame_buffer<float> _radius_frame;
    Frame_buffer<int> _parent_id_frame;

    GLuint _buffer_square_positions;
    GLuint _buffer_square_tex_coords;

//    GLuint _position_texture;
//    GLuint _charge_texture;
//    GLuint _id_texture;
};

#endif // GPU_FORCE_H
