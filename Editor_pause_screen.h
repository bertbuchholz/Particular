#ifndef EDITOR_PAUSE_SCREEN_H
#define EDITOR_PAUSE_SCREEN_H

#include "Screen.h"

#include <Picking.h>

#include "Renderer.h"
#include "Core.h"
#include "My_viewer.h"
#include "Main_menu_screen.h"

#include "Draggable.h"


class Editor_pause_screen : public Screen
{
public:
    Editor_pause_screen(My_viewer & viewer, Core & core, Screen * calling_state);

    bool mousePressEvent(QMouseEvent * event) override;
    bool keyPressEvent(QKeyEvent * event) override;

    void draw() override;
    void draw_draggables_for_picking();

    void init();

    void continue_game();
    void return_to_main_menu();
    void play_level();
    void return_to_editor();
    void load_level();
    void save_level();

private:
    Core & _core;

    Screen * _calling_screen;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;

    Picking _picking;
};


#endif // EDITOR_PAUSE_SCREEN_H
