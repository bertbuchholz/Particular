#ifndef FPS_H
#define FPS_H

#include <chrono>

class Fps_meter
{
public:
    Fps_meter() :
        _fps(0.0f),
        _frame_counter(0)
    {
        start();
    }

    void start()
    {
        _last_check = std::chrono::steady_clock::now();
    }

    float get_fps()
    {
        _has_new_value = false;

        return _fps;
    }

    bool has_new_value()
    {
        return _has_new_value;
    }

    void update()
    {
        ++_frame_counter;

        std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();

        int const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (t-_last_check).count();

        if (elapsed_milliseconds > 1000)
        {
            _fps = _frame_counter / float(elapsed_milliseconds) * 1000.0f;

            _frame_counter = 0;
            _last_check = t;

            _has_new_value = true;
        }
    }

private:
    float _fps;
    int _frame_counter;
    std::chrono::steady_clock::time_point _last_check;
    bool _has_new_value;
};

#endif // FPS_H
