#include "Pause_screen.h"

#include "Core.h"
#include "My_viewer.h"
#include "Main_menu_screen.h"

Pause_screen::Pause_screen(My_viewer &viewer, Core &core, Screen *calling_state) : Menu_screen(viewer, core), _calling_screen(calling_state)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}


bool Pause_screen::keyPressEvent(QKeyEvent *event)
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


void Pause_screen::init()
{
    // Pause menu
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.85f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Restart Level",  std::bind(&Pause_screen::restart_level, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.65f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Back to Main Menu",  std::bind(&Pause_screen::return_to_main_menu, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.45f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Continue", std::bind(&Pause_screen::continue_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }
}


void Pause_screen::continue_game()
{
    kill();

    _calling_screen->resume();
}


void Pause_screen::return_to_main_menu()
{
    kill();

    _viewer.add_screen(new Main_menu_screen(_viewer, _core));
}


void Pause_screen::restart_level()
{
    kill();

    _core.reset_level();

    _calling_screen->resume();
}
