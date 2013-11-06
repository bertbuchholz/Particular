#include "Main_options_window.h"

#include <Q_parameter_bridge.h>


QWidget *Main_options_window::add_parameter_list(const std::string &name, const Parameter_list &parameters)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (_param_group_to_widget_map.find(name) != _param_group_to_widget_map.end())
    {
        std::cout << __PRETTY_FUNCTION__ << " widget already existed: " << name << std::endl;
        remove_parameter_list(name);
    }

    QGroupBox * group_box = new QGroupBox(QString::fromStdString(name));
    QLayout * box_layout = new QVBoxLayout;
    group_box->setLayout(box_layout);

    QFont f = group_box->font();
    f.setPointSize(f.pointSize() * 0.9f);
    group_box->setFont(f);

    box_layout->setSpacing(0);
    box_layout->setMargin(0);
    box_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    draw_instance_list(parameters, box_layout);

    add_widget(group_box);

    _param_group_to_widget_map[name] = group_box;

    return group_box;
}


QFrame *Main_options_window::create_options_widget() const
{
    std::cout << "create_options_widget()" << std::endl;

    QFrame * frame = new QFrame;

    QFont f = frame->font();
    f.setPointSize(f.pointSize() * 0.9f);
    frame->setFont(f);

    QLayout * menu_layout = new QVBoxLayout(frame);

    menu_layout->setSpacing(0);
    menu_layout->setMargin(0);
    menu_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    frame->setWindowTitle("Main Options");
    frame->setLayout(menu_layout);

    return frame;
}


void Main_options_window::remove_parameter_list(const std::string &name)
{
    if (_param_group_to_widget_map.find(name) == _param_group_to_widget_map.end()) return;

//    assert(_param_group_to_widget_map.find(name) != _param_group_to_widget_map.end());

    QWidget * w = _param_group_to_widget_map[name];
    w->hide();
    _menu_frame->widget()->layout()->removeWidget(w);

//    for (QObject * q : w->children())
//    {
//        std::cout << __PRETTY_FUNCTION__ << " " << q << std::endl;
//    }

    delete w;
    _param_group_to_widget_map.erase(name);
}


Main_options_window *Main_options_window::get_instance()
{
    if (!_instance)
    {
        _instance = std::unique_ptr<Main_options_window>(new Main_options_window);
    }

    return _instance.get();
}


Main_options_window::Main_options_window()
{
    _menu_frame = new QScrollArea;

    _menu_frame->setWidget(create_options_widget());
    _menu_frame->setWidgetResizable(true);
    _menu_frame->setWindowFlags(_menu_frame->windowFlags() | Qt::Tool);
    _menu_frame->show();
}


void Main_options_window::remove_widget(QWidget *w)
{
    _menu_frame->widget()->layout()->removeWidget(w);
}


void Main_options_window::add_widget(QWidget *w)
{
    _menu_frame->widget()->layout()->addWidget(w);
}
