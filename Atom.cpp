#include "Atom.h"

#include <Color.h>

std::vector<Color> Atom::atom_colors =
{
    Color(0.3f, 0.2f, 0.7f), // Atom::Type::Charge
    Color(1.0f, 1.0f, 1.0f), // Atom::Type::H
    Color(0.9f, 0.2f, 0.2f), // Atom::Type::O
    Color(0.3f, 0.3f, 0.4f), // Atom::Type::C
    Color(0.8f, 0.7f, 0.2f), // Atom::Type::S
    Color(0.8f, 0.3f, 0.8f), // Atom::Type::N
    Color(0.8f, 0.3f, 0.8f), // Atom::Type::Na
    Color(0.5f, 0.8f, 0.3f)  // Atom::Type::Cl
};

std::unordered_map< std::string, std::function<Molecule(Eigen::Vector3f const& position)> > Molecule::_molecule_factory_map =
{
    {"O2",     Molecule::create_oxygen},
    {"H2O",    Molecule::create_water},
    {"SDS",    Molecule::create_sulfate},
    {"Na",     Molecule::create_charged_natrium},
    {"Cl",     Molecule::create_charged_chlorine},
    {"Dipole", Molecule::create_dipole}
};


void Molecule::init()
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

        _accumulated_charge += a._charge;
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
        a.set_position(_R * a._r_0 + _x);
    }

    // sanity check
    Eigen::Vector3f particle_pos_sum = Eigen::Vector3f::Zero();

    for (Atom const& a : _atoms)
    {
        particle_pos_sum += a._mass * a._r_0;
    }

    assert(particle_pos_sum.norm() < 1e-4f);
}

void Molecule::from_state(const Body_state &, const float mass_factor)
{
    //        _L = state._L;
    //        _P = state._P;
    //        _q = state._q;
    //        _x = state._x;

    _v = _P / (_mass * mass_factor);
    //    if (_v.squaredNorm() > 100.0f)
    //    {
    //        _v = std::min(_v.norm(), 10.0f) * _v.normalized();
    //    }
    _q.normalize();
    //    _R = _q.normalized().toRotationMatrix();
    _R = _q.toRotationMatrix();
    _I_inv = _R * _I_body_inv * _R.transpose();

//    std::cout << "_I_inv * _L: " << (_I_inv * _L) << std::endl;

    _omega = (1.0f / mass_factor) * _I_inv * _L;
    //    if (_omega.squaredNorm() > 1.0f)
    //    {
    //        _omega = std::min(_omega.norm(), 1.0f) * _omega.normalized();
    //    }

//    assert(!std::isnan(_omega.x()) && !std::isinf(_omega.x()));
//    assert(!std::isnan(_omega.y()) && !std::isinf(_omega.y()));
//    assert(!std::isnan(_omega.z()) && !std::isinf(_omega.z()));


    for (Atom & a : _atoms)
    {
        a.set_position(_R * a._r_0 + _x);
        assert(!std::isnan(a.get_position()[0]) && !std::isinf(a.get_position()[0]));
    }
}
