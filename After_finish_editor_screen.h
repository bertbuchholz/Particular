#ifndef AFTER_FINISH_EDITOR_SCREEN_H
#define AFTER_FINISH_EDITOR_SCREEN_H

#include "Menu_screen.h"

class After_finish_editor_screen : public Menu_screen
{
public:
    After_finish_editor_screen(My_viewer & viewer, Core & core);

    void init();

    void draw() override;

    void play_again();
    void change_to_statistics();
    void return_to_editor();

    void update_event(const float time_step) override;

private:
    boost::shared_ptr<Draggable_button> _next_level_button;

    Targeted_particle_system _score_particle_system;
};


#endif // AFTER_FINISH_EDITOR_SCREEN_H
