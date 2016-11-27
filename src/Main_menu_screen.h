#ifndef MAIN_MENU_SCREEN_H
#define MAIN_MENU_SCREEN_H

#include "Menu_screen.h"

class Main_menu_screen : public Menu_screen
{
public:
    Main_menu_screen(My_viewer & viewer, Core & core);

    void draw() override;

    void init();

    void start_new_game();
    void continue_game();
    void quit_game();
    void start_editor();
    void pick_level();

    void update_event(const float time_step) override;

private:
    Targeted_particle_system _game_name_system;
};


#endif // MAIN_MENU_SCREEN_H
