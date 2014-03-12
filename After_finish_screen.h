#ifndef AFTER_FINISH_SCREEN_H
#define AFTER_FINISH_SCREEN_H

#include "Menu_screen.h"

#include "Draggable_event.h"

class After_finish_screen : public Menu_screen
{
public:
    After_finish_screen(My_viewer & viewer, Core & core);

    void init();

    void draw() override;

    void load_next_level();
    void change_to_statistics();
    void change_to_main_menu();

    void start_score_animation();
    void add_particle_system();

    void update_event(const float time_step) override;

private:
    boost::shared_ptr<Draggable_button> _next_level_button;

    Targeted_particle_system _score_particle_system;

    boost::shared_ptr<Draggable_label> _score_label;
    boost::shared_ptr<Draggable_label> _penalty_label;
    float _animate_score_time;

    std::vector< boost::shared_ptr<Draggable_event> > _events;
};

#endif // AFTER_FINISH_SCREEN_H
