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
    glBindTexture(GL_TEXTURE_2D, _depth_tex);
    _blur_program->setUniformValue("clip_distances", QVector2D(camera->zNear(), camera->zFar()));
    _blur_program->setUniformValue("tex_size", screen_size);
    _blur_program->setUniformValue("focus_distance", float(camera->position()[1]));
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
