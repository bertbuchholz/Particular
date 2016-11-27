#pragma once

#include "Menu_screen.h"

#include <boost/ptr_container/ptr_vector.hpp>

#include "Draggable_event.h"

class Level_picker_screen : public Menu_screen
{
public:
    Level_picker_screen(My_viewer & viewer, Core & core, Screen * calling_screen);

    void init();

//    void draw() override;

//    void update_event(const float time_step) override;

    void exit();
//    void repeat();

    void play_level(std::string const& level_name);
    void show_stats(std::string const& level_name);
    void change_page(std::string const& left_or_right);
    void return_to_main_menu();

    void show_page(int const page_index);

private:
    int _current_page;

    std::vector< boost::shared_ptr< Draggable_label> >  _level_name_labels;
    std::vector< boost::shared_ptr< Draggable_button> > _play_buttons;
    std::vector< boost::shared_ptr< Draggable_button> > _statistic_buttons;
    std::vector< boost::shared_ptr< Draggable_button> > _page_arrow_buttons; // 0: left, 1: right

    Screen * _calling_screen;
};
