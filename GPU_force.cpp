#include "GPU_force.h"




GLuint create_single_channel_texture(const Frame_buffer<int> &frame)
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


GLuint create_single_channel_float_texture(int const size)
{
    GLuint texture_index;

    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size, size, 0,
                 GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_index;
}


GPU_force::GPU_force(QGLContext * context)
{
    initializeOpenGLFunctions();

    _max_num_atoms = 100 * 100;
    _size = std::sqrt(_max_num_atoms);

    _fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(_size, _size, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA));
    glBindTexture(GL_TEXTURE_2D, _fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    _position_frame = Frame_buffer<Eigen::Vector3f>(_size, _size);
    _charge_frame = Frame_buffer<float>(_size, _size);
//    _radius_frame = Frame_buffer<float>(_size, _size);
//    _parent_id_frame = Frame_buffer<int>(_size, _size);

    _position_tex = create_three_channel_float_texture(_size);
    _charge_tex   = create_single_channel_float_texture(_size);

    _shader = std::unique_ptr<QGLShaderProgram>(init_program(context, Data_config::get_instance()->get_absolute_qfilename("shaders/force_calc.vert"), Data_config::get_instance()->get_absolute_qfilename("shaders/force_calc.frag")));

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

//    std::vector<float> tex_coords = {
//        0.0f, 0.0f,
//        1.0f, 0.0f,
//        1.0f, 1.0f,
//        0.0f, 1.0f
//    };

    glGenBuffers(1, &_buffer_square_positions);

    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    glGenBuffers(1, &_buffer_square_tex_coords);

//    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_tex_coords);
//    glBufferData(GL_ARRAY_BUFFER, tex_coords.size() * sizeof(float), tex_coords.data(), GL_STATIC_DRAW);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::vector<Eigen::Vector3f> GPU_force::calc_forces(std::list<Molecule> const& molecules)
{
    // go over all atoms (inside the molecules) and store their data in textures

    int num_atoms = 0;

    for (Molecule const& sender : molecules)
    {
        for (Atom const& sender_atom : sender._atoms)
        {
            _position_frame.set_data(num_atoms, sender_atom.get_position());
            _charge_frame.set_data(num_atoms, sender_atom._charge);
//            _radius_frame.set_data(num_atoms, sender_atom._radius);
//            _parent_id_frame.set_data(num_atoms, sender.get_id());

            ++num_atoms;
        }
    }

    assert(num_atoms < _max_num_atoms);

    glBindTexture(GL_TEXTURE_2D, _position_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, _size, GL_RGB, GL_FLOAT, _position_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, _charge_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _size, _size, GL_RED, GL_FLOAT, _charge_frame.get_raw_data());

    glBindTexture(GL_TEXTURE_2D, 0);

    glPushAttrib(GL_COLOR_BUFFER_BIT);

    _fbo->bind();
//            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fbo_tex, 0); // FIXME: possibly needs a nearest neighbor texture


    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);


    _shader->bind();
//    _shader->setUniformValue("scale", 1.0f);
//    _shader->setUniformValue("offset", QVector2D(0.0f, 0.0f));
    _shader->setUniformValue("tex_size", QVector2D(100, 100));
    _shader->setUniformValue("num_atoms", num_atoms);

    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_positions);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

//    glBindBuffer(GL_ARRAY_BUFFER, _buffer_square_tex_coords);
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _position_tex);
    _shader->setUniformValue("pos_tex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _charge_tex);
    _shader->setUniformValue("charge_tex", 1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    _shader->release();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    glDisableVertexAttribArray(1);

    _fbo->release();

//    std::cout << __func__ << " path: " << (QDir::tempPath() + "/" + QString("force_calc.png")).toStdString() << std::endl;
//    _fbo->toImage().save(QDir::tempPath() + "/" + QString("force_calc.png"));

    std::vector<Eigen::Vector3f> resulting_forces;
    resulting_forces.reserve(num_atoms);

    Frame_buffer<Eigen::Vector4f> result(_size, _size);

    glBindTexture(GL_TEXTURE_2D, _fbo->texture());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, result.get_raw_data());
    glBindTexture(GL_TEXTURE_2D, 0);

    glPopAttrib();

    for (size_t i = 0; i < result.get_size(); ++i)
    {
        resulting_forces.push_back(Eigen::Vector3f(result.get_data(i)[0], result.get_data(i)[1], result.get_data(i)[2]));
    }

    return resulting_forces;
}

