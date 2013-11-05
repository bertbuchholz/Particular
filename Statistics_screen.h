#ifndef STATISTICS_SCREEN_H
#define STATISTICS_SCREEN_H

#include "Screen.h"

#include "Draggable.h"
#include "Picking.h"
#include "Core.h"

class Statistics_screen : public Screen
{
public:
    Statistics_screen(My_viewer & viewer, Core & core, Screen * calling_screen);

    void init();

    bool mousePressEvent(QMouseEvent *event) override;

    void draw() override;
    void draw_draggables_for_picking();

    void update_event(const float time_step) override;

    void setup_statistics(Sensor_data const& sensor_data);

    void exit();
    void repeat();

private:
    Core & _core;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;
//    std::vector< boost::shared_ptr<Draggable_label> > _labels;
    std::vector<Draggable_statistics> _statistics;

    boost::shared_ptr<Draggable_button> _next_level_button;

    Picking _picking;

    Screen * _calling_screen;
};

#endif // STATISTICS_SCREEN_H
