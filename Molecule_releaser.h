#ifndef MOLECULE_RELEASER_H
#define MOLECULE_RELEASER_H

#include "Level_element.h"

#include "Atom.h"

class Molecule_releaser : public Level_element
{
public:
    Molecule_releaser() :
        _next_molecule_prepared(false),
        _particle_duration(0.5f),
        _animation_count(0.0f)
    {}

    Molecule_releaser(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const first_release, float const interval);

    void animate(const float timestep) override;

    Targeted_particle_system init_particle_system(Molecule const& m, Eigen::Vector3f const& local_pos);

    bool check_do_release(float const time);

    virtual Molecule release(float const time);

    Eigen::AlignedBox<float, 3> const& get_box() const;

    void set_size(Eigen::Vector3f const& extent);
    Eigen::Vector3f get_extent() const;

    void set_molecule_type(std::string const& molecule_type);
    std::string const& get_molecule_type() const;

    int get_num_max_molecules() const;
    void set_num_max_molecules(int const num_max_molecules);

    float get_interval() const;
    void set_interval(float const interval);

    float get_first_release() const;
    void set_first_release(float const first_release);

    int get_num_released_molecules() const;
    std::vector<Targeted_particle_system> const& get_particle_systems() const;

    float get_animation_count() const;

    void get_corner_min_max(Eigen::AlignedBox<float, 3>::CornerType const cornertype, Eigen::Vector3f & min, Eigen::Vector3f & max) const;

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void accept(Level_element_visitor const* visitor) override;

    void reset() override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Level_element);

        ar & boost::serialization::make_nvp("boxmin", _box.min());
        ar & boost::serialization::make_nvp("boxmax", _box.max());
        ar & BOOST_SERIALIZATION_NVP(_first_release);
        ar & BOOST_SERIALIZATION_NVP(_last_release);
        ar & BOOST_SERIALIZATION_NVP(_interval);
        ar & BOOST_SERIALIZATION_NVP(_num_max_molecules);
        ar & BOOST_SERIALIZATION_NVP(_num_released_molecules);
        ar & BOOST_SERIALIZATION_NVP(_molecule_type);

        if (version > 0)
        {
            ar & BOOST_SERIALIZATION_NVP(_next_molecule);
            ar & BOOST_SERIALIZATION_NVP(_next_molecule_prepared);
        }
    }

protected:
    Eigen::AlignedBox<float, 3> _box;
    float _first_release;
    float _last_release;
    float _interval;
    int _num_max_molecules;
    int _num_released_molecules;
    std::string _molecule_type;

    Molecule _next_molecule;
    bool _next_molecule_prepared;

    std::vector<Targeted_particle_system> _particles;

    float _particle_duration;

    float _animation_count;
};

BOOST_CLASS_VERSION(Molecule_releaser, 1)


class Atom_cannon : public Molecule_releaser
{
public:
    Atom_cannon() {}

    Atom_cannon(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const first_release, float const interval, float const speed, float const charge);

    void set_property_values(Parameter_list const& properties) override;

    Molecule release(float const time) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Molecule_releaser);
        ar & BOOST_SERIALIZATION_NVP(_speed);
        ar & BOOST_SERIALIZATION_NVP(_charge);
    }

private:
    float _speed;
    float _charge;
};


#endif // MOLECULE_RELEASER_H
