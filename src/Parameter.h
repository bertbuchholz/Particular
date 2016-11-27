#ifndef PARAMETER_H
#define PARAMETER_H

#include <functional>
#include <unordered_map>
#include <cassert>
#include <map>
#include <iostream>
#include <istream>
#include <algorithm>
#include <fstream>
#include <sstream>



//#include <boost/lexical_cast.hpp>

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/variant.hpp>

#include <boost/variant.hpp>
#endif

class Notifiable
{
public:
    virtual ~Notifiable() {}

    virtual void notify() = 0;
};


class Parameter
{

public:
    enum class Type { Normal, List, Button };
    typedef boost::variant<bool, float, int, std::string> My_variant;

    Parameter() : _hidden(false) {}

    Parameter(std::string const& name, bool const& value, std::function<void()> callback_on_change = std::function<void()>()) :
        _name(name),
        _hidden(false),
        _callback_on_change(callback_on_change)
    {
        _value = value;

        _special_type = Type::Normal;
    }

    Parameter(std::string const& name, std::string const& value, std::function<void()> callback_on_change = std::function<void()>()) :
        _name(name),
        _hidden(false),
        _callback_on_change(callback_on_change)
    {
        _value = value;

        _special_type = Type::Normal;
    }

    template <class T>
    Parameter(std::string const& name, T const& value, T const& min, T const& max, std::function<void()> callback_on_change = std::function<void()>()) :
        _name(name),
        _hidden(false),
        _callback_on_change(callback_on_change)
    {
        _value = value;
        set_min_max(min, max);

        _special_type = Type::Normal;
    }

    template <class T>
    Parameter(std::string const& name, int const index, std::vector<T> const& value_list, std::function<void()> callback_on_change = std::function<void()>()) :
        _name(name),
        _hidden(false),
        _callback_on_change(callback_on_change)
    {
        assert(index < int(value_list.size()));

        for (size_t i = 0; i < value_list.size(); ++i)
        {
            My_variant variant = value_list[i];
            _value_list.push_back(variant);
        }

        _index = index;

        if (index >= 0)
        {
            _value = _value_list[_index];
        }

        _special_type = Type::List;

        // assert(boost::get<T>(_value_list_2[0]) == value_list[0]);
    }

    template <class T>
    Parameter(std::string const& name, int const index, std::map<std::string, T> const& value_list, std::function<void()> callback_on_change = std::function<void()>()) :
        _name(name),
        _hidden(false),
        _callback_on_change(callback_on_change)
    {
        assert(index < int(value_list.size()));

        typename std::map<std::string, T>::const_iterator iter;

        for (iter = value_list.begin(); iter != value_list.end(); ++iter)
        {
            My_variant variant = iter->first;
            _value_list.push_back(variant);
        }

        _index = index;
        _value = _value_list[_index];

        _special_type = Type::List;
    }

    void set_callback(std::function<void()> const& callback_function)
    {
        _callback_on_change = callback_function;
    }

    static Parameter * create_button(std::string const& name, std::function<void()> callback_on_change = std::function<void()>())
    {
        Parameter * p = new Parameter;
        p->_name = name;
        p->_special_type = Type::Button;
        p->_callback_on_change = callback_on_change;

        return p;
    }

    void do_callback() { if (_callback_on_change) _callback_on_change(); }

    void set_from_index(int index);

    void set_bool_from_int(int const value)
    {
        _value = bool(value);
        if (_callback_on_change) _callback_on_change();
        notify_observers();
    }

    template <class T>
    void set_value(T const& value)
    {
        set_value_no_update(value);

        if (_callback_on_change) _callback_on_change();
//        notify_observers();
    }

    template <class T>
    void set_value_no_update(T const& value)
    {
        if (_special_type == Type::List)
        {
            bool found = false;

            for (size_t i = 0; i < _value_list.size(); ++i)
            {
                try
                {
                    if (boost::get<T>(_value_list[i]) == value)
                    {
                        _index = int(i);
                        found = true;
                        _value = value;
                    }
                }
                catch (...)
                {
                    std::cout << __FUNCTION__ << " get failed on list parameter: " << _name << " value: " << value << std::endl;
                    std::cout << typeid(value).name() << " " << typeid(T).name() << std::endl;
                    std::cout << "Possible values:" << std::endl;

                    for (size_t j = 0; j < _value_list.size(); ++j)
                    {
                        std::cout << j << ": " << boost::get<T>(_value_list[j]) << std::endl;
                    }
                }
            }

            if (!found)
            {
                std::cout << __FUNCTION__ << "  << failed on list parameter: " << _name << " value: " << value << std::endl;
                std::cout << "Possible values:" << std::endl;

                for (size_t i = 0; i < _value_list.size(); ++i)
                {
                    std::cout << boost::get<T>(_value_list[i]) << std::endl;
                }

                assert(false);
            }
        }
        else if (_value_list.size() == 2) // when there is a min/max
        {
            // clamp value to min/max instead of asserting
            _value = std::min(get_max<T>(), std::max(value, get_min<T>()));
//            assert(value <= get_max<T>() && value >= get_min<T>());
        }
        else
        {
            _value = value;
        }

        notify_observers();
    }

    int get_index() const { return _index; }

    void set_index(int const index)
    {
        _index = index;
        notify_observers();
    }

    template <class T>
    T get_value() const
    {
        // std::cout << "Parameter::get_value(): " << _name << std::endl;

        try
        {
            if (_special_type == Type::List)
            {
                return boost::get<T>(_value_list[_index]);
            }
            else
            {
                return boost::get<T>(_value);
            }
        }
        catch (...)
        {
            std::cout << "Parameter::get_value(): Exception " << _name << std::endl;
            std::cout << "Requested type: " << typeid(T).name() << ", stored type: " << _value.type().name() << std::endl;
            throw;
        }
    }

    template <class T>
    void set_min_max(T const& min, T const& max)
    {
        _value_list.clear();
        _value_list.push_back(min);
        _value_list.push_back(max);

        notify_observers();
    }

    template <class T>
    T get_min() const
    {
        assert(_value_list.size() == 2);

        try
        {
            return boost::get<T>(_value_list.front());
        }
        catch (...)
        {
            std::cout << "Parameter::get_min(): Exception " << _name << std::endl;
            throw;
        }
    }

    template <class T>
    T get_max() const
    {
        assert(_value_list.size() == 2);

        try
        {
            return boost::get<T>(_value_list.back());
        }
        catch (...)
        {
            std::cout << "Parameter::get_max(): Exception " << _name << std::endl;
            throw;
        }
    }

    std::vector<My_variant> const& get_value_list() const
    {
        return _value_list;
    }

    bool has_list() const { return (_special_type == Type::List); }


    std::type_info const& get_type() const;

    void set_name(std::string const& name);
    void set_description(std::string const& description);

    std::string const& get_name() const;
    // std::string const& get_group() const;
    std::string const& get_description() const;

    My_variant const& get_variant() const
    {
        return _value;
    }

    void set_variant(My_variant const& variant)
    {
        _value = variant;

        if (_special_type != Type::Button)
        {
            if (_callback_on_change) _callback_on_change();
        }
        notify_observers();
    }

    void set_order_index(int const order_index)
    {
        _order_index = order_index;
    }

    int get_order_index() const
    {
        return _order_index;
    }

    Type get_special_type() const
    {
        return _special_type;
    }

    std::string to_string()
    {
        std::stringstream ss;

        ss << _name << ", type: " << _value.type().name();

        return ss.str();
    }

    bool is_hidden() const
    {
        return _hidden;
    }

    void set_hidden(bool const hidden)
    {
        _hidden = hidden;
    }


    void notify_observers()
    {
//        std::cout << __FUNCTION__ << " " << _name << " num observers: " << _observers.size() << std::endl;

        for (auto o : _observers)
        {
            o->notify();
        }
    }

    void add_observer(Notifiable * observer)
    {
        _observers.push_back(observer);
    }

    void remove_observer(Notifiable * observer)
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_name);
        ar & BOOST_SERIALIZATION_NVP(_description);

        ar & BOOST_SERIALIZATION_NVP(_value);

        ar & BOOST_SERIALIZATION_NVP(_index);

        ar & BOOST_SERIALIZATION_NVP(_value_list);
        ar & BOOST_SERIALIZATION_NVP(_special_type);

        ar & BOOST_SERIALIZATION_NVP(_order_index);
    }


private:
    std::string _name;
    std::string _description;

    My_variant _value;
    std::vector<My_variant> _value_list;
    Type _special_type;
    int _index;

    int _order_index;

    bool _hidden;

    std::function<void()> _callback_on_change;

    std::vector<Notifiable*> _observers;
};


class Sort_by_order_index
{
public:
    bool operator() (Parameter const* p_0, Parameter const* p_1) const
    {
        return (p_0->get_order_index() < p_1->get_order_index());
    }
};

class Parameter_list
{
public:
    // typedef std::unordered_map<std::string, Parameter_list*> Child_map;
    typedef std::map<std::string, Parameter_list*> Child_map;

    typedef std::map<std::string, Parameter*>::const_iterator const_iterator;
    typedef std::map<std::string, Parameter*>::iterator       iterator;

    enum Children_type { Normal, Single_select, Multi_select };

    Parameter_list() : _children_type(Normal)
    {}

    void add_parameter(std::string const& id, Parameter * new_parameter);
    void add_parameter(Parameter * new_parameter) { add_parameter(new_parameter->get_name(), new_parameter); }
    void add_parameter_keep_order(std::string const& id, Parameter * new_parameter);

    std::map<std::string, Parameter*>::iterator begin() { return _list.begin(); }
    std::map<std::string, Parameter*>::const_iterator begin() const { return _list.begin(); }
    std::map<std::string, Parameter*>::iterator end() { return _list.end(); }
    std::map<std::string, Parameter*>::const_iterator end() const { return _list.end(); }

    std::map<std::string, Parameter*>::iterator find(std::string const& name) { return _list.find(name); }

    size_t size() const { return _list.size(); }

    Parameter * get_parameter(std::string const& name) { return const_cast<Parameter*>(static_cast<const Parameter_list*>(this)->get_parameter(name)); }
    Parameter const* get_parameter(std::string const& name) const;

    Parameter const* operator[] (std::string const& name) const { return get_parameter(name); }
    Parameter * operator[] (std::string const& name) { return get_parameter(name); }

    std::vector<Parameter const*> get_ordered() const;

    Parameter_list * add_child(std::string const& name, Children_type const type = Normal);
    void add_child(std::string const& name, Parameter_list * list);

    Parameter_list const* get_child(std::string const& name) const;
    Parameter_list * get_child(std::string const& name) { return const_cast<Parameter_list*>(static_cast<const Parameter_list*>(this)->get_child(name)); }

    Child_map const& get_children() const { return _children; }
    Child_map & get_children() { return _children; }

    Children_type get_children_type() const { return _children_type; }
    void set_children_type(Children_type const t) { _children_type = t; }

    void set_children_callback_function(Child_map & children, std::function<void(void)> update_function);

    // possible FIXME: this will really only append the argument, it will not take copies
    // of the pointers contained in the maps (shallow copy)!
    void append(Parameter_list const& parameter_list, std::function<void(void)> update_function = std::function<void(void)>());

    void print() const;

    void save(std::string const& file_name) const;
    void load(std::string const& file_name);
    void load(Parameter_list const& tmp_param_list);
    // recursive loading, setting only values that already exist, not creating new values
    void load(std::map<std::string, Parameter*> & list, std::map<std::string, Parameter*> const& tmp_list, Child_map & children, Child_map const& tmp_children) const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_list);
        ar & BOOST_SERIALIZATION_NVP(_children);
        ar & BOOST_SERIALIZATION_NVP(_children_type);
    }

private:
    std::map<std::string, Parameter*> _list;
    Child_map _children;

    Children_type _children_type;
};


#endif // PARAMETER_H
