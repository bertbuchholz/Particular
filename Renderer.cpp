#include <Renderer.h>

void setup_gl_points(const bool distance_dependent)
{
    if (distance_dependent)
    {

        float quadratic[] =  { 0.0f, 0.0f, 0.001f };
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION, quadratic);

        //        glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

        //        //Tell it the max and min sizes we can use using our pre-filled array.
    }
    else
    {
        float quadratic[] =  { 1.0f, 0.0f, 0.0f };
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION, quadratic);
//        glPointParameterf(GL_POINT_SIZE_MIN, 1.0f);
//        glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);
//        glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
//        glPointSize(8.0f);

    }

    glDisable(GL_POINT_SPRITE);
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
    glPointParameterf(GL_POINT_SIZE_MIN, 2.0f);
    glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);

    glEnable(GL_POINT_SMOOTH);
}


void draw_cube()
{
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, -1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -1.0f);
    glRotatef(180.0f, 0.0f, -1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();
}


Shader_renderer::~Shader_renderer()
{
    glDeleteBuffers(1, &_ice_texture);
    glDeleteBuffers(1, &_backdrop_texture);
    glDeleteBuffers(1, &_blurred_backdrop_texture);
    glDeleteBuffers(1, &_background_grid_texture);
    glDeleteBuffers(2, _tmp_screen_texture);
    glDeleteBuffers(1, &_depth_texture);
}

void Shader_renderer::init(const QGLContext *context, const QSize &size)
{
    initializeGLFunctions(context);

    _molecule_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/simple.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/molecule.frag")));
    _temperature_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.frag")));
    _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/simple_texture.frag")));
    _post_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/post.frag")));
    _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/depth_blur_1D.frag")));

    _sphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/icosphere_3.obj"));
    _grid_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/grid_10x10.obj"));
    _bg_hemisphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/bg_hemisphere.obj"));

    typename MyMesh::ConstVertexIter vIt(_grid_mesh.vertices_begin()), vEnd(_grid_mesh.vertices_end());

    for (; vIt!=vEnd; ++vIt)
    {
        MyMesh::Point const& v = _grid_mesh.point(vIt.handle());

        MyMesh::TexCoord2D t(v[0] * 0.5f + 0.5f, v[2] * 0.5f + 0.5f);
        _grid_mesh.set_texcoord2D(vIt.handle(), t);
    }

    Frame_buffer<Color> ice_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/ice_texture.png")));
    _ice_texture = create_texture(ice_tex_fb);

    Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/iss_interior_1.png")));
    _backdrop_texture = create_texture(backdrop_tex_fb);

    Frame_buffer<Color> blurred_backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/iss_interior_1_blurred.png")));
    _blurred_backdrop_texture = create_texture(blurred_backdrop_tex_fb);

    Frame_buffer<Color> backdrop_grid_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/background_grid.png")));
    _background_grid_texture = create_texture(backdrop_grid_tex_fb);

    resize(size);

    _level_element_draw_visitor.init(context, size);
    _level_element_ui_draw_visitor.init(context);
}

void Shader_renderer::resize(const QSize &size)
{
    //        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));

    glGenTextures(1, &_depth_texture); // FIXME: need to delete first
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.width(), size.height(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // FIXME: need to delete first
    _tmp_screen_texture[0] = create_texture(size.width(), size.height());
    _tmp_screen_texture[1] = create_texture(size.width(), size.height());

    _scene_fbo->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
    _scene_fbo->release();

    _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

    _level_element_draw_visitor.resize(size);
}

void Shader_renderer::update(const Level_data &level_data)
{
    glDeleteTextures(1, &_backdrop_texture);
    QImage background(Data_config::get_instance()->get_absolute_qfilename("textures/" + QString::fromStdString(level_data._background_name)));
    Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(background);
    _backdrop_texture = create_texture(backdrop_tex_fb);

    QImage blurred_background = background.scaled(background.size() * 0.05f);
    Frame_buffer<Color> blurred_backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(blurred_background);
    _blurred_backdrop_texture = create_texture(blurred_backdrop_tex_fb);
}

void Shader_renderer::draw_atom(const Atom &atom, const float scale, const float alpha)
{
    float radius = scale * atom._radius;

    Color4 color(Atom::atom_colors[int(atom._type)], alpha);

    if (atom._type == Atom::Type::Charge)
    {
        radius = 0.3f;
    }

    glm::mat4x4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, glm::vec3(atom.get_position()[0], atom.get_position()[1], atom.get_position()[2]));
    model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, radius));

    glUniformMatrix4fv(_molecule_program->uniformLocation("m_model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

    glUniform4fv(_molecule_program->uniformLocation("color"), 1, color.data());

    draw_mesh(_sphere_mesh);
}

void Shader_renderer::draw_molecule(const Molecule &molecule, const float scale, const float alpha)
{
    for (Atom const& atom : molecule._atoms)
    {
        draw_atom(atom, scale, alpha);
    }
}

float Shader_renderer::get_brownian_strength(const Eigen::Vector3f &pos, const std::vector<Brownian_element *> &elements, const float general_temperature) const
{
    float factor = general_temperature;

    for (Brownian_element const* element : elements)
    {
        factor += element->get_brownian_motion_factor(pos);
    }

    float const max_strength = 50.0f;

    float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

    return strength;
}

void Shader_renderer::draw_temperature_mesh(const MyMesh &mesh, const Level_data &level_data, const GLuint bg_texture, const QSize &screen_size, const float time)
{
    if (level_data._game_field_borders.size() != 6)
    {
        std::cout << __PRETTY_FUNCTION__ << " no game field borders or too many/few, not drawing temperature mesh" << std::endl;
        return;
    }

    float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

    glDisable(GL_DEPTH_TEST);

    _temperature_program->bind();

    _temperature_program->setUniformValue("ice_texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _ice_texture);

    _temperature_program->setUniformValue("scene_texture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bg_texture);

    _temperature_program->setUniformValue("screen_size", screen_size);
    _temperature_program->setUniformValue("time", time);

    //        glColor3f(0.5f, 0.5f, 0.5f);
    //        draw_backdrop_quad();

    std::vector<Brownian_element*> const& elements = level_data._brownian_elements;

    auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

    assert(front_face_iter != level_data._game_field_borders.end());

    Plane_barrier const* front_face = front_face_iter->second;
    Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

    typename MyMesh::ConstFaceIter fIt(mesh.faces_begin()), fEnd(mesh.faces_end());

    glBegin(GL_TRIANGLES);
    for (; fIt!=fEnd; ++fIt)
    {
        typename MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());

        Eigen::Vector3f p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
        ++fvIt;
        p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
        ++fvIt;
        p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
    }
    glEnd();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    _temperature_program->release();
}

void Shader_renderer::draw_temperature(const Level_data &level_data) const
{
    auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

    assert(front_face_iter != level_data._game_field_borders.end());

    float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

    Plane_barrier const* front_face = front_face_iter->second;
    Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

    Eigen::Vector3f grid_start = front_face->get_position() - extent;
    Eigen::Vector3f grid_end = front_face->get_position() + extent;

    float resolution = 1.0f;

    std::cout << __PRETTY_FUNCTION__ << " " << grid_start << " " << grid_end << std::endl;

    for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
    {
        for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
        {
            Eigen::Vector3f pos(x, front_face->get_position()[1], z);

            float const strength = get_brownian_strength(pos, level_data._brownian_elements, general_temperature);

            Color c(strength, 0.0f, 1.0f - strength);

            glColor3fv(c.data());

            glBegin(GL_POINTS);
            glVertex3fv(pos.data());
            glEnd();
        }
    }
}

void Shader_renderer::draw_level_elements(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Shader_renderer::draw_particle_systems(const Level_data &level_data) const
{
    for (Particle_system_element * element : level_data._particle_system_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Shader_renderer::draw_elements_ui(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_ui_draw_visitor);
    }
}

void Shader_renderer::draw_backdrop_quad() const
{
    glPushMatrix();

    glTranslatef(0.0f, 300.0f, 0.0f);

    glScalef(300.0f, 1.0f, 300.0f);

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    draw_quad_with_tex_coords();

    glPopMatrix();
}

void Shader_renderer::render(QGLFramebufferObject *main_fbo, const Level_data &level_data, const float time, const qglviewer::Camera *camera)
{
    glEnable(GL_DEPTH_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    QSize screen_size(camera->screenWidth(), camera->screenHeight());

    _scene_fbo->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    assert(_scene_fbo->isValid());

    // draw the complete scene into an FB

    //    glDisable(GL_TEXTURE_2D);

    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);
    glDisable(GL_LIGHTING);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _background_grid_texture);
    //            draw_backdrop_quad();

    glPushMatrix();
    //    glTranslatef(0.0f, 200.0f, 0.0f);
    glScalef(200.0f, 200.0f, 200.0f);
    //            draw_mesh(_bg_hemisphere_mesh);
    draw_cube();
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    std::list<Molecule> const& molecules = level_data._molecules;

    _molecule_program->bind();
    {
        GLdouble m_projection[16];
        glGetDoublev(GL_PROJECTION_MATRIX, m_projection);

        GLdouble m_view[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, m_view);

        _molecule_program->setUniformValue("m_projection", QMatrix4x4(m_projection).transposed());
        _molecule_program->setUniformValue("m_view", QMatrix4x4(m_view).transposed());
        _molecule_program->setUniformValue("light_pos", QVector3D(-5.0f, 5.0f, 5.0f));
        _molecule_program->setUniformValue("camera_pos", QVector3D(camera->position()[0], camera->position()[1], camera->position()[2]));
        _molecule_program->setUniformValue("bg_texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _blurred_backdrop_texture);

        for (Molecule const& molecule : molecules)
        {
            draw_molecule(molecule, _scale);
        }
    }
    _molecule_program->release();

    glDisable(GL_TEXTURE_2D);

    draw_level_elements(level_data);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    draw_particle_systems(level_data);

    _scene_fbo->release();

    _post_fbo->bind();


    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[0], 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _blur_program->bind();
    _blur_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _scene_fbo->texture());
    _blur_program->setUniformValue("depth_texture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
    _blur_program->setUniformValue("clip_distances", QVector2D(camera->zNear(), camera->zFar()));
    _blur_program->setUniformValue("tex_size", screen_size);
//    _blur_program->setUniformValue("focus_distance", float(camera->position()[1]));
    _blur_program->setUniformValue("focus_distance", float(camera->position().norm()));
    _blur_program->setUniformValue("direction", QVector2D(1.0, 0.0));

    draw_quad_with_tex_coords();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[1], 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _blur_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[0]);
    _blur_program->setUniformValue("direction", QVector2D(0.0, 1.0));
    draw_quad_with_tex_coords();

    _blur_program->release();

    _post_fbo->release();


//    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
//                                          _post_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
//                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    return;


    //        draw_temperature(level_data);

    // distort/"freeze" the scene texture by using the temperature on the front game field plane

    _temperature_fbo->bind();


    QGLFramebufferObject::blitFramebuffer(_temperature_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _post_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_temperature_mesh(_grid_mesh, level_data, _tmp_screen_texture[1], screen_size, time);

    _temperature_fbo->release();

    main_fbo->bind();

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glDisable(GL_DEPTH_TEST);
    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    _screen_quad_program->bind();
    _screen_quad_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _temperature_fbo->texture());
    draw_quad_with_tex_coords();
    _screen_quad_program->release();

    glPopAttrib();

    // put the depth buffer from the scene drawing into the returned buffer to draw the ui elements correctly
    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    draw_elements_ui(level_data);

    glEnable(GL_DEPTH_TEST);

    main_fbo->release();
}


Editor_renderer::~Editor_renderer()
{
    glDeleteBuffers(1, &_ice_texture);
    glDeleteBuffers(1, &_backdrop_texture);
    glDeleteBuffers(2, _tmp_screen_texture);
    glDeleteBuffers(1, &_depth_texture);
}

void Editor_renderer::init(const QGLContext *context, const QSize &size)
{
    initializeGLFunctions(context);

    _molecule_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/simple.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/molecule.frag")));
    _temperature_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.frag")));
    _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/simple_texture.frag")));
    _post_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/post.frag")));
    _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/depth_blur_1D.frag")));

    _sphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/icosphere_3.obj"));
    _grid_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/grid_10x10.obj"));

    typename MyMesh::ConstVertexIter vIt(_grid_mesh.vertices_begin()), vEnd(_grid_mesh.vertices_end());

    for (; vIt!=vEnd; ++vIt)
    {
        MyMesh::Point const& v = _grid_mesh.point(vIt.handle());

        MyMesh::TexCoord2D t(v[0] * 0.5f + 0.5f, v[2] * 0.5f + 0.5f);
        _grid_mesh.set_texcoord2D(vIt.handle(), t);
    }

    Frame_buffer<Color> ice_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/ice_texture.png")));
    _ice_texture = create_texture(ice_tex_fb);

    Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/iss_interior_1.png")));
    _backdrop_texture = create_texture(backdrop_tex_fb);

    resize(size);

    _level_element_draw_visitor.init(context, size);
    _level_element_ui_draw_visitor.init(context);
}

void Editor_renderer::resize(const QSize &size)
{
    //        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));


    glGenTextures(1, &_depth_texture); // FIXME: need to delete first
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.width(), size.height(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // FIXME: need to delete first
    _tmp_screen_texture[0] = create_texture(size.width(), size.height());
    _tmp_screen_texture[1] = create_texture(size.width(), size.height());

    _scene_fbo->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
    _scene_fbo->release();

    _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

    _level_element_draw_visitor.resize(size);
}

void Editor_renderer::draw_atom(const Atom &atom, const float scale, const float alpha)
{
    float radius = scale * atom._radius;

    Color4 color(Atom::atom_colors[int(atom._type)], alpha);

    if (atom._type == Atom::Type::Charge)
    {
        radius = 0.3f;
    }

    glm::mat4x4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, glm::vec3(atom.get_position()[0], atom.get_position()[1], atom.get_position()[2]));
    model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, radius));

    glUniformMatrix4fv(_molecule_program->uniformLocation("m_model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

    glUniform4fv(_molecule_program->uniformLocation("color"), 1, color.data());

    draw_mesh(_sphere_mesh);
}

void Editor_renderer::draw_molecule(const Molecule &molecule, const float scale, const float alpha)
{
    for (Atom const& atom : molecule._atoms)
    {
        draw_atom(atom, scale, alpha);
    }
}

float Editor_renderer::get_brownian_strength(const Eigen::Vector3f &pos, const std::vector<Brownian_element *> &elements, const float general_temperature) const
{
    float factor = general_temperature;

    for (Brownian_element const* element : elements)
    {
        factor += element->get_brownian_motion_factor(pos);
    }

    float const max_strength = 50.0f;

    float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

    return strength;
}

void Editor_renderer::draw_temperature_mesh(const MyMesh &mesh, const Level_data &level_data, const GLuint bg_texture, const QSize &screen_size, const float time)
{
    if (level_data._game_field_borders.size() != 6)
    {
        std::cout << __PRETTY_FUNCTION__ << " no game field borders or too many/few, not drawing temperature mesh: " << level_data._game_field_borders.size() << std::endl;
        return;
    }

    float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

    glDisable(GL_DEPTH_TEST);

    _temperature_program->bind();

    _temperature_program->setUniformValue("ice_texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _ice_texture);

    _temperature_program->setUniformValue("scene_texture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bg_texture);

    _temperature_program->setUniformValue("screen_size", screen_size);
    _temperature_program->setUniformValue("time", time);

    std::vector<Brownian_element*> const& elements = level_data._brownian_elements;

    auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

    assert(front_face_iter != level_data._game_field_borders.end());

    Plane_barrier const* front_face = front_face_iter->second;
    Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

    typename MyMesh::ConstFaceIter fIt(mesh.faces_begin()), fEnd(mesh.faces_end());

    glBegin(GL_TRIANGLES);
    for (; fIt!=fEnd; ++fIt)
    {
        typename MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());

        Eigen::Vector3f p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
        ++fvIt;
        p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
        ++fvIt;
        p = OM2Eigen(mesh.point(fvIt.handle()));
        p = p.cwiseProduct(extent) + front_face->get_position();
        //            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
        glVertex3fv(p.data());
    }
    glEnd();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    _temperature_program->release();
}

void Editor_renderer::draw_temperature(const Level_data &level_data) const
{
    auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

    assert(front_face_iter != level_data._game_field_borders.end());

    float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

    Plane_barrier const* front_face = front_face_iter->second;
    Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

    Eigen::Vector3f grid_start = front_face->get_position() - extent;
    Eigen::Vector3f grid_end = front_face->get_position() + extent;

    float resolution = 1.0f;

    std::cout << __PRETTY_FUNCTION__ << " " << grid_start << " " << grid_end << std::endl;

    for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
    {
        for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
        {
            Eigen::Vector3f pos(x, front_face->get_position()[1], z);

            float const strength = get_brownian_strength(pos, level_data._brownian_elements, general_temperature);

            Color c(strength, 0.0f, 1.0f - strength);

            glColor3fv(c.data());

            glBegin(GL_POINTS);
            glVertex3fv(pos.data());
            glEnd();
        }
    }
}

void Editor_renderer::draw_level_elements(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Editor_renderer::draw_particle_systems(const Level_data &level_data) const
{
    for (Particle_system_element * element : level_data._particle_system_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Editor_renderer::draw_elements_ui(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_ui_draw_visitor);
    }
}

void Editor_renderer::draw_backdrop_quad() const
{
    glPushMatrix();

    glTranslatef(0.0f, 300.0f, 0.0f);

    glScalef(300.0f, 1.0f, 300.0f);

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    draw_quad_with_tex_coords();

    glPopMatrix();
}

void Editor_renderer::render(QGLFramebufferObject *main_fbo, const Level_data &level_data, const float time, const qglviewer::Camera *camera)
{
    glEnable(GL_DEPTH_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    QSize screen_size(camera->screenWidth(), camera->screenHeight());

    _scene_fbo->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    assert(_scene_fbo->isValid());

    glDisable(GL_TEXTURE_2D);

    //        glDisable(GL_LIGHTING);

    //        glColor3f(1.0f, 1.0f, 1.0f);

    //        draw_backdrop_quad();

    //        glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_LIGHTING);

    std::list<Molecule> const& molecules = level_data._molecules;

    _molecule_program->bind();
    {
        GLdouble m_projection[16];
        glGetDoublev(GL_PROJECTION_MATRIX, m_projection);

        GLdouble m_view[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, m_view);

        _molecule_program->setUniformValue("m_projection", QMatrix4x4(m_projection).transposed());
        _molecule_program->setUniformValue("m_view", QMatrix4x4(m_view).transposed());
        _molecule_program->setUniformValue("light_pos", QVector3D(-5.0f, 5.0f, 5.0f));
        _molecule_program->setUniformValue("camera_pos", QVector3D(camera->position()[0], camera->position()[1], camera->position()[2]));

        for (Molecule const& molecule : molecules)
        {
            draw_molecule(molecule, _scale);
        }
    }
    _molecule_program->release();

    draw_level_elements(level_data);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    draw_particle_systems(level_data);

    _scene_fbo->release();

    // distort/"freeze" the scene texture by using the temperature on the front game field plane

    _temperature_fbo->bind();

    QGLFramebufferObject::blitFramebuffer(_temperature_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_temperature_mesh(_grid_mesh, level_data, _scene_fbo->texture(), screen_size, time);

    _temperature_fbo->release();

    main_fbo->bind();

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glDisable(GL_DEPTH_TEST);
    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    _screen_quad_program->bind();
    _screen_quad_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _temperature_fbo->texture());
    draw_quad_with_tex_coords();
    _screen_quad_program->release();

    glPopAttrib();

    // put the depth buffer from the scene drawing into the returned buffer to draw the ui elements correctly
    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    draw_elements_ui(level_data);

    glEnable(GL_DEPTH_TEST);

    main_fbo->release();
}
