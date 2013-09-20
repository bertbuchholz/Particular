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
#include "Progress.h"
//#include "State.h"



class My_viewer : public Options_viewer // , public QGLFunctions
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    enum class Mouse_state { None, Init_drag_handle, Init_drag_molecule, Dragging_molecule, Dragging_handle };
    enum class Selection { None, Level_element, Molecule };
    enum class Ui_state { Level_editor, Playing };
    enum class Level_state { Main_menu, Intro, Before_start, Running, After_finish, Statistics };
    enum class Intro_state { Beginning, Single_molecule, Two_molecules_0, Two_molecules_1, Two_molecules_2, Two_molecules_3, Finishing, Finished };

    My_viewer(QGLFormat const& format = QGLFormat()) : Options_viewer(format),
        _mouse_state(Mouse_state::None), _selection(Selection::None), _selected_level_element(nullptr), _level_state(Level_state::Main_menu)
    {
        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

        _parameters.add_parameter(new Parameter("levels", std::string("")));

        _parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&My_viewer::update_physics_timestep, this)));
        _parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update));
        _parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));

        _parameters.add_parameter(new Parameter("z_near", 0.1f, 0.01f, 100.0f, std::bind(&My_viewer::change_clipping, this)));
        _parameters.add_parameter(new Parameter("z_far", 100.0f, 1.0f, 1000.0f, std::bind(&My_viewer::change_clipping, this)));

        _parameters.add_parameter(new Parameter("game_field_left", -40.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));
        _parameters.add_parameter(new Parameter("game_field_right", 40.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));

        _parameters.add_parameter(new Parameter("game_field_front", -20.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));
        _parameters.add_parameter(new Parameter("game_field_back", 20.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));

        _parameters.add_parameter(new Parameter("game_field_bottom", 0.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));
        _parameters.add_parameter(new Parameter("game_field_top", 40.0f, -1000.0f, 1000.0f, std::bind(&My_viewer::change_game_field_borders, this)));

        Parameter_registry<Core>::create_normal_instance("Core", &_parameters, std::bind(&My_viewer::change_core_settings, this));

        Parameter_registry<Level_data>::create_normal_instance("Level_data", &_parameters, std::bind(&My_viewer::change_level_data_settings, this));

        _parameters.add_parameter(new Parameter("draw_closest_force", true, update));

        _parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));

        _parameters.add_parameter(new Parameter("draw_tree_depth", 1, -1, 10, update));

        _parameters.add_parameter(new Parameter("draw_handles", true, update));

        std::vector<std::string> particle_types { "O2", "H2O", "SDS", "Na", "Cl", "Dipole",
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

        _parameters.add_parameter(new Parameter("particle_type", 0, particle_types, update));

        std::vector<std::string> ui_states { "Level_editor", "Playing" };

        _parameters.add_parameter(new Parameter("ui_state", 0, ui_states, std::bind(&My_viewer::change_ui_state, this)));

        _parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&My_viewer::toggle_simulation, this)));
        _parameters.add_parameter(Parameter::create_button("Save level", std::bind(&My_viewer::save_level, this)));
        _parameters.add_parameter(Parameter::create_button("Load level", std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::load_level), this)));
        _parameters.add_parameter(Parameter::create_button("Save settings", std::bind(&My_viewer::save_parameters_with_check, this)));
        _parameters.add_parameter(Parameter::create_button("Load settings", std::bind(&My_viewer::restore_parameters_with_check, this)));
        _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));
        _parameters.add_parameter(Parameter::create_button("Start Level", std::bind(&My_viewer::start_level, this)));
        _parameters.add_parameter(Parameter::create_button("Reset Level", std::bind(&My_viewer::reset_level, this)));
        _parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));
        _parameters.add_parameter(Parameter::create_button("Show cam orientation", std::bind(&My_viewer::print_cam_orientation, this)));

        Parameter_registry<Molecule_renderer>::create_single_select_instance(&_parameters, "Molecule Renderer", std::bind(&My_viewer::change_renderer, this));

        setup_ui_elements();

        change_renderer();
        change_core_settings();
        change_ui_state();

        connect(&_core, SIGNAL(game_state_changed()), this, SLOT(handle_game_state_change()));

        setup_fonts();
    }

    void print_cam_orientation()
    {
        qglviewer::Vec axis = camera()->orientation().axis();
        std::cout << "angle: " << camera()->orientation().angle() << " axis: " << axis[0] << ", " << axis[1] << ", " << axis[2] << " pos: " << QGLV2Eigen(camera()->position()) << std::endl;
    }

    void setup_fonts()
    {
        int id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_qdata_path() +  "/fonts/Matiz.ttf");
        QString family = QFontDatabase::applicationFontFamilies(id).at(0);
        _main_font = QFont(family);

        id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_qdata_path() +  "/fonts/LondrinaOutline-Regular.otf");
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
            filename = QFileDialog::getSaveFileName(this, tr("Save File"),
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
            filename = QFileDialog::getOpenFileName(this, tr("Save File"),
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
        _core.load_level(filename);
        _core.reset_level();

        update_level_element_buttons();

        update_draggable_to_level_element();
        update_active_draggables();
        change_renderer();
        change_core_settings();

        _parameters.get_child(_core.get_level_data().name())->load(_core.get_level_data().get_current_parameters());

        _current_level_name = filename;

        _renderer->update(_core.get_level_data());

        update_game_camera();

        update();
    }

    void restore_parameters() override
    {
        Base::restore_parameters();

        update();
    }

    void start_level()
    {
//        assert(_level_state == Level_state::Before_start);

        _core.start_level();
        set_simulation_state(true);

        update();
    }

    void load_next_level()
    {
        std::cout << __PRETTY_FUNCTION__ << " next level: " << _progress.last_level << std::endl;

        if (_level_names.size() <= _progress.last_level)
        {
            std::cout << "No more levels." << std::endl;
            return;
        }

        set_simulation_state(false);

        std::string const filename = (Data_config::get_instance()->get_qdata_path() + "/levels/" + _level_names[_progress.last_level] + ".data").toStdString();

        load_level(filename);

        reset_level();

        _particle_systems[int(Level_state::Before_start)].clear();
        _particle_systems[int(Level_state::Before_start)].push_back(Targeted_particle_system(3.0f));
        _particle_systems[int(Level_state::Before_start)].back().generate(QString("Level %1").arg(_progress.last_level + 1).toStdString(), _particle_font, QRectF(0.0f, 0.1f, 1.0f, 0.3f));

        change_level_state(Level_state::Before_start);

        update();
    }

    void reset_level()
    {
        _core.reset_level();
        update_draggable_to_level_element();
        update_active_draggables();
        update();
    }

    void update_level_element_buttons()
    {
        int i = 0;

        _buttons[int(Level_state::Running)].clear();

        for (auto const& iter : _core.get_level_data()._available_elements)
        {
            auto const& image_iter = _element_images.find(iter.first);

            if (image_iter != _element_images.end()) continue;

            _element_images[iter.first] = QImage(Data_config::get_instance()->get_qdata_path() +  "/textures/button_" + QString::fromStdString(iter.first) + ".png");
        }

        for (auto const& iter : _core.get_level_data()._available_elements)
        {
            if (iter.second > 0)
            {
                Eigen::Vector3f pos(0.05f + i * 0.06f, 0.95f, 0.0f);
                Eigen::Vector2f size(0.04f, 0.04f * aspectRatio());

                boost::shared_ptr<Draggable_button> button(new Draggable_button(pos, size, "", std::bind(&My_viewer::element_button_pressed, this, std::placeholders::_1), iter.first));

//                QImage button_img(100, 100, QImage::Format_ARGB32);
//                button_img.fill(Qt::black);

//                button->set_texture(bindTexture(img));

                QImage button_img = _element_images[iter.first];

                QPainter p(&button_img);

                QFont font = _main_font;
        //        font.setWeight(QFont::Bold);
                font.setPixelSize(100);
//                font.setPointSizeF(20.0f);
                p.setFont(font);

                p.setPen(QColor(0, 0, 0));

                p.drawText(25, 200, QString("%1").arg(iter.second));

//                p.drawText(QRect(5, 5, 90, 90), Qt::AlignBottom | Qt::AlignRight, QString("%1").arg(iter.second));

                deleteTexture(button->get_texture());
                button->set_texture(bindTexture(button_img));

                _buttons[int(Level_state::Running)].push_back(button);

                ++i;
            }
        }

        update_draggable_to_level_element();
        update_active_draggables();
    }

    void element_button_pressed(std::string const& type)
    {
        _core.get_level_data()._available_elements[type] -= 1;

        float top_pos = _core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2];

        Eigen::Vector3f const position(0.0f, 0.0f, top_pos + 10.0f);

        add_element(position, type);

        update_level_element_buttons();
    }

    void change_game_field_borders()
    {
        Eigen::Vector3f min(_parameters["game_field_left"]->get_value<float>(),
                _parameters["game_field_front"]->get_value<float>(),
                _parameters["game_field_bottom"]->get_value<float>());

        Eigen::Vector3f max(_parameters["game_field_right"]->get_value<float>(),
                _parameters["game_field_back"]->get_value<float>(),
                _parameters["game_field_top"]->get_value<float>());

        _core.set_game_field_borders(min, max);
        update_draggable_to_level_element();
        update_active_draggables();
        update();
    }

    void change_renderer()
    {
        _renderer = std::unique_ptr<Molecule_renderer>(Parameter_registry<Molecule_renderer>::get_class_from_single_select_instance_2(_parameters.get_child("Molecule Renderer")));
        _renderer->init(context(), size());
        _renderer->update(_core.get_level_data());
        update();
    }

    void change_core_settings()
    {
        _core.set_parameters(*_parameters.get_child("Core"));
        update();
    }

    void change_level_data_settings()
    {
        _core.get_level_data().set_parameters(*_parameters.get_child("Level_data"));
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
        if (_parameters["ui_state"]->get_value<std::string>() == "Level_editor")
        {
            _ui_state = Ui_state::Level_editor;

            camera()->frame()->setConstraint(nullptr);

            _particle_systems.clear();
        }
        else if (_parameters["ui_state"]->get_value<std::string>() == "Playing")
        {
            _ui_state = Ui_state::Playing;

//            update_game_camera();
            init_game();
        }

        update_draggable_to_level_element();
        update_active_draggables();
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

        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);

//        _picking.init(context(), size()); // FIXME: needs to be resized when viewer changes
        _picking.init(context());

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

        _rotate_tex = bindTexture(QImage(Data_config::get_instance()->get_qdata_path() + "/textures/rotate.png"));
        _move_tex = bindTexture(QImage(Data_config::get_instance()->get_qdata_path() + "/textures/move.png"));
        _scale_tex = bindTexture(QImage(Data_config::get_instance()->get_qdata_path() + "/textures/scale.png"));
        _slider_tex = bindTexture(QImage(Data_config::get_instance()->get_qdata_path() + "/textures/slider.png"));
        _particle_tex = bindTexture(QImage(Data_config::get_instance()->get_qdata_path() +  "/textures/particle.png"));

        generate_ui_textures();

        restore_parameters();
//        init_game();

        update_draggable_to_level_element();
        update_active_draggables();

        startAnimation();

        _parameters["ui_state"]->set_value(std::string("Playing"));
    }

    void init_game()
    {
//        restore_parameters();

//        _parameters["ui_state"]->set_value(std::string("Playing"));

        _particle_systems[int(_level_state)].clear();
        _particle_systems[int(_level_state)].push_back(Targeted_particle_system(3.0f));
        _particle_systems[int(_level_state)].back().generate("I NEED A NAME", _particle_font, QRectF(0.0f, 0.6f, 1.0f, 0.3f));

        QString const level_string = QString::fromStdString(_parameters["levels"]->get_value<std::string>());
        _level_names = level_string.split(",", QString::SkipEmptyParts);

        load_progress();

        float z = 0.0f;

        if (_core.get_level_data()._game_field_borders.size() > 0)
        {
            z = 0.5f * (_core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
                    + _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2]);
        }

        _my_camera->setUpVector(qglviewer::Vec(0.0f, 0.0f, 1.0f));
        _my_camera->setViewDirection(qglviewer::Vec(0.0f, 1.0f, 0.0f));
        _my_camera->setPosition(qglviewer::Vec(0.0f, -80.0f, z));
    }

    void draw_molecules_for_picking()
    {
        for (Molecule const& molecule : _core.get_molecules())
        {
            int const index = molecule.get_id();
            _picking.set_index(index);

            for (Atom const& atom : molecule._atoms)
            {
                if (atom._type == Atom::Type::Charge) continue;

                glPushMatrix();

                glTranslatef(atom.get_position()[0], atom.get_position()[1], atom.get_position()[2]);

//                float radius = scale * atom._radius;
                float radius = atom._radius;
                glScalef(radius, radius, radius);

                _icosphere.draw();

                glPopMatrix();
            }
        }
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
        setup_gl_points(true);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        _renderer->render(_core.get_level_data(), _core.get_current_time(), _my_camera);

        // debug displays

//        draw_tree();

//        draw_tree_for_point(Eigen::Vector3f(0.0f, 0.0f, 0.0f));

        setup_gl_points(false);
        glPointSize(8.0f);

        glDisable(GL_LIGHTING);
        draw_draggables();

        for (Targeted_particle_system const& s : _particle_systems[int(_level_state)])
        {
            draw_particle_system(s);
        }

//        draw_indicators();

//        draw_user_force();

//        draw_closest_force();

//        draw_temperature();
    }

    void draw_textured_quad(GLuint const tex_id)
    {
        glBindTexture(GL_TEXTURE_2D, tex_id);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void draw_draggables() // FIXME: use visitors or change it so that draggables can only have a single type of handles (Draggable_point)
    {
        glDisable(GL_DEPTH_TEST);

        float const scale = 1.5f;

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        float const z_offset = -0.01f;

        for (auto const& d : _draggable_to_level_element)
        {
            if (Draggable_box const* d_box = dynamic_cast<Draggable_box const*>(d.first))
            {
                std::vector<Draggable_point> const& corners = d_box->get_corners();

                Level_element::Edit_type edit_type = d.second->is_user_editable();

//                draw_box(d_box->get_min(), d_box->get_max());

                glPushMatrix();

                glTranslatef(d_box->get_position()[0], d_box->get_position()[1], d_box->get_position()[2]);
                glMultMatrixf(d_box->get_transform().data());

                if (_ui_state == Ui_state::Level_editor || (int(edit_type) & int(Level_element::Edit_type::Scale)))
                {
                    for (Draggable_point const& p : corners)
                    {
                        //                    glColor3f(0.7f, 0.7f, 0.7f);
                        //                    draw_box_from_center(p.get_position(), Eigen::Vector3f(scale, scale, scale));

                        glPushMatrix();

                        glTranslatef(p.get_position()[0] + z_offset, p.get_position()[1] + z_offset, p.get_position()[2] + z_offset);
                        glScalef(scale, scale, scale);
                        glRotatef(90, 1.0, 0.0, 0.0);
                        draw_textured_quad(_scale_tex);

                        glPopMatrix();
                    }
                }

                if (_ui_state == Ui_state::Level_editor || (int(edit_type) & int(Level_element::Edit_type::Translate)))
                {
                    for (Draggable_disc const& d_disc : d_box->get_position_points())
                    {
                        //                    draw_sphere_ico(Eigen2OM(d_disc.get_position()), 2.0f, Color(0.7f, 0.7f, 1.0f));

                        glPushMatrix();

                        glTranslatef(d_disc.get_position()[0] + z_offset, d_disc.get_position()[1] + z_offset, d_disc.get_position()[2] + z_offset);
                        glScalef(scale, scale, scale);
                        glRotatef(90, 1.0, 0.0, 0.0);
                        draw_textured_quad(_move_tex);

                        glPopMatrix();
                    }
                }

                if (_ui_state == Ui_state::Level_editor || (int(edit_type) & int(Level_element::Edit_type::Rotate)))
                {
                    for (Draggable_disc const& d_disc : d_box->get_rotation_handles())
                    {
                        //                    draw_sphere_ico(Eigen2OM(d_disc.get_position()), 2.0f, Color(1.0f, 0.7f, 0.7f));
                        glPushMatrix();

                        glTranslatef(d_disc.get_position()[0] + z_offset, d_disc.get_position()[1] + z_offset, d_disc.get_position()[2] + z_offset);
                        glScalef(scale, scale, scale);
                        glRotatef(90, 1.0, 0.0, 0.0);
                        draw_textured_quad(_rotate_tex);

                        glPopMatrix();
                    }
                }

                if (_ui_state == Ui_state::Level_editor || (int(edit_type) & int(Level_element::Edit_type::Property)))
                {
                    for (auto iter : d_box->get_property_handles())
                    {
                        //                    glColor3f(0.8f, 0.8f, 0.8f);
                        //                    draw_box_from_center(iter.second.get_position(), Eigen::Vector3f(scale, scale, scale));

                        Eigen::Vector3f const& p = iter.second.get_position();

                        glPushMatrix();

                        glTranslatef(p[0] + z_offset, p[1] - 0.03f, p[2] + z_offset);
                        glScalef(0.9f, 0.9f, 0.9f);
                        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                        draw_textured_quad(_slider_tex);

                        glPopMatrix();
                    }
                }

                glPopMatrix();
            }
        }


        if (_ui_state != Ui_state::Level_editor)
        {
            start_normalized_screen_coordinates();

            for (boost::shared_ptr<Draggable_button> const& button : _buttons[int(_level_state)])
            {
                if (button->is_visible())
                {
                    draw_button(button.get(), false);
                }
            }

            for (boost::shared_ptr<Draggable_label> const& label : _labels[int(_level_state)])
            {
                if (label->is_visible())
                {
                    draw_label(label.get());
                }
            }

            for (auto const& stat : _statistics[int(_level_state)])
            {
                if (stat.is_visible())
                {
                    draw_statistic(stat);
                }
            }

            stop_normalized_screen_coordinates();
        }

        glEnable(GL_DEPTH_TEST);
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

    void draw_draggables_for_picking()
    {
        float const scale = 1.5f;

        for (int i = 0; i < int(_active_draggables.size()); ++i)
        {
            Draggable const* draggable = _active_draggables[i];

            glPushMatrix();

            Draggable const* parent = draggable->get_parent();

            glTranslatef(parent->get_position()[0], parent->get_position()[1], parent->get_position()[2]);
            glMultMatrixf(parent->get_transform().data());

            _picking.set_index(i);

            if (Draggable_point const* d_point = dynamic_cast<Draggable_point const*>(draggable))
            {
                draw_box_from_center(d_point->get_position(), Eigen::Vector3f(scale, scale, scale));
            }
            else if (Draggable_disc const* d_point = dynamic_cast<Draggable_disc const*>(draggable))
            {
                draw_sphere_ico(Eigen2OM(d_point->get_position()), 2.0f);
            }
            else if (Draggable_button const* d_button = dynamic_cast<Draggable_button const*>(draggable))
            {
                start_normalized_screen_coordinates();
                draw_button(d_button, true);
                stop_normalized_screen_coordinates();
            }

            glPopMatrix();
        }
    }

    void draw_particle_system(Targeted_particle_system const& system)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(0.0f, 1.0f, 0.0f, 1.0, 1.0f, -1.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glTranslatef(0.5f, 0.5f, 0.0f);

        glScalef(0.5f, 0.5f, 1.0f);

//        glBindTexture(GL_TEXTURE_2D, _particle_tex);

        glPointSize(4.0f * (height()) / (768.0f));

        glBegin(GL_POINTS);

        for (Targeted_particle const& p : system.get_particles())
        {
            glColor3fv(p.color.data());
            glVertex3fv(p.position.data());
        }

        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void draw_portals()
    {
        Level_element_draw_visitor v;

        for (Portal * element : _core.get_portals())
        {
            element->accept(&v);
        }
    }

    void draw_molecule_releasers()
    {
        Level_element_draw_visitor v;

        for (Molecule_releaser * element : _core.get_molecule_releasers())
        {
            element->accept(&v);
        }
    }

    void draw_temperature()
    {
        Eigen::Vector2f grid_start(-40.0f, 0.0f);
        Eigen::Vector2f grid_end  ( 40.0f, 40.0f);

        float resolution = 1.0f;

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float z = grid_start[1]; z < grid_end[1]; z += resolution)
            {
                float factor = 0.0f;

                Eigen::Vector3f pos(x, 0, z);

                for (Brownian_element const* element : _core.get_brownian_elements())
                {
                    factor += element->get_brownian_motion_factor(pos);
                }

                float const max_strength = 50.0f;

                float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

                Color c(strength, 0.0f, 1.0f - strength);

                glColor3fv(c.data());

                glBegin(GL_POINTS);
                glVertex3fv(pos.data());
                glEnd();
            }
        }
    }

    void draw_brownian_elements()
    {
        Level_element_draw_visitor v;

        for (Brownian_element * element : _core.get_brownian_elements())
        {
            element->accept(&v);
        }
    }

    void draw_barriers()
    {
        Level_element_draw_visitor v;

        for (Barrier * barrier : _core.get_barriers())
        {
            barrier->accept(&v);
        }
    }

    void draw_user_force() const
    {
        Molecule_external_force const& force = _core.get_user_force();

        if (force._end_time > _core.get_current_time())
        {
            glBegin(GL_LINES);

            glVertex3fv(force._origin.data());
            glVertex3fv(Eigen::Vector3f(force._origin + force._force).data());

            glEnd();

            glPushMatrix();

            glTranslatef(force._origin[0], force._origin[1], force._origin[2]);

            //                float radius = scale * atom._radius;
            float const radius = 0.2f;
            glScalef(radius, radius, radius);

            _icosphere.draw();

            glPopMatrix();
        }
    }

    void draw_indicators() const
    {
        glLineWidth(1.0f);

        if (_parameters["Core/use_indicators"]->get_value<bool>())
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

    void add_element_event(QPoint const& position)
    {
        qglviewer::Vec qglv_origin;
        qglviewer::Vec qglv_dir;

        camera()->convertClickToLine(position, qglv_origin, qglv_dir);

        Vec origin = QGLV2OM(qglv_origin);
        Vec dir    = QGLV2OM(qglv_dir);

        float const t = ray_plane_intersection(origin, dir, Vec(0.0f, 0.0f, 0.0f), Vec(0.0f, 1.0f, 0.0f));

        if (t > 0)
        {
            Eigen::Vector3f const intersect_pos = OM2Eigen(origin + t * dir);

            std::string const element_type = _parameters["particle_type"]->get_value<std::string>();

            add_element(intersect_pos, element_type);
        }
    }

    void add_element(Eigen::Vector3f const& position, std::string const& element_type)
    {
        float front_pos = _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Y]->get_position()[1];
        float back_pos  = _core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Y]->get_position()[1];

        if (Molecule::molecule_exists(element_type))
        {
            _core.add_molecule(Molecule::create(element_type, position));
        }
        else if (element_type == std::string("Box_barrier"))
        {
            float const strength = 10000.0f;
            float const radius   = 2.0f;

            _core.add_barrier(new Box_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius));
        }
        else if (element_type == std::string("Moving_box_barrier"))
        {
            float const strength = 10000.0f;
            float const radius   = 2.0f;

            _core.add_barrier(new Moving_box_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius));
        }
        else if (element_type == std::string("Blow_barrier"))
        {
            float const strength = 10000.0f;
            float const radius   = 2.0f;

            Blow_barrier * e = new Blow_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position,
                                                               Blow_barrier::Axis::X, 30.0f,
                                                               strength, radius);
            e->set_user_editable(Level_element::Edit_type::All);
            _core.add_barrier(e);
        }
        else if (element_type == std::string("Plane_barrier"))
        {
            float const strength = 10000.0f;
            float const radius   = 5.0f;

            _core.add_barrier(new Plane_barrier(position, Eigen::Vector3f::UnitZ(), strength, radius, Eigen::Vector2f(10.0f, 20.0)));
        }
        else if (element_type == std::string("Brownian_box"))
        {
            float const strength = 10.0f;
            float const radius   = 25.0f;

            Brownian_box * e = new Brownian_box(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius);
            e->set_user_editable(Level_element::Edit_type::All);
            e->set_persistent(false);
            _core.add_brownian_element(e);
        }
        else if (element_type == std::string("Box_portal"))
        {
            _core.add_portal(new Box_portal(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position));
        }
        else if (element_type == std::string("Sphere_portal"))
        {
            _core.add_portal(new Sphere_portal(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position));
        }
        else if (element_type == std::string("Molecule_releaser"))
        {
            Molecule_releaser * m = new Molecule_releaser(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, 1.0f, 1.0f);
            m->set_molecule_type("H2O");
            _core.add_molecule_releaser(m);
        }
        else if (element_type == std::string("Atom_cannon"))
        {
            Atom_cannon * m = new Atom_cannon(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, 1.0f, 1.0f, 10.0f, 0.0f);
            _core.add_molecule_releaser(m);
        }
        else if (element_type == std::string("Charged_barrier"))
        {
            float const strength = 10000.0f;
            float const radius   = 2.0f;

            Charged_barrier * b = new Charged_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + position, strength, radius, 10.0f);
            _core.add_barrier(b);
        }
        else if (element_type == std::string("Tractor_barrier"))
        {
            float const strength = 10000.0f;

            Tractor_barrier * b = new Tractor_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + position, strength);
            b->set_user_editable(Level_element::Edit_type::All);
            b->set_persistent(false);
            _core.add_barrier(b);
        }
        else
        {
            std::cout << __PRETTY_FUNCTION__ << " element does not exist: " << element_type << std::endl;
        }

        update_draggable_to_level_element();
        update_active_draggables();
    }

    void mousePressEvent(QMouseEvent *event)
    {
        bool handled = false;

        if (_level_state == Level_state::Intro)
        {
            handled = true;
        }
        else if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier)
        {
            add_element_event(event->pos());
            handled = true;
        }
        else if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::AltModifier)
        {
            std::cout << __PRETTY_FUNCTION__ << " Drag/Click" << std::endl;
            _dragging_start = event->pos();

//            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&My_viewer::draw_molecules_for_picking, this));
            _picked_index = _picking.do_pick(event->pos().x() / float(camera()->screenWidth()), (height() - event->pos().y())  / float(camera()->screenHeight()), std::bind(&My_viewer::draw_molecules_for_picking, this));
//            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&Molecule_renderer::picking_draw, _molecule_renderer));

            std::cout << __PRETTY_FUNCTION__ << " index: " << _picked_index << std::endl;

            if (_picked_index != -1)
            {
                bool found;
                qglviewer::Vec world_pos = camera()->pointUnderPixel(event->pos(), found);

                if (found)
                {
                    boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

                    assert(picked_molecule);

                    Molecule_external_force & f = _core.get_user_force();
                    f._molecule_id = _picked_index;
//                    qglviewer::Vec dir = world_pos - camera()->position();
//                    dir.normalize();
                    f._force.setZero();
                    f._origin = QGLV2Eigen(world_pos);
                    f._local_origin = picked_molecule->_R.transpose() * (f._origin - picked_molecule->_x);
                    f._end_time = _core.get_current_time();

                    _mouse_state = Mouse_state::Init_drag_molecule;

                    std::cout << "Apply force: " << f._origin << std::endl;
                }
            }

            handled = true;
        }
        else
        {
            _picked_index = _picking.do_pick(event->pos().x() / float(camera()->screenWidth()), (height() - event->pos().y())  / float(camera()->screenHeight()),
                                             std::bind(&My_viewer::draw_draggables_for_picking, this));

//            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(),
//                      std::bind(&My_viewer::draw_draggables_for_picking, this));

            std::cout << __PRETTY_FUNCTION__ << " picked_index: " << _picked_index << std::endl;

            if (_picked_index != -1)
            {
                _dragging_start = event->pos();

                bool found;
                qglviewer::Vec world_pos = camera()->pointUnderPixel(event->pos(), found);

                _mouse_state = Mouse_state::Init_drag_handle;
                std::cout << __PRETTY_FUNCTION__ << " Init_drag_handle" << std::endl;

                if (found)
                {
                    _dragging_start_3d = QGLV2Eigen(world_pos);
                }

                handled = true;
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

        if (_mouse_state == Mouse_state::Init_drag_molecule)
        {
                if (_picked_index != -1 && (_dragging_start - event->pos()).manhattanLength() > 0)
                {
                    _mouse_state = Mouse_state::Dragging_molecule;
                }
        }
        else if (_mouse_state == Mouse_state::Dragging_molecule)
        {
            boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

            assert(picked_molecule);

            Molecule_external_force & f = _core.get_user_force();

            f._plane_normal = -1.0f * QGLV2Eigen(camera()->viewDirection());

            Eigen::Hyperplane<float, 3> view_plane(f._plane_normal, f._origin);

            qglviewer::Vec qglv_origin; // camera pos
            qglviewer::Vec qglv_dir;    // origin - camera_pos

            camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

            Eigen::Vector3f origin = QGLV2Eigen(qglv_origin);
            Eigen::Vector3f dir    = QGLV2Eigen(qglv_dir).normalized();

            Eigen::ParametrizedLine<float, 3> line(origin, dir);

            Eigen::Vector3f new_force_target = line.intersectionPoint(view_plane);

            f._origin = picked_molecule->_R * f._local_origin + picked_molecule->_x;
            //                f._force = 1.0f * (new_force_target - f._origin).normalized();
            f._force = new_force_target - f._origin;
            f._end_time = _core.get_current_time() + 0.1f;

            handled = true;
        }
        else if (_mouse_state == Mouse_state::Init_drag_handle && _active_draggables[_picked_index]->is_draggable())
        {
            if (_picked_index != -1 && (_dragging_start - event->pos()).manhattanLength() > 0)
            {
                _mouse_state = Mouse_state::Dragging_handle;
            }
        }
        else if (_mouse_state == Mouse_state::Dragging_handle) // TODO: currently has Y plane constraint, move constraints into Draggable, consider giving it the viewline instead of a single position
        {
            Eigen::Hyperplane<float, 3> y_plane(Eigen::Vector3f(0.0f, 1.0f, 0.0f), _active_draggables[_picked_index]->get_position());

            qglviewer::Vec qglv_origin; // camera pos
            qglviewer::Vec qglv_dir;    // normalize(origin - camera_pos)

            camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

            Eigen::ParametrizedLine<float, 3> view_ray(QGLV2Eigen(qglv_origin), QGLV2Eigen(qglv_dir).normalized());

            Eigen::Vector3f new_position = view_ray.intersectionPoint(y_plane);

            Draggable * parent = _active_draggables[_picked_index]->get_parent();

            auto iter = _draggable_to_level_element.find(parent);

            assert(iter != _draggable_to_level_element.end());

            Level_element * level_element = iter->second;

//            if (!check_for_collision(level_element))
            {
                _active_draggables[_picked_index]->set_position_from_world(new_position);
                _active_draggables[_picked_index]->update();

                std::cout << __PRETTY_FUNCTION__ << ": " << parent << std::endl;

                level_element->accept(parent);

                update();
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
        if (_mouse_state == Mouse_state::Init_drag_molecule)
        {
            std::cout << __PRETTY_FUNCTION__ << " click on molecule" << std::endl;

            if (_picked_index != -1)
            {
                _selection = Selection::Molecule;

                Molecule_external_force & f = _core.get_user_force();

                f._end_time = _core.get_current_time() + 0.5f;
            }
        }
        else if (_mouse_state == Mouse_state::Init_drag_handle)
        {
            std::cout << __PRETTY_FUNCTION__ << " click on handle" << std::endl;

            if (_picked_index != -1)
            {
                _selection = Selection::Level_element;
                Draggable * parent = _active_draggables[_picked_index]->get_parent();
                parent->clicked();

                auto iter = _draggable_to_level_element.find(parent);
                if (iter != _draggable_to_level_element.end())
                {
                    _selected_level_element = iter->second;
                    _selected_level_element->set_selected(true);

                    if (event->button() == Qt::RightButton && _ui_state == Ui_state::Level_editor)
                    {
                        show_context_menu_for_element();
                    }
                }
            }
        }
        else
        {
            if (_selected_level_element)
            {
                _selected_level_element->set_selected(false);
                _selected_level_element = nullptr;
            }

            _selection = Selection::None;

            Base::mouseReleaseEvent(event);
        }

        _mouse_state = Mouse_state::None;
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) && _ui_state == Ui_state::Level_editor)
        {
            delete_selected_element();
        }
        else if (event->key() == Qt::Key_F8)
        {
            _particle_systems[int(_level_state)].push_back(Targeted_particle_system(3.0f));
            _particle_systems[int(_level_state)].back().generate("Blubber", _particle_font, QRectF(0.0f, 0.5f, 1.0f, 0.3f));
        }
        else if (_ui_state == Ui_state::Playing && event->key() == Qt::Key_Escape)
        {
            if (_level_state == Level_state::Intro) // skip intro
            {
                camera()->deletePath(0);

                load_next_level();
            }
            else if (_level_state == Level_state::Running) // go to pause menu
            {

            }
        }
        else
        {
            Base::keyPressEvent(event);
        }
    }

    void delete_selected_element()
    {
        if (_selection == Selection::Molecule)
        {

        }
        else if (_selection == Selection::Level_element)
        {
            Draggable * parent = _active_draggables[_picked_index]->get_parent();
            assert(_draggable_to_level_element.find(parent) != _draggable_to_level_element.end());

            _core.delete_level_element(_draggable_to_level_element.find(parent)->second);

            update_draggable_to_level_element();
            update_active_draggables();

            _selection = Selection::None;
            _selected_level_element = nullptr;

            update();
        }
    }

    class Widget_text_combination : public QWidget
    {
        public:
        Widget_text_combination(QString const& text, QWidget * widget)
        {
            QHBoxLayout * layout = new QHBoxLayout;
            layout->addWidget(new QLabel(text));
            layout->addWidget(widget);
            setLayout(layout);
        }
    };

    void show_context_menu_for_element()
    {
        Draggable * parent = _active_draggables[_picked_index]->get_parent();

        assert(_draggable_to_level_element.find(parent) != _draggable_to_level_element.end());

        std::cout << __PRETTY_FUNCTION__ << ": " << parent << std::endl;

        Level_element * element = _draggable_to_level_element[parent];

        QMenu menu;

        QWidgetAction * action_persistent = new QWidgetAction(this);
        QCheckBox * persistent_checkbox = new QCheckBox("Persistent");
        persistent_checkbox->setChecked(element->is_persistent());
        action_persistent->setDefaultWidget(persistent_checkbox);

        QWidgetAction * action_user_translate = new QWidgetAction(this);
        QCheckBox * user_translate_checkbox = new QCheckBox("Translate");
        user_translate_checkbox->setChecked(int(element->is_user_editable()) & int(Level_element::Edit_type::Translate));
        action_user_translate->setDefaultWidget(user_translate_checkbox);

        QWidgetAction * action_user_scale = new QWidgetAction(this);
        QCheckBox * user_scale_checkbox = new QCheckBox("Scale");
        user_scale_checkbox->setChecked(int(element->is_user_editable()) & int(Level_element::Edit_type::Scale));
        action_user_scale->setDefaultWidget(user_scale_checkbox);

        QWidgetAction * action_user_rotate = new QWidgetAction(this);
        QCheckBox * user_rotate_checkbox = new QCheckBox("Rotate");
        user_rotate_checkbox->setChecked(int(element->is_user_editable()) & int(Level_element::Edit_type::Rotate));
        action_user_rotate->setDefaultWidget(user_rotate_checkbox);

        QWidgetAction * action_user_property = new QWidgetAction(this);
        QCheckBox * user_property_checkbox = new QCheckBox("Property");
        user_property_checkbox->setChecked(int(element->is_user_editable()) & int(Level_element::Edit_type::Property));
        action_user_property->setDefaultWidget(user_property_checkbox);

        menu.addAction(action_persistent);
        menu.addAction(action_user_rotate);
        menu.addAction(action_user_scale);
        menu.addAction(action_user_translate);
        menu.addAction(action_user_property);

        if (Moving_box_barrier * b = dynamic_cast<Moving_box_barrier *>(element))
        {
            QWidgetAction * action_duration = new QWidgetAction(this);
            FloatSlider * slider = new FloatSlider(0.1f, 10.0f, b->get_animation().get_duration());
//            action_duration->setDefaultWidget(slider);
            action_duration->setDefaultWidget(new Widget_text_combination("Duration", slider));

            menu.addAction("Set startpoint");
            menu.addAction("Set endpoint");
            menu.addAction(action_duration);

            QAction * selected_action = menu.exec(QCursor::pos());

            if (selected_action)
            {
                if (selected_action->text() == "Set startpoint")
                {
                    b->get_animation().set_start_point(b->get_position());
                }
                else if (selected_action->text() == "Set endpoint")
                {
                    b->get_animation().set_end_point(b->get_position());
                }
            }

            b->get_animation().set_duration(slider->getValueF());

            delete action_duration;
        }
        else if (Molecule_releaser * m = dynamic_cast<Molecule_releaser *>(element))
        {
            QWidgetAction * action_num_max_molecules = new QWidgetAction(this);

            QSpinBox * spinbox_num_max_molecules = new QSpinBox();
            {
                spinbox_num_max_molecules->setMaximum(1000);
                spinbox_num_max_molecules->setMinimum(1);
                spinbox_num_max_molecules->setValue(m->get_num_max_molecules());

                action_num_max_molecules->setDefaultWidget(new Widget_text_combination("Max. Molecules", spinbox_num_max_molecules));
            }

            QWidgetAction * action_interval = new QWidgetAction(this);

            QDoubleSpinBox * spinbox_interval = new QDoubleSpinBox();
            {
                spinbox_interval->setRange(0.1f, 10.0f);
                spinbox_interval->setValue(m->get_interval());

                action_interval->setDefaultWidget(new Widget_text_combination("Rel. Interval (s)", spinbox_interval));
            }

            QWidgetAction * action_first_release = new QWidgetAction(this);

            QDoubleSpinBox * spinbox_first_release = new QDoubleSpinBox();
            {
                spinbox_first_release->setRange(0.0f, 10000.0f);
                spinbox_first_release->setValue(m->get_first_release());

                action_first_release->setDefaultWidget(new Widget_text_combination("First release", spinbox_first_release));
            }

            QWidgetAction * action_molecule_type = new QWidgetAction(this);
            QComboBox * combo_molecule_type = new QComboBox();
            {
                std::vector<std::string> molecule_names = Molecule::get_molecule_names();

                for (std::string const& name : molecule_names)
                {
                    combo_molecule_type->addItem(QString::fromStdString(name));

                }

                combo_molecule_type->setCurrentIndex(combo_molecule_type->findText(QString::fromStdString(m->get_molecule_type())));

                action_molecule_type->setDefaultWidget(new Widget_text_combination("Molecule Type", combo_molecule_type));
            }

            menu.addAction(action_molecule_type);
            menu.addAction(action_num_max_molecules);
            menu.addAction(action_interval);
            menu.addAction(action_first_release);

            menu.exec(QCursor::pos());

            m->set_molecule_type(combo_molecule_type->currentText().toStdString());
            m->set_num_max_molecules(spinbox_num_max_molecules->value());
            m->set_interval(spinbox_interval->value());
            m->set_first_release(spinbox_first_release->value());

            delete action_molecule_type;
            delete action_num_max_molecules;
            delete action_interval;
            delete action_first_release;
        }
        else if (Portal * m = dynamic_cast<Portal*>(element))
        {
            QWidgetAction * action_num_min_captured_molecules = new QWidgetAction(this);

            QSpinBox * spinbox_num_min_captured_molecules = new QSpinBox();
            {
                spinbox_num_min_captured_molecules->setMinimum(1);
                spinbox_num_min_captured_molecules->setMaximum(1000);
                spinbox_num_min_captured_molecules->setValue(m->get_condition().get_min_captured_molecules());

                action_num_min_captured_molecules->setDefaultWidget(new Widget_text_combination("Captured Molecules", spinbox_num_min_captured_molecules));
            }

            QWidgetAction * action_type = new QWidgetAction(this);

            QComboBox * combo_type = new QComboBox();
            {
                combo_type->addItem("And");
                combo_type->addItem("Or");

                if (m->get_condition().get_type() == End_condition::Type::And)
                {
                    combo_type->setCurrentIndex(0);
                }
                else
                {
                    combo_type->setCurrentIndex(1);
                }

                action_type->setDefaultWidget(new Widget_text_combination("Type", combo_type));
            }

            QWidgetAction * action_score_factor = new QWidgetAction(this);
            QDoubleSpinBox * spinbox_score_factor = new QDoubleSpinBox();
            {
                spinbox_score_factor->setMinimum(1);
                spinbox_score_factor->setMaximum(100);
                spinbox_score_factor->setValue(m->get_score_factor());

                action_score_factor->setDefaultWidget(new Widget_text_combination("Score Factor", spinbox_score_factor));
            }

            QWidgetAction * action_destroy_on_enter = new QWidgetAction(this);
            QCheckBox * checkbox_destroy_on_enter = new QCheckBox("Destroy on enter");
            {
                checkbox_destroy_on_enter->setChecked(m->do_destroy_on_entering());
                action_destroy_on_enter->setDefaultWidget(checkbox_destroy_on_enter);
            }

            menu.addAction(action_score_factor);
            menu.addAction(action_destroy_on_enter);
            menu.addAction(action_num_min_captured_molecules);
            menu.addAction(action_type);
            menu.addAction(QString("Collected Molecules: %1").arg(m->get_condition().get_num_captured_molecules()));

            menu.exec(QCursor::pos());

            m->get_condition().set_min_captured_molecules(spinbox_num_min_captured_molecules->value());

            if (combo_type->currentText() == "Or")
            {
                m->get_condition().set_type(End_condition::Type::Or);
            }
            if (combo_type->currentText() == "And")
            {
                m->get_condition().set_type(End_condition::Type::And);
            }

            m->set_score_factor(spinbox_score_factor->value());
            m->set_destroy_on_entering(checkbox_destroy_on_enter->isChecked());

            delete action_score_factor;
            delete action_num_min_captured_molecules;
            delete action_type;
            delete action_destroy_on_enter;
        }
        else
        {
            menu.exec(QCursor::pos());
        }

        element->set_persistent(persistent_checkbox->isChecked());

        int edit_type = int(Level_element::Edit_type::None);

        if (user_rotate_checkbox->isChecked()) edit_type |= int(Level_element::Edit_type::Rotate);
        if (user_translate_checkbox->isChecked()) edit_type |= int(Level_element::Edit_type::Translate);
        if (user_scale_checkbox->isChecked()) edit_type |= int(Level_element::Edit_type::Scale);
        if (user_property_checkbox->isChecked()) edit_type |= int(Level_element::Edit_type::Property);

        element->set_user_editable(Level_element::Edit_type(edit_type));

        delete action_persistent;
        delete action_user_rotate;
        delete action_user_scale;
        delete action_user_translate;
        delete action_user_property;
    }

    void do_physics_timestep()
    {
        update_physics();

        update();
    }

    void animate() override
    {
        if (_mouse_state == Mouse_state::Dragging_molecule)
        {
            boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

            assert(picked_molecule);

            Molecule_external_force & f = _core.get_user_force();

            Eigen::Vector3f old_force_target = f._origin + f._force;

            f._origin = picked_molecule->_R * f._local_origin + picked_molecule->_x;
            f._force = old_force_target - f._origin;
            f._end_time = _core.get_current_time() + 0.1f;
        }

        for (Targeted_particle_system & p : _particle_systems[int(_level_state)])
        {
            p.animate(animationPeriod() / 1000.0f);
        }

        for (auto & stat : _statistics[int(_level_state)])
        {
            stat.animate(animationPeriod() / 1000.0f);
        }

        for (auto & l : _labels[int(_level_state)])
        {
            l->animate(animationPeriod() / 1000.0f);
        }

        if (_level_state == Level_state::Intro)
        {
            update_intro(animationPeriod() / 1000.0f);
        }

        Base::animate();
    }

    void update_active_draggables()
    {
        _active_draggables.clear();

        for (auto const& d : _draggable_to_level_element)
        {
            Level_element::Edit_type edit_type = Level_element::Edit_type::All;

            if (_ui_state != Ui_state::Level_editor)
            {
                edit_type = d.second->is_user_editable();
            }

            std::vector<Draggable*> const draggables = d.first->get_draggables(edit_type);
            std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
        }

        if (_ui_state != Ui_state::Level_editor)
        {
            for (boost::shared_ptr<Draggable_button> const& button : _buttons[int(_level_state)])
            {
                Draggable_button * b = button.get();

                std::vector<Draggable*> const draggables = b->get_draggables(Level_element::Edit_type::None);
                std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
            }
        }
    }

    void update_draggable_to_level_element()
    {
        for (auto & d : _draggable_to_level_element)
        {
            delete d.first;
        }

        _draggable_to_level_element.clear();

        if (_level_state == Level_state::Running || _ui_state == Ui_state::Level_editor)
        {
            for (boost::shared_ptr<Level_element> const& element : _core.get_level_data()._level_elements)
            {
                Level_element * e = element.get();

                if (Brownian_box const* b = dynamic_cast<Brownian_box const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform(), b->get_parameters());
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Moving_box_barrier * b = dynamic_cast<Moving_box_barrier*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform(), b->get_parameters(), b);
                    b->add_observer(draggable);
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Box_barrier const* b = dynamic_cast<Box_barrier const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform(), b->get_parameters());
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Blow_barrier const* b = dynamic_cast<Blow_barrier const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Box_portal const* b = dynamic_cast<Box_portal const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Molecule_releaser const* m = dynamic_cast<Molecule_releaser const*>(e)) // TODO: if Atom_cannon is added, must be before this entry
                {
                    Draggable_box * draggable = new Draggable_box(m->get_position(), m->get_extent(), m->get_transform(), m->get_parameters());
                    _draggable_to_level_element[draggable] = e;
                }
            }
        }
    }

    void clear()
    {
        _core.clear();
        _active_draggables.clear();

        for (auto & d : _draggable_to_level_element)
        {
            delete d.first;
        }

        _draggable_to_level_element.clear();

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
        External_force gravity;

        gravity._force  = Eigen::Vector3f(0.0f, 0.0f, -1.0f);
        gravity._origin = Eigen::Vector3f(0.0f, 0.0f, 1e6f);

        _core.add_external_force("gravity", gravity);

//        Eigen::AlignedBox<float, 3> play_box(Eigen::Vector3f(-40.0f, -20.0f, 0.0f), Eigen::Vector3f(60.0f, 20.0f, 40.0f));

//        _core.set_game_field_borders(play_box.min(), play_box.max());

        _parameters["game_field_left"]->set_value(-40.0f);
        _parameters["game_field_right"]->set_value(40.0f);
        _parameters["game_field_front"]->set_value(-20.0f);
        _parameters["game_field_back"]->set_value(20.0f);
        _parameters["game_field_bottom"]->set_value(0.0f);
        _parameters["game_field_top"]->set_value(40.0f);

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


        Brownian_box * e = new Brownian_box(Eigen::Vector3f(-10.0f, -20.0f, -10.0f), Eigen::Vector3f(10.0f, 20.0f, 10.0f), 10.0f, 25.0f);
        e->set_user_editable(Level_element::Edit_type::All);
        e->set_persistent(false);
        _core.add_brownian_element(e);

        _parameters["Molecule Renderer/type"]->set_value<std::string>("Shader Renderer");
//        change_renderer();

        update_draggable_to_level_element();
        update_active_draggables();

        update();
    }

    void resizeEvent(QResizeEvent *ev)
    {
        _renderer->resize(ev->size());

        generate_ui_textures();

        Base::resizeEvent(ev);
    }

    void generate_ui_textures()
    {
        for (auto & iter : _buttons)
        {
            for (boost::shared_ptr<Draggable_button> const& button : iter.second)
            {
                generate_button_texture(button.get());
            }
        }

        for (auto & iter : _labels)
        {
            for (boost::shared_ptr<Draggable_label> const& label : iter.second)
            {
                generate_label_texture(label.get());
            }
        }

        for (auto & iter : _statistics)
        {
            for (auto & stat : iter.second)
            {
                generate_statistics_texture(stat);
            }
        }

        update_level_element_buttons();
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

    void setup_intro();

    void update_intro(float const timestep);

    void start_new_game()
    {
        // set level to 0 (including introduction part) and start game

        _progress.last_level = 0;

        change_level_state(Level_state::Intro);

        _core.clear();

        setup_intro();
    }

    void continue_game()
    {
        // load current progress and start game
        load_next_level();
    }

    void change_level_state(Level_state const new_level_state)
    {
        _level_state = new_level_state;

        update_draggable_to_level_element();
        update_active_draggables();
    }

    void quit_game()
    {
        save_progress();

        QApplication::quit();
    }

    void change_state_to_main_menu()
    {
        change_level_state(Level_state::Main_menu);
    }

    void save_progress()
    {
        std::ofstream out_file("progress.data");
        boost::archive::xml_oarchive oa(out_file);

        oa << BOOST_SERIALIZATION_NVP(_progress);

        out_file.close();
    }

    void load_progress()
    {
        std::ifstream in_file("progress.data");

        if (in_file)
        {
            boost::archive::xml_iarchive ia(in_file);

            try
            {
                ia >> BOOST_SERIALIZATION_NVP(_progress);
            }
            catch (std::exception e)
            {
                std::cout << "Failed to load progress file, progress reset: " << e.what() << std::endl;
                _progress = Progress();
            }

            in_file.close();
        }
        else
        {
            std::cout << "No progress file found" << std::endl;
        }
    }


    void setup_ui_elements()
    {
        // main menu
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.85f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Start New Game",  std::bind(&My_viewer::start_new_game, this));
            _buttons[int(Level_state::Main_menu)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.65f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Continue Game",  std::bind(&My_viewer::continue_game, this));
            _buttons[int(Level_state::Main_menu)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.45f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Quit", std::bind(&My_viewer::quit_game, this));
            _buttons[int(Level_state::Main_menu)].push_back(boost::shared_ptr<Draggable_button>(button));
        }


        // start level screen
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.35f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Start Level",  std::bind(&My_viewer::start_level, this));
            _buttons[int(Level_state::Before_start)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.15f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Back to Main Menu",  std::bind(&My_viewer::change_state_to_main_menu, this));
            _buttons[int(Level_state::Before_start)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        // After finish screen
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Next Level",  std::bind(&My_viewer::load_next_level, this));
            _next_level_button = boost::shared_ptr<Draggable_button>(button);
            _buttons[int(Level_state::After_finish)].push_back(_next_level_button);
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Show Statistics", std::bind(&My_viewer::change_state_to_statistics, this));
            _buttons[int(Level_state::After_finish)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.75f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back to Main Menu", std::bind(&My_viewer::change_state_to_main_menu, this));
            _buttons[int(Level_state::After_finish)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.8f, 0.3f), "Level finished!");
            _labels[int(Level_state::After_finish)].push_back(boost::shared_ptr<Draggable_label>(label));
        }

        // Statistics
        _statistics[int(Level_state::Statistics)].resize(_core.get_sensor_data().get_num_data_types());

        {
            Draggable_statistics stat(Eigen::Vector3f(0.25f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Released Molecules");
            _statistics[int(Level_state::Statistics)][int(Sensor_data::Type::RelMol)] = stat;
        }

        {
            Draggable_statistics stat(Eigen::Vector3f(0.75f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Collected Molecules");
            _statistics[int(Level_state::Statistics)][int(Sensor_data::Type::ColMol)] = stat;
        }

        {
            Draggable_statistics stat(Eigen::Vector3f(0.25f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Avg. Temperature");
            _statistics[int(Level_state::Statistics)][int(Sensor_data::Type::AvgTemp)] = stat;
        }

        {
            Draggable_statistics stat(Eigen::Vector3f(0.75f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Energy Consumption");
            _statistics[int(Level_state::Statistics)][int(Sensor_data::Type::EnergyCon)] = stat;
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back",  std::bind(&My_viewer::return_from_stats, this));
            _buttons[int(Level_state::Statistics)].push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Repeat",  std::bind(&My_viewer::change_state_to_statistics, this));
            _buttons[int(Level_state::Statistics)].push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }

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

        std::cout << __PRETTY_FUNCTION__ << " time elapsed: " << elapsed_milliseconds << std::endl;
    }

    void handle_game_state_change()
    {
        if ((_core.get_previous_game_state() == Core::Game_state::Unstarted ||
             _core.get_previous_game_state() == Core::Game_state::Finished) &&
                _core.get_game_state() == Core::Game_state::Running)
        {
            std::cout << __PRETTY_FUNCTION__ << " starting the game" << std::endl;

            change_level_state(Level_state::Running);
        }
        else if (_core.get_previous_game_state() == Core::Game_state::Running && _core.get_game_state() == Core::Game_state::Finished)
        {
            // show score etc. on a screen that allows "replay", "next level" etc.
            std::cout << __PRETTY_FUNCTION__ << " finishing the game" << std::endl;

            change_level_state(Level_state::After_finish);

            QTimer::singleShot(3000, this, SLOT(show_afterstate_ui_elements()));

            int const score_count = _core.get_sensor_data().calculate_score(_core.get_level_data()._score_time_factor);

            _particle_systems[int(Level_state::After_finish)].clear();
            _particle_systems[int(Level_state::After_finish)].push_back(Targeted_particle_system(3.0f));
            _particle_systems[int(Level_state::After_finish)].back().generate(QString("%1").arg(score_count, 8, 10, QChar('0')).toStdString(), _particle_font, QRectF(0.0f, 0.5f, 1.0f, 0.3f));

            setup_statistics(_core.get_sensor_data());

            Score score;
            score.final_score = score_count;
            score.sensor_data = _core.get_sensor_data();

            _progress.last_level += 1;
            _progress.scores[_current_level_name].push_back(score);

            _next_level_button->set_visible(_progress.last_level < _level_names.size());

            save_progress();
        }
    }

    void show_afterstate_ui_elements()
    {
        for (boost::shared_ptr<Draggable_button> const& button : _buttons[int(Level_state::After_finish)])
        {
            button->set_visible(true);
        }

        for (boost::shared_ptr<Draggable_label> const& label : _labels[int(Level_state::After_finish)])
        {
            label->set_visible(true);
        }

        update_draggable_to_level_element();
        update_active_draggables();
    }

    void change_state_to_statistics()
    {
        change_level_state(Level_state::Statistics);

        for (auto & iter : _statistics)
        {
            for (auto & stat : iter.second)
            {
                stat.reset_animation();
            }
        }
    }

    void return_from_stats()
    {
        change_level_state(Level_state::After_finish);
    }

    void setup_statistics(Sensor_data const& sensor_data)
    {
//        Sensor_data fake_sensor_data;

//        std::vector<float> dummy_data;

//        dummy_data.push_back(0);

//        for (int i = 1; i < 50; ++i)
//        {
//            dummy_data.push_back(dummy_data[i - 1] + int(rand() / float(RAND_MAX) * 2));
//            fake_sensor_data.add_value(Sensor_data::Type::ColMol, dummy_data[i]);
//        }

        for (int i = 0; i < sensor_data.get_num_data_types(); ++i)
        {
            _statistics[int(Level_state::Statistics)][i].set_values(sensor_data.get_data(Sensor_data::Type(i)));
//            _statistics[int(Level_state::Statistics)][i].set_values(fake_sensor_data.get_data(Sensor_data::Type(i)));
        }
    }

    void intro_cam1_end_reached();
    void intro_cam2_end_reached();

private:
    QTimer * _physics_timer;
    std::chrono::time_point<std::chrono::system_clock> _physics_elapsed_time;

    Core _core;

    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;

    Picking _picking;
    Mouse_state _mouse_state;
    Selection _selection;
    Level_element * _selected_level_element;
    Ui_state _ui_state;
    Level_state _level_state;

    QPoint _dragging_start;
    Eigen::Vector3f _dragging_start_3d;
    int _picked_index;

    GLuint _rotate_tex;
    GLuint _scale_tex;
    GLuint _move_tex;
    GLuint _slider_tex;

    GLuint _particle_tex;

    std::unordered_map<std::string, QImage> _element_images;

    StandardCamera * _my_camera;

    std::unique_ptr<Molecule_renderer> _renderer;

    std::vector<Draggable*> _active_draggables;

    std::unordered_map<Draggable*, Level_element*> _draggable_to_level_element;

    std::unordered_map<int, std::vector< boost::shared_ptr<Draggable_button> > > _buttons;
    std::unordered_map<int, std::vector< boost::shared_ptr<Draggable_label> > > _labels;
    std::unordered_map<int, std::vector<Draggable_statistics> > _statistics;
    std::unordered_map<int, std::vector<Targeted_particle_system> > _particle_systems;

    boost::shared_ptr<Draggable_button> _next_level_button;

    std::string _current_level_name;

    QStringList _level_names;

    Progress _progress;

    QFont _main_font;
    QFont _particle_font;

    float _intro_time;
    Intro_state _intro_state;
};


#endif
