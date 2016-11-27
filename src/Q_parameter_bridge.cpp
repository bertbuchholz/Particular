#include <sstream>
#include <QPushButton>

#include "Q_parameter_bridge.h"

#include "FloatSlider.h"
#include "Combo_switcher.h"
#include "FoldableGroupBox.h"
#include "qxtgroupbox.h"

QWidget * create_parameter_widget(Parameter const* parameter)
{
    QWidget * widget = NULL;

    Q_parameter_bridge * bridge; // = new Q_parameter_bridge(parameter);

    std::cout << "create_parameter_widget(): " << parameter->get_type().name() << " " << parameter->get_name() << std::endl;

    if (parameter->get_special_type() == Parameter::Type::Normal)
    {
        if (parameter->get_type() == typeid(bool))
        {
            bridge = new Q_bool_parameter(parameter);
            widget = bridge->get_widget();
        }
        else if (parameter->get_type() == typeid(int))
        {
            bridge = new Q_int_parameter(parameter);
            widget = bridge->get_widget();
        }
        else if (parameter->get_type() == typeid(float))
        {
            bridge = new Q_float_parameter(parameter);
            widget = bridge->get_widget();
        }
        else if (parameter->get_type() == typeid(std::string))
        {
            bridge = new Q_string_parameter(parameter);
            widget = bridge->get_widget();
        }
    }
    else if (parameter->get_special_type() == Parameter::Type::Button)
    {
        bridge = new Q_parameter_bridge(parameter);
        QPushButton * button = new QPushButton(QString::fromStdString(parameter->get_name()));
        widget = button;

        QObject::connect(button, SIGNAL(clicked()), bridge, SLOT(do_callback()));
    }
    else if (parameter->get_special_type() == Parameter::Type::List)
    {
        bridge = new Q_list_parameter(parameter);
        widget = bridge->get_widget();
    }

    assert(widget);

    return widget;
}


void add_params_to_layout(std::vector<Parameter const*> const& parameters, QLayout * layout)
{
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        if (parameters[i]->get_name() == "multi_enable" || parameters[i]->is_hidden()) continue;

        QWidget * w = create_parameter_widget(parameters[i]);

        if (w->layout())
        {
            w->layout()->setSpacing(0);
            w->layout()->setMargin(0);
        }

        layout->addWidget(w);
    }
}

void draw_instance_list(Parameter_list const& parameters, QLayout * layout)
{
    if (parameters.get_children_type() == Parameter_list::Single_select)
    {
        layout->addWidget(create_single_select_widget(parameters["type"], parameters.get_children()));
    }
    else
    {
        add_params_to_layout(parameters.get_ordered(), layout);

        Parameter_list::Child_map const& children = parameters.get_children();

        Parameter_list::Child_map::const_iterator child_iter;

        for (child_iter = children.begin(); child_iter != children.end(); ++child_iter)
        {
//            FoldableGroupBox * child_box = new FoldableGroupBox(child_iter->first.c_str(), false);
//            QLayout   * sub_layout = child_box->get_widget_layout();

//            QxtGroupBox * child_box = new QxtGroupBox(child_iter->first.c_str());
//            child_box->setCollapsive(true);
            QGroupBox * child_box = new QGroupBox(child_iter->first.c_str());
            QLayout   * sub_layout = new QVBoxLayout(child_box);
            child_box->setLayout(sub_layout);

            sub_layout->setSpacing(0);
            sub_layout->setMargin(0);

            Parameter_list const& sub_list = *child_iter->second;

            if (parameters.get_children_type() == Parameter_list::Multi_select)
            {
                std::cout << "Parameter_list::Multi_select" << std::endl;
                Q_groupbox_parameter * groupbox_param = new Q_groupbox_parameter(sub_list["multi_enable"], child_box);

                assert(groupbox_param->get_widget() == child_box);
            }

            draw_instance_list(sub_list, sub_layout);

            layout->addWidget(child_box);
        }
    }
}

QWidget * create_single_select_widget(Parameter const* parameter, Parameter_list::Child_map const& children)
{
    std::cout << "create_single_select_widget(): " << parameter->get_name() << std::endl;

    Q_parameter_bridge * bridge = new Q_parameter_bridge(parameter);

    std::vector<Parameter::My_variant> const& value_list = parameter->get_value_list();


    QComboBox * combo_box = new QComboBox();

    for (std::size_t i = 0; i < value_list.size(); ++i)
    {
        std::stringstream ss;
        ss << value_list[i];
        combo_box->addItem(QString::fromStdString(ss.str()));
    }

    combo_box->setCurrentIndex(parameter->get_index());

    QObject::connect(combo_box, SIGNAL(currentIndexChanged(int)), bridge, SLOT(set_from_index(int)));


    std::vector<QGroupBox*> sub_boxes;

    for (std::size_t i = 0; i < value_list.size(); ++i)
    {
        Parameter_list::Child_map::const_iterator child_iter = children.find(boost::get<std::string>(value_list[i]));

        assert(child_iter != children.end());

        FoldableGroupBox * sub_box = new FoldableGroupBox(boost::get<std::string>(value_list[i]).c_str(), false);
//        QLayout   * sub_layout = new QVBoxLayout(sub_box);
        QLayout   * sub_layout = sub_box->get_widget_layout();

//        sub_layout->setSpacing(0);
//        sub_layout->setMargin(0);

        Parameter_list const& sub_list = *child_iter->second;

        draw_instance_list(sub_list, sub_layout);

        sub_boxes.push_back(sub_box);
    }

    assert(int(sub_boxes.size()) == combo_box->count());

    Combo_switcher * cs = new Combo_switcher(combo_box, sub_boxes);

    return cs;
}

