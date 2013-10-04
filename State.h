#ifndef STATE_H
#define STATE_H

#include <QtGui>

class My_viewer;

class Screen
{
public:
    enum class Type { Modal = 0b01, Fullscreen = 0b10 };
    enum class State { Paused = 0, Resuming, Pausing, Running, Fading_out, Fading_in, Faded_out, Killing, Killed };

    Screen(My_viewer & viewer) : _viewer(viewer), _state(State::Resuming), _transition_progress(0.0f)
    { }

    Type get_type() const
    {
        return _type;
    }

    State get_state() const
    {
        return _state;
    }


    virtual bool mousePressEvent(QMouseEvent *) { return false; }
    virtual bool mouseMoveEvent(QMouseEvent *) { return false; }
    virtual bool mouseReleaseEvent(QMouseEvent * ) { return false; }
    virtual bool keyPressEvent(QKeyEvent *) { return false; }

//    virtual void draw(qglviewer::Camera const* camera, Parameter_list const& parameters);
    virtual void draw() { }

    virtual void pause()
    {
        _state = State::Pausing;
    }

    virtual void resume()
    {
        if (_state == State::Paused || _state == State::Resuming)
        {
            _state = State::Resuming;
        }
    }


    virtual void deactivate() { }
    virtual void activate() { }

    void update(float const time_step)
    {
        _transition_progress += 1.0f * time_step;

        if (_state == State::Resuming)
        {
            if (_transition_progress >= 1.0f)
            {
                _transition_progress = 0.0f;
                _state = State::Running;

                state_changed_event(State::Running, State::Resuming);
            }
        }
        else if (_state == State::Pausing)
        {
            if (_transition_progress >= 1.0f)
            {
                _transition_progress = 0.0f;
                _state = State::Paused;

                state_changed_event(State::Paused, State::Pausing);
            }
        }
        else if (_state == State::Fading_in)
        {
            if (_transition_progress >= 1.0f)
            {
                _transition_progress = 0.0f;
                _state = State::Running;

                state_changed_event(State::Running, State::Fading_in);
            }
        }
        else if (_state == State::Fading_out)
        {
            if (_transition_progress >= 1.0f)
            {
                _transition_progress = 0.0f;
                _state = State::Faded_out;

                state_changed_event(State::Faded_out, State::Fading_out);
            }
        }
        else if (_state == State::Killing)
        {
            if (_transition_progress >= 1.0f)
            {
                _transition_progress = 0.0f;
                _state = State::Killed;

                state_changed_event(State::Killed, State::Killing);
            }
        }
    }

    virtual void update_event(float const /* time_step */) { }


    virtual void state_changed_event(State const /* current_state */, State const /* previous_state */)
    { }

    static bool is_dead(Screen const* s)
    {
        return (s->_state == State::Killed);
    }

protected:
    My_viewer & _viewer;

    Type _type;
    State _state;

    float _transition_progress;
};

/*
class Main_game_state : public State
{
public:
    enum class Mouse_state { None, Init_drag_handle, Init_drag_molecule, Dragging_molecule, Dragging_handle };
    enum class Selection { None, Level_element, Molecule };

    Main_game_state(My_viewer & viewer) : State(viewer)
    { }


    bool mousePressEvent(QMouseEvent *event) override
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

    static std::string name()
    {
        return "Main_game_state";
    }


private:


    std::vector<Draggable*> _active_draggables;

    std::unordered_map<Draggable*, Level_element*> _draggable_to_level_element;
};
*/


#endif // STATE_H
