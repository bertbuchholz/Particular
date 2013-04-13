#ifndef CORE_H
#define CORE_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "Atom.h"

class Core
{
public:
    void compute_force_and_torque(Molecule & receiver)
    {
        receiver._force.setZero();
        receiver._torque.setZero();

        for (Atom const& receiver_atom : receiver._atoms)
        {
            Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

            for (Molecule const& sender : _molecules)
            {
                for (Atom const& sender_atom : sender._atoms)
                {
                    if ((receiver_atom._r - sender_atom._r).norm() > 1e-4f)
                    {
                        force_i += calc_force_between_atoms(receiver_atom, sender_atom);
                    }
                }
            }

            receiver._force += force_i;
            receiver._torque += (receiver_atom._r - receiver._x).cross(force_i);
        }
    }

    Eigen::Quaternion<float> scale(Eigen::Quaternion<float> const& quat, float const factor)
    {
        return Eigen::Quaternion<float>(quat.w() * factor, quat.x() * factor, quat.y() * factor, quat.z() * factor);
    }

    Eigen::Quaternion<float> add(Eigen::Quaternion<float> const& q_0, Eigen::Quaternion<float> const& q_1)
    {
        return Eigen::Quaternion<float>(q_0.w() * q_1.w(), q_0.x() * q_1.x(), q_0.y() * q_1.y(), q_0.z() * q_1.z());
    }

    void update(float const time_step)
    {
//        std::vector<Atom> tmp_atoms = _atoms;

//        for (Atom & atom : _atoms)
//        {
//            Vec force_sum(0.0f, 0.0f, 0.0f);

//            for (Atom const& effector : tmp_atoms)
//            {
//                Vec force = calc_force_between_atoms(atom, effector);
//                force_sum += force;
//            }

//            Vec acceleration = force_sum / atom._mass;

//            atom._speed += acceleration * time_step;
//            atom._position += atom._speed * time_step;

//            atom._speed *= 0.9f; // arbitrary damping
//        }

        std::vector<Body_state> tmp_states;

        for (Molecule & m : _molecules)
        {
            tmp_states.push_back(m.to_state());
            compute_force_and_torque(m);
        }

        for (size_t i = 0; i < tmp_states.size(); ++i)
        {
            Body_state & state = tmp_states[i];
            Molecule & molecule = _molecules[i];

            state._x += molecule._v * time_step;

//            Eigen::Matrix3f R_dot = star_matrix(molecule._omega) * molecule._R;
//            state._R += R_dot * time_step;

            Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
            Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
            state._q = add(state._q, scale(q_dot, time_step));

            state._P += molecule._force * time_step;

            state._L += molecule._torque * time_step;

            molecule.from_state(state);
        }
    }

    std::vector<Atom> const& get_atoms() const
    {
        return _atoms;
    }

    void add_atom(Atom const& atom)
    {
        _atoms.push_back(atom);
    }

    std::vector<Molecule> const& get_molecules() const
    {
        return _molecules;
    }

    void add_molecule(Molecule const& molecule)
    {
        _molecules.push_back(molecule);
    }

    void clear()
    {
        _atoms.clear();
        _molecules.clear();
    }

private:
    std::vector<Atom> _atoms;
    std::vector<Molecule> _molecules;
};

#endif // CORE_H
