#ifndef ATOM_H
#define ATOM_H

#include <vector>
#include <cmath>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>


#include <OpenMesh/Core/Geometry/VectorT.hh>

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

class Atom
{
public:
    static Atom create_oxygen(Vec const& position)
    {
        return Atom(position, 20.66f, 1.0f, 152.0f / 100.0f);
    }

    static Atom create_hydrogen(Vec const& position)
    {
        return Atom(position, 1.66f, 1.0f, 120.0f / 100.f);
    }

    Atom(Vec const& position, float const mass, float const charge, float const radius) :
        _r_0(OM2Eigen(position)), _mass(mass), _charge(charge), _radius(radius)
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
    Eigen::Vector3f _r_0; // FIXME: new body space position of particle
    Eigen::Vector3f _r;   // world space position
    float _mass;
    float _charge;
    float _radius;
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
        Molecule m(position, 24.0f);

        m._atoms.push_back(Atom::create_oxygen(Vec(0.0f)));

        float const radius = 0.96f;

        m._atoms.push_back(Atom::create_hydrogen(Vec(radius, 0.0f, 0.0f)));

        float const angle = 104.5f / 360.0f * 2.0f * M_PI;

        m._atoms.push_back(Atom::create_hydrogen(Vec(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f)));

        m.init();

        return m;
    }

    void init()
    {
        Eigen::Matrix3f identity;
        identity.setIdentity();

        Eigen::Vector3f center_of_mass = Eigen::Vector3f::Zero();

        _mass = 0.0f;

        _I_body.setZero();

        for (Atom const& a : _atoms)
        {
            _mass += a._mass;
            center_of_mass += a._mass * a._r_0; // FIXME: _x should be the center of mass in world space, need to handle correctly if object's position is not the origin

            _I_body += a._mass * ((a._r_0.dot(a._r_0) * identity) - (a._r_0 * a._r_0.transpose()));
        }

        _I_body_inv = _I_body.inverse();

        center_of_mass /= _mass;

        //_x += center_of_mass;

        _R.setIdentity();
        _P.setZero();
        _L.setZero();
        _q.setIdentity();

        _v = Eigen::Vector3f::Zero();
        _omega = Eigen::Vector3f::Zero();

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

        assert(particle_pos_sum.norm() < 1e-6f);
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

    void from_state(Body_state const& state)
    {
        _L = state._L;
        _P = state._P;
        _q = state._q;
        _x = state._x;

        _v = _P / _mass;
        _R = _q.normalized().toRotationMatrix();
        _I_inv = _R * _I_body_inv * _R.transpose();
        _omega = _I_inv * _L;

        for (Atom & a : _atoms)
        {
            a._r = _R * a._r_0 + _x;
        }
    }

    std::vector<Atom> _atoms;

    /* Constant quantities */
    double _mass;   /* mass M*/
    Eigen::Matrix3f _I_body;  /* I_body */
    Eigen::Matrix3f _I_body_inv; /* I−1 (inverse of Ibody) */

    /* State variables */
    Eigen::Vector3f _x;
//    Eigen::Matrix3f _R;
    Eigen::Quaternion<float> _q;
    Eigen::Vector3f _P;
    Eigen::Vector3f _L;

    /* Derived quantities (auxiliary variables) */
    Eigen::Matrix3f _I_inv;
    Eigen::Matrix3f _R;
    Eigen::Vector3f _v;
    Eigen::Vector3f _omega;

    /* Computed quantities */
    Eigen::Vector3f _force;  /* F(t) */
    Eigen::Vector3f _torque; /* τ(t) */

private:
    Molecule(Eigen::Vector3f const& position, float const mass) :
        _mass(mass), _x(position)
    { }
};


// http://en.wikipedia.org/wiki/Coulomb's_law

// k_e * q1 * q2 * dir / distance**2

inline Eigen::Vector3f coulomb_force(float const distance, float const charge_0, float const charge_1, Eigen::Vector3f const& direction)
{
    return charge_0 * charge_1 * direction / (distance * distance);
}


inline Eigen::Vector3f calc_force_between_atoms(Atom const& a_0, Atom const& a_1)
{
    Eigen::Vector3f direction = a_0._r - a_1._r;
    float const distance = direction.norm();

    if (distance < 1e-5f) return Eigen::Vector3f::Zero();

    direction.normalize();

    Eigen::Vector3f const force = coulomb_force(distance, a_0._charge, a_1._charge, direction);

    return force;
}

#endif // ATOM_H
