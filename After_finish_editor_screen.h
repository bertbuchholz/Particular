#ifndef AFTER_FINISH_EDITOR_SCREEN_H
#define AFTER_FINISH_EDITOR_SCREEN_H

#include "Menu_screen.h"

#include "Draggable_event.h"

class After_finish_editor_screen : public Menu_screen
{
public:
    After_finish_editor_screen(My_viewer & viewer, Core & core);

    void init();

    void draw() override;

    void play_again();
    void change_to_statistics();
    void return_to_editor();

    void start_score_animation();
    void add_particle_system();

    void update_event(const float time_step) override;

private:
    Targeted_particle_system _score_particle_system;

    boost::shared_ptr<Draggable_label> _score_label;
    boost::shared_ptr<Draggable_label> _penalty_label;
    float _animate_score_time;

    std::vector< boost::shared_ptr<Draggable_event> > _events;

    Score _score;
};


#endif // AFTER_FINISH_EDITOR_SCREEN_H
