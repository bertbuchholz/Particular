#ifndef DRAGGABLE_EVENT_H
#define DRAGGABLE_EVENT_H

#include <functional>
#include <QEasingCurve>
#include <QFlags>

#include "Draggable.h"


class Draggable_event
{
public:
    enum Type { Fade_in = 0x01,
                      Fade_out = 0x02,
                      Move = 0x04,
                      Scale = 0x08,
                      Animate = 0x10 };

    Q_DECLARE_FLAGS(Types, Type)

    Draggable_event(boost::shared_ptr<Draggable_label> const& draggable, int const type_flags, float const start_time = 0.0f, QEasingCurve::Type easing_curve_type = QEasingCurve::InOutQuad) :
        _draggable(draggable), _type(type_flags), _duration(1.0f), _current_time(0.0f), _active(false), _start_time(start_time)
    {
        _easing_curve = QEasingCurve(easing_curve_type);
    }

    void make_move_event(Eigen::Vector3f const& start, Eigen::Vector3f const& target);

    void trigger()
    {
        _active = true;
    }

    void update(const float time, float const timestep);

    void set_duration(float const duration)
    {
        _duration = duration;
    }

    void set_finish_function(std::function<void(void)> const& finish_callback)
    {
        _finish_callback = finish_callback;
    }

    void reset();

private:
    boost::shared_ptr<Draggable_label> _draggable;
    Types _type;
    float _duration;
    float _current_time;
    Eigen::Vector3f _start_position;
    Eigen::Vector3f _target_position;
    Eigen::Vector2f _extent;

    bool _active;
    float _start_time;

    std::function<void(void)> _finish_callback;

    QEasingCurve _easing_curve;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Draggable_event::Types)

#endif // DRAGGABLE_EVENT_H
