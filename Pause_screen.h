#ifndef PAUSE_SCREEN_H
#define PAUSE_SCREEN_H

#include "Screen.h"

#include <Picking.h>

#include "Renderer.h"
#include "Core.h"
#include "My_viewer.h"
#include "Main_menu_screen.h"

#include "Draggable.h"


class Pause_screen : public Screen
{
public:
    Pause_screen(My_viewer & viewer, Core & core, Screen * calling_state) : Screen(viewer), _core(core), _calling_screen(calling_state)
    {
        _type = Screen::Type::Modal;

        init();

        _picking.init(_viewer.context());
    }

    bool mousePressEvent(QMouseEvent * event) override
    {
        if (event->buttons() & Qt::LeftButton)
        {
            int picked_index = _picking.do_pick(
                        event->pos().x() / float(_viewer.camera()->screenWidth()),
                        (_viewer.camera()->screenHeight() - event->pos().y()) / float(_viewer.camera()->screenHeight()),
                        std::bind(&Pause_screen::draw_draggables_for_picking, this));

            if (picked_index > -1)
            {
                _buttons[picked_index]->clicked();
            }
        }

        return true;
    }

    bool keyPressEvent(QKeyEvent * event) override
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

    void draw() override
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

    void draw_draggables_for_picking()
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

    void init()
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
            _viewer.generate_button_texture(button.get());
        }
    }

    void continue_game()
    {
        kill();

        _calling_screen->resume();
    }

    void return_to_main_menu()
    {
        kill();

        _viewer.add_screen(new Main_menu_screen(_viewer, _core));
    }

    void restart_level()
    {
        kill();

        _core.reset_level();

        _calling_screen->resume();
    }

private:
    Core & _core;

    Screen * _calling_screen;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;

    Picking _picking;
};

#endif // PAUSE_SCREEN_H
