#ifndef MAIN_GAME_STATE_H
#define MAIN_GAME_STATE_H

#include "Screen.h"

#include "Renderer.h"
#include "Picking.h"
#include "Draggable.h"
#include "GL_texture.h"
#include "Draggable_event.h"

class Core;

class Main_game_screen : public Screen, public QGLFunctions
{
    Q_OBJECT

public:
    enum class Mouse_state { None, Init_drag_handle, /* Init_drag_molecule, Dragging_molecule, */ Dragging_handle, Level_element_button_selected };
    enum class Selection { None, Level_element, Molecule };
    enum class Ui_state { Editor, Editor_playing, Playing };
    enum class Level_state { Intro, Running };
    enum class Intro_state { Beginning, Single_molecule, Two_molecules_0, Two_molecules_1, Two_molecules_2, Two_molecules_3, Finishing, Finished };

    Main_game_screen(My_viewer & viewer, Core & core, Ui_state ui_state = Ui_state::Playing);

    ~Main_game_screen();

    void init_labels();

    bool mousePressEvent(QMouseEvent *event) override;
    bool mouseMoveEvent(QMouseEvent *) override;
    bool mouseReleaseEvent(QMouseEvent * event) override;
    bool keyPressEvent(QKeyEvent * event) override;


    void draw() override;

    void state_changed_event(State const current_state, State const previous_state) override;

    void update_event(float const time_step) override;

    void delete_selected_element();
    void show_context_menu_for_element();

    void draw_molecules_for_picking();
    void draw_draggables_for_picking();

    void draw_draggables();
    void update_active_draggables();
    void update_draggable_to_level_element();
    void update_score_labels();

    void add_element_event(QPoint const& position);
    void add_element(Eigen::Vector3f const& position, std::string const& element_type, bool const make_fully_editable = false);

    void update_level_element_buttons();
    void level_element_button_pressed(std::string const& type);
    void add_selected_level_element(const QPoint &mouse_pos);

    void change_renderer();

    void change_speed_pressed();

    void resize(QSize const& size) override;

public Q_SLOTS:
    void handle_level_change(Main_game_screen::Level_state);
    void handle_game_state_change();

    // Intro ---------------------
protected Q_SLOTS:
    void intro_cam2_end_reached();
    void intro_cam1_end_reached();
    void update_intro(const float timestep);
    void setup_intro();

private:
    float _intro_time;
    Intro_state _intro_state;
    // ---------------------------

protected:
    Core & _core;

    Parameter_list _parameters;

    std::unique_ptr<World_renderer> _renderer;
    Ui_renderer const& _ui_renderer;

    Picking _picking;

    QPoint _dragging_start;
    Eigen::Vector3f _dragging_start_3d;
    int _picked_index;

    Mouse_state _mouse_state;
    Selection _selection;
    Level_element * _selected_level_element;
    Ui_state _ui_state;
    Level_state _level_state;

    std::string _selected_level_element_button_type;

    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;

    std::vector<Draggable*> _active_draggables;
    std::unordered_map<Draggable*, Level_element*> _draggable_to_level_element;

    std::vector< boost::shared_ptr<Draggable_button> > _buttons;
    std::vector< boost::shared_ptr<Draggable_slider> > _sliders;
    std::vector< boost::shared_ptr<Draggable_spinbox> > _spinboxes;
    std::vector< boost::shared_ptr<Draggable_label> > _labels;
    std::unordered_map<Draggable*, boost::shared_ptr<Draggable_tooltip> > _tooltips_map;

    boost::shared_ptr<Draggable_label> _energy_amount_label;
    boost::shared_ptr<Draggable_label> _energy_bonus_label;
    boost::shared_ptr<Draggable_label> _time_label;

    std::vector< boost::shared_ptr<Draggable_event> > _draggable_events;

    int _last_updated_bonus;

    std::unordered_map<std::string, QImage> _element_images;

    GLuint _rotate_tex;
    GLuint _scale_tex;
    GLuint _move_tex;
    GLuint _slider_tex;

    std::unique_ptr<QGLFramebufferObject> _main_fbo;
    std::unique_ptr<QGLShaderProgram> _screen_quad_program;
    std::unique_ptr<QGLShaderProgram> _blur_program;
    std::unique_ptr<QGLShaderProgram> _drop_shadow_program;

//    GLuint _tmp_screen_texture[2];
    GL_texture _tmp_screen_texture[2];
};

#endif // MAIN_GAME_STATE_H
