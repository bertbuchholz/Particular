#include "Core.h"

#include <QFrame>
#include <QVBoxLayout>
#include <Q_parameter_bridge.h>

#include "Molecule_releaser.h"

#include "fp_exception_glibc_extension.h"
#include <fenv.h>


Core::Core() :
    _game_state(Game_state::Unstarted),
    _previous_game_state(Game_state::Unstarted),
    _molecule_id_counter(0),
    _current_time(0.0f),
    _last_sensor_check(0.0f)
  //        _molecule_hash(Molecule_atom_hash(100, 4.0f))
{
//    Eigen::Vector2f grid_start(-10.0f, -10.0f);
//    Eigen::Vector2f grid_end  ( 10.0f,  10.0f);

//    float resolution = 0.5f;

//    for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
//    {
//        for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
//        {
//            _indicators.push_back(Force_indicator(Eigen::Vector3f(x, y, 0.0f)));
//        }
//    }

    std::function<void(void)> update = std::bind(&Core::parameter_changed, this);

    _parameters.add_parameter(new Parameter("mass_factor", 1.0f, 0.01f, 10.0f, update));
    _parameters.add_parameter(new Parameter("do_constrain_forces", true, update));
    _parameters.add_parameter(new Parameter("max_force", 10.0f, 0.1f, 500.0f, update));
    _parameters.add_parameter(new Parameter("max_force_distance", 10.0f, 1.0f, 1000.0f, update));
    _parameters.add_parameter(new Parameter("gravity", 1.0f, 0.0f, 100.0f, update));

    Parameter_registry<Atomic_force>::create_multi_select_instance(&_parameters, "Atomic Force Type", update);
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


Eigen::Vector3f Core::apply_forces_using_ann_tree(Atom const& receiver_atom) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    std::vector<Atom const*> const closest_atoms = _ann_wrapper.find_closest_atoms(receiver_atom);

    for (Atom const* sender_atom : closest_atoms)
    {
        if (receiver_atom._parent_id == sender_atom->_parent_id) continue;

        force_i += calc_forces_between_atoms(receiver_atom, *sender_atom);
    }

    return force_i;
}



Eigen::Vector3f Core::apply_forces_using_tree(const Atom &receiver_atom) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    std::queue<My_tree const*> queue;
    queue.push(&_tree);

    while (!queue.empty())
    {
        My_tree const* node = queue.front();
        queue.pop();

        if (node->get_is_leaf())
        {
            for (Atom const* sender_atom : node->get_data())
            {
                if (!(sender_atom->_parent_id >= 0))
                {
                    std::cout << sender_atom << " " << sender_atom->_parent_id << std::endl;
                }

                assert(sender_atom->_parent_id >= 0);

                if (sender_atom->_parent_id == receiver_atom._parent_id) continue;

                ++_debug_leaf_usage_count;
                force_i += calc_forces_between_atoms(receiver_atom, *sender_atom);
            }
        }
        else
        {
            float const dist = (receiver_atom.get_position() - node->get_averaged_data().get_position()).norm();
            float const dist_to_cell_center = (receiver_atom.get_position() - node->get_center()).norm();

            bool const too_close_to_cell = dist_to_cell_center < 1.5f * node->get_radius();

            if (too_close_to_cell ||
                    node->is_point_in(receiver_atom.get_position()) ||
                    dist < _max_force_distance ||
                    dist < node->get_averaged_data()._radius * 2.0f) // too large, push children
            {
                for (My_tree const& child : node->get_children())
                {
                    if (child.has_averaged_data())
                    {
                        queue.push(&child);
                    }
                }
            }
            else
            {
                ++_debug_inner_node_usage_count;
                force_i += calc_forces_between_atoms(receiver_atom, node->get_averaged_data());
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


std::vector<const Atom *> Core::get_atoms_from_tree(const Atom &receiver_atom) const
{
    std::vector<Atom const*> result;

    std::queue<My_tree const*> queue;
    queue.push(&_tree);

    while (!queue.empty())
    {
        My_tree const* node = queue.front();
        queue.pop();

        if (node->get_is_leaf())
        {
            for (Atom const* sender_atom : node->get_data())
            {
                if (!(sender_atom->_parent_id >= 0))
                {
                    std::cout << sender_atom << " " << sender_atom->_parent_id << std::endl;
                }

                assert(sender_atom->_parent_id >= 0);

                if (sender_atom->_parent_id == receiver_atom._parent_id) continue;

                result.push_back(sender_atom);
            }
        }
        else
        {
            float const dist = (receiver_atom.get_position() - node->get_averaged_data().get_position()).norm();
            float const dist_to_cell_center = (receiver_atom.get_position() - node->get_center()).norm();

            bool const too_close_to_cell = dist_to_cell_center < 1.5f * node->get_radius();

            if (too_close_to_cell ||
                    node->is_point_in(receiver_atom.get_position()) ||
                    dist < _max_force_distance ||
                    dist < node->get_averaged_data()._radius * 2.0f) // too large, push children
            {
                for (My_tree const& child : node->get_children())
                {
                    if (child.has_averaged_data())
                    {
                        queue.push(&child);
                    }
                }
            }
            else
            {
                result.push_back(&node->get_averaged_data());
            }
        }
    }

    return result;
}


Eigen::Vector3f Core::force_on_atom(const Atom &receiver_atom) const
{
    Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

    //        force_i = apply_forces_brute_force(receiver_atom, parent_molecule_id);
//    force_i = apply_forces_using_tree(receiver_atom);
    //        force_i = apply_forces_from_vector(receiver_atom, get_atoms_from_tree(receiver_atom));
    force_i = apply_forces_using_ann_tree(receiver_atom);

    // per atom force application, replaced by per molecule application
    //        for (Barrier const* b : _level_data._barriers)
    //        {
    //            force_i += b->calc_force(receiver_atom);
    //        }

    if (std::isnan(force_i[0]))
    {
        std::cout << __PRETTY_FUNCTION__ << " isnan, atom: " << &receiver_atom << std::endl;
        force_i = Eigen::Vector3f::Zero();
    }

    //        assert(!std::isnan(force_i[0]));

    return force_i;
}


void Core::compute_force_and_torque(Molecule &receiver)
{
    float const translation_to_rotation_ratio = 0.05f;

    receiver._force.setZero();
    receiver._torque.setZero();

    for (Atom const& receiver_atom : receiver._atoms)
    {
        Eigen::Vector3f force_i = force_on_atom(receiver_atom);

        receiver._force += force_i;
        receiver._torque += (receiver_atom.get_position() - receiver._x).cross(force_i);
    }

    for (Barrier const* b : _level_data._barriers)
    {
        receiver._force += b->calc_force_on_molecule(receiver);
    }

    float brownian_translation_factor = _level_data._translation_fluctuation;
    float brownian_rotation_factor = _level_data._rotation_fluctuation * translation_to_rotation_ratio;

    for (Brownian_element const* element : _level_data._brownian_elements)
    {
        float const factor = element->get_brownian_motion_factor(receiver._x);

        brownian_translation_factor += factor;
        brownian_rotation_factor += translation_to_rotation_ratio * factor;
    }

    Eigen::Vector3f random_dir = Eigen::Vector3f::Random().normalized();

    receiver._force  += std::max(0.0f, brownian_translation_factor) * random_dir;
    receiver._torque += std::max(0.0f, brownian_rotation_factor)    * Eigen::Vector3f::Random().normalized();

    for (auto const& f : _external_forces)
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

        if (force_size > 10.0f)
        {
            receiver._force  = (_max_force / force_size)  * receiver._force;
        }
    }

    receiver._force += -_level_data._translation_damping * receiver._v;
    receiver._torque += -_level_data._rotation_damping * receiver._omega;
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

//                std::cout << __PRETTY_FUNCTION__ << " molecule in portal" << std::endl;
            }
        }

        if (!has_been_removed)
        {
            ++molecule_iter;
        }
    }
}


void Core::update(const float time_step)
{
    _current_time += time_step;

    bool const time_debug = true;

    int elapsed_milliseconds;
    std::chrono::time_point<std::chrono::system_clock> timer_start, timer_end;


    for (Molecule_releaser * m : _level_data._molecule_releasers)
    {
        if (m->check_do_release(_current_time))
        {
            add_molecule(m->release(_current_time));
        }
    }

    if (time_debug)
    {
        timer_start = std::chrono::system_clock::now();
    }

    //        update_spatial_hash();

    if (time_debug)
    {
        timer_end = std::chrono::system_clock::now();

        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "update_spatial_hash(): " << elapsed_milliseconds << std::endl;
        }
    }

    if (time_debug)
    {
        timer_start = std::chrono::system_clock::now();
    }

    update_tree();

    _ann_wrapper = ANN_wrapper();
    _ann_wrapper.generate_tree_from_molecules(_level_data._molecules);

    if (time_debug)
    {
        timer_end = std::chrono::system_clock::now();

        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "update_tree(): " << elapsed_milliseconds << std::endl;
        }
    }

    _level_data._particle_system_elements.erase(std::remove_if(_level_data._particle_system_elements.begin(), _level_data._particle_system_elements.end(), Particle_system_element::check_if_dead()),
                                                _level_data._particle_system_elements.end());

    for (Level_element * e : _level_data._particle_system_elements)
    {
        e->animate(time_step);
    }

    for (boost::shared_ptr<Level_element> const& e : _level_data._level_elements)
    {
        e->animate(time_step);
    }

    if (time_debug)
    {
        timer_start = std::chrono::system_clock::now();
    }

    _debug_leaf_usage_count = 0;
    _debug_inner_node_usage_count = 0;

    for (Molecule & m : _level_data._molecules)
    {
        compute_force_and_torque(m);
    }

    if (time_debug)
    {
        timer_end = std::chrono::system_clock::now();

        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "compute_force_and_torque(): " << elapsed_milliseconds << std::endl;
//            std::cout << "leaf usage: " << _debug_leaf_usage_count << " ";
//            std::cout << "inner node usage: " << _debug_inner_node_usage_count << std::endl;
        }
    }

    _molecule_external_forces.erase(std::remove_if(_molecule_external_forces.begin(), _molecule_external_forces.end(), Check_duration(_current_time)),
                                    _molecule_external_forces.end());

//    if (_use_indicators)
//    {
//        for (Force_indicator & f : _indicators)
//        {
//            f._force = force_on_atom(f._atom);
//        }
//    }

    if (time_debug)
    {
        timer_start = std::chrono::system_clock::now();
    }

    for (Molecule & molecule : _level_data._molecules)
    {
        molecule._x += molecule._v * time_step;

//        feenableexcept(FE_INVALID | FE_OVERFLOW);

        Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
        Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
        molecule._q = add(molecule._q, scale(q_dot, time_step));

        assert(!std::isnan(molecule._q.w()) && !std::isinf(molecule._q.w()));

        molecule._P += molecule._force * time_step;

        molecule._L += molecule._torque * time_step;

        molecule.from_state(Body_state(), _mass_factor);

//        fedisableexcept(FE_INVALID | FE_OVERFLOW);
    }

    if (time_debug)
    {
        timer_end = std::chrono::system_clock::now();

        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                (timer_end-timer_start).count();

        if (elapsed_milliseconds > 1)
        {
            std::cout << "update molecules: " << elapsed_milliseconds << std::endl;
        }
    }

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

    for (Brownian_element const* p : _level_data._brownian_elements)
    {
        energy_consumption += std::abs(p->get_strength()) * p->get_radius();
        average_temperature += p->get_strength() * p->get_radius();
    }

    _sensor_data.add_value(Sensor_data::Type::AvgTemp, average_temperature);
    _sensor_data.add_value(Sensor_data::Type::ColMol, num_collected_molecules);
    _sensor_data.add_value(Sensor_data::Type::RelMol, num_released_molecules);
    _sensor_data.add_value(Sensor_data::Type::EnergyCon, energy_consumption);
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


void Core::update_tree()
{
    Eigen::Vector3f min(1e10f, 1e10f, 1e10f), max(-1e10f, -1e10f, -1e10f);

    for (Molecule const& m : _level_data._molecules)
    {
        for (Atom const& a : m._atoms)
        {
            for (int i = 0; i < 3; ++i)
            {
                min[i] = std::min(a.get_position()[i], min[i]);
                max[i] = std::max(a.get_position()[i], max[i]);
            }
        }
    }

    min -= Eigen::Vector3f(0.1f, 0.1f, 0.1f);
    max += Eigen::Vector3f(0.1f, 0.1f, 0.1f);

    _tree = My_tree(min, max, 10, 10);

    for (Molecule const& m : _level_data._molecules)
    {
        for (Atom const& a : m._atoms)
        {
            _tree.add_point(a.get_position(), &a);
        }
    }

    My_tree::average_data(&_tree, Atom_averager());
}


void Core::set_game_field_borders(const Eigen::Vector3f &min, const Eigen::Vector3f &max)
{
    for (auto const& e : _level_data._game_field_borders)
    {
        Level_element * b = e.second;
        delete_level_element(b);
    }

    _level_data._game_field_borders.clear();

    Eigen::AlignedBox<float, 3> play_box(min, max);
    Eigen::Vector3f play_box_center(play_box.center());
    Eigen::Vector3f play_box_extent((play_box.max() - play_box.min()));

    float const strength = 10000.0f;
    float const radius   = 2.0f;

    for (int axis = 0; axis < 3; ++axis)
    {
        for (int sign = -1; sign <= 2; sign += 2)
        {
            Eigen::Vector3f normal = Eigen::Vector3f::Zero();
            normal[axis] = -1.0f * sign;
            int const first_axis = (axis + 1) % 3;
            int const second_axis = (axis + 2) % 3;
            Eigen::Vector2f extent(play_box_extent[first_axis > second_axis ? second_axis : first_axis], play_box_extent[first_axis < second_axis ? second_axis : first_axis]);
            Plane_barrier * b = new Plane_barrier(play_box_center - normal * play_box_extent[axis] * 0.5f, normal, strength, radius, extent);

            int const sign_axis_to_enum = axis + ((sign < 0) ? 0 : 3);
            Level_data::Plane const plane = Level_data::Plane(sign_axis_to_enum);
            _level_data._game_field_borders[plane] = b;
            add_barrier(b);
        }
    }
}


const std::vector<Brownian_element *> &Core::get_brownian_elements() const
{
    return _level_data._brownian_elements;
}


Eigen::Vector3f Core::calc_forces_between_atoms(const Atom &a_0, const Atom &a_1) const
{
    Eigen::Vector3f resulting_force(0.0f, 0.0f, 0.0f);

    for (Atomic_force const* force : _atomic_forces)
    {
        resulting_force += force->calc_force_between_atoms(a_0, a_1);
    }

    return resulting_force;
}


boost::optional<const Molecule &> Core::get_molecule(const int id) const
{
    auto iter = _molecule_id_to_molecule_map.find(id);

    if (iter != _molecule_id_to_molecule_map.end())
    {
        return boost::optional<Molecule const&>(*iter->second);
    }

    return boost::optional<Molecule const&>();
}


void Core::delete_level_element(Level_element *level_element)
{
    _level_data._molecule_releasers.erase(std::remove(_level_data._molecule_releasers.begin(), _level_data._molecule_releasers.end(), level_element), _level_data._molecule_releasers.end());
    _level_data._barriers.erase(std::remove(_level_data._barriers.begin(), _level_data._barriers.end(), level_element), _level_data._barriers.end());
    _level_data._portals.erase(std::remove(_level_data._portals.begin(), _level_data._portals.end(), level_element), _level_data._portals.end());
    _level_data._brownian_elements.erase(std::remove(_level_data._brownian_elements.begin(), _level_data._brownian_elements.end(), level_element), _level_data._brownian_elements.end());

    _level_data._level_elements.erase(std::remove_if(_level_data._level_elements.begin(), _level_data._level_elements.end(), Ptr_contains_predicate<Level_element>(level_element)), _level_data._level_elements.end());

    assert(_level_data._level_elements.size() == _level_data._barriers.size() +
           _level_data._brownian_elements.size() +
           _level_data._portals.size() +
           _level_data._molecule_releasers.size());
}


void Core::reset_level_elements()
{
    for (boost::shared_ptr<Level_element> const& e : _level_data._level_elements)
    {
        e->reset();
    }
}


void Core::start_level()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    reset_level();

    set_new_game_state(Game_state::Running);
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
    _molecule_external_forces.clear();
    _molecule_id_to_molecule_map.clear();
    _external_forces.clear();
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

    reset_level_elements();

    _current_time = 0.0f;
    _last_sensor_check = 0.0f;
}


void Core::save_state(const std::string &file_name) const
{
    std::ofstream out_file(file_name.c_str(), std::ios_base::binary);
    boost::archive::text_oarchive oa(out_file);

    //        std::cout << "Parameter_list::save: " << out_file << std::endl;

    oa << *this;

    out_file.close();
}


void Core::load_state(const std::string &file_name)
{
    std::cout << __PRETTY_FUNCTION__ << " file: " << file_name << std::endl;

    std::ifstream in_file(file_name.c_str(), std::ios_base::binary);
    boost::archive::text_iarchive ia(in_file);

    ia >> *this;

    in_file.close();
}


void Core::save_level(const std::string &file_name) const
{
    //        std::ofstream out_file(file_name.c_str(), std::ios_base::binary);
    std::ofstream out_file(file_name.c_str());
    //        boost::archive::text_oarchive oa(out_file);
    boost::archive::xml_oarchive oa(out_file);

    //        std::cout << "Parameter_list::save: " << out_file << std::endl;

    //        oa << BOOST_SERIALIZATION_NVP(_level_data);

    //        oa << BOOST_SERIALIZATION_NVP(*this);
    oa << boost::serialization::make_nvp("Core", *this);

    out_file.close();
}


void Core::load_level(const std::string &file_name)
{
    std::cout << __PRETTY_FUNCTION__ << " file: " << file_name << std::endl;

    //        std::ifstream in_file(file_name.c_str(), std::ios_base::binary);
    //        boost::archive::text_iarchive ia(in_file);
    std::ifstream in_file(file_name.c_str());
    boost::archive::xml_iarchive ia(in_file);

    try
    {
        //            ia >> BOOST_SERIALIZATION_NVP(_level_data);
        //            ia >> BOOST_SERIALIZATION_NVP(*this);
        ia >> boost::serialization::make_nvp("Core", *this);
    }
    catch (boost::archive::archive_exception & e)
    {
        std::cout << "Boost Archive Exception. Failed to load level file: " << file_name << ", reason: " << e.what() << std::endl;
        _level_data = Level_data();
        throw;
    }
    catch (std::exception & e)
    {
        std::cout << "Failed to load level file: " << file_name << ", level reset: " << e.what() << std::endl;
        _level_data = Level_data();
        throw;
    }

    in_file.close();

    assert(_level_data.validate_elements());
}


void Core::parameter_changed()
{
    _do_constrain_forces = _parameters["do_constrain_forces"]->get_value<bool>();
    _max_force = _parameters["max_force"]->get_value<float>();

    _mass_factor = _parameters["mass_factor"]->get_value<float>();

    _max_force_distance = _parameters["max_force_distance"]->get_value<float>();

    for (auto * f : _atomic_forces)
    {
        delete f;
    }

    _atomic_forces = std::vector<Atomic_force*>(Parameter_registry<Atomic_force>::get_classes_from_multi_select_instance(_parameters.get_child("Atomic Force Type")));

    _external_forces["gravity"]._force[2] = -_parameters["gravity"]->get_value<float>();
}


void Core::update_parameter_list(Parameter_list &parameters) const
{
    auto iter = _external_forces.find("gravity");

    if (iter != _external_forces.end())
    {
        parameters["gravity"]->set_value_no_update(-iter->second._force[2]);
    }
}

QWidget * Core::get_parameter_widget() const
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    QFrame * frame = new QFrame;

    QFont f = frame->font();
    f.setPointSize(f.pointSize() * 0.9f);
    frame->setFont(f);

    QLayout * menu_layout = new QVBoxLayout;

    draw_instance_list(_parameters, menu_layout);

    menu_layout->setSpacing(0);
    menu_layout->setMargin(0);

//    frame->setWindowTitle(windowTitle() + " Options");
    frame->setLayout(menu_layout);

    return frame;
}


//Parameter_list Core::get_parameters()
//{
//    Parameter_list parameters;
//    parameters.add_parameter(new Parameter("mass_factor", 1.0f, 0.01f, 10.0f));
//    parameters.add_parameter(new Parameter("do_constrain_forces", true));
//    parameters.add_parameter(new Parameter("max_force", 10.0f, 0.1f, 500.0f));
//    parameters.add_parameter(new Parameter("max_force_distance", 10.0f, 1.0f, 1000.0f));
//    parameters.add_parameter(new Parameter("gravity", 1.0f, 0.0f, 100.0f));

//    Parameter_registry<Atomic_force>::create_multi_select_instance(&parameters, "Atomic Force Type");

//    return parameters;
//}


const std::vector<Molecule_releaser *> &Core::get_molecule_releasers() const
{
    return _level_data._molecule_releasers;
}


const std::vector<Portal *> &Core::get_portals() const
{
    return _level_data._portals;
}


const std::vector<Barrier *> &Core::get_barriers() const
{
    return _level_data._barriers;
}


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


void Core::add_external_force(const std::string &name, const External_force &force)
{
    _external_forces[name] = force;
}


void Core::add_molecule_external_force(const Molecule_external_force &force)
{
    _molecule_external_forces.push_back(force);
}


const std::vector<Force_indicator> &Core::get_force_indicators() const
{
    return _indicators;
}


void Core::add_molecule_releaser(Molecule_releaser *molecule_releaser)
{
    _level_data._molecule_releasers.push_back(molecule_releaser);
    _level_data._level_elements.push_back(boost::shared_ptr<Level_element>(molecule_releaser));
}


void Core::add_portal(Portal *portal)
{
    _level_data._portals.push_back(portal);
    _level_data._level_elements.push_back(boost::shared_ptr<Level_element>(portal));
}


void Core::add_brownian_element(Brownian_element *element)
{
    _level_data._brownian_elements.push_back(element);
    _level_data._level_elements.push_back(boost::shared_ptr<Level_element>(element));
}


void Core::add_barrier(Barrier *barrier)
{
    _level_data._barriers.push_back(barrier);
    _level_data._level_elements.push_back(boost::shared_ptr<Level_element>(barrier));
}
