#ifndef SQUARED_FLOAT_SLIDER_H
#define SQUARED_FLOAT_SLIDER_H

#include <QtGui>
#include <QtWidgets>

#include <iostream>
#include <cmath>

class Squared_float_slider : public QSlider
{
    Q_OBJECT

public:
    static void test()
    {
        QSlider * fl = new Squared_float_slider(0, 2, 0.5);
        fl->show();
    }

    Squared_float_slider(float const min = 0.0f, float const max = 1.0f, float const initial = 0.0f, float const scale_compression = 3.0f) :
        _precision(10000.0f),
        _scale_compression(scale_compression),
        _showValue(true)
    {
        setMinimumF(min);
        setMaximumF(max);
        setValueF(initial);

        setOrientation(Qt::Horizontal);

        connect(this, SIGNAL(valueChanged(int)), this, SLOT(changeValue(int)));

        setMouseTracking(true);
    }

    ~Squared_float_slider() {}

    virtual void mouseMoveEvent(QMouseEvent *event)
    {
        QSlider::mouseMoveEvent(event);

        if (event->modifiers() & Qt::ControlModifier)
        {
            int const discretization = _precision * 0.1f;

            setValue((value() / discretization) * discretization);
        }

        if (_showValue)
        {
            QPoint position;

            if (orientation() == Qt::Vertical)
            {
                position = mapToGlobal(QPoint(0, 0)) + QPoint(width() / 2.0f - 45, (1.0f - ((value() - minimum()) / float(maximum() - minimum()))) * height());
            }
            else
            {
                position = mapToGlobal(QPoint(0, 0)) + QPoint(((value() - minimum()) / float(maximum() - minimum())) * width(), height() / 2.0f - 45);
            }

            QToolTip::showText(position, QString("%1").arg(getValueF()), this);
        }
    }

    virtual float getValueF()
    {
        return convert_to_output(value());
    }

    void setMinimumF(float const v)
    {
        //std::cout << "setting " << v << std::endl;
        QSlider::setMinimum(convert_to_slider(v));
    }

    void setMaximumF(float const v)
    {
        //std::cout << "setting " << v << std::endl;
        QSlider::setMaximum(convert_to_slider(v));
    }

    virtual void setValueF(float const v)
    {
        //std::cout << "setting " << v << std::endl;
        QSlider::setValue(convert_to_slider(v));
    }

    void setShowValue(bool showValue)
    {
        _showValue = showValue;
    }

    float convert_to_slider(float const input)
    {
        // return std::log(input) * _precision;
        return std::pow(input, 1.0f / _scale_compression) * _precision;
    }

    float convert_to_output(float const input)
    {
        // return (input / _precision) * (input / _precision);
        // return std::exp(input / _precision);
        return std::pow(input / _precision, _scale_compression);
    }

Q_SIGNALS:
    void valueChanged(float);

protected:
    float _precision;
    float _scale_compression;

    bool _showValue;

private Q_SLOTS:
    void changeValue(int /* v */)
    {
        Q_EMIT valueChanged(getValueF());
    }
};

#endif // SQUARED_FLOAT_SLIDER_H
