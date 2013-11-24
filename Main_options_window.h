#ifndef MAIN_OPTIONS_WINDOW_H
#define MAIN_OPTIONS_WINDOW_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <Parameter.h>

class Main_options_window
{
public:
    static Main_options_window * get_instance();
    static Main_options_window * create();


    void remove_parameter_list(std::string const& name);
    QFrame * create_options_widget() const;
    QWidget * add_parameter_list(std::string const& name, Parameter_list const& parameters);

    void show();

private:
    Main_options_window();
    void add_widget(QWidget * w);
    void remove_widget(QWidget * w);

    static std::unique_ptr<Main_options_window> _instance;

    std::unique_ptr<QScrollArea> _menu_frame;

    std::unordered_map<std::string, QWidget*> _param_group_to_widget_map;
};

#endif // MAIN_OPTIONS_WINDOW_H
