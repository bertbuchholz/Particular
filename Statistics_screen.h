#ifndef STATISTICS_SCREEN_H
#define STATISTICS_SCREEN_H

#include "Menu_screen.h"

class Draggable_event
{
public:
    enum class Type { Fade_in, Fade_out, Move, Scale, Animate };

    Draggable_event(boost::shared_ptr<Draggable_label> const& draggable, Type const type) : _draggable(draggable), _type(type), _duration(1.0f), _current_time(0.0f), _active(false)
    { }

    void trigger()
    {
        _active = true;
    }

    void update(float const timestep)
    {
        if (_type == Type::Animate)
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

        if (_type == Type::Fade_in)
        {
            float const alpha = _current_time / _duration;

            _draggable->set_alpha(alpha);
        }
        else if (_type == Type::Fade_out)
        {
            float const alpha = _current_time / _duration;

            _draggable->set_alpha(1.0f - alpha);
        }
    }

    void set_duration(float const duration)
    {
        _duration = duration;
    }

    void set_finish_function(std::function<void(void)> const& finish_callback)
    {
        _finish_callback = finish_callback;
    }

private:
    boost::shared_ptr<Draggable_label> _draggable;
    Type _type;
    float _duration;
    float _current_time;
    Eigen::Vector3f _position;
    Eigen::Vector2f _extent;

    bool _active;

    std::function<void(void)> _finish_callback;
};

class Statistics_screen : public Menu_screen
{
public:
    Statistics_screen(My_viewer & viewer, Core & core, Screen * calling_screen);

    void init();

    void draw() override;

    void update_event(const float time_step) override;

    void setup_statistics(Sensor_data const& sensor_data);

    void exit();
    void repeat();

private:
    std::vector< boost::shared_ptr<Draggable_statistics> > _statistics;

    boost::shared_ptr<Draggable_label> _collected_score_label;
    boost::shared_ptr<Draggable_label> _power_score_label;

    Screen * _calling_screen;

    std::vector< boost::shared_ptr<Draggable_event> > _events;
//    QTimer _;
};

#endif // STATISTICS_SCREEN_H
