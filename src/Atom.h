#ifndef ATOM_H
#define ATOM_H

#include <vector>
#include <cmath>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>


#include <OpenMesh/Core/Geometry/VectorT.hh>

#include "Parameter.h"
#include "Utilities.h"
#include "Color.h"
#include "Eigen_Matrix_serializer.h"
#include "Level_element.h"

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


class Atom : public Level_element
{
public:
    enum class Type { Charge = 0, H, O, C, S, N, Na, Cl };

    static std::vector<Color> atom_colors; // Type to Color mapping

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

    void accept(Level_element_visitor const* /* visitor */ ) override { /* visitor->visit(this); */ }

    Eigen::AlignedBox<float, 3> get_world_aabb() const override { return Eigen::AlignedBox3f(); }



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
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(_r_0);

        if (version < 1)
        {
//            ar & BOOST_SERIALIZATION_NVP(_r);
        }
        else
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Level_element);
        }

        ar & BOOST_SERIALIZATION_NVP(_mass);
        ar & BOOST_SERIALIZATION_NVP(_charge);
        ar & BOOST_SERIALIZATION_NVP(_radius);
        ar & BOOST_SERIALIZATION_NVP(_type);
        ar & BOOST_SERIALIZATION_NVP(_parent_id);
    }

    Atom()
    {
//        std::cout << "Atom() should not be used outside serialization" << std::endl;
        _parent_id = -1;
    }

//private:
    Eigen::Vector3f _r_0; // body space position of particle
//    Eigen::Vector3f _r;   // world space position // replaced by _position in Level_element
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

BOOST_CLASS_VERSION(Atom, 1)

class Molecule
{
public:
    static std::unordered_map< std::string, std::function<Molecule(Eigen::Vector3f const& position)> > _molecule_factory_map;

    static bool molecule_exists(std::string const& name);
    static std::vector<std::string> get_molecule_names();

    static Molecule create(std::string const& name, Eigen::Vector3f const& position = Eigen::Vector3f::Zero());
    static Molecule create_water(Eigen::Vector3f const& position);
    static Molecule create_oxygen(Eigen::Vector3f const& position);
    static Molecule create_dipole(Eigen::Vector3f const& position);
    static Molecule create_charged_natrium(Eigen::Vector3f const& position);
    static Molecule create_charged_chlorine(Eigen::Vector3f const& position);
    static Molecule create_sulfate(Eigen::Vector3f const& position);

    // last added atom becomes new connector
    int set_new_connector();
    void add_connection(int const connector);

    void reset();
    void init();

    Body_state to_state() const;
    void from_state(Body_state const& /* state */, float const mass_factor);

    void apply_orientation(Eigen::Quaternion<float> const& orientation);

    void update_atom_positions();

    int get_id() const { return _id; }
    void set_id(int const id);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_atoms);

        ar & BOOST_SERIALIZATION_NVP(_mass);
        ar & BOOST_SERIALIZATION_NVP(_I_body);
        ar & BOOST_SERIALIZATION_NVP(_I_body_inv);

        ar & BOOST_SERIALIZATION_NVP(_x);
        ar & boost::serialization::make_nvp("qx", _q.x());
        ar & boost::serialization::make_nvp("qy", _q.y());
        ar & boost::serialization::make_nvp("qz", _q.z());
        ar & boost::serialization::make_nvp("qw", _q.w());

        ar & BOOST_SERIALIZATION_NVP(_P);
        ar & BOOST_SERIALIZATION_NVP(_L);

        ar & BOOST_SERIALIZATION_NVP(_I_inv);
        ar & BOOST_SERIALIZATION_NVP(_R);
        ar & BOOST_SERIALIZATION_NVP(_v);
        ar & BOOST_SERIALIZATION_NVP(_omega);

        ar & BOOST_SERIALIZATION_NVP(_force);
        ar & BOOST_SERIALIZATION_NVP(_torque);

        ar & BOOST_SERIALIZATION_NVP(_connectivity);

        ar & BOOST_SERIALIZATION_NVP(_accumulated_charge);

        ar & BOOST_SERIALIZATION_NVP(_id);
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
    float _accumulated_charge;

    Molecule()
    {
        std::cout << "Molecule() should not be used outside serialization" << std::endl;
    }

private:
    Molecule(Eigen::Vector3f const& position) :
        _x(position), _accumulated_charge(0.0f)
    { }

    int _id;
};

struct Compare_by_id
{
    Compare_by_id(int const id) : _id(id) { }

    bool operator() (Molecule const& m) const
    {
        return (m.get_id() == _id);
    }

    int _id;
};


struct Force_indicator
{
    Force_indicator(Eigen::Vector3f const& position) :
        _atom(Atom(position, 1.0f, 1.0f, 1.0f)), _force(Eigen::Vector3f(0.0f, 0.0f, 0.0f))
    {
        _atom.set_position(position);
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
    float const dist = (a_0.get_position() - a_1.get_position()).norm();
    return 4.0f * (std::pow(sigma / dist, 12.0f) - std::pow(sigma / dist, 6.0f));
}

struct External_force
{
    External_force()
    {
        _origin = Eigen::Vector3f::Zero();
        _force  = Eigen::Vector3f::Zero();
    }

    Eigen::Vector3f _origin;
    Eigen::Vector3f _force;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_origin);
        ar & BOOST_SERIALIZATION_NVP(_force);
    }
};

struct Molecule_external_force
{
    Molecule_external_force()
    {
        _origin = Eigen::Vector3f::Zero();
        _force  = Eigen::Vector3f::Zero();

        _molecule_id = -1;
        _local_origin = Eigen::Vector3f::Zero();
        _plane_normal = Eigen::Vector3f::Zero();
        _end_time = 0.0f;
    }

    int _molecule_id;
    Eigen::Vector3f _origin;
    Eigen::Vector3f _local_origin; // force origin expressed in molecule's space
    Eigen::Vector3f _force;
    Eigen::Vector3f _plane_normal;
    float _end_time;
};

#endif // ATOM_H
