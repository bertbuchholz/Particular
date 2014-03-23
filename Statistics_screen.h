#ifndef STATISTICS_SCREEN_H
#define STATISTICS_SCREEN_H

#include "Menu_screen.h"

#include "Draggable_event.h"

class Statistics_screen : public Menu_screen
{
public:
    Statistics_screen(My_viewer & viewer, Core & core, Screen * calling_screen, Score const& score);

    void init();

    void draw() override;

    void update_event(const float time_step) override;

    void setup_statistics(Sensor_data const& sensor_data, Score const& score);

    void exit();
    void repeat();

    void update_score_label();

private:
    std::vector< boost::shared_ptr<Draggable_statistics> > _statistics;

    boost::shared_ptr<Draggable_label> _collected_score_label;
    boost::shared_ptr<Draggable_label> _penalty_label;

    Screen * _calling_screen;

    std::vector< boost::shared_ptr<Draggable_event> > _draggable_events;

    float _stat_anim_duration;

    Score const& _score;
//    QTimer _;
};

#endif // STATISTICS_SCREEN_H
