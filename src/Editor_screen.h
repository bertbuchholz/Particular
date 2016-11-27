#ifndef EDITOR_SCREEN_H
#define EDITOR_SCREEN_H

#include "Main_game_screen.h"

#include "Main_options_window.h"
#include "widget_text_combination.h"

class Editor_screen : public Main_game_screen
{
public:
    Editor_screen(My_viewer & viewer, Core & core);
    ~Editor_screen();

    bool mousePressEvent(QMouseEvent *event) override;
    bool mouseMoveEvent(QMouseEvent *) override;
    bool mouseReleaseEvent(QMouseEvent * event) override;
    bool keyPressEvent(QKeyEvent * event) override;

    void init_controls();
    void level_element_button_pressed(std::string const& type);
    void molecule_button_right_click_event(std::string const& type);
    void parameter_slider_right_click_event(std::string const& parameter_name);

    void add_selected_level_element(QPoint const& mouse_pos);


private:
    void toggle_simulation();
    void hide_controls();
    void show_controls();
    void slider_changed();
    void reset_level();
    void clear_level();
    void show_advanced_options();
    void hide_advanced_options();
    void load_defaults();

    std::vector<std::string> _placeable_molecules;
    std::unordered_map<std::string, int> _num_molecules_to_be_placed_per_type;

    std::map<std::string, boost::shared_ptr<Draggable_button> > _level_element_buttons;

    boost::shared_ptr<Draggable_button> _toggle_simulation_button;
    boost::shared_ptr<Draggable_button> _hide_controls_button;
    boost::shared_ptr<Draggable_button> _show_controls_button;
    boost::shared_ptr<Draggable_button> _button_hide_advanced_options;
    boost::shared_ptr<Draggable_button> _button_show_advanced_options;

    std::unique_ptr<Main_options_window> _advanced_options_window;

    std::vector< boost::shared_ptr<Draggable> > _normal_controls;
    std::vector< boost::shared_ptr<Draggable> > _advanced_controls;

    std::unordered_map<std::string, Parameter*> _slider_parameter_names_to_parameters;

//    boost::shared_ptr<Draggable_slider> _translation_fluctuation_slider;

};



#endif // EDITOR_SCREEN_H
