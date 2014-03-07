#include "Draggable_event.h"


void Draggable_event::make_move_event(const Eigen::Vector3f &start, const Eigen::Vector3f &target)
{
    _start_position = start;
    _target_position = target;

    _type |= Type::Move;
}

void Draggable_event::update(const float time, const float timestep)
{
    if (time < _start_time) return;

    if (_type.testFlag(Type::Animate))
    {
        _draggable->animate(timestep);
    }

    if (!_active) return;

    _current_time = std::min(_duration + 0.0001f, _current_time + timestep);

    if (_current_time > _duration)
    {
        _active = false;

        if (_finish_callback)
        {
            _finish_callback();
        }
    }

    float const alpha = _current_time / _duration;

    float const progress = _easing_curve.valueForProgress(alpha);

    if (_type.testFlag(Type::Fade_in))
    {
        _draggable->set_alpha(progress);
    }
    else if (_type.testFlag(Type::Fade_out))
    {
        _draggable->set_alpha(1.0f - progress);
    }

    if (_type.testFlag(Type::Move))
    {
        Eigen::Vector3f pos = _start_position * (1.0f - progress) + _target_position * progress;

        _draggable->set_position(pos);
    }
}

void Draggable_event::reset()
{
    _current_time = 0.0f;
    _active = true;
}
