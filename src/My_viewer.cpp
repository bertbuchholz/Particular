#include "My_viewer.h"

#include "Experiment_screen.h"
#include "Main_menu_screen.h"
#include "Main_options_window.h"
#include "Help_screen.h"


class Game_camera_constraint : public qglviewer::Constraint
{
public:
    Game_camera_constraint(float const box_size_half) : _box_size_half(box_size_half)
    { }

    void constrainTranslation(qglviewer::Vec & t_world, qglviewer::Frame * const frame) override
    {
        qglviewer::Vec new_world_pos = t_world + frame->position();

        new_world_pos.x = into_range(float(new_world_pos.x), -_box_size_half, _box_size_half);
        new_world_pos.y = into_range(float(new_world_pos.y), -_box_size_half, _box_size_half);
        new_world_pos.z = into_range(float(new_world_pos.z), -_box_size_half, _box_size_half);

        t_world = new_world_pos - frame->position();
    }

private:
    float _box_size_half;
};

My_viewer::My_viewer(Core &core, const QGLFormat &format) : Options_viewer(format), _core(core)
{
    std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

    _parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));
    _parameters["global_scale"]->set_hidden(true);

    _parameters.add_parameter(new Parameter("z_near", 0.1f, 0.01f, 100.0f, std::bind(&My_viewer::change_clipping, this)));
//    _parameters["z_near"]->set_hidden(true);
    _parameters.add_parameter(new Parameter("z_far", 100.0f, 1.0f, 1000.0f, std::bind(&My_viewer::change_clipping, this)));
//    _parameters["z_far"]->set_hidden(true);

    _parameters.add_parameter(new Parameter("draw_closest_force", true, update));
    _parameters["draw_closest_force"]->set_hidden(true);

    _parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));
    _parameters["indicator_scale"]->set_hidden(true);

    _parameters.add_parameter(new Parameter("draw_tree_depth", 1, -1, 10, update));
    _parameters["draw_tree_depth"]->set_hidden(true);

    _parameters.add_parameter(new Parameter("draw_handles", true, update));
    _parameters["draw_handles"]->set_hidden(true);

//    std::vector<std::string> ui_states { "Level Editor", "Playing" };
//    _parameters.add_parameter(new Parameter("Interface", 0, ui_states, std::bind(&My_viewer::change_ui_state, this)));

    _parameters.add_parameter(Parameter::create_button("Save Settings", std::bind(&My_viewer::save_parameters_with_check, this)));
    _parameters.add_parameter(Parameter::create_button("Load Settings", std::bind(&My_viewer::restore_parameters_with_check, this)));
//    _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));
//    _parameters.add_parameter(Parameter::create_button("Reset Level", std::bind(&My_viewer::reset_level, this)));

    setup_fonts();

    connect(&_core, SIGNAL(level_changed(Main_game_screen::Level_state)), this, SLOT(handle_level_change(Main_game_screen::Level_state)));
}

void My_viewer::print_cam_orientation()
{
    qglviewer::Vec axis = camera()->orientation().axis();
    std::cout << "angle: " << camera()->orientation().angle() << " axis: " << axis[0] << ", " << axis[1] << ", " << axis[2] << " pos: " << QGLV2Eigen(camera()->position()) << std::endl;
}

void My_viewer::setup_fonts()
{
//    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
//    _main_font = QFont(family);

    int id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_absolute_qfilename("fonts/Matiz.ttf"));
//    int id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_absolute_qfilename("fonts/LondrinaOutline-Regular.otf"));
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    _particle_font = QFont(family);
}


Eigen::Vector3f My_viewer::calc_camera_starting_point_from_borders()
{
    float const game_field_width_x = _core.get_level_data()._game_field_borders[Level_data::Plane::Pos_X]->get_position()[0]
            - _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_X]->get_position()[0];
    float const game_field_width_y = _core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Y]->get_position()[1]
            - _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Y]->get_position()[1];
    float const game_field_width_z = _core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
            - _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2];

    float const margin = 25.0f;

    float const full_width[2] = { game_field_width_x + 2.0f * margin, game_field_width_z + 2.0f * margin };

    float const aspect_ratio = camera()->aspectRatio();

    float const fov_horizontal = 90.0f / 360.0f * 2.0f * float(M_PI);
    float const fov_vertical = fov_horizontal / aspect_ratio;

    float const y_max = -std::max(full_width[0] * 0.5f / std::tan(fov_horizontal * 0.5f),
            full_width[1] * 0.5f / std::tan(fov_vertical * 0.5f)) - game_field_width_y * 0.5f;

    return Eigen::Vector3f(0.0f, y_max, 0.0f);
}


void My_viewer::update_game_camera()
{
    float const z = 0.5f * (_core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
            + _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2]);

    //        _my_camera->setUpVector(qglviewer::Vec(0.0f, 0.0f, 1.0f));
    //        _my_camera->setViewDirection(qglviewer::Vec(0.0f, 1.0f, 0.0f));
    //        _my_camera->setPosition(qglviewer::Vec(0.0f, -80.0f, z));

    Eigen::Vector3f cam_start_position = calc_camera_starting_point_from_borders();

    qglviewer::Vec position(0.0f, cam_start_position[1], z);

    //        qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
    //        kfi->addKeyFrame(*camera()->frame());
    //        camera()->setKeyFrameInterpolator(0, kfi);
    //        qglviewer::Quaternion orientation(qglviewer::Vec(0.0f, 1.0f, 0.0f), 0.5f * M_PI);
    //        qglviewer::Quaternion orientation(qglviewer::Vec(0.0f, 0.0f, 1.0f), 0.0f);
    qglviewer::Quaternion orientation(qglviewer::Vec(1.0f, 0.0f, 0.0f), M_PI * 0.5f);

    qglviewer::Frame level_initial_view(position, orientation);
    //        level_initial_view.rotate(orientation);
    //        level_initial_view.setPosition(level_start_position);
    //        kfi->addKeyFrame(level_initial_view);
    //        kfi->setInterpolationTime(2.0f);
    //        kfi->startInterpolation();

    camera()->interpolateTo(level_initial_view, 2.0f);
}

void My_viewer::change_clipping()
{
    _my_camera->set_near_far(_parameters["z_near"]->get_value<float>(), _parameters["z_far"]->get_value<float>());
    update();
}

void My_viewer::init()
{
    std::cout << __FUNCTION__ << std::endl;

    bool const opengl_initialized = initializeOpenGLFunctions();

    if (!opengl_initialized)
    {
        QMessageBox e;
        e.setText("LD_LIBRARY_PATH=Couldn't initialize OpenGL.");
        e.exec();

        abort();
    }

    Base::init();

//    restoreStateFromFile();

    resize(1280, 720);
    move(QApplication::desktop()->screen()->rect().center() - rect().center());

    _my_camera = new StandardCamera(10.0f, 1000.0f);
    _my_camera->frame()->setSpinningSensitivity(1000.0f);

    Game_camera_constraint * camera_constraint = new Game_camera_constraint(195.0f);
    _my_camera->frame()->setConstraint(camera_constraint);

    // constrains the rotation so that the camera can't rotate around the viewing axis
    _my_camera->setUpVector({0.0f, 0.0f, 1.0f});
    _my_camera->frame()->setRotatesAroundUpVector(true);

    setCamera(_my_camera);

    // effectively disables update() calls during manipulation since they are not necessary when running animation
    disconnect(camera()->frame(), SIGNAL(manipulated()), this, SLOT(update()));
    disconnect(camera()->frame(), SIGNAL(spun()), this, SLOT(update()));

    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setSceneRadius(50.0f);

    const GLubyte* opengl_version = glGetString(GL_VERSION);
    const GLubyte* shader_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << __FUNCTION__ << " opengl version: " << opengl_version << std::endl;
    std::cout << __FUNCTION__ << " shader version: " << shader_version << std::endl;

    glEnable(GL_TEXTURE_2D);

    setBackgroundColor(QColor::fromRgbF(1.0f, 1.0f, 1.0f, 1.0f));
}

void My_viewer::start()
{
    restore_parameters();

    setAnimationPeriod(16);

    startAnimation();

    _frame_timer.start();

    _ui_renderer.init(context(), size());

    _screen_stack.clear();

    _core.gl_init(context());

    Screen * s = new Main_game_screen(*this, _core);
    s->pause();
    add_screen(s);
    add_screen(new Main_menu_screen(*this, _core));

//    add_screen(new Experiment_screen(*this, _core)); // DEBUG screen
//    add_screen(Help_screen::test(*this, _core)); // DEBUG screen
}


void My_viewer::draw()
{
    std::vector<Screen*> reverse_screens;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        if (s->get_state() == Screen::State::Killed) continue;

        reverse_screens.push_back(s.get());

        if (int(s->get_type()) & int(Screen::Type::Fullscreen))
        {
            break;
        }
    }

    std::reverse(reverse_screens.begin(), reverse_screens.end());

    for (Screen * s : reverse_screens)
    {
        s->draw();
    }
}

Eigen::Vector2f My_viewer::get_projected_coordinates(Eigen::Vector3f const& world_position) const
{
    Eigen::Vector3f projected_position = QGLV2Eigen(_my_camera->projectedCoordinatesOf(Eigen2QGLV(world_position)));

    projected_position[0] /= float(_my_camera->screenWidth());
    projected_position[1] /= float(_my_camera->screenHeight());
    projected_position[1] = 1.0f - projected_position[1];

    return Eigen::Vector2f(projected_position[0], projected_position[1]);
}

void My_viewer::draw_textured_quad(const GLuint tex_id)
{
    glBindTexture(GL_TEXTURE_2D, tex_id);
    draw_quad_with_tex_coords();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void My_viewer::start_normalized_screen_coordinates()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0f, 1.0f, 0.0f, 1.0, 0.0f, -1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void My_viewer::stop_normalized_screen_coordinates()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void My_viewer::draw_button(const Draggable_button *b, const bool for_picking, float const alpha)
{
    glPushMatrix();

    glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);

    glScalef(b->get_extent()[0] * 0.5f, b->get_extent()[1] * 0.5f, 1.0f);

    if (for_picking)
    {
        draw_quad_with_tex_coords();
    }
    else
    {
        if (b->is_pressable() && !b->is_pressed())
        {
            glColor4f(0.8f, 0.8f, 0.9f, alpha * 0.7f);
        }

        draw_textured_quad(b->get_texture());

        glColor4f(1.0f, 1.0f, 1.0f, alpha);
    }

    glPopMatrix();
}

void My_viewer::draw_label(const Draggable_label *b, float const /*alpha*/)
{
    glPushMatrix();

    glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
    glScalef(b->get_extent()[0] * 0.5f, b->get_extent()[1] * 0.5f, 1.0f);
    glColor4f(b->get_color()[0], b->get_color()[1], b->get_color()[2], b->get_alpha());

    draw_textured_quad(b->get_texture());

    glPopMatrix();
}

void My_viewer::draw_slider(Draggable_slider const& s, const bool for_picking, float const alpha)
{
    Draggable_screen_point const& p = s.get_slider_marker();

    glPushMatrix();

    Eigen::Transform<float, 3, Eigen::Affine> const slider_system = s.get_transform();

    if (!for_picking)
    {
        Eigen::Vector3f const line_start = slider_system * Eigen::Vector3f(-1.0f, 0.0f, 0.0f);
        Eigen::Vector3f const line_stop  = slider_system * Eigen::Vector3f( 1.0f, 0.0f, 0.0f);

        glColor3f(0.2f, 0.2f, 0.3f);

        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3fv(line_start.data());
        glVertex3fv(line_stop.data());
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);

        glPushMatrix();
        glTranslatef(s.get_position()[0] - s.get_extent()[0] * 0.5f - s.get_extent()[1] * 0.5f, s.get_position()[1], s.get_position()[2]);
        glScalef(s.get_extent()[1] * 0.3f, s.get_extent()[1] * 0.3f * camera()->aspectRatio(), 1.0f);

        draw_textured_quad(s.get_texture());
        glPopMatrix();
    }

    Eigen::Vector3f const marker_pos = slider_system * p.get_position();

    glTranslatef(marker_pos[0], marker_pos[1], marker_pos[2]);
    glScalef(0.01f, 0.01f * camera()->aspectRatio(), 1.0f); // draw a square
    glColor4f(1.0f, 1.0f, 1.0f, s.get_alpha() * alpha);

    if (for_picking)
    {
        draw_quad_with_tex_coords();
    }
    else
    {
        draw_textured_quad(s.get_slider_marker_texture());
    }

    glPopMatrix();
}

//void My_viewer::draw_spinbox(Draggable_spinbox const& s, const bool for_picking, float const alpha)
//{
////    Draggable_screen_point const& p = s.get_slider_marker();

//    glPushMatrix();

//    Eigen::Transform<float, 3, Eigen::Affine> const spinbox_system = s.get_transform();

////    if (!for_picking)
////    {
////        Eigen::Vector3f const line_start = spinbox_system * Eigen::Vector3f(-1.0f, 0.0f, 0.0f);
////        Eigen::Vector3f const line_stop  = spinbox_system * Eigen::Vector3f( 1.0f, 0.0f, 0.0f);

////        glColor3f(0.2f, 0.2f, 0.3f);

////        glLineWidth(2.0f);
////        glBegin(GL_LINES);
////        glVertex3fv(line_start.data());
////        glVertex3fv(line_stop.data());
////        glEnd();

////        glColor3f(1.0f, 1.0f, 1.0f);

////        glPushMatrix();
////        glTranslatef(s.get_position()[0] - s.get_extent()[0] * 0.5f - s.get_extent()[1] * 0.5f, s.get_position()[1], s.get_position()[2]);
////        glScalef(s.get_extent()[1] * 0.3f, s.get_extent()[1] * 0.3f * camera()->aspectRatio(), 1.0f);

////        draw_textured_quad(s.get_texture());
////        glPopMatrix();
////    }

//    Eigen::Vector3f const decrement_pos = spinbox_system * s.get_decrement_draggable().get_position();

//    glPushMatrix();
//    glTranslatef(decrement_pos[0], decrement_pos[1], decrement_pos[2]);
//    glScalef(0.01f, 0.01f * camera()->aspectRatio(), 1.0f); // draw a square
//    glColor4f(1.0f, 1.0f, 0.0f, s.get_alpha() * alpha);

//    if (for_picking)
//    {
//        draw_quad_with_tex_coords();
//    }
//    else
//    {
//        draw_quad_with_tex_coords();
////        draw_textured_quad(s.get_slider_marker_texture());
//    }

//    glPopMatrix();

//    Eigen::Vector3f const increment_pos = spinbox_system * s.get_increment_draggable().get_position();

//    glPushMatrix();
//    glTranslatef(increment_pos[0], increment_pos[1], increment_pos[2]);
//    glScalef(0.01f, 0.01f * camera()->aspectRatio(), 1.0f); // draw a square
//    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
//    glColor4f(1.0f, 0.0f, 1.0f, s.get_alpha() * alpha);

//    if (for_picking)
//    {
//        draw_quad_with_tex_coords();
//    }
//    else
//    {
//        draw_quad_with_tex_coords();
////        draw_textured_quad(s.get_slider_marker_texture());
//    }

//    glPopMatrix();


//    glPopMatrix();
//}

void My_viewer::draw_statistic(const Draggable_statistics &b)
{
    std::vector<float> const& values = b.get_values();

    if (values.empty()) return;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPushMatrix();

    glTranslatef(b.get_position()[0], b.get_position()[1], b.get_position()[2]);
    glScalef(b.get_extent()[0] * 0.5f, b.get_extent()[1] * 0.5f, 1.0f);

    draw_textured_quad(b.get_texture());

    glScalef(2.0f, 2.0f, 1.0f);

    glTranslatef(-0.5f, -0.5f, 0.0f);

    glTranslatef(0.1f, 0.15f, 0.0f);
    glScalef(0.8f, 0.6f, 1.0f);

    _ui_renderer.draw_curved_particle_system_in_existing_coord_sys(b.get_particle_system(), height());

    glPopMatrix();
}

void My_viewer::mousePressEvent(QMouseEvent *event)
{
    bool handled = false;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        if (s->get_state() == Screen::State::Killing || s->get_state() == Screen::State::Killed) continue;

        handled = s->mousePressEvent(event);

        if (handled || int(s->get_type()) & int(Screen::Type::Modal))
        {
            break;
        }
    }

    if (!handled)
    {
//        std::cout << __FUNCTION__ << std::endl;
        Base::mousePressEvent(event);
    }
}

void My_viewer::mouseMoveEvent(QMouseEvent *event)
{
    bool handled = false;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        if (s->get_state() == Screen::State::Killing || s->get_state() == Screen::State::Killed) continue;

        handled = s->mouseMoveEvent(event);

        if (handled || int(s->get_type()) & int(Screen::Type::Modal))
        {
            break;
        }
    }

    if (!handled)
    {
//        std::cout << __FUNCTION__ << std::endl;
        Base::mouseMoveEvent(event);
    }
}

//bool My_viewer::check_for_collision(const Level_element *level_element)
//{
//    for (boost::shared_ptr<Level_element> const& l : _core.get_level_data()._level_elements)
//    {
//        if (l.get() != level_element && level_element->does_intersect(l.get()))
//        {
//            return true;
//        }
//    }

//    return false;
//}

void My_viewer::mouseReleaseEvent(QMouseEvent *event)
{
    bool handled = false;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        if (s->get_state() == Screen::State::Killing || s->get_state() == Screen::State::Killed) continue;

        handled = s->mouseReleaseEvent(event);

        if (handled || int(s->get_type()) & int(Screen::Type::Modal))
        {
            break;
        }
    }

    if (!handled)
    {
//        std::cout << __FUNCTION__ << std::endl;
        Base::mouseReleaseEvent(event);
    }
}

void My_viewer::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        handled = s->keyPressEvent(event);

        if (handled || int(s->get_type()) & int(Screen::Type::Modal))
        {
            break;
        }
    }

    if (!handled && event->key() == Qt::Key_Escape)
    {
        // don't react to ESC
        handled = true;
    }

    if (event->key() == Qt::Key_S && !(event->modifiers() & Qt::ControlModifier) &&
            !(event->modifiers() & Qt::ShiftModifier))
    {
        std::cout << __FUNCTION__ << " Screen Stack" << std::endl;

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
            Screen const& screenRef = *(s.get());
            std::cout << typeid(screenRef).name() << " state: " << int(s->get_state()) << " ptr: " << s.get() << std::endl;
        }

        handled = true;
    }
    else if (event->key() == Qt::Key_O)
    {
        if (event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier)
        {
            Main_options_window::get_instance()->show();
        }
//        Main_options_window * w = Main_options_window::create();
//        w->add_parameter_list("Viewer", _parameters);
//        w->show();

        handled = true;
    }
    else if (event->key() == Qt::Key_F10)
    {
        std::cout << __func__ << " core.gl_init()" << std::endl;

        _core.gl_init(context());

        handled = true;
    }

    if (!handled)
    {
        Base::keyPressEvent(event);
    }
}

void My_viewer::wheelEvent(QWheelEvent * event)
{
    bool handled = false;

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        if (s->get_state() == Screen::State::Killing || s->get_state() == Screen::State::Killed) continue;

        handled = s->wheelEvent(event);

        if (handled || int(s->get_type()) & int(Screen::Type::Modal))
        {
            break;
        }
    }

    if (!handled)
    {
//        std::cout << __FUNCTION__ << std::endl;
        Base::wheelEvent(event);
    }
}

void My_viewer::animate()
{
//    float const time_step = _frame_timer.restart() / 1000.0f;
    float const time_step = animationPeriod() / 1000.0f;
//    std::cout << __func__ << " frame time: " << time_step << std::endl;

//    _core.update_level_elements(time_step);

    _screen_stack.erase(std::remove_if(_screen_stack.begin(), _screen_stack.end(), Screen::is_dead), _screen_stack.end());

    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        s->update(time_step);
        s->update_events(time_step);

//        if (int(s->get_type()) & int(Screen::Type::Fullscreen))
//        {
//            break;
//        }
    }

    for (Screen * s : _delayed_screen_stack)
    {
        add_screen(s);
    }

    _delayed_screen_stack.clear();

    Base::animate();
}

void My_viewer::add_screen(Screen *s)
{
    _screen_stack.push_front(std::unique_ptr<Screen>(s));
}

void My_viewer::add_screen_delayed(Screen *s)
{
    _delayed_screen_stack.push_back(s);
}

void My_viewer::kill_all_screens()
{
    for (std::unique_ptr<Screen> const& s : _screen_stack)
    {
        s->kill();
    }
}

void My_viewer::replace_screens(Screen *s)
{
    kill_all_screens();
    add_screen(s);
}

//void My_viewer::kill_screens_on_top(Screen *s)
//{
//    bool done = false;

//    assert(std::find(_screen_stack.begin(), _screen_stack.end(), s) != _screen_stack.end());

//    while (!done)
//    {
//        Screen * top_screen = _screen_stack.front().get();

//        if (top_screen != s)
//        {
//            s->kill();
//        }
//        else
//        {
//            done = true;
//        }
//    }
//}

Screen *My_viewer::get_current_screen() const
{
    return _screen_stack.front().get();
}

void My_viewer::load_defaults()
{
    _core.load_level_defaults();
}

void My_viewer::resizeEvent(QResizeEvent *ev)
{
//    float const aspect_ratio = 16.0f / 9.0f;

//    float const actual_ratio = ev->size().width() / float(ev->size().height());

//    if (aspect_ratio / actual_ratio < 1.0f)
//    {
////        resize(ev->size().width(), ev->oldSize().height());
//        setGeometry(0, 0, ev->size().width(), ev->oldSize().height());
//    }
//    else if (aspect_ratio / actual_ratio > 1.0f)
//    {
////        resize(ev->oldSize().width(), ev->size().height());
//        setGeometry(0, 0, ev->oldSize().width(), ev->size().height());
////        resize(actual_ratio / aspect_ratio * ev->size());
//    }

    Base::resizeEvent(ev);

    glViewport(0, 0, ev->size().width(), ev->size().height());

    _ui_renderer.resize(ev->size());

    for (std::unique_ptr<Screen> const& screen : _screen_stack)
    {
        screen->resize(ev->size());
    }

    if (camera()->screenHeight() != ev->size().height())
    {
        std::cout << __FUNCTION__ << " size mismatch: " << camera()->screenWidth() << " " << ev->size().width() << std::endl;
        std::cout << __FUNCTION__ << " size mismatch: " << camera()->screenHeight() << " " << ev->size().height() << std::endl;

//        assert(false);
    }
}

void My_viewer::quit_game()
{
    _core.quit();
    close();
}

const Ui_renderer &My_viewer::get_renderer() const
{
    return _ui_renderer;
}

Ui_renderer &My_viewer::get_renderer()
{
    return _ui_renderer;
}

Eigen::Vector2f My_viewer::qpixel_to_uniform_screen_pos(const QPoint & p)
{
    return Eigen::Vector2f(
                p.x() / float(camera()->screenWidth()),
                (camera()->screenHeight() - p.y())  / float(camera()->screenHeight())
                );
}

void My_viewer::disable_camera_control()
{
    setMouseBinding(Qt::NoModifier, Qt::LeftButton, QGLViewer::CAMERA, QGLViewer::NO_MOUSE_ACTION);
    setMouseBinding(Qt::NoModifier, Qt::RightButton, QGLViewer::CAMERA, QGLViewer::NO_MOUSE_ACTION);
    setMouseBinding(Qt::NoModifier, Qt::MidButton, QGLViewer::CAMERA, QGLViewer::NO_MOUSE_ACTION);
    setWheelBinding(Qt::NoModifier, QGLViewer::CAMERA, QGLViewer::NO_MOUSE_ACTION);
}

void My_viewer::enable_camera_control()
{
    setMouseBinding(Qt::NoModifier, Qt::LeftButton, QGLViewer::CAMERA, QGLViewer::ROTATE);
    setMouseBinding(Qt::NoModifier, Qt::RightButton, QGLViewer::CAMERA, QGLViewer::TRANSLATE);
    setMouseBinding(Qt::NoModifier, Qt::MidButton, QGLViewer::CAMERA, QGLViewer::ZOOM);
    setWheelBinding(Qt::NoModifier, QGLViewer::CAMERA, QGLViewer::ZOOM);
}

void My_viewer::handle_level_change(const Main_game_screen::Level_state state)
{
    if (state == Main_game_screen::Level_state::Running)
    {
        update_game_camera();
    }
}
