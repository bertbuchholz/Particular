#include "Editor_pause_screen.h"

#include "Editor_screen.h"
#include "My_viewer.h"
#include "Main_menu_screen.h"

Editor_pause_screen::Editor_pause_screen(My_viewer &viewer, Core &core, Screen *calling_state) : Menu_screen(viewer, core), _calling_screen(calling_state)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}


bool Editor_pause_screen::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;

    std::cout << __FUNCTION__ << " " << event->key() << " state: " << int(get_state()) << std::endl;

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
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.44f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Back to Main Menu",  std::bind(&Editor_pause_screen::return_to_main_menu, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.32f, 0.0f), Eigen::Vector2f(0.5f, 0.1f), "Continue", std::bind(&Editor_pause_screen::continue_game, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
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
        _core.save_simulation_settings();

        Screen * s = new Main_game_screen(_viewer, _core, Main_game_screen::Ui_state::Playing);
        s->pause();
        _viewer.replace_screens(s);
        _viewer.add_screen(new Main_menu_screen(_viewer, _core));
    }
}

void Editor_pause_screen::play_level()
{
    if (_core.get_level_data()._portals.size() == 0)
    {
        QMessageBox::critical(&_viewer, "Portal Missing", "At least one portal is needed to have a playable level.");
    }
    else
    {
        _viewer.replace_screens(new Main_game_screen(_viewer, _core, Main_game_screen::Ui_state::Editor_playing));

        _core.start_level();
    }
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
