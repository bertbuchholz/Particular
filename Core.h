#ifndef CORE_H
#define CORE_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/optional.hpp>

#include <Registry_parameters.h>

#include "Atom.h"

class Core
{
public:
    Core() : _current_time(0.0f)
    {
        Eigen::Vector2f grid_start(-10.0f, -10.0f);
        Eigen::Vector2f grid_end  ( 10.0f,  10.0f);

        float resolution = 0.5f;

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
            {
                _indicators.push_back(Force_indicator(Eigen::Vector3f(x, y, 0.0f)));
            }
        }
    }

    Eigen::Vector3f force_on_atom(Atom const& receiver_atom, int const parent_molecule_id = -1)
    {        
        Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

        for (Molecule const& sender : _molecules)
        {
            if (parent_molecule_id == sender._id) continue;

            for (Atom const& sender_atom : sender._atoms)
            {
                if ((receiver_atom._r - sender_atom._r).norm() > 1e-4f)
                {
//                    force_i += calc_force_between_atoms(receiver_atom, sender_atom);
                    force_i += _atomic_force->calc_force_between_atoms(receiver_atom, sender_atom);
                }
            }
        }

        for (Barrier const* b : _barriers)
        {
            force_i += b->calc_force(receiver_atom);
        }

        return force_i;
    }

    void compute_force_and_torque(Molecule & receiver)
    {
        receiver._force.setZero();
        receiver._torque.setZero();

        for (Atom const& receiver_atom : receiver._atoms)
        {
            Eigen::Vector3f force_i = force_on_atom(receiver_atom, receiver._id);

            receiver._force += force_i;
            receiver._torque += (receiver_atom._r - receiver._x).cross(force_i);
        }

        for (Molecule const& sender : _molecules)
        {
            if (receiver._id == sender._id) continue;

            float const lj_potential = calc_lennard_jones_potential(receiver, sender);
            receiver._force += lj_potential * (receiver._x - sender._x).normalized();
        }

        receiver._force += -_translation_damping * receiver._v;
        receiver._torque += -_rotation_damping * receiver._omega;

        receiver._torque += _rotation_fluctuation * Eigen::Vector3f(1.0f - 2.0f * rand() / float(RAND_MAX),
                                                                    1.0f - 2.0f * rand() / float(RAND_MAX),
                                                                    1.0f - 2.0f * rand() / float(RAND_MAX));

        receiver._force += _translation_fluctuation * Eigen::Vector3f(1.0f - 2.0f * rand() / float(RAND_MAX),
                                                                      1.0f - 2.0f * rand() / float(RAND_MAX),
                                                                      1.0f - 2.0f * rand() / float(RAND_MAX));

        // TODO: do the search for affected molecules somewhat less brute force
        for (Molecule_external_force const& f : _external_forces)
        {
            if (receiver._id == f._molecule_id)
            {
                receiver._force += f._force;
                receiver._torque += (f._origin - receiver._x).cross(f._force);
            }
        }

        if (_user_force._end_time > _current_time && receiver._id == _user_force._molecule_id)
        {
            receiver._force += _user_force._force;
            receiver._torque += (_user_force._origin - receiver._x).cross(_user_force._force);
        }
    }

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


    void update(float const time_step)
    {
        _current_time += time_step;

//        std::vector<Body_state> tmp_states;

        for (Molecule & m : _molecules)
        {
//            tmp_states.push_back(m.to_state());
            compute_force_and_torque(m);
        }

        _external_forces.erase(std::remove_if(_external_forces.begin(), _external_forces.end(), Check_duration(_current_time)),
                        _external_forces.end());

        if (_use_indicators)
        {
            for (Force_indicator & f : _indicators)
            {
                f._force = force_on_atom(f._atom);
            }
        }

        for (size_t i = 0; i < _molecules.size(); ++i)
        {
//            Body_state & state = tmp_states[i];
            Molecule & molecule = _molecules[i];

//            state._x += molecule._v * time_step;

//            Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
//            Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
//            state._q = add(state._q, scale(q_dot, time_step));

//            state._P += molecule._force * time_step;

//            state._L += molecule._torque * time_step;

//            molecule.from_state(state);

            molecule._x += molecule._v * time_step;

            Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
            Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
            molecule._q = add(molecule._q, scale(q_dot, time_step));

            molecule._P += molecule._force * time_step;

            molecule._L += molecule._torque * time_step;

            molecule.from_state(Body_state());
        }
    }

    std::vector<Molecule> const& get_molecules() const
    {
        return _molecules;
    }

    void add_molecule(Molecule const& molecule)
    {
        _molecules.push_back(molecule);
        _molecules.back()._id = _molecules.size();
        _molecules.back()._mass_factor = _mass_factor;
    }

    void add_barrier(Barrier * barrier)
    {
        _barriers.push_back(barrier);
    }

    std::vector<Force_indicator> const& get_force_indicators() const
    {
        return _indicators;
    }

    void add_external_force(Molecule_external_force const& force)
    {
        _external_forces.push_back(force);
    }

    float get_current_time() const
    {
        return _current_time;
    }

    Molecule_external_force & get_user_force()
    {
        return _user_force;
    }

    boost::optional<Molecule const&> get_molecule(int const id)
    {
        // TODO: stupid brute force search, maybe better structure
        for (Molecule const& m : _molecules)
        {
            if (m._id == id)
            {
                return boost::optional<Molecule const&>(m);
            }
        }

        return boost::optional<Molecule const&>();
    }

    void clear()
    {
        _molecules.clear();
        _barriers.clear();
        _external_forces.clear();
    }

    void set_parameters(Parameter_list const& parameters)
    {
        _use_indicators = parameters["use_indicators"]->get_value<bool>();
        _rotation_damping = parameters["rotation_damping"]->get_value<float>();
        _translation_damping = parameters["translation_damping"]->get_value<float>();
        _rotation_fluctuation = parameters["rotation_fluctuation"]->get_value<float>();
        _translation_fluctuation = parameters["translation_fluctuation"]->get_value<float>();

        _mass_factor = parameters["mass_factor"]->get_value<float>();

        for (Molecule & m : _molecules)
        {
            m._mass_factor = _mass_factor;
        }

        _atomic_force = std::unique_ptr<Atomic_force>(Parameter_registry<Atomic_force>::get_class_from_single_select_instance_2(parameters.get_child("Atomic Force Type")));
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("use_indicators", true));
        parameters.add_parameter(new Parameter("rotation_damping", 0.1f, 0.0f, 10.0f));
        parameters.add_parameter(new Parameter("translation_damping", 0.1f, 0.0f, 10.0f));
        parameters.add_parameter(new Parameter("rotation_fluctuation", 0.1f, 0.0f, 100.0f));
        parameters.add_parameter(new Parameter("translation_fluctuation", 0.1f, 0.0f, 100.0f));
        parameters.add_parameter(new Parameter("mass_factor", 1.0f, 0.01f, 10.0f));

        Parameter_registry<Atomic_force>::create_single_select_instance(&parameters, "Atomic Force Type");

        return parameters;
    }

    static Core * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Core";
    }

private:
    std::vector<Molecule> _molecules;

    bool _use_indicators;
    std::vector<Force_indicator> _indicators;

    std::vector<Barrier*> _barriers;

    std::vector<Molecule_external_force> _external_forces;

    Molecule_external_force _user_force;

    std::unique_ptr<Atomic_force> _atomic_force;

    float _translation_damping;
    float _rotation_damping;

    float _rotation_fluctuation;
    float _translation_fluctuation;

    float _mass_factor;

    float _current_time;
};

REGISTER_BASE_CLASS_WITH_PARAMETERS(Core);

#endif // CORE_H
