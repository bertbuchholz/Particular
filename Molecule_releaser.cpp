#include "Molecule_releaser.h"

Molecule_releaser::Molecule_releaser(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float first_release, const float interval) :
    _first_release(first_release),
    _last_release(first_release),
    _interval(interval),
    _num_max_molecules(100),
    _num_released_molecules(0),
    _next_molecule_prepared(false),
    _particle_duration(0.5f),
    _animation_count(0.0f)
{
    set_transform(Eigen::Transform<float, 3, Eigen::Affine>::Identity());

    set_position((min + max) * 0.5f);

    _box = Eigen::AlignedBox<float, 3>(min - get_position(), max - get_position());
}


void Molecule_releaser::animate(const float timestep)
{
    for (Targeted_particle_system & p : _particles)
    {
        p.animate(timestep);
    }

    _animation_count += timestep / _interval;

    if (_animation_count > 1.0f)
    {
        _animation_count = 0.0f;
    }
}

Targeted_particle_system Molecule_releaser::init_particle_system(const Molecule &m, const Eigen::Vector3f &local_pos)
{
    std::vector<Targeted_particle> particles;

    int const num_particles = 100;

    Halton halton2(2);
    Halton halton3(3);
    Halton halton5(5);

    particles.resize(num_particles);

    for (int i = 0; i < num_particles; ++i)
    {
        Atom const& a = m._atoms[std::min(int(m._atoms.size() - 1), int(halton2.getNext() * m._atoms.size()))];

        float const theta = 2.0f * std::acos(std::sqrt(1.0f - halton3.getNext()));
        float const phi = 2.0f * M_PI * halton5.getNext();

        Eigen::Vector3f pos(std::sin(theta) * std::cos(phi),
                            std::sin(theta) * std::sin(phi),
                            std::cos(theta));

        pos *= a._radius;
        pos += a._r_0 + local_pos;

        Eigen::Vector3f const speed = Eigen::Vector3f::Random() * 10.0f;

        Targeted_particle p;
        p.position = _box.sample();
        p.target = pos;
        p.speed = speed;
        p.color = Color4(Atom::atom_colors[int(a._type)], 1.0f);
        p.age = 0.0f;

        particles[i] = p;
    }

    Targeted_particle_system p_system(_particle_duration);

    p_system.init(particles);

    return p_system;
}

bool Molecule_releaser::check_do_release(const float time)
{
    float const next_release = _last_release + _interval;

    _particles.erase(std::remove_if(_particles.begin(), _particles.end(), Targeted_particle_system::is_dead),
                     _particles.end());

    if (next_release - _particle_duration < time && !_next_molecule_prepared)
    {
        _next_molecule_prepared = true;

        _next_molecule = Molecule::create(_molecule_type);

        // get random position in the box (minus some margin), create molecule and give it a certain speed towards the release_axis
        Eigen::Vector3f local_pos = _box.sample() * 0.8f;
        local_pos[0] = _box.center()[0] + _box.sizes()[0] * 0.5f;

        _next_molecule._x = get_transform() * local_pos + get_position();
        _next_molecule._P = get_transform() * Eigen::Vector3f(1.0f, 0.0f, 0.0f) * (4.0f + 4.0f * std::rand() / float(RAND_MAX));

        _particles.push_back(init_particle_system(_next_molecule, local_pos));
    }

    return (next_release < time && _num_released_molecules < _num_max_molecules && _next_molecule_prepared);
}

Molecule Molecule_releaser::release(const float time)
{
//    assert(_next_molecule._x[0] < -1.0f);

    _last_release = time;

    ++_num_released_molecules;

    _next_molecule_prepared = false;

    return _next_molecule;
}

const Eigen::AlignedBox<float, 3> &Molecule_releaser::get_box() const
{
    return _box;
}

void Molecule_releaser::set_size(const Eigen::Vector3f &extent)
{
    _box.min() = -extent * 0.5f;
    _box.max() =  extent * 0.5f;
}

Eigen::Vector3f Molecule_releaser::get_extent() const
{
    return _box.max() - _box.min();
}

void Molecule_releaser::set_molecule_type(const std::string &molecule_type)
{
    _molecule_type = molecule_type;
}

const std::string &Molecule_releaser::get_molecule_type() const
{
    return _molecule_type;
}

int Molecule_releaser::get_num_max_molecules() const
{
    return _num_max_molecules;
}

void Molecule_releaser::set_num_max_molecules(const int num_max_molecules)
{
    _num_max_molecules = num_max_molecules;
}

float Molecule_releaser::get_interval() const
{
    return _interval;
}

void Molecule_releaser::set_interval(const float interval)
{
    _interval = interval;
}

float Molecule_releaser::get_first_release() const
{
    return _first_release;
}

void Molecule_releaser::set_first_release(const float first_release)
{
    _first_release = first_release;
}

int Molecule_releaser::get_num_released_molecules() const
{
    return _num_released_molecules;
}

const std::vector<Targeted_particle_system> &Molecule_releaser::get_particle_systems() const
{
    return _particles;
}

float Molecule_releaser::get_animation_count() const
{
    return _animation_count;
}

void Molecule_releaser::get_corner_min_max(const Eigen::AlignedBox<float, 3>::CornerType cornertype, Eigen::Vector3f &min, Eigen::Vector3f &max) const
{
    Eigen::Vector3f world_corner = get_transform() * _box.corner(cornertype) + get_position();

    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(world_corner[i], min[i]);
        max[i] = std::max(world_corner[i], max[i]);
    }
}

Eigen::AlignedBox<float, 3> Molecule_releaser::get_world_aabb() const
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

void Molecule_releaser::accept(const Level_element_visitor *visitor)
{
    visitor->visit(this);
}

void Molecule_releaser::reset()
{
    Level_element::reset();
    _last_release = _first_release;
    _num_released_molecules = 0;
    _next_molecule_prepared = false;
    _animation_count = 0.0f;
}

Atom_cannon::Atom_cannon(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const float first_release, const float interval, const float speed, const float charge) :
    Molecule_releaser(min, max, first_release, interval),
    _speed(speed),
    _charge(charge)
{
    add_property(new Parameter("charge", 0.0f, -1.0f, 1.0f));
    add_property(new Parameter("speed", 1.0f, 0.5f, 100.0f));
}

void Atom_cannon::set_property_values(const Parameter_list &properties)
{
    _charge = properties["charge"]->get_value<float>();
    _speed = properties["speed"]->get_value<float>();
}

Molecule Atom_cannon::release(const float time)
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
    m._P = get_transform() * (aim_pos - local_pos).normalized() * _speed; // start with an initial impulse

    ++_num_released_molecules;

    return m;
}
