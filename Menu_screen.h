#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "Screen.h"

#include <Picking.h>

#include "Renderer.h"
#include "Draggable.h"
#include "Core.h"

class Menu_screen : public Screen
{
public:
    Menu_screen(My_viewer & viewer, Core & core);

    void draw() override;

    bool mousePressEvent(QMouseEvent * event) override;
    bool mouseMoveEvent(QMouseEvent *event) override;
    bool wheelEvent(QWheelEvent *) override { return true; } // eat all wheel events in menus to avoid them being used in underlying screens

    void resize(QSize const&) override;

    void draw_draggables_for_picking();
    void draw_hovered_button(Draggable_button const* b, const float time, const float alpha = 1.0f);

    void update_event(float const timestep) override;

protected:
    Core & _core;

    Ui_renderer const& _renderer;
    Picking _picking;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;
    std::vector< boost::shared_ptr<Draggable_label> > _labels;

    int _hover_index;

    std::unique_ptr<QGLShaderProgram> _heat_program;

    float _time;

    GL_functions _gl_functions;
};

#endif // MENU_SCREEN_H
