#ifndef GPU_FORCE_H
#define GPU_FORCE_H

#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QOpenGLFunctions_4_3_Core>

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

// GL_R32I internalFormat, single 32bit int on red channel

inline GLuint create_single_channel_texture(Frame_buffer<int> const& frame);

inline GLuint create_single_channel_texture(Frame_buffer<float> const& frame);

class GPU_force : public QOpenGLFunctions_4_3_Core
{
public:
    GPU_force(QGLContext *context);

    void init_vertex_data();

    std::vector<Eigen::Vector3f> calc_forces(std::list<Molecule> const& molecules);

private:
    std::unique_ptr<QGLFramebufferObject> _fbo;
    std::unique_ptr<QGLShaderProgram> _shader;

    int _max_num_atoms;
    int _size;

    Frame_buffer<Eigen::Vector3f> _position_frame;
    Frame_buffer<float> _charge_frame;
    Frame_buffer<float> _radius_frame;
    Frame_buffer<int> _parent_id_frame;

    GLuint _position_tex;
    GLuint _charge_tex;

    GLuint _buffer_square_positions;
//    GLuint _buffer_square_tex_coords;

//    GLuint _position_texture;
//    GLuint _charge_texture;
//    GLuint _id_texture;
};

#endif // GPU_FORCE_H
