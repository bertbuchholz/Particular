#include "Editor_screen.h"

#include "Editor_pause_screen.h"
#include "My_viewer.h"


std::string get_element_description(std::string const& element_type)
{
    if (element_type == std::string("H2O"))
    {
        return std::string("Water molecule");
    }
    else if (element_type == std::string("Na"))
    {
        return std::string("Positively charged sodium atom (Na+)");
    }
    else if (element_type == std::string("Cl"))
    {
        return std::string("Negatively charged chlorine atom (Cl-)");
    }
    else if (element_type == std::string("Box_barrier"))
    {
        return std::string("Blocking box which repels molecules");
    }
    else if (element_type == std::string("Brownian_box"))
    {
        return std::string("Heating/cooling element allowing to add or remove heat locally");
    }
    else if (element_type == std::string("Box_portal"))
    {
        return std::string("Box portal to capture molecules");
    }
    else if (element_type == std::string("Sphere_portal"))
    {
        return std::string("Sphere portal to capture molecules");
    }
    else if (element_type == std::string("Molecule_releaser"))
    {
        return std::string("Element that releases molecules");
    }
    else if (element_type == std::string("Charged_barrier"))
    {
        return std::string("Charged box to attract/repel charged molecules (like Na+ and Cl-)");
    }
    else if (element_type == std::string("Tractor_barrier"))
    {
        return std::string("Pushing/pulling element to move molecules along a direction");
    }

    return std::string();
}


Editor_screen::Editor_screen(My_viewer &viewer, Core &core) : Main_game_screen(viewer, core, Ui_state::Editor)
{
    _placeable_molecules = { "H2O", "Na", "Cl" };

    for (std::string const& m : _placeable_molecules)
    {
        _num_molecules_to_be_placed_per_type[m] = 1;
    }

    init_controls();

    Main_options_window::get_instance()->add_parameter_list("Editor_screen", _parameters);
}

Editor_screen::~Editor_screen()
{
    std::cout << __FUNCTION__ << std::endl;
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

    for (auto const& tooltip : _tooltips_map)
    {
        _labels.erase(std::remove(_labels.begin(), _labels.end(), tooltip.second), _labels.end());
    }

    _tooltips_map.clear();

    _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                     std::bind(&Main_game_screen::draw_draggables_for_picking, this));

    std::cout << __FUNCTION__ << " picked_index: " << _picked_index << std::endl;

    if (_picked_index != -1)
    {
        if (_mouse_state == Mouse_state::Level_element_button_selected)
        {
            QApplication::restoreOverrideCursor();

            _level_element_buttons[_selected_level_element_button_type]->reset();
        }

        _dragging_start = event->pos();

        bool found;
        qglviewer::Vec const world_pos = _viewer.camera()->pointUnderPixel(event->pos(), found);

        _mouse_state = Mouse_state::Init_drag_handle;
        std::cout << __FUNCTION__ << " Init_drag_handle" << std::endl;

        if (found)
        {
            _dragging_start_3d = QGLV2Eigen(world_pos);
        }

        handled = true;
    }
    else if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        handled = true;
    }

    return handled;
}

bool Editor_screen::mouseMoveEvent(QMouseEvent * event)
{
    bool handled = false;


    if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        handled = true;
    }
    else if (_mouse_state == Mouse_state::Init_drag_handle && _active_draggables[_picked_index]->is_draggable() && event->buttons() & Qt::LeftButton)
    {
        if (_picked_index != -1 && (_dragging_start - event->pos()).manhattanLength() > 0)
        {
            _mouse_state = Mouse_state::Dragging_handle;
        }

        handled = true;
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

            Level_element * level_element = iter->second;

            //            if (!check_for_collision(level_element))
            {
                std::cout << __FUNCTION__ << ": " << parent << std::endl;

                level_element->accept(parent);
            }
        }

        handled = true;
    }
    else if (_mouse_state == Mouse_state::None)
    {
        int const new_picking_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                         std::bind(&Main_game_screen::draw_draggables_for_picking, this));

        if (new_picking_index != _picked_index)
        {
            if (_picked_index != -1)
            {
                // stopped hovering over item
                std::cout << __FUNCTION__ << " stopped hovering" << std::endl;

                Draggable * parent = _active_draggables[_picked_index]->get_parent();
                _labels.erase(std::remove(_labels.begin(), _labels.end(), _tooltips_map[parent]), _labels.end());
                _tooltips_map.erase(parent);

                _picked_index = -1;
            }

            if (new_picking_index != -1 && new_picking_index < int(_active_draggables.size()))
            {
                // entered new picked item

                _picked_index = new_picking_index;
                std::cout << __FUNCTION__ << " started hovering" << std::endl;

                Draggable * parent = _active_draggables[_picked_index]->get_parent();

                // TODO: change to real extent, needs to be known in base Draggable
                if (!parent->get_tooltip_text().empty())
                {
                    boost::shared_ptr<Draggable_tooltip> s(_ui_renderer.generate_tooltip(parent->get_position(), Eigen::Vector3f(0.05f, 0.05f, 0.0f),
                                                                                    parent->get_tooltip_text()));

                    s->start_fade_in();

                    _labels.push_back(s);
                    _tooltips_map[parent] = s;
                }
            }
        }
    }

    return handled;
}

bool Editor_screen::mouseReleaseEvent(QMouseEvent * event)
{
    bool handled = false;

    if (_mouse_state == Mouse_state::Init_drag_handle)
    {
        std::cout << __FUNCTION__ << " click on handle" << std::endl;

        _mouse_state = Mouse_state::None;

        if (_picked_index != -1)
        {
            if (Draggable_screen_point * d_point = dynamic_cast<Draggable_screen_point *>(_active_draggables[_picked_index]))
            {
                d_point->update();
            }

            _selection = Selection::Level_element;
            Draggable * parent = _active_draggables[_picked_index]->get_parent();

            if (event->button() == Qt::LeftButton)
            {
                parent->clicked();
            }
            else if (event->button() == Qt::RightButton)
            {
                parent->right_clicked();
            }

            auto iter = _draggable_to_level_element.find(parent);
            if (iter != _draggable_to_level_element.end())
            {
                _selected_level_element = iter->second;
                _selected_level_element->set_selected(true);

                if (event->button() == Qt::RightButton)
                {
                    show_context_menu_for_element();
                }
                else
                {
                    _active_draggables[_picked_index]->clicked();
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

        if (!(event->modifiers() & Qt::ShiftModifier))
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

    std::cout << __FUNCTION__ << " " << event->key() << std::endl;

    if (event->key() == Qt::Key_Escape)
    {
        if (get_state() == State::Running)
        {
            // go into pause and start pause menu
            pause();

            _viewer.add_screen(new Editor_pause_screen(_viewer, _core, this));

            _core.set_simulation_state(false);

            QApplication::restoreOverrideCursor(); // in case we were in placement mode

            handled = true;
        }
        else if (get_state() == State::Paused || get_state() == State::Pausing)
        {
            resume();

            handled = true;
        }
    }
    else if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
    {
        delete_selected_element();
    }
    else if (event->key() == Qt::Key_F11)
    {
        std::cout << __func__ << " re-init screen" << std::endl;

        _renderer->init(_viewer.context(), QSize(_viewer.camera()->screenWidth(), _viewer.camera()->screenHeight()));

        handled = true;
    }

    return handled;
}

void Editor_screen:: init_controls()
{
    std::cout << __FUNCTION__ << std::endl;

    int i = 0;

    GL_functions f;
    f.init();

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
        button->set_tooltip_text(get_element_description(iter.first));
//        button->set_texture(_viewer.bindTexture(button_img));

        Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(button_img);
        button->set_texture(f.create_texture(texture_fb));

        _buttons.push_back(button);
        _level_element_buttons[iter.first] = button;
        _normal_controls.push_back(button);

        boost::shared_ptr<Draggable_spinbox> spinbox(
                    new Draggable_spinbox(pos + Eigen::Vector3f(0.0f, -0.1f, 0.0f),
                                          Eigen::Vector2f(0.04f, 0.1f),
                                          _core.get_level_data()._parameters["Available elements/" + iter.first]));

        spinbox->set_tooltip_text("Number of available items of this type when playing the level");
        spinbox->set_alpha(0.7f);

        _spinboxes.push_back(spinbox);

        _advanced_controls.push_back(spinbox);

        ++i;
    }


    for (std::string const& molecule : _placeable_molecules)
    {
        Eigen::Vector3f pos(0.05f + i * 0.06f, 0.95f, 0.0f);
        Eigen::Vector2f size(0.04f, 0.04f * _viewer.camera()->aspectRatio());

        boost::shared_ptr<Draggable_button> button(new Draggable_button(pos, size, "", std::bind(&Editor_screen::level_element_button_pressed, this, std::placeholders::_1), molecule));
        button->set_pressable(true);
        button->set_right_click_callback_with_data(std::bind(&Editor_screen::molecule_button_right_click_event, this, std::placeholders::_1), molecule);

//        QImage button_img(Data_config::get_instance()->get_absolute_qfilename("textures/button_" + QString::fromStdString(molecule) + ".png"));

        button->set_tooltip_text(get_element_description(molecule) + "\n(Right click for number to be added)");
//        button->set_texture(_viewer.bindTexture(button_img));
        button->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/button_" + QString::fromStdString(molecule) + ".png"), true));

        _buttons.push_back(button);
        _level_element_buttons[molecule] = button;

        _normal_controls.push_back(button);

        ++i;
    }

    _toggle_simulation_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.95f, 0.95f, 0.0f),
                                     Eigen::Vector2f(0.04f, 0.04f * _viewer.camera()->aspectRatio()),
                                     "", std::bind(&Editor_screen::toggle_simulation, this)));
    _toggle_simulation_button->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/button_play.png")));
    _toggle_simulation_button->set_pressable(true);
//    _toggle_simulation_button->set_pressed(true);
    _toggle_simulation_button->set_parameter(_core.get_parameters()["Toggle simulation"]);
    _toggle_simulation_button->set_tooltip_text("Pause/continue the simulation");

    _buttons.push_back(_toggle_simulation_button);
    _normal_controls.push_back(_toggle_simulation_button);


    boost::shared_ptr<Draggable_button> button_reset_camera = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.89f, 0.95f, 0.0f),
                                     Eigen::Vector2f(0.04f, 0.04f * _viewer.camera()->aspectRatio()),
                                     "", std::bind(&My_viewer::update_game_camera, &_viewer)));
    button_reset_camera->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/button_reset_camera.png")));
    button_reset_camera->set_tooltip_text("Reset the camera");

    _buttons.push_back(button_reset_camera);
    _normal_controls.push_back(button_reset_camera);


    boost::shared_ptr<Draggable_button> button_change_speed = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.83f, 0.95f, 0.0f),
                                     Eigen::Vector2f(0.04f, 0.04f * _viewer.camera()->aspectRatio()),
                                     "", std::bind(&Main_game_screen::change_speed_pressed, this)));
    button_change_speed->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/button_change_speed.png"))));
    button_change_speed->set_tooltip_text("Change game speed");

    _buttons.push_back(button_change_speed);
    _normal_controls.push_back(button_change_speed);


    _hide_controls_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.1f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Hide Controls", std::bind(&Editor_screen::hide_controls, this)));
    _ui_renderer.generate_button_texture(_hide_controls_button.get());
//    _hide_controls_button->set_tooltip_text("Hide the controls");

    _buttons.push_back(_hide_controls_button);
    _normal_controls.push_back(_hide_controls_button);


    _show_controls_button = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.1f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Show Controls", std::bind(&Editor_screen::show_controls, this)));
    _show_controls_button->set_visible(false);
    _ui_renderer.generate_button_texture(_show_controls_button.get());

    _buttons.push_back(_show_controls_button);
    _normal_controls.push_back(_show_controls_button);


    boost::shared_ptr<Draggable_button> button_reset(
                new Draggable_button(Eigen::Vector3f(0.27f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Reset", std::bind(&Editor_screen::reset_level, this)));
    _ui_renderer.generate_button_texture(button_reset.get());
    button_reset->set_tooltip_text("Reset and delete all non-persistent elements");

    _buttons.push_back(button_reset);
    _normal_controls.push_back(button_reset);


    boost::shared_ptr<Draggable_button> button_clear(
                new Draggable_button(Eigen::Vector3f(0.44f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Clear", std::bind(&Editor_screen::clear_level, this)));
    _ui_renderer.generate_button_texture(button_clear.get());
    button_clear->set_tooltip_text("Delete all elements");

    _buttons.push_back(button_clear);
    _normal_controls.push_back(button_clear);


    _button_show_advanced_options = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.61f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Show Adv. Options", std::bind(&Editor_screen::show_advanced_options, this)));
    _ui_renderer.generate_button_texture(_button_show_advanced_options.get());
    _button_show_advanced_options->set_tooltip_text("Show advanced options, useful for playable level design and more fun in general");
    _button_show_advanced_options->set_visible(false);

    _buttons.push_back(_button_show_advanced_options);
    _normal_controls.push_back(_button_show_advanced_options);


    _button_hide_advanced_options = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.61f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Hide Adv. Options", std::bind(&Editor_screen::hide_advanced_options, this)));
    _ui_renderer.generate_button_texture(_button_hide_advanced_options.get());

    _buttons.push_back(_button_hide_advanced_options);
    _normal_controls.push_back(_button_hide_advanced_options);


    boost::shared_ptr<Draggable_button> button_load_defaults = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.78f, 0.05f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.05f * _viewer.camera()->aspectRatio()),
                                     "Load Defaults", std::bind(&Editor_screen::load_defaults, this)));
    _ui_renderer.generate_button_texture(button_load_defaults.get());
    button_load_defaults->set_tooltip_text("Load default settings");
    button_load_defaults->set_visible(true);



    // Normal Controls

    _buttons.push_back(button_load_defaults);
    _normal_controls.push_back(button_load_defaults);


    boost::shared_ptr<Draggable_slider> translation_fluctuation_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.88f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Temperature"]));
    translation_fluctuation_slider->set_slider_marker_texture(_slider_tex);
    translation_fluctuation_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_temperature.png")));
    translation_fluctuation_slider->set_tooltip_text("Overall temperature control, things start moving fast and randomly when it is hot\n(Right click for numerical input)");

    translation_fluctuation_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Temperature");
    _slider_parameter_names_to_parameters["Temperature"] = _core.get_level_data()._parameters["Temperature"];

    _sliders.push_back(translation_fluctuation_slider);
    _normal_controls.push_back(translation_fluctuation_slider);


    boost::shared_ptr<Draggable_slider> damping_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.83f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Damping"]));
    damping_slider->set_slider_marker_texture(_slider_tex);
    damping_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_damping.png")));
    damping_slider->set_tooltip_text("Damping, controls how quickly moving objects lose their speed\n(Right click for numerical input)");

    damping_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Damping");
    _slider_parameter_names_to_parameters["Damping"] = _core.get_level_data()._parameters["Damping"];

    _sliders.push_back(damping_slider);
    _normal_controls.push_back(damping_slider);


    boost::shared_ptr<Draggable_slider> gravity_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.78f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["gravity"]));
    gravity_slider->set_slider_marker_texture(_slider_tex);
    gravity_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_gravity.png")));
    gravity_slider->set_tooltip_text("Gravity, controls how fast the apple falls\n(Right click for numerical input)");

    gravity_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Gravity");
    _slider_parameter_names_to_parameters["Gravity"] = _core.get_level_data()._parameters["gravity"];

    _sliders.push_back(gravity_slider);
    _normal_controls.push_back(gravity_slider);


    // Advanced Controls

    boost::shared_ptr<Draggable_label> adv_options_label(
                new Draggable_label(Eigen::Vector3f(0.9f, 0.71f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.07f),
                                     "Adv. Controls:"));
    _ui_renderer.generate_label_texture(adv_options_label.get());

    _labels.push_back(adv_options_label);
    _advanced_controls.push_back(adv_options_label);


    boost::shared_ptr<Draggable_slider> mass_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.67f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_parameters()["Mass Factor"]));
    mass_slider->set_slider_marker_texture(_slider_tex);
    mass_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_mass.png")));
    mass_slider->set_tooltip_text("Mass, controls the relative mass of the atoms\n(Right click for numerical input)");

    mass_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Mass Factor");
    _slider_parameter_names_to_parameters["Mass Factor"] = _core.get_parameters()["Mass Factor"];

    _sliders.push_back(mass_slider);
    _advanced_controls.push_back(mass_slider);


    boost::shared_ptr<Draggable_slider> coulomb_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.62f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_parameters()["Atomic Force Type/Coulomb Force/Strength"]));
    coulomb_slider->set_slider_marker_texture(_slider_tex);
    coulomb_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_coulomb.png")));
    coulomb_slider->set_tooltip_text("Coulomb Force, controls the strength of electric attraction and repulsion between molecules\n(Right click for numerical input)");

    coulomb_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Coulomb Force");
    _slider_parameter_names_to_parameters["Coulomb Force"] = _core.get_parameters()["Atomic Force Type/Coulomb Force/Strength"];

    _sliders.push_back(coulomb_slider);
    _advanced_controls.push_back(coulomb_slider);


    boost::shared_ptr<Draggable_slider> waals_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.57f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_parameters()["Atomic Force Type/Van der Waals Force/Strength"]));
    waals_slider->set_slider_marker_texture(_slider_tex);
    waals_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_waals.png")));
    waals_slider->set_tooltip_text("Van der Waals Potential, controls the strength of attraction between chargeless molecules\n(Right click for numerical input)");

    waals_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Van der Waals Force");
    _slider_parameter_names_to_parameters["Van der Waals Force"] = _core.get_parameters()["Atomic Force Type/Van der Waals Force/Strength"];

    _sliders.push_back(waals_slider);
    _advanced_controls.push_back(waals_slider);


    boost::shared_ptr<Draggable_slider> width_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.52f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Game Field Width"]));
    width_slider->set_slider_marker_texture(_slider_tex);
    width_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_width.png")));
    width_slider->set_tooltip_text("Width of the containing box\n(Right click for numerical input)");

    width_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Game Field Width");
    _slider_parameter_names_to_parameters["Game Field Width"] = _core.get_level_data()._parameters["Game Field Width"];

    _sliders.push_back(width_slider);
    _advanced_controls.push_back(width_slider);


    boost::shared_ptr<Draggable_slider> height_slider(
                new Draggable_slider(Eigen::Vector3f(0.93f, 0.47f, 0.0f),
                                     Eigen::Vector2f(0.1f, 0.05f),
                                     _core.get_level_data()._parameters["Game Field Height"]));
    height_slider->set_slider_marker_texture(_slider_tex);
    height_slider->set_texture(f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider_height.png")));
    height_slider->set_tooltip_text("Height of the containing box\n(Right click for numerical input)");

    height_slider->set_right_click_callback_with_data(std::bind(&Editor_screen::parameter_slider_right_click_event, this, std::placeholders::_1), "Game Field Height");
    _slider_parameter_names_to_parameters["Game Field Height"] = _core.get_level_data()._parameters["Game Field Height"];

    _sliders.push_back(height_slider);
    _advanced_controls.push_back(height_slider);


    hide_advanced_options();

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

void Editor_screen::molecule_button_right_click_event(std::string const& type)
{
    std::cout << __FUNCTION__ << std::endl;

    QMenu menu;

    QWidgetAction * action_num_max_molecules = new QWidgetAction(this);

    QSpinBox * spinbox_num_molecules = new QSpinBox();
    {
        spinbox_num_molecules->setMaximum(1000);
        spinbox_num_molecules->setMinimum(1);
        spinbox_num_molecules->setValue(_num_molecules_to_be_placed_per_type[type]);

        action_num_max_molecules->setDefaultWidget(new Widget_text_combination(QString("Number of %1 molecules to place").arg(QString::fromStdString(type)), spinbox_num_molecules));
    }

    menu.addAction(action_num_max_molecules);

    menu.exec(QCursor::pos());

    _num_molecules_to_be_placed_per_type[type] = spinbox_num_molecules->value();

    delete action_num_max_molecules;
}

void Editor_screen::parameter_slider_right_click_event(std::string const& parameter_name)
{
    std::cout << __FUNCTION__ << std::endl;

    Parameter * parameter = _slider_parameter_names_to_parameters[parameter_name];

    QString min_max_str = QString("(Min: %1, Max: %2)").arg(parameter->get_min<float>(), 0, 'f', 1).arg(parameter->get_max<float>(), 0, 'f', 1);

    QMenu menu;

    QWidgetAction * action_param = new QWidgetAction(this);

    QDoubleSpinBox * spinbox_param = new QDoubleSpinBox();
    {
        float const step = (parameter->get_max<float>() - parameter->get_min<float>()) / 50.0f;
        spinbox_param->setSingleStep(step);
        spinbox_param->setRange(parameter->get_min<float>(), parameter->get_max<float>());
        spinbox_param->setValue(parameter->get_value<float>());

        action_param->setDefaultWidget(new Widget_text_combination(QString::fromStdString(parameter_name) + " " + min_max_str, spinbox_param));
    }

    menu.addAction(action_param);

    menu.exec(QCursor::pos());

    parameter->set_value(float(spinbox_param->value()));

    delete action_param;
}


void Editor_screen::add_selected_level_element(const QPoint &mouse_pos)
{
    bool const is_molecule_added = std::find(_placeable_molecules.begin(), _placeable_molecules.end(), _selected_level_element_button_type) != _placeable_molecules.end();

    Eigen::Hyperplane<float, 3> intersection_plane;

    int num_to_add = 1;

    if (is_molecule_added)
    {
        intersection_plane = Eigen::Hyperplane<float, 3>(Eigen::Vector3f::UnitY(), Eigen::Vector3f::Zero());
        num_to_add = _num_molecules_to_be_placed_per_type[_selected_level_element_button_type];
    }
    else
    {
        intersection_plane = Eigen::Hyperplane<float, 3>(Eigen::Vector3f::UnitY(),
                                             Eigen::Vector3f(0.0f, _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Y]->get_position()[1], 0.0f));
    }


    qglviewer::Vec qglv_origin; // camera pos
    qglviewer::Vec qglv_dir;    // origin - camera_pos

    _viewer.camera()->convertClickToLine(mouse_pos, qglv_origin, qglv_dir);

    Eigen::Vector3f origin = QGLV2Eigen(qglv_origin);
    Eigen::Vector3f dir    = QGLV2Eigen(qglv_dir).normalized();

    Eigen::ParametrizedLine<float, 3> line(origin, dir);

    Eigen::Vector3f placement_position = line.intersectionPoint(intersection_plane);
    placement_position[1] = 0.0f;

//    if (std::find(_placeable_molecules.begin(), _placeable_molecules.end(), _selected_level_element_button_type) == _placeable_molecules.end())
//    {
//        _core.get_level_data()._available_elements[_selected_level_element_button_type] -= 1;
//    }

    add_element(placement_position, _selected_level_element_button_type, false, num_to_add);
}

void Editor_screen::toggle_simulation()
{
    _core.set_simulation_state(_toggle_simulation_button->is_pressed());
}

void Editor_screen::slider_changed()
{
    std::cout << __FUNCTION__ << std::endl;
}

void Editor_screen::reset_level()
{
    QMessageBox msgBox(QMessageBox::Warning, "Reset Level", "The level elements will be reset and all molecules removed.");
    QPushButton * keep_molecules_button = msgBox.addButton(tr("Keep molecules"), QMessageBox::ActionRole);
    QPushButton * continue_button = msgBox.addButton(tr("Continue"), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Abort);

    msgBox.exec();

    if (msgBox.clickedButton() == keep_molecules_button)
    {
        _core.reset_level(true);
    }
    else if (msgBox.clickedButton() == continue_button)
    {
        _core.reset_level(false);
    }
}

void Editor_screen::hide_controls()
{
    std::cout << __FUNCTION__ << std::endl;

    for (boost::shared_ptr<Draggable> const& d : _normal_controls)
    {
        d->set_visible(false);
    }

    for (boost::shared_ptr<Draggable> const& d : _advanced_controls)
    {
        d->set_visible(false);
    }

    _show_controls_button->set_visible(true);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::show_controls()
{
    std::cout << __FUNCTION__ << std::endl;

//    for (boost::shared_ptr<Draggable_button> const& b : _buttons)
//    {
//        b->set_visible(true);
//    }

//    for (boost::shared_ptr<Draggable_slider> const& s : _sliders)
//    {
//        s->set_visible(true);
//    }

//    for (boost::shared_ptr<Draggable_spinbox> const& s : _spinboxes)
//    {
//        s->set_visible(true);
//    }

    for (boost::shared_ptr<Draggable> const& d : _normal_controls)
    {
        d->set_visible(true);
    }

    _show_controls_button->set_visible(false);

    _button_hide_advanced_options->set_visible(false);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::clear_level()
{
    QMessageBox::StandardButton button = QMessageBox::warning(&_viewer, "Clear Level", "This will clear the level, unsaved work will be lost. Continue?", QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes)
    {
        _core.clear();

        _core.load_level_defaults();
    }
}

void Editor_screen::show_advanced_options()
{
//    Parameter_list elements_list;
//    elements_list.add_child("Available elements", _core.get_level_data()._parameters.get_child("Available elements"));

//    Parameter_list sim_list;
//    sim_list.add_parameter(_core.get_parameters()["Mass Factor"]);
//    sim_list.add_child("Intermolecular Forces", _core.get_parameters().get_child("Atomic Force Type"));

//    _advanced_options_window = std::unique_ptr<Main_options_window>(Main_options_window::create());

//    _advanced_options_window->add_parameter_list("Level Options", elements_list);
//    _advanced_options_window->add_parameter_list("Adv. Simulation Options", sim_list);

////    _advanced_options_window->add_parameter_list("Available elements", *_core.get_level_data()._parameters.get_child("Available elements"));

////    _advanced_options_window->add_parameter_list("Intermolecular Forces", *_core.get_parameters().get_child("Atomic Force Type"));

//    _advanced_options_window->show();

    for (boost::shared_ptr<Draggable> const& d : _advanced_controls)
    {
        d->set_visible(true);
    }

    _button_show_advanced_options->set_visible(false);
    _button_hide_advanced_options->set_visible(true);

    update_draggable_to_level_element();
    update_active_draggables();
}


void Editor_screen::hide_advanced_options()
{
    for (boost::shared_ptr<Draggable> const& d : _advanced_controls)
    {
        d->set_visible(false);
    }

    _button_hide_advanced_options->set_visible(false);
    _button_show_advanced_options->set_visible(true);

    update_draggable_to_level_element();
    update_active_draggables();
}

void Editor_screen::load_defaults()
{
    bool const b = _core.get_parameters()["Toggle simulation"]->get_value<bool>();
    _core.load_default_simulation_settings();
    _core.get_level_data().load_defaults();
    _core.get_parameters()["Toggle simulation"]->set_value(b);
}
