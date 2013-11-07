#include "Editor_screen.h"

#include "Editor_pause_screen.h"
#include "My_viewer.h"

Editor_screen::Editor_screen(My_viewer &viewer, Core &core) : Main_game_screen(viewer, core, Ui_state::Level_editor)
{
//    _placeable_molecules = std::vector<std::string>{ "H2O", "Na", "Cl" };
    _placeable_molecules = { "H2O", "Na", "Cl" };

    init_level_element_buttons();

    Main_options_window::get_instance()->add_parameter_list("Editor_screen", _parameters);
}

Editor_screen::~Editor_screen()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    Main_options_window::get_instance()->remove_parameter_list("Editor_screen");
}

bool Editor_screen::mousePressEvent(QMouseEvent * event)
{
    bool handled = false;


//    if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier)
//    {
//        add_element_event(event->pos());
//        handled = true;
//    }
//    else

//    if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::AltModifier)
//    {
//        std::cout << __PRETTY_FUNCTION__ << " Drag/Click" << std::endl;
//        _dragging_start = event->pos();

////            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&My_viewer::draw_molecules_for_picking, this));
//        _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()), std::bind(&Main_game_screen::draw_molecules_for_picking, this));
////            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&Molecule_renderer::picking_draw, _molecule_renderer));

//        std::cout << __PRETTY_FUNCTION__ << " index: " << _picked_index << std::endl;

//        if (_picked_index != -1)
//        {
//            bool found;
//            qglviewer::Vec world_pos = _viewer.camera()->pointUnderPixel(event->pos(), found);

//            if (found)
//            {
//                boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

//                assert(picked_molecule);

//                Molecule_external_force & f = _core.get_user_force();
//                f._molecule_id = _picked_index;
////                    qglviewer::Vec dir = world_pos - camera()->position();
////                    dir.normalize();
//                f._force.setZero();
//                f._origin = QGLV2Eigen(world_pos);
//                f._local_origin = picked_molecule->_R.transpose() * (f._origin - picked_molecule->_x);
//                f._end_time = _core.get_current_time();

//                _mouse_state = Mouse_state::Init_drag_molecule;

//                std::cout << "Apply force: " << f._origin << std::endl;
//            }
//        }

//        handled = true;
//    }
//    else

    if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        handled = true;
    }
    else
    {
        _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                         std::bind(&Main_game_screen::draw_draggables_for_picking, this));

        std::cout << __PRETTY_FUNCTION__ << " picked_index: " << _picked_index << std::endl;

        if (_picked_index != -1)
        {
            if (_mouse_state == Mouse_state::Level_element_button_selected)
            {
                QApplication::restoreOverrideCursor();
            }

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
    else if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        handled = true;
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
        if (Draggable_screen_point * d_point = dynamic_cast<Draggable_screen_point *>(_active_draggables[_picked_index]))
        {
//            d_point->set_position_from_world(Eigen::Vector3f(event->pos().x() / float(_viewer.camera()->screenWidth()), event->pos().y() / float(_viewer.camera()->screenHeight()), 0.0f));
            d_point->set_position(d_point->get_transform().inverse() * Eigen::Vector3f(event->pos().x() / float(_viewer.camera()->screenWidth()), event->pos().y() / float(_viewer.camera()->screenHeight()), 0.0f));
//            _position =  * (position - get_parent()->get_position());
            d_point->update();
        }
        else
        {
            Eigen::Hyperplane<float, 3> y_plane(Eigen::Vector3f(0.0f, 1.0f, 0.0f), _active_draggables[_picked_index]->get_position());

            qglviewer::Vec qglv_origin; // camera pos
            qglviewer::Vec qglv_dir;    // normalize(origin - camera_pos)

            _viewer.camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

            Eigen::ParametrizedLine<float, 3> const view_ray(QGLV2Eigen(qglv_origin), QGLV2Eigen(qglv_dir).normalized());

            Eigen::Vector3f const new_position = view_ray.intersectionPoint(y_plane);

            Draggable * parent = _active_draggables[_picked_index]->get_parent();

            _active_draggables[_picked_index]->set_position_from_world(new_position);
            _active_draggables[_picked_index]->update();

            auto iter = _draggable_to_level_element.find(parent);

            assert(iter != _draggable_to_level_element.end());

//            if (iter != _draggable_to_level_element.end())
            {
                Level_element * level_element = iter->second;

                //            if (!check_for_collision(level_element))
                {


                    std::cout << __PRETTY_FUNCTION__ << ": " << parent << std::endl;

                    level_element->accept(parent);

                    //            update();
                }
            }
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

        _mouse_state = Mouse_state::None;

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

        _mouse_state = Mouse_state::None;

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
    else if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        if (event->button() & Qt::LeftButton)
        {
            add_selected_level_element(event->pos());
        }
        else
        {
            _mouse_state = Mouse_state::None;
            QApplication::restoreOverrideCursor();

            _level_element_buttons[_selected_level_element_button_type]->reset();
        }

        handled = true;
    }
    else
    {
        if (_selected_level_element)
        {
            _selected_level_element->set_selected(false);
            _selected_level_element = nullptr;
        }

        _selection = Selection::None;
        _mouse_state = Mouse_state::None;
    }

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

            _viewer.add_screen(new Editor_pause_screen(_viewer, _core, this));

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

void Editor_screen::init_level_element_buttons()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    int i = 0;

//    _buttons.clear();
//    _level_element_buttons.clear();

    for (auto const& iter : _core.get_level_data()._available_elements)
    {
        auto const& image_iter = _element_images.find(iter.first);

        if (image_iter != _element_images.end()) continue;

        _element_images[iter.first] = QImage(Data_config::get_instance()->get_absolute_qfilename("textures/button_" + QString::fromStdString(iter.first) + ".png"));
    }

    for (auto const& iter : _core.get_level_data()._available_elements)
    {
        Eigen::Vector3f pos(0.05f + i * 0.06f, 0.95f, 0.0f);
        Eigen::Vector2f size(0.04f, 0.04f * _viewer.camera()->aspectRatio());

        QImage button_img = _element_images[iter.first];

        boost::shared_ptr<Draggable_button> button(new Draggable_button(pos, size, "", std::bind(&Editor_screen::level_element_button_pressed, this, std::placeholders::_1), iter.first));
        button->set_pressable(true);

        button->set_texture(_viewer.bindTexture(button_img));

        _buttons.push_back(button);
        _level_element_buttons[iter.first] = button;

        ++i;
    }

    for (std::string const& molecule : _placeable_molecules)
    {
        Eigen::Vector3f pos(0.05f + i * 0.06f, 0.95f, 0.0f);
        Eigen::Vector2f size(0.04f, 0.04f * _viewer.camera()->aspectRatio());

        boost::shared_ptr<Draggable_button> button(new Draggable_button(pos, size, "", std::bind(&Editor_screen::level_element_button_pressed, this, std::placeholders::_1), molecule));
        button->set_pressable(true);

        QImage button_img(Data_config::get_instance()->get_absolute_qfilename("textures/button_" + QString::fromStdString(molecule) + ".png"));

        button->set_texture(_viewer.bindTexture(button_img));

        _buttons.push_back(button);
        _level_element_buttons[molecule] = button;

        ++i;
    }

    _toggle_simulation_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.95f, 0.95f, 0.0f),
                                     Eigen::Vector2f(0.04f, 0.04f * _viewer.camera()->aspectRatio()),
                                     "", std::bind(&Editor_screen::toggle_simulation, this)));
    _toggle_simulation_button->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/button_play.png"))));
    _toggle_simulation_button->set_pressable(true);
//    _toggle_simulation_button->set_pressed(true);
    _toggle_simulation_button->set_parameter(_core.get_parameters()["Toggle simulation"]);

    _buttons.push_back(_toggle_simulation_button);

    _hide_controls_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.1f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Hide Controls", std::bind(&Editor_screen::hide_controls, this)));
    _viewer.generate_button_texture(_hide_controls_button.get());

    _buttons.push_back(_hide_controls_button);

    _show_controls_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.1f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Show Controls", std::bind(&Editor_screen::show_controls, this)));
    _show_controls_button->set_visible(false);
    _viewer.generate_button_texture(_show_controls_button.get());

    _buttons.push_back(_show_controls_button);

    boost::shared_ptr<Draggable_button> button_reset(
                new Draggable_button(Eigen::Vector3f(0.27f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Reset", std::bind(&Core::reset_level, &_core)));
    _viewer.generate_button_texture(button_reset.get());
    _buttons.push_back(button_reset);

    boost::shared_ptr<Draggable_button> button_clear(
                new Draggable_button(Eigen::Vector3f(0.44f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Clear", std::bind(&Editor_screen::clear_level, this)));
    _viewer.generate_button_texture(button_clear.get());
    _buttons.push_back(button_clear);


    boost::shared_ptr<Draggable_slider> translation_fluctuation_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.88f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Temperature"], std::bind(&Editor_screen::slider_changed, this)));
    translation_fluctuation_slider->set_slider_marker_texture(_slider_tex);
    translation_fluctuation_slider->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/slider_temperature.png"))));

    _sliders.push_back(translation_fluctuation_slider);

    boost::shared_ptr<Draggable_slider> damping_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.83f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Damping"], std::bind(&Editor_screen::slider_changed, this)));
    damping_slider->set_slider_marker_texture(_slider_tex);
    damping_slider->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/slider_damping.png"))));

    _sliders.push_back(damping_slider);

    boost::shared_ptr<Draggable_slider> gravity_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.78f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["gravity"], std::bind(&Editor_screen::slider_changed, this)));
    gravity_slider->set_slider_marker_texture(_slider_tex);
    gravity_slider->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/slider_gravity.png"))));


    _sliders.push_back(gravity_slider);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::level_element_button_pressed(const std::string &type)
{
    for (auto const& pair : _level_element_buttons)
    {
        if (pair.first == type) continue;

        pair.second->reset();
    }

    if (_level_element_buttons[type]->is_pressed())
    {
        _mouse_state = Mouse_state::Level_element_button_selected;
        _selected_level_element_button_type = type;
        QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
    }
    else
    {
        _mouse_state = Mouse_state::None;
        QApplication::restoreOverrideCursor();
    }
}

void Editor_screen::add_selected_level_element(const QPoint &mouse_pos)
{
    Eigen::Hyperplane<float, 3> xz_plane(Eigen::Vector3f::UnitY(),
                                         Eigen::Vector3f(0.0f, _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Y]->get_position()[1], 0.0f));

    qglviewer::Vec qglv_origin; // camera pos
    qglviewer::Vec qglv_dir;    // origin - camera_pos

    _viewer.camera()->convertClickToLine(mouse_pos, qglv_origin, qglv_dir);

    Eigen::Vector3f origin = QGLV2Eigen(qglv_origin);
    Eigen::Vector3f dir    = QGLV2Eigen(qglv_dir).normalized();

    Eigen::ParametrizedLine<float, 3> line(origin, dir);

    Eigen::Vector3f placement_position = line.intersectionPoint(xz_plane);
    placement_position[1] = 0.0f;

//    if (std::find(_placeable_molecules.begin(), _placeable_molecules.end(), _selected_level_element_button_type) == _placeable_molecules.end())
//    {
        _core.get_level_data()._available_elements[_selected_level_element_button_type] -= 1;
//    }

    add_element(placement_position, _selected_level_element_button_type);
}

void Editor_screen::toggle_simulation()
{
    _core.set_simulation_state(_toggle_simulation_button->is_pressed());
}

void Editor_screen::slider_changed()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Editor_screen::hide_controls()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    for (boost::shared_ptr<Draggable_button> const& b : _buttons)
    {
        b->set_visible(false);
    }

    for (boost::shared_ptr<Draggable_slider> const& s : _sliders)
    {
        s->set_visible(false);
    }

    _show_controls_button->set_visible(true);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::show_controls()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    for (boost::shared_ptr<Draggable_button> const& b : _buttons)
    {
        b->set_visible(true);
    }

    for (boost::shared_ptr<Draggable_slider> const& s : _sliders)
    {
        s->set_visible(true);
    }

    _show_controls_button->set_visible(false);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::clear_level()
{
    _core.clear();

    _core.load_level_defaults();
}
