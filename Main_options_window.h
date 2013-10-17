#ifndef MAIN_OPTIONS_WINDOW_H
#define MAIN_OPTIONS_WINDOW_H

#include <QtGui>
#include <memory>

#include <Q_parameter_bridge.h>

class Main_options_window
{
public:
    static Main_options_window * get_instance()
    {
        if (!_instance)
        {
            _instance = std::unique_ptr<Main_options_window>(new Main_options_window);
        }

        return _instance.get();
    }

    void add_widget(QWidget * w)
    {
        _menu_frame->widget()->layout()->addWidget(w);
    }

    void remove_widget(QWidget * w)
    {
        _menu_frame->widget()->layout()->removeWidget(w);
    }

    QFrame * create_options_widget() const
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

    QWidget * add_parameter_list(Parameter_list const& parameters)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        QFrame * frame = new QFrame;
        QLayout * frame_layout = new QVBoxLayout;

        draw_instance_list(parameters, frame_layout);

        frame->setLayout(frame_layout);

        add_widget(frame);

        return frame;
    }

private:
    Main_options_window()
    {
        _menu_frame = new QScrollArea;

        _menu_frame->setWidget(create_options_widget());
        _menu_frame->setWidgetResizable(true);
        _menu_frame->setWindowFlags(_menu_frame->windowFlags() | Qt::Tool);
        _menu_frame->show();
    }

    static std::unique_ptr<Main_options_window> _instance;

    QScrollArea * _menu_frame;
};

#endif // MAIN_OPTIONS_WINDOW_H
