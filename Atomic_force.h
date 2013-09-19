#ifndef ATOMIC_FORCE_H
#define ATOMIC_FORCE_H

#include "Atom.h"

class Atomic_force
{
public:
    virtual ~Atomic_force() {}

    virtual std::string get_instance_name() const { return std::string(); }

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

    Eigen::Vector3f calc_force_between_atoms(Atom const& a_0, Atom const& a_1) const
    {
        Eigen::Vector3f direction = a_0.get_position() - a_1.get_position();
        float const distance = std::max(1e-5f, direction.norm()); // cap the distance at 1e-5 to avoid the singularity

        direction.normalize();

        Eigen::Vector3f const force = direction * calc_force(distance, a_0, a_1);

        return force;
    }

private:
    virtual float calc_force(float const distance, Atom const& a_0, Atom const& a_1) const = 0;
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
    float calc_force(float const , Atom const& , Atom const& ) const override
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

    std::string get_instance_name() const override
    {
        return name();
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
        parameters.add_parameter(new Parameter("strength", 1.0f, 1.0f, 1000.0f));
        return parameters;
    }

private:
    // http://en.wikipedia.org/wiki/Coulomb's_law

    // k_e * q1 * q2 * dir / distance**2

    float calc_force(float const distance, Atom const& a_0, Atom const& a_1) const override
    {
        return _strength * a_0._charge * a_1._charge / (distance * distance);
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
        parameters.add_parameter(new Parameter("strength", 100.0f, 1.0f, 10000.0f));
        parameters.add_parameter(new Parameter("radius",   5.0f, 1.0f, 100.0f));
        return parameters;
    }

private:
    float calc_force(float const distance, Atom const& a_0, Atom const& a_1) const override
    {
        return a_0._charge * a_1._charge * _strength * std::max(0.0f, wendland_2_1(distance / _radius));
    }

    float _strength;
    float _radius;
};

REGISTER_CLASS_WITH_PARAMETERS(Atomic_force, Wendland_force);

class Lennard_jones_force : public Atomic_force
{
public:
    static std::string name()
    {
        return "Lennard_jones_force";
    }

    std::string get_instance_name() const override
    {
        return name();
    }

    static Atomic_force * create()
    {
        return new Lennard_jones_force;
    }

    void set_parameters(Parameter_list const& parameters) override
    {
        _strength = parameters["strength"]->get_value<float>();
        _radius_factor = parameters["radius_factor"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("strength", 1.0f, 0.001f, 10.0f));
        parameters.add_parameter(new Parameter("radius_factor",   1.0f, 0.5f, 10.0f));
        return parameters;
    }

private:
    float calc_force(float const distance, Atom const& a_0, Atom const& a_1) const override
    {
        if (a_0._type == Atom::Type::Charge || a_1._type == Atom::Type::Charge) return 0.0f;

        float const vdw_radii = a_0._radius + a_1._radius;
        float const sigma = _radius_factor * vdw_radii;
        float const sigma_distance = sigma / distance;
//        float const pow_6 = std::pow(sigma / distance, 6.0f);
        float const pow_6 = sigma_distance * sigma_distance * sigma_distance * sigma_distance * sigma_distance * sigma_distance;
        return _strength * 4.0f * (pow_6 * pow_6 - pow_6);
    }

    float _strength;
    float _radius_factor;
};

REGISTER_CLASS_WITH_PARAMETERS(Atomic_force, Lennard_jones_force);

#endif // ATOMIC_FORCE_H
