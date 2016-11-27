#ifndef REGISTRY_H
#define REGISTRY_H

#include <functional>
#include <map>
#include <vector>


template<class T>
class Registry
{
public:
    static Registry * get()
    {
        if (!_instance)
        {
            _instance = new Registry<T>;
        }

        return _instance;
    }

    void register_class(std::string const& class_name, std::function<T*(void)> const& creator)
    {
        if (_creators.find(class_name) == _creators.end())
        {
            _creators[class_name] = creator;
        }
    }

    T * create(std::string const& class_name)
    {
        if (_creators.find(class_name) != _creators.end())
        {
            return _creators[class_name]();
        }

        return NULL;
    }

    std::vector<std::string> get_registered_names() const
    {
        std::vector<std::string> names;

        typename std::map<std::string, std::function<T*(void)> >::const_iterator iter;

        for (iter = _creators.begin(); iter != _creators.end(); ++iter)
        {
            names.push_back(iter->first);
        }

        return names;
    }

private:
    std::map<std::string, std::function<T*(void)> > _creators;
    static Registry<T> * _instance;
};

template <class T>
Registry<T> * Registry<T>::_instance = NULL;


template <class T>
class Base_registration
{
public:
    Base_registration(std::string const& class_name, std::function<T*(void)> const& creator)
    {
        Registry<T>::get()->register_class(class_name, creator);
    }
};


#define REGISTER_CLASS(name, base_class, derived_class) \
    static Base_registration<base_class> class_ ## derived_class((name), &derived_class::create)


#endif
