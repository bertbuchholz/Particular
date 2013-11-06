#include "Level_element.h"

#include "Atom.h"

void Level_element::set_position(const Eigen::Vector3f &position)
{
    _position = position;
    notify_observers();
}


const Eigen::Vector3f &Level_element::get_position() const
{
    return _position;
}


const Eigen::Transform<float, 3, Eigen::Affine> &Level_element::get_transform() const
{
    return _transform;
}


Eigen::Transform<float, 3, Eigen::Affine> const& Level_element::get_inverse_transform() const
{
    return _transform;
}


void Level_element::set_transform(Eigen::Transform<float, 3, Eigen::Affine> const& transform)
{
    _transform = transform;
    _inverse_transform = _transform.inverse();
}


Level_element::Edit_type Level_element::is_user_editable() const
{
    return _user_editable;
}


void Level_element::set_user_editable(const Level_element::Edit_type b)
{
    _user_editable = b;
}


void Level_element::animate(const float timestep)
{
    if (_animations.empty()) return;

    for (Animation & a : _animations)
    {
        a.advance(timestep);
    }

    handle_animation();
}


void Level_element::add_animation(const Animation &animation)
{
    _animations.push_back(animation);
}


void Level_element::handle_animation()
{
    for (Animation const& a : _animations)
    {
        if (a.get_type() == Animation::Type::Position)
        {
            set_position(a.get_value());
        }
    }
}


void Level_element::notify_observers()
{
    for (auto o : _observers)
    {
        o->notify();
    }
}


void Level_element::add_observer(Notifiable *n)
{
    _observers.push_back(n);
}


void Level_element::remove_observer(Notifiable *n)
{
    _observers.erase(std::remove(_observers.begin(), _observers.end(), n), _observers.end());
}


void Level_element::reset()
{
    for (Animation & a : _animations)
    {
        a.reset();
    }

    handle_animation();

    _selected = false;
}


bool Level_element::is_persistent() const
{
    return _persistent;
}


void Level_element::set_persistent(const bool p)
{
    _persistent = p;
}


bool Level_element::is_selected() const
{
    return _selected;
}


void Level_element::set_selected(const bool s)
{
    _selected = s;
}


bool Level_element::does_intersect(const Level_element *element) const
{
    return (element->get_world_aabb().squaredExteriorDistance(get_world_aabb()) < 1e-6f);
}


void Level_element::add_property(const Parameter *parameter)
{
    _properties.add_parameter(parameter->get_name(), new Parameter(*parameter));
}


const Parameter_list &Level_element::get_parameters() const
{
    return _properties;
}


Parameter_list &Level_element::get_parameters()
{
    return _properties;
}



Plane_barrier::Plane_barrier(const Eigen::Vector3f &position, const Eigen::Vector3f &normal, const float strength, const float radius, const boost::optional<Eigen::Vector2f> &extent) :
    //        _plane(Eigen::Hyperplane<float, 3>(normal, position)), _strength(strength), _radius(radius), _extent(extent)
    _strength(strength), _radius(radius), _extent(extent)
{
    Eigen::Vector3f tangent, bitangent;

    create_orthogonal_cs(normal, tangent, bitangent);
    Eigen::Matrix3f m;
    m.col(0) = tangent;
    m.col(1) = bitangent;
    m.col(2) = normal;

    //        set_transform(Eigen::Transform<float, 3, Eigen::Affine>::Identity());
    set_transform(Eigen::Transform<float, 3, Eigen::Affine>(m));
    //        Eigen::Transform<float, 3, Eigen::Affine> t(Eigen::AngleAxisf(M_PI * 0.5f, cross(normal, Eigen::Vector3f::UnitZ())));
    //        set_transform(t);
    set_position(position);
}


Eigen::Vector3f Plane_barrier::calc_force_on_atom(const Atom &a) const
{
    Eigen::Vector3f const& normal = get_transform().linear().col(2);
    Eigen::Vector3f const local_pos = a.get_position() - get_position();

    float const signed_distance = normal.dot(local_pos);

    return falloff_function(signed_distance) * normal;
}

Eigen::Vector3f Plane_barrier::calc_force_on_molecule(const Molecule &m) const
{
    Eigen::Vector3f const& normal = get_transform().linear().col(2);
    Eigen::Vector3f const local_pos = m._x - get_position();

    float const signed_distance = normal.dot(local_pos);

    return falloff_function(signed_distance) * normal;
}

const boost::optional<Eigen::Vector2f> &Plane_barrier::get_extent() const
{
    return _extent;
}

void Plane_barrier::set_extent(const boost::optional<Eigen::Vector2f> &extent)
{
    _extent = extent;
}

Eigen::AlignedBox<float, 3> Plane_barrier::get_world_aabb() const
{
    return Eigen::AlignedBox<float, 3>();
}

void Plane_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

float Plane_barrier::falloff_function(const float distance) const
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




void Portal::handle_molecule_entering()
{
    _end_condition.set_num_captured_molecules(_end_condition.get_num_captured_molecules() + 1);
}

bool Portal::do_destroy_on_entering() const
{
    return _destroy_on_entering;
}

void Portal::set_destroy_on_entering(bool const destroy_on_entering)
{
    _destroy_on_entering = destroy_on_entering;
}

const Molecule_capture_condition &Portal::get_condition() const
{
    return _end_condition;
}


Molecule_capture_condition &Portal::get_condition()
{
    return _end_condition;
}


float Portal::get_score_factor() const
{
    return _score_factor;
}


void Portal::set_score_factor(const float score_factor)
{
    _score_factor = score_factor;
}


void Portal::reset()
{
    Level_element::reset();
    _end_condition.set_num_captured_molecules(0);
}


Box_portal::Box_portal(const Eigen::Vector3f &min, const Eigen::Vector3f &max)
{
    set_transform(Eigen::Transform<float, 3, Eigen::Affine>::Identity());

    set_position((min + max) * 0.5f);

    _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
}

bool Box_portal::contains(const Eigen::Vector3f &pos) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (pos - get_position());

    return (_box.exteriorDistance(local_pos) < 0.000001f);
}

const Eigen::AlignedBox<float, 3> &Box_portal::get_box() const
{
    return _box;
}

void Box_portal::set_size(const Eigen::Vector3f &extent)
{
    _box.min() = -extent * 0.5f;
    _box.max() =  extent * 0.5f;
}

Eigen::Vector3f Box_portal::get_extent() const
{
    return _box.max() - _box.min();
}

void Box_portal::init_particle_system()
{
    int const num_particles = get_condition().get_min_captured_molecules() - get_condition().get_num_captured_molecules();

    //        int get_min_captured_molecules() const

    //        int get_num_captured_molecules() const

    _particles.resize(num_particles);

    for (int i = 0; i < num_particles; ++i)
    {
        Eigen::Vector3f pos = _box.sample();

        Eigen::Vector3f const speed = Eigen::Vector3f::Random() * 10.0f;

        Particle p;
        p.position = pos;
        p.speed = speed;
        p.color = Color4(0.3f, 0.8f, 0.4f, 1.0f);

        _particles[i] = p;
    }
}

void Box_portal::animate(const float timestep)
{
    int const num_expected_particles = get_condition().get_min_captured_molecules() - get_condition().get_num_captured_molecules();

    if (num_expected_particles >= 0 && int(_particles.size()) > num_expected_particles)
    {
        _particles.resize(num_expected_particles);
    }

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
            float const absolute_speed = p.speed.norm();
            p.speed = p.speed.normalized() * std::min(absolute_speed, 10.0f);
        }
    }
}

void Box_portal::reset()
{
    Portal::reset();
    init_particle_system();
}

const std::vector<Particle> &Box_portal::get_particles() const
{
    return _particles;
}

void Box_portal::get_corner_min_max(const Eigen::AlignedBox<float, 3>::CornerType cornertype, Eigen::Vector3f &min, Eigen::Vector3f &max) const
{
    Eigen::Vector3f world_corner = get_transform() * _box.corner(cornertype) + get_position();

    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(world_corner[i], min[i]);
        max[i] = std::max(world_corner[i], max[i]);
    }
}

Eigen::AlignedBox<float, 3> Box_portal::get_world_aabb() const
{
    Eigen::Vector3f min(1e10f, 1e10f, 1e10f), max(-1e10f, -1e10f, -1e10f);

    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeft,      min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRight,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeft,         min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRight,        min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeftCeil,  min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRightCeil, min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeftCeil,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRightCeil,    min, max);

    return Eigen::AlignedBox<float, 3>(min, max);
}

void Box_portal::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

Box_barrier::Box_barrier(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float strength, const float radius) :
    //        _box(Eigen::AlignedBox<float, 3>(min, max)),
    _strength(strength), _radius(radius)
{
    set_transform(Eigen::Transform<float, 3, Eigen::Affine>::Identity());

    set_position((min + max) * 0.5f);

    _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
}

Eigen::Vector3f Box_barrier::calc_force_on_molecule(const Molecule &m) const
{
    float const distance_to_center = (m._x - get_position()).norm();

    if (distance_to_center > _box_radius + _radius) return Eigen::Vector3f(0.0f, 0.0f, 0.0f);

    Eigen::Vector3f const local_pos = get_inverse_transform() * (m._x - get_position());

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

const Eigen::AlignedBox<float, 3> &Box_barrier::get_box() const
{
    return _box;
}

void Box_barrier::set_size(const Eigen::Vector3f &extent)
{
    _box.min() = -extent * 0.5f;
    _box.max() =  extent * 0.5f;

    _box_radius = (_box.max() - _box.min()).norm() * 0.5f;
}

Eigen::Vector3f Box_barrier::get_extent() const
{
    return _box.max() - _box.min();
}

void Box_barrier::get_corner_min_max(const Eigen::AlignedBox<float, 3>::CornerType cornertype, Eigen::Vector3f &min, Eigen::Vector3f &max) const
{
    Eigen::Vector3f world_corner = get_transform() * _box.corner(cornertype) + get_position();

    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(world_corner[i], min[i]);
        max[i] = std::max(world_corner[i], max[i]);
    }
}

Eigen::AlignedBox<float, 3> Box_barrier::get_world_aabb() const
{
    Eigen::Vector3f min(1e10f, 1e10f, 1e10f), max(-1e10f, -1e10f, -1e10f);

    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeft,      min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRight,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeft,         min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRight,        min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeftCeil,  min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRightCeil, min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeftCeil,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRightCeil,    min, max);

    return Eigen::AlignedBox<float, 3>(min, max);
}

void Box_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

float Box_barrier::falloff_function(const float distance) const
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

Charged_barrier::Charged_barrier(Eigen::Vector3f const& min, Eigen::Vector3f const& max, float const strength, float const radius, float const charge) :
    Box_barrier(min, max, strength, radius), _charge(charge)
{
    add_property(new Parameter("charge", charge, -50.0f, 50.0f));

    set_property_values(_properties);
}

Eigen::Vector3f Charged_barrier::calc_force_on_molecule(const Molecule &m) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (m._x - get_position());

    Eigen::Vector3f closest_point;
    float distance;

    closest_point_to_box(_box, local_pos, closest_point, distance);

    if (distance < 0.000001f)
    {
        return _strength * (get_transform() * local_pos.normalized());
    }

    return (_strength * falloff_function(distance) + _charge * m._accumulated_charge) * (get_transform() * (local_pos - closest_point).normalized());
}

float Charged_barrier::get_charge() const
{
    return _charge;
}

void Charged_barrier::set_property_values(const Parameter_list &properties)
{
    _charge = properties["charge"]->get_value<float>();
}

void Charged_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}


Tractor_barrier::Tractor_barrier(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float strength) :
    Box_barrier(min, max, strength, 0.0f), _last_created_particle(0.0f), _rotation_angle(0.0f)
{
    add_property(new Parameter("tractor_strength", 2.0f, -10.0f, 10.0f));
    add_property(new Parameter("radius", 5.0f, 5.0f, 100.0f));

    set_property_values(_properties);
}

Eigen::Vector3f Tractor_barrier::calc_force_on_molecule(const Molecule &m) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (m._x - get_position());

    if (local_pos[2] > _box.max()[2] || local_pos[2] < _box.min()[2]) return Eigen::Vector3f::Zero();

    //        float const distance = std::abs(local_pos[0] - (_box.max()[0] - _box.min()[0]) * 0.5f);

    float const distance = std::abs(local_pos[0]);

    //        if (distance < 0.000001f)
    //        {
    //            return _strength * (get_transform() * local_pos.normalized());
    //        }

    return (_tractor_strength * falloff_function(distance)) * (get_transform() * Eigen::Vector3f::UnitX());
}

bool Tractor_barrier::is_dead(const Particle &p)
{
    return p.age > 0.99f;
}

void Tractor_barrier::animate(const float timestep)
{
    _particles.erase(std::remove_if(_particles.begin(), _particles.end(), Tractor_barrier::is_dead), _particles.end());

    float const area_measure = sqrt(_box.sizes()[1] * _box.sizes()[2]);

    float const max_speed = _tractor_strength * 2.0f;

    if (_last_created_particle > 10.0f / (area_measure * std::abs(_tractor_strength)))
    {
        _last_created_particle = 0.0f;

        Particle p;
        p.age = 0.0f;
        p.color = Color4(1.0f, 0.3f, 0.05f, 1.0f);

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

float Tractor_barrier::get_rotation_angle() const
{
    return _rotation_angle;
}

const std::list<Particle> &Tractor_barrier::get_particles()
{
    return _particles;
}

void Tractor_barrier::set_property_values(const Parameter_list &properties)
{
    _tractor_strength = properties["tractor_strength"]->get_value<float>();
    _radius   = properties["radius"]->get_value<float>();
}

void Tractor_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}


Moving_box_barrier::Moving_box_barrier(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float strength, const float radius) :
    Box_barrier(min, max, strength, radius)
{
    _animations.push_back(Animation(get_position(), get_position(), 2.0f, Animation::Type::Position));
}

Animation &Moving_box_barrier::get_animation()
{
    return _animations[0];
}

void Moving_box_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}


Eigen::Vector3f Blow_barrier::calc_force_on_atom(const Atom &a) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (a.get_position() - get_position());

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

void Blow_barrier::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

Blow_barrier::Axis Blow_barrier::get_direction() const
{
    return _direction;
}

float Brownian_element::get_strength() const
{
    return _strength;
}

void Brownian_element::set_strength(const float strength)
{
    _strength = strength;
}

float Brownian_element::get_radius() const
{
    return _radius;
}

void Brownian_element::set_radius(const float radius)
{
    _radius = radius;
}

float Brownian_plane::get_brownian_motion_factor(const Eigen::Vector3f &point) const
{
    return falloff_function(_plane.absDistance(point));
}

const Eigen::Hyperplane<float, 3> &Brownian_plane::get_plane() const
{
    return _plane;
}

void Brownian_plane::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

float Brownian_plane::falloff_function(const float distance) const
{
    return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
}


Brownian_box::Brownian_box(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float strength, const float radius) :
    Brownian_element(strength, radius)
{
    set_transform(Eigen::Transform<float, 3, Eigen::Affine>::Identity());

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

float Brownian_box::get_brownian_motion_factor(const Eigen::Vector3f &point) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (point - get_position());

    //        return falloff_function(_box.exteriorDistance(point));
    return falloff_function(_box.exteriorDistance(local_pos));
}

void Brownian_box::set_size(const Eigen::Vector3f &extent)
{
    _box.min() = -extent * 0.5f;
    _box.max() =  extent * 0.5f;
}

Eigen::Vector3f Brownian_box::get_extent() const
{
    return _box.max() - _box.min();
}

const Eigen::AlignedBox<float, 3> &Brownian_box::get_box() const
{
    return _box;
}

void Brownian_box::animate(const float timestep)
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

const std::vector<Particle> &Brownian_box::get_particles() const
{
    return _particles;
}

void Brownian_box::get_corner_min_max(const Eigen::AlignedBox<float, 3>::CornerType cornertype, Eigen::Vector3f &min, Eigen::Vector3f &max) const
{
    Eigen::Vector3f world_corner = get_transform() * _box.corner(cornertype) + get_position();

    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(world_corner[i], min[i]);
        max[i] = std::max(world_corner[i], max[i]);
    }
}

Eigen::AlignedBox<float, 3> Brownian_box::get_world_aabb() const
{
    Eigen::Vector3f min(1e10f, 1e10f, 1e10f), max(-1e10f, -1e10f, -1e10f);

    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeft,      min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRight,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeft,         min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRight,        min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomLeftCeil,  min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::BottomRightCeil, min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopLeftCeil,     min, max);
    get_corner_min_max(Eigen::AlignedBox<float, 3>::TopRightCeil,    min, max);

    return Eigen::AlignedBox<float, 3>(min, max);
}

void Brownian_box::set_property_values(const Parameter_list &properties)
{
    _strength = properties["strength"]->get_value<float>();
    _radius   = properties["radius"]->get_value<float>();
}

void Brownian_box::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

float Brownian_box::falloff_function(const float distance) const
{
    return _strength * wendland_2_1(std::min(1.0f, (distance / _radius)));
}


void Particle_system_element::init(const Molecule &m)
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
        pos += a.get_position();

        Eigen::Vector3f const speed = Eigen::Vector3f::Random() * 10.0f;

        Particle p;
        p.position = pos;
        p.speed = speed;
        p.color = Color4(Atom::atom_colors[int(a._type)], 1.0f);

        _particles[i] = p;
    }
}

void Particle_system_element::animate(const float timestep)
{
    _age += timestep;

    for (Particle & p : _particles)
    {
        p.position += p.speed * timestep;
        p.speed += Eigen::Vector3f::Random() * timestep;
        //            p.speed = p.speed.normalized();
    }
}

bool Particle_system_element::is_dead() const
{
    return _age > _life_time;
}

float Particle_system_element::get_life_percentage()
{
    return _age / _life_time;
}

const std::vector<Particle> &Particle_system_element::get_particles() const
{
    return _particles;
}

Eigen::AlignedBox<float, 3> Particle_system_element::get_world_aabb() const
{
    return Eigen::AlignedBox<float, 3>();
}

void Particle_system_element::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}



bool Sphere_portal::contains(Eigen::Vector3f const& pos) const
{
    Eigen::Vector3f const local_pos = get_inverse_transform() * (pos - get_position());

    Eigen::Vector3f const extent = _box.sizes() * 0.5f;

    return (local_pos[0] * local_pos[0]) / (extent[0] * extent[0]) +
            (local_pos[1] * local_pos[1]) / (extent[1] * extent[1]) +
            (local_pos[2] * local_pos[2]) / (extent[2] * extent[2]) <= 1.0f;
}


void Portal::start_update()
{
    if (!_destroy_on_entering)
    {
        _end_condition.set_num_captured_molecules(0);
    }
}

void Sphere_portal::reset()
{
    Portal::reset();
    init_particle_system();
}

void Sphere_portal::init_particle_system()
{
    int const num_particles = get_condition().get_min_captured_molecules() - get_condition().get_num_captured_molecules();

    //        int get_min_captured_molecules() const

    //        int get_num_captured_molecules() const

    _particles.resize(num_particles);

    for (int i = 0; i < num_particles; ++i)
    {
        Eigen::Vector3f pos = _box.sample();

        Eigen::Vector3f const speed = Eigen::Vector3f::Random() * 10.0f;

        Particle p;
        p.position = pos;
        p.speed = speed;
        p.color = Color4(0.3f, 0.8f, 0.4f, 1.0f);

        _particles[i] = p;
    }
}

void Sphere_portal::animate(const float timestep)
{
    int const num_expected_particles = get_condition().get_min_captured_molecules() - get_condition().get_num_captured_molecules();

    if (num_expected_particles >= 0 && int(_particles.size()) > num_expected_particles)
    {
        _particles.resize(num_expected_particles);
    }

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
            float const absolute_speed = p.speed.norm();
            p.speed = p.speed.normalized() * std::min(absolute_speed, 10.0f);
        }
    }
}

void Sphere_portal::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}
