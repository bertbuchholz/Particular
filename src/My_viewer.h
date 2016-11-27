#ifndef TEMPLATE_VIEWER_H
#define TEMPLATE_VIEWER_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QSpinBox>
#include <QOpenGLFunctions_3_3_Core>
#include <iostream>

#include <Eigen/Geometry>

#include "Options_viewer.h"
#include "Draw_functions.h"
#include "Registry_parameters.h"
#include "Geometry_utils.h"
#include "Core.h"
#include "Renderer.h"
#include "Screen.h"
#include "Main_game_screen.h"


class My_viewer : public Options_viewer, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    My_viewer(Core & core, QGLFormat const& format = QGLFormat());

    void print_cam_orientation();

    Eigen::Vector3f calc_camera_starting_point_from_borders();

    void update_game_camera();

    void change_clipping();

    void init() override;

    void start();

    void draw() override;

    Eigen::Vector2f get_projected_coordinates(Eigen::Vector3f const& world_position) const;

    void draw_textured_quad(GLuint const tex_id);
    void start_normalized_screen_coordinates();
    void stop_normalized_screen_coordinates();

//    bool check_for_collision(Level_element const* level_element);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent * event) override;
    void mouseDoubleClickEvent(QMouseEvent * /* event */) override {} // ignore doubleclicks

    void animate() override;

    void add_screen(Screen * s);
    void add_screen_delayed(Screen * s);
    void kill_all_screens();
    void replace_screens(Screen * s);
//    void kill_screens_on_top(Screen * s); // remove the screens that are in the stack above s
    Screen * get_current_screen() const;

    //    void clear();

    void load_defaults() override;
    void resizeEvent(QResizeEvent *ev) override;

    void setup_fonts();

    void draw_button(Draggable_button const* b, bool const for_picking, const float alpha = 1.0f);
    void draw_label(Draggable_label const* b, const float alpha = 1.0f);
    void draw_statistic(Draggable_statistics const& b);
    void draw_slider(Draggable_slider const& s, const bool for_picking, const float alpha = 1.0f);

//    void draw_spinbox(const Draggable_spinbox &s, const bool for_picking, const float alpha = 1.0f);

    void quit_game();

    Ui_renderer const& get_renderer() const;
    Ui_renderer & get_renderer();

    QFont const& get_particle_font() const { return _particle_font; }

    Eigen::Vector2f qpixel_to_uniform_screen_pos(QPoint const& p);

    void disable_camera_control();
    void enable_camera_control();

public Q_SLOTS:
    void handle_level_change(Main_game_screen::Level_state const state);

private:
    Core & _core;

    StandardCamera * _my_camera;

    QFont _particle_font;

    std::deque< std::unique_ptr<Screen> > _screen_stack;
    std::vector<Screen*> _delayed_screen_stack;

    Ui_renderer _ui_renderer;

    QElapsedTimer _frame_timer;
};


#endif
