#ifndef HELP_SCREEN_H
#define HELP_SCREEN_H

#include "Menu_screen.h"

class Help_screen : public Menu_screen
{
public:
    struct Help_item
    {
        QString _text;
        Eigen::Vector2f _text_rect_position; // upper left corner
        Eigen::Vector2f _text_rect_size;
        Eigen::Vector2f _position;
        Eigen::Vector2f _radius;
        Curved_particle_system _particle_system;
        bool _use_particle_system;
    };

    Help_screen(My_viewer & viewer, Core & core, Screen & calling_screen, Screen::Type const type = Screen::Type::Modal);

//    bool keyPressEvent(QKeyEvent * event) override;

    void init();

    void draw() override;
    void update_event(float const time_step) override;

    virtual void continue_game();
    void next_help();

    static Help_screen * test(My_viewer &viewer, Core &core);

    std::vector<Help_item> _help_items;

protected:
    Screen & _calling_screen;

    int _current_item_index;
};

#endif // HELP_SCREEN_H
