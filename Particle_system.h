#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <QImage>
#include <QFont>
#include <QtGui>
#include <Eigen/Core>
#include <functional>
#include <vector>
#include <random>

#ifndef Q_MOC_RUN
#include <boost/serialization/version.hpp>
#endif

#include <Utilities.h>
#include <Color.h>
#include <Frame_buffer.h>

#include "PolygonalCurve.h"

struct Particle
{
    Particle() :
        position(Eigen::Vector3f::Zero()),
        speed(Eigen::Vector3f::Zero()),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        age(0.0f),
        size_factor(1.0f)
    { }

    virtual ~Particle() {}

    Eigen::Vector3f position;
    Eigen::Vector3f speed;
    Color4 color;
    Color4 current_color;
    float age; // between 0 and 1
    float size_factor;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(position);
        ar & BOOST_SERIALIZATION_NVP(speed);
        ar & BOOST_SERIALIZATION_NVP(color);
        ar & BOOST_SERIALIZATION_NVP(age);
    }
};

//BOOST_CLASS_VERSION(Particle, 1)

struct Targeted_particle : public Particle
{
    Targeted_particle() : Particle(),
        target(Eigen::Vector3f::Zero()),
        initial_speed(Eigen::Vector3f::Zero())
    { }

    Eigen::Vector3f target;
    Eigen::Vector3f initial_speed;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Particle);
        ar & BOOST_SERIALIZATION_NVP(initial_speed);
        ar & BOOST_SERIALIZATION_NVP(target);
    }
};

class Particle_system
{
public:
    void animate(std::function<void(Particle&, float const)> particle_function, float const timestep)
    {
        for (Particle & p : _particles)
        {
            particle_function(p, timestep);
        }
    }

protected:
    std::vector<Particle> _particles;
};

class Targeted_particle_system
{
public:
    Targeted_particle_system(float const total_duration = 10.0f) : _total_duration(total_duration), _age(0.0f)
    { }

    // rect in 0..1 coordinates
    void generate(std::string const& text, QFont const& main_font, QRectF const& rect);

    void generate(QImage const& image);

    void init(std::vector<Targeted_particle> const& particles);

    void animate(float const timestep);

    static bool is_dead(Targeted_particle_system const& p)
    {
        return p._age > p._total_duration;
    }

    std::vector<Targeted_particle> const& get_particles() const
    {
        return _particles;
    }

    std::vector<Targeted_particle> & get_particles()
    {
        return _particles;
    }

private:
    std::vector<Targeted_particle> _particles;

    float _total_duration;
    float _age;
};

class Curved_particle_system
{
public:
    Curved_particle_system();

    Curved_particle_system(std::vector<Eigen::Vector3f> const& curve, float const total_time);

    void animate(float const timestep);

    std::vector<Particle> const& get_curve_particles() const { return _curve_particles; }

    std::vector<Particle> const& get_effect_particles() const { return _effect_particles; }

    void set_effect_spread(float const spread) { _effect_spread = spread; }

    void set_particle_size(float const size) { _particle_size = size; }

    void set_tail_dissipation_speed(float const tail_dissipation_speed);

    void set_tangent_speed_factor(float const tangent_speed_factor);

    Color4 const& get_curve_color() const;
    void set_curve_color(Color4 const& get_curve_color);

    Color4 const& get_effect_color() const;
    void set_effect_color(const Color4 &get_effect_color);

    void set_display_ratio(float const ratio) { _display_ratio = ratio; }

    Polygonal_curve const& get_curve() const { return _curve; }

    void reset();

protected:
    float _total_time;
    float _current_time;

    Polygonal_curve _curve;

    std::vector<Particle> _curve_particles;
    std::vector<Particle> _effect_particles;

    std::mt19937 _rng;

    float _effect_spread;
    float _particle_size;
    float _tail_dissipation_speed;
    float _tangent_speed_factor;

    Color4 _curve_color;
    Color4 _effect_color;

    float _display_ratio;
};

//void draw_particle_system(Targeted_particle_system const& system, int const height);

#endif // PARTICLE_SYSTEM_H
