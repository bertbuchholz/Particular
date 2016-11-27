#include "Atom.h"

#include "Color.h"

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
//    {"O2",     Molecule::create_oxygen},
    {"H2O",    Molecule::create_water},
//    {"SDS",    Molecule::create_sulfate},
    {"Na",     Molecule::create_charged_natrium},
    {"Cl",     Molecule::create_charged_chlorine}
//    {"Dipole", Molecule::create_dipole}
};


bool Molecule::molecule_exists(const std::string &name)
{
    auto iter = Molecule::_molecule_factory_map.find(name);

    return (iter != _molecule_factory_map.end());
}

std::vector<std::string> Molecule::get_molecule_names()
{
    std::vector<std::string> result;

    for (auto & iter : _molecule_factory_map)
    {
        result.push_back(iter.first);
    }

    return result;
}

Molecule Molecule::create(const std::string &name, const Eigen::Vector3f &position)
{
    auto iter = Molecule::_molecule_factory_map.find(name);

    assert(iter != _molecule_factory_map.end());

    return iter->second(position);
}

Molecule Molecule::create_water(const Eigen::Vector3f &position)
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
        float const angle_2 = 0.5f * 104.5f / 360.0f * 2.0f * float(M_PI);

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

Molecule Molecule::create_oxygen(const Eigen::Vector3f &position)
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

Molecule Molecule::create_dipole(const Eigen::Vector3f &position)
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

Molecule Molecule::create_charged_natrium(const Eigen::Vector3f &position)
{
    Molecule m(position);

    m._atoms.push_back(Atom::create_natrium(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));
    m._atoms.back()._charge = 1.0f;

    m.reset();
    m.init();

    return m;
}

Molecule Molecule::create_charged_chlorine(const Eigen::Vector3f &position)
{
    Molecule m(position);

    m._atoms.push_back(Atom::create_chlorine(Eigen::Vector3f(0.0f, 0.0f, 0.0f)));
    m._atoms.back()._charge = -1.0f;

    m.reset();
    m.init();

    return m;
}

Molecule Molecule::create_sulfate(const Eigen::Vector3f &position)
{
    Molecule m(position);

    float current_z_offset = 0.0f;

    m._atoms.push_back(Atom::create_hydrogen(Eigen::Vector3f(0.0f, 0.0f, -0.5f)));
    int current_connector = m.set_new_connector();

    for (int i = 0; i < 12; ++i)
    {
        float const radius = 0.96f;
        Eigen::Vector3f const offset((i % 2 == 0) ? -0.2f : 0.2f, 0.0f, current_z_offset);

        Eigen::AngleAxisf const rotation_pos(50.0f / 360.0f * 2.0f * float(M_PI), Eigen::Vector3f(0.0f, 0.0f,  1.0f));
        Eigen::AngleAxisf const rotation_neg(50.0f / 360.0f * 2.0f * float(M_PI), Eigen::Vector3f(0.0f, 0.0f, -1.0f));

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

int Molecule::set_new_connector()
{
    int const new_connector = int(_atoms.size()) - 1;
    _connectivity.resize(new_connector + 1);
    return new_connector;
}

void Molecule::add_connection(const int connector)
{
    assert(connector < int(_connectivity.size()));
    _connectivity[connector].push_back(int(_atoms.size()) - 1);
}

void Molecule::reset()
{
    _P.setZero();
    _L.setZero();
    _q.setIdentity();

    _v = Eigen::Vector3f::Zero();
    _omega = Eigen::Vector3f::Zero();
    _I_inv.setIdentity();
    _R.setIdentity();
}

void Molecule::init()
{
    Eigen::Matrix3f identity(Eigen::Matrix3f::Identity());
    //        identity.setIdentity();

    Eigen::Vector3f center_of_mass = Eigen::Vector3f::Zero();

    _I_body.setZero();
    _mass = 0.0f;
    _accumulated_charge = 0.0f;

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

Body_state Molecule::to_state() const
{
    Body_state state;
    state._L = _L;
    state._P = _P;
    state._q = _q;
    state._x = _x;

    return state;
}

void Molecule::from_state(const Body_state &, const float mass_factor)
{
    _v = _P / (_mass * mass_factor);
    _q.normalize();
    _R = _q.toRotationMatrix();
    _I_inv = _R * _I_body_inv * _R.transpose();

    //    std::cout << "_I_inv * _L: " << (_I_inv * _L) << std::endl;

    _omega = (1.0f / mass_factor) * _I_inv * _L;

    //    assert(!std::isnan(_omega.x()) && !std::isinf(_omega.x()));
    //    assert(!std::isnan(_omega.y()) && !std::isinf(_omega.y()));
    //    assert(!std::isnan(_omega.z()) && !std::isinf(_omega.z()));

    update_atom_positions();
}

void Molecule::apply_orientation(const Eigen::Quaternion<float> &orientation)
{
    _q = orientation;
    _R = _q.normalized().toRotationMatrix();
    //        _I_inv = _R * _I_body_inv * _R.transpose();

    update_atom_positions();
}

void Molecule::update_atom_positions()
{
    for (Atom & a : _atoms)
    {
        a.set_position(_R * a._r_0 + _x);
        assert(!std::isnan(a.get_position()[0]) && !std::isinf(a.get_position()[0]));
    }
}

void Molecule::set_id(const int id)
{
    _id = id;

    for (Atom & a : _atoms)
    {
        a._parent_id = _id;
    }
}
