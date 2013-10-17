#include "Screen.h"


void Screen::pause()
{
    _state = State::Pausing;
}


void Screen::resume()
{
    if (_state == State::Paused || _state == State::Resuming)
    {
        _state = State::Resuming;
    }
}


void Screen::update(const float time_step)
{
    _transition_progress += 1.0f * time_step;

    if (_state == State::Resuming)
    {
        if (_transition_progress >= 1.0f)
        {
            _transition_progress = 0.0f;
            _state = State::Running;

            state_changed_event(State::Running, State::Resuming);
        }
    }
    else if (_state == State::Pausing)
    {
        if (_transition_progress >= 1.0f)
        {
            _transition_progress = 0.0f;
            _state = State::Paused;

            state_changed_event(State::Paused, State::Pausing);
        }
    }
    else if (_state == State::Fading_in)
    {
        if (_transition_progress >= 1.0f)
        {
            _transition_progress = 0.0f;
            _state = State::Running;

            state_changed_event(State::Running, State::Fading_in);
        }
    }
    else if (_state == State::Fading_out)
    {
        if (_transition_progress >= 1.0f)
        {
            _transition_progress = 0.0f;
            _state = State::Faded_out;

            state_changed_event(State::Faded_out, State::Fading_out);
        }
    }
    else if (_state == State::Killing)
    {
        if (_transition_progress >= 1.0f)
        {
            _transition_progress = 0.0f;
            _state = State::Killed;

            state_changed_event(State::Killed, State::Killing);
        }
    }

    update_event(time_step);
}
