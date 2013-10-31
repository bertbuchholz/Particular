#ifndef BEFORE_START_SCREEN_H
#define BEFORE_START_SCREEN_H

#include "Screen.h"

#include <Picking.h>

#include "Core.h"

#include "Draggable.h"


class Before_start_screen : public Screen
{
public:
    Before_start_screen(My_viewer & viewer, Core & core);

    bool mousePressEvent(QMouseEvent * event) override;

    void draw() override;
    void draw_draggables_for_picking();

    void init();

    void return_to_main_menu();
    void start_level();

    void update_event(const float time_step) override;

private:
    Core & _core;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;

    Picking _picking;

    Targeted_particle_system _particle_system;
};


#endif // BEFORE_START_SCREEN_H
