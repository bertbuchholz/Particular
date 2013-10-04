#ifndef MAIN_GAME_STATE_H
#define MAIN_GAME_STATE_H

#include "State.h"

#include "Renderer.h"

#include "Core.h"

class Main_game_screen : public Screen
{
public:
    Main_game_screen(My_viewer & viewer, Core & core, std::unique_ptr<World_renderer> & renderer) : Screen(viewer), _core(core), _renderer(renderer)
    {
        _type = Screen::Type::Fullscreen;
        _state = State::Running;
    }

    bool mousePressEvent(QMouseEvent *) override { return false; }

    bool mouseMoveEvent(QMouseEvent *) override { return false; }

    bool mouseReleaseEvent(QMouseEvent * ) override { return false; }

    bool keyPressEvent(QKeyEvent * event) override;

    void draw() override;

    void state_changed_event(State const current_state, State const previous_state) override;

//    void pause() override;
//    void resume() override;
    void update_event(float const time_step) override;

private:
    Core & _core;

    std::unique_ptr<World_renderer> & _renderer;
};

#endif // MAIN_GAME_STATE_H
