#ifndef LEVEL_ELEMENTS_H
#define LEVEL_ELEMENTS_H

#include <QVariantAnimation>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/list.hpp>

#include <Geometry_utils.h>
#include <Low_discrepancy_sequences.h>

#include "Atom.h"
#include "End_condition.h"
#include "Particle_system.h"

#include "Visitor.h"
//#include "Barrier_draw_visitor.h"

#include "Eigen_Matrix_serializer.h"


template<typename Scalar,int AmbientDim>
inline void closest_point_to_box(Eigen::AlignedBox<Scalar, AmbientDim> const& b, Eigen::Vector3f const& p, Eigen::Vector3f & result, float & distance)
{
    result = p;
    Scalar dist2(0);
    Scalar aux;

    for (typename Eigen::AlignedBox<Scalar, AmbientDim>::Index k = 0; k < b.dim(); ++k)
    {
        if(b.min()[k] > p[k])
        {
            result[k] = b.min()[k];
            aux = b.min()[k] - p[k];
            dist2 += aux*aux;
        }
        else if(p[k] > b.max()[k])
        {
            result[k] = b.max()[k];
            aux = p[k] - b.max()[k];
            dist2 += aux * aux;
        }
    }

    distance = std::sqrt(dist2);
}


class Animation
{
public:
    enum class Type { Position, Rotation };

    Animation() {}

    Animation(Eigen::Vector3f const& start, Eigen::Vector3f const& end, float const duration, Type const type) :
        _start(start), _end(end), _time(0.0f), _duration(duration), _direction(1), _type(type)
    { }

    // ping-pong animation

    void advance(float const timestep)
    {
        _time += _direction * timestep; // add easing curve

        if (!is_in_range(_time, 0.0f, _duration))
        {
            _direction *= -1;
            _time = into_range(_time, 0.0f, _duration);
        }
    }

    Eigen::Vector3f get_value() const
    {
        return _start + (_end - _start) * (_time / _duration);
    }

    Type get_type() const
    {
        return _type;
    }

    void set_start_point(Eigen::Vector3f const& p)
    {
        _start = p;
    }

    void set_end_point(Eigen::Vector3f const& p)
    {
        _end = p;
    }

    void set_duration(float const duration)
    {
        _duration = duration;
    }

    float get_duration() const
    {
        return _duration;
    }

    void reset()
    {
        _time = 0.0f;
        _direction = 1;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _start
                & _end
                & _time
                & _duration
                & _direction
                & _type;
    }

private:
    Eigen::Vector3f _start, _end;
    float _time;
    float _duration;
    int _direction; // -1 or +1
    Type _type;
};


class Level_element
{
public:
    enum class Edit_type { None = 0, Rotate = 0b1, Translate = 0b10, Scale = 0b100, Property = 0b1000, All = 0b1111  };

    virtual ~Level_element() {}

    Level_element() : _user_editable(Edit_type::None), _persistent(true), _selected(false)
    { }

    virtual void accept(Level_element_visitor const* visitor) = 0;

    Eigen::Vector3f const& get_position() const
    {
        return _position;
    }

    void set_position(Eigen::Vector3f const& position)
    {
        _position = position;
        notify_observers();
    }

    Eigen::Transform<float, 3, Eigen::Isometry> const& get_transform() const
    {
        return _transform;
    }

    void set_transform(Eigen::Transform<float, 3, Eigen::Isometry> const& transform)
    {
        _transform = transform;
    }

    Edit_type is_user_editable() const
    {
        return _user_editable;
    }

    void set_user_editable(Edit_type const b)
    {
        _user_editable = b;
    }

    virtual void animate(float const timestep)
    {
        if (_animations.empty()) return;

        for (Animation & a : _animations)
        {
            a.advance(timestep);
        }

        handle_animation();
    }

    void add_animation(Animation const& animation)
    {
        _animations.push_back(animation);
    }

    void handle_animation()
    {
        for (Animation const& a : _animations)
        {
            if (a.get_type() == Animation::Type::Position)
            {
                set_position(a.get_value());
            }
        }
    }

    void notify_observers()
    {
        for (auto o : _observers)
        {
            o->notify();
        }
    }

    void add_observer(Notifiable * n)
    {
        _observers.push_back(n);
    }

    void remove_observer(Notifiable * n)
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), n), _observers.end());
    }

    virtual void reset()
    {
        for (Animation & a : _animations)
        {
            a.reset();
        }

        handle_animation();

        _selected = false;
    }

    bool is_persistent() const
    {
        return _persistent;
    }

    void set_persistent(bool const p)
    {
        _persistent = p;
    }

    bool is_selected() const
    {
        return _selected;
    }

    void set_selected(bool const s)
    {
        _selected = s;
    }

    void add_property(Parameter const* parameter)
    {
        _properties.add_parameter(parameter->get_name(), new Parameter(*parameter));
    }

    virtual void set_property_values(Parameter_list const& /* properties */) {}

    Parameter_list const& get_parameters() const
    {
        return _properties;
    }

    Parameter_list & get_parameters()
    {
        return _properties;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _position;
        ar & _transform.matrix();
        ar & _user_editable;
        ar & _animations;
        ar & _persistent;
        ar & _selected;
        ar & _properties;
    }

protected:
    Eigen::Vector3f _position;
    Eigen::Transform<float, 3, Eigen::Isometry> _transform;

    Edit_type _user_editable;

    std::vector<Animation> _animations;

    std::vector<Notifiable*> _observers;

    bool _persistent;

    bool _selected;

    Parameter_list _properties;
};

class Barrier : public Level_element
{
public:
    virtual ~Barrier() {}

    Eigen::Vector3f virtual calc_force_on_atom(Atom const& /* a */) const { return Eigen::Vector3f::Zero(); }
    Eigen::Vector3f virtual calc_force_on_molecule(Molecule const& /* m */) const { return Eigen::Vector3f::Zero(); }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Level_element>(*this);
    }
};

class Plane_barrier : public Barrier
{
public:
    Plane_barrier() {}

    Plane_barrier(Eigen::Vector3f const& position, Eigen::Vector3f const& normal, float const strength, float const radius, boost::optional<Eigen::Vector2f> const& extent = boost::optional<Eigen::Vector2f>()) :
//        _plane(Eigen::Hyperplane<float, 3>(normal, position)), _strength(strength), _radius(radius), _extent(extent)
        _strength(strength), _radius(radius), _extent(extent)
    {
        Eigen::Vector3f tangent, bitangent;

        create_orthogonal_cs(normal, tangent, bitangent);
        Eigen::Matrix3f m;
        m.col(0) = tangent;
        m.col(1) = bitangent;
        m.col(2) = normal;

//        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>(m));
//        Eigen::Transform<float, 3, Eigen::Isometry> t(Eigen::AngleAxisf(M_PI * 0.5f, cross(normal, Eigen::Vector3f::UnitZ())));
//        set_transform(t);
        set_position(position);
    }

    Eigen::Vector3f calc_force_on_atom(Atom const& a) const override
    {
        Eigen::Vector3f const& normal = get_transform().linear().col(2);
        Eigen::Vector3f const local_pos = a._r - get_position();

        float const signed_distance = normal.dot(local_pos);

        return falloff_function(signed_distance) * normal;
    }

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override
    {
        Eigen::Vector3f const& normal = get_transform().linear().col(2);
        Eigen::Vector3f const local_pos = m._x - get_position();

        float const signed_distance = normal.dot(local_pos);

        return falloff_function(signed_distance) * normal;
    }


    boost::optional<Eigen::Vector2f> const& get_extent() const
    {
        return _extent;
    }

    void set_extent(boost::optional<Eigen::Vector2f> const& extent)
    {
        _extent = extent;
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Barrier>(*this);
        ar & _strength;
        ar & _radius;
        ar & _extent;
    }

private:
    float falloff_function(float const distance) const
    {
        if (distance > 0)
        {
            return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
        }
        else
        {
            return _strength;
        }
    }

    float _strength;
    float _radius;
    boost::optional<Eigen::Vector2f> _extent;
};

class Molecule_releaser : public Level_element
{
public:
    Molecule_releaser() {}

    Molecule_releaser(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const first_release, float const interval) :
        _first_release(first_release), _last_release(first_release), _interval(interval), _num_max_molecules(100), _num_released_molecules(0)
    {
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());

        set_position((min + max) * 0.5f);

        _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
    }

    bool check_do_release(float const time) const
    {
        return (_last_release + _interval < time && _num_released_molecules < _num_max_molecules);
    }

    virtual Molecule release(float const time)
    {
        _last_release = time;

        // get random position in the box (minus some margin), create molecule and give it a certain speed towards the release_axis

        Molecule m(_exemplar);

        Eigen::Vector3f local_pos = _box.sample() * 0.8f;
        local_pos[0] = _box.center()[0] + _box.sizes()[0] * 0.5f;

        m._x = get_transform() * local_pos + get_position();
        m._P = get_transform() * Eigen::Vector3f(1.0f, 0.0f, 0.0f) * 4.0f;

        ++_num_released_molecules;

        return m;
    }

    Eigen::AlignedBox<float, 3> const& get_box() const
    {
        return _box;
    }

    void set_size(Eigen::Vector3f const& extent)
    {
        _box.min() = -extent * 0.5f;
        _box.max() =  extent * 0.5f;
    }

    Eigen::Vector3f get_extent() const
    {
        return _box.max() - _box.min();
    }

    void set_exemplar(Molecule const& molecule)
    {
        _exemplar = molecule;
    }

    int get_num_max_molecules() const
    {
        return _num_max_molecules;
    }

    void set_num_max_molecules(int const num_max_molecules)
    {
        _num_max_molecules = num_max_molecules;
    }

    float get_interval() const
    {
        return _interval;
    }

    void set_interval(float const interval)
    {
        _interval = interval;
    }

    float get_first_release() const
    {
        return _first_release;
    }

    void set_first_release(float const first_release)
    {
        _first_release = first_release;
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    void reset() override
    {
        Level_element::reset();
        _last_release = _first_release;
        _num_released_molecules = 0;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Level_element>(*this);
        ar & _box.min();
        ar & _box.max();
        ar & _first_release;
        ar & _last_release;
        ar & _interval;
        ar & _num_max_molecules;
        ar & _num_released_molecules;
        ar & _exemplar;
    }

protected:
    Eigen::AlignedBox<float, 3> _box;
    float _first_release;
    float _last_release;
    float _interval;
    int _num_max_molecules;
    int _num_released_molecules;
    Molecule _exemplar;
};


class Atom_cannon : public Molecule_releaser
{
public:
    Atom_cannon() {}

    Atom_cannon(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const first_release, float const interval, float const speed, float const charge) :
        Molecule_releaser(min, max, first_release, interval),
        _speed(speed),
        _charge(charge)
    {
        add_property(new Parameter("charge", 0.0f, -1.0f, 1.0f));
        add_property(new Parameter("speed", 1.0f, 0.5f, 100.0f));
    }

    void set_property_values(Parameter_list const& properties) override
    {
        _charge = properties["charge"]->get_value<float>();
        _speed = properties["speed"]->get_value<float>();
    }

    Molecule release(float const time) override
    {
        _last_release = time;

        Molecule m = Molecule::create_charged_chlorine(Eigen::Vector3f::Zero()); // FIXME: make it a "Particle" or generic type molecule

        m._atoms[0]._charge = _charge;

        Eigen::Vector3f const random_pos = Eigen::Vector3f::Random();

        Eigen::Vector3f const local_pos = _box.center() - Eigen::Vector3f(_box.sizes()[0] * 0.4f, random_pos[1] * 0.4f * _box.sizes()[1], 0.0f);

        Eigen::Vector3f const aim_pos = _box.center() + Eigen::Vector3f(
                    _box.sizes()[0] * 0.5f,
                local_pos[1],
                random_pos[2] * 0.4f * _box.sizes()[2]);

        m._x = get_transform() * local_pos + get_position();
//        m._v = get_transform() * (aim_pos - local_pos).normalized() * _speed; // start with an initial speed
        m._P = get_transform() * (aim_pos - local_pos).normalized() * _speed; // start with an initial speed

        ++_num_released_molecules;

        return m;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Molecule_releaser>(*this);
        ar & _speed;
        ar & _charge;
    }

private:
    float _speed;
    float _charge;
};


class Portal : public Level_element
{
public:
    virtual ~Portal() {}

    virtual bool contains(Eigen::Vector3f const& pos) const = 0;

    void handle_molecule_entering()
    {
        _end_condition.set_num_captured_molecules(_end_condition.get_num_captured_molecules() + 1);
    }

    Molecule_capture_condition const& get_condition() const
    {
        return _end_condition;
    }

    Molecule_capture_condition & get_condition()
    {
        return _end_condition;
    }

    void reset() override
    {
        Level_element::reset();
        _end_condition.set_num_captured_molecules(0);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Level_element>(*this);
        ar & _end_condition;
    }

private:
    Molecule_capture_condition _end_condition;
};

class Box_portal : public Portal
{
public:
    Box_portal() {}

    Box_portal(Eigen::Vector3f const& min, Eigen::Vector3f const& max)
    {
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());

        set_position((min + max) * 0.5f);

        _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
    }

    bool contains(Eigen::Vector3f const& pos) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (pos - get_position());

        return (_box.exteriorDistance(local_pos) < 0.000001f);
    }

    Eigen::AlignedBox<float, 3> const& get_box() const
    {
        return _box;
    }

    void set_size(Eigen::Vector3f const& extent)
    {
        _box.min() = -extent * 0.5f;
        _box.max() =  extent * 0.5f;
    }

    Eigen::Vector3f get_extent() const
    {
        return _box.max() - _box.min();
    }

    Eigen::Vector3f get_world_min() const
    {
        return get_transform() * _box.min() + get_position();
    }

    Eigen::Vector3f get_world_max() const
    {
        return get_transform() * _box.max() + get_position();
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Portal>(*this);
        ar & _box.min();
        ar & _box.max();
    }

protected:
    float falloff_function(float const distance) const
    {
        if (distance > 0)
        {
            return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
        }
        else
        {
            return _strength;
        }
    }

    Eigen::AlignedBox<float, 3> _box;
    float _strength;
    float _radius;
};

class Box_barrier : public Barrier
{
public:
    Box_barrier() {}

    Box_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius) :
//        _box(Eigen::AlignedBox<float, 3>(min, max)),
        _strength(strength), _radius(radius)
    {
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());

        set_position((min + max) * 0.5f);

        _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
    }

//    Eigen::Vector3f calc_force(Atom const& a) const override
//    {
//        float const distance_to_center = (a._r - get_position()).norm();

//        if (distance_to_center > _box_radius + _radius) return Eigen::Vector3f(0.0f, 0.0f, 0.0f);

//        Eigen::Vector3f const local_pos = get_transform().inverse() * (a._r - get_position());

//        Eigen::Vector3f closest_point;
//        float distance;

//        closest_point_to_box(_box, local_pos, closest_point, distance);

//        distance -= a._radius;

////        return (get_transform() * local_pos.normalized()) * _strength * std::pow(1.0f / distance, 12.0f);

//        if (distance < 0.000001f)
//        {
//            return _strength * (get_transform() * local_pos.normalized()); // FIXME: maybe change it to make it orthogonal to the box walls
////            return Eigen::Vector3f(0.0f, 0.0f, 0.0f);
//        }

////        return falloff_function(_box.exteriorDistance(a._r)) * (a._r - _box.center()).normalized();
////        return falloff_function(_box.exteriorDistance(local_pos)) * (a._r - get_position()).normalized();
//        return falloff_function(distance) * (get_transform() * (local_pos - closest_point).normalized());
//    }

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override
    {
        float const distance_to_center = (m._x - get_position()).norm();

        if (distance_to_center > _box_radius + _radius) return Eigen::Vector3f(0.0f, 0.0f, 0.0f);

        Eigen::Vector3f const local_pos = get_transform().inverse() * (m._x - get_position());

        Eigen::Vector3f closest_point;
        float distance;

        closest_point_to_box(_box, local_pos, closest_point, distance);

//        distance -= m._radius;

        if (distance < 0.000001f)
        {
            return _strength * (get_transform() * local_pos.normalized());
        }

        return _strength * falloff_function(distance) * (get_transform() * (local_pos - closest_point).normalized());
    }

    Eigen::AlignedBox<float, 3> const& get_box() const
    {
        return _box;
    }

    void set_size(Eigen::Vector3f const& extent)
    {
        _box.min() = -extent * 0.5f;
        _box.max() =  extent * 0.5f;

        _box_radius = (_box.max() - _box.min()).norm() * 0.5f;
    }

    Eigen::Vector3f get_extent() const
    {
        return _box.max() - _box.min();
    }

    Eigen::Vector3f get_world_min() const
    {
        return get_transform() * _box.min() + get_position();
    }

    Eigen::Vector3f get_world_max() const
    {
        return get_transform() * _box.max() + get_position();
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Barrier>(*this);
        ar & _box.min();
        ar & _box.max();
        ar & _strength;
        ar & _radius;
        ar & _box_radius;
    }

protected:
    float falloff_function(float const distance) const
    {
        if (distance > 0)
        {
            return wendland_2_1(std::min(1.0f, (distance / _radius)));
        }
        else
        {
            return 1.0f;
        }
    }

    Eigen::AlignedBox<float, 3> _box;
    float _strength;
    float _radius;
    float _box_radius;
};


class Charged_barrier : public Box_barrier
{
public:
    Charged_barrier() {}

    Charged_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius, float const charge) :
        Box_barrier(min, max, strength, radius), _charge(charge)
    { }

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (m._x - get_position());

        Eigen::Vector3f closest_point;
        float distance;

        closest_point_to_box(_box, local_pos, closest_point, distance);

        if (distance < 0.000001f)
        {
            return _strength * (get_transform() * local_pos.normalized());
        }

        return (_strength * falloff_function(distance) + _charge * m._accumulated_charge) * (get_transform() * (local_pos - closest_point).normalized());
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Box_barrier>(*this);
        ar & _charge;
    }

private:
    float _charge;
};


class Tractor_barrier : public Box_barrier
{
public:
    Tractor_barrier() {}

    Tractor_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength) :
        Box_barrier(min, max, strength, 0.0f), _last_created_particle(0.0f), _rotation_angle(0.0f)
    {
        add_property(new Parameter("tractor_strength", 2.0f, -10.0f, 10.0f));
        add_property(new Parameter("radius", 5.0f, 5.0f, 100.0f));

        set_property_values(_properties);
    }

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (m._x - get_position());

        if (local_pos[2] > _box.max()[2] || local_pos[2] < _box.min()[2]) return Eigen::Vector3f::Zero();

        float const distance = std::abs(local_pos[0] - (_box.max()[0] - _box.min()[0]) * 0.5f);

        if (distance < 0.000001f)
        {
            return _strength * (get_transform() * local_pos.normalized());
        }

        return (_tractor_strength * falloff_function(distance)) * (get_transform() * Eigen::Vector3f::UnitX());
    }

    static bool is_dead(Particle const& p)
    {
        return p.age > 0.99f;
    }

    void animate(const float timestep) override
    {
        _particles.erase(std::remove_if(_particles.begin(), _particles.end(), Tractor_barrier::is_dead), _particles.end());

        float const area_measure = sqrt(_box.sizes()[1] * _box.sizes()[2]);

        float const max_speed = _tractor_strength * 2.0f;

        if (_last_created_particle > 10.0f / (area_measure * std::abs(_tractor_strength)))
        {
            _last_created_particle = 0.0f;

            Particle p;
            p.age = 0.0f;
            p.color = Color(1.0f, 0.3f, 0.05f);

            float const direction = _tractor_strength >= 0.0f ? 1.0f : -1.0f;
            p.position = Eigen::Vector3f::Random();
            p.position[0] = -direction * (_radius * 0.95f + _box.sizes()[0] * 0.5f);
            p.position[1] *= _box.sizes()[1] * 0.5f;
            p.position[2] *= _box.sizes()[2] * 0.5f;
            p.speed = Eigen::Vector3f(max_speed, 0.0f, 0.0f);

            _particles.push_back(p);
        }

        _last_created_particle += timestep;

        for (Particle & p : _particles)
        {
            float const distance = std::abs(p.position[0]);

            const float falloff = std::min(1.0f, (distance / (_radius + _box.sizes()[0] * 0.5f)));
            p.age = falloff;

            float const diff = max_speed - p.speed[0];
            p.speed[0] += diff * timestep;

            p.position += p.speed * timestep;
        }

        _rotation_angle += 5.0f * _tractor_strength * timestep;
    }

    float get_rotation_angle() const
    {
        return _rotation_angle;
    }

    std::list<Particle> const& get_particles()
    {
        return _particles;
    }

    void set_property_values(Parameter_list const& properties) override
    {
        _tractor_strength = properties["tractor_strength"]->get_value<float>();
        _radius   = properties["radius"]->get_value<float>();
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Box_barrier>(*this);
        ar & _tractor_strength;

        ar & _particles;
    }

private:
    float _tractor_strength;

    float _last_created_particle;

    float _rotation_angle;

    std::list<Particle> _particles;
};


class Moving_box_barrier : public Box_barrier
{
public:
    Moving_box_barrier() {}

    Moving_box_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius) :
        Box_barrier(min, max, strength, radius)
    {
        _animations.push_back(Animation(get_position(), get_position(), 2.0f, Animation::Type::Position));
    }

    Animation & get_animation()
    {
        return _animations[0];
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Box_barrier>(*this);
    }
};

class Blow_barrier : public Box_barrier
{
public:
    enum class Axis { X = 0, Y, Z };

    Blow_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, Axis const& direction, float const blow_distance, float const strength, float const radius) :
        Box_barrier(min, max, strength, radius),
        _direction(direction),
        _blow_distance(blow_distance)
    { }

    Eigen::Vector3f calc_force_on_atom(Atom const& a) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (a._r - get_position());

        Eigen::Vector3f closest_point;
        float distance;

        closest_point_to_box(_box, local_pos, closest_point, distance);

        if (distance < 0.000001f)
        {
            return Eigen::Vector3f(0.0f, 0.0f, 0.0f);
        }

        int const axis_0 = (int(_direction) + 1) % 3;
        int const axis_1 = (int(_direction) + 2) % 3;

        if (is_in_range(local_pos[axis_0], _box.min()[axis_0], _box.max()[axis_0]) &&
            is_in_range(local_pos[axis_1], _box.min()[axis_1], _box.max()[axis_1]))
        {
            return 10.0f * wendland_2_1(std::min(1.0f, (local_pos[int(_direction)] / _blow_distance))) * (get_transform() * (local_pos - closest_point).normalized());
        }
        else
        {
            return _strength * falloff_function(distance) * (get_transform() * (local_pos - closest_point).normalized());
        }

//        return falloff_function(_box.exteriorDistance(a._r)) * (a._r - _box.center()).normalized();
//        return falloff_function(_box.exteriorDistance(local_pos)) * (a._r - get_position()).normalized();
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    Axis get_direction() const
    {
        return _direction;
    }

private:
    Axis _direction;
    float _blow_distance;
};

class Brownian_element : public Level_element
{
public:
    virtual ~Brownian_element() {}

    virtual float get_brownian_motion_factor(Eigen::Vector3f const& point) const = 0;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Level_element>(*this);
    }
};

class Brownian_plane : public Brownian_element
{
public:
    Brownian_plane(Eigen::Vector3f const& position, Eigen::Vector3f const& normal, float const strength, float const radius) :
        _plane(Eigen::Hyperplane<float, 3>(normal, position)), _strength(strength), _radius(radius)
    { }

    float get_brownian_motion_factor(Eigen::Vector3f const& point) const override
    {
        return falloff_function(_plane.absDistance(point));
    }

    Eigen::Hyperplane<float, 3> const& get_plane() const
    {
        return _plane;
    }

    float get_strength() const
    {
        return _strength;
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

private:
    float falloff_function(float const distance) const
    {
        return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
    }

    Eigen::Hyperplane<float, 3> _plane;
    float _strength;
    float _radius;
};


class Brownian_box : public Brownian_element
{
public:
    Brownian_box() {}

    Brownian_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius) :
//        _box(Eigen::AlignedBox<float, 3>(min, max)),
        _strength(strength), _radius(radius)
    {
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());

        set_position((min + max) * 0.5f);

        _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());

        int const num_particles = 100;

        for (int i = 0; i < num_particles; ++i)
        {
            Particle p;
            p.position = Eigen::Vector3f::Random().cwiseProduct(get_extent()) * 0.5f;
            p.speed = Eigen::Vector3f::Random() * 0.1f;
            p.speed *= (_strength + 50.0f) * 0.01f;

            _particles.push_back(p);
        }

        add_property(new Parameter("radius", 10.0f, 5.0f, 100.0f));
        add_property(new Parameter("strength", 0.0f, -50.0f, 50.0f));
    }

    float get_brownian_motion_factor(Eigen::Vector3f const& point) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (point - get_position());

//        return falloff_function(_box.exteriorDistance(point));
        return falloff_function(_box.exteriorDistance(local_pos));
    }

//    void set_size(Eigen::Vector3f const& min, Eigen::Vector3f const& max)
//    {
//        _box.min() = min;
//        _box.max() = max;
//    }

    void set_size(Eigen::Vector3f const& extent)
    {
        _box.min() = -extent * 0.5f;
        _box.max() =  extent * 0.5f;
    }

    Eigen::Vector3f get_extent() const
    {
        return _box.max() - _box.min();
    }

    Eigen::AlignedBox<float, 3> const& get_box() const
    {
        return _box;
    }

    Eigen::Vector3f get_world_min() const
    {
        return get_transform() * _box.min() + get_position();
    }

    Eigen::Vector3f get_world_max() const
    {
        return get_transform() * _box.max() + get_position();
    }

    float get_strength() const
    {
        return _strength;
    }

    void set_strength(float const strength)
    {
        _strength = strength;
    }

    float get_radius() const
    {
        return _radius;
    }

    void set_radius(float const radius)
    {
        _radius = radius;
    }

    void animate(const float timestep) override
    {
        for (Particle & p : _particles)
        {
            p.position += p.speed * timestep;

            float distance;
            Eigen::Vector3f closest_point;

            closest_point_to_box(_box, p.position, closest_point, distance);

            if (distance > 4.0f) // can happen when downsizing the element
            {
                p.speed = p.speed.norm() * (closest_point - p.position).normalized();
            }
            else if (distance > 0.0001f)
            {
                if ((closest_point - p.position).dot(p.speed) < 0.0f)
                {
                    Eigen::Vector3f normal = (closest_point - p.position).normalized();
                    p.speed = p.speed - 2.0f * (p.speed.dot(normal) * normal);
                }
            }
            else
            {
                p.speed += Eigen::Vector3f::Random();
                p.speed = p.speed.normalized() * (_strength + 50.0f) * 0.1f;
            }
        }

//        _particles2.animate(std::bind(&Brownian_box::p_function, this, std::placeholders::_1, std::placeholders::_2), timestep);
    }

//    void p_function(Particle & p, float const timestep)
//    {
//        p.position += p.speed * timestep;

//        float distance;
//        Eigen::Vector3f closest_point;

//        closest_point_to_box(_box, p.position, closest_point, distance);

//        if (distance > 4.0f) // can happen when downsizing the element
//        {
//            p.speed = p.speed.norm() * (closest_point - p.position).normalized();
//        }
//        else if (distance > 0.0001f)
//        {
//            if ((closest_point - p.position).dot(p.speed) < 0.0f)
//            {
//                Eigen::Vector3f normal = (closest_point - p.position).normalized();
//                p.speed = p.speed - 2.0f * (p.speed.dot(normal) * normal);
//            }
//        }
//        else
//        {
//            p.speed += Eigen::Vector3f::Random();
//            p.speed = p.speed.normalized() * (_strength + 50.0f) * 0.1f;
//        }
//    }

    std::vector<Particle> const& get_particles() const
    {
        return _particles;
    }


    void set_property_values(Parameter_list const& properties) override
    {
        _strength = properties["strength"]->get_value<float>();
        _radius   = properties["radius"]->get_value<float>();
    }


    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Brownian_element>(*this);
        ar & _box.min();
        ar & _box.max();
        ar & _strength;
        ar & _radius;

        ar & _particles;
    }

private:
    float falloff_function(float const distance) const
    {
        return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
    }

    Eigen::AlignedBox<float, 3> _box;
    float _strength;
    float _radius;

    std::vector<Particle> _particles;
};


class Particle_system_element : public Level_element
{
public:
    Particle_system_element() : _age(0.0f), _life_time(1.0f)
    { }

    struct check_if_dead
    {
        bool operator()(Particle_system_element const* p) const
        {
            return p->is_dead();
        }
    };

    void init(Molecule const& m)
    {
        int const num_particles = 100;

        Halton halton2(2);
        Halton halton3(3);
        Halton halton5(5);

        _particles.resize(num_particles);

        for (int i = 0; i < num_particles; ++i)
        {
            Atom const& a = m._atoms[std::min(int(m._atoms.size() - 1), int(halton2.getNext() * m._atoms.size()))];

            float const theta = 2.0f * std::acos(std::sqrt(1.0f - halton3.getNext()));
            float const phi = 2.0f * M_PI * halton5.getNext();

            Eigen::Vector3f pos(std::sin(theta) * std::cos(phi),
                                std::sin(theta) * std::sin(phi),
                                std::cos(theta));

            pos *= a._radius;
            pos += a._r;

            Eigen::Vector3f const speed = Eigen::Vector3f::Random() * 10.0f;

            Particle p;
            p.position = pos;
            p.speed = speed;
            p.color = Atom::atom_colors[int(a._type)];

            _particles[i] = p;
        }
    }

    void animate(const float timestep) override
    {
        _age += timestep;

        for (Particle & p : _particles)
        {
            p.position += p.speed * timestep;
            p.speed += Eigen::Vector3f::Random() * timestep;
//            p.speed = p.speed.normalized();
        }
    }

    bool is_dead() const
    {
        return _age > _life_time;
    }

    float get_life_percentage()
    {
        return _age / _life_time;
    }

    std::vector<Particle> const& get_particles() const
    {
        return _particles;
    }

    void accept(Level_element_visitor const* visitor) override
    {
        visitor->visit(this);
    }

private:
    std::vector<Particle> _particles;

    float _age;
    float _life_time;
};



#endif // LEVEL_ELEMENTS_H
