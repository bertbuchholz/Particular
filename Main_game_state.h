#ifndef MAIN_GAME_STATE_H
#define MAIN_GAME_STATE_H

#include "State.h"

#include "Renderer.h"

#include "Core.h"

class Main_game_state : public State
{
public:
    Main_game_state(My_viewer & viewer, Core & core) : State(viewer), _core(core) {}

    virtual bool mousePressEvent(QMouseEvent *) override { return false; }

    bool mouseMoveEvent(QMouseEvent *) override { return false; }

    bool mouseReleaseEvent(QMouseEvent * ) override { return false; }

    bool keyPressEvent(QKeyEvent *) override { return false; }

//    virtual void draw(qglviewer::Camera const* camera, Parameter_list const& parameters);
    void draw() override;

private:
    Core & _core;

    std::unique_ptr<World_renderer> _renderer;
};

#endif // MAIN_GAME_STATE_H
