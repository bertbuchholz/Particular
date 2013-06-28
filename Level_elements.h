#ifndef LEVEL_ELEMENTS_H
#define LEVEL_ELEMENTS_H

#include <QVariantAnimation>
#include <boost/serialization/optional.hpp>

#include <Geometry_utils.h>

#include "Atom.h"
#include "End_condition.h"

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
    virtual ~Level_element() {}

    Level_element() : _user_editable(false), _persistent(true)
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

    bool is_user_editable() const
    {
        return _user_editable;
    }

    void set_user_editable(bool const b)
    {
        _user_editable = b;
    }

    void animate(float const timestep)
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

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _position;
        ar & _transform.matrix();
        ar & _user_editable;
        ar & _animations;
        ar & _persistent;
        ar & _selected;
    }

protected:
    Eigen::Vector3f _position;
    Eigen::Transform<float, 3, Eigen::Isometry> _transform;

    bool _user_editable;

    std::vector<Animation> _animations;

    std::vector<Notifiable*> _observers;

    bool _persistent;

    bool _selected;
};

class Barrier : public Level_element
{
public:
    virtual ~Barrier() {}

    Eigen::Vector3f virtual calc_force(Atom const& a) const = 0;

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

    Eigen::Vector3f calc_force(Atom const& a) const override
    {
        Eigen::Vector3f const& normal = get_transform().linear().col(2);
        Eigen::Vector3f const local_pos = a._r - get_position();

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

    void accept(Level_element_visitor const* visitor)
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

//    Eigen::Hyperplane<float, 3> _plane;
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

    void accept(Level_element_visitor const* visitor)
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

private:
    Eigen::AlignedBox<float, 3> _box;
    float _first_release;
    float _last_release;
    float _interval;
    int _num_max_molecules;
    int _num_released_molecules;
    Molecule _exemplar;
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

    void accept(Level_element_visitor const* visitor)
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

    Eigen::Vector3f calc_force(Atom const& a) const override
    {
        Eigen::Vector3f const local_pos = get_transform().inverse() * (a._r - get_position());

        Eigen::Vector3f closest_point;
        float distance;

        closest_point_to_box(_box, local_pos, closest_point, distance);

        distance -= a._radius;

//        return (get_transform() * local_pos.normalized()) * _strength * std::pow(1.0f / distance, 12.0f);

        if (distance < 0.000001f)
        {
            return _strength * (get_transform() * local_pos.normalized()); // FIXME: maybe change it to make it orthogonal to the box walls
//            return Eigen::Vector3f(0.0f, 0.0f, 0.0f);
        }

//        return falloff_function(_box.exteriorDistance(a._r)) * (a._r - _box.center()).normalized();
//        return falloff_function(_box.exteriorDistance(local_pos)) * (a._r - get_position()).normalized();
        return falloff_function(distance) * (get_transform() * (local_pos - closest_point).normalized());
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

    void accept(Level_element_visitor const* visitor)
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

    void accept(Level_element_visitor const* visitor)
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

    Eigen::Vector3f calc_force(Atom const& a) const override
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
            return falloff_function(distance) * (get_transform() * (local_pos - closest_point).normalized());
        }

//        return falloff_function(_box.exteriorDistance(a._r)) * (a._r - _box.center()).normalized();
//        return falloff_function(_box.exteriorDistance(local_pos)) * (a._r - get_position()).normalized();
    }

    void accept(Level_element_visitor const* visitor)
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

    void accept(Level_element_visitor const* visitor)
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
    Brownian_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius) :
//        _box(Eigen::AlignedBox<float, 3>(min, max)),
        _strength(strength), _radius(radius)
    {
        set_transform(Eigen::Transform<float, 3, Eigen::Isometry>::Identity());

        set_position((min + max) * 0.5f);

        _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
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

    void accept(Level_element_visitor const* visitor)
    {
        visitor->visit(this);
    }

private:
    float falloff_function(float const distance) const
    {
        return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
    }

    Eigen::AlignedBox<float, 3> _box;
    float _strength;
    float _radius;
};


#endif // LEVEL_ELEMENTS_H
