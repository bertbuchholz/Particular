#ifndef CORE_H
#define CORE_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <Registry_parameters.h>

#include "Atom.h"

class Core
{
public:
    Core()
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

        receiver._torque += 0.1f * Eigen::Vector3f(1.0f - 2.0f * rand() / float(RAND_MAX),
                                                   1.0f - 2.0f * rand() / float(RAND_MAX),
                                                   1.0f - 2.0f * rand() / float(RAND_MAX));
    }

    Eigen::Quaternion<float> scale(Eigen::Quaternion<float> const& quat, float const factor)
    {
        return Eigen::Quaternion<float>(quat.w() * factor, quat.x() * factor, quat.y() * factor, quat.z() * factor);
    }

    Eigen::Quaternion<float> add(Eigen::Quaternion<float> const& q_0, Eigen::Quaternion<float> const& q_1)
    {
        return Eigen::Quaternion<float>(q_0.w() + q_1.w(), q_0.x() + q_1.x(), q_0.y() + q_1.y(), q_0.z() + q_1.z());
    }

    void update(float const time_step)
    {
//        std::vector<Body_state> tmp_states;

        for (Molecule & m : _molecules)
        {
//            tmp_states.push_back(m.to_state());
            compute_force_and_torque(m);
        }

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
    }

    void add_barrier(Barrier * barrier)
    {
        _barriers.push_back(barrier);
    }

    std::vector<Force_indicator> const& get_force_indicators() const
    {
        return _indicators;
    }

    void clear()
    {
        _molecules.clear();
    }

    void set_parameters(Parameter_list const& parameters)
    {
        _use_indicators = parameters["use_indicators"]->get_value<bool>();
        _rotation_damping = parameters["rotation_damping"]->get_value<float>();
        _translation_damping = parameters["translation_damping"]->get_value<float>();
        _atomic_force = std::unique_ptr<Atomic_force>(Parameter_registry<Atomic_force>::get_class_from_single_select_instance_2(parameters.get_child("Atomic Force Type")));
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("use_indicators", true));
        parameters.add_parameter(new Parameter("rotation_damping", 0.1f, 0.0f, 10.0f));
        parameters.add_parameter(new Parameter("translation_damping", 0.1f, 0.0f, 10.0f));

        // FIXME: memory leak
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

    std::unique_ptr<Atomic_force> _atomic_force;

    float _translation_damping;
    float _rotation_damping;

};

REGISTER_BASE_CLASS_WITH_PARAMETERS(Core);

#endif // CORE_H
