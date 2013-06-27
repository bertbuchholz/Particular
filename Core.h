#ifndef CORE_H
#define CORE_H

#include <QObject>

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/optional.hpp>

#include <Registry_parameters.h>

#include "GPU_force.h"
#include "Atom.h"
#include "Atomic_force.h"
#include "Spatial_hash.h"
#include "RegularBspTree.h"
#include "Level_elements.h"
#include "Level_data.h"
#include "End_condition.h"

class Core : public QObject
{
    Q_OBJECT
public:

    struct Molecule_atom_id
    {
        Molecule_atom_id(int const m, int const a) : m_id(m), a_id(a)
        { }

        int m_id;
        int a_id;
    };

    typedef Spatial_hash<Eigen::Vector3f, Molecule_atom_id> Molecule_atom_hash;
    typedef Regular_bsp_tree<Eigen::Vector3f, 3, Atom> My_tree;

    Core() :
        _current_time(0.0f),
        _molecule_hash(Molecule_atom_hash(100, 4.0f))
    {
        Eigen::Vector2f grid_start(-10.0f, -10.0f);
        Eigen::Vector2f grid_end  ( 10.0f,  10.0f);

        float resolution = 0.5f;

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
            {
                _indicators.push_back(Force_indicator(Eigen::Vector3f(x, y, 0.0f)));
            }
        }
    }

    Eigen::Vector3f apply_forces_brute_force(Atom const& receiver_atom) const
    {
        Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

        for (Molecule const& sender : _level_data._molecules)
        {
//            if (parent_molecule_id == sender.get_id()) continue;

            for (Atom const& sender_atom : sender._atoms)
            {
                if (receiver_atom._parent_id == sender_atom._parent_id) continue;

                float const dist = (receiver_atom._r - sender_atom._r).norm();

                if (dist > 1e-4f && dist < _max_force_distance)
                {
                    force_i += calc_forces_between_atoms(receiver_atom, sender_atom);
                }
            }
        }

        return force_i;
    }


    Eigen::Vector3f apply_forces_using_tree(Atom const& receiver_atom) const
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
                    assert(sender_atom->_parent_id >= 0);

                    if (sender_atom->_parent_id == receiver_atom._parent_id) continue;

                    float const dist = (receiver_atom._r - sender_atom->_r).norm();

                    if (dist > 1e-4f)
                    {
                        force_i += calc_forces_between_atoms(receiver_atom, *sender_atom);
                    }
                }
            }
            else
            {
                float const dist = (receiver_atom._r - node->get_averaged_data()._r).norm();

                if (node->is_point_in(receiver_atom._r) ||
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
                    force_i += calc_forces_between_atoms(receiver_atom, node->get_averaged_data());
                }
            }
        }

        return force_i;
    }

    std::vector<Atom const*> get_atoms_from_tree(Atom const& receiver_atom) const
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
                    assert(sender_atom->_parent_id >= 0);

                    if (sender_atom->_parent_id == receiver_atom._parent_id) continue;

                    result.push_back(sender_atom);
                }
            }
            else
            {
                float const dist = (receiver_atom._r - node->get_averaged_data()._r).norm();
                float const dist_to_cell_center = (receiver_atom._r - node->get_center()).norm();

                bool too_close_to_cell = dist_to_cell_center < 1.5f * node->get_radius();

                if (too_close_to_cell ||
                        node->is_point_in(receiver_atom._r) ||
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

    Eigen::Vector3f apply_forces_from_vector(Atom const& receiver_atom, std::vector<Atom const*> const& atoms) const
    {
        Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

        for (Atom const* a : atoms)
        {
            force_i += calc_forces_between_atoms(receiver_atom, *a);
        }

        return force_i;
    }


    Eigen::Vector3f force_on_atom(Atom const& receiver_atom) const
    {        
        Eigen::Vector3f force_i = Eigen::Vector3f::Zero();

//        force_i = apply_forces_brute_force(receiver_atom, parent_molecule_id);
//        force_i = apply_forces_using_tree(receiver_atom);
        force_i = apply_forces_from_vector(receiver_atom, get_atoms_from_tree(receiver_atom));


        for (Barrier const* b : _level_data._barriers)
        {
            force_i += b->calc_force(receiver_atom);
        }

        assert(!std::isnan(force_i[0]));

        return force_i;
    }

    void compute_force_and_torque(Molecule & receiver)
    {
        float const translation_to_rotation_ratio = 0.05f;

        receiver._force.setZero();
        receiver._torque.setZero();

        for (Atom const& receiver_atom : receiver._atoms)
        {
            Eigen::Vector3f force_i = force_on_atom(receiver_atom);

            receiver._force += force_i;
            receiver._torque += (receiver_atom._r - receiver._x).cross(force_i);
        }

        receiver._force += -_translation_damping * receiver._v;
        receiver._torque += -_rotation_damping * receiver._omega;

        float brownian_translation_factor = _translation_fluctuation;
        float brownian_rotation_factor = _rotation_fluctuation;

        for (Brownian_element const* element : _level_data._brownian_elements)
        {
            float const factor = element->get_brownian_motion_factor(receiver._x);

            brownian_translation_factor += factor;
            brownian_rotation_factor += translation_to_rotation_ratio * factor;
        }

        receiver._force  += std::max(0.0f, brownian_translation_factor) * Eigen::Vector3f::Random().normalized();
        receiver._torque += std::max(0.0f, brownian_rotation_factor)    * Eigen::Vector3f::Random().normalized();

        for (auto f : _external_forces)
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
    }

    Eigen::Quaternion<float> scale(Eigen::Quaternion<float> const& quat, float const factor)
    {
        return Eigen::Quaternion<float>(quat.w() * factor, quat.x() * factor, quat.y() * factor, quat.z() * factor);
    }

    Eigen::Quaternion<float> add(Eigen::Quaternion<float> const& q_0, Eigen::Quaternion<float> const& q_1)
    {
        return Eigen::Quaternion<float>(q_0.w() + q_1.w(), q_0.x() + q_1.x(), q_0.y() + q_1.y(), q_0.z() + q_1.z());
    }

    struct Check_duration
    {
        Check_duration(float const end_time) : _end_time(end_time) {}

        bool operator() (Molecule_external_force const& f) const
        {
            return (f._end_time < _end_time);
        }

        float _end_time;
    };


    void update(float const time_step)
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

        update_spatial_hash();


        if (time_debug)
        {
            timer_start = std::chrono::system_clock::now();
        }

        update_spatial_hash();

        if (time_debug)
        {
            timer_end = std::chrono::system_clock::now();

            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timer_end-timer_start).count();
            std::cout << "update_spatial_hash(): " << elapsed_milliseconds << std::endl;
        }

        if (time_debug)
        {
            timer_start = std::chrono::system_clock::now();
        }

        update_tree();

        if (time_debug)
        {
            timer_end = std::chrono::system_clock::now();

            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timer_end-timer_start).count();
            std::cout << "update_tree(): " << elapsed_milliseconds << std::endl;
        }


        if (time_debug)
        {
            timer_start = std::chrono::system_clock::now();
        }

        for (Molecule & m : _level_data._molecules)
        {
            if (!m._active) continue;

            for (Portal * p : _level_data._portals)
            {
                if (p->contains(m._x))
                {
                    p->handle_molecule_entering();

                    m._active = false;
                }
            }
        }


        for (Barrier * b : _level_data._barriers)
        {
            b->animate(time_step);
        }


        for (Molecule & m : _level_data._molecules)
        {
            if (!m._active) continue;

            // tmp_states.push_back(m.to_state());
            compute_force_and_torque(m);
        }

        if (time_debug)
        {
            timer_end = std::chrono::system_clock::now();

            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timer_end-timer_start).count();

            std::cout << "compute_force_and_torque(): " << elapsed_milliseconds << std::endl;
        }


        _molecule_external_forces.erase(std::remove_if(_molecule_external_forces.begin(), _molecule_external_forces.end(), Check_duration(_current_time)),
                        _molecule_external_forces.end());

        if (_use_indicators)
        {
            for (Force_indicator & f : _indicators)
            {
                f._force = force_on_atom(f._atom);
            }
        }

        if (time_debug)
        {
            timer_start = std::chrono::system_clock::now();
        }

        for (size_t i = 0; i < _level_data._molecules.size(); ++i)
        {
//            Body_state & state = tmp_states[i];
            Molecule & molecule = _level_data._molecules[i];

            if (!molecule._active) continue;

//            state._x += molecule._v * time_step;

//            Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
//            Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
//            state._q = add(state._q, scale(q_dot, time_step));

//            state._P += molecule._force * time_step;

//            state._L += molecule._torque * time_step;

//            molecule.from_state(state);

            molecule._x += molecule._v * time_step;

            Eigen::Quaternion<float> omega_quaternion(0.0f, molecule._omega[0], molecule._omega[1], molecule._omega[2]);
            Eigen::Quaternion<float> q_dot = scale(omega_quaternion * molecule._q, 0.5f);
            molecule._q = add(molecule._q, scale(q_dot, time_step));

            assert(!std::isnan(molecule._q.w()));

            molecule._P += molecule._force * time_step;

            molecule._L += molecule._torque * time_step;

            molecule.from_state(Body_state(), _mass_factor);
        }

        if (time_debug)
        {
            timer_end = std::chrono::system_clock::now();

            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timer_end-timer_start).count();

            std::cout << "update molecules: " << elapsed_milliseconds << std::endl;
        }
    }

    bool check_is_finished()
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

    std::vector<Molecule> const& get_molecules() const
    {
        return _level_data._molecules;
    }

    Molecule_atom_hash const& get_molecule_hash() const
    {
        return _molecule_hash;
    }

    My_tree const& get_tree() const
    {
        return _tree;
    }

    void add_molecule(Molecule molecule)
    {
        molecule.set_id(_level_data._molecules.size());
        _level_data._molecules.push_back(molecule);
    }

    void update_spatial_hash()
    {
        _molecule_hash.clear();

        for (Molecule const& m : _level_data._molecules)
        {
            if (!m._active) continue;

            for (size_t i = 0; i < m._atoms.size(); ++i)
            {
                _molecule_hash.add_point(m._atoms[i]._r, Molecule_atom_id(m.get_id(), i));
            }
        }
    }

    struct Atom_averager
    {
        Atom operator() (std::vector<Atom const*> const& atoms) const
        {
            Atom result;

            result._type = Atom::Type::H;

            result._r.setZero();

            result._mass = 0.0f;
            result._charge = 0.0f;
            result._radius = 0.0f;

            float charge_abssum = 0.0f;

            for (Atom const* a : atoms)
            {
                float abs_charge = std::abs(a->_charge);
                abs_charge = abs_charge > 0.001f ? abs_charge : 1.0f;

                result._r += a->_r * abs_charge; // weigh the position by the charge
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

            result._r /= charge_abssum;

            result._radius = std::pow(result._radius, 0.3333f);

            // not averaging, accumulating!
//            result._mass /= atoms.size();
//            result._charge /= atoms.size();
//            result._radius /= atoms.size();

            return result;
        }
    };

    void update_tree()
    {
        Eigen::Vector3f min(1e10f, 1e10f, 1e10f), max(-1e10f, -1e10f, -1e10f);

        for (Molecule const& m : _level_data._molecules)
        {
            if (!m._active) continue;

            for (Atom const& a : m._atoms)
            {
                for (int i = 0; i < 3; ++i)
                {
                    min[i] = std::min(a._r[i], min[i]);
                    max[i] = std::max(a._r[i], max[i]);
                }
            }
        }

        min -= Eigen::Vector3f(0.1f, 0.1f, 0.1f);
        max += Eigen::Vector3f(0.1f, 0.1f, 0.1f);

        _tree = My_tree(min, max, 10, 10);

        for (Molecule const& m : _level_data._molecules)
        {
            if (!m._active) continue;

            for (Atom const& a : m._atoms)
            {
                _tree.add_point(a._r, &a); // FIXME: possibly bad, when the vector containing a gets moved, then &a changes
            }
        }

        My_tree::average_data(&_tree, Atom_averager());
    }

    void add_barrier(Barrier * barrier)
    {
        _level_data._barriers.push_back(barrier);
    }

    void add_brownian_element(Brownian_element * element)
    {
        _level_data._brownian_elements.push_back(element);
    }

    void add_portal(Portal * portal)
    {
        _level_data._portals.push_back(portal);
    }

    void add_molecule_releaser(Molecule_releaser * molecule_releaser)
    {
        _level_data._molecule_releasers.push_back(molecule_releaser);
    }

    std::vector<Force_indicator> const& get_force_indicators() const
    {
        return _indicators;
    }

    void add_molecule_external_force(Molecule_external_force const& force)
    {
        _molecule_external_forces.push_back(force);
    }

    void add_external_force(std::string const& name, External_force const& force)
    {
        _external_forces[name] = force;
    }

    float get_current_time() const
    {
        return _current_time;
    }

    Molecule_external_force & get_user_force()
    {
        return _user_force;
    }

    Molecule_external_force const& get_user_force() const
    {
        return _user_force;
    }

    std::vector<Barrier*> const& get_barriers() const
    {
        return _level_data._barriers;
    }

    std::vector<Portal*> const& get_portals() const
    {
        return _level_data._portals;
    }

    std::vector<Molecule_releaser*> const& get_molecule_releasers() const
    {
        return _level_data._molecule_releasers;
    }

    void set_game_field_borders(Eigen::Vector3f const& min, Eigen::Vector3f const& max)
    {
        for (Barrier * b : _level_data._game_field_borders)
        {
            _level_data._barriers.erase(std::remove(_level_data._barriers.begin(), _level_data._barriers.end(), b), _level_data._barriers.end());
            delete b;
        }

        _level_data._game_field_borders.clear();

        Eigen::AlignedBox<float, 3> play_box(min, max);
        Eigen::Vector3f play_box_center(play_box.center());
        Eigen::Vector3f play_box_extent((play_box.max() - play_box.min()));

        float const strength = 100.0f;
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
                Barrier * b = new Plane_barrier(play_box_center - normal * play_box_extent[axis] * 0.5f, normal, strength, radius, extent);

                _level_data._game_field_borders.push_back(b);
                _level_data._barriers.push_back(b);
            }
        }
    }

    std::vector<Brownian_element*> const& get_brownian_elements() const
    {
        return _level_data._brownian_elements;
    }

    Eigen::Vector3f calc_forces_between_atoms(Atom const& a_0, Atom const& a_1) const
    {
        Eigen::Vector3f resulting_force(0.0f, 0.0f, 0.0f);

        for (Atomic_force const* force : _atomic_forces)
        {
            resulting_force += force->calc_force_between_atoms(a_0, a_1);
        }

        return resulting_force;
    }

//    std::unique_ptr<Atomic_force> const& get_atomic_force()
//    {
//        return _atomic_force;
//    }

    boost::optional<Molecule const&> get_molecule(int const id) const
    {
        // TODO: stupid brute force search, maybe better structure
        for (Molecule const& m : _level_data._molecules)
        {
            if (!m._active) continue;

            if (m.get_id() == id)
            {
                return boost::optional<Molecule const&>(m);
            }
        }

        return boost::optional<Molecule const&>();
    }

    void delete_level_element(Level_element * level_element)
    {
        _level_data._molecule_releasers.erase(std::remove(_level_data._molecule_releasers.begin(), _level_data._molecule_releasers.end(), level_element), _level_data._molecule_releasers.end());
        _level_data._barriers.erase(std::remove(_level_data._barriers.begin(), _level_data._barriers.end(), level_element), _level_data._barriers.end());
        _level_data._portals.erase(std::remove(_level_data._portals.begin(), _level_data._portals.end(), level_element), _level_data._portals.end());
        _level_data._brownian_elements.erase(std::remove(_level_data._brownian_elements.begin(), _level_data._brownian_elements.end(), level_element), _level_data._brownian_elements.end());
    }

    void reset_level_elements()
    {
        for (Level_element * e : _level_data._barriers)
        {
            e->reset();
        }

        for (Level_element * e : _level_data._brownian_elements)
        {
            e->reset();
        }

        for (Level_element * e : _level_data._portals)
        {
            e->reset();
        }

        for (Level_element * e : _level_data._molecule_releasers)
        {
            e->reset();
        }
    }

    void start_level()
    {
        reset_level();
    }


    static bool is_not_persistent(Level_element const* e)
    {
        return !e->is_persistent();
    }

    void delete_non_persistent_objects()
    {
        _level_data._barriers.erase(std::remove_if(_level_data._barriers.begin(), _level_data._barriers.end(), is_not_persistent), _level_data._barriers.end());

        _level_data._brownian_elements.erase(std::remove_if(_level_data._brownian_elements.begin(), _level_data._brownian_elements.end(), is_not_persistent),
                                             _level_data._brownian_elements.end());

        _level_data._portals.erase(std::remove_if(_level_data._portals.begin(), _level_data._portals.end(), is_not_persistent),
                                             _level_data._portals.end());

        _level_data._molecule_releasers.erase(std::remove_if(_level_data._molecule_releasers.begin(), _level_data._molecule_releasers.end(), is_not_persistent),
                                             _level_data._molecule_releasers.end());
    }

    void clear()
    {
        // TODO & FIXME: delete stuff or use freaking smart pointers

        _level_data._molecules.clear();
        _level_data._barriers.clear();
        _level_data._brownian_elements.clear();
        _level_data._portals.clear();
        _level_data._game_field_borders.clear();
        _level_data._molecule_releasers.clear();
        _molecule_external_forces.clear();
        _external_forces.clear();
        _molecule_hash.clear();
    }

    void reset_level()
    {
        delete_non_persistent_objects();

        _level_data._molecules.clear();

        _molecule_external_forces.clear();
        _external_forces.clear();
        _molecule_hash.clear();

        reset_level_elements();

        _current_time = 0.0f;
    }

    void save_state(std::string const& file_name) const
    {
        std::ofstream out_file(file_name.c_str(), std::ios_base::binary);
        boost::archive::text_oarchive oa(out_file);

//        std::cout << "Parameter_list::save: " << out_file << std::endl;

        oa << *this;

        out_file.close();
    }

    void load_state(std::string const& file_name)
    {
        std::ifstream in_file(file_name.c_str(), std::ios_base::binary);
        boost::archive::text_iarchive ia(in_file);

        ia >> *this;

        in_file.close();
    }

    void set_parameters(Parameter_list const& parameters)
    {
        _use_indicators = parameters["use_indicators"]->get_value<bool>();
        _rotation_damping = parameters["rotation_damping"]->get_value<float>();
        _translation_damping = parameters["translation_damping"]->get_value<float>();
        _rotation_fluctuation = parameters["rotation_fluctuation"]->get_value<float>();
        _translation_fluctuation = parameters["translation_fluctuation"]->get_value<float>();
        _do_constrain_forces = parameters["do_constrain_forces"]->get_value<bool>();
        _max_force = parameters["max_force"]->get_value<float>();

        _mass_factor = parameters["mass_factor"]->get_value<float>();

        _max_force_distance = parameters["max_force_distance"]->get_value<float>();

        for (auto * f : _atomic_forces)
        {
            delete f;
        }

        _atomic_forces = std::vector<Atomic_force*>(Parameter_registry<Atomic_force>::get_classes_from_multi_select_instance(parameters.get_child("Atomic Force Type")));

        _external_forces["gravity"]._force[2] = -parameters["gravity"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("use_indicators", true));
        parameters.add_parameter(new Parameter("rotation_damping", 0.1f, 0.0f, 10.0f));
        parameters.add_parameter(new Parameter("translation_damping", 0.1f, 0.0f, 10.0f));
        parameters.add_parameter(new Parameter("rotation_fluctuation", 0.1f, 0.0f, 100.0f));
        parameters.add_parameter(new Parameter("translation_fluctuation", 0.1f, 0.0f, 100.0f));
        parameters.add_parameter(new Parameter("mass_factor", 1.0f, 0.01f, 10.0f));
        parameters.add_parameter(new Parameter("do_constrain_forces", true));
        parameters.add_parameter(new Parameter("max_force", 10.0f, 0.1f, 30.0f));
        parameters.add_parameter(new Parameter("max_force_distance", 10.0f, 1.0f, 1000.0f));
        parameters.add_parameter(new Parameter("gravity", 1.0f, 0.0f, 100.0f));

//        Parameter_registry<Atomic_force>::create_single_select_instance(&parameters, "Atomic Force Type");
        Parameter_registry<Atomic_force>::create_multi_select_instance(&parameters, "Atomic Force Type");

        return parameters;
    }

    static Core * create()
    {
        assert(false);
        return NULL;
    }

    static std::string name()
    {
        return "Core";
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
//        ar & _molecules;
        ar & _level_data;
    }

private:
//    std::vector<Molecule> _molecules;

//    std::vector<Barrier*> _game_field_borders;

//    std::vector<Barrier*> _barriers;
//    std::vector<Portal*> _portals;
//    std::vector<Brownian_element*> _brownian_elements;

    Level_data _level_data;

    bool _use_indicators;
    std::vector<Force_indicator> _indicators;

    std::unordered_map<std::string, External_force> _external_forces;

    std::vector<Molecule_external_force> _molecule_external_forces;

    Molecule_external_force _user_force;

//    std::unique_ptr<Atomic_force> _atomic_force;
    std::vector<Atomic_force*> _atomic_forces;

    float _translation_damping;
    float _rotation_damping;

    float _rotation_fluctuation;
    float _translation_fluctuation;

    float _mass_factor;

    float _current_time;

    bool _do_constrain_forces;
    float _max_force;

    float _max_force_distance;

    Molecule_atom_hash _molecule_hash;
    My_tree _tree;
};

REGISTER_BASE_CLASS_WITH_PARAMETERS(Core);

#endif // CORE_H
