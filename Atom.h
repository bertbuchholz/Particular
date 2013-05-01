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

typedef OpenMesh::Vec3f Vec;


inline Eigen::Vector3f OM2Eigen(OpenMesh::Vec3f const& p)
{
    return Eigen::Vector3f(p[0], p[1], p[2]);
}

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
    enum class Type { Charge, H, O, C, S, N };

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
        a._type = Type::N;
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

//private:
    Eigen::Vector3f _r_0; // body space position of particle
    Eigen::Vector3f _r;   // world space position
    float _mass;
    float _charge;
    float _radius;
    Type _type;
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
        m._atoms.back()._charge = 0.0f;

        int current_connector = m.set_new_connector();


        {
            float const oxygen_charge_angle_deg = 120.0f;
            float const radius = 0.7f;
            float const angle_2 = 0.5f * oxygen_charge_angle_deg / 360.0f * 2.0f * M_PI;

            m._atoms.push_back(Atom::create_charge(Eigen::Vector3f(-std::cos( angle_2) * radius, 0.0f, std::sin( angle_2) * radius), -0.4f));
            m._atoms.push_back(Atom::create_charge(Eigen::Vector3f(-std::cos(-angle_2) * radius, 0.0f, std::sin(-angle_2) * radius), -0.4f));
        }

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

        Eigen::Vector3f natrium_pos(-0.2f, 0.0f, current_z_offset + 4.0f);
        m._atoms.push_back(Atom::create_natrium(natrium_pos));

        m.reset();
        m.init();

        return m;
    }

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
        Eigen::Matrix3f identity;
        identity.setIdentity();

        Eigen::Vector3f center_of_mass = Eigen::Vector3f::Zero();

        _I_body.setZero();
        _mass = 0.0f;

        for (Atom const& a : _atoms)
        {
            _mass += a._mass;
            center_of_mass += a._mass * a._r_0;

            _I_body += a._mass * ((a._r_0.dot(a._r_0) * identity) - (a._r_0 * a._r_0.transpose()));
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

    void from_state(Body_state const& state, float const mass_factor)
    {
//        _L = state._L;
//        _P = state._P;
//        _q = state._q;
//        _x = state._x;

        _v = _P / (_mass * mass_factor);
        _R = _q.normalized().toRotationMatrix();
        _I_inv = _R * _I_body_inv * _R.transpose();
        _omega = (1.0f / mass_factor) * _I_inv * _L;

        for (Atom & a : _atoms)
        {
            a._r = _R * a._r_0 + _x;
        }
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

    int _id;

private:
    Molecule(Eigen::Vector3f const& position) :
        _x(position)
    { }
};


class Barrier
{
public:
    Eigen::Vector3f virtual calc_force(Atom const& a) const = 0;
};

class Line_barrier : public Barrier
{
public:
    Line_barrier(Eigen::Vector3f const& position, Eigen::Vector3f const& normal, float const strength, float const radius) :
        _point(position), _normal(normal), _strength(strength), _radius(radius)
    {}

    Eigen::Vector3f calc_force(Atom const& a) const override
    {
        return falloff_function(distance_to_object(a._r)) * _normal;
    }

private:
    float distance_to_object(Eigen::Vector3f const& p) const
    {
        return (p - _point).dot(_normal);
    }

    float falloff_function(float const distance) const
    {
        return std::max(0.0f, _strength * (1.0f -  distance / _radius));
    }

    Eigen::Vector3f _point;
    Eigen::Vector3f _normal;
    float _strength;
    float _radius;
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


class Atomic_force
{
public:
    virtual ~Atomic_force() {}

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

    Eigen::Vector3f calc_force_between_atoms(Atom const& a_0, Atom const& a_1)
    {
        Eigen::Vector3f direction = a_0._r - a_1._r;
        float const distance = std::max(1e-5f, direction.norm()); // cap the distance at 1e-5 to avoid the singularity

        direction.normalize();

        Eigen::Vector3f const force = direction * calc_force(distance, a_0._charge, a_1._charge);

        return force;
    }

private:
    virtual float calc_force(float const distance, float const charge_0, float const charge_1) const = 0;
};

class Null_force : public Atomic_force
{
public:
    static std::string name()
    {
        return "Null_force";
    }

    static Atomic_force * create()
    {
        return new Null_force;
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

private:
    float calc_force(float const , float const , float const ) const override
    {
        return 0.0f;
    }
};

REGISTER_CLASS_WITH_PARAMETERS(Atomic_force, Null_force);


class Coulomb_force : public Atomic_force
{
public:
    static std::string name()
    {
        return "Coulomb_force";
    }

    static Atomic_force * create()
    {
        return new Coulomb_force;
    }

    void set_parameters(Parameter_list const& parameters) override
    {
        _strength = parameters["strength"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("strength", 1.0f, 0.1f, 100.0f));
        return parameters;
    }

private:
    // http://en.wikipedia.org/wiki/Coulomb's_law

    // k_e * q1 * q2 * dir / distance**2

    float calc_force(float const distance, float const charge_0, float const charge_1) const override
    {
        return _strength * charge_0 * charge_1 / (distance * distance);
    }

    float _strength;
};

REGISTER_CLASS_WITH_PARAMETERS(Atomic_force, Coulomb_force);

class Wendland_force : public Atomic_force
{
public:
    static std::string name()
    {
        return "Wendland_force";
    }

    static Atomic_force * create()
    {
        return new Wendland_force;
    }

    void set_parameters(Parameter_list const& parameters) override
    {
        _strength = parameters["strength"]->get_value<float>();
        _radius = parameters["radius"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("strength", 100.0f, 10.0f, 10000.0f));
        parameters.add_parameter(new Parameter("radius",   5.0f, 1.0f, 100.0f));
        return parameters;
    }

private:
    float calc_force(float const distance, float const charge_0, float const charge_1) const override
    {
        return charge_0 * charge_1 * _strength * std::max(0.0f, wendland_2_1(distance / _radius));
    }

    float _strength;
    float _radius;
};

REGISTER_CLASS_WITH_PARAMETERS(Atomic_force, Wendland_force);

inline float calc_lennard_jones_potential(Molecule const& m_0, Molecule const& m_1)
{
    float const vdw_radius = 2.0f; // vdW radius for a water molecule
    float const sigma = 2.0f * vdw_radius;
    float const dist = (m_0._x - m_1._x).norm();
    return 4.0f * (std::pow(sigma / dist, 12.0f) - std::pow(sigma / dist, 6.0f));
}

class Molecule_external_force
{
public:
    int _molecule_id;
    Eigen::Vector3f _origin;
    Eigen::Vector3f _local_origin; // force origin expressed in molecule's space
    Eigen::Vector3f _force;
    float _end_time;
};

#endif // ATOM_H
