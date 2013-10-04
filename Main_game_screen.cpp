#include "Main_game_state.h"

#include "My_viewer.h"
#include "Pause_screen.h"

bool Main_game_screen::keyPressEvent(QKeyEvent * event)
{
    bool handled = false;

    std::cout << __PRETTY_FUNCTION__ << " " << event->key() << std::endl;

    if (event->key() == Qt::Key_Escape)
    {
        if (_state == State::Running && _viewer.get_level_state() != My_viewer::Level_state::Intro)
        {
            // go into pause and start pause menu
            _state = State::Pausing;

            _viewer.add_screen(new Pause_screen(_viewer, _core, this));

            _viewer.set_simulation_state(false);

            handled = true;
        }
        else if ((_state == State::Paused || _state == State::Pausing) && _viewer.get_level_state() != My_viewer::Level_state::Intro)
        {
            // go into pause and start pause menu
            _state = State::Resuming;

//            _viewer.set_simulation_state(true);

            handled = true;
        }
        else if (_viewer.get_level_state() == My_viewer::Level_state::Intro)
        {
            _viewer.camera()->deletePath(0);

            _viewer.load_next_level();
        }

//        if (_level_state == Level_state::Intro) // skip intro
//        {
//            camera()->deletePath(0);

//            load_next_level();
//        }
//        else if (_level_state == Level_state::Running) // go to pause menu
//        {
//            enter_pause_menu();

//        }
    }

    return handled;
}

void Main_game_screen::draw()
{
    setup_gl_points(true);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    _renderer->render(_core.get_level_data(), _core.get_current_time(), _viewer.camera());
}

void Main_game_screen::state_changed_event(const Screen::State current_state, const Screen::State previous_state)
{
    std::cout << __PRETTY_FUNCTION__ << " " << int(current_state) << " " << int(previous_state) << std::endl;

    if (current_state == State::Running)
    {
        _viewer.set_simulation_state(true);
    }
}



void Main_game_screen::update_event(const float time_step)
{
    if (_state == State::Running)
    {
        // normal update
    }
}
