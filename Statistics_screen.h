#ifndef STATISTICS_SCREEN_H
#define STATISTICS_SCREEN_H

#include "Menu_screen.h"

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
    std::vector<Draggable_statistics> _statistics;

    Screen * _calling_screen;
};

#endif // STATISTICS_SCREEN_H
