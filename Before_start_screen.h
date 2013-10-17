#ifndef BEFORE_START_SCREEN_H
#define BEFORE_START_SCREEN_H

#include "Screen.h"

#include <Picking.h>

#include "Renderer.h"
#include "Core.h"
#include "My_viewer.h"
#include "Main_menu_screen.h"

#include "Draggable.h"


class Before_start_screen : public Screen
{
public:
    Before_start_screen(My_viewer & viewer, Core & core) : Screen(viewer), _core(core)
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
                        std::bind(&Before_start_screen::draw_draggables_for_picking, this));

            if (picked_index > -1)
            {
                _buttons[picked_index]->clicked();
            }
        }

        return true;
    }

    bool mouseMoveEvent(QMouseEvent *) override { return false; }

    bool mouseReleaseEvent(QMouseEvent * ) override { return false; }

    void draw() override
    {
//        std::cout << "Pause screen" << std::endl;

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        _viewer.start_normalized_screen_coordinates();

        for (boost::shared_ptr<Draggable_button> const& button : _buttons)
        {
//            if (button->is_visible())
            {
                _viewer.draw_button(button.get(), false);
            }
        }

        _viewer.stop_normalized_screen_coordinates();

        draw_particle_system(_particle_system, _viewer.height());
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

    void state_changed_event(const Screen::State new_state, const Screen::State previous_state)
    {
        std::cout << __PRETTY_FUNCTION__ << " " << int(new_state) << " " << int(previous_state) << std::endl;
    }

    void init()
    {
        // start level screen
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.35f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Start Level",  std::bind(&Before_start_screen::start_level, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.15f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Back to Main Menu",  std::bind(&Before_start_screen::return_to_main_menu, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        for (boost::shared_ptr<Draggable_button> const& button : _buttons)
        {
            _viewer.generate_button_texture(button.get());
        }

        _particle_system = Targeted_particle_system(3.0f);
        _particle_system.generate(QString("Level %1").arg(_core.get_progress().last_level + 1).toStdString(), _viewer.get_particle_font(), QRectF(0.0f, 0.1f, 1.0f, 0.3f));
    }

    void return_to_main_menu()
    {
        _state = State::Killing;

        _viewer.add_screen(new Main_menu_screen(_viewer, _core));
    }

    void start_level()
    {
//        assert(_level_state == Level_state::Before_start);

        _core.start_level();
        _viewer.set_simulation_state(true);

        _state = State::Killing;
    }

    void update_event(const float time_step) override
    {
        _particle_system.animate(time_step);
    }

private:
    Core & _core;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;

    Picking _picking;

    Targeted_particle_system _particle_system;
};


#endif // BEFORE_START_SCREEN_H
