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
    _R = _q.normalized().toRotationMatrix();
    _I_inv = _R * _I_body_inv * _R.transpose();
    _omega = (1.0f / mass_factor) * _I_inv * _L;
//    if (_omega.squaredNorm() > 1.0f)
//    {
//        _omega = std::min(_omega.norm(), 1.0f) * _omega.normalized();
//    }

    assert(!std::isnan(_omega.x()));

    for (Atom & a : _atoms)
    {
        a.set_position(_R * a._r_0 + _x);
        assert(!std::isnan(a.get_position()[0]) && !std::isinf(a.get_position()[0]));
    }
}
