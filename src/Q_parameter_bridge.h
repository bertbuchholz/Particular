#ifndef Q_PARAMETER_BRIDGE_H
#define Q_PARAMETER_BRIDGE_H

#include <QWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

#include "Squared_float_slider.h"
#include "Parameter.h"
#include "Utilities.h"

class Mouse_wheel_eater : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject * object, QEvent * event)
    {
//        if (event->type() == QEvent::Wheel)
//        {
//            QWidget * widget = qobject_cast<QWidget*>(object);

//            if (widget && !widget->hasFocus())
//            {
//                std::cout << __FUNCTION__ << " filtering" << std::endl;
//                return true;
//            }
//        }


        QWidget * widget = qobject_cast<QWidget*>(object);

        if (widget)
        {
            if (event->type() == QEvent::Wheel)
            {
                if(widget->focusPolicy() == Qt::WheelFocus)
                {
                    event->accept();
                    return false;
                }
                else
                {
                    event->ignore();
                    return true;
                }
            }
            else if (event->type() == QEvent::FocusIn)
            {
                widget->setFocusPolicy(Qt::WheelFocus);
            }
            else if (event->type() == QEvent::FocusOut)
            {
                widget->setFocusPolicy(Qt::StrongFocus);
            }
        }

        return QObject::eventFilter(object, event);
    }
};


class Q_parameter_bridge : public QObject
{
    Q_OBJECT

public:
    Q_parameter_bridge(Parameter const* parameter) : _parameter(const_cast<Parameter*>(parameter))
    { }

    ~Q_parameter_bridge()
    {
//        std::cout << __FUNCTION__ << std::endl;
    }

public Q_SLOTS:
    void set_int(int const value) { _parameter->set_value(value); }

    void set_float(float const value) { _parameter->set_value(value); }
    void set_double(double const value) { _parameter->set_value(float(value)); }

    void set_bool(bool const value)         { _parameter->set_value(value); }
    void set_bool_from_int(int const value) { _parameter->set_value(bool(value)); }

    void set_string(QString const& text) { _parameter->set_value(text.toStdString()); }

    void do_callback() { _parameter->do_callback(); }

    void set_from_index(int index) { _parameter->set_from_index(index); }

    void handle_widget_destroyed()
    {
//        std::cout << __FUNCTION__ << std::endl;
        delete this;
    }

    QWidget * get_widget()
    {
        return _widget;
    }

protected:
    Parameter * _parameter;
    QWidget * _widget;
};


class Q_bool_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_bool_parameter(Parameter const* parameter) : Q_parameter_bridge(parameter)
    {
        _check_box = new QCheckBox(QString::fromStdString(parameter->get_name()));
        _check_box->setChecked(parameter->get_value<bool>());
        _widget = _check_box;

        QObject::connect(_check_box, SIGNAL(stateChanged(int)), this, SLOT(set_bool_from_int(int)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_bool_parameter()
    {
        _parameter->remove_observer(this);
    }

    void notify() override
    {
        _check_box->blockSignals(true);
        _check_box->setChecked(_parameter->get_value<bool>());
        _check_box->blockSignals(false);
    }

private:
    QCheckBox * _check_box;
};

class Q_groupbox_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_groupbox_parameter(Parameter const* parameter, QGroupBox * group_box = nullptr) : Q_parameter_bridge(parameter)
    {
        if (group_box)
        {
            _group_box = group_box;
        }
        else
        {
            _group_box = new QGroupBox(QString::fromStdString(parameter->get_name()));
        }

        _group_box->setCheckable(true);
        _group_box->setChecked(parameter->get_value<bool>());
        _widget = _group_box;

        QObject::connect(_group_box, SIGNAL(toggled(bool)), this, SLOT(set_bool(bool)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_groupbox_parameter()
    {
        _parameter->remove_observer(this);
    }

    QGroupBox * get_group_box()
    {
        return _group_box;
    }

    void notify() override
    {
        _group_box->blockSignals(true);
        _group_box->setChecked(_parameter->get_value<bool>());
        _group_box->blockSignals(false);
    }

private:
    QGroupBox * _group_box;
};

class Q_int_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_int_parameter(Parameter const* parameter) : Q_parameter_bridge(parameter)
    {
        QFrame * frame = new QFrame;
        QGridLayout * layout = new QGridLayout;

        _spin_box = new QSpinBox;
        _spin_box->setMinimum(parameter->get_min<int>());
        _spin_box->setMaximum(parameter->get_max<int>());
        _spin_box->setValue(parameter->get_value<int>());
        _spin_box->setKeyboardTracking(false);

        _spin_box->setFocusPolicy(Qt::StrongFocus);
        _spin_box->installEventFilter(new Mouse_wheel_eater);

        QLabel * label = new QLabel(QString::fromStdString(parameter->get_name()));

        layout->addWidget(label,     0, 0);
        layout->addWidget(_spin_box, 0, 1);

        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 1);

        frame->setLayout(layout);
        _widget = frame;

        QObject::connect(_spin_box, SIGNAL(valueChanged(int)), this, SLOT(set_int(int)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_int_parameter()
    {
        _parameter->remove_observer(this);
    }

    void notify() override
    {
        _spin_box->blockSignals(true);
        _spin_box->setMinimum(_parameter->get_min<int>());
        _spin_box->setMaximum(_parameter->get_max<int>());
        _spin_box->setValue(_parameter->get_value<int>());
        _spin_box->blockSignals(false);
    }

private:
    QSpinBox * _spin_box;
};


class Q_float_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_float_parameter(Parameter const* parameter) : Q_parameter_bridge(parameter)
    {
        QFrame * frame = new QFrame;
        // QHBoxLayout * layout = new QHBoxLayout;
        QGridLayout * layout = new QGridLayout;

        //            FloatSlider * slider = new FloatSlider(parameter->get_min<float>(),
        //                                                   parameter->get_max<float>(),
        //                                                   parameter->get_float());

//        _slider = new Squared_float_slider(parameter->get_min<float>(),
//                                           parameter->get_max<float>(),
//                                           parameter->get_value<float>());

        _slider = new QDoubleSpinBox;
        _slider->setMinimum(parameter->get_min<float>());
        _slider->setMaximum(parameter->get_max<float>());
        _slider->setValue(parameter->get_value<float>());

        _slider->setFocusPolicy(Qt::StrongFocus);
        _slider->installEventFilter(new Mouse_wheel_eater);

        float const range = std::abs(parameter->get_max<float>() - parameter->get_min<float>());

        _slider->setSingleStep(range * 0.001f);
        _slider->setDecimals(4.0f - into_range(std::log10(range), 0.0f, 4.0f));

        _slider->setKeyboardTracking(false);

        QLabel * label = new QLabel(QString::fromStdString(parameter->get_name()));

        layout->addWidget(label, 0, 0);
        layout->addWidget(_slider, 0, 1);

        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 1);

        frame->setLayout(layout);
        _widget = frame;

        QObject::connect(_slider, SIGNAL(valueChanged(double)), this, SLOT(set_double(double)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_float_parameter()
    {
//        std::cout << __FUNCTION__ << std::endl;
        _parameter->remove_observer(this);
    }

    void notify() override
    {
        _slider->blockSignals(true);

        _slider->setMinimum(_parameter->get_min<float>());
        _slider->setMaximum(_parameter->get_max<float>());
        _slider->setValue(_parameter->get_value<float>());

        _slider->blockSignals(false);

//        _slider->setMinimumF(_parameter->get_min<float>());
//        _slider->setMaximumF(_parameter->get_max<float>());
//        _slider->setValueF(_parameter->get_value<float>());
    }

private:
//    Squared_float_slider * _slider;
    QDoubleSpinBox * _slider;
};


class Q_list_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_list_parameter(Parameter const* parameter) : Q_parameter_bridge(parameter)
    {
        std::vector<Parameter::My_variant> const& value_list = _parameter->get_value_list();

        QFrame * frame = new QFrame;
        QHBoxLayout * layout = new QHBoxLayout;

        _combo_box = new QComboBox;
        QLabel * label = new QLabel(QString::fromStdString(_parameter->get_name()));

        for (std::size_t i = 0; i < value_list.size(); ++i)
        {
            std::stringstream ss;
            ss << value_list[i];
            _combo_box->addItem(QString::fromStdString(ss.str()));
        }

        _combo_box->setCurrentIndex(_parameter->get_index());

        _combo_box->setFocusPolicy(Qt::StrongFocus);
        _combo_box->installEventFilter(new Mouse_wheel_eater);

        layout->addWidget(label);
        layout->addWidget(_combo_box);

        frame->setLayout(layout);
        _widget = frame;

        QObject::connect(_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(set_from_index(int)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_list_parameter()
    {
        _parameter->remove_observer(this);
    }

    void init_item()
    {
        std::vector<Parameter::My_variant> const& value_list = _parameter->get_value_list();

        _combo_box->clear();

        for (std::size_t i = 0; i < value_list.size(); ++i)
        {
            std::stringstream ss;
            ss << value_list[i];
            _combo_box->addItem(QString::fromStdString(ss.str()));
        }

        _combo_box->setCurrentIndex(_parameter->get_index());
    }

    void notify() override
    {
//        _combo_box->setCurrentIndex(_parameter->get_index());
        _combo_box->blockSignals(true);
        init_item();
        _combo_box->blockSignals(false);
    }

private:
    QComboBox * _combo_box;
};


class Q_string_parameter : public Q_parameter_bridge, Notifiable
{
public:
    Q_string_parameter(Parameter const* parameter) : Q_parameter_bridge(parameter)
    {
        QFrame * frame = new QFrame;
        // QHBoxLayout * layout = new QHBoxLayout;
        QGridLayout * layout = new QGridLayout;

        _textbox = new QLineEdit;
        _textbox->setText(QString::fromStdString(_parameter->get_value<std::string>()));

        QLabel * label = new QLabel(QString::fromStdString(parameter->get_name()));

        layout->addWidget(label, 0, 0);
        layout->addWidget(_textbox, 0, 1);

        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 1);

        frame->setLayout(layout);
        _widget = frame;

        QObject::connect(_textbox, SIGNAL(textChanged(QString const&)), this, SLOT(set_string(QString const&)));
        connect(_widget, SIGNAL(destroyed()), this, SLOT(handle_widget_destroyed()));

        _parameter->add_observer(this);
    }

    ~Q_string_parameter()
    {
        _parameter->remove_observer(this);
    }

    void notify() override
    {
        _textbox->blockSignals(true);

        _textbox->setText(QString::fromStdString(_parameter->get_value<std::string>()));

        _textbox->blockSignals(false);
    }

private:
    QLineEdit * _textbox;
};


QWidget * create_parameter_widget(Parameter const* parameter);

QWidget * create_single_select_widget(Parameter const* parameter, const Parameter_list::Child_map &children);

void draw_instance_list(Parameter_list const& parameters, QLayout * layout);

#endif
