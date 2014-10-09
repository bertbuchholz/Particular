#ifndef EXPERIMENT_SCREEN_H
#define EXPERIMENT_SCREEN_H

#include "Menu_screen.h"

class Experiment_screen : public Menu_screen
{
public:
    Experiment_screen(My_viewer & viewer, Core & core);

    void draw() override;

    void init();

    void update_event(const float time_step) override;

private:
    Curved_particle_system _curved_system;

    Draggable_statistics _statistic;

    Targeted_particle_system _game_name_system;
};

#endif // EXPERIMENT_SCREEN_H
