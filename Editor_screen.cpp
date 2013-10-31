#include "Editor_screen.h"

#include "Pause_screen.h"
#include "My_viewer.h"

bool Editor_screen::mousePressEvent(QMouseEvent * event)
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
        _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()), std::bind(&Main_game_screen::draw_molecules_for_picking, this));
//            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&Molecule_renderer::picking_draw, _molecule_renderer));

        std::cout << __PRETTY_FUNCTION__ << " index: " << _picked_index << std::endl;

        if (_picked_index != -1)
        {
            bool found;
            qglviewer::Vec world_pos = _viewer.camera()->pointUnderPixel(event->pos(), found);

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
        _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                         std::bind(&Main_game_screen::draw_draggables_for_picking, this));

        std::cout << __PRETTY_FUNCTION__ << " picked_index: " << _picked_index << std::endl;

        if (_picked_index != -1)
        {
            _dragging_start = event->pos();

            bool found;
            qglviewer::Vec world_pos = _viewer.camera()->pointUnderPixel(event->pos(), found);

            _mouse_state = Mouse_state::Init_drag_handle;
            std::cout << __PRETTY_FUNCTION__ << " Init_drag_handle" << std::endl;

            if (found)
            {
                _dragging_start_3d = QGLV2Eigen(world_pos);
            }

            handled = true;
        }
    }

    return handled;
}

bool Editor_screen::mouseMoveEvent(QMouseEvent * event)
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

        f._plane_normal = -1.0f * QGLV2Eigen(_viewer.camera()->viewDirection());

        Eigen::Hyperplane<float, 3> view_plane(f._plane_normal, f._origin);

        qglviewer::Vec qglv_origin; // camera pos
        qglviewer::Vec qglv_dir;    // origin - camera_pos

        _viewer.camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

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
    else if (_mouse_state == Mouse_state::Init_drag_handle && _active_draggables[_picked_index]->is_draggable() && event->buttons() & Qt::LeftButton)
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

        _viewer.camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

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

//            update();
        }
    }

    return handled;
}

bool Editor_screen::mouseReleaseEvent(QMouseEvent * event)
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

            handled = true;
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

                if (event->button() == Qt::RightButton)
                {
                    show_context_menu_for_element();
                }
            }

            handled = true;
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
    }

    _mouse_state = Mouse_state::None;

    return handled;
}

bool Editor_screen::keyPressEvent(QKeyEvent * event)
{
    bool handled = false;

    std::cout << __PRETTY_FUNCTION__ << " " << event->key() << std::endl;

    if (event->key() == Qt::Key_Escape)
    {
        if (get_state() == State::Running)
        {
            // go into pause and start pause menu
            pause();

            _viewer.add_screen(new Pause_screen(_viewer, _core, this)); // TODO: make this the editor pause menu

            _core.set_simulation_state(false);

            handled = true;
        }
        else if (get_state() == State::Paused || get_state() == State::Pausing)
        {
            resume();

            handled = true;
        }
    }
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
    {
        delete_selected_element();
    }

    return handled;
}
