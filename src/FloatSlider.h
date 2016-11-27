#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include <QtGui>
#include <QSlider>
#include <QToolTip>

#include <iostream>
#include <cmath>

class FloatSlider : public QSlider
{
    Q_OBJECT

public:
    static void test()
    {
        FloatSlider* fl = new FloatSlider(0, 2, 0.5);
        fl->show();
    }

    FloatSlider(float min = 0.0f, float max = 1.0f, float initial = 0.0f) :
        _precision(10000.0f),
        _showValue(true)
    {
        setMinimum(min * _precision);
        setMaximum(max * _precision);
        setValueF(initial);

        setOrientation(Qt::Horizontal);

        connect(this, SIGNAL(valueChanged(int)), this, SLOT(changeValue(int)));
    }

    ~FloatSlider() {}

    virtual void mouseMoveEvent(QMouseEvent *event)
    {
        QSlider::mouseMoveEvent(event);

        if (_showValue)
        {
            QToolTip::showText(mapToGlobal(QPoint(0, 0)) + QPoint(((value() - minimum()) / float(maximum() - minimum())) * width(), height() / 2.0f - 45), QString("%1").arg(getValueF()), this);
        }
    }

    virtual float getValueF()
    {
        return value() / _precision;
    }

    virtual void setValueF(float v)
    {
        //std::cout << "setting " << v << std::endl;
        QSlider::setValue(v * _precision);
    }

    void setShowValue(bool showValue)
    {
        _showValue = showValue;
    }

signals:
    void valueChanged(float);

protected:
    float _precision;
    bool _showValue;

private slots:
    void changeValue(int /* v */)
    {
        emit valueChanged(getValueF());
    }
};

#endif // FLOATSLIDER_H
