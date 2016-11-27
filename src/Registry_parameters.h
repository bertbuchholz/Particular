#ifndef REGISTRY_PARAMETERS_H
#define REGISTRY_PARAMETERS_H

#include "Registry.h"
#include "Parameter.h"


template<class T>
class Parameter_registry : public Registry<T>
{
public:
    static Parameter_registry * get()
    {
        if (!_instance)
        {
            _instance = new Parameter_registry<T>;
        }

        return _instance;
    }

    void add_parameters(std::string const& class_name, Parameter_list const& parameters)
    {
        if (_parameter_lists.find(class_name) == _parameter_lists.end())
        {
            _parameter_lists[class_name] = parameters;
        }
    }


    Parameter_list const& get_parameters(std::string const& class_name)
    {
        return _parameter_lists[class_name];
    }

    static void add_params_recursive(Parameter_list * parent, Parameter_list const& p_list, std::function<void(void)> update_function)
    {
//        Parameter_list * class_list = parent, Parameter_list * p_list->add_child(name);

        std::vector<Parameter const*> const parameters = p_list.get_ordered();
        for (std::size_t j = 0; j < parameters.size(); ++j)
        {
            Parameter * p = new Parameter(*parameters[j]);
            p->set_callback(update_function);
            parent->add_parameter(p->get_name(), p);
        }

        for (auto const& child : p_list.get_children())
        {
            Parameter_list * new_parent = parent->add_child(child.first, child.second->get_children_type());
            add_params_recursive(new_parent, *child.second, update_function);
        }
    }

//    static void create_normal_instance(std::string const& name, Parameter_list * result, std::function<void(void)> update_function)
//    {
//        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();

//        assert(class_names.size() == 1);

//        Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(name);

////        add_params_recursive(class_names[0], result, update_function);


//        Parameter_list * class_list = result->add_child(name);


//        std::vector<Parameter const*> const parameters = p_list.get_ordered();
//        for (std::size_t j = 0; j < parameters.size(); ++j)
//        {
//            Parameter * p = new Parameter(*parameters[j]);
//            p->set_callback(update_function);
//            class_list->add_parameter(p->get_name(), p);
//        }
//    }

    // for classes which are not part of an inheritance hierarchy
    static void create_normal_instance(std::string const& name, Parameter_list * result, std::function<void(void)> update_function = std::function<void(void)>())
    {
        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();

        assert(class_names.size() == 1);

        Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(name);

        Parameter_list * class_list = result->add_child(name);

        add_params_recursive(class_list, p_list, update_function);
    }


    static void create_multi_select_instance(Parameter_list * result, std::string const& instance_name, std::function<void(void)> update_function = std::function<void(void)>())
    {
        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();


        Parameter_list * instance_list = result->add_child(instance_name);
        instance_list->set_children_type(Parameter_list::Multi_select);

        for (std::size_t i = 0; i < class_names.size(); ++i)
        {
            Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(class_names[i]);

            Parameter_list * class_list = instance_list->add_child(class_names[i]);

            Parameter * enable = new Parameter("multi_enable", false);
            enable->set_callback(update_function);
            class_list->add_parameter(enable);

            std::vector<Parameter const*> const parameters = p_list.get_ordered();
            for (std::size_t j = 0; j < parameters.size(); ++j)
            {
                Parameter * p = new Parameter(*parameters[j]);
                p->set_callback(update_function);
                class_list->add_parameter(p->get_name(), p);
            }
        }
    }


    static void create_single_select_instance(Parameter_list * result, std::string const& instance_name, std::function<void(void)> update_function = std::function<void(void)>())
    {
        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();

        // result->add_parameter(new Parameter(instance_name, 0, class_names, update_function));

        Parameter_list * instance_list = result->add_child(instance_name);

        instance_list->add_parameter(new Parameter("type", 0, class_names, update_function));

        instance_list->set_children_type(Parameter_list::Single_select);

        for (std::size_t i = 0; i < class_names.size(); ++i)
        {
            Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(class_names[i]);

            Parameter_list * class_list = instance_list->add_child(class_names[i]);

            std::vector<Parameter const*> const parameters = p_list.get_ordered();
            for (std::size_t j = 0; j < parameters.size(); ++j)
            {
                Parameter * p = new Parameter(*parameters[j]);
                p->set_callback(update_function);
                class_list->add_parameter(p->get_name(), p);
            }
        }
    }

    static T* get_class_from_single_select_instance_2(Parameter_list const* parameters)
    {
        assert(parameters->get_children_type() == Parameter_list::Single_select);

        std::string const& class_name = parameters->get_parameter("type")->get_value<std::string>();

        T * result = Parameter_registry<T>::get()->create(class_name);
        result->set_parameters(*parameters->get_child(class_name));

        return result;
    }

    static std::vector<T*> get_classes_from_multi_select_instance(Parameter_list const* parameters)
    {
        /*
        std::string const& class_name = parameters->get_parameter("type")->get_value<std::string>();

        T * result = Parameter_registry<T>::get()->create(class_name);
        result->set_parameters(*parameters->get_child(class_name));

        return result;
        */

        std::vector<T*> result;

        // std::vector<Parameter const*> switchable_classes_parameters = parameters.extract_by_instance_and_name(instance_name, "class_switch");

        assert(parameters->get_children_type() == Parameter_list::Multi_select);

        Parameter_list::Child_map const& children = parameters->get_children();

        Parameter_list::Child_map::const_iterator iter = children.begin();

        for (iter = children.begin(); iter != children.end(); ++iter)
        {
            Parameter_list * p_list = iter->second;

            if (p_list->get_parameter("multi_enable")->get_value<bool>())
            {
                T * instance = Parameter_registry<T>::get()->create(iter->first);
                instance->set_parameters(*p_list);
                result.push_back(instance);
            }
        }

        return result;
    }


    static std::vector< std::unique_ptr<T> > get_unique_ptr_classes_from_multi_select_instance(Parameter_list const* parameters)
    {
        /*
        std::string const& class_name = parameters->get_parameter("type")->get_value<std::string>();

        T * result = Parameter_registry<T>::get()->create(class_name);
        result->set_parameters(*parameters->get_child(class_name));

        return result;
        */

        std::vector<std::unique_ptr<T>> result;

        // std::vector<Parameter const*> switchable_classes_parameters = parameters.extract_by_instance_and_name(instance_name, "class_switch");

        assert(parameters->get_children_type() == Parameter_list::Multi_select);

        Parameter_list::Child_map const& children = parameters->get_children();

        Parameter_list::Child_map::const_iterator iter = children.begin();

        for (iter = children.begin(); iter != children.end(); ++iter)
        {
            Parameter_list * p_list = iter->second;

            if (p_list->get_parameter("multi_enable")->get_value<bool>())
            {
                T * instance = Parameter_registry<T>::get()->create(iter->first);
                instance->update_variables(*p_list);
                result.push_back(std::unique_ptr<T>(instance));
            }
        }

        return result;
    }

//    static Parameter_list create_single_select_instance(std::string const& instance_name, std::function<void(void)> update_function = std::function<void(void)>())
//    {
//        Parameter_list result;

//        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();

//        result.add_parameter(new Parameter(instance_name, 0, class_names, update_function));

//        for (std::size_t i = 0; i < class_names.size(); ++i)
//        {
//            Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(class_names[i]);

//            std::vector<Parameter const*> const parameters = p_list.get_ordered();
//            for (std::size_t j = 0; j < parameters.size(); ++j)
//            {
//                Parameter * p = new Parameter(*parameters[j]);
//                p->set_class_name(class_names[i]);
//                p->set_instance_name(instance_name);
//                p->set_callback(update_function);
//                result.add_parameter(instance_name + "/" + class_names[i] + "/" + p->get_name(), p);
//            }
//        }

//        return result;
//    }

//    static T* get_class_from_single_select_instance(std::string const& instance_name, Parameter_list const& parameters)
//    {
//        std::string const& class_name = parameters[instance_name]->get_value<std::string>();

//        T * result = Parameter_registry<T>::get()->create(class_name);
//        result->set_parameters(parameters.extract_by_instance_and_class(instance_name, class_name));

//        return result;
//    }


    // FIXME: rebuild for child-way of parameter_list
//    static Parameter_list create_multi_select_instance(std::string const& instance_name, std::function<void(void)> update_function)
//    {
//        Parameter_list result;

//        std::vector<std::string> class_names = Parameter_registry<T>::get()->get_registered_names();

//        for (std::size_t i = 0; i < class_names.size(); ++i)
//        {
//            Parameter_list const& p_list = Parameter_registry<T>::get()->get_parameters(class_names[i]);

//            Parameter * switch_param = new Parameter("class_switch", false, update_function);
//            switch_param->set_class_name(class_names[i]);
//            switch_param->set_instance_name(instance_name);
//            result.add_parameter(instance_name + "/" + class_names[i] + "/class_switch", switch_param);

//            std::vector<Parameter const*> const parameters = p_list.get_ordered();
//            for (std::size_t j = 0; j < parameters.size(); ++j)
//            {
//                Parameter * p = new Parameter(*parameters[j]);
//                p->set_class_name(class_names[i]);
//                p->set_instance_name(instance_name);
//                p->set_callback(update_function);
//                result.add_parameter(instance_name + "/" + class_names[i] + "/" + p->get_name(), p);
//            }
//        }

//        return result;
//    }

//    static std::vector<T*> get_classes_from_multi_select_instance(std::string const& instance_name, Parameter_list const& parameters)
//    {
//        std::vector<T*> result;

//        std::vector<Parameter const*> switchable_classes_parameters = parameters.extract_by_instance_and_name(instance_name, "class_switch");

//        for (size_t i = 0; i < switchable_classes_parameters.size(); ++i)
//        {
//            if (switchable_classes_parameters[i]->get_value<bool>())
//            {
//                std::string const& class_name = switchable_classes_parameters[i]->get_class_name();

//                T * c = Parameter_registry<T>::get()->create(class_name);
//                c->set_parameters(parameters.extract_by_instance_and_class(instance_name, class_name));
//                result.push_back(c);
//            }
//        }

//        return result;
//    }


private:
    std::map<std::string, Parameter_list> _parameter_lists;
    static Parameter_registry<T> * _instance;
};

template <class T>
Parameter_registry<T> * Parameter_registry<T>::_instance = NULL;


template <class Base, class Derived>
class Class_parameter_registration
{
public:
    Class_parameter_registration(std::function<Base*(void)> const& creator, Parameter_list const& parameters)
    {
        std::string class_name = Derived::name();

        Parameter_registry<Base>::get()->register_class(class_name, creator);

        Parameter_list base_parameters = Base::get_parameters();
        Parameter_list extended_parameters = base_parameters;
        extended_parameters.append(parameters);

        Parameter_registry<Base>::get()->add_parameters(class_name, extended_parameters);
    }
};


#define REGISTER_CLASS_WITH_PARAMETERS(base_class, derived_class) \
    static Class_parameter_registration< base_class, derived_class > class_with_parameters_ ## derived_class(&derived_class::create, derived_class::get_parameters())

/*#define REGISTER_TEMPLATE_WITH_PARAMETERS(name, base_class, derived_class) \
    static Class_parameter_registration< base_class > class_with_parameters_ ## derived_class<>((name), &derived_class::create, derived_class::get_parameters())
    */

template <class Base>
class Base_class_parameter_registration
{
public:
    Base_class_parameter_registration(std::function<Base*(void)> const& creator, Parameter_list const& parameters)
    {
        std::string class_name = Base::name();

        Parameter_registry<Base>::get()->register_class(class_name, creator);

        Parameter_registry<Base>::get()->add_parameters(class_name, parameters);
    }
};

#define REGISTER_BASE_CLASS_WITH_PARAMETERS(base_class) \
    static Base_class_parameter_registration< base_class > class_with_parameters_ ## base_class(&base_class::create, base_class::get_parameters())


#endif
