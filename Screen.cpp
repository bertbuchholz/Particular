#include "Screen.h"


void Screen::pause()
{
    if (_state == State::Resuming)
    {
        _transition_progress = 1.0f - _transition_progress;
    }

    _state = State::Pausing;
}


void Screen::resume()
{
    if (_state == State::Pausing || _state == State::Killing)
    {
        _transition_progress = 1.0f - _transition_progress;
    }

    if (_state == State::Paused || _state == State::Pausing || _state == State::Killing)
    {
        _state = State::Resuming;
    }
}


void Screen::kill()
{
    _state = State::Killing;
}


void Screen::update(const float time_step)
{
    if (_state != State::Running && _state != State::Paused && _state != State::Faded_out)
    {
        _transition_progress += 1.0f * time_step;
    }

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
