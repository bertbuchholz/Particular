#ifndef FOLDABLEGROUPBOX_H
#define FOLDABLEGROUPBOX_H

#include <QtGui>

template <class State>
class StateLabel : public QLabel
{
public:
    void setState(State state)
    {
        _state = state;
        setPixmap(_pixmaps[state]);
    }

    void addState(State state, QPixmap const& pixmap)
    {
        _pixmaps[state] = new QPixmap(pixmap);
    }

private:
    State _state;
    std::map<State, QPixmap*> _pixmaps;
};

class FoldableGroupBox : public QGroupBox
{
public:
    FoldableGroupBox(QWidget* widget, const QString& text = "", bool const is_foldable = true) : FoldableGroupBox(text, is_foldable)
    {
        init(text);
        setWidget(widget);

        _widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    }

    FoldableGroupBox(const QString& text = "", bool const is_foldable = true) : _widget(NULL), _showing(true), _is_foldable(is_foldable)
    {
        init(text);
    }

    void init(const QString& text)
    {
        QPixmap pixmap(30,200);
        QPainter p(&pixmap);
        QFont font = p.font();
        font.setPointSizeF(7);
        p.setFont(font);

        QRect boundingRect = p.fontMetrics().boundingRect(text);

        p.fillRect(pixmap.rect(), QBrush(Qt::white));
        p.rotate(90.0f);
        p.drawText(5, -5, text);

        p.end();


//        QPixmap tmp;
//        QPainter p(&tmp);
//        QFont font = p.font();
//        font.setPointSizeF(7);
//        p.setFont(font);

//        QRect boundingRect = p.fontMetrics().boundingRect(text);

//        QPixmap pixmap(QSize(boundingRect.height() + 10, boundingRect.width() + 10));

//        p.begin(&pixmap);
//        p.fillRect(pixmap.rect(), QBrush(Qt::white));
//        p.rotate(90.0f);
//        p.drawText(5, -5, text);
//        p.end();

        _label = new QLabel;
//        _label->setPixmap(pixmap);
        _label->setPixmap(pixmap.copy(0, 0, boundingRect.height() + 5, boundingRect.width() + 10));

        QHBoxLayout* layout = new QHBoxLayout;
//        layout->addWidget(_label);

        layout->setSpacing(0);
        layout->setMargin(0);
//        layout->setContentsMargins(0, 0, 0, 0);

        layout->setAlignment(_label, Qt::AlignTop);

        _widget = new QFrame;
        _widget->setLayout(new QVBoxLayout);
        _widget->layout()->setSpacing(0);
        _widget->layout()->setMargin(0);
        layout->addWidget(_widget);

        setLayout(layout);

        setTitle(text);
    }

    QLayout * get_widget_layout()
    {
        return _widget->layout();
    }

    void setWidget(QWidget* widget)
    {
        if (_widget)
        {
            std::cout << "Already have a widget" << std::endl;
            return;
        }

        _widget = widget;
        layout()->addWidget(_widget);

        _widget->setVisible(_showing);
    }

    virtual void mousePressEvent(QMouseEvent* event)
    {
        if (_label->geometry().contains(event->pos()) && _is_foldable)
        {
            _showing = !_showing;

            _widget->setVisible(_showing);

            update();
        }
        else
        {
            event->ignore();
            QGroupBox::mousePressEvent(event);
        }
    }

    void bla()
    {
        _showing = false;

        _widget->setVisible(_showing);

        update();
    }

//    virtual void paintEvent(QPaintEvent * event)
//    {
//        QGroupBox::paintEvent(event);

//        QPainter painter(this);
//        QPen p;
//        p.setColor(Qt::lightGray);
//        painter.setPen(p);
//        QRectF rect(QPointF(0, 0), size() - QSize(1, 1));
//        painter.drawRect(rect);
//    }

    static void test()
    {
        std::cout << "FoldableGroupBox::test();" << std::endl;

        QLabel* l1 = new QLabel("Label1");
        QLabel* l2 = new QLabel("Label2");

        FoldableGroupBox* f1 = new FoldableGroupBox;
        f1->setWidget(l1);

        FoldableGroupBox* f2 = new FoldableGroupBox;
        f2->setWidget(l2);

        QFrame* frame = new QFrame;
        QHBoxLayout* layout = new QHBoxLayout;
        layout->addWidget(f1);
        layout->addWidget(f2);
        layout->addStretch(0);
        frame->setLayout(layout);

        frame->show();
    }

private:
    QLabel* _label;
    QWidget* _widget;

    bool _showing;
    bool _is_foldable;
};

#endif // FOLDABLEGROUPBOX_H
