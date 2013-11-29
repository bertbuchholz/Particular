#include "Main_game_screen.h"

#include "My_viewer.h"
#include "Pause_screen.h"
#include "Editor_pause_screen.h"
#include "Before_start_screen.h"
#include "After_finish_screen.h"
#include "After_finish_editor_screen.h"
#include "FloatSlider.h"
#include "GL_texture.h"

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

//Main_game_screen::Main_game_screen(My_viewer &viewer, Core &core, std::unique_ptr<World_renderer> &renderer) : Screen(viewer),
Main_game_screen::Main_game_screen(My_viewer &viewer, Core &core, Ui_state ui_state) : Screen(viewer),
    _core(core),
//    _renderer(renderer),
    _ui_renderer(_viewer.get_renderer()),
    _picked_index(-1),
    _mouse_state(Mouse_state::None),
    _selection(Selection::None),
    _selected_level_element(nullptr),
    _ui_state(ui_state),
    _level_state(Level_state::Running)
{
    std::cout << __FUNCTION__ << std::endl;

    GL_functions f;
    f.init(_viewer.context());

    _type = Screen::Type::Fullscreen;
//    _state = State::Running;

    initializeGLFunctions(_viewer.context());

    _picking.init(_viewer.context());

    _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);

    _rotate_tex = f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/rotate.png"));
    _move_tex = f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/move.png"));
    _scale_tex = f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/scale.png"));
    _slider_tex = f.create_texture(Data_config::get_instance()->get_absolute_qfilename("textures/slider.png"));

    _main_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(QSize(_viewer.camera()->screenWidth(), _viewer.camera()->screenHeight()), QGLFramebufferObject::Depth));

    _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(_viewer.context(),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/simple_texture.frag")));

    _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(_viewer.context(),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                   Data_config::get_instance()->get_absolute_qfilename("shaders/blur_1D.frag")));


    _drop_shadow_program = std::unique_ptr<QGLShaderProgram>(init_program(_viewer.context(),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/drop_shadow.frag")));

    _tmp_screen_texture[0].set_context(_viewer.context());
    _tmp_screen_texture[1].set_context(_viewer.context());
    _tmp_screen_texture[0].reset(f.create_texture(_viewer.camera()->screenWidth(), _viewer.camera()->screenHeight()));
    _tmp_screen_texture[1].reset(f.create_texture(_viewer.camera()->screenWidth(), _viewer.camera()->screenHeight()));

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

    _parameters.add_parameter(new Parameter("Object Type", 0, object_types));

    Parameter_registry<World_renderer>::create_single_select_instance(&_parameters, "Renderer", std::bind(&Main_game_screen::change_renderer, this));

    _parameters["Renderer/type"]->set_value<std::string>("Shader Renderer");

    Main_options_window::get_instance()->add_parameter_list("Main_game_screen", _parameters);

    connect(&_core, SIGNAL(level_changed(Main_game_screen::Level_state)), this, SLOT(handle_level_change(Main_game_screen::Level_state)));
    connect(&_core, SIGNAL(game_state_changed()), this, SLOT(handle_game_state_change()));
}

Main_game_screen::~Main_game_screen()
{
    std::cout << __FUNCTION__ << std::endl;
    Main_options_window::get_instance()->remove_parameter_list("Main_game_screen");
}

bool Main_game_screen::mousePressEvent(QMouseEvent * event)
{
    bool handled = false;

    if (_level_state == Level_state::Intro)
    {
        handled = true;
    }
    else if (_mouse_state == Mouse_state::Level_element_button_selected)
    {
        handled = true;
    }
    else
    {
        _picked_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                         std::bind(&Main_game_screen::draw_draggables_for_picking, this));

        std::cout << __FUNCTION__ << " picked_index: " << _picked_index << std::endl;

        if (_picked_index != -1)
        {
            _dragging_start = event->pos();

            bool found;
            qglviewer::Vec world_pos = _viewer.camera()->pointUnderPixel(event->pos(), found);

            _mouse_state = Mouse_state::Init_drag_handle;
            std::cout << __FUNCTION__ << " Init_drag_handle" << std::endl;

            if (found)
            {
                _dragging_start_3d = QGLV2Eigen(world_pos);
            }

            handled = true;
        }
    }

    return handled;
}

bool Main_game_screen::mouseMoveEvent(QMouseEvent * event)
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

            std::cout << __FUNCTION__ << ": " << parent << std::endl;

            level_element->accept(parent);

//            update();
        }
    }

    return handled;
}

bool Main_game_screen::mouseReleaseEvent(QMouseEvent * event)
{
    bool handled = false;

    if (_mouse_state == Mouse_state::Init_drag_handle)
    {
        std::cout << __FUNCTION__ << " click on handle" << std::endl;

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

bool Main_game_screen::keyPressEvent(QKeyEvent * event)
{
    bool handled = false;

    std::cout << __FUNCTION__ << " " << event->key() << std::endl;

    if (event->key() == Qt::Key_Escape)
    {
            if (get_state() == State::Running && _level_state != Level_state::Intro)
            {
                // go into pause and start pause menu
                pause();

                if (_ui_state == Ui_state::Level_editor)
                {
                    _viewer.add_screen(new Editor_pause_screen(_viewer, _core, this));
                }
                else
                {
                    _viewer.add_screen(new Pause_screen(_viewer, _core, this));
                }

                _core.set_simulation_state(false);

                handled = true;
            }
            else if ((get_state() == State::Paused || get_state() == State::Pausing) && _level_state != Level_state::Intro)
            {
                resume();

                handled = true;
            }
            else if (_level_state == Level_state::Intro)
            {
                pause();

                _viewer.camera()->deletePath(0);

                _core.load_next_level();

                _viewer.add_screen(new Before_start_screen(_viewer, _core));

                _level_state = Level_state::Running;

                handled = true;
            }
    }

    return handled;
}

void Main_game_screen::draw()
{
    _renderer->setup_gl_points(true);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    _main_fbo->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[0].get_id(), 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _main_fbo->release();

    _renderer->render(_main_fbo.get(), _core.get_level_data(), _core.get_current_time(), _viewer.camera());

    _renderer->setup_gl_points(false);
    glPointSize(8.0f);

    glDisable(GL_LIGHTING);

    _main_fbo->bind();
    draw_draggables();
    _main_fbo->release();

    glDisable(GL_DEPTH_TEST);


    if (get_state() == State::Running)
    {
        _screen_quad_program->bind();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _screen_quad_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[0].get_id());
//        glBindTexture(GL_TEXTURE_2D, _main_fbo->texture());

    //        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[1]);
        draw_quad_with_tex_coords();
        _screen_quad_program->release();
    }
    else if (get_state() == State::Pausing || get_state() == State::Resuming || get_state() == State::Paused)
    {
        float blur_strength = 20.0f * _viewer.camera()->screenWidth() / 1000.0f;

        if (get_state() == State::Pausing)
        {
            blur_strength *= _transition_progress;
        }
        else if (get_state() == State::Resuming)
        {
            blur_strength *= (1.0f - _transition_progress);
        }

        _main_fbo->bind();

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[1].get_id(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _blur_program->bind();
        _blur_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[0].get_id());

        _blur_program->setUniformValue("tex_size", QSize(_viewer.camera()->screenWidth(), _viewer.camera()->screenHeight()));
        _blur_program->setUniformValue("blur_strength", blur_strength);
        _blur_program->setUniformValue("direction", QVector2D(1.0, 0.0));

        draw_quad_with_tex_coords();

        _main_fbo->release();

//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[0], 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _blur_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[1].get_id());
        _blur_program->setUniformValue("direction", QVector2D(0.0, 1.0));
        draw_quad_with_tex_coords();

        _blur_program->release();
    }
}

void Main_game_screen::state_changed_event(const Screen::State new_state, const Screen::State previous_state)
{
    std::cout << __FUNCTION__ << " " << int(new_state) << " " << int(previous_state) << std::endl;

    if (new_state == State::Running)
    {
        _core.set_simulation_state(true);
    }
}


void Main_game_screen::update_event(const float time_step)
{
    if (get_state() == State::Running)
    {
        // normal update

//        if (_mouse_state == Mouse_state::Dragging_molecule)
//        {
//            boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

//            assert(picked_molecule);

//            Molecule_external_force & f = _core.get_user_force();

//            Eigen::Vector3f old_force_target = f._origin + f._force;

//            f._origin = picked_molecule->_R * f._local_origin + picked_molecule->_x;
//            f._force = old_force_target - f._origin;
//            f._end_time = _core.get_current_time() + 0.1f;
//        }
    }

    if (_level_state == Level_state::Intro)
    {
        update_intro(time_step);
    }

    for (auto & l : _labels)
    {
        l->animate(time_step);
    }
}

void Main_game_screen::delete_selected_element()
{
    if (_selection == Selection::Molecule)
    {

    }
    else if (_selection == Selection::Level_element)
    {
//        Draggable * parent = _active_draggables[_picked_index]->get_parent();
//        assert(_draggable_to_level_element.find(parent) != _draggable_to_level_element.end());

//        _core.get_level_data().delete_level_element(_draggable_to_level_element.find(parent)->second);
        _core.get_level_data().delete_level_element(_selected_level_element);

        update_draggable_to_level_element();
        update_active_draggables();

        _selection = Selection::None;
        _selected_level_element = nullptr;
    }
}

void Main_game_screen::show_context_menu_for_element()
{
    Draggable * parent = _active_draggables[_picked_index]->get_parent();

    assert(_draggable_to_level_element.find(parent) != _draggable_to_level_element.end());

    std::cout << __FUNCTION__ << ": " << parent << std::endl;

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

void Main_game_screen::draw_molecules_for_picking()
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

void Main_game_screen::draw_draggables_for_picking()
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

        if (Draggable_screen_point const* d_point = dynamic_cast<Draggable_screen_point const*>(draggable))
        {
            _viewer.start_normalized_screen_coordinates();

            Eigen::Transform<float, 3, Eigen::Affine> parent_system = Eigen::Transform<float, 3, Eigen::Affine>::Identity();

            if (d_point->get_parent())
            {
                parent_system = d_point->get_transform();
            }

            Eigen::Vector3f const point_pos = parent_system * d_point->get_position();

            glTranslatef(point_pos[0], point_pos[1], point_pos[2]);
            // TODO: extent must be part of draggable, otherwise all Draggable_screen_point will be of the same size
            glScalef(0.01f, 0.01f * _viewer.camera()->aspectRatio(), 1.0f);

            draw_quad_with_tex_coords();
            _viewer.stop_normalized_screen_coordinates();
        }
        else if (Draggable_point const* d_point = dynamic_cast<Draggable_point const*>(draggable))
        {
            draw_box_from_center(d_point->get_position(), Eigen::Vector3f(scale, scale, scale));
        }
        else if (Draggable_disc const* d_point = dynamic_cast<Draggable_disc const*>(draggable))
        {
            draw_sphere_ico(Eigen2OM(d_point->get_position()), 2.0f);
        }
        else if (Draggable_button const* d_button = dynamic_cast<Draggable_button const*>(draggable))
        {
            _viewer.start_normalized_screen_coordinates();
            _viewer.draw_button(d_button, true);
            _viewer.stop_normalized_screen_coordinates();
        }
//        else if (Draggable_slider const* d_slider = dynamic_cast<Draggable_slider const*>(draggable))
//        {
//            _viewer.start_normalized_screen_coordinates();
//            _viewer.draw_slider(d_slider, true);
//            _viewer.stop_normalized_screen_coordinates();
//        }

        glPopMatrix();
    }
}

void Main_game_screen::update_active_draggables()
{
    _active_draggables.clear();

    for (auto const& d : _draggable_to_level_element)
    {
        Level_element::Edit_type edit_type = Level_element::Edit_type::All;

        if (_core.get_game_state() == Core::Game_state::Running)
//        if (_ui_state != Ui_state::Level_editor)
        {
            edit_type = d.second->is_user_editable();
        }

        std::vector<Draggable*> const draggables = d.first->get_draggables(edit_type);
        std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        if (button->is_visible())
        {
            Draggable_button * b = button.get();

            std::vector<Draggable*> const draggables = b->get_draggables(Level_element::Edit_type::None);
            std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
        }
    }

    for (boost::shared_ptr<Draggable_slider> const& slider : _sliders)
    {
        if (slider->is_visible())
        {
            Draggable_slider * b = slider.get();

            std::vector<Draggable*> const draggables = b->get_draggables(Level_element::Edit_type::None);
            std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
        }
    }

    for (boost::shared_ptr<Draggable_spinbox> const& spinbox : _spinboxes)
    {
        if (spinbox->is_visible())
        {
            Draggable_spinbox * b = spinbox.get();

            std::vector<Draggable*> const draggables = b->get_draggables(Level_element::Edit_type::None);
            std::copy(draggables.begin(), draggables.end(), std::back_inserter(_active_draggables));
        }
    }

}

void Main_game_screen::update_draggable_to_level_element()
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
            else if (Charged_barrier const* b = dynamic_cast<Charged_barrier const*>(e))
            {
                Draggable_box * draggable = new Draggable_box(b->get_position(), b->get_extent(), b->get_transform(), b->get_parameters());
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

void Main_game_screen::draw_draggables() // FIXME: use visitors or change it so that draggables can only have a single type of handles (Draggable_point)
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

            bool const always_draw = _ui_state == Ui_state::Level_editor && _core.get_game_state() != Core::Game_state::Running;

            //                draw_box(d_box->get_min(), d_box->get_max());

            glPushMatrix();

            glTranslatef(d_box->get_position()[0], d_box->get_position()[1], d_box->get_position()[2]);
            glMultMatrixf(d_box->get_transform().data());

            if (always_draw || (int(edit_type) & int(Level_element::Edit_type::Scale)))
            {
                for (Draggable_point const& p : corners)
                {
                    glPushMatrix();

                    glTranslatef(p.get_position()[0] + z_offset, p.get_position()[1] + z_offset, p.get_position()[2] + z_offset);
                    glScalef(scale, scale, scale);
                    glRotatef(90, 1.0, 0.0, 0.0);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                    _viewer.draw_textured_quad(_scale_tex);

                    glPopMatrix();
                }
            }

            if (always_draw || (int(edit_type) & int(Level_element::Edit_type::Translate)))
            {
                for (Draggable_disc const& d_disc : d_box->get_position_points())
                {
                    glPushMatrix();

                    glTranslatef(d_disc.get_position()[0] + z_offset, d_disc.get_position()[1] + z_offset, d_disc.get_position()[2] + z_offset);
                    glScalef(scale, scale, scale);
                    glRotatef(90, 1.0, 0.0, 0.0);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                    _viewer.draw_textured_quad(_move_tex);

                    glPopMatrix();
                }
            }

            if (always_draw || (int(edit_type) & int(Level_element::Edit_type::Rotate)))
            {
                for (Draggable_disc const& d_disc : d_box->get_rotation_handles())
                {
                    glPushMatrix();

                    glTranslatef(d_disc.get_position()[0] + z_offset, d_disc.get_position()[1] + z_offset, d_disc.get_position()[2] + z_offset);
                    glScalef(scale, scale, scale);
                    glRotatef(90, 1.0, 0.0, 0.0);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                    _viewer.draw_textured_quad(_rotate_tex);

                    glPopMatrix();
                }
            }

            if (always_draw || (int(edit_type) & int(Level_element::Edit_type::Property)))
            {
                for (auto iter : d_box->get_property_handles())
                {
                    Eigen::Vector3f const& p = iter.second.get_position();

                    glPushMatrix();

                    glTranslatef(p[0] + z_offset, p[1] - 0.03f, p[2] + z_offset);
                    glScalef(0.9f, 0.9f, 0.9f);
                    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    _viewer.draw_textured_quad(_slider_tex);

                    glPopMatrix();
                }
            }

            glPopMatrix();
        }
    }


    _viewer.start_normalized_screen_coordinates();

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        if (button->is_visible())
        {
            _viewer.draw_button(button.get(), false);
        }
    }

    for (boost::shared_ptr<Draggable_slider> const& slider : _sliders)
    {
        if (slider->is_visible())
        {
            _viewer.draw_slider(*slider.get(), false);
        }
    }

    for (boost::shared_ptr<Draggable_spinbox> const& spinbox : _spinboxes)
    {
        if (spinbox->is_visible())
        {
            _ui_renderer.draw_spinbox(*spinbox.get(), false);
        }
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _drop_shadow_program->bind();

        _drop_shadow_program->setUniformValue("texture", 0);
        _drop_shadow_program->setUniformValue("blur_size", 5);
        _drop_shadow_program->setUniformValue("offset", 0.01f);
        _drop_shadow_program->setUniformValue("tex_size", QVector2D(label->get_extent()[0] * _viewer.width(), label->get_extent()[1] * _viewer.height()));
        glActiveTexture(GL_TEXTURE0);

        if (label->is_visible())
        {
            _viewer.draw_label(label.get());
        }

        _drop_shadow_program->release();
    }

    _viewer.stop_normalized_screen_coordinates();

    glEnable(GL_DEPTH_TEST);
}

void Main_game_screen::add_element_event(const QPoint &position)
{
    qglviewer::Vec qglv_origin;
    qglviewer::Vec qglv_dir;

    _viewer.camera()->convertClickToLine(position, qglv_origin, qglv_dir);

    Vec origin = QGLV2OM(qglv_origin);
    Vec dir    = QGLV2OM(qglv_dir);

    float const t = ray_plane_intersection(origin, dir, Vec(0.0f, 0.0f, 0.0f), Vec(0.0f, 1.0f, 0.0f));

    if (t > 0)
    {
        Eigen::Vector3f const intersect_pos = OM2Eigen(origin + t * dir);

        std::string const element_type = _parameters["Object Type"]->get_value<std::string>();

        add_element(intersect_pos, element_type);
    }
}

void Main_game_screen::add_element(const Eigen::Vector3f &position, const std::string &element_type, bool const make_fully_editable)
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

        Box_barrier * barrier = new Box_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius);

        if (make_fully_editable)
        {
            barrier->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_barrier(barrier);
    }
    else if (element_type == std::string("Moving_box_barrier"))
    {
        float const strength = 10000.0f;
        float const radius   = 2.0f;

        Moving_box_barrier * b = new Moving_box_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius);

        if (make_fully_editable)
        {
            b->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_barrier(b);
    }
    else if (element_type == std::string("Blow_barrier"))
    {
        float const strength = 10000.0f;
        float const radius   = 2.0f;

        Blow_barrier * e = new Blow_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position,
                                            Blow_barrier::Axis::X, 30.0f,
                                            strength, radius);
        e->set_user_editable(Level_element::Edit_type::All);
        _core.get_level_data().add_barrier(e);
    }
    else if (element_type == std::string("Plane_barrier"))
    {
        float const strength = 10000.0f;
        float const radius   = 5.0f;

        Plane_barrier * b = new Plane_barrier(position, Eigen::Vector3f::UnitZ(), strength, radius, Eigen::Vector2f(10.0f, 20.0));

        if (make_fully_editable)
        {
            b->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_barrier(b);
    }
    else if (element_type == std::string("Brownian_box"))
    {
        float const strength = 0.0f;
        float const radius   = 25.0f;

        Brownian_box * e = new Brownian_box(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, strength, radius);
        e->set_user_editable(Level_element::Edit_type::All);
        e->set_persistent(false);
        _core.get_level_data().add_brownian_element(e);
    }
    else if (element_type == std::string("Box_portal"))
    {
        Box_portal * p = new Box_portal(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position);

        if (make_fully_editable)
        {
            p->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_portal(p);
    }
    else if (element_type == std::string("Sphere_portal"))
    {
        Sphere_portal * p = new Sphere_portal(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position);

        if (make_fully_editable)
        {
            p->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_portal(p);
    }
    else if (element_type == std::string("Molecule_releaser"))
    {
        Molecule_releaser * m = new Molecule_releaser(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, 1.0f, 1.0f);
        m->set_molecule_type("H2O");

        if (make_fully_editable)
        {
            m->set_user_editable(Level_element::Edit_type::All);
        }

        _core.get_level_data().add_molecule_releaser(m);
    }
    else if (element_type == std::string("Atom_cannon"))
    {
        Atom_cannon * m = new Atom_cannon(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, back_pos, 10.0f) + position, 1.0f, 1.0f, 10.0f, 0.0f);
        _core.get_level_data().add_molecule_releaser(m);
    }
    else if (element_type == std::string("Charged_barrier"))
    {
        float const strength = 10000.0f;
        float const radius   = 2.0f;

        Charged_barrier * b = new Charged_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + position, strength, radius, 0.0f);

        b->set_user_editable(Level_element::Edit_type::All);

        _core.get_level_data().add_barrier(b);
    }
    else if (element_type == std::string("Tractor_barrier"))
    {
        float const strength = 10000.0f;

        Tractor_barrier * b = new Tractor_barrier(Eigen::Vector3f(-10.0f, front_pos, -10.0f) + position, Eigen::Vector3f(10.0f, 20.0f, 10.0f) + position, strength);
        b->set_user_editable(Level_element::Edit_type::All);
        b->set_persistent(false);
        _core.get_level_data().add_barrier(b);
    }
    else
    {
        std::cout << __FUNCTION__ << " element does not exist: " << element_type << std::endl;
    }

    update_draggable_to_level_element();
    update_active_draggables();
}

void Main_game_screen::update_level_element_buttons()
{
    int i = 0;

//    GL_functions f;
//    f.init(_viewer.context());

    _buttons.clear();

    for (auto const& iter : _core.get_level_data()._available_elements)
    {
        auto const& image_iter = _element_images.find(iter.first);

        if (image_iter != _element_images.end()) continue;

        _element_images[iter.first] = QImage(Data_config::get_instance()->get_absolute_qfilename("textures/button_" + QString::fromStdString(iter.first) + ".png"));
    }

    for (auto const& iter : _core.get_level_data()._available_elements)
    {
        if (iter.second > 0)
        {
            Eigen::Vector3f pos(0.05f + i * 0.06f, 0.95f, 0.0f);
            Eigen::Vector2f size(0.04f, 0.04f * _viewer.camera()->aspectRatio());

            boost::shared_ptr<Draggable_button> button(new Draggable_button(pos, size, "", std::bind(&Main_game_screen::level_element_button_pressed, this, std::placeholders::_1), iter.first));

            //                QImage button_img(100, 100, QImage::Format_ARGB32);
            //                button_img.fill(Qt::black);

            //                button->set_texture(bindTexture(img));

            QImage button_img = _element_images[iter.first];

            QPainter p(&button_img);

            QFont font = _ui_renderer.get_main_font();
            //        font.setWeight(QFont::Bold);
            font.setPixelSize(100);
            //                font.setPointSizeF(20.0f);
            p.setFont(font);

            p.setPen(QColor(0, 0, 0));

            p.drawText(25, 200, QString("%1").arg(iter.second));

            //                p.drawText(QRect(5, 5, 90, 90), Qt::AlignBottom | Qt::AlignRight, QString("%1").arg(iter.second));

            _viewer.deleteTexture(button->get_texture());
            button->set_texture(_viewer.bindTexture(button_img));

            _buttons.push_back(button);

            ++i;
        }
    }

    boost::shared_ptr<Draggable_button> button_reset_camera = boost::shared_ptr<Draggable_button>(
                new Draggable_button(Eigen::Vector3f(0.95f, 0.95f, 0.0f),
                                     Eigen::Vector2f(0.04f, 0.04f * _viewer.camera()->aspectRatio()),
                                     "", std::bind(&My_viewer::update_game_camera, &_viewer)));
    button_reset_camera->set_texture(_viewer.bindTexture(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/button_reset_camera.png"))));
    button_reset_camera->set_tooltip_text("Reset the camera");

    _buttons.push_back(button_reset_camera);


    update_draggable_to_level_element();
    update_active_draggables();
}

void Main_game_screen::level_element_button_pressed(const std::string &type)
{
    _mouse_state = Mouse_state::Level_element_button_selected;

    _selected_level_element_button_type = type;

    QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
}

void Main_game_screen::add_selected_level_element(QPoint const& mouse_pos)
{
    _mouse_state = Mouse_state::None;

    QApplication::restoreOverrideCursor();

    if (_core.get_level_data()._available_elements[_selected_level_element_button_type] > 0)
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

        _core.get_level_data()._available_elements[_selected_level_element_button_type] -= 1;

        add_element(placement_position, _selected_level_element_button_type, true);

        update_level_element_buttons();
    }
}

void Main_game_screen::change_renderer()
{
    _renderer = std::unique_ptr<World_renderer>(Parameter_registry<World_renderer>::get_class_from_single_select_instance_2(_parameters.get_child("Renderer")));
    _renderer->init(_viewer.context(), _viewer.size());
    _renderer->update(_core.get_level_data());
}

void Main_game_screen::resize(QSize const& size)
{
    _renderer->resize(size);

    _main_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

    GL_functions f(_viewer.context());

    _tmp_screen_texture[0].reset(f.create_texture(size.width(), size.height()));
    _tmp_screen_texture[1].reset(f.create_texture(size.width(), size.height()));
}

void Main_game_screen::handle_level_change(Main_game_screen::Level_state const level_state)
{
    std::cout << __FUNCTION__ << std::endl;

    _level_state = level_state;

    if (_level_state == Level_state::Intro)
    {
        setup_intro();
        assert(_core.get_level_data()._game_field_borders.size() == 6);
    }

//    if (_ui_state != Ui_state::Level_editor)
    if (_core.get_game_state() == Core::Game_state::Running)
    {
        update_level_element_buttons();
    }

    update_draggable_to_level_element();
    update_active_draggables();
}

void Main_game_screen::handle_game_state_change()
{
    std::cout << __FUNCTION__ << std::endl;

    // check for killing when this state is started from the editor, don't want to revive the dying editor
    if (_core.get_game_state() == Core::Game_state::Running && get_state() != State::Killing)
    {
        resume();
    }
    else if (_core.get_game_state() == Core::Game_state::Finished)
    {
        pause();

        if (_ui_state == Ui_state::Level_editor)
        {
            _viewer.add_screen(new After_finish_editor_screen(_viewer, _core));
        }
        else
        {
            _viewer.add_screen(new After_finish_screen(_viewer, _core));
        }
    }
}



void Main_game_screen::setup_intro()
{
    resume();

    _viewer.camera()->setUpVector(qglviewer::Vec(0.0f, 0.0f, 1.0f));
    _viewer.camera()->setViewDirection(qglviewer::Vec(0.0f, 1.0f, 0.0f));
    _viewer.camera()->setPosition(qglviewer::Vec(0.0f, -80.0f, 0.0f));

    Eigen::Vector3f grid_start(-20.0f, -20.0f, -20.0f);
    Eigen::Vector3f grid_end  ( 20.0f,  20.0f, 20.0f);

    Eigen::AlignedBox3f box(grid_start, grid_end);

    for (int i = 0; i < 100; ++i)
    {
        _core.add_molecule(Molecule::create_water(box.sample()));
    }

    _core.get_level_data()._rotation_fluctuation = 0.5f;
    _core.get_level_data()._translation_fluctuation = 2.0f;

    _core.get_level_data()._rotation_damping = 0.5f;
    _core.get_level_data()._translation_damping = 0.1f;

//    _parameters["Level_data/rotation_fluctuation"]->set_value(0.5f);
//    _parameters["Level_data/translation_fluctuation"]->set_value(2.0f);

//    _parameters["Level_data/rotation_damping"]->set_value(0.5f);
//    _parameters["Level_data/translation_damping"]->set_value(0.1f);

    _core.get_parameters()["Mass Factor"]->set_value(0.1f);

//    _parameters["Core/mass_factor"]->set_value(0.1f);

    _core.get_level_data()._parameters["Game Field Width"]->set_value(100.0f);
    _core.get_level_data()._parameters["Game Field Height"]->set_value(100.0f);
    _core.get_level_data()._parameters["Game Field Depth"]->set_value(100.0f);

    assert(_core.get_level_data()._game_field_borders.size() == 6);

    for (int i = 0; i < 100; ++i)
    {
        _core.update(0.01f);
    }

    qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(_viewer.camera()->frame());
    kfi->addKeyFrame(*_viewer.camera()->frame(), 0.0f);
    _viewer.camera()->setKeyFrameInterpolator(0, kfi);

    qglviewer::Frame front_view = *_viewer.camera()->frame();
    front_view.setPosition(0.0f, -50.0f, 0.0f);
    kfi->addKeyFrame(front_view, 10.0f);

    qglviewer::Frame side_view = *_viewer.camera()->frame();
    qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, -0.57735f, -0.57735f), 2.094f);
    side_view.setOrientation(orientation);
    side_view.setPosition(0.0f, -40.0f, 0.0f);
    kfi->addKeyFrame(side_view, 16.0f);

    kfi->startInterpolation();

    connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));

    _intro_time = 0.0f;
    _intro_state = Intro_state::Beginning;

    _core.set_simulation_state(true);
}

void Main_game_screen::update_intro(const float timestep)
{
    _intro_time += timestep;

    if (_intro_state == Intro_state::Beginning)
    {
        if (_intro_time > 10.0f)
        {
//            _parameters["Level_data/rotation_fluctuation"]->set_value(0.0f);
//            _parameters["Level_data/translation_fluctuation"]->set_value(0.0f);

            _core.get_level_data()._rotation_fluctuation = 0.0f;
            _core.get_level_data()._translation_fluctuation = 0.0f;

//            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
//            kfi->addKeyFrame(*camera()->frame());
//            camera()->setKeyFrameInterpolator(0, kfi);

//            qglviewer::Frame side_view = *camera()->frame();
//            qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, -0.57735f, -0.57735f), 2.094f);
//            side_view.setOrientation(orientation);
//            side_view.setPosition(0.0f, -40.0f, 0.0f);

//            kfi->addKeyFrame(side_view);
//            kfi->setInterpolationTime(5.0f);
//            kfi->startInterpolation();
//            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));

            Molecule m = Molecule::create_water(Eigen::Vector3f(20.0f, -40.0f, 0.0f));
            Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
            t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));
            t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
            m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));

            _core.add_molecule(m);

            _intro_state = Intro_state::Single_molecule;
            _intro_time = 0.0f;
        }
    }
    else if (_intro_state == Intro_state::Single_molecule)
    {
        if (_intro_time > 8.0f)
        {
            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(_viewer.camera()->frame());
            kfi->addKeyFrame(*_viewer.camera()->frame());
            _viewer.camera()->setKeyFrameInterpolator(0, kfi);
            qglviewer::Frame translated_view = *_viewer.camera()->frame();
            translated_view.translate(qglviewer::Vec(-5.0f, 5.0f, 0.0f));
            kfi->addKeyFrame(translated_view);
            kfi->setInterpolationTime(4.0f);
            kfi->startInterpolation();

            _core.add_molecule(Molecule::create_water(Eigen::Vector3f(20.0f, -30.0f, 0.0f)));
            Molecule & m = _core.get_molecules().back();
            Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
            t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));
            t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
            m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));

            for (Atom & a : m._atoms)
            {
                std::string sign = "-";

                if (a._charge > 0.0f)
                {
                    sign = "+";
                }

                Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, _viewer.camera());
                _ui_renderer.generate_label_texture(label);
                _labels.push_back(boost::shared_ptr<Draggable_label>(label));
            }

//            set_simulation_state(false);

            _intro_state = Intro_state::Two_molecules_0;
            _intro_time = 0.0f;
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_0)
    {
        if ((_core.get_molecules().begin()->_x - (++_core.get_molecules().begin())->_x).norm() < 4.0f)
        {
            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_1;
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_1)
    {
        if (_intro_time > 5.0f)
        {
            _labels.clear();
            _core.get_molecules().clear();

//            for (boost::shared_ptr<Draggable_label> & label : _labels[int(Level_state::Intro)])
//            {
//                label->set_alpha(0.0f);
//            }

            for (int i = 0; i < 2; ++i)
            {
                _core.add_molecule(Molecule::create_water(Eigen::Vector3f(-20.0f, -32.5f - i * 5.0f, 0.0f)));
                Molecule & m = _core.get_molecules().back();
                Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
                t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));

                if (i == 0)
                {
                    t.rotate(Eigen::AngleAxisf(float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
                }
                else
                {
                    t.rotate(Eigen::AngleAxisf(-float(M_PI) * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
                }

                m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));
            }

            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(_viewer.camera()->frame());
            kfi->addKeyFrame(*_viewer.camera()->frame());
            _viewer.camera()->setKeyFrameInterpolator(0, kfi);

            qglviewer::Frame side_view = *_viewer.camera()->frame();
            qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, 0.57735f, 0.57735f), 2.094f);
            side_view.setOrientation(orientation);
            side_view.setPosition(qglviewer::Vec(5.0f, -35.0f, 0.0f));

            kfi->addKeyFrame(side_view, 8.0f);

            kfi->startInterpolation();
//            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));

            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_2;

            // damp a lot to force standstill, but don't disable the simulation to keep getting notification for the labels' position updates
//            _parameters["Level_data/rotation_damping"]->set_value(100.0f);
//            _parameters["Level_data/translation_damping"]->set_value(100.0f);

            _core.get_level_data()._rotation_damping = 100.0f;
            _core.get_level_data()._translation_damping = 100.0f;

            _core.set_simulation_state(false);
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_2)
    {
        if (_intro_time > 8.0f)
        {
            // remove excessive damping
//            _parameters["Level_data/rotation_damping"]->set_value(0.5f);
//            _parameters["Level_data/translation_damping"]->set_value(0.1f);

            _core.get_level_data()._rotation_damping = 0.5f;
            _core.get_level_data()._translation_damping = 0.1f;

            _core.set_simulation_state(true);

            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_3;

            for (Molecule & m : _core.get_molecules())
            {
                for (Atom & a : m._atoms)
                {
                    std::string sign = "-";

                    if (a._charge > 0.0f)
                    {
                        sign = "+";
                    }

                    Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, _viewer.camera());
                    _ui_renderer.generate_label_texture(label);

                    _labels.push_back(boost::shared_ptr<Draggable_label>(label));
                }
            }
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_3)
    {
        if (_intro_time > 10.0f)
        {
            for (boost::shared_ptr<Draggable_label> & label : _labels)
            {
                label->set_alpha(0.0f);
            }

            _intro_time = 0.0f;
            _intro_state = Intro_state::Finishing;
        }
    }
    else if (_intro_state == Intro_state::Finishing)
    {
        if (_intro_time > 4.0f)
        {
            float const z = 0.5f * (_core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
                    + _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2]);

            qglviewer::Vec level_start_position(0.0f, -80.0f, z);

            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(_viewer.camera()->frame());
            kfi->addKeyFrame(*_viewer.camera()->frame());
            _viewer.camera()->setKeyFrameInterpolator(0, kfi);

            qglviewer::Quaternion orientation(qglviewer::Vec(1.0f, 0.0f, 0.0f), M_PI * 0.5f);
            qglviewer::Frame front_view = *_viewer.camera()->frame();
            front_view.setOrientation(orientation);
            front_view.setPosition(level_start_position);

            kfi->addKeyFrame(front_view);
            kfi->setInterpolationTime(2.0f);
            kfi->startInterpolation();

            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam2_end_reached()));

            _intro_state = Intro_state::Finished;
        }
    }
}

void Main_game_screen::intro_cam1_end_reached()
{
    _core.get_molecules().erase(_core.get_molecules().begin(), --_core.get_molecules().end());

    Molecule & m = _core.get_molecules().front();

    for (Atom & a : m._atoms)
    {
        std::string sign = "-";

        if (a._charge > 0.0f)
        {
            sign = "+";
        }

        Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, _viewer.camera());
        _ui_renderer.generate_label_texture(label);
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
    }
}

void Main_game_screen::intro_cam2_end_reached()
{
    pause();

    _core.load_next_level();

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    _level_state = Level_state::Running;
}
