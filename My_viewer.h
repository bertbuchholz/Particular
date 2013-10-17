#ifndef TEMPLATE_VIEWER_H
#define TEMPLATE_VIEWER_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QSpinBox>
#include <iostream>

#include <Eigen/Geometry>

#include <Options_viewer.h>
#include <Draw_functions.h>

#include <Picking.h>
#include <Registry_parameters.h>
#include <Geometry_utils.h>
#include <FloatSlider.h>

#include "Core.h"
#include "Atom.h"
#include "Spatial_hash.h"
#include "Renderer.h"

#include "Draggable.h"
#include "Level_element_draw_visitor.h"
#include "Screen.h"
#include "Main_game_screen.h"
#include "Main_menu_screen.h"
#include "Main_options_window.h"

class My_viewer : public Options_viewer // , public QGLFunctions
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    enum class Level_state { /* Main_menu, Pause_menu, */ Intro, /* Before_start, */ Running /* , After_finish, Statistics */ };

    My_viewer(QGLFormat const& format = QGLFormat()) : Options_viewer(format)
    {
        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

        _parameters.add_parameter(new Parameter("levels", std::string("")));

        _parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&My_viewer::update_physics_timestep, this)));
        _parameters["physics_timestep_ms"]->set_hidden(true);
        _parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update));
        _parameters["physics_speed"]->set_hidden(true);
        _parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));
        _parameters["global_scale"]->set_hidden(true);

        _parameters.add_parameter(new Parameter("z_near", 0.1f, 0.01f, 100.0f, std::bind(&My_viewer::change_clipping, this)));
        _parameters["z_near"]->set_hidden(true);
        _parameters.add_parameter(new Parameter("z_far", 100.0f, 1.0f, 1000.0f, std::bind(&My_viewer::change_clipping, this)));
        _parameters["z_far"]->set_hidden(true);

//        Parameter_registry<Core>::create_normal_instance("Core", &_parameters, std::bind(&My_viewer::change_core_settings, this));
//        add_widget_to_options(_core.get_parameter_widget());
//        Main_options_window::get_instance()->add_widget(_core.get_parameter_widget());


//        Parameter_registry<Level_data>::create_normal_instance("Level_data", &_parameters, std::bind(&My_viewer::change_level_data_settings, this));

        _parameters.add_parameter(new Parameter("draw_closest_force", true, update));
        _parameters["draw_closest_force"]->set_hidden(true);

        _parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));
        _parameters["indicator_scale"]->set_hidden(true);

        _parameters.add_parameter(new Parameter("draw_tree_depth", 1, -1, 10, update));
        _parameters["draw_tree_depth"]->set_hidden(true);

        _parameters.add_parameter(new Parameter("draw_handles", true, update));
        _parameters["draw_handles"]->set_hidden(true);

        std::vector<std::string> ui_states { "Level Editor", "Playing" };
        _parameters.add_parameter(new Parameter("Interface", 0, ui_states, std::bind(&My_viewer::change_ui_state, this)));

        std::vector<std::string> object_types { "O2", "H2O", "SDS", "Na", "Cl", "Dipole",
//                                                  "Plane_barrier",
                                                  "Box_barrier",
                                                  "Brownian_box",
                                                  "Box_portal",
                                                  "Sphere_portal",
//                                                  "Blow_barrier",
//                                                  "Moving_box_barrier",
                                                  "Molecule_releaser",
//                                                  "Atom_cannon",
                                                  "Charged_barrier",
                                                  "Tractor_barrier" };

        _parameters.add_parameter(new Parameter("Object Type", 0, object_types, update));


        _parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&My_viewer::toggle_simulation, this)));
        _parameters.add_parameter(Parameter::create_button("Save Level", std::bind(&My_viewer::save_level, this)));
        _parameters.add_parameter(Parameter::create_button("Load Level", std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::load_level), this)));
        _parameters.add_parameter(Parameter::create_button("Save Settings", std::bind(&My_viewer::save_parameters_with_check, this)));
        _parameters.add_parameter(Parameter::create_button("Load Settings", std::bind(&My_viewer::restore_parameters_with_check, this)));
        _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));
//        _parameters.add_parameter(Parameter::create_button("Start Level", std::bind(&My_viewer::start_level, this)));
        _parameters.add_parameter(Parameter::create_button("Reset Level", std::bind(&My_viewer::reset_level, this)));
//        _parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));
//        _parameters.add_parameter(Parameter::create_button("Show cam orientation", std::bind(&My_viewer::print_cam_orientation, this)));

//        setup_ui_elements();

//        change_core_settings();
//        change_ui_state();

        setup_fonts();
    }

    void print_cam_orientation()
    {
        qglviewer::Vec axis = camera()->orientation().axis();
        std::cout << "angle: " << camera()->orientation().angle() << " axis: " << axis[0] << ", " << axis[1] << ", " << axis[2] << " pos: " << QGLV2Eigen(camera()->position()) << std::endl;
    }

    void setup_fonts()
    {
        int id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_absolute_qfilename("fonts/Matiz.ttf"));
        QString family = QFontDatabase::applicationFontFamilies(id).at(0);
        _main_font = QFont(family);

        id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_absolute_qfilename("fonts/LondrinaOutline-Regular.otf"));
        family = QFontDatabase::applicationFontFamilies(id).at(0);
        _particle_font = QFont(family);
    }

    void save_level()
    {
        QString filename;

        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        {
            filename = "state.data";
        }
        else
        {
            filename = QFileDialog::getSaveFileName(this, tr("Save Level"),
                                                    ".",
                                                    tr("State File (*.data)"));
        }

        if (!filename.isEmpty())
        {
            _core.save_level(filename.toStdString());
        }
    }

    void load_level()
    {
        QString filename;

        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        {
            filename = "state.data";
        }
        else
        {
            filename = QFileDialog::getOpenFileName(this, tr("Load Level"),
                                                    ".",
                                                    tr("State File (*.data)"));
        }

        if (!filename.isEmpty())
        {
            load_level(filename.toStdString());
        }
    }

    void load_level(std::string const& filename)
    {
        std::cout << __PRETTY_FUNCTION__ << " " << filename << std::endl;

        clear();
        set_simulation_state(false);
        try
        {
            _core.load_level(filename);
        }
        catch (...)
        {
            load_defaults();
            QMessageBox::warning(this, "Error", QString("Error reading the specified level file ") + QString::fromStdString(filename) + "\nLoading defaults.");
        }

//        _core.update_parameter_list(*_parameters.get_child("Core"));
        _core.reset_level();

//        _parameters.get_child(_core.get_level_data().name())->load(_core.get_level_data().get_current_parameters());
        _core.get_level_data().update_parameters();

        _current_level_name = filename;

//        _renderer->update(_core.get_level_data());

        update_game_camera();

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        update();
    }

    void restore_parameters() override
    {
        Base::restore_parameters();

        update();
    }

    void load_next_level()
    {
        std::cout << __PRETTY_FUNCTION__ << " next level: " << _core.get_progress().last_level << std::endl;

        if (_level_names.size() <= _core.get_progress().last_level)
        {
            std::cout << "No more levels." << std::endl;
            return;
        }

        set_simulation_state(false);

        std::string const filename = (Data_config::get_instance()->get_absolute_filename("levels/" + _level_names[_core.get_progress().last_level] + ".data"));

        load_level(filename);

        reset_level();

        update();
    }

    void reset_level()
    {
        _core.reset_level();
//        update_draggable_to_level_element();
//        update_active_draggables();

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        update();
    }

//    void change_core_settings()
//    {
//        _core.set_parameters(*_parameters.get_child("Core"));
//        update();
//    }

    void change_level_data_settings()
    {
//        _core.get_level_data().set_parameters(*_parameters.get_child("Level_data"));
        update();
    }

    class Game_camera_constraint_old : public qglviewer::Constraint
    {
    public:
        Game_camera_constraint_old(std::map<Level_data::Plane, Plane_barrier*> const& game_field_borders) : _game_field_borders(game_field_borders)
        { }

        void constrainTranslation(qglviewer::Vec & t, qglviewer::Frame * const frame) override
        {
            // Express t in the world coordinate system.
            const qglviewer::Vec tWorld = frame->inverseTransformOf(t);
            if (frame->position().z + tWorld.z > _game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2])
            {
                std::cout << frame->position().z << " "
                          << tWorld.z << " "
                          << _game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2] << "\n";
//                t.z = frame->transformOf(-frame->position().z); // t.z is clamped so that next z position is 0.0
                t.z = _game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2] - frame->position().z;

                std::cout << "new t.z: " << t.z << "\n";

            }
            if (frame->position().z + tWorld.z < _game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2])
            {
                t.z = _game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2] - frame->position().z;
            }
            if (frame->position().x + tWorld.x > _game_field_borders[Level_data::Plane::Pos_X]->get_position()[0])
            {
                t.x = _game_field_borders[Level_data::Plane::Pos_X]->get_position()[0] - frame->position().x;
            }
            if (frame->position().x + tWorld.x < _game_field_borders[Level_data::Plane::Neg_X]->get_position()[0])
            {
                t.x = _game_field_borders[Level_data::Plane::Neg_X]->get_position()[0] - frame->position().x;
            }

            t.y = 0.0f;
        }

        void constrainRotation(qglviewer::Quaternion & rotation, qglviewer::Frame * const frame) override
        {
            qglviewer::Vec rotation_axis = frame->inverseTransformOf(rotation.axis());

            std::cout << __PRETTY_FUNCTION__ << " " << rotation_axis.x << " " << rotation_axis.y << " " << rotation_axis.z << std::endl;

            rotation_axis.y = 0.0f;

//            into_range(rotation_axis.x, -0.2, 0.2);
//            into_range(rotation_axis.y, -0.2, 0.2);
            rotation.setAxisAngle(frame->transformOf(rotation_axis), rotation.angle());
        }

    private:
        std::map<Level_data::Plane, Plane_barrier*> _game_field_borders;
    };

    class Game_camera_constraint : public qglviewer::Constraint
    {
    public:
        Game_camera_constraint(std::map<Level_data::Plane, Plane_barrier*> const& game_field_borders) : _game_field_borders(game_field_borders)
        { }

        void constrainTranslation(qglviewer::Vec & t, qglviewer::Frame * const frame) override
        {
            float const game_field_width_x = _game_field_borders[Level_data::Plane::Pos_X]->get_position()[0] - _game_field_borders[Level_data::Plane::Neg_X]->get_position()[0];
            float const game_field_width_z = _game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2] - _game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2];

            float const margin = 25.0f;

            float const full_width[2] = { game_field_width_x + 2.0f * margin, game_field_width_z + 2.0f * margin };

            float const aspect_ratio = 1.333f;

            float const fov_horizontal = 90.0f / 360.0f * 2.0f * M_PI;
            float const fov_vertical = fov_horizontal / aspect_ratio;

            float const y_max = -std::max(full_width[0] * 0.5f / std::tan(fov_horizontal * 0.5f),
                    full_width[1] * 0.5f / std::tan(fov_vertical * 0.5f));

            float const y_min = y_max * 0.5f;

            // Express t in the world coordinate system.
            const qglviewer::Vec tWorld = frame->inverseTransformOf(t);

            Eigen::Vector3f pyramid_tip_point(0.0f, y_max, 0.0f);

            float const z_offset = std::tan(fov_vertical * 0.5f) * (y_max - y_min);
            float const x_offset = std::tan(fov_horizontal * 0.5f) * (y_max - y_min);

            std::vector<Eigen::Vector3f> corner_points;

            corner_points.push_back(Eigen::Vector3f( x_offset, y_min,  z_offset));
            corner_points.push_back(Eigen::Vector3f(-x_offset, y_min,  z_offset));
            corner_points.push_back(Eigen::Vector3f(-x_offset, y_min, -z_offset));
            corner_points.push_back(Eigen::Vector3f( x_offset, y_min, -z_offset));

            std::vector< Eigen::Hyperplane<float, 3> > planes;

            planes.push_back(Eigen::Hyperplane<float, 3>(Eigen::Vector3f(0.0f, 1.0f, 0.0f), pyramid_tip_point));

            planes.push_back(Eigen::Hyperplane<float, 3>((corner_points[3] - pyramid_tip_point).cross(corner_points[0] - pyramid_tip_point).normalized(), pyramid_tip_point));
            planes.push_back(Eigen::Hyperplane<float, 3>((corner_points[0] - pyramid_tip_point).cross(corner_points[1] - pyramid_tip_point).normalized(), pyramid_tip_point));
            planes.push_back(Eigen::Hyperplane<float, 3>((corner_points[1] - pyramid_tip_point).cross(corner_points[2] - pyramid_tip_point).normalized(), pyramid_tip_point));
            planes.push_back(Eigen::Hyperplane<float, 3>((corner_points[2] - pyramid_tip_point).cross(corner_points[3] - pyramid_tip_point).normalized(), pyramid_tip_point));

            Eigen::Vector3f const new_world = QGLV2Eigen(frame->position() + tWorld);

            std::vector<int> front_planes;

            for (size_t i = 0; i < planes.size(); ++i)
            {
                if (planes[i].signedDistance(new_world) > 0.0f)
                {
                    front_planes.push_back(i);
                }
            }

            Eigen::Vector3f constrained_world;

            if (front_planes.size() == 4)
            {
                constrained_world = pyramid_tip_point;
            }
            else if (front_planes.size() == 3)
            {
                Eigen::ParametrizedLine<float, 3> const line = plane_plane_intersection(planes[front_planes[0]], planes[front_planes[1]]);

                Eigen::Vector3f const point = line.intersectionPoint(planes[front_planes[2]]);

                constrained_world = point;
            }
            else if (front_planes.size() == 2)
            {
                Eigen::ParametrizedLine<float, 3> const line = plane_plane_intersection(planes[front_planes[0]], planes[front_planes[1]]);

                constrained_world = line.projection(new_world);
            }
            else if (front_planes.size() == 1)
            {
//                assert(front_planes.size() == 1);

                constrained_world = planes[front_planes[0]].projection(new_world);
            }

            if (front_planes.size() > 0)
            {
                t = Eigen2QGLV(constrained_world) - frame->position();
            }
        }

//        void constrainRotation(qglviewer::Quaternion & rotation, qglviewer::Frame * const frame) override
//        {
//            qglviewer::Vec rotation_axis = frame->inverseTransformOf(rotation.axis());

//            std::cout << __PRETTY_FUNCTION__ << " " << rotation_axis.x << " " << rotation_axis.y << " " << rotation_axis.z << std::endl;

//            rotation_axis.y = 0.0f;

////            into_range(rotation_axis.x, -0.2, 0.2);
////            into_range(rotation_axis.y, -0.2, 0.2);
//            rotation.setAxisAngle(frame->transformOf(rotation_axis), rotation.angle());
//        }

    private:
        std::map<Level_data::Plane, Plane_barrier*> _game_field_borders;
    };

    Eigen::Vector3f calc_camera_starting_point_from_borders()
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

        float const fov_horizontal = 90.0f / 360.0f * 2.0f * M_PI;
        float const fov_vertical = fov_horizontal / aspect_ratio;

        float const y_max = -std::max(full_width[0] * 0.5f / std::tan(fov_horizontal * 0.5f),
                full_width[1] * 0.5f / std::tan(fov_vertical * 0.5f)) - game_field_width_y * 0.5f;

        return Eigen::Vector3f(0.0f, y_max, 0.0f);
    }

    void change_ui_state()
    {
        if (_parameters["Interface"]->get_value<std::string>() == "Level Editor")
        {
//            _ui_state = Ui_state::Level_editor;
            _screen_stack.clear();
            add_screen(new Main_game_screen(*this, _core, Main_game_screen::Ui_state::Level_editor));

            camera()->frame()->setConstraint(nullptr);

            load_defaults();
//            _particle_systems.clear();
        }
        else if (_parameters["Interface"]->get_value<std::string>() == "Playing")
        {
//            _ui_state = Ui_state::Playing;

            _screen_stack.clear();
            add_screen(new Main_game_screen(*this, _core, Main_game_screen::Ui_state::Playing));

//            update_game_camera();
            init_game();
        }

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        update();
    }

    void update_game_camera()
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

        Game_camera_constraint * camera_constraint = new Game_camera_constraint(_core.get_level_data()._game_field_borders); // FIXME: leak

//        delete camera()->frame()->constraint();
//        camera()->frame()->setConstraint(camera_constraint);
    }

    void change_clipping()
    {
        _my_camera->set_near_far(_parameters["z_near"]->get_value<float>(), _parameters["z_far"]->get_value<float>());
        update();
    }

    void init() override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

//        initializeGLFunctions();

        Base::init();

        restoreStateFromFile();

        _physics_timer = new QTimer;
        _physics_timer->setInterval(_parameters["physics_timestep_ms"]->get_value<int>());

        connect(_physics_timer, SIGNAL(timeout()), this, SLOT(update_physics()));
//        _physics_timer->start();

        qglviewer::ManipulatedCameraFrame * frame = camera()->frame();
        _my_camera = new StandardCamera(0.1f, 1000.0f);
        _my_camera->setFrame(frame);
        setCamera(_my_camera);

//        _picking.init(context(), size()); // FIXME: needs to be resized when viewer changes
//        _picking.init(context());

        glEnable(GL_NORMALIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        setSceneRadius(50.0f);

        const GLubyte* opengl_version = glGetString(GL_VERSION);
        const GLubyte* shader_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

        std::cout << __PRETTY_FUNCTION__ << " opengl version: " << opengl_version << std::endl;
        std::cout << __PRETTY_FUNCTION__ << " shader version: " << shader_version << std::endl;

//        GPU_force * gpu_force = new GPU_force(context());
//        gpu_force->calc_forces(_core.get_molecules());

        glEnable(GL_TEXTURE_2D);

//        _particle_tex = bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/particle.png")));

        restore_parameters();
//        init_game();

//        update_draggable_to_level_element();
//        update_active_draggables();

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        startAnimation();

        _parameters["Interface"]->set_value(std::string("Playing"));

        _screen_stack.clear();
        add_screen(new Main_game_screen(*this, _core));
        add_screen(new Main_menu_screen(*this, _core));
    }

    void init_game()
    {
//        restore_parameters();

//        _parameters["Interface"]->set_value(std::string("Playing"));

        QString const level_string = QString::fromStdString(_parameters["levels"]->get_value<std::string>());
        _level_names = level_string.split(",", QString::SkipEmptyParts);

        _core.load_progress();

        float z = 0.0f;

        if (_core.get_level_data()._game_field_borders.size() > 0)
        {
            z = 0.5f * (_core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
                    + _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2]);
        }

        camera()->setUpVector(qglviewer::Vec(0.0f, 0.0f, 1.0f));
        camera()->setViewDirection(qglviewer::Vec(0.0f, 1.0f, 0.0f));
        camera()->setPosition(qglviewer::Vec(0.0f, -80.0f, z));
    }

    struct Reject_condition
    {
        bool operator() (Core::Molecule_atom_id const& d) const
        {
            return d.m_id == molecule_id;
        }

        bool operator() (Atom const& atom) const
        {
            return atom._parent_id == molecule_id;
        }

        int molecule_id;
    };

    void draw() override
    {
        std::vector<Screen*> reverse_screens;

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
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

    void draw_textured_quad(GLuint const tex_id)
    {
        glBindTexture(GL_TEXTURE_2D, tex_id);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void start_normalized_screen_coordinates()
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(0.0f, 1.0f, 0.0f, 1.0, 0.0f, -1.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    void stop_normalized_screen_coordinates()
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void draw_button(Draggable_button const* b, bool const for_picking)
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
            draw_textured_quad(b->get_texture());
        }

        glPopMatrix();
    }

    void draw_label(Draggable_label const* b)
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glScalef(b->get_extent()[0] * 0.5f, b->get_extent()[1] * 0.5f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, b->get_alpha());

        draw_textured_quad(b->get_texture());

        glPopMatrix();
    }

    void draw_statistic(Draggable_statistics const& b)
    {
        std::vector<float> const& values = b.get_values();

        if (values.empty()) return;

        glPushMatrix();

        glTranslatef(b.get_position()[0], b.get_position()[1], b.get_position()[2]);
        glScalef(b.get_extent()[0] * 0.5f, b.get_extent()[1] * 0.5f, 1.0f);

        draw_textured_quad(b.get_texture());


        glScalef(2.0f, 2.0f, 1.0f);

        glTranslatef(-0.5f, -0.5f, 0.0f);

        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);

        glLineWidth(2.0f);

        glTranslatef(0.2f, 0.2f, 0.0f);
        glScalef(0.7f, 0.6f, 1.0f);

        glBegin(GL_LINE_STRIP);

        size_t i;
        float const time = values.size() * b.get_normalized_time();

        for (i = 0; i < time; ++i)
        {
            glVertex2f(i / float(values.size()), values[i] / (b.get_max_value() - b.get_min_value()) + b.get_min_value());
        }

        --i;
        Eigen::Vector2f v0(i / float(values.size()), values[i] / (b.get_max_value() - b.get_min_value()) + b.get_min_value());
        i = std::min(values.size() - 1, i + 1);
        Eigen::Vector2f v1(i / float(values.size()), values[i] / (b.get_max_value() - b.get_min_value()) + b.get_min_value());

        float const alpha = time - int(time);

        glVertex2fv(Eigen::Vector2f(v0 * (1.0f - alpha) + v1 * alpha).data());

        glEnd();

        glPopMatrix();
    }

//    void draw_user_force() const
//    {
//        Molecule_external_force const& force = _core.get_user_force();

//        if (force._end_time > _core.get_current_time())
//        {
//            glBegin(GL_LINES);

//            glVertex3fv(force._origin.data());
//            glVertex3fv(Eigen::Vector3f(force._origin + force._force).data());

//            glEnd();

//            glPushMatrix();

//            glTranslatef(force._origin[0], force._origin[1], force._origin[2]);

//            //                float radius = scale * atom._radius;
//            float const radius = 0.2f;
//            glScalef(radius, radius, radius);

//            _icosphere.draw();

//            glPopMatrix();
//        }
//    }

    void draw_indicators() const
    {
        glLineWidth(1.0f);

        if (_core.get_parameters()["use_indicators"]->get_value<bool>())
        {
            glDisable(GL_LIGHTING);

            float const indicator_scale = _parameters["indicator_scale"]->get_value<float>();

            set_color(Color(1.0f));

            for (Force_indicator const& f : _core.get_force_indicators())
            {
                draw_point(f._atom.get_position());

                //glLineWidth(into_range(1.0f, 5.0f, f._force.norm()));
                //            draw_arrow_z_plane(Eigen2OM(f._atom._r), Eigen2OM(f._atom._r + f._force.normalized()));

                draw_arrow_z_plane(Eigen2OM(f._atom.get_position()), Eigen2OM(f._atom.get_position() + f._force * indicator_scale));
            }
        }
    }

//    void draw_closest_force() const
//    {
//        if (_parameters["draw_closest_force"]->get_value<bool>())
//        {
//            Reject_condition reject_cond;

//            for (Molecule const& molecule : _core.get_molecules())
//            {
//                reject_cond.molecule_id = molecule.get_id();

//                for (Atom const& a : molecule._atoms)
//                {
//                    if (std::abs(a._charge) < 0.001f) continue;

//                    boost::optional<Core::Molecule_atom_hash::Point_data const&> opt_pd = _core.get_molecule_hash().get_closest_point(a._r, reject_cond);

//                    if (!opt_pd) continue;

//                    Core::Molecule_atom_hash::Point_data const& pd = opt_pd.get();

//                    boost::optional<Molecule const&> opt_molecule = _core.get_molecule(pd.data.m_id);

//                    assert(opt_molecule);

//                    Atom const& closest_atom = opt_molecule->_atoms[pd.data.a_id];

//                    if (std::abs(closest_atom._charge) < 0.001f) continue;

////                    Atom const& closest_atom = *_core.get_tree().find_closest<Reject_condition>(a._r, reject_cond);

//                    Eigen::Vector3f const force = _core.calc_forces_between_atoms(closest_atom, a);

//                    if (force.norm() > 1e-6f)
//                    {
//                        if (force.dot(closest_atom._r - a._r) < 0.0f)
//                        {
//                            glColor3f(0.3f, 0.8f, 0.2f);
//                        }
//                        else
//                        {
//                            glColor3f(0.8f, 0.2f, 0.2f);
//                        }

//                        glLineWidth(into_range(100.0f * force.norm(), 0.5f, 10.0f));
//                        draw_line(a._r, closest_atom._r);
//                    }
//                }
//            }
//        }
//    }

    void draw_tree() const
    {
        Ball_renderer ball_renderer;

        int const depth = _parameters["draw_tree_depth"]->get_value<int>();

        if (depth < 0) return;

        std::vector<Core::My_tree const*> nodes = _core.get_tree().get_nodes(depth);

        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        for (Core::My_tree const* node : nodes)
        {
            draw_box(node->get_min(), node->get_max());
        }

        glEnable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for (Core::My_tree const* node : nodes)
        {
            if (node->has_averaged_data())
            {
                ball_renderer.draw_atom(node->get_averaged_data(), 1.0f);
            }
        }
    }

    void draw_tree_for_point(Eigen::Vector3f const& point) const
    {
        Ball_renderer ball_renderer;

        Atom dummy = Atom::create_oxygen(point);
        dummy.set_position(point);
        dummy._parent_id = 1000000;

        std::vector<Atom const*> atoms = _core.get_atoms_from_tree(dummy);

        for (Atom const* a : atoms)
        {
            ball_renderer.draw_atom(*a, 1.0f);
        }
    }

    void update_physics_timestep()
    {
        _physics_timer->setInterval(_parameters["physics_timestep_ms"]->get_value<int>());
    }

    void mousePressEvent(QMouseEvent *event)
    {
        bool handled = false;

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
            handled = s->mousePressEvent(event);

            if (handled || int(s->get_type()) & int(Screen::Type::Modal))
            {
                break;
            }
        }

        if (!handled)
        {
            Base::mousePressEvent(event);
        }
    }

    void mouseMoveEvent(QMouseEvent * event) override
    {
        bool handled = false;

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
            handled = s->mouseMoveEvent(event);

            if (handled || int(s->get_type()) & int(Screen::Type::Modal))
            {
                break;
            }
        }

        if (!handled)
        {
            Base::mouseMoveEvent(event);
        }
    }

    bool check_for_collision(Level_element const* level_element)
    {
        for (boost::shared_ptr<Level_element> const& l : _core.get_level_data()._level_elements)
        {
            if (l.get() != level_element && level_element->does_intersect(l.get()))
            {
                return true;
            }
        }

        return false;
    }

    void mouseReleaseEvent(QMouseEvent * event)
    {
        bool handled = false;

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
            handled = s->mouseReleaseEvent(event);

            if (handled || int(s->get_type()) & int(Screen::Type::Modal))
            {
                break;
            }
        }

        if (!handled)
        {
            Base::mouseReleaseEvent(event);
        }
    }

    void keyPressEvent(QKeyEvent *event) override
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

        if (!handled)
        {
            Base::keyPressEvent(event);
        }

//        if (event->key() == Qt::Key_F8)
//        {
//            _particle_systems[int(_level_state)].push_back(Targeted_particle_system(3.0f));
//            _particle_systems[int(_level_state)].back().generate("Blubber", _particle_font, QRectF(0.0f, 0.5f, 1.0f, 0.3f));
//        }
//        else if (event->key() == Qt::Key_Escape)
//        {
//            // don't react to ESC
//        }
//        else
//        {
//            Base::keyPressEvent(event);
//        }
    }


    void do_physics_timestep()
    {
        update_physics();

        update();
    }

    void animate() override
    {
        float const time_step = animationPeriod() / 1000.0f;

        _screen_stack.erase(std::remove_if(_screen_stack.begin(), _screen_stack.end(), Screen::is_dead), _screen_stack.end());

        for (std::unique_ptr<Screen> const& s : _screen_stack)
        {
            s->update(time_step);

            if (int(s->get_type()) & int(Screen::Type::Fullscreen))
            {
                break;
            }
        }

        Base::animate();
    }

    void add_screen(Screen * s)
    {
        _screen_stack.push_front(std::unique_ptr<Screen>(s));
    }

    void clear()
    {
        _core.clear();

        load_defaults();

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        update();
    }

    void toggle_simulation()
    {
        if (!_parameters["Toggle simulation"]->get_value<bool>())
        {
            _physics_timer->stop();
        }
        else
        {
            _physics_timer->start();
            _physics_elapsed_time = std::chrono::system_clock::now();
        }
    }

    void set_simulation_state(bool const s)
    {
        _parameters["Toggle simulation"]->set_value(s);
    }

    void load_defaults() override
    {
//        External_force gravity;

//        gravity._force  = Eigen::Vector3f(0.0f, 0.0f, -1.0f);
//        gravity._origin = Eigen::Vector3f(0.0f, 0.0f, 1e6f);

//        _core.add_external_force("gravity", gravity);

//        Eigen::AlignedBox<float, 3> play_box(Eigen::Vector3f(-40.0f, -20.0f, 0.0f), Eigen::Vector3f(60.0f, 20.0f, 40.0f));

//        _core.set_game_field_borders(play_box.min(), play_box.max());

        _core.get_level_data()._parameters["Game Field Width"]->set_value(80.0f);
        _core.get_level_data()._parameters["Game Field Height"]->set_value(40.0f);
        _core.get_level_data()._parameters["Game Field Depth"]->set_value(40.0f);

//        _core.add_barrier(new Box_barrier(Eigen::Vector3f(-10.0f, -20.0f, 0.0f), Eigen::Vector3f(10.0f, 20.0f, 20.0f), strength, radius));

//        _core.add_brownian_element(new Brownian_box(Eigen::Vector3f(10.0f, -20.0f, -1.0f), Eigen::Vector3f(40.0f, 20.0f, 0.0f),
//                                                    50.0f, 40.0f));

//        _core.add_brownian_element(new Brownian_box(Eigen::Vector3f(-40.0f, -20.0f, 40.0f), Eigen::Vector3f(-10.0f, 20.0f, 41.0f),
//                                                    -50.0f, 40.0f));

        float const na_cl_distance = Atom::create_chlorine(Eigen::Vector3f::Zero())._radius + Atom::create_natrium(Eigen::Vector3f::Zero())._radius;

        Eigen::Vector3f grid_start(15.0f, -15.0f, 5.0f);
        Eigen::Vector3f grid_end  (35.0f,  15.0f, 25.0f);

//        float resolution = 4.0f;
        float resolution = na_cl_distance;

        Eigen::Vector3f num_grid_cells = (grid_end - grid_start) / resolution;

        Eigen::Vector3f offset(resolution / 2.0f, resolution / 2.0f, resolution / 2.0f);

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
            {
                for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
                {
                    int const sum = x + y + z;

                    if ((sum % 2) == 0)
                    {
//                        _core.add_molecule(Molecule::create_charged_chlorine(Eigen::Vector3f(x, y, z)));
                    }
                    else
                    {
//                        _core.add_molecule(Molecule::create_charged_natrium(Eigen::Vector3f(x, y, z) + offset));
//                        _core.add_molecule(Molecule::create_charged_natrium(Eigen::Vector3f(x, y, z)));
                    }
                }
            }
        }

        for (int x = 0; x < num_grid_cells[0]; ++x)
        {
            for (int y = 0; y < num_grid_cells[1]; ++y)
            {
                for (int z = 0; z < num_grid_cells[2]; ++z)
                {
                    int const sum = x + y + z;

//                    Eigen::Vector3f pos = Eigen::Vector3f(x, y, z) * resolution + grid_start;

                    if ((sum % 2) == 0)
                    {
//                        _core.add_molecule(Molecule::create_charged_chlorine(pos));
                    }
                    else
                    {
//                        _core.add_molecule(Molecule::create_charged_natrium(pos));
                    }
                }
            }
        }

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
            {
                for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
                {
//                    _core.add_molecule(Molecule::create_water(Eigen::Vector3f(x, y, z)));
                }
            }
        }


//        Brownian_box * e = new Brownian_box(Eigen::Vector3f(-10.0f, -20.0f, -10.0f), Eigen::Vector3f(10.0f, 20.0f, 10.0f), 10.0f, 25.0f);
//        e->set_user_editable(Level_element::Edit_type::All);
//        e->set_persistent(false);
//        _core.add_brownian_element(e);

//        _parameters["Renderer/type"]->set_value<std::string>("Shader Renderer");
//        change_renderer();

//        update_draggable_to_level_element();
//        update_active_draggables();

        Q_EMIT level_changed(Main_game_screen::Level_state::Running);

        update();
    }

    void resizeEvent(QResizeEvent *ev)
    {
        for (std::unique_ptr<Screen> const& screen : _screen_stack)
        {
            screen->resize(ev->size());
        }

//        _inactive_world_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(ev->size()));

//        glDeleteTextures(1, &_inactive_world_tex);
//        _inactive_world_tex = create_texture(size.width(), size.height());

        Base::resizeEvent(ev);
    }

    QSize adapt_bounding_size(QSize b_size, QSize const& target_size) const
    {
        float const width_ratio = target_size.width() / float(b_size.width());
        float const height_ratio = target_size.height() / float(b_size.height());

        if (width_ratio < height_ratio)
        {
            b_size.setWidth(b_size.width() * width_ratio);
            b_size.setHeight(b_size.height() * width_ratio);
        }
        else
        {
            b_size.setWidth(b_size.width() * height_ratio);
            b_size.setHeight(b_size.height() * height_ratio);
        }

        return b_size;
    }

    float get_scale(QSize const& b_size, QSize const& target_size)
    {
        float const width_ratio = target_size.width() / float(b_size.width());
        float const height_ratio = target_size.height() / float(b_size.height());

        return std::min(width_ratio, height_ratio);
    }

    void generate_button_texture(Draggable_button * b)
    {
        std::cout << __PRETTY_FUNCTION__ << " constructing button texture" << std::endl;

        QSize const pixel_size(width() * b->get_extent()[0], height() * b->get_extent()[1]);

        QImage img(pixel_size, QImage::Format_ARGB32);
        img.fill(QColor(0, 0, 0, 0));

        QPainter p;
        p.begin(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QPen pen;
        pen.setWidth(5);
        pen.setColor(QColor(20, 133, 204));
        p.setPen(pen);

        p.setBrush(QBrush(QColor(76, 153, 204, 120)));

        p.drawRoundedRect(QRect(5, 5, pixel_size.width() - 10, pixel_size.height() - 10), pixel_size.width() * 0.1f, pixel_size.width() * 0.1f);

        QFont font = _main_font;
//        font.setWeight(QFont::Bold);
        font.setPointSizeF(10.0f);
        p.setFont(font);

        p.setPen(QColor(255, 255, 255));

        QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.8f);

        QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

        font.setPointSizeF(10.0f * get_scale(b_size, text_size));
        p.setFont(font);

        p.drawText(img.rect(), Qt::AlignCenter, QString::fromStdString(b->get_text()));

        p.end();

        deleteTexture(b->get_texture());
        b->set_texture(bindTexture(img));
    }

    void generate_label_texture(Draggable_label * b)
    {
        std::cout << __PRETTY_FUNCTION__ << " constructing label texture" << std::endl;

        QSize const pixel_size(width() * b->get_extent()[0], height() * b->get_extent()[1]);

        QImage img(pixel_size, QImage::Format_ARGB32);
        img.fill(QColor(0, 0, 0, 0));

        QPainter p;
        p.begin(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QFont font = _main_font;
//        font.setWeight(QFont::Bold);
        font.setPointSizeF(10.0f);
        p.setFont(font);

        p.setPen(QColor(255, 255, 255));

        QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.8f);

        QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

//        font.setPointSizeF(10.0f * text_size.height() / float(b_size.height())); // always use height ratio to ensure same size text for buttons of same height
        font.setPointSizeF(10.0f * get_scale(b_size, text_size));
        p.setFont(font);

        p.drawText(img.rect(), Qt::AlignCenter, QString::fromStdString(b->get_text()));

        p.end();

        deleteTexture(b->get_texture());
        b->set_texture(bindTexture(img));
    }

    void generate_statistics_texture(Draggable_statistics & b)
    {
        std::cout << __PRETTY_FUNCTION__ << " constructing statistic texture" << std::endl;

        QSize const pixel_size(width() * b.get_extent()[0], height() * b.get_extent()[1]);

        QImage img(pixel_size, QImage::Format_ARGB32);
        img.fill(QColor(0, 0, 0, 0));

        QPainter p;
        p.begin(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QPen pen;
        pen.setWidth(2);
        pen.setColor(QColor(20, 133, 204));
        p.setPen(pen);

        p.setBrush(QBrush(QColor(76, 153, 204, 120)));

        p.drawRoundedRect(QRect(2, 2, pixel_size.width() - 4, pixel_size.height() - 4), pixel_size.width() * 0.02f, pixel_size.width() * 0.02f);


        QFont font = _main_font;
//        font.setWeight(QFont::Bold);
        font.setPixelSize(0.1f * pixel_size.height());
        p.setFont(font);

        p.setPen(QColor(255, 255, 255));

//        QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.8f);

//        QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

//        font.setPointSizeF(10.0f * get_scale(b_size, text_size));
//        p.setFont(font);

        p.drawText(0.2f * pixel_size.width(), 0.15f * pixel_size.height(), QString::fromStdString(b.get_text()));

        pen.setWidthF(0.5f);
        pen.setColor(QColor(255, 255, 255, 100));
        p.setPen(pen);

        p.drawLine(0.2f * pixel_size.width(), 0.8f * pixel_size.height(), 0.9f * pixel_size.width(), 0.8f * pixel_size.height());

        for (int i = 1; i < 5; ++i)
        {
            float const height = 0.8f - 0.6f * i / 4.0f;
            p.drawLine(0.2f * pixel_size.width(), height * pixel_size.height(), 0.9f * pixel_size.width(), height * pixel_size.height());
        }

        p.end();

        deleteTexture(b.get_texture());
        b.set_texture(bindTexture(img));
    }

    void change_level_state(Main_game_screen::Level_state const new_level_state)
    {
        Q_EMIT level_changed(new_level_state);
    }

    void quit_game()
    {
//        QApplication::quit();
        close();
    }

//    void change_state_to_main_menu()
//    {
////        change_level_state(Level_state::Main_menu);
//    }


public Q_SLOTS:
    void update_physics()
    {
        std::chrono::time_point<std::chrono::system_clock> const timer_start = std::chrono::system_clock::now();

//        int const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
//                (std::chrono::system_clock::now() - _physics_elapsed_time).count();

//        _physics_elapsed_time = std::chrono::system_clock::now();

        // FIXME: currently constant update time step, not regarding at all the actually elapsed time
        // some updates are really far away from the set time step, not sure why
        _core.update(_parameters["physics_timestep_ms"]->get_value<int>() / 1000.0f * _parameters["physics_speed"]->get_value<float>());
//        _core.update(elapsed_milliseconds / 1000.0f * _parameters["physics_speed"]->get_value<float>());


        std::chrono::time_point<std::chrono::system_clock> const timer_end = std::chrono::system_clock::now();

        int const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << __PRETTY_FUNCTION__ << " time elapsed: " << elapsed_milliseconds << std::endl;
        }
    }

//    void handle_game_state_change()
//    {
//        if ((_core.get_previous_game_state() == Core::Game_state::Unstarted ||
//             _core.get_previous_game_state() == Core::Game_state::Finished) &&
//                _core.get_game_state() == Core::Game_state::Running)
//        {
//            std::cout << __PRETTY_FUNCTION__ << " starting the game" << std::endl;

//            change_level_state(Level_state::Running);
//        }
//        else if (_core.get_previous_game_state() == Core::Game_state::Running && _core.get_game_state() == Core::Game_state::Finished)
//        {
//            // show score etc. on a screen that allows "replay", "next level" etc.
//            std::cout << __PRETTY_FUNCTION__ << " finishing the game" << std::endl;

//            change_level_state(Level_state::After_finish);

////            QTimer::singleShot(3000, this, SLOT(show_afterstate_ui_elements()));


//        }
//    }

//    void show_afterstate_ui_elements()
//    {
//        for (boost::shared_ptr<Draggable_button> const& button : _buttons[int(Level_state::After_finish)])
//        {
//            button->set_visible(true);
//        }

//        for (boost::shared_ptr<Draggable_label> const& label : _labels[int(Level_state::After_finish)])
//        {
//            label->set_visible(true);
//        }

//        update_draggable_to_level_element();
//        update_active_draggables();
//    }

//    void change_state_to_statistics()
//    {
//        change_level_state(Level_state::Statistics);

//        for (auto & iter : _statistics)
//        {
//            for (auto & stat : iter.second)
//            {
//                stat.reset_animation();
//            }
//        }
//    }

//    Level_state get_level_state() const
//    {
//        return _level_state;
//    }

    QFont const& get_particle_font() const
    {
        return _particle_font;
    }

    QFont const& get_main_font() const
    {
        return _main_font;
    }

    QStringList const& get_level_names() const
    {
        return _level_names;
    }

    std::string const& get_current_level_name() const
    {
        return _current_level_name;
    }

Q_SIGNALS:
    void level_changed(Main_game_screen::Level_state);

private:
    QTimer * _physics_timer;
    std::chrono::time_point<std::chrono::system_clock> _physics_elapsed_time;

    Core _core;

    StandardCamera * _my_camera;

    std::string _current_level_name;

    QStringList _level_names;

    QFont _main_font;
    QFont _particle_font;

    std::deque< std::unique_ptr<Screen> > _screen_stack;
};


#endif
