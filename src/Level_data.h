#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#ifndef Q_MOC_RUN
#include <boost/serialization/shared_ptr.hpp>
#endif

#include "Registry_parameters.h"
#include "Atom.h"
#include "Level_element.h"
#include "Molecule_releaser.h"
#include "unique_ptr_serialization.h"

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

    Level_data();

    bool validate_elements();

    void load_defaults();

    void update_variables();
    void update_parameters();

    float get_temperature(Eigen::Vector3f const& world_pos) const;

    void add_molecule_releaser(Molecule_releaser *molecule_releaser);
    void add_portal(Portal *portal);
    void add_brownian_element(Brownian_element *element);
    void add_barrier(Barrier *barrier);
    void change_game_field_borders();
    void set_game_field_borders(Eigen::Vector3f const& min, Eigen::Vector3f const& max);

    void delete_level_element(Level_element *level_element);
    void reset_level_elements();

    void use_unstable_options();

    static Level_data * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Level_data";
    }

    template<class Archive>
    void save(Archive & ar, const unsigned int /* version */) const
    {
        ar & BOOST_SERIALIZATION_NVP(_game_field_borders);
        ar & BOOST_SERIALIZATION_NVP(_barriers);
        ar & BOOST_SERIALIZATION_NVP(_portals);
        ar & BOOST_SERIALIZATION_NVP(_molecule_releasers);
        ar & BOOST_SERIALIZATION_NVP(_brownian_elements);
        ar & BOOST_SERIALIZATION_NVP(_level_elements);

        ar & BOOST_SERIALIZATION_NVP(_available_elements);
        ar & BOOST_SERIALIZATION_NVP(_score_time_factor);

        ar & BOOST_SERIALIZATION_NVP(_background_name);

        ar & BOOST_SERIALIZATION_NVP(_translation_damping);
        ar & BOOST_SERIALIZATION_NVP(_rotation_damping);

        ar & BOOST_SERIALIZATION_NVP(_rotation_fluctuation);
        ar & BOOST_SERIALIZATION_NVP(_translation_fluctuation);
        ar & BOOST_SERIALIZATION_NVP(_external_forces);

        ar & BOOST_SERIALIZATION_NVP(_molecules);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
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

        if (version > 1)
        {
            // average damping from rotation and translation:
            // this might break existing levels if they rely on a certain amount of damping (mostly translation, probably)
            float rot_damping;
            float trans_damping;

            ar & boost::serialization::make_nvp("_translation_damping", trans_damping);
            ar & boost::serialization::make_nvp("_rotation_damping", rot_damping);
//            ar & BOOST_SERIALIZATION_NVP(rot_damping);
//            ar & BOOST_SERIALIZATION_NVP(trans_damping);

            _translation_damping = (rot_damping + trans_damping) * 0.5f;
            _rotation_damping = _translation_damping;

            ar & BOOST_SERIALIZATION_NVP(_rotation_fluctuation);
            ar & BOOST_SERIALIZATION_NVP(_translation_fluctuation);
        }

        if (version == 3)
        {
            float _gravity;
            ar & BOOST_SERIALIZATION_NVP(_gravity);
            _external_forces["gravity"]._force[2] = -_gravity;
        }

        if (version > 3)
        {
            ar & BOOST_SERIALIZATION_NVP(_external_forces);
        }

        if (version > 4)
        {
            // Core contains several variables () that contain the number of molecules, atoms and even pointers
            // to molecules, which are not restored here. probably need to be handled when loading a level!
            // Solved by adding the option to keep molecules in Core::reset_level, won't keep their external forces however.
            ar & BOOST_SERIALIZATION_NVP(_molecules);
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

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

    float _translation_damping;
    float _rotation_damping;

    float _rotation_fluctuation;
    float _translation_fluctuation;

    float _game_field_width;
    float _game_field_height;

    Frame_buffer<float> _temperature_grid;

//    float _gravity;

    std::map<std::string, External_force> _external_forces;

    Parameter_list _parameters;
};

//REGISTER_BASE_CLASS_WITH_PARAMETERS(Level_data);

BOOST_CLASS_VERSION(Level_data, 5)

#endif // LEVEL_DATA_H
