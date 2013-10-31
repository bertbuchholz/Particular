#ifndef AFTER_FINISH_SCREEN_H
#define AFTER_FINISH_SCREEN_H

#include "Screen.h"

#include "Draggable.h"
#include "Picking.h"
#include "Core.h"

class After_finish_screen : public Screen
{
public:
    After_finish_screen(My_viewer & viewer, Core & core);

    void init();

    void draw() override;

    bool mousePressEvent(QMouseEvent * event) override;

    void draw_draggables_for_picking();

    void load_next_level();
    void change_to_statistics();
    void change_to_main_menu();

    void update_event(const float time_step) override;

private:
    Core & _core;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;
    std::vector< boost::shared_ptr<Draggable_label> > _labels;

    boost::shared_ptr<Draggable_button> _next_level_button;

    Picking _picking;

    Targeted_particle_system _score_particle_system;
};

#endif // AFTER_FINISH_SCREEN_H
