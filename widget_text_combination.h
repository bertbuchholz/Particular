#pragma once

#include <QWidget>

class Widget_text_combination : public QWidget
{
    public:
    Widget_text_combination(QString const& text, QWidget * widget)
    {
        QHBoxLayout * layout = new QHBoxLayout;
        layout->addWidget(new QLabel(text));
        layout->addWidget(widget);
        setLayout(layout);
    }
};
