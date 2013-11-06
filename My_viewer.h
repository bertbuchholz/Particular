#ifndef TEMPLATE_VIEWER_H
#define TEMPLATE_VIEWER_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QSpinBox>
#include <iostream>

#include <Eigen/Geometry>

#include <Options_viewer.h>
#include <Draw_functions.h>

#include <Picking.h>
#include <Registry_parameters.h>
#include <Geometry_utils.h>
//#include <FloatSlider.h>

#include "Core.h"
#include "Atom.h"
//#include "Spatial_hash.h"
//#include "Renderer.h"

#include "Draggable.h"
#include "Level_element_draw_visitor.h"
#include "Screen.h"
#include "Main_game_screen.h"
#include "Main_menu_screen.h"
#include "Main_options_window.h"

class My_viewer : public Options_viewer // , public QGLFunctions
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    enum class Level_state { /* Main_menu, Pause_menu, */ Intro, /* Before_start, */ Running /* , After_finish, Statistics */ };

    My_viewer(Core & core, QGLFormat const& format = QGLFormat());

    void print_cam_orientation();

//    void save_level();
//    void load_level();

//    void restore_parameters() override
//    {
//        Base::restore_parameters();

//        update();
//    }

    void reset_level();

    Eigen::Vector3f calc_camera_starting_point_from_borders();

    void update_game_camera();

    void change_ui_state();
    void change_clipping();

    void init() override;

    void start();
    void init_game();

    void draw() override;

    void draw_textured_quad(GLuint const tex_id);
    void start_normalized_screen_coordinates();
    void stop_normalized_screen_coordinates();

    bool check_for_collision(Level_element const* level_element);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event);
    void keyPressEvent(QKeyEvent *event) override;

    void animate() override;

    void add_screen(Screen * s);
    void remove_all_screens();
    void replace_screens(Screen * s);

    void clear();

    void load_defaults() override;
    void resizeEvent(QResizeEvent *ev);

    void setup_fonts();

    void draw_button(Draggable_button const* b, bool const for_picking, const float alpha = 1.0f);
    void draw_label(Draggable_label const* b, const float alpha = 1.0f);
    void draw_statistic(Draggable_statistics const& b);
    void draw_slider(Draggable_slider const& s, const bool for_picking, const float alpha = 1.0f);

    void generate_button_texture(Draggable_button * b);
    void generate_label_texture(Draggable_label * b);
    void generate_statistics_texture(Draggable_statistics & b);

    void quit_game();


    QFont const& get_particle_font() const { return _particle_font; }

    QFont const& get_main_font() const { return _main_font; }

public Q_SLOTS:
    void handle_level_change(Main_game_screen::Level_state const state);

private:
    Core & _core;

    StandardCamera * _my_camera;

    QFont _main_font;
    QFont _particle_font;

    std::deque< std::unique_ptr<Screen> > _screen_stack;
};


#endif
