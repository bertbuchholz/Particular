#ifndef PAUSE_SCREEN_H
#define PAUSE_SCREEN_H

#include "Menu_screen.h"

class Pause_screen : public Menu_screen
{
public:
    Pause_screen(My_viewer & viewer, Core & core, Screen * calling_state);

    bool keyPressEvent(QKeyEvent * event) override;

    void init();

    void continue_game();
    void return_to_main_menu();
    void restart_level();

private:
    Screen * _calling_screen;
};

#endif // PAUSE_SCREEN_H
