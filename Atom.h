#ifndef ATOM_H
#define ATOM_H

#include <vector>
#include <cmath>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>


#include <OpenMesh/Core/Geometry/VectorT.hh>

#include <Parameter.h>
#include <Utilities.h>

#include "Eigen_Matrix_serializer.h"

typedef OpenMesh::Vec3f Vec;

inline Eigen::Matrix3f star_matrix(Eigen::Vector3f const& v)
{
    Eigen::Matrix3f result;
    result <<  0.0f , -v[2] ,  v[1] ,
               v[2] ,  0.0f , -v[0] ,
              -v[1] ,  v[0] ,  0.0f;

    return result;
}

// 1 mol of H weights 1.008 g
// 1 mol = 6.02 x 10^23 particles
// Mass of 1 atom = 1.008 g / 6.02 x 10^23 = 1.67 x 10^-24 g


class Atom
{
public:
    enum class Type { Charge, H, O, C, S, N, Na, Cl };

    static float get_atom_mass(float const weight_per_mol)
    {
        return weight_per_mol / 6.02f; // see above, missing 1e-24 factor
    }

    static Atom create_charge(Eigen::Vector3f const& position, float const charge)
    {
        Atom a(position, 0.0f, charge, 0.0f);
        a._type = Type::Charge;
        return a;
    }

    static Atom create_hydrogen(Eigen::Vector3f const& position)
    {
        Atom a(position, 1.008f, 0.4f, 120.0f / 100.f);
        a._type = Type::H;
        return a;
    }

    static Atom create_oxygen(Eigen::Vector3f const& position)
    {
        Atom a(position, 16.0f, -0.8f, 152.0f / 100.0f);
        a._type = Type::O;
        return a;
    }

    static Atom create_carbon(Eigen::Vector3f const& position)
    {
        Atom a(position, 12.01f, 0.137f, 170.0f / 100.f);
        a._type = Type::C;
        return a;
    }

    static Atom create_sulfur(Eigen::Vector3f const& position)
    {
        Atom a(position, 32.07f, 1.284f, 180.0f / 100.f);
        a._type = Type::S;
        return a;
    }

    static Atom create_natrium(Eigen::Vector3f const& position)
    {
        Atom a(position, 22.99f, 1.0f, 227.0f / 100.f);
        a._type = Type::Na;
        return a;
    }

    static Atom create_chlorine(Eigen::Vector3f const& position)
    {
        Atom a(position, 35.4f, 1.0f, 180.0f / 100.f);
        a._type = Type::Cl;
        return a;
    }

    Atom(Eigen::Vector3f const& position, float const weight_per_mol, float const charge, float const radius) :
        _r_0(position), _mass(get_atom_mass(weight_per_mol)), _charge(charge), _radius(radius)
    { }

//    Vec const& get_speed() const
//    {
//        return _speed;
//    }

//    Vec const& get_position() const
//    {
//        return _position;
//    }

//    float get_mass() const
//    {
//        return _mass;
//    }

//    float get_charge() const
//    {
//        return _charge;
//    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _r_0;
        ar & _r;
        ar & _mass;
        ar & _charge;
        ar & _radius;
        ar & _type;
        ar & _parent_id;
    }

    Atom()
    {
//        std::cout << "Atom() should not be used outside serialization" << std::endl;
        _parent_id = -1;
    }

//private:
    Eigen::Vector3f _r_0; // body space position of particle
    Eigen::Vector3f _r;   // world space position
    float _mass;
    float _charge;
    float _radius;
    Type _type;
    int _parent_id;
};

struct Body_state
{
    /* State variables */
    Eigen::Vector3f _x;
    Eigen::Quaternion<float> _q;
    Eigen::Vector3f _P;
    Eigen::Vector3f _L;
};

class Molecule
{
public:
    static Molecule create_water(Eigen::Vector3f const& position)
    {
        Molecule m(position);

        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));
//        m._atoms.back()._charge = 0.0f;

        int current_connector = m.set_new_connector();


//        {
//            float const oxygen_charge_angle_deg = 120.0f;
//            float const radius = 0.7f;
//            float const angle_2 = 0.5f * oxygen_charge_angle_deg / 360.0f * 2.0f * M_PI;

//            m._atoms.push_back(Atom::create_charge(Eigen::Vector3f(-std::cos( angle_2) * radius, 0.0f, std::sin( angle_2) * radius), -0.4f));
//            m._atoms.push_back(Atom::create_charge(Eigen::Vector3f(-std::cos(-angle_2) * radius, 0.0f, std::sin(-angle_2) * radius), -0.4f));
//        }

        {
            float const radius = 0.96f;
            float const angle_2 = 0.5f * 104.5f / 360.0f * 2.0f * M_PI;

            //        m._atoms.push_back(Atom::create_hydrogen(Eigen::Vector3f(radius, 0.0f, 0.0f)));
            m._atoms.push_back(Atom::create_hydrogen(Eigen::Vector3f(std::cos( angle_2) * radius, std::sin( angle_2) * radius, 0.0f)));
            m.add_connection(current_connector);
            m._atoms.push_back(Atom::create_hydrogen(Eigen::Vector3f(std::cos(-angle_2) * radius, std::sin(-angle_2) * radius, 0.0f)));
            m.add_connection(current_connector);
        }

        m.reset();
        m.init();

        return m;
    }

    static Molecule create_oxygen(Eigen::Vector3f const& position)
    {
        Molecule m(position);

        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(0.3f, 0.4f, 0.0f)));

//        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));

//        float const radius = 1.2f;

//        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(radius, 0.0f, 0.0f)));

        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(1.4f, 0.5f, 0.0f)));

        m.reset();
        m.init();

        return m;
    }

    static Molecule create_dipole(Eigen::Vector3f const& position)
    {
        Molecule m(position);


        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(0.0f, 0.707f, 0.0f)));
        m._atoms.back()._charge = -1.0f;
        m._atoms.back()._radius = 1.0f;
        m._atoms.back()._type = Atom::Type::H;
        int current_connector = m.set_new_connector();

        m._atoms.push_back(Atom::create_oxygen(Eigen::Vector3f(0.707f, 0.0f, 0.0f)));
        m._atoms.back()._charge = 1.0f;
        m._atoms.back()._radius = 1.0f;
        m.add_connection(current_connector);

        m.reset();
        m.init();

        return m;
    }

    static Molecule create_charged_natrium(Eigen::Vector3f const& position)
    {
        Molecule m(position);

        m._atoms.push_back(Atom::create_natrium(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));
        m._atoms.back()._charge = 1.0f;

        m.reset();
        m.init();

        return m;
    }

    static Molecule create_charged_chlorine(Eigen::Vector3f const& position)
    {
        Molecule m(position);

        m._atoms.push_back(Atom::create_chlorine(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));
        m._atoms.back()._charge = -1.0f;

        m.reset();
        m.init();

        return m;
    }

    static Molecule create_sulfate(Eigen::Vector3f const& position)
    {
        Molecule m(position);

        float current_z_offset = 0.0f;

        m._atoms.push_back(Atom::create_hydrogen(Eigen::Vector3f(0.0f, 0.0f, -0.5f)));
        int current_connector = m.set_new_connector();

        for (int i = 0; i < 12; ++i)
        {
            float const radius = 0.96f;
            Eigen::Vector3f const offset((i % 2 == 0) ? -0.2f : 0.2f, 0.0f, current_z_offset);

            Eigen::AngleAxisf const rotation_pos(50.0f / 360.0f * 2.0f * M_PI, Eigen::Vector3f(0.0f, 0.0f,  1.0f));
            Eigen::AngleAxisf const rotation_neg(50.0f / 360.0f * 2.0f * M_PI, Eigen::Vector3f(0.0f, 0.0f, -1.0f));

            m._atoms.push_back(Atom::create_carbon(offset));
            m.add_connection(current_connector);
            current_connector = m.set_new_connector();
            m._atoms.push_back(Atom::create_hydrogen(rotation_pos * Eigen::Vector3f(((i % 2 == 0) ? -1.0f : 1.0f) * radius, 0.0f, 0.0f) + offset));
            m.add_connection(current_connector);
            m._atoms.push_back(Atom::create_hydrogen(rotation_neg * Eigen::Vector3f(((i % 2 == 0) ? -1.0f : 1.0f) * radius, 0.0f, 0.0f) + offset));
            m.add_connection(current_connector);

            current_z_offset += 1.0f;
        }

        for (Atom & a : m._atoms)
        {
            a._charge = 0.0f;
        }

        (m._atoms.end() - 3)->_charge = 0.137f;

        Eigen::Vector3f const o_4_pos(-0.2f, 0.0f, current_z_offset);

        m._atoms.push_back(Atom::create_oxygen(o_4_pos));
        m._atoms.back()._charge = -0.459f;

        m.add_connection(current_connector);
        current_connector = m.set_new_connector();

        current_z_offset += 1.0f;

        Eigen::Vector3f const sulfur_pos(0.2f, 0.0f, current_z_offset);

        m.add_connection(current_connector);
        current_connector = m.set_new_connector();

        m._atoms.push_back(Atom::create_sulfur(sulfur_pos));

        Eigen::Vector3f const rotation_axis((sulfur_pos - o_4_pos).normalized());
        Eigen::Vector3f const triangle_center(2.0f * sulfur_pos - o_4_pos);

        Eigen::Vector3f const orthogonal_vec(rotation_axis.cross(Eigen::Vector3f(0.0f, 0.0f, 1.0f)).normalized() * 1.2f);

        for (int i = 0; i < 3; ++i)
        {
            Eigen::AngleAxisf rotation(i * 120.0f / 360.0f * 2.0f * M_PI, rotation_axis);

            Eigen::Vector3f o_1_3_pos(rotation * orthogonal_vec + triangle_center);

            m._atoms.push_back(Atom::create_oxygen(o_1_3_pos));
            m._atoms.back()._charge = -0.654f;

            m.add_connection(current_connector);
        }

//        Eigen::Vector3f natrium_pos(-0.2f, 0.0f, current_z_offset + 4.0f);
//        m._atoms.push_back(Atom::create_natrium(natrium_pos));

        m.reset();
        m.init();

        return m;
    }

    // last added atom becomes new connector
    int set_new_connector()
    {
       int const new_connector = _atoms.size() - 1;
       _connectivity.resize(new_connector + 1);
       return new_connector;
    }

    void add_connection(int const connector)
    {
        assert(connector < int(_connectivity.size()));
        _connectivity[connector].push_back(_atoms.size() - 1);
    }

    void reset()
    {
        _R.setIdentity();
        _P.setZero();
        _L.setZero();
        _q.setIdentity();

        _v = Eigen::Vector3f::Zero();
        _omega = Eigen::Vector3f::Zero();
    }

    void init()
    {
        Eigen::Matrix3f identity(Eigen::Matrix3f::Identity());
//        identity.setIdentity();

        Eigen::Vector3f center_of_mass = Eigen::Vector3f::Zero();

        _I_body.setZero();
        _mass = 0.0f;

        for (Atom const& a : _atoms)
        {
            _mass += a._mass;
            center_of_mass += a._mass * a._r_0;

            _I_body += a._mass * ((a._r_0.dot(a._r_0) * identity) - (a._r_0 * a._r_0.transpose()));
        }

        if (_I_body.isZero())
        {
            _I_body = identity;
        }

        _I_body_inv = _I_body.inverse();

        center_of_mass /= _mass;

        for (Atom & a : _atoms)
        {
            a._r_0 = a._r_0 - center_of_mass;
            a._r = _R * a._r_0 + _x;
        }

        // sanity check
        Eigen::Vector3f particle_pos_sum = Eigen::Vector3f::Zero();

        for (Atom const& a : _atoms)
        {
            particle_pos_sum += a._mass * a._r_0;
        }

        assert(particle_pos_sum.norm() < 1e-4f);
    }

    Body_state to_state() const
    {
        Body_state state;
        state._L = _L;
        state._P = _P;
        state._q = _q;
        state._x = _x;

        return state;
    }

    void from_state(Body_state const& /* state */, float const mass_factor)
    {
//        _L = state._L;
//        _P = state._P;
//        _q = state._q;
//        _x = state._x;

        _v = _P / (_mass * mass_factor);
        _R = _q.normalized().toRotationMatrix();
        _I_inv = _R * _I_body_inv * _R.transpose();
        _omega = (1.0f / mass_factor) * _I_inv * _L;

        assert(!std::isnan(_omega.x()));

        for (Atom & a : _atoms)
        {
            a._r = _R * a._r_0 + _x;
            assert(!std::isnan(a._r[0]) && !std::isinf(a._r[0]));
        }
    }

    int get_id() const
    {
        return _id;
    }

    void set_id(int const id)
    {
        _id = id;

        for (Atom & a : _atoms)
        {
            a._parent_id = _id;
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _atoms;

        ar & _mass;
        ar & _I_body;
        ar & _I_body_inv;

        ar & _x;
        ar & _q.x();
        ar & _q.y();
        ar & _q.z();
        ar & _q.w();
        ar & _P;
        ar & _L;

        ar & _I_inv;
        ar & _R;
        ar & _v;
        ar & _omega;

        ar & _force;
        ar & _torque;

        ar & _connectivity;

        ar & _active;

        ar & _id;
    }

    std::vector<Atom> _atoms;

    /* Constant quantities */
    float _mass;
    Eigen::Matrix3f _I_body;
    Eigen::Matrix3f _I_body_inv; /* inverse of I_body */

    /* State variables */
    Eigen::Vector3f _x;
    Eigen::Quaternion<float> _q;
    Eigen::Vector3f _P;
    Eigen::Vector3f _L;

    /* Derived quantities (auxiliary variables) */
    Eigen::Matrix3f _I_inv;
    Eigen::Matrix3f _R; // no need to store, can be derived from q
    Eigen::Vector3f _v;
    Eigen::Vector3f _omega;

    /* Computed quantities */
    Eigen::Vector3f _force;  /* F(t) */
    Eigen::Vector3f _torque; /* omega(t) */

    std::vector< std::vector<int> > _connectivity;

    bool _active;

    Molecule()
    {
        std::cout << "Molecule() should not be used outside serialization" << std::endl;
    }

private:
    Molecule(Eigen::Vector3f const& position) :
        _x(position),
        _active(true)
    { }

    int _id;
};

struct Force_indicator
{
    Force_indicator(Eigen::Vector3f const& position) :
        _atom(Atom(position, 1.0f, 1.0f, 1.0f)), _force(Eigen::Vector3f(0.0f, 0.0f, 0.0f))
    {
        _atom._r = position;
    }

    Atom _atom;
    Eigen::Vector3f _force;
};


inline float calc_lennard_jones_potential(Molecule const& m_0, Molecule const& m_1)
{
    float const vdw_radius = 2.0f; // vdW radius for a water molecule
    float const sigma = 2.0f * vdw_radius;
    float const dist = (m_0._x - m_1._x).norm();
    return 4.0f * (std::pow(sigma / dist, 12.0f) - std::pow(sigma / dist, 6.0f));
}

inline float calc_lennard_jones_potential(Atom const& a_0, Atom const& a_1)
{
    float const vdw_radii = a_0._radius + a_1._radius;
    float const sigma = 2.0f * vdw_radii;
    float const dist = (a_0._r - a_1._r).norm();
    return 4.0f * (std::pow(sigma / dist, 12.0f) - std::pow(sigma / dist, 6.0f));
}

struct External_force
{
    Eigen::Vector3f _origin;
    Eigen::Vector3f _force;
};

struct Molecule_external_force
{
    int _molecule_id;
    Eigen::Vector3f _origin;
    Eigen::Vector3f _local_origin; // force origin expressed in molecule's space
    Eigen::Vector3f _force;
    Eigen::Vector3f _plane_normal;
    float _end_time;
};

#endif // ATOM_H
