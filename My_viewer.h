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
//#include "State.h"



class My_viewer : public Options_viewer
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    enum class Mouse_state { None, Init_drag_handle, Init_drag_molecule, Dragging_molecule, Dragging_handle };
    enum class Selection { None, Level_element, Molecule };
    enum class Ui_state { Level_editor, Playing };
    enum class Level_state { Before_start, Running, After_finish };

    My_viewer() :
        _mouse_state(Mouse_state::None), _selection(Selection::None), _selected_level_element(nullptr)
    {
        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

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

        _parameters.add_parameter(new Parameter("draw_closest_force", true, update));

        _parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));

        _parameters.add_parameter(new Parameter("draw_tree_depth", 1, -1, 10, update));

        _parameters.add_parameter(new Parameter("draw_handles", true, update));

        std::vector<std::string> particle_types { "O2", "H2O", "SDS", "Na", "Cl", "Dipole",
                                                  "Plane_barrier", "Box_barrier", "Brownian_box", "Box_portal", "Blow_barrier", "Moving_box_barrier", "Molecule_releaser" };

        _parameters.add_parameter(new Parameter("particle_type", 0, particle_types, update));

        std::vector<std::string> ui_states { "Level_editor", "Playing" };

        _parameters.add_parameter(new Parameter("ui_state", 0, ui_states, std::bind(&My_viewer::change_ui_state, this)));

        _parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&My_viewer::toggle_simulation, this)));
        _parameters.add_parameter(Parameter::create_button("Save state", std::bind(&My_viewer::save_state, this)));
        _parameters.add_parameter(Parameter::create_button("Load state", std::bind(&My_viewer::load_state, this)));
        _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));
        _parameters.add_parameter(Parameter::create_button("Start Level", std::bind(&My_viewer::start_level, this)));
        _parameters.add_parameter(Parameter::create_button("Reset Level", std::bind(&My_viewer::reset_level, this)));
        _parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));

        Parameter_registry<Molecule_renderer>::create_single_select_instance(&_parameters, "Molecule Renderer", std::bind(&My_viewer::change_renderer, this));

        change_renderer();
        change_core_settings();
        change_ui_state();

        connect(&_core, SIGNAL(game_state_changed()), this, SLOT(handle_game_state_change()));
    }

    void save_state()
    {
        QString filename;

        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        {
            filename = "state.save";
        }
        else
        {
            filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    ".",
                                                    tr("State File (*.save)"));
        }

        if (!filename.isEmpty())
        {
            _core.save_state(filename.toStdString());
        }
    }

    void load_state()
    {
        QString filename;

        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        {
            filename = "state.save";
        }
        else
        {
            filename = QFileDialog::getOpenFileName(this, tr("Save File"),
                                                    ".",
                                                    tr("State File (*.save)"));
        }

        if (!filename.isEmpty())
        {
            clear();
            _core.load_state(filename.toStdString());
            update_draggable_to_level_element();
            update_active_draggables();
            update();
        }
    }

    void restore_parameters() override
    {
        Base::restore_parameters();

        change_renderer();
        change_core_settings();
    }

    void start_level()
    {
        if (!animationIsStarted())
        {
            startAnimation();
        }

        _core.start_level();
        _parameters["Toggle simulation"]->set_value(true);
        update();
    }

    void reset_level()
    {
        _core.reset_level();
        update_draggable_to_level_element();
        update_active_draggables();
        update();
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
        _molecule_renderer = std::unique_ptr<Molecule_renderer>(Parameter_registry<Molecule_renderer>::get_class_from_single_select_instance_2(_parameters.get_child("Molecule Renderer")));
        update();
    }

    void change_core_settings()
    {
        _core.set_parameters(*_parameters.get_child("Core"));
        update();
    }

    class Game_camera_constraint : public qglviewer::WorldConstraint
    {
    public:
//        virtual void constrainTranslation(qglviewer::Vec & t, Frame * const frame)
//        {
//            // Express t in the world coordinate system.
//            const qglviewer::Vec tWorld = frame->inverseTransformOf(t);
//            if (frame->position().z + tWorld.z < 0.0) // check the new fr z coordinate
//                t.z = frame->transformOf(-frame->position().z); // t.z is clamped so that next z position is 0.0
//        }

        void constrainRotation(qglviewer::Quaternion & rotation, qglviewer::Frame * const frame) override
        {
            qglviewer::Vec rotation_axis = frame->inverseTransformOf(rotation.axis());

            std::cout << __PRETTY_FUNCTION__ << " " << rotation_axis.x << " " << rotation_axis.y << " " << rotation_axis.z << std::endl;

            into_range(rotation_axis.x, -0.2, 0.2);
            into_range(rotation_axis.y, -0.2, 0.2);
            rotation.setAxisAngle(frame->transformOf(rotation_axis), rotation.angle());
        }
    };

    void change_ui_state()
    {
        if (_parameters["ui_state"]->get_value<std::string>() == "Level_editor")
        {
            _ui_state = Ui_state::Level_editor;

            camera()->frame()->setConstraint(nullptr);
        }
        else if (_parameters["ui_state"]->get_value<std::string>() == "Playing")
        {
            _ui_state = Ui_state::Playing;


            Game_camera_constraint * camera_constraint = new Game_camera_constraint;
            camera_constraint->setTranslationConstraint(qglviewer::AxisPlaneConstraint::PLANE, qglviewer::Vec(0.0f, 1.0f, 0.0f));

//            camera_constraint->setRotationConstraint();

            delete camera()->frame()->constraint();
            camera()->frame()->setConstraint(camera_constraint);
        }

        update_draggable_to_level_element();
        update_active_draggables();
        update();
    }

    void change_clipping()
    {
        _my_camera->set_near_far(_parameters["z_near"]->get_value<float>(), _parameters["z_far"]->get_value<float>());
        update();
    }

    void init() override
    {
        Base::init();

        restoreStateFromFile();

        glPointSize(5.0f);

        _physics_timer = new QTimer;
        _physics_timer->setInterval(_parameters["physics_timestep_ms"]->get_value<int>());

        connect(_physics_timer, SIGNAL(timeout()), this, SLOT(update_physics()));
//        _physics_timer->start();

        qglviewer::ManipulatedCameraFrame * frame = camera()->frame();
        _my_camera = new StandardCamera(0.1f, 1000.0f);
        _my_camera->setFrame(frame);
        setCamera(_my_camera);

        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);

        _picking.init(context(), size()); // FIXME: needs to be resized when viewer changes

        glEnable(GL_NORMALIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        setSceneRadius(50.0f);

        const GLubyte* pVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "shader version: " << pVersion << std::endl;

//        GPU_force * gpu_force = new GPU_force(context());
//        gpu_force->calc_forces(_core.get_molecules());

        startAnimation();
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

                glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

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
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        _molecule_renderer->render(_core.get_molecules(), _my_camera);

        // debug displays

        draw_tree();

//        draw_tree_for_point(Eigen::Vector3f(0.0f, 0.0f, 0.0f));

        draw_barriers();

        draw_brownian_elements();

        draw_portals();

        draw_molecule_releasers();

        if (_parameters["draw_handles"]->get_value<bool>())
        {
            draw_draggables();
        }

        glDisable(GL_LIGHTING);

        draw_indicators();

        draw_user_force();

        draw_closest_force();

        draw_temperature();
    }

    void draw_draggables() // FIXME: use visitors or change it so that draggables can only have a single type of handles (Draggable_point)
    {
        float const scale = 1.5f;

        for (auto const& d : _draggable_to_level_element)
        {
            if (Draggable_box const* d_box = dynamic_cast<Draggable_box const*>(d.first))
            {
                std::vector<Draggable_point> const& corners = d_box->get_corners();

//                draw_box(d_box->get_min(), d_box->get_max());

                glPushMatrix();

                glTranslatef(d_box->get_position()[0], d_box->get_position()[1], d_box->get_position()[2]);
                glMultMatrixf(d_box->get_transform().data());


                for (Draggable_point const& p : corners)
                {
                    glColor3f(0.7f, 0.7f, 0.7f);
                    draw_box_from_center(p.get_position(), Eigen::Vector3f(scale, scale, scale));
                }
                for (Draggable_disc const& d_disc : d_box->get_position_points())
                {
                    draw_sphere_ico(Eigen2OM(d_disc.get_position()), 2.0f, Color(0.7f, 0.7f, 1.0f));
                }
                for (Draggable_disc const& d_disc : d_box->get_rotation_handles())
                {
                    draw_sphere_ico(Eigen2OM(d_disc.get_position()), 2.0f, Color(1.0f, 0.7f, 0.7f));
                }
                for (auto iter : d_box->get_property_handles())
                {
                    glColor3f(0.8f, 0.8f, 0.8f);
                    draw_box_from_center(iter.second.get_position(), Eigen::Vector3f(scale, scale, scale));
                }

                glPopMatrix();
            }
        }
    }

    void draw_draggables_for_picking()
    {
        float const scale = 1.5f;

        for (int i = 0; i < int(_active_draggables.size()); ++i)
        {
            Draggable const* draggable = _active_draggables[i];

            glPushMatrix();

            glTranslatef(draggable->get_parent()->get_position()[0], draggable->get_parent()->get_position()[1], draggable->get_parent()->get_position()[2]);
            glMultMatrixf(draggable->get_parent()->get_transform().data());

            _picking.set_index(i);

            if (Draggable_point const* d_point = dynamic_cast<Draggable_point const*>(draggable))
            {
                draw_box_from_center(d_point->get_position(), Eigen::Vector3f(scale, scale, scale));
            }
            else if (Draggable_disc const* d_point = dynamic_cast<Draggable_disc const*>(draggable))
            {
                draw_sphere_ico(Eigen2OM(d_point->get_position()), 2.0f);
            }

            glPopMatrix();
        }
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
                draw_point(f._atom._r);

                //glLineWidth(into_range(1.0f, 5.0f, f._force.norm()));
                //            draw_arrow_z_plane(Eigen2OM(f._atom._r), Eigen2OM(f._atom._r + f._force.normalized()));

                draw_arrow_z_plane(Eigen2OM(f._atom._r), Eigen2OM(f._atom._r + f._force * indicator_scale));
            }
        }
    }

    void draw_closest_force() const
    {
        if (_parameters["draw_closest_force"]->get_value<bool>())
        {
            Reject_condition reject_cond;

            for (Molecule const& molecule : _core.get_molecules())
            {
                reject_cond.molecule_id = molecule.get_id();

                for (Atom const& a : molecule._atoms)
                {
                    if (std::abs(a._charge) < 0.001f) continue;

                    boost::optional<Core::Molecule_atom_hash::Point_data const&> opt_pd = _core.get_molecule_hash().get_closest_point(a._r, reject_cond);

                    if (!opt_pd) continue;

                    Core::Molecule_atom_hash::Point_data const& pd = opt_pd.get();

                    boost::optional<Molecule const&> opt_molecule = _core.get_molecule(pd.data.m_id);

                    assert(opt_molecule);

                    Atom const& closest_atom = opt_molecule->_atoms[pd.data.a_id];

                    if (std::abs(closest_atom._charge) < 0.001f) continue;

//                    Atom const& closest_atom = *_core.get_tree().find_closest<Reject_condition>(a._r, reject_cond);

                    Eigen::Vector3f const force = _core.calc_forces_between_atoms(closest_atom, a);

                    if (force.norm() > 1e-6f)
                    {
                        if (force.dot(closest_atom._r - a._r) < 0.0f)
                        {
                            glColor3f(0.3f, 0.8f, 0.2f);
                        }
                        else
                        {
                            glColor3f(0.8f, 0.2f, 0.2f);
                        }

                        glLineWidth(into_range(100.0f * force.norm(), 0.5f, 10.0f));
                        draw_line(a._r, closest_atom._r);
                    }
                }
            }
        }
    }

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
        dummy._r = point;
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

            if (Molecule::molecule_exists(element_type))
            {
                _core.add_molecule(Molecule::create(element_type, intersect_pos));
            }
            else if (element_type == std::string("Box_barrier"))
            {
                float const strength = 10000.0f;
                float const radius   = 2.0f;

                _core.add_barrier(new Box_barrier(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, strength, radius));
            }
            else if (element_type == std::string("Moving_box_barrier"))
            {
                float const strength = 10000.0f;
                float const radius   = 2.0f;

                _core.add_barrier(new Moving_box_barrier(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, strength, radius));
            }
            else if (element_type == std::string("Blow_barrier"))
            {
                float const strength = 10000.0f;
                float const radius   = 2.0f;

                Blow_barrier * e = new Blow_barrier(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos,
                                                                   Blow_barrier::Axis::X, 30.0f,
                                                                   strength, radius);
                e->set_user_editable(true);
                _core.add_barrier(e);
            }
            else if (element_type == std::string("Plane_barrier"))
            {
                float const strength = 100.0f;
                float const radius   = 5.0f;

                _core.add_barrier(new Plane_barrier(intersect_pos, Eigen::Vector3f::UnitZ(), strength, radius, Eigen::Vector2f(10.0f, 20.0)));
            }
            else if (element_type == std::string("Brownian_box"))
            {
                float const strength = 10.0f;
                float const radius   = 25.0f;

                Brownian_box * e = new Brownian_box(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, strength, radius);
                e->set_user_editable(true);
                e->set_persistent(false);
                _core.add_brownian_element(e);
            }
            else if (element_type == std::string("Box_portal"))
            {
                _core.add_portal(new Box_portal(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos));
            }
            else if (element_type == std::string("Molecule_releaser"))
            {
                Molecule_releaser * m = new Molecule_releaser(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, 1.0f, 1.0f);
                m->set_exemplar(Molecule::create_water(Eigen::Vector3f::Zero()));
                _core.add_molecule_releaser(m);
            }

            update_draggable_to_level_element();
            update_active_draggables();
        }
    }

    void mousePressEvent(QMouseEvent *event)
    {
        bool handled = false;

        if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier)
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

                if (found)
                {
                    _dragging_start_3d = QGLV2Eigen(world_pos);
                    _mouse_state = Mouse_state::Init_drag_handle;
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
        else if (_mouse_state == Mouse_state::Init_drag_handle)
        {
            if (_picked_index != -1 && (_dragging_start - event->pos()).manhattanLength() > 0)
            {
                _mouse_state = Mouse_state::Dragging_handle;
            }
        }
        else if (_mouse_state == Mouse_state::Dragging_handle) // TODO: currently has Y plane constraint, move constraints into Draggable, consider giving it the viewline instead of a single position
        {
            Eigen::Hyperplane<float, 3> y_plane(Eigen::Vector3f(0.0f, 1.0f, 0.0f), _active_draggables[_picked_index]->get_position());
            //                Eigen::Hyperplane<float, 3> y_plane(Eigen::Vector3f(0.0f, 1.0f, 0.0f),
            //                                                    Eigen::Vector3f(
            //                                                        _dragging_start_3d[0],
            //                                                        _active_draggables[_picked_index]->get_position()[1],
            //                                                        _dragging_start_3d[2]));

            //                Eigen::Hyperplane<float, 3> view_plane(QGLV2Eigen(camera()->viewDirection()), _dragging_start_3d);
            //                Eigen::Hyperplane<float, 3> view_plane(QGLV2Eigen(camera()->viewDirection()), _active_draggables[_picked_index]->get_position());


            qglviewer::Vec qglv_origin; // camera pos
            qglviewer::Vec qglv_dir;    // origin - camera_pos

            camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

            Eigen::ParametrizedLine<float, 3> view_ray(QGLV2Eigen(qglv_origin), QGLV2Eigen(qglv_dir).normalized());

            Eigen::Vector3f new_position = view_ray.intersectionPoint(y_plane);
            //                Eigen::Vector3f new_position = view_ray.intersectionPoint(view_plane);

            //                new_position[1] =_active_draggables[_picked_index]->get_position()[1];

//          _active_draggables[_picked_index].to_local();

            _active_draggables[_picked_index]->set_position_from_world(new_position);
            _active_draggables[_picked_index]->update();

            Draggable * parent = _active_draggables[_picked_index]->get_parent();

            assert(_draggable_to_level_element.find(parent) != _draggable_to_level_element.end());

            std::cout << __PRETTY_FUNCTION__ << ": " << parent << std::endl;

            _draggable_to_level_element[parent]->accept(parent);

            update();
        }

        if (!handled)
        {
            Base::mouseMoveEvent(event);
        }
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
                _selected_level_element = _draggable_to_level_element[parent];
                _selected_level_element->set_selected(true);

                if (event->button() == Qt::RightButton)
                {
                    show_context_menu_for_element();
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
        if (event->key() == Qt::Key_Delete && _ui_state == Ui_state::Level_editor)
        {
            delete_selected_element();
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

        if (Moving_box_barrier * b = dynamic_cast<Moving_box_barrier *>(element))
        {
            QMenu menu;

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
            QMenu menu;

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
                spinbox_first_release->setValue(m->get_interval());

                action_first_release->setDefaultWidget(new Widget_text_combination("First release", spinbox_first_release));
            }

            menu.addAction(action_num_max_molecules);
            menu.addAction(action_interval);
            menu.addAction(action_first_release);

            menu.exec(QCursor::pos());

            m->set_num_max_molecules(spinbox_num_max_molecules->value());
            m->set_interval(spinbox_interval->value());
            m->set_first_release(spinbox_first_release->value());

            delete action_num_max_molecules;
            delete action_interval;
            delete action_first_release;
        }
        else if (Portal * m = dynamic_cast<Portal*>(element))
        {
            QMenu menu;

            QWidgetAction * action_num_min_captured_molecules = new QWidgetAction(this);

            QSpinBox * spinbox_num_min_captured_molecules = new QSpinBox();
            {
                spinbox_num_min_captured_molecules->setMaximum(1000);
                spinbox_num_min_captured_molecules->setMinimum(1);
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

            menu.addAction(action_num_min_captured_molecules);
            menu.addAction(action_type);

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

            delete action_num_min_captured_molecules;
            delete action_type;
        }
    }

    void do_physics_timestep()
    {
        update_physics();

//        std::cout << __PRETTY_FUNCTION__ << std::endl;

//        for (Molecule const& m : _core.get_molecules())
//        {
//            std::cout << "x: " << m._x << " F: " << m._force << " v: " << m._v << std::endl;
//        }

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

        Base::animate();
    }

    void update_active_draggables()
    {
        _active_draggables.clear();

        for (auto const& d : _draggable_to_level_element)
        {
            std::vector<Draggable*> const draggables = d.first->get_draggables();
            std::copy (draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
        }
    }

    void update_draggable_to_level_element()
    {
        for (auto & d : _draggable_to_level_element)
        {
            delete d.first;
        }

        _draggable_to_level_element.clear();

        for (Brownian_element * e : _core.get_brownian_elements())
        {
            if (_ui_state == Ui_state::Level_editor || (_ui_state != Ui_state::Level_editor && e->is_user_editable()))
            {
                if (Brownian_box const* b = dynamic_cast<Brownian_box const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    draggable->add_property_handle("radius", Eigen::Vector2f(5.0f, 100.0f), b->get_radius());
                    draggable->add_property_handle("strength", Eigen::Vector2f(-50.0f, 50.0f), b->get_strength());
                    _draggable_to_level_element[draggable] = e;
                }
            }
        }

        for (Barrier * e : _core.get_barriers())
        {
            if (_ui_state == Ui_state::Level_editor || (_ui_state != Ui_state::Level_editor && e->is_user_editable()))
            {
                if (Plane_barrier const* b = dynamic_cast<Plane_barrier const*>(e))
                {
                    Eigen::Vector3f extent = Eigen::Vector3f::Zero();

                    if (b->get_extent())
                    {
                        extent = Eigen::Vector3f(b->get_extent().get()[0], b->get_extent().get()[1], 0.0f);
                    }

                    Draggable_box * draggable = new Draggable_box(b->get_position() - extent * 0.5f,
                                                                  b->get_position() + extent * 0.5f,
                    {
                                                                      { Draggable_box::Corner_type::Min, Draggable_box::Corner_type::Min, Draggable_box::Corner_type::Min },
                                                                      { Draggable_box::Corner_type::Max, Draggable_box::Corner_type::Min, Draggable_box::Corner_type::Min },
                                                                  },
                    { Draggable_box::Plane::Neg_Y }, { Draggable_box::Plane::Neg_Y } );

                    draggable->set_transform(b->get_transform());

                    _draggable_to_level_element[draggable] = e;
                }
                else if (Moving_box_barrier * b = dynamic_cast<Moving_box_barrier*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform(), b);
                    b->add_observer(draggable);
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Box_barrier const* b = dynamic_cast<Box_barrier const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    _draggable_to_level_element[draggable] = e;
                }
                else if (Blow_barrier const* b = dynamic_cast<Blow_barrier const*>(e))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    _draggable_to_level_element[draggable] = e;
                }
            }
        }

        for (Portal * p : _core.get_portals())
        {
            if (_ui_state == Ui_state::Level_editor || (_ui_state != Ui_state::Level_editor && p->is_user_editable()))
            {
                if (Box_portal const* b = dynamic_cast<Box_portal const*>(p))
                {
                    Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                    _draggable_to_level_element[draggable] = p;
                }
            }
        }

        for (Molecule_releaser * m : _core.get_molecule_releasers())
        {
            if (_ui_state == Ui_state::Level_editor || (_ui_state != Ui_state::Level_editor && m->is_user_editable()))
            {
                Draggable_box * draggable = new Draggable_box(m->get_position(), m->get_extent(), m->get_transform());
                _draggable_to_level_element[draggable] = m;
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

        Eigen::AlignedBox<float, 3> play_box(Eigen::Vector3f(-40.0f, -20.0f, 0.0f), Eigen::Vector3f(60.0f, 20.0f, 40.0f));

        _core.set_game_field_borders(play_box.min(), play_box.max());

//        Eigen::Vector3f play_box_center(play_box.center());
//        Eigen::Vector3f play_box_extent((play_box.max() - play_box.min()));

//        float const strength = 100.0f;
//        float const radius   = 5.0f;

//        for (int axis = 0; axis < 3; ++axis)
//        {
//            for (int sign = -1; sign <= 2; sign += 2)
//            {
//                Eigen::Vector3f normal = Eigen::Vector3f::Zero();
//                normal[axis] = -1.0f * sign;
//                int const first_axis = (axis + 1) % 3;
//                int const second_axis = (axis + 2) % 3;
//                Eigen::Vector2f extent(play_box_extent[first_axis > second_axis ? second_axis : first_axis], play_box_extent[first_axis < second_axis ? second_axis : first_axis]);
//                _core.add_barrier(new Plane_barrier(play_box_center - normal * play_box_extent[axis] * 0.5f, normal, strength, radius, extent));
//            }
//        }

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

                    Eigen::Vector3f pos = Eigen::Vector3f(x, y, z) * resolution + grid_start;

                    if ((sum % 2) == 0)
                    {
                        _core.add_molecule(Molecule::create_charged_chlorine(pos));
                    }
                    else
                    {
                        _core.add_molecule(Molecule::create_charged_natrium(pos));
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

        update_draggable_to_level_element();
        update_active_draggables();

        update();
    }

public Q_SLOTS:
    void update_physics()
    {
//        int const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
//                (std::chrono::system_clock::now() - _physics_elapsed_time).count();

//        _physics_elapsed_time = std::chrono::system_clock::now();

//        std::cout << "update_physics() time elapsed: " << elapsed_milliseconds << " now: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
        // FIXME: currently constant update time step, not regarding at all the actually elapsed time
        // some updates are really far away from the set time step, not sure why
        _core.update(_parameters["physics_timestep_ms"]->get_value<int>() / 1000.0f * _parameters["physics_speed"]->get_value<float>());
//        _core.update(elapsed_milliseconds / 1000.0f * _parameters["physics_speed"]->get_value<float>());
    }

    void handle_game_state_change()
    {
        if (_core.get_previous_game_state() == Core::Game_state::Unstarted && _core.get_game_state() == Core::Game_state::Running)
        {
            // show a "Start!" sort of message
            std::cout << __PRETTY_FUNCTION__ << " starting the game" << std::endl;
        }
        else if (_core.get_previous_game_state() == Core::Game_state::Running && _core.get_game_state() == Core::Game_state::Finished)
        {
            // show score etc. on a screen that allows "replay", "next level" etc.
            std::cout << __PRETTY_FUNCTION__ << " finishing the game" << std::endl;
            set_simulation_state(false);
        }
    }

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
//    bool _is_dragging;
    QPoint _dragging_start;
    Eigen::Vector3f _dragging_start_3d;
    int _picked_index;

    StandardCamera * _my_camera;

    std::unique_ptr<Molecule_renderer> _molecule_renderer;

//    std::vector<Draggable*> _draggables;

    std::vector<Draggable*> _active_draggables;

    std::unordered_map<Draggable*, Level_element*> _draggable_to_level_element;
};


#endif
