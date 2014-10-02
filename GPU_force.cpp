#include "GPU_force.h"




GLuint create_single_channel_int_texture(int const size)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, size, size, 0,
                 GL_RED, GL_INT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_index;
}


GLuint create_single_channel_float_texture(int const size, GLint const interpolation_type = GL_NEAREST)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size, size, 0,
                 GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation_type);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation_type);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_index;
}


GLuint create_three_channel_float_texture(int const size)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size, size, 0,
                 GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_index;
}


GPU_force::GPU_force(QGLContext * context, int const temperature_grid_size)
{
    initializeOpenGLFunctions();

    _size = 64;
    _max_num_atoms = _size * _size;

    _fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(_size, _size, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA));
    glBindTexture(GL_TEXTURE_2D, _fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    _position_frame = Frame_buffer<Eigen::Vector3f>(_size, _size);
    _charge_frame = Frame_buffer<float>(_size, _size);
    _radius_frame = Frame_buffer<float>(_size, _size);
    _parent_id_frame = Frame_buffer<float>(_size, _size);

    _fbo_tex = create_three_channel_float_texture(_size);
    _position_tex = create_three_channel_float_texture(_size);
    _charge_tex   = create_single_channel_float_texture(_size);
    _radius_tex   = create_single_channel_float_texture(_size);
//    _parent_id_tex = create_single_channel_int_texture(_size);
    _parent_id_tex = create_single_channel_float_texture(_size);
    _temperature_tex = create_single_channel_float_texture(temperature_grid_size, GL_LINEAR);

    _shader = std::unique_ptr<QGLShaderProgram>(init_program(context, Data_config::get_instance()->get_absolute_qfilename("shaders/force_calc.vert"), Data_config::get_instance()->get_absolute_qfilename("shaders/force_calc.frag")));

    _result_fb = Frame_buffer<Eigen::Vector3f>(_size, _size);
    _resulting_forces.resize(_size * _size);

    init_vertex_data();
}

void GPU_force::init_vertex_data()
{
    std::vector<float> vertices = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };

    glGenBuffers(1, &_buffer_square_positions);

    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::vector<Eigen::Vector3f> const& GPU_force::calc_forces(std::list<Molecule> const& molecules,
                                                           const float coulomb_factor, const float vdw_factor, const float vdw_radius,
                                                           const float time, QVector2D const& bounding_box_size)
{
    glDisable(GL_BLEND);

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


    int const needed_height = (num_atoms / _size) + 1;
//    int const needed_height = _size;

    glBindTexture(GL_TEXTURE_2D, _position_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, needed_height, GL_RGB, GL_FLOAT, _position_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, _charge_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, needed_height, GL_RED, GL_FLOAT, _charge_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, _radius_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, needed_height, GL_RED, GL_FLOAT, _radius_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, _parent_id_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, needed_height, GL_RED, GL_FLOAT, _parent_id_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, 0);

    glPushAttrib(GL_COLOR_BUFFER_BIT);

    _fbo->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fbo_tex, 0); // FIXME: possibly needs a nearest neighbor texture

    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    assert(_fbo->isValid());

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);


    _shader->bind();
    _shader->setUniformValue("tex_size", QSize(_size, _size));
    _shader->setUniformValue("num_atoms", num_atoms);
    _shader->setUniformValue("time", time);
    _shader->setUniformValue("bounding_box_size", 0.5f * bounding_box_size);

    _shader->setUniformValue("coulomb_factor", coulomb_factor);
    _shader->setUniformValue("vdw_factor", vdw_factor);
    _shader->setUniformValue("vdw_radius_factor", vdw_radius);


    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _position_tex);
    _shader->setUniformValue("pos_tex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _charge_tex);
    _shader->setUniformValue("charge_tex", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _radius_tex);
    _shader->setUniformValue("radius_tex", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _parent_id_tex);
    _shader->setUniformValue("parent_id_tex", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _temperature_tex);
    _shader->setUniformValue("temperature_tex", 4);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    _shader->release();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glReadPixels(0, 0, _size, needed_height, GL_RGB, GL_FLOAT, _result_fb.get_raw_data());

    _fbo->release();

    glPopAttrib();

    return _result_fb.get_data();
}

void GPU_force::update_temperature_tex(Frame_buffer<float> temperature_grid)
{
    glBindTexture(GL_TEXTURE_2D, _temperature_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, temperature_grid.get_width(), temperature_grid.get_height(), GL_RED, GL_FLOAT, temperature_grid.get_raw_data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

