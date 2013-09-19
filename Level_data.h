#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <boost/serialization/shared_ptr.hpp>

#include <Registry_parameters.h>

#include "Atom.h"
#include "Level_element.h"
#include "Molecule_releaser.h"
#include "End_condition.h"


template <typename T>
std::ostream & operator<< (std::ostream & s, std::vector<T> const& v)
{
    std::ostream_iterator<T> out_it (s, ", ");
    std::copy(v.begin(), v.end(), out_it);
    return s;
}

template <typename T>
std::ostream & operator<< (std::ostream & s, std::vector< boost::shared_ptr<T> > const& v)
{
    for (auto const& iter : v)
    {
        s << iter.get() << " ";
    }

    return s;
}

class Level_data
{
public:
    enum class Plane { Neg_X = 0, Neg_Y, Neg_Z, Pos_X, Pos_Y, Pos_Z };

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(_game_field_borders);
        ar & BOOST_SERIALIZATION_NVP(_barriers);
        ar & BOOST_SERIALIZATION_NVP(_portals);
        ar & BOOST_SERIALIZATION_NVP(_molecule_releasers);
        ar & BOOST_SERIALIZATION_NVP(_brownian_elements);
        ar & BOOST_SERIALIZATION_NVP(_level_elements);

        ar & BOOST_SERIALIZATION_NVP(_available_elements);
        ar & BOOST_SERIALIZATION_NVP(_score_time_factor);

        if (version > 0)
        {
            ar & BOOST_SERIALIZATION_NVP(_background_name);
        }
    }

    bool validate_elements()
    {
        std::cout << __PRETTY_FUNCTION__ << " "
                  << _level_elements.size() << " "
                  << _barriers.size() << " "
                  << _portals.size() << " "
                  << _molecule_releasers.size() << " "
                  << _brownian_elements.size()
                  << std::endl;

        std::cout << "LE: " << _level_elements << "\n"
                  << "BA: " << _barriers << "\n"
                  << "PO: " << _portals << "\n"
                  << "MR: " << _molecule_releasers << "\n"
                  << "BE: " << _brownian_elements << "\n";

        for (auto const& e : _level_elements)
        {
            Level_element const* e_ptr = e.get();

            if (std::find(_barriers.begin(), _barriers.end(), e_ptr) == _barriers.end() &&
                    std::find(_portals.begin(), _portals.end(), e_ptr) == _portals.end() &&
                    std::find(_molecule_releasers.begin(), _molecule_releasers.end(), e_ptr) == _molecule_releasers.end() &&
                    std::find(_brownian_elements.begin(), _brownian_elements.end(), e_ptr) == _brownian_elements.end()
                    )
            {
                assert(false);
                return false;
            }

        }

        for (auto const& iter : _game_field_borders)
        {
            Barrier const* b = iter.second;

            if (std::find(_barriers.begin(), _barriers.end(), b) == _barriers.end())
            {
                assert(false);
                return false;
            }
        }

        return true;
    }

    void set_parameters(Parameter_list const& parameters)
    {
        _score_time_factor = parameters["score_time_factor"]->get_value<float>();
        _background_name = parameters["background_name"]->get_value<std::string>();

        Parameter_list const* available_list = parameters.get_child("Available elements");

        for (auto const& iter : *available_list)
        {
            _available_elements[iter.first] = iter.second->get_value<int>();
        }
    }

    Parameter_list get_current_parameters()
    {
        Parameter_list result = get_parameters();

        result["score_time_factor"]->set_value_no_update(_score_time_factor);
        result["background_name"]->set_value_no_update(_background_name);

        Parameter_list * available_list = result.get_child("Available elements");

        for (auto & iter : *available_list)
        {
            iter.second->set_value(_available_elements[iter.first]);
        }

        return result;
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("score_time_factor", 60.0f, 1.0f, 3000.0f));

        parameters.add_parameter(new Parameter("background_name", std::string("iss_interior_1.png")));

        Parameter_list * available_list = parameters.add_child("Available elements");
        available_list->add_parameter(new Parameter("Box_barrier", 0, 0, 10));
        available_list->add_parameter(new Parameter("Brownian_box", 0, 0, 10));
        available_list->add_parameter(new Parameter("Box_portal", 0, 0, 10));
        available_list->add_parameter(new Parameter("Molecule_releaser", 0, 0, 10));
        available_list->add_parameter(new Parameter("Charged_barrier", 0, 0, 10));
        available_list->add_parameter(new Parameter("Tractor_barrier", 0, 0, 10));

        return parameters;
    }

    static Level_data * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Level_data";
    }


    std::list<Molecule> _molecules;

    std::map<Plane, Plane_barrier*> _game_field_borders;

    std::vector<Barrier*> _barriers;
    std::vector<Portal*> _portals;
    std::vector<Brownian_element*> _brownian_elements;
    std::vector<Molecule_releaser*> _molecule_releasers;
    std::vector<Particle_system_element*> _particle_system_elements;

    std::vector< boost::shared_ptr<Level_element> > _level_elements;

    std::map<std::string, int> _available_elements;

    float _score_time_factor;

    std::string _background_name;
};

REGISTER_BASE_CLASS_WITH_PARAMETERS(Level_data);

BOOST_CLASS_VERSION(Level_data, 1)

#endif // LEVEL_DATA_H
