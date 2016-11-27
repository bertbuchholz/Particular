#ifndef EDITOR_PAUSE_SCREEN_H
#define EDITOR_PAUSE_SCREEN_H

#include "Menu_screen.h"

class Editor_pause_screen : public Menu_screen
{
public:
    Editor_pause_screen(My_viewer & viewer, Core & core, Screen * calling_state);

    bool keyPressEvent(QKeyEvent * event) override;

    void init();

    void continue_game();
    void return_to_main_menu();
    void play_level();
    void return_to_editor();
    void load_level();
    void save_level();

private:
    Screen * _calling_screen;
};


#endif // EDITOR_PAUSE_SCREEN_H
