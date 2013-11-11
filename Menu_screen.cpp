#include "Menu_screen.h"

#include "My_viewer.h"

Menu_screen::Menu_screen(My_viewer & viewer, Core & core) : Screen(viewer), _core(core), _renderer(_viewer.get_renderer())
{
//    _renderer.init(_viewer.context(), _viewer.size());

    _picking.init(_viewer.context());
}

void Menu_screen::draw()
{
    if (get_state() == State::Paused) return;

    float alpha = 1.0f;

    if (get_state() == State::Killing || get_state() == State::Resuming || get_state() == State::Pausing)
    {
        if (get_state() == State::Killing || get_state() == State::Pausing)
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
        _viewer.draw_button(button.get(), false, alpha);
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _viewer.draw_label(label.get(), alpha);
    }

    _viewer.stop_normalized_screen_coordinates();
}

bool Menu_screen::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int picked_index = _picking.do_pick(
                    event->pos().x() / float(_viewer.camera()->screenWidth()),
                    (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                    std::bind(&Main_menu_screen::draw_draggables_for_picking, this));

        if (picked_index > -1)
        {
            _buttons[picked_index]->clicked();
        }
    }

    return true;
}

void Menu_screen::draw_draggables_for_picking()
{
    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        if (_buttons[i]->is_visible())
        {
            _picking.set_index(i);
            _viewer.draw_button(_buttons[i].get(), true);
        }
    }

    _viewer.stop_normalized_screen_coordinates();
}
