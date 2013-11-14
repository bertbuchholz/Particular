#ifndef LEVEL_ELEMENTS_H
#define LEVEL_ELEMENTS_H

#include <QVariantAnimation>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/list.hpp>

#include <Geometry_utils.h>
#include <Low_discrepancy_sequences.h>
#include <Parameter.h>

#include "End_condition.h"
#include "Particle_system.h"

#include "Visitor.h"
//#include "Barrier_draw_visitor.h"

#include "Eigen_Matrix_serializer.h"

class Molecule;
class Atom;


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
        _start(start), _end(end), _time(0.0f), _duration(duration), _direction(1), _type(int(type))
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
        return Type(_type);
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
        ar & BOOST_SERIALIZATION_NVP(_start);
        ar & BOOST_SERIALIZATION_NVP(_end);
        ar & BOOST_SERIALIZATION_NVP(_time);
        ar & BOOST_SERIALIZATION_NVP(_duration);
        ar & BOOST_SERIALIZATION_NVP(_direction);
        ar & BOOST_SERIALIZATION_NVP(_type);
    }

private:
    Eigen::Vector3f _start, _end;
    float _time;
    float _duration;
    int _direction; // -1 or +1
//    Type _type;
    int _type;
};


class Level_element
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    enum class Edit_type { None = 0, Rotate = 0x01, Translate = 0x02, Scale = 0x04, Property = 0x08, All = 0x0F  };

    virtual ~Level_element() {}

    Level_element() : _user_editable(Edit_type::None), _persistent(true), _selected(false)
    { }

    virtual void accept(Level_element_visitor const* visitor) = 0;

    Eigen::Vector3f const& get_position() const;
    void set_position(Eigen::Vector3f const& position);

    Eigen::Transform<float, 3, Eigen::Affine> const& get_transform() const;
    Eigen::Transform<float, 3, Eigen::Affine> const& get_inverse_transform() const;
    void set_transform(Eigen::Transform<float, 3, Eigen::Affine> const& transform);

    Edit_type is_user_editable() const;

    void set_user_editable(Edit_type const b);

    virtual void animate(float const timestep);
    void add_animation(Animation const& animation);
    void handle_animation();

    void notify_observers();
    void add_observer(Notifiable * n);
    void remove_observer(Notifiable * n);

    virtual void reset();

    bool is_persistent() const;
    void set_persistent(bool const p);

    bool is_selected() const;
    void set_selected(bool const s);

    bool does_intersect(Level_element const* element) const;

    virtual Eigen::AlignedBox<float, 3> get_world_aabb() const = 0;

    void add_property(Parameter const* parameter);
    virtual void set_property_values(Parameter_list const& /* properties */) {}

    Parameter_list const& get_parameters() const;
    Parameter_list & get_parameters();

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_position);
        ar & boost::serialization::make_nvp("transform", _transform.matrix());
        ar & BOOST_SERIALIZATION_NVP(_user_editable);
        ar & BOOST_SERIALIZATION_NVP(_animations);
        ar & BOOST_SERIALIZATION_NVP(_persistent);
        ar & BOOST_SERIALIZATION_NVP(_selected);
        ar & BOOST_SERIALIZATION_NVP(_properties);
    }

protected:
    Eigen::Vector3f _position;

    Edit_type _user_editable;

    std::vector<Animation> _animations;

    std::vector<Notifiable*> _observers;

    bool _persistent;

    bool _selected;

    Parameter_list _properties;

private:
    Eigen::Transform<float, 3, Eigen::Affine> _transform;
    Eigen::Transform<float, 3, Eigen::Affine> _inverse_transform;
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
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Level_element);
    }
};

class Plane_barrier : public Barrier
{
public:
    Plane_barrier() {}

    Plane_barrier(Eigen::Vector3f const& position, Eigen::Vector3f const& normal, float const strength, float const radius, boost::optional<Eigen::Vector2f> const& extent = boost::optional<Eigen::Vector2f>());

    Eigen::Vector3f calc_force_on_atom(Atom const& a) const override;

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override;

    boost::optional<Eigen::Vector2f> const& get_extent() const;
    void set_extent(boost::optional<Eigen::Vector2f> const& extent);

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Barrier);
        ar & BOOST_SERIALIZATION_NVP(_strength);
        ar & BOOST_SERIALIZATION_NVP(_radius);
        ar & BOOST_SERIALIZATION_NVP(_extent);
    }

private:
    float falloff_function(float const distance) const;

    float _strength;
    float _radius;
    boost::optional<Eigen::Vector2f> _extent;
};


class Portal : public Level_element
{
public:
    Portal() : _score_factor(60.0f), _destroy_on_entering(false) {}

    virtual ~Portal() {}

    virtual bool contains(Eigen::Vector3f const& pos) const = 0;

    void handle_molecule_entering();

    Molecule_capture_condition const& get_condition() const;
    Molecule_capture_condition & get_condition();

    float get_score_factor() const;
    void set_score_factor(float const score_factor);

    bool do_destroy_on_entering() const;
    void set_destroy_on_entering(bool const destroy_on_entering);

    void start_update();

    void reset() override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Level_element);
        ar & BOOST_SERIALIZATION_NVP(_end_condition);
        ar & BOOST_SERIALIZATION_NVP(_score_factor);

        if (version > 0)
        {
            ar & BOOST_SERIALIZATION_NVP(_destroy_on_entering);
        }
    }

private:
    Molecule_capture_condition _end_condition;
    float _score_factor;
    bool _destroy_on_entering;
};

BOOST_CLASS_VERSION(Portal, 1)

class Box_portal : public Portal
{
public:
    Box_portal() {}

    Box_portal(Eigen::Vector3f const& min, Eigen::Vector3f const& max);

    bool contains(Eigen::Vector3f const& pos) const override;

    Eigen::AlignedBox<float, 3> const& get_box() const;

    void set_size(Eigen::Vector3f const& extent);
    Eigen::Vector3f get_extent() const;

    void init_particle_system();

    void animate(const float timestep) override;

    void reset() override;

    std::vector<Particle> const& get_particles() const;

    void get_corner_min_max(Eigen::AlignedBox<float, 3>::CornerType const cornertype, Eigen::Vector3f & min, Eigen::Vector3f & max) const;

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Portal);
        ar & boost::serialization::make_nvp("boxmin", _box.min());
        ar & boost::serialization::make_nvp("boxmax", _box.max());
    }

protected:
    Eigen::AlignedBox<float, 3> _box;

    std::vector<Particle> _particles;
};


class Sphere_portal : public Box_portal
{
public:
    Sphere_portal() {}

    Sphere_portal(Eigen::Vector3f const& min, Eigen::Vector3f const& max) : Box_portal(min, max) {}

    bool contains(Eigen::Vector3f const& pos) const override;

    void init_particle_system();

    void animate(const float timestep) override;

    void reset() override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Box_portal);
    }
};

class Box_barrier : public Barrier
{
public:
    Box_barrier() {}

    Box_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius);

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

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override;

    Eigen::AlignedBox<float, 3> const& get_box() const;

    void set_size(Eigen::Vector3f const& extent);
    Eigen::Vector3f get_extent() const;

    void get_corner_min_max(Eigen::AlignedBox<float, 3>::CornerType const cornertype, Eigen::Vector3f & min, Eigen::Vector3f & max) const;

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Barrier);
        ar & boost::serialization::make_nvp("boxmin", _box.min());
        ar & boost::serialization::make_nvp("boxmax", _box.max());
        ar & BOOST_SERIALIZATION_NVP(_strength);
        ar & BOOST_SERIALIZATION_NVP(_radius);
        ar & BOOST_SERIALIZATION_NVP(_box_radius);
    }

protected:
    float falloff_function(float const distance) const;

    Eigen::AlignedBox<float, 3> _box;
    float _strength;
    float _radius;
    float _box_radius;
};


class Charged_barrier : public Box_barrier
{
public:
    Charged_barrier() {}

    Charged_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius, float const charge);

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override;

    void set_property_values(const Parameter_list &properties) override;
    void accept(const Level_element_visitor *visitor) override;

    float get_charge() const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Box_barrier);
        ar & BOOST_SERIALIZATION_NVP(_charge);
    }

private:
    float _charge;
};


class Tractor_barrier : public Box_barrier
{
public:
    Tractor_barrier() {}

    Tractor_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength);

    Eigen::Vector3f calc_force_on_molecule(Molecule const& m) const override;

    static bool is_dead(Particle const& p);

    void animate(const float timestep) override;

    float get_rotation_angle() const;

    std::list<Particle> const& get_particles();

    void set_property_values(Parameter_list const& properties) override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Box_barrier);
        ar & BOOST_SERIALIZATION_NVP(_tractor_strength);

        ar & BOOST_SERIALIZATION_NVP(_particles);
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

    Moving_box_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius);

    Animation & get_animation();

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Box_barrier);
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

    Eigen::Vector3f calc_force_on_atom(Atom const& a) const override;

    void accept(Level_element_visitor const* visitor) override;

    Axis get_direction() const;

private:
    Axis _direction;
    float _blow_distance;
};

class Brownian_element : public Level_element
{
public:
    Brownian_element() {}

    Brownian_element(float const strength, float const radius) : _strength(strength), _radius(radius) {}

    virtual ~Brownian_element() {}

    virtual float get_brownian_motion_factor(Eigen::Vector3f const& point) const = 0;

    float get_strength() const;
    void set_strength(float const strength);

    float get_radius() const;
    void set_radius(float const radius);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Level_element);
    }

protected:
    float _strength;
    float _radius;
};

class Brownian_plane : public Brownian_element
{
public:
    Brownian_plane(Eigen::Vector3f const& position, Eigen::Vector3f const& normal, float const strength, float const radius) :
        Brownian_element(strength, radius),
        _plane(Eigen::Hyperplane<float, 3>(normal, position))
    { }

    float get_brownian_motion_factor(Eigen::Vector3f const& point) const override;

    Eigen::Hyperplane<float, 3> const& get_plane() const;

    void accept(Level_element_visitor const* visitor) override;

private:
    float falloff_function(float const distance) const;

    Eigen::Hyperplane<float, 3> _plane;
};


class Brownian_box : public Brownian_element
{
public:
    Brownian_box() {}

    Brownian_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius);

    float get_brownian_motion_factor(Eigen::Vector3f const& point) const override;

//    void set_size(Eigen::Vector3f const& min, Eigen::Vector3f const& max)
//    {
//        _box.min() = min;
//        _box.max() = max;
//    }

    void set_size(Eigen::Vector3f const& extent);
    Eigen::Vector3f get_extent() const;

    Eigen::AlignedBox<float, 3> const& get_box() const;

    void animate(const float timestep) override;

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

    std::vector<Particle> const& get_particles() const;

    void get_corner_min_max(Eigen::AlignedBox<float, 3>::CornerType const cornertype, Eigen::Vector3f & min, Eigen::Vector3f & max) const;

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void set_property_values(Parameter_list const& properties) override;

    void accept(Level_element_visitor const* visitor) override;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Brownian_element);
        ar & boost::serialization::make_nvp("boxmin", _box.min());
        ar & boost::serialization::make_nvp("boxmax", _box.max());
        ar & BOOST_SERIALIZATION_NVP(_strength);
        ar & BOOST_SERIALIZATION_NVP(_radius);

        ar & BOOST_SERIALIZATION_NVP(_particles);
    }

private:
    float falloff_function(float const distance) const;

    Eigen::AlignedBox<float, 3> _box;

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

    void init(Molecule const& m);

    void animate(const float timestep) override;

    bool is_dead() const;

    float get_life_percentage();

    std::vector<Particle> const& get_particles() const;

    Eigen::AlignedBox<float, 3> get_world_aabb() const override;

    void accept(Level_element_visitor const* visitor) override;

private:
    std::vector<Particle> _particles;

    float _age;
    float _life_time;
};

#endif // LEVEL_ELEMENTS_H
