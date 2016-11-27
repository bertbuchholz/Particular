#include "Parameter.h"

#ifndef Q_MOC_RUN
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/nvp.hpp>
#endif

#include <QFileInfo>

std::type_info const& Parameter::get_type() const
{
    return _value.type();
}

void Parameter::set_name(std::string const& name)
{
    _name = name;
}

void Parameter::set_description(std::string const& description)
{
    _description = description;
}

std::string const& Parameter::get_name() const
{
    return _name;
}

//std::string const& Parameter::get_group() const
//{
//    return _group;
//}

std::string const& Parameter::get_description() const
{
    return _description;
}


void Parameter::set_from_index(int index)
{
    _index = index;
    _value = _value_list[_index];
    if (_callback_on_change) _callback_on_change();
    notify_observers();
}



void test()
{
    std::map<std::string, Parameter*> parameter_map;

    parameter_map["dijkstra_vertical_cost"] = new Parameter("Vertical Cost", 0.01f, 0.0f, 0.5f);

    std::vector<float> float_vec;
    float_vec.push_back(0.1f);
    float_vec.push_back(0.5f);
    float_vec.push_back(0.9f);

    parameter_map["floats"] = new Parameter("Floats", 1, float_vec);
    /*
    for (std::map<std::string, Parameter*>::const_iterator iter = parameter_map.begin();
         iter != parameter_map.end();
         ++iter)
    {
        seqLayout->addRow(create_parameter_widget(iter->second));
    }
    */
}


void Parameter_list::load(std::map<std::string, Parameter *> &list, const std::map<std::string, Parameter *> &tmp_list, Parameter_list::Child_map &children, const Parameter_list::Child_map &tmp_children) const
{

    std::map<std::string, Parameter*>::const_iterator tmp_list_iter;
    for (tmp_list_iter = tmp_list.begin(); tmp_list_iter != tmp_list.end(); ++tmp_list_iter)
    {
        std::string const& param_name = tmp_list_iter->first;

        std::map<std::string, Parameter*>::iterator iter = list.find(param_name);

        if (iter == list.end())
        {
            std::cout << "Parameter_list::load(): No such parameter: " << param_name << std::endl;
        }
        else
        {
            Parameter* p = iter->second;
            p->set_variant(tmp_list_iter->second->get_variant());
            p->set_index(tmp_list_iter->second->get_index());
        }
    }

    Child_map::const_iterator tmp_children_iter;
    for (tmp_children_iter = tmp_children.begin(); tmp_children_iter != tmp_children.end(); ++tmp_children_iter)
    {
        std::string const& param_list_name = tmp_children_iter->first;

        Child_map::iterator iter = children.find(param_list_name);

        if (iter == children.end())
        {
            std::cout << "Parameter_list::load(): No such child: " << param_list_name << std::endl;
        }
        else
        {
            Parameter_list * param_list = iter->second;
            Parameter_list * tmp_param_list = tmp_children_iter->second;

            load(param_list->_list, tmp_param_list->_list, param_list->_children, tmp_param_list->_children);
        }
    }
}


void Parameter_list::load(const Parameter_list &tmp_param_list)
{
    load(_list, tmp_param_list._list, _children, tmp_param_list._children);
    tmp_param_list.print();
}


const Parameter_list *Parameter_list::get_child(const std::string &name) const
{
    Child_map::const_iterator iter = _children.find(name);

    if (iter == _children.end())
    {
        std::cout << "Parameter_list::get_child_helper(): child not found: " << name << std::endl;
        std::cout << "Available: " << std::endl;
        print();

        assert(false);
    }

    return iter->second;
}

void Parameter_list::add_child(const std::string &name, Parameter_list *list)
{
    Child_map::const_iterator iter = _children.find(name);

    assert(iter == _children.end()); // make sure name doesn't exist yet

    _children[name] = list;
}

const Parameter *Parameter_list::get_parameter(const std::string &name) const
{
    size_t const separator_pos = name.find_first_of('/');

    if (separator_pos == std::string::npos)
    {
        std::map<std::string, Parameter*>::const_iterator iter = _list.find(name);

        if (iter == _list.end())
        {
            std::cout << "Parameter_list[]: No such parameter: " << name << std::endl;
            assert(false);
        }

        return iter->second;
    }
    else
    {
        std::string child_name = name.substr(0, separator_pos);
        std::string rest_name = name.substr(separator_pos + 1);
        //            shortened_name.erase(0, separator_pos + 1);

        return get_child(child_name)->get_parameter(rest_name);
    }
}

void Parameter_list::add_parameter(const std::string &id, Parameter *new_parameter)
{
    assert(_list.find(id) == _list.end()); // make sure no parameter of that name already exists

    new_parameter->set_order_index(int(_list.size()));
    _list[id] = new_parameter;
}

void Parameter_list::add_parameter_keep_order(const std::string &id, Parameter *new_parameter)
{
    assert(_list.find(id) == _list.end()); // make sure no parameter of that name already exists
    _list[id] = new_parameter;
}

std::vector<const Parameter *> Parameter_list::get_ordered() const
{
    std::vector<Parameter const*> ordered_list;

    std::map<std::string, Parameter*>::const_iterator iter;

    for (iter = _list.begin(); iter != _list.end(); ++iter)
    {
        ordered_list.push_back(iter->second);
    }

    std::sort(ordered_list.begin(), ordered_list.end(), Sort_by_order_index());

    return ordered_list;
}

Parameter_list *Parameter_list::add_child(const std::string &name, const Parameter_list::Children_type type)
{
    Child_map::const_iterator iter = _children.find(name);

    assert(iter == _children.end()); // make sure name doesn't exist yet

    Parameter_list *& list = _children[name];
    list = new Parameter_list;
    list->_children_type = type;
    return list;
}

void Parameter_list::set_children_callback_function(Parameter_list::Child_map &children, std::function<void ()> update_function)
{
    Child_map::const_iterator child_iter;

    for (child_iter = children.begin(); child_iter != children.end(); ++child_iter)
    {
        Parameter_list * child = child_iter->second;

        Parameter_list::const_iterator param_iter;

        for (param_iter = child->begin(); param_iter != child->end(); ++param_iter)
        {
            param_iter->second->set_callback(update_function);
        }

        set_children_callback_function(child->get_children(), update_function);
    }
}

void Parameter_list::append(const Parameter_list &parameter_list, std::function<void ()> update_function)
{
    Parameter_list::const_iterator iter;

    int const current_list_size = int(_list.size());

    for (iter = parameter_list.begin(); iter != parameter_list.end(); ++iter)
    {
        iter->second->set_order_index(iter->second->get_order_index() + current_list_size);
        add_parameter(iter->first, iter->second);
        iter->second->set_callback(update_function);
    }

    Child_map const& appended_children = parameter_list.get_children();
    Child_map::const_iterator child_iter;

    for (child_iter = appended_children.begin(); child_iter != appended_children.end(); ++child_iter)
    {
        assert(_children.find(child_iter->first) == _children.end());

        _children[child_iter->first] = new Parameter_list(*child_iter->second);
    }

    set_children_callback_function(_children, update_function);
}

void Parameter_list::print() const
{
    std::map<std::string, Parameter*>::const_iterator iter;

    for (iter = _list.begin(); iter != _list.end(); ++iter)
    {
        std::cout << iter->first << ": " << iter->second->to_string() << std::endl;
    }
}

void Parameter_list::save(const std::string &file_name) const
{
    std::cout << __FUNCTION__ << " " << file_name << std::endl;

    std::ofstream out_file(file_name.c_str(), std::ios_base::binary);
//    boost::archive::text_oarchive oa(out_file);

    if (out_file)
    {
        boost::archive::xml_oarchive oa(out_file);

        //        std::cout << "Parameter_list::save: " << out_file << std::endl;

        QFileInfo f_info(QString::fromStdString(file_name));
        oa << boost::serialization::make_nvp(f_info.fileName().toLocal8Bit(), *this);

        out_file.close();
    }
}

void Parameter_list::load(const std::string &file_name)
{
    std::ifstream in_file(file_name.c_str(), std::ios_base::binary);

    if (in_file)
    {
//        boost::archive::text_iarchive ia(in_file);
        boost::archive::xml_iarchive ia(in_file);

        Parameter_list tmp_param_list;
//        ia >> tmp_param_list;

        QFileInfo f_info(QString::fromStdString(file_name));
        ia >> boost::serialization::make_nvp(f_info.fileName().toLocal8Bit(), tmp_param_list);

        load(tmp_param_list);

        in_file.close();
    }
    else
    {
        std::cout << "Parameter file not found" << std::endl;
    }
}
