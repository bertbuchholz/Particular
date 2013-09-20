#ifndef CORE_H
#define CORE_H

#include <QObject>

#include <vector>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

//#include <boost/optional.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <Registry_parameters.h>

#include "unique_ptr_serialization.h"
#include "GPU_force.h"
#include "Atom.h"
#include "Atomic_force.h"
#include "Spatial_hash.h"
#include "RegularBspTree.h"
#include "Level_element.h"
#include "Level_data.h"
#include "End_condition.h"
#include "Sensor_data.h"
#include "ANN_wrapper_functions.h"




class Core : public QObject
{
    Q_OBJECT
public:

    enum class Game_state { Unstarted, Running, Finished };

    struct Molecule_atom_id
    {
        Molecule_atom_id(int const m, int const a) : m_id(m), a_id(a)
        { }

        int m_id;
        int a_id;
    };

    typedef Spatial_hash<Eigen::Vector3f, Molecule_atom_id> Molecule_atom_hash;
    typedef Regular_bsp_tree<Eigen::Vector3f, 3, Atom> My_tree;

    Core();

    Eigen::Vector3f apply_forces_brute_force(Atom const& receiver_atom) const;
    Eigen::Vector3f apply_forces_using_tree(Atom const& receiver_atom) const;
    Eigen::Vector3f apply_forces_from_vector(Atom const& receiver_atom, std::vector<Atom const*> const& atoms) const;
    Eigen::Vector3f apply_forces_using_ann_tree(const Atom &receiver_atom) const;

    std::vector<Atom const*> get_atoms_from_tree(Atom const& receiver_atom) const;

    Eigen::Vector3f force_on_atom(Atom const& receiver_atom) const;
    void compute_force_and_torque(Molecule & receiver);

    Eigen::Quaternion<float> scale(Eigen::Quaternion<float> const& quat, float const factor)
    {
        return Eigen::Quaternion<float>(quat.w() * factor, quat.x() * factor, quat.y() * factor, quat.z() * factor);
    }

    Eigen::Quaternion<float> add(Eigen::Quaternion<float> const& q_0, Eigen::Quaternion<float> const& q_1)
    {
        return Eigen::Quaternion<float>(q_0.w() + q_1.w(), q_0.x() + q_1.x(), q_0.y() + q_1.y(), q_0.z() + q_1.z());
    }

    struct Check_duration
    {
        Check_duration(float const end_time) : _end_time(end_time) {}

        bool operator() (Molecule_external_force const& f) const
        {
            return (f._end_time < _end_time);
        }

        float _end_time;
    };

    void check_molecules_in_portals();

    void update(float const time_step);

    void do_sensor_check();

    bool check_is_finished() const;

    std::list<Molecule> const& get_molecules() const
    {
        return _level_data._molecules;
    }

    std::list<Molecule> & get_molecules()
    {
        return _level_data._molecules;
    }

    Level_data const& get_level_data() const
    {
        return _level_data;
    }

    Level_data & get_level_data()
    {
        return _level_data;
    }

//    Molecule_atom_hash const& get_molecule_hash() const
//    {
//        return _molecule_hash;
//    }

    My_tree const& get_tree() const
    {
        return _tree;
    }

    void add_molecule(Molecule molecule);

//    void update_spatial_hash()
//    {
//        _molecule_hash.clear();

//        for (Molecule const& m : _level_data._molecules)
//        {
//            for (size_t i = 0; i < m._atoms.size(); ++i)
//            {
//                _molecule_hash.add_point(m._atoms[i]._r, Molecule_atom_id(m.get_id(), i));
//            }
//        }
//    }

    struct Atom_averager
    {
        Atom operator() (std::vector<Atom const*> const& atoms) const
        {
            Atom result;

            result._type = Atom::Type::H;

            result.set_position(Eigen::Vector3f::Zero());

            result._mass = 0.0f;
            result._charge = 0.0f;
            result._radius = 0.0f;

            float charge_abssum = 0.0f;

            for (Atom const* a : atoms)
            {
                float abs_charge = std::abs(a->_charge);
                abs_charge = abs_charge > 0.001f ? abs_charge : 1.0f;

                result.set_position(result.get_position() + a->get_position() * abs_charge); // weigh the position by the charge
                charge_abssum += abs_charge;

                result._mass += a->_mass;
                result._charge += a->_charge;
                result._radius += a->_radius * a->_radius * a->_radius;
            }

            if (charge_abssum < 0.001f)
            {
                std::cout << "charge_abssum " << charge_abssum << " #atoms " << atoms.size() << std::endl;
                assert(false);
            }

            result.set_position(result.get_position() / charge_abssum);

            result._radius = std::pow(result._radius, 0.3333f);

            // not averaging, accumulating!
//            result._mass /= atoms.size();
//            result._charge /= atoms.size();
//            result._radius /= atoms.size();

            return result;
        }
    };

    void update_tree();

    void add_barrier(Barrier * barrier);
    void add_brownian_element(Brownian_element * element);
    void add_portal(Portal * portal);
    void add_molecule_releaser(Molecule_releaser * molecule_releaser);
    void add_molecule_external_force(Molecule_external_force const& force);
    void add_external_force(std::string const& name, External_force const& force);

    std::vector<Force_indicator> const& get_force_indicators() const;

    float get_current_time() const;

    Molecule_external_force & get_user_force();
    Molecule_external_force const& get_user_force() const;

    std::vector<Barrier*> const& get_barriers() const;
    std::vector<Portal*> const& get_portals() const;
    std::vector<Molecule_releaser*> const& get_molecule_releasers() const;
    std::vector<Brownian_element*> const& get_brownian_elements() const;
    boost::optional<Molecule const&> get_molecule(int const id) const;

    void set_game_field_borders(Eigen::Vector3f const& min, Eigen::Vector3f const& max);

    void delete_level_element(Level_element * level_element);
    void reset_level_elements();

    Eigen::Vector3f calc_forces_between_atoms(Atom const& a_0, Atom const& a_1) const;

    void start_level();
    void set_new_game_state(Game_state state);

    Game_state get_game_state() const;
    Game_state get_previous_game_state() const;

    Sensor_data const& get_sensor_data() const;

    static bool is_not_persistent(Level_element const* e);
    static bool is_not_persistent_boost_shared_ptr(boost::shared_ptr<Level_element> const& e);
    void delete_non_persistent_objects();

    void clear();
    void reset_level();

    void save_state(std::string const& file_name) const;
    void load_state(std::string const& file_name);

    void save_level(std::string const& file_name) const;
    void load_level(std::string const& file_name);

    void set_parameters(Parameter_list const& parameters);
    void update_parameter_list(Parameter_list & parameters) const;
    static Parameter_list get_parameters();

    static Core * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Core";
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(_level_data);

        if (version > 0)
        {
            ar & BOOST_SERIALIZATION_NVP(_translation_damping);
            ar & BOOST_SERIALIZATION_NVP(_rotation_damping);

            ar & BOOST_SERIALIZATION_NVP(_rotation_fluctuation);
            ar & BOOST_SERIALIZATION_NVP(_translation_fluctuation);

            ar & BOOST_SERIALIZATION_NVP(_external_forces);
        }
    }

signals:
    void game_state_changed();

private:
    Level_data _level_data;

    Game_state _game_state;
    Game_state _previous_game_state;

    bool _use_indicators;
    std::vector<Force_indicator> _indicators;

    std::map<std::string, External_force> _external_forces;

    std::vector<Molecule_external_force> _molecule_external_forces;

    std::unordered_map<int, Molecule*> _molecule_id_to_molecule_map;

    int _molecule_id_counter;

    Molecule_external_force _user_force;

//    std::unique_ptr<Atomic_force> _atomic_force;
    std::vector<Atomic_force*> _atomic_forces;

    float _translation_damping;
    float _rotation_damping;

    float _rotation_fluctuation;
    float _translation_fluctuation;

    float _mass_factor;

    float _current_time;
    float _last_sensor_check;

    Sensor_data _sensor_data;

    bool _do_constrain_forces;
    float _max_force;

    float _max_force_distance;

//    Molecule_atom_hash _molecule_hash;
    My_tree _tree;

    ANN_wrapper _ann_wrapper;

    mutable int _debug_leaf_usage_count;
    mutable int _debug_inner_node_usage_count;
};

REGISTER_BASE_CLASS_WITH_PARAMETERS(Core);

BOOST_CLASS_VERSION(Core, 1);

#endif // CORE_H
