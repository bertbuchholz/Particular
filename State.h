#ifndef STATE_H
#define STATE_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <iostream>

#include <Eigen/Geometry>

#include <Options_viewer.h>
#include <Draw_functions.h>

#include <Picking.h>
#include <Registry_parameters.h>
#include <Geometry_utils.h>

#include "Core.h"
#include "Atom.h"
#include "Spatial_hash.h"
#include "Renderer.h"

#include "Draggable.h"
#include "Level_element_draw_visitor.h"

class State
{
public:
    State(Core & core, StandardCamera & camera) : _core(core), _camera(camera)
    { }

    virtual bool mousePressEvent(QMouseEvent *) { return false; }
    virtual bool mouseMoveEvent(QMouseEvent *) { return false; }
    virtual bool mouseReleaseEvent(QMouseEvent * ) { return false; }
    virtual bool keyPressEvent(QKeyEvent *) { return false; }

//    virtual void draw(qglviewer::Camera const* camera, Parameter_list const& parameters);
    virtual void draw(Parameter_list const& parameters);

    virtual void pause() {}
    virtual void resume() {}

protected:
    Core & _core;
    StandardCamera & _camera;
};

class Main_game_state : public State
{
public:
    enum class Mouse_state { None, Init_drag_handle, Init_drag_molecule, Dragging_molecule, Dragging_handle };
    enum class Selection { None, Level_element, Molecule };

    Main_game_state(Core & core, StandardCamera & camera) : State(core, camera)
    { }

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
            if (Brownian_box const* b = dynamic_cast<Brownian_box const*>(e))
            {
                Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                draggable->add_property_handle("radius", Eigen::Vector2f(5.0f, 100.0f));
                draggable->add_property_handle("strength", Eigen::Vector2f(-50.0f, 50.0f));
                _draggable_to_level_element[draggable] = e;
            }
        }

        for (Barrier * e : _core.get_barriers())
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
            else if (Box_barrier const* b = dynamic_cast<Box_barrier const*>(e))
            {
                Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                _draggable_to_level_element[draggable] = e;
            }
        }

        for (Portal * p : _core.get_portals())
        {
            if (Box_portal const* b = dynamic_cast<Box_portal const*>(p))
            {
                Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform());
                _draggable_to_level_element[draggable] = p;
            }
        }
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

    void draw(Parameter_list const& parameters) override
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        _molecule_renderer->render(_core.get_molecules(), _my_camera);

        // debug displays

        draw_tree();

//        draw_tree_for_point(Eigen::Vector3f(0.0f, 0.0f, 0.0f));

        draw_barriers();

        draw_brownian_elements();

        draw_portals();

        if (parameters["draw_handles"]->get_value<bool>())
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

    void draw_portals()
    {
        Level_element_draw_visitor v;

        for (Portal * element : _core.get_portals())
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

    void add_element_event(QPoint const& position)
    {
        qglviewer::Vec qglv_origin;
        qglviewer::Vec qglv_dir;

        _camera.convertClickToLine(position, qglv_origin, qglv_dir);

        Vec origin = QGLV2OM(qglv_origin);
        Vec dir    = QGLV2OM(qglv_dir);

        float const t = ray_plane_intersection(origin, dir, Vec(0.0f, 0.0f, 0.0f), Vec(0.0f, 1.0f, 0.0f));

        if (t > 0)
        {
            Eigen::Vector3f const intersect_pos = OM2Eigen(origin + t * dir);

            std::string const particle_type = _parameters["particle_type"]->get_value<std::string>();

            if (particle_type == std::string("O2"))
            {
                _core.add_molecule(Molecule::create_oxygen(intersect_pos));
            }
            else if (particle_type == std::string("H2O"))
            {
                _core.add_molecule(Molecule::create_water(intersect_pos));
            }
            else if (particle_type == std::string("SDS"))
            {
                _core.add_molecule(Molecule::create_sulfate(intersect_pos));
            }
            else if (particle_type == std::string("Na"))
            {
                _core.add_molecule(Molecule::create_charged_natrium(intersect_pos));
            }
            else if (particle_type == std::string("Cl"))
            {
                _core.add_molecule(Molecule::create_charged_chlorine(intersect_pos));
            }
            else if (particle_type == std::string("Dipole"))
            {
                _core.add_molecule(Molecule::create_dipole(intersect_pos));
            }
            else if (particle_type == std::string("Box_barrier"))
            {
                float const strength = 100.0f;
                float const radius   = 5.0f;

                _core.add_barrier(new Box_barrier(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, strength, radius));
                update_draggable_to_level_element();
                update_active_draggables();
            }
            else if (particle_type == std::string("Plane_barrier"))
            {
                float const strength = 100.0f;
                float const radius   = 5.0f;

                _core.add_barrier(new Plane_barrier(intersect_pos, Eigen::Vector3f::UnitZ(), strength, radius, Eigen::Vector2f(10.0f, 20.0) ));
                update_draggable_to_level_element();
                update_active_draggables();
            }
            else if (particle_type == std::string("Brownian"))
            {
                float const strength = 100.0f;
                float const radius   = 5.0f;

                _core.add_brownian_element(new Brownian_box(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos, strength, radius));
                update_draggable_to_level_element();
                update_active_draggables();
            }
            else if (particle_type == std::string("Box_portal"))
            {
                _core.add_portal(new Box_portal(Eigen::Vector3f(-10.0f, -20.0f, -10.0f) + intersect_pos, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + intersect_pos));
                update_draggable_to_level_element();
                update_active_draggables();
            }
        }
    }


    bool mousePressEvent(QMouseEvent *event)
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

            _picked_index = _picking.do_pick(event->pos().x(), _camera.screenHeight() - event->pos().y(), std::bind(&Main_game_state::draw_molecules_for_picking, this));
//            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&Molecule_renderer::picking_draw, _molecule_renderer));

            std::cout << __PRETTY_FUNCTION__ << " index: " << _picked_index << std::endl;

            if (_picked_index != -1)
            {
                bool found;
                qglviewer::Vec world_pos = _camera.pointUnderPixel(event->pos(), found);

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
            _picked_index = _picking.do_pick(event->pos().x(), _camera.screenHeight() - event->pos().y(), std::bind(&Main_game_state::draw_draggables_for_picking, this));

            std::cout << __PRETTY_FUNCTION__ << " picked_index: " << _picked_index << std::endl;

            if (_picked_index != -1)
            {
                _dragging_start = event->pos();

                bool found;
                qglviewer::Vec world_pos = _camera.pointUnderPixel(event->pos(), found);

                if (found)
                {
                    _dragging_start_3d = QGLV2Eigen(world_pos);
                    _mouse_state = Mouse_state::Init_drag_handle;
                }

                handled = true;
            }
        }

        return handled;
    }

    bool mouseMoveEvent(QMouseEvent * event) override
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

            f._plane_normal = -1.0f * QGLV2Eigen(_camera.viewDirection());

            Eigen::Hyperplane<float, 3> view_plane(f._plane_normal, f._origin);

            qglviewer::Vec qglv_origin; // camera pos
            qglviewer::Vec qglv_dir;    // origin - camera_pos

            _camera.convertClickToLine(event->pos(), qglv_origin, qglv_dir);

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

            _camera.convertClickToLine(event->pos(), qglv_origin, qglv_dir);

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

        return handled;
    }

    bool mouseReleaseEvent(QMouseEvent * event)
    {
        bool handled = false;

        if (_mouse_state == Mouse_state::Init_drag_molecule)
        {
            std::cout << __PRETTY_FUNCTION__ << " click on molecule" << std::endl;

            if (_picked_index != -1)
            {
                _selection = Selection::Molecule;

                Molecule_external_force & f = _core.get_user_force();

                f._end_time = _core.get_current_time() + 0.5f;
            }

            handled = true;
        }
        else if (_mouse_state == Mouse_state::Init_drag_handle)
        {
            std::cout << __PRETTY_FUNCTION__ << " click on handle" << std::endl;

            if (_picked_index != -1)
            {
                _selection = Selection::Level_element;
            }

            handled = true;
        }
        else
        {
            _selection = Selection::None;
        }

        _mouse_state = Mouse_state::None;

        return handled;
    }

    bool keyPressEvent(QKeyEvent *event) override
    {
        bool handled = false;

        if (event->key() == Qt::Key_Delete)
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

                update();
            }

            handled = true;
        }

        return handled;
    }


    void set_parameters(Parameter_list const& parameters)
    {

    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;

        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

        parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&My_viewer::update_physics_timestep, this)));
        parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update));
        parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));

        parameters.add_parameter(new Parameter("z_near", 0.1f, 0.01f, 100.0f, std::bind(&My_viewer::change_clipping, this)));
        parameters.add_parameter(new Parameter("z_far", 100.0f, 1.0f, 1000.0f, std::bind(&My_viewer::change_clipping, this)));

        parameters.add_parameter(new Parameter("draw_closest_force", true, update));

        parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));

        parameters.add_parameter(new Parameter("draw_tree_depth", 1, -1, 10, update));

        parameters.add_parameter(new Parameter("draw_handles", true, update));

        std::vector<std::string> particle_types { "O2", "H2O", "SDS", "Na", "Cl", "Dipole", "Plane_barrier", "Box_barrier", "Brownian", "Box_portal" };

        parameters.add_parameter(new Parameter("particle_type", 0, particle_types, update));

        parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&My_viewer::toggle_simulation, this)));
        parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));
        parameters.add_parameter(Parameter::create_button("Save state", std::bind(&My_viewer::save_state, this)));
        parameters.add_parameter(Parameter::create_button("Load state", std::bind(&My_viewer::load_state, this)));
        parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));

        Parameter_registry<Molecule_renderer>::create_single_select_instance(&_parameters, "Molecule Renderer", std::bind(&My_viewer::change_renderer, this));

        return parameters;
    }

    static Core * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Main_game_state";
    }


private:
    QTimer * _physics_timer;
    std::chrono::time_point<std::chrono::system_clock> _physics_elapsed_time;

    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;

    Picking _picking;
    Mouse_state _mouse_state;
    Selection _selection;
    QPoint _dragging_start;
    Eigen::Vector3f _dragging_start_3d;
    int _picked_index;

    StandardCamera * _my_camera;

    std::unique_ptr<Molecule_renderer> _molecule_renderer;

//    std::vector<Draggable*> _draggables;

    std::vector<Draggable*> _active_draggables;

    std::unordered_map<Draggable*, Level_element*> _draggable_to_level_element;
};


#endif // STATE_H
