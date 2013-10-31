#ifndef EDITOR_SCREEN_H
#define EDITOR_SCREEN_H

#include "Main_game_screen.h"

class Editor_screen : public Main_game_screen
{
public:
    Editor_screen(My_viewer & viewer, Core & core) : Main_game_screen(viewer, core, Ui_state::Level_editor) {}

    bool mousePressEvent(QMouseEvent *event) override;
    bool mouseMoveEvent(QMouseEvent *) override;
    bool mouseReleaseEvent(QMouseEvent * event) override;
    bool keyPressEvent(QKeyEvent * event) override;

};

#endif // EDITOR_SCREEN_H
