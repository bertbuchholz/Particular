#ifndef BEFORE_START_SCREEN_H
#define BEFORE_START_SCREEN_H

#include "Menu_screen.h"

class Before_start_screen : public Menu_screen
{
public:
    Before_start_screen(My_viewer & viewer, Core & core);

    void draw() override;

    void init();

    void return_to_main_menu();
    void start_level();

    void update_event(const float time_step) override;

private:
    Targeted_particle_system _particle_system;
};


#endif // BEFORE_START_SCREEN_H
