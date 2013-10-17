#ifndef MAIN_MENU_SCREEN_H
#define MAIN_MENU_SCREEN_H

#include "Screen.h"

#include <Picking.h>

//#include "Renderer.h"
#include "Core.h"
//#include "My_viewer.h"

#include "Draggable.h"

class My_viewer;

class Main_menu_screen : public Screen
{
public:
    Main_menu_screen(My_viewer & viewer, Core & core /*, Screen * calling_state */);

    bool mousePressEvent(QMouseEvent * event) override;

    void draw() override;

    void draw_draggables_for_picking();

    void state_changed_event(const Screen::State new_state, const Screen::State previous_state);

    void init();

    void start_new_game();
    void continue_game();
    void quit_game();

    void update_event(const float time_step) override;

private:
    Core & _core;

//    Screen * _calling_screen;

    Targeted_particle_system _game_name_system;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;

    Picking _picking;
};


#endif // MAIN_MENU_SCREEN_H
