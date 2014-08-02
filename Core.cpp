#include "Core.h"

#include <QMessageBox>
#include <QFrame>
#include <QVBoxLayout>

#include <Q_parameter_bridge.h>

#include "Main_options_window.h"
#include "Molecule_releaser.h"
#include "Data_config.h"

struct Atom_averager
{
    Atom operator() (std::vector<Atom const*> const& atoms) const
    {
        Atom result;

        result._type = Atom::Type::H;

        result.set_position(Eigen::Vector3f::Zero());

        result._mass = 0.0f;
        result._charge = 0.0f;
        result._radius = 0.0f;

        float charge_abssum = 0.0f;

        for (Atom const* a : atoms)
        {
            float abs_charge = std::abs(a->_charge);
            abs_charge = abs_charge > 0.001f ? abs_charge : 1.0f;

            result.set_position(result.get_position() + a->get_position() * abs_charge); // weigh the position by the charge
            charge_abssum += abs_charge;

            result._mass += a->_mass;
            result._charge += a->_charge;
            result._radius += a->_radius * a->_radius * a->_radius;
        }

        if (charge_abssum < 0.001f)
        {
            std::cout << "charge_abssum " << charge_abssum << " #atoms " << atoms.size() << std::endl;
            assert(false);
        }

        result.set_position(result.get_position() / charge_abssum);

        result._radius = std::pow(result._radius, 0.3333f);

        // not averaging, accumulating!
//            result._mass /= atoms.size();
//            result._charge /= atoms.size();
//            result._radius /= atoms.size();

        return result;
    }
};


Core::Core() :
    _game_state(Game_state::Unstarted),
    _previous_game_state(Game_state::Unstarted),
    _molecule_id_counter(0),
    _current_time(0.0f),
    _last_sensor_check(0.0f),
    _animation_interval(0.04f),
    _last_animation_time(0.0f)
  //        _molecule_hash(Molecule_atom_hash(100, 4.0f))
{
    std::function<void(void)> update_variables = std::bind(&Core::update_variables, this);

    _parameters.add_parameter(new Parameter("levels", std::string(""), update_variables));

    _parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&Core::toggle_simulation, this)));
    _parameters.add_parameter(new Parameter("Mass Factor", 0.1f, 0.1f, 1.0f, update_variables));
    _parameters.add_parameter(new Parameter("do_constrain_forces", true, update_variables));
    _parameters.add_parameter(new Parameter("max_force", 10.0f, 0.1f, 500.0f, update_variables));
    _parameters.add_parameter(new Parameter("max_force_distance", 10.0f, 1.0f, 1000.0f, update_variables));

    _parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&Core::update_physics_timestep, this)));
    _parameters["physics_timestep_ms"]->set_hidden(false);
    _parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update_variables));
    _parameters["physics_speed"]->set_hidden(true);
    _parameters.add_parameter(new Parameter("Use midpoint", true, update_variables));


    Parameter_registry<Atomic_force>::create_multi_select_instance(&_parameters, "Atomic Force Type", update_variables);

    Main_options_window::get_instance()->add_parameter_list("Core", _parameters);

    _physics_timer.setInterval(_parameters["physics_timestep_ms"]->get_value<int>());

    connect(&_physics_timer, SIGNAL(timeout()), this, SLOT(update_physics()));

    load_default_simulation_settings();
    init_game();
}

Core::~Core()
{
    Main_options_window::get_instance()->remove_parameter_list("Core");
}


Eigen::Vector3f Core::apply_forces_brute_force(const Atom &receiver_atom) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    for (Molecule const& sender : _level_data._molecules)
    {
        //            if (parent_molecule_id == sender.get_id()) continue;

        for (Atom const& sender_atom : sender._atoms)
        {
            if (receiver_atom._parent_id == sender_atom._parent_id) continue;

            float const dist = (receiver_atom.get_position() - sender_atom.get_position()).norm();

            if (dist > 1e-4f && dist < _max_force_distance)
            {
                force_i += calc_forces_between_atoms(receiver_atom, sender_atom);
            }
        }
    }

    return force_i;
}


Eigen::Vector3f Core::apply_forces_from_vector(const Atom &receiver_atom, const std::vector<const Atom *> &atoms) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    for (Atom const* a : atoms)
    {
        force_i += calc_forces_between_atoms(receiver_atom, *a);
    }

    return force_i;
}


Eigen::Vector3f Core::force_on_atom(const Atom &receiver_atom) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    //        force_i = apply_forces_brute_force(receiver_atom, parent_molecule_id);

    // per atom force application, replaced by per molecule application
    //        for (Barrier const* b : _level_data._barriers)
    //        {
    //            force_i += b->calc_force(receiver_atom);
    //        }

    if (std::isnan(force_i[0]))
    {
        std::cout << __FUNCTION__ << " isnan, atom: " << &receiver_atom << std::endl;
        force_i = Eigen::Vector3f::Zero();
    }

    //        assert(!std::isnan(force_i[0]));

    return force_i;
}


void Core::compute_force_and_torque(Molecule &receiver, int & atom_index, std::vector<Eigen::Vector3f> const& forces_on_atoms)
{
//    float const translation_to_rotation_ratio = 1.0f;
    float const translation_to_rotation_ratio = 0.1f;

    receiver._force.setZero();
    receiver._torque.setZero();

    for (Atom const& receiver_atom : receiver._atoms)
    {
        Eigen::Vector3f const& force_i = forces_on_atoms[atom_index];

        receiver._force += force_i;
        receiver._torque += translation_to_rotation_ratio * (receiver_atom.get_position() - receiver._x).cross(force_i);

        ++atom_index;
    }

    for (Barrier const* b : _level_data._barriers)
    {
        receiver._force += b->calc_force_on_molecule(receiver);
    }

//    float const brownian_translation_factor = _level_data.get_temperature(receiver._x);
//    float const brownian_rotation_factor = translation_to_rotation_ratio * brownian_translation_factor;

//    Eigen::Vector3f random_dir_f = _random_generator.generator_unit_vector();
//    Eigen::Vector3f random_dir_t = _random_generator.generator_unit_vector();

//    receiver._force  += std::max(0.0f, brownian_translation_factor) * random_dir_f;
//    receiver._torque += std::max(0.0f, brownian_rotation_factor)    * random_dir_t;

    for (auto const& f : _level_data._external_forces)
    {
        receiver._force += f.second._force * receiver._mass * _mass_factor; // FIXME: using mass here only true if "force" is actually an acceleration (F = m * a)
        //            receiver._torque += (f._origin - receiver._x).cross(f._force);
    }

    // TODO: do the search for affected molecules somewhat less brute force
    for (Molecule_external_force const& f : _molecule_external_forces)
    {
        if (receiver.get_id() == f._molecule_id)
        {
            receiver._force += f._force;
            receiver._torque += translation_to_rotation_ratio * (f._origin - receiver._x).cross(f._force);
        }
    }

    if (_user_force._end_time > _current_time && receiver.get_id() == _user_force._molecule_id)
    {
        // lower all other forces to give better control
        receiver._force *= 0.2f;
        receiver._torque *= 0.01f;

        receiver._force += _user_force._force;
        Eigen::Vector3f constrained_torque = (_user_force._origin - receiver._x).cross(_user_force._force) * 0.01f;
        receiver._torque += _user_force._plane_normal * constrained_torque.dot(_user_force._plane_normal);
    }

    if (_do_constrain_forces)
    {
        float const force_size = receiver._force.norm();
        float const torque_size = receiver._torque.norm();

        if (torque_size > _max_force * translation_to_rotation_ratio)
        {
            receiver._torque = ((_max_force * translation_to_rotation_ratio) / torque_size) * receiver._torque;
        }

        if (force_size > _max_force)
        {
            receiver._force  = (_max_force / force_size)  * receiver._force;
        }
    }

//    std::cout << "rcv.torque: " << receiver._torque << std::endl << "rcv.omega: " << receiver._omega << std::endl;

    receiver._force -= _level_data._translation_damping * receiver._v;
//    receiver._torque += -_level_data._rotation_damping * translation_to_rotation_ratio * receiver._omega;
    receiver._torque -= _level_data._rotation_damping * receiver._omega;

//    assert(!std::isnan(receiver._force[0]));
//    assert(!std::isnan(receiver._force[1]));
//    assert(!std::isnan(receiver._force[2]));
//    assert(!std::isnan(receiver._torque[0]));
//    assert(!std::isnan(receiver._torque[1]));
//    assert(!std::isnan(receiver._torque[2]));

//    std::cout << "after subtr. rcv.torque: " << receiver._torque << std::endl;
}


void Core::check_molecules_in_portals()
{
    for (Portal * p : _level_data._portals)
    {
        p->start_update();
    }

    auto molecule_iter = std::begin(_level_data._molecules);

    while (molecule_iter != std::end(_level_data._molecules))
    {
        Molecule & m = *molecule_iter;

        bool has_been_removed = false;

        for (Portal * p : _level_data._portals)
        {
            if (p->contains(m._x) && !has_been_removed)
            {
                p->handle_molecule_entering();

                if (p->do_destroy_on_entering())
                {
                    Particle_system_element * p = new Particle_system_element;
                    p->init(m);

                    _level_data._particle_system_elements.push_back(p);

                    _molecule_id_to_molecule_map.erase(m.get_id());

                    molecule_iter = _level_data._molecules.erase(std::remove_if(_level_data._molecules.begin(), _level_data._molecules.end(), Compare_by_id(m.get_id())),
                                                                 _level_data._molecules.end());

                    has_been_removed = true;
                }

//                std::cout << __FUNCTION__ << " molecule in portal" << std::endl;
            }
        }

        if (!has_been_removed)
        {
            ++molecule_iter;
        }
    }
}


void update_temperature_grid(Level_data const& level_data, Frame_buffer<float> & grid)
{
    float const game_field_width  = level_data._parameters["Game Field Width"]->get_value<float>();
    float const game_field_height = level_data._parameters["Game Field Height"]->get_value<float>();

    float const temp_min = level_data._parameters["Temperature"]->get_min<float>();
    float const temp_max = level_data._parameters["Temperature"]->get_max<float>();

    float const overall_temperature = level_data._parameters["Temperature"]->get_value<float>();

    for (int i = 0; i < grid.get_size(); ++i)
    {
        int x, y;
        grid.get_coordinates(i, x, y);

        float const normalized_x = x / float(grid.get_width() - 1)  * 2.0f - 1.0f;
        float const normalized_z = y / float(grid.get_height() - 1) * 2.0f - 1.0f;

        Eigen::Vector3f pos(normalized_x * game_field_width * 0.5f, 0.0f, normalized_z * game_field_height * 0.5f);

        float temperature = overall_temperature;

        for (Brownian_element const* element : level_data._brownian_elements)
        {
            temperature += element->get_brownian_motion_factor(pos);
        }

        grid.set_data(i, into_range(temperature, temp_min, temp_max));
    }
}


void Core::update(const float time_step)
{
    _current_time += time_step;

    float const time_since_animation_update = _current_time - _last_animation_time;

    if (time_since_animation_update > _animation_interval)
    {
//        std::cout << __FUNCTION__ << " " << time_since_animation_update << std::endl;

        _last_animation_time = _current_time;

        update_level_elements(time_since_animation_update);
    }

    if (_parameters["Use midpoint"]->get_value<bool>())
    {
        midpoint_integration(_level_data._molecules, time_step);
    }
    else
    {
        update_physics_elements(time_step);
    }
}


void Core::update_level_elements(const float time_step)
{
    if (!_parameters["Toggle simulation"]->get_value<bool>()) return;

    _level_data._particle_system_elements.erase(std::remove_if(_level_data._particle_system_elements.begin(), _level_data._particle_system_elements.end(), Particle_system_element::check_if_dead()),
                                                _level_data._particle_system_elements.end());

    _molecule_external_forces.erase(std::remove_if(_molecule_external_forces.begin(), _molecule_external_forces.end(), Check_duration(_current_time)),
                                    _molecule_external_forces.end());

    for (Molecule_releaser * m : _level_data._molecule_releasers)
    {
        if (m->check_do_release(_current_time))
        {
            add_molecule(m->release(_current_time));
        }
    }

    for (Level_element * e : _level_data._particle_system_elements)
    {
        e->animate(time_step);
    }

    for (boost::shared_ptr<Level_element> const& e : _level_data._level_elements)
    {
        e->animate(time_step);
    }

    update_temperature_grid(_level_data, _level_data._temperature_grid);
    _gpu_force->update_temperature_tex(_level_data._temperature_grid);

    if (_current_time - _last_sensor_check > _sensor_data.get_check_interval())
    {
        check_molecules_in_portals(); // portal checks don't need to be done that often

        _last_sensor_check = _current_time;
        if (_game_state == Game_state::Running)
        {
            do_sensor_check();
        }
    }

    if (_game_state == Game_state::Running && check_is_finished())
    {
        set_new_game_state(Game_state::Finished);
    }
}


void Core::do_physics_step(std::list<Molecule> & molecules, float const current_time, float const time_step)
{
    std::vector<Eigen::Vector3f> const& forces_on_atoms = _gpu_force->calc_forces(molecules,
            _parameters["Atomic Force Type/Coulomb Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Radius Factor"]->get_value<float>(),
            current_time, QVector2D(_level_data._game_field_width, _level_data._game_field_height));

    int atom_index = 0;

    for (Molecule & molecule : molecules)
    {
        compute_force_and_torque(molecule, atom_index, forces_on_atoms);

        molecule._x += molecule._v * time_step;

        Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
        Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
        molecule._q = add(molecule._q, scale(q_dot, time_step));

        molecule._P += molecule._force * time_step;
        molecule._L += molecule._torque * time_step;

        molecule.from_state(Body_state(), _mass_factor);
    }
}


void Core::midpoint_integration(std::list<Molecule> & molecules, float const time_step)
{
    std::list<Molecule> molecules_at_half_time = molecules;
    do_physics_step(molecules_at_half_time, _current_time, time_step * 0.5f);

    std::vector<Eigen::Vector3f> const& forces_on_atoms_at_half_time = _gpu_force->calc_forces(molecules_at_half_time,
            _parameters["Atomic Force Type/Coulomb Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Radius Factor"]->get_value<float>(),
            _current_time + 0.5f * time_step, QVector2D(_level_data._game_field_width, _level_data._game_field_height));

    int atom_index = 0;

    for (Molecule & m : molecules_at_half_time)
    {
        compute_force_and_torque(m, atom_index, forces_on_atoms_at_half_time);
    }

    auto iter_molecule = molecules.begin();
    auto iter_molecule_half_time = molecules_at_half_time.cbegin();

    for ( ; iter_molecule != molecules.end(); ++iter_molecule, ++iter_molecule_half_time)
    {
        Molecule & molecule = *iter_molecule;
        Molecule const& molecule_at_halt_time = *iter_molecule_half_time;

        molecule._x += molecule_at_halt_time._v * time_step;

        Eigen::Quaternion<float> omega_quaternion(0.0f, molecule_at_halt_time._omega[0], molecule_at_halt_time._omega[1], molecule_at_halt_time._omega[2]);
        Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule_at_halt_time._q, 0.5f);
        molecule._q = add(molecule._q, scale(q_dot, time_step));

        molecule._P += molecule_at_halt_time._force * time_step;
        molecule._L += molecule_at_halt_time._torque * time_step;

        molecule.from_state(Body_state(), _mass_factor);
    }

    assert(iter_molecule_half_time == molecules_at_half_time.end());
}


void Core::update_physics_elements(const float time_step)
{
    bool const time_debug = false;

    int elapsed_milliseconds;
    std::chrono::steady_clock::time_point timer_start, timer_end;

    if (time_debug)
    {
        timer_start = std::chrono::steady_clock::now();
    }

    std::vector<Eigen::Vector3f> const& forces_on_atoms = _gpu_force->calc_forces(_level_data._molecules,
            _parameters["Atomic Force Type/Coulomb Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Strength"]->get_value<float>(),
            _parameters["Atomic Force Type/Van der Waals Force/Radius Factor"]->get_value<float>(),
            _current_time, QVector2D(_level_data._game_field_width, _level_data._game_field_height));

    if (time_debug)
    {
        timer_end = std::chrono::steady_clock::now();

        elapsed_milliseconds = std::chrono::duration <double, std::milli>(timer_end - timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "gpu_force update: " << elapsed_milliseconds << std::endl;
        }
    }

    if (time_debug)
    {
        timer_start = std::chrono::steady_clock::now();
    }

    int atom_index = 0;

    for (Molecule & m : _level_data._molecules)
    {
        compute_force_and_torque(m, atom_index, forces_on_atoms);
    }

    if (time_debug)
    {
        timer_end = std::chrono::steady_clock::now();

        elapsed_milliseconds = std::chrono::duration <double, std::milli>(timer_end - timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "compute_force_and_torque(): " << elapsed_milliseconds << std::endl;
        }
    }

    if (time_debug)
    {
        timer_start = std::chrono::steady_clock::now();
    }

    for (Molecule & molecule : _level_data._molecules)
    {
        molecule._x += molecule._v * time_step;

//        assert(!std::isnan(molecule._q.w()) && !std::isinf(molecule._q.w()));
//        assert(!std::isnan(molecule._q.x()) && !std::isinf(molecule._q.x()));
//        assert(!std::isnan(molecule._q.y()) && !std::isinf(molecule._q.y()));
//        assert(!std::isnan(molecule._q.z()) && !std::isinf(molecule._q.z()));


//        assert(!std::isnan(molecule._omega[0]) && !std::isinf(molecule._omega[0]));
//        assert(!std::isnan(molecule._omega[1]) && !std::isinf(molecule._omega[1]));
//        assert(!std::isnan(molecule._omega[2]) && !std::isinf(molecule._omega[2]));


        Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);

//        std::cout << "omega: " << omega_quaternion.coeffs() << std::endl << "_q:" << molecule._q.coeffs() << std::endl;

        Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);

//        assert(!std::isnan(q_dot.w()) && !std::isinf(q_dot.w()));

        molecule._q = add(molecule._q, scale(q_dot, time_step));

//        assert(!std::isnan(molecule._q.w()) && !std::isinf(molecule._q.w()));

        molecule._P += molecule._force * time_step;

        molecule._L += molecule._torque * time_step;

//        std::cout << "L: " << molecule._L << std::endl;

        molecule.from_state(Body_state(), _mass_factor);
    }

    if (time_debug)
    {
        timer_end = std::chrono::steady_clock::now();

        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "update molecules: " << elapsed_milliseconds << std::endl;
        }
    }
}


void Core::do_sensor_check()
{
    float num_collected_molecules = 0.0f;
    float num_released_molecules = 0.0f;
    float average_temperature = 0.0f;
    float energy_consumption = 0.0f;

    for (Portal const* p : _level_data._portals)
    {
        Molecule_capture_condition const& condition = p->get_condition();
        num_collected_molecules += condition.get_num_captured_molecules();
    }

    for (Molecule_releaser const* p : _level_data._molecule_releasers)
    {
        num_released_molecules += p->get_num_released_molecules();
    }

    // TODO: add power consumption of tractors
//    for (Brownian_element const* p : _level_data._brownian_elements)
//    {
//        Eigen::AlignedBox3f const world_aabb = p->get_world_aabb();
//        float const volume = (world_aabb.sizes()[0] + p->get_radius()) * (world_aabb.sizes()[2] + p->get_radius());

//        energy_consumption += std::abs(p->get_strength()) * volume;
//    }

    float const max_energy_use = _level_data._game_field_height * _level_data._game_field_width;

    for (boost::shared_ptr<Level_element> const& l : _level_data._level_elements)
    {
        energy_consumption += std::min(l->get_energy_use() / max_energy_use, 1.0f) * 2.0f;
    }

    for (float const temp : _level_data._temperature_grid.get_data())
    {
        assert(is_in_range(temp, -50.0f, 50.0f));
        average_temperature += temp;
    }

    average_temperature /= float(_level_data._temperature_grid.get_size());

    assert(is_in_range(average_temperature, -50.0f, 50.0f));

    _sensor_data.add_value(Sensor_data::Type::AvgTemp, average_temperature);
    _sensor_data.add_value(Sensor_data::Type::ColMol, num_collected_molecules);
    _sensor_data.add_value(Sensor_data::Type::RelMol, num_released_molecules);
    _sensor_data.add_value(Sensor_data::Type::EnergyCon, energy_consumption);

    _sensor_data.update_energy_bonus();
}


bool Core::check_is_finished() const
{
    End_condition::State end_state = End_condition::State::Finished;

    for (Portal const* p : _level_data._portals)
    {
        End_condition const& condition = p->get_condition();

        if (condition.check_state(_level_data._molecules) == End_condition::State::Finished && condition.get_type() == End_condition::Type::Or)
        {
            end_state = End_condition::State::Finished;
            break;
        }

        if (condition.check_state(_level_data._molecules) == End_condition::State::Not_finished && condition.get_type() == End_condition::Type::And)
        {
            end_state = End_condition::State::Not_finished;
            break;
        }
    }

    return (end_state == End_condition::State::Finished);
}


void Core::add_molecule(Molecule molecule)
{
    molecule.set_id(_molecule_id_counter);
    _level_data._molecules.push_back(molecule);
    _molecule_id_to_molecule_map[_molecule_id_counter] = &_level_data._molecules.back();
    ++_molecule_id_counter;
}


Eigen::Vector3f Core::calc_forces_between_atoms(const Atom &a_0, const Atom &a_1) const
{
    Eigen::Vector3f resulting_force(0.0f, 0.0f, 0.0f);

    for (std::unique_ptr<Atomic_force> const& force : _atomic_forces)
    {
        resulting_force += force->calc_force_between_atoms(a_0, a_1);
    }

    return resulting_force;
}


//boost::optional<const Molecule &> Core::get_molecule(const int id) const
//{
//    auto iter = _molecule_id_to_molecule_map.find(id);

//    if (iter != _molecule_id_to_molecule_map.end())
//    {
//        return boost::optional<Molecule const&>(*iter->second);
//    }

//    return boost::optional<Molecule const&>();
//}


void Core::start_level()
{
    std::cout << __FUNCTION__ << std::endl;

    set_new_game_state(Game_state::Running);

    reset_level();

    set_simulation_state(true);
}


void Core::set_new_game_state(Core::Game_state state)
{
    _previous_game_state = _game_state;
    _game_state = state;

    Q_EMIT game_state_changed();
}


Core::Game_state Core::get_game_state() const
{
    return _game_state;
}


Core::Game_state Core::get_previous_game_state() const
{
    return _previous_game_state;
}


const Sensor_data &Core::get_sensor_data() const
{
    return _sensor_data;
}


bool Core::is_not_persistent(const Level_element *e)
{
    return !e->is_persistent();
}


bool Core::is_not_persistent_boost_shared_ptr(const boost::shared_ptr<Level_element> &e)
{
    return is_not_persistent(e.get());
}


void Core::delete_non_persistent_objects()
{
    _level_data._barriers.erase(std::remove_if(_level_data._barriers.begin(), _level_data._barriers.end(), Core::is_not_persistent), _level_data._barriers.end());

    _level_data._brownian_elements.erase(std::remove_if(_level_data._brownian_elements.begin(), _level_data._brownian_elements.end(), is_not_persistent),
                                         _level_data._brownian_elements.end());

    _level_data._portals.erase(std::remove_if(_level_data._portals.begin(), _level_data._portals.end(), is_not_persistent),
                               _level_data._portals.end());

    _level_data._molecule_releasers.erase(std::remove_if(_level_data._molecule_releasers.begin(), _level_data._molecule_releasers.end(), is_not_persistent),
                                          _level_data._molecule_releasers.end());

    _level_data._level_elements.erase(std::remove_if(_level_data._level_elements.begin(), _level_data._level_elements.end(), is_not_persistent_boost_shared_ptr),
                                      _level_data._level_elements.end());

    assert(_level_data._level_elements.size() == _level_data._barriers.size() +
           _level_data._brownian_elements.size() +
           _level_data._portals.size() +
           _level_data._molecule_releasers.size());

    _level_data._particle_system_elements.clear();
}


void Core::clear()
{
    _level_data._molecules.clear();
    _level_data._barriers.clear();
    _level_data._brownian_elements.clear();
    _level_data._portals.clear();
    _level_data._level_elements.clear();
    _level_data._game_field_borders.clear();
    _level_data._molecule_releasers.clear();
    _level_data._particle_system_elements.clear();
    _level_data._external_forces.clear();
    _molecule_external_forces.clear();
    _molecule_id_to_molecule_map.clear();
    //        _molecule_hash.clear();
}


void Core::reset_level()
{
    delete_non_persistent_objects();

    _level_data._molecules.clear();

    _molecule_external_forces.clear();

    //        _molecule_hash.clear();
    _molecule_id_to_molecule_map.clear();

    _sensor_data.clear();
    _sensor_data.set_game_field_volume(_level_data._game_field_height * _level_data._game_field_width);

    _level_data.reset_level_elements();

    _current_time = 0.0f;
    _last_animation_time = 0.0f;
    _last_sensor_check = 0.0f;

    change_level_state(Main_game_screen::Level_state::Running);
}


void Core::save_simulation_settings()
{
    std::string file_name = Data_config::get_instance()->get_absolute_filename("simulation_settings.data", false);

    try
    {
        _parameters.save(file_name);
    }
    catch (std::exception const& e)
    {
        std::cout << "Couldn't save sim settings: " << e.what() << std::endl;
    }
}


void Core::load_simulation_settings()
{
    try
    {
        std::string file_name = Data_config::get_instance()->get_absolute_filename("simulation_settings.data");
        _parameters.load(file_name);
        update_variables();
    }
    catch (std::runtime_error const& e)
    {
        std::cout << "Couldn't load simulation settings file: simulation_settings.data, " << e.what() << std::endl;

        load_default_simulation_settings();
    }
}

void Core::load_default_simulation_settings()
{
    try
    {
        std::string file_name = Data_config::get_instance()->get_absolute_filename("default_simulation_settings.data");
        _parameters.load(file_name);
        update_variables();
    }
    catch (std::runtime_error const& e)
    {
        std::cout << "Couldn't load simulation settings file: default_simulation_settings.data, " << e.what() << std::endl;
    }
}

void Core::save_level(const std::string &file_name) const
{
    //        std::ofstream out_file(file_name.c_str(), std::ios_base::binary);
    std::ofstream out_file(file_name.c_str(), std::fstream::binary | std::fstream::out);
    //        boost::archive::text_oarchive oa(out_file);
    boost::archive::xml_oarchive oa(out_file);

    //        std::cout << "Parameter_list::save: " << out_file << std::endl;

    //        oa << BOOST_SERIALIZATION_NVP(_level_data);

    //        oa << BOOST_SERIALIZATION_NVP(*this);
//    oa << boost::serialization::make_nvp("Core", *this);

    oa << BOOST_SERIALIZATION_NVP(_level_data);

    out_file.close();
}


void Core::save_progress()
{
    std::string file_name = Data_config::get_instance()->get_absolute_filename("progress.data", false);
    std::ofstream out_file(file_name);
    boost::archive::xml_oarchive oa(out_file);

    oa << BOOST_SERIALIZATION_NVP(_progress);

    out_file.close();
}

void Core::load_progress()
{
    std::string file_name = Data_config::get_instance()->get_absolute_filename("progress.data", false);
    std::ifstream in_file(file_name);

    if (in_file)
    {
        boost::archive::xml_iarchive ia(in_file);

        try
        {
            ia >> BOOST_SERIALIZATION_NVP(_progress);
        }
        catch (std::exception e)
        {
            std::cout << "Failed to load progress file, progress reset: " << e.what() << std::endl;
            _progress = Progress();
        }

        in_file.close();
    }
    else
    {
        std::cout << "No progress file found" << std::endl;
    }
}

void Core::toggle_simulation()
{
    if (!_parameters["Toggle simulation"]->get_value<bool>())
    {
        _physics_timer.stop();
    }
    else
    {
        _physics_timer.start();
        _physics_elapsed_time = std::chrono::steady_clock::now();
    }
}

void Core::set_simulation_state(const bool s)
{
    _parameters["Toggle simulation"]->set_value(s);
}

bool Core::get_simulation_state() const
{
    return _parameters["Toggle simulation"]->get_value<bool>();
}

void Core::update_physics_timestep()
{
    _physics_timer.setInterval(_parameters["physics_timestep_ms"]->get_value<int>());
}


void Core::update_variables()
{
    QString const level_string = QString::fromStdString(_parameters["levels"]->get_value<std::string>());
    _level_names = level_string.split(",", QString::SkipEmptyParts);

    _do_constrain_forces = _parameters["do_constrain_forces"]->get_value<bool>();
    _max_force = _parameters["max_force"]->get_value<float>();
    _mass_factor = _parameters["Mass Factor"]->get_value<float>();
    _max_force_distance = _parameters["max_force_distance"]->get_value<float>();

    _atomic_forces = std::vector< std::unique_ptr<Atomic_force> >(Parameter_registry<Atomic_force>::get_unique_ptr_classes_from_multi_select_instance(_parameters.get_child("Atomic Force Type")));
}


void Core::update_parameters()
{
    _parameters["levels"]->set_value_no_update(_level_names.join(",").toStdString());

    _parameters["do_constrain_forces"]->set_value_no_update(_do_constrain_forces);
    _parameters["max_force"]->set_value_no_update(_max_force);
    _parameters["Mass Factor"]->set_value_no_update(_mass_factor);
    _parameters["max_force_distance"]->set_value_no_update(_max_force_distance);

    for (std::unique_ptr<Atomic_force> const& f : _atomic_forces)
    {
        Parameter_list * child = _parameters.get_child("Atomic Force Type")->get_child(f->get_instance_name());
        child->get_parameter("multi_enable")->set_value(true);

        f->update_parameters(*child);
    }
}

void Core::quit()
{
//    save_simulation_settings();
}

std::string const& Core::get_current_level_name() const
{
    return _current_level_name;
}

const QStringList &Core::get_level_names() const
{
    return _level_names;
}

void Core::update_physics()
{
    float const time_debug = false;

    std::chrono::steady_clock::time_point timer_start = std::chrono::steady_clock::now();

    if (time_debug)
    {
        timer_start = std::chrono::steady_clock::now();
    }

    // FIXME: currently constant update time step, not regarding at all the actually elapsed time
    // some updates are really far away from the set time step, not sure why
    update(_parameters["physics_timestep_ms"]->get_value<int>() / 1000.0f * _parameters["physics_speed"]->get_value<float>());

    if (time_debug)
    {
        std::chrono::steady_clock::time_point const timer_end = std::chrono::steady_clock::now();

        int const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << __FUNCTION__ << " time elapsed: " << elapsed_milliseconds << std::endl;
        }
    }
}


//const std::vector<Molecule_releaser *> &Core::get_molecule_releasers() const
//{
//    return _level_data._molecule_releasers;
//}


//const std::vector<Portal *> &Core::get_portals() const
//{
//    return _level_data._portals;
//}


//const std::vector<Barrier *> &Core::get_barriers() const
//{
//    return _level_data._barriers;
//}


const Molecule_external_force &Core::get_user_force() const
{
    return _user_force;
}


Molecule_external_force &Core::get_user_force()
{
    return _user_force;
}


float Core::get_current_time() const
{
    return _current_time;
}

void Core::gl_init(QGLContext * context)
{
    _gpu_force = std::unique_ptr<GPU_force>(new GPU_force(context, _level_data._temperature_grid.get_width()));
}


//void Core::add_external_force(const std::string &name, const External_force &force)
//{
//    _external_forces[name] = force;
//}


void Core::add_molecule_external_force(const Molecule_external_force &force)
{
    _molecule_external_forces.push_back(force);
}


//const std::vector<Force_indicator> &Core::get_force_indicators() const
//{
//    return _indicators;
//}


void Core::load_level_defaults()
{
//    _level_data = Level_data();
    clear();
    _level_data.load_defaults();

    Main_options_window::get_instance()->add_parameter_list("Level Data", _level_data._parameters);

    change_level_state(Main_game_screen::Level_state::Running);
}

void Core::load_level(std::string const& file_name)
{
    std::cout << __FUNCTION__ << " " << file_name << std::endl;

    clear();
    set_simulation_state(false);

    std::ifstream in_file(file_name.c_str(), std::fstream::binary | std::fstream::in);
    boost::archive::xml_iarchive ia(in_file);

    try
    {
        ia >> BOOST_SERIALIZATION_NVP(_level_data);
        std::cout << __FUNCTION__ << " A " << _level_data._parameters["gravity"]->get_value<float>() << std::endl;
    }
    catch (boost::archive::archive_exception & e)
    {
        std::cout << "Boost Archive Exception. Failed to load level file: " << file_name << ", reason: " << e.what() << std::endl;
        load_level_defaults();
    }
    catch (std::exception & e)
    {
        std::cout << "Failed to load level file: " << file_name << ", level reset: " << e.what() << std::endl;
        load_level_defaults();
    }
    catch (...)
    {
        load_level_defaults();
        QMessageBox::warning(nullptr, "Error", QString("Error reading the specified level file ") + QString::fromStdString(file_name) + "\nLoading defaults.");
    }

    in_file.close();

//    update_parameters();
    _level_data.update_parameters();

    Main_options_window::get_instance()->add_parameter_list("Level Data", _level_data._parameters);


    assert(_level_data.validate_elements());

    reset_level();

    std::cout << __FUNCTION__ << " B " << _level_data._parameters["gravity"]->get_value<float>() << std::endl;

    _current_level_name = file_name;

    change_level_state(Main_game_screen::Level_state::Running);
}

void Core::change_level_state(const Main_game_screen::Level_state new_level_state)
{
    Q_EMIT level_changed(new_level_state);
}

void Core::load_next_level()
{
    std::cout << __FUNCTION__ << " next level: " << get_progress().last_level << std::endl;

    if (_level_names.size() <= get_progress().last_level)
    {
        std::cout << "No more levels." << std::endl;
        return;
    }

    std::string const filename = Data_config::get_instance()->get_absolute_filename("levels/" + _level_names[get_progress().last_level] + ".data");

    load_level(filename);
}

void Core::init_game()
{
    QString const level_string = QString::fromStdString(_parameters["levels"]->get_value<std::string>());
    _level_names = level_string.split(",", QString::SkipEmptyParts);

    load_progress();
}
