#ifndef CORE_H
#define CORE_H

#include <QObject>

#include <vector>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

#ifndef Q_MOC_RUN
#include <boost/serialization/map.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#endif

#include <Registry_parameters.h>

#include "unique_ptr_serialization.h"
#include "GPU_force.h"
#include "Atom.h"
#include "Atomic_force.h"
#include "Level_element.h"
#include "Level_data.h"
#include "End_condition.h"
#include "Sensor_data.h"
#include "Progress.h"
#include "Main_game_screen.h"
#include "Random_generator.h"

void update_temperature_grid(Level_data const& level_data, Frame_buffer<float> & grid);

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

    Core();

    ~Core();

    Eigen::Vector3f apply_forces_brute_force(Atom const& receiver_atom) const;
    Eigen::Vector3f apply_forces_from_vector(Atom const& receiver_atom, std::vector<Atom const*> const& atoms) const;

    Eigen::Vector3f force_on_atom(Atom const& receiver_atom) const;
    void compute_force_and_torque(Molecule & receiver, int & atom_index, std::vector<Eigen::Vector3f> const& forces_on_atoms);

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
    void update_level_elements(const float time_step);
    void update_physics_elements(const float time_step);

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

    void add_molecule(Molecule const& molecule);

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


    void add_molecule_external_force(Molecule_external_force const& force);
//    void add_external_force(std::string const& name, External_force const& force);

    float get_current_time() const;

    void gl_init(QGLContext *);

    Molecule_external_force & get_user_force();
    Molecule_external_force const& get_user_force() const;

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

    void save_simulation_settings();
    void load_simulation_settings();
    void load_default_simulation_settings();

    void save_level(std::string const& file_name) const;
    void load_level(std::string const& file_name);
    void load_next_level();
    void change_level_state(Main_game_screen::Level_state const new_level_state);
    std::string const& get_current_level_name() const;
    QStringList const& get_level_names() const;

    void init_game();

    Progress & get_progress() { return _progress; }
    void save_progress();
    void load_progress();

    void toggle_simulation();
    void set_simulation_state(bool const s);
    bool get_simulation_state() const;
    void update_physics_timestep();

//    void set_parameters(Parameter_list const& parameters);
//    QWidget * get_parameter_widget() const;
    Parameter_list & get_parameters() { return _parameters; }
    Parameter_list const& get_parameters() const { return _parameters; }

    void update_variables();
    void update_parameters();

    void load_level_defaults();

    void quit();

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
    }

    void do_physics_step(std::list<Molecule> &molecules, const float current_time, const float time_step);
    void midpoint_integration(std::list<Molecule> &molecules, const float time_step);

public Q_SLOTS:
    void update_physics();

Q_SIGNALS:
    void game_state_changed();
    void level_changed(Main_game_screen::Level_state);

private:
    Level_data _level_data;

    Game_state _game_state;
    Game_state _previous_game_state;

//    std::vector<Force_indicator> _indicators;

    std::vector<Molecule_external_force> _molecule_external_forces;

    std::unordered_map<int, Molecule*> _molecule_id_to_molecule_map;

    int _molecule_id_counter;
    int _num_atoms;

    Molecule_external_force _user_force;

    std::vector< std::unique_ptr<Atomic_force> > _atomic_forces;

    float _mass_factor;

    float _current_time;
    float _last_sensor_check;

    Sensor_data _sensor_data;

    bool _do_constrain_forces;
    float _max_force;

    float _max_force_distance;

    Parameter_list _parameters;

    Progress _progress;

    QTimer _physics_timer;
    std::chrono::steady_clock::time_point _physics_elapsed_time;

    float _animation_interval;
    float _last_animation_time;

    QStringList _level_names;
    std::string _current_level_name;

    std::unique_ptr<GPU_force> _gpu_force;

    Random_generator _random_generator;
};

//REGISTER_BASE_CLASS_WITH_PARAMETERS(Core);

BOOST_CLASS_VERSION(Core, 2)

#endif // CORE_H
