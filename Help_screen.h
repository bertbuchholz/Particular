#ifndef HELP_SCREEN_H
#define HELP_SCREEN_H

#include "Menu_screen.h"

class Help_event
{
public:
    Help_event() {}

    Help_event(QString const& text, Eigen::Vector3f const& position, float const radius) :
        _text(text), _position(position), _radius(radius)
    {}

//private:
    QString _text;
    Eigen::Vector3f _position;
    float _radius;
};

class Help_screen : public Menu_screen
{
public:
    Help_screen(My_viewer & viewer, Core & core, Screen * calling_state);

//    bool keyPressEvent(QKeyEvent * event) override;

    void init();

//    void continue_game();
//    void return_to_main_menu();
//    void restart_level();

private:
    Screen * _calling_screen;
};

#endif // HELP_SCREEN_H
