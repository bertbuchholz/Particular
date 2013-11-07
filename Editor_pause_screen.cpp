#include "Editor_pause_screen.h"

#include "Editor_screen.h"

Editor_pause_screen::Editor_pause_screen(My_viewer &viewer, Core &core, Screen *calling_state) : Screen(viewer), _core(core), _calling_screen(calling_state)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

bool Editor_pause_screen::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int picked_index = _picking.do_pick(
                    event->pos().x() / float(_viewer.camera()->screenWidth()),
                    (_viewer.camera()->screenHeight() - event->pos().y()) / float(_viewer.camera()->screenHeight()),
                    std::bind(&Editor_pause_screen::draw_draggables_for_picking, this));

        if (picked_index > -1)
        {
            _buttons[picked_index]->clicked();
        }
    }

    return true;
}

bool Editor_pause_screen::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;

    std::cout << __PRETTY_FUNCTION__ << " " << event->key() << " state: " << int(get_state()) << std::endl;

    if (event->key() == Qt::Key_Escape)
    {
        if (get_state() == State::Running || get_state() == State::Resuming)
        {
            kill();

            _calling_screen->resume();

            handled = true;
        }
        else if (get_state() == State::Killing)
        {
            resume();

            _calling_screen->pause();

            handled = true;
        }
    }

    return handled;
}

void Editor_pause_screen::draw()
{
    //        std::cout << "Pause screen" << std::endl;

    float alpha = 1.0f;

    if (get_state() == State::Killing || get_state() == State::Resuming)
    {
        if (get_state() == State::Killing)
        {
            alpha = 1.0f - _transition_progress;
        }
        else if (get_state() == State::Resuming)
        {
            alpha = _transition_progress;
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    _viewer.start_normalized_screen_coordinates();

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        //            if (button->is_visible())
        {
            _viewer.draw_button(button.get(), false, alpha);
        }
    }

    _viewer.stop_normalized_screen_coordinates();
}

void Editor_pause_screen::draw_draggables_for_picking()
{
    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        //            if (button->is_visible())
        {
            _picking.set_index(i);
            _viewer.draw_button(_buttons[i].get(), true);
        }
    }

    _viewer.stop_normalized_screen_coordinates();
}

void Editor_pause_screen::init()
{
    // Pause menu
    if (_core.get_game_state() == Core::Game_state::Running)
    {
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.85f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Return to Editor",  std::bind(&Editor_pause_screen::return_to_editor, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }
    else
    {
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Play Level",  std::bind(&Editor_pause_screen::play_level, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.68f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Load Level",  std::bind(&Editor_pause_screen::load_level, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.56f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Save Level",  std::bind(&Editor_pause_screen::save_level, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.44, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Back to Main Menu",  std::bind(&Editor_pause_screen::return_to_main_menu, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.32f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Continue", std::bind(&Editor_pause_screen::continue_game, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _viewer.generate_button_texture(button.get());
    }
}

void Editor_pause_screen::continue_game()
{
    kill();

    _calling_screen->resume();
}

void Editor_pause_screen::return_to_main_menu()
{
    QMessageBox::StandardButton button = QMessageBox::warning(&_viewer, "Return to Main Menu", "Unsaved changes will be lost. Really return to the main menu?", QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes)
    {
        _viewer.add_screen(new Main_game_screen(_viewer, _core, Main_game_screen::Ui_state::Playing));
        _viewer.add_screen(new Main_menu_screen(_viewer, _core));

        _calling_screen->kill();

        kill();
    }
}

void Editor_pause_screen::play_level()
{
    _viewer.replace_screens(new Main_game_screen(_viewer, _core, Main_game_screen::Ui_state::Level_editor));

    _core.start_level();

//    _viewer.replace_screens(new Editor_screen(_viewer, _core));
}

void Editor_pause_screen::return_to_editor()
{
    _core.reset_level();

    _core.set_new_game_state(Core::Game_state::Unstarted);

    _viewer.camera()->frame()->setConstraint(nullptr);

    _core.set_simulation_state(false);

    _viewer.replace_screens(new Editor_screen(_viewer, _core));
}


void Editor_pause_screen::load_level()
{
    QString filename;

    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        filename = "state.data";
    }
    else
    {
        filename = QFileDialog::getOpenFileName(&_viewer, tr("Load Level"),
                                                ".",
                                                tr("State File (*.data)"));
    }

    if (!filename.isEmpty())
    {
        _core.load_level(filename.toStdString());
    }
}


void Editor_pause_screen::save_level()
{
    QString filename;

    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        filename = "state.data";
    }
    else
    {
        filename = QFileDialog::getSaveFileName(&_viewer, tr("Save Level"),
                                                ".",
                                                tr("State File (*.data)"));
    }

    if (!filename.isEmpty())
    {
        _core.save_level(filename.toStdString());
    }
}
