#ifndef DRAGGABLE_H
#define DRAGGABLE_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "Level_elements.h"
#include "Visitor.h"

class Constraint
{
public:
    virtual ~Constraint() {}

    virtual Eigen::Vector3f apply(Eigen::Vector3f const& position) const = 0;
};

class X_axis_constraint : public Constraint
{
public:
    X_axis_constraint(Eigen::Vector3f const& pos_on_yz_plane) : _position_on_yz_plane(pos_on_yz_plane)
    { }

    Eigen::Vector3f apply(Eigen::Vector3f const& position) const override
    {
        Eigen::Vector3f result(position);

        result[1] = _position_on_yz_plane[1];
        result[2] = _position_on_yz_plane[2];

        return result;
    }

private:
    Eigen::Vector3f _position_on_yz_plane;
};

class Range_constraint : public Constraint
{
public:
    Range_constraint(Eigen::Vector3f const& min, Eigen::Vector3f const& max) : _min(min), _max(max)
    { }

    Eigen::Vector3f apply(Eigen::Vector3f const& position) const override
    {
        Eigen::Vector3f result(position);

        result[0] = into_range(result[0], _min[0], _max[0]);
        result[1] = into_range(result[1], _min[1], _max[1]);
        result[2] = into_range(result[2], _min[2], _max[2]);

        return result;
    }

private:
    Eigen::Vector3f _min, _max;
};

class Y_plane_constraint : public Constraint
{
public:
    Y_plane_constraint(Eigen::Vector3f const& pos_on_plane) : _position_on_plane(pos_on_plane)
    { }

    Eigen::Vector3f apply(Eigen::Vector3f const& position) const override
    {
        Eigen::Vector3f result(position);

        result[1] = _position_on_plane[1];

        return result;
    }

private:
    Eigen::Vector3f _position_on_plane;
};

class Draggable : public Level_element_visitor, public Notifiable
{
public:
    Draggable(Level_element * level_element = nullptr) : Draggable(Eigen::Vector3f::Zero(), level_element)
    { }

    Draggable(Eigen::Vector3f const& position, Level_element * level_element = nullptr) :
        _parent(nullptr), _position(position), _changed(false), _level_element(level_element), _slider_movement_range(3.9f)
    { }

    virtual ~Draggable()
    {
        if (_level_element)
        {
            _level_element->remove_observer(this);
        }
    }

    virtual void update()
    {
        set_changed(true);

        if (_parent)
        {
            _parent->update();
        }

        set_changed(false);
    }

    Eigen::Vector3f const& get_position() const
    {
        return _position;
    }

    Eigen::Vector3f & get_position()
    {
        return _position;
    }

    virtual void set_position(Eigen::Vector3f const& position)
    {
        _position = position;
    }

    void set_position_from_world(Eigen::Vector3f const& position)
    {
        _position = get_transform().inverse() * (position - get_parent()->get_position());

        for (Constraint * c : _constraints)
        {
            _position = c->apply(_position);
        }
    }

    void set_changed(bool const c)
    {
        _changed = c;
    }

    bool is_changed() const
    {
        return _changed;
    }

    void set_parent(Draggable * p)
    {
        _parent = p;
    }

    virtual std::vector<Draggable *> get_draggables()
    {
        return std::vector<Draggable *>();
    }

    virtual Eigen::Transform<float, 3, Eigen::Isometry> get_transform() const
    {
        if (!_parent)
        {
            return Eigen::Transform<float, 3, Eigen::Isometry>::Identity();
        }
        else
        {
            return _parent->get_transform();
        }
    }

    Draggable * get_parent()
    {
        if (_parent)
        {
            return _parent->get_parent();
        }
        else
        {
            return this;
        }
    }

    Draggable const* get_parent() const
    {
        if (_parent)
        {
            return _parent->get_parent();
        }
        else
        {
            return this;
        }
    }

    void add_constraint(Constraint * c)
    {
        _constraints.push_back(c);
    }

    void notify() override
    {
        set_position(_level_element->get_position());
    }

protected:
    Draggable * _parent;

    Eigen::Vector3f _position;

    std::vector<Constraint*> _constraints;

    bool _changed;

    Level_element * _level_element;

    float _slider_movement_range;
};

class Draggable_point : public Draggable
{

};

class Draggable_disc : public Draggable
{

};


class Draggable_box : public Draggable
{
public:
    enum class Corner_type { Min = -1, Max = 1 };
    enum class Axis { X = 0, Y, Z };
    enum class Plane { Neg_X, Neg_Y, Neg_Z, Pos_X, Pos_Y, Pos_Z };

    Draggable_box() {}

    Draggable_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max,
                  std::vector< std::vector<Corner_type> > const& corner_types,
                  std::vector<Plane> const& position_types,
                  std::vector<Plane> const& rotation_types,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr) :
        Draggable(level_element),
        _properties(properties),
        _corner_types(corner_types),
        _position_types(position_types),
        _rotation_types(rotation_types)
    {
        set_position((min + max) * 0.5f);
        _extent_2 = (max - min) * 0.5f;
        _transform = Eigen::Transform<float, 3, Eigen::Isometry>::Identity();

        init();
    }

    Draggable_box(Eigen::Vector3f const& position,
                  Eigen::Vector3f const& extent,
                  Eigen::Transform<float, 3, Eigen::Isometry> const& transform,
                  std::vector< std::vector<Corner_type> > const& corner_types,
                  std::vector<Plane> const& position_types,
                  std::vector<Plane> const& rotation_types,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr) :
        Draggable(position, level_element),
        _extent_2(extent * 0.5f),
        _transform(transform),
        _properties(properties),
        _corner_types(corner_types),
        _position_types(position_types),
        _rotation_types(rotation_types)
    {
        init();
    }

    Draggable_box(Eigen::Vector3f const& position,
                  Eigen::Vector3f const& extent,
                  Eigen::Transform<float, 3, Eigen::Isometry> const& transform,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr) :
        Draggable(position, level_element),
        _extent_2(extent * 0.5f),
        _transform(transform),
        _properties(properties)
    {
        _corner_types = {
            { Corner_type::Min, Corner_type::Min, Corner_type::Min },
//            { Corner_type::Min, Corner_type::Max, Corner_type::Min },
            { Corner_type::Min, Corner_type::Min, Corner_type::Max },
//            { Corner_type::Min, Corner_type::Max, Corner_type::Max },
            { Corner_type::Max, Corner_type::Min, Corner_type::Min },
//            { Corner_type::Max, Corner_type::Max, Corner_type::Min },
            { Corner_type::Max, Corner_type::Min, Corner_type::Max },
//            { Corner_type::Max, Corner_type::Max, Corner_type::Max }
        };

        _position_types = {
//            Plane::Neg_X,
            Plane::Neg_Y
        };

        _rotation_types = {
            Plane::Neg_Y
        };

        init();
    }

    Draggable_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr) :
        Draggable(level_element),
        _properties(properties)
    {
        _corner_types = {
            { Corner_type::Min, Corner_type::Min, Corner_type::Min },
//            { Corner_type::Min, Corner_type::Max, Corner_type::Min },
            { Corner_type::Min, Corner_type::Min, Corner_type::Max },
//            { Corner_type::Min, Corner_type::Max, Corner_type::Max },
            { Corner_type::Max, Corner_type::Min, Corner_type::Min },
//            { Corner_type::Max, Corner_type::Max, Corner_type::Min },
            { Corner_type::Max, Corner_type::Min, Corner_type::Max },
//            { Corner_type::Max, Corner_type::Max, Corner_type::Max }
        };

        _position_types = {
//            Plane::Neg_X,
            Plane::Neg_Y
        };

        _rotation_types = {
            Plane::Neg_Y
        };

        set_position((min + max) * 0.5f);
        _extent_2 = (max - min) * 0.5f;
        _transform = Eigen::Transform<float, 3, Eigen::Isometry>::Identity();

        init();
    }

    void setup_handles()
    {
        for (int i = 0; i < int(_corner_types.size()); ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                _size_handles[i].get_position()[j] = float(_corner_types[i][j]) * _extent_2[j];
            }
        }

        for (int i = 0; i < int(_position_types.size()); ++i)
        {
            Eigen::Vector3f pos(0.0f, 0.0f, 0.0f);

            if (_position_types[i] == Plane::Neg_X)
            {
                pos[0] -= _extent_2[0];
            }
            else if (_position_types[i] == Plane::Neg_Y)
            {
                pos[1] -= _extent_2[1];
            }

            _position_handles[i].set_position(pos);
        }

        float const rotation_handles_distance = 4.0f;

        for (int i = 0; i < int(_rotation_types.size()); ++i)
        {
            Eigen::Vector3f pos_0(0.0f, 0.0f, 0.0f);
            Eigen::Vector3f pos_1(0.0f, 0.0f, 0.0f);

            if (_rotation_types[i] == Plane::Neg_X)
            {
                pos_0[0] -= _extent_2[0];
                pos_1[0] -= _extent_2[0];

                pos_0[1] -= rotation_handles_distance;
                pos_1[1] += rotation_handles_distance;
            }
            else if (_rotation_types[i] == Plane::Neg_Y)
            {
                pos_0[1] -= _extent_2[1];
                pos_1[1] -= _extent_2[1];

                pos_0[0] -= rotation_handles_distance;
                pos_1[0] += rotation_handles_distance;
            }
            else if (_rotation_types[i] == Plane::Pos_Z)
            {
                pos_0[2] += _extent_2[2];
                pos_1[2] += _extent_2[2];

                pos_0[0] -= rotation_handles_distance;
                pos_1[0] += rotation_handles_distance;
            }

            _rotation_handles[2 * i].set_position(pos_0);
            _rotation_handles[2 * i + 1].set_position(pos_1);
        }
    }

    void update() override
    {
        Eigen::Vector3f new_extent = _extent_2;
        Eigen::Vector3f new_position = Eigen::Vector3f::Zero();
        Eigen::Transform<float, 3, Eigen::Isometry> new_transform = _transform;

        for (size_t i = 0; i < _size_handles.size(); ++i)
        {
            Draggable const& d = _size_handles[i];

            if (d.is_changed())
            {
                Eigen::Vector3f local_pos = d.get_position();

                for (int j = 0; j < 3; ++j)
                {
                    new_extent[j] = std::abs(local_pos[j]); // this way, scaling is done from all sides at the same time
                }
            }
        }

        for (size_t i = 0; i < _position_handles.size(); ++i)
        {
            Draggable_disc const& d = _position_handles[i];

            // TODO: handle transformation, currently only works if handle is a rotation axis and there is only this single rotation

            if (d.is_changed())
            {
                if (_position_types[i] == Plane::Neg_X)
                {
                    new_position[1] = d.get_position()[1];
                    new_position[2] = d.get_position()[2];
                }
                else if (_position_types[i] == Plane::Neg_Y)
                {
                    new_position[0] = d.get_position()[0];
                    new_position[2] = d.get_position()[2];
                }
            }
        }

        for (size_t i = 0; i < _rotation_handles.size(); ++i)
        {
            Draggable_disc const& d = _rotation_handles[i];

            if (d.is_changed())
            {
                int const rotation_type_index = i / 2;

                float angle = 0.0f;

                Eigen::Vector3f rotation_axis;

                Eigen::Vector3f local_pos = d.get_position();

                if (_rotation_types[rotation_type_index] == Plane::Neg_X)
                {
                    angle = std::atan2(local_pos[2], local_pos[1]);
                    rotation_axis = -Eigen::Vector3f::UnitX();
                }
                else if (_rotation_types[rotation_type_index] == Plane::Neg_Y)
                {
                     angle = std::atan2(local_pos[2], local_pos[0]);
                     rotation_axis = -Eigen::Vector3f::UnitY();
                }
                else if (_rotation_types[rotation_type_index] == Plane::Pos_Z)
                {
                    angle = std::atan2(local_pos[1], local_pos[0]);
                    rotation_axis = Eigen::Vector3f::UnitZ();
                }

                if ((i % 2) == 0) angle += M_PI;
                new_transform = _transform * Eigen::AngleAxisf(angle, rotation_axis);
            }
        }

        for (auto const p_iter : _property_handles)
        {
            Draggable_point const& p_handle = p_iter.second;
            if (!p_handle.is_changed()) continue;

            Parameter * p = _properties[p_iter.first];

            float new_value = into_range(p_handle.get_position()[0], -_slider_movement_range, _slider_movement_range);
            new_value = (new_value + _slider_movement_range) / (2.0f * _slider_movement_range) * (p->get_max<float>() - p->get_min<float>()) + p->get_min<float>();

            p->set_value_no_update(new_value);
        }

        set_position(get_position() + _transform * new_position);
        _extent_2 = new_extent;
        _transform = new_transform;

        setup_handles();
    }

    std::vector<Draggable_point> const& get_corners() const
    {
        return _size_handles;
    }

    std::vector<Draggable_disc> const& get_position_points() const
    {
        return _position_handles;
    }

    std::vector<Draggable_disc> const& get_rotation_handles() const
    {
        return _rotation_handles;
    }


    std::unordered_map<std::string, Draggable_point> const& get_property_handles() const
    {
        return _property_handles;
    }

    Eigen::Vector3f get_min() const
    {
        return get_position() - _extent_2;
    }

    Eigen::Vector3f get_max() const
    {
        return get_position() + _extent_2;
    }

    std::vector<Draggable*> get_draggables() override
    {
        std::vector<Draggable*> result;

        for (Draggable_point & d : _size_handles)
        {
            result.push_back(&d);
        }

        for (Draggable_disc & d : _position_handles)
        {
            result.push_back(&d);
        }

        for (Draggable_disc & d : _rotation_handles)
        {
            result.push_back(&d);
        }

        for (auto & iter : _property_handles)
        {
            result.push_back(&iter.second);
        }

        return result;
    }

    void set_transform(Eigen::Transform<float, 3, Eigen::Isometry> const& transform)
    {
        _transform = transform;
        setup_handles();
    }

    Eigen::Transform<float, 3, Eigen::Isometry> get_transform() const override
    {
        return _transform;
    }

    void visit(Brownian_box * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Moving_box_barrier * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Box_barrier * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Blow_barrier * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Box_portal * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Plane_barrier * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_extent(Eigen::Vector2f(_extent_2[0] * 2.0f, _extent_2[1] * 2.0f));
        b->set_property_values(_properties);
    }

    void visit(Molecule_releaser * b) const override
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

    void visit(Tractor_barrier * b) const override
    {
        b->set_transform(_transform);
        b->set_position(get_position());
        b->set_size(_extent_2 * 2.0f);
        b->set_property_values(_properties);
    }

private:
    void init()
    {
        _size_handles.resize(_corner_types.size());
        _position_handles.resize(_position_types.size());
        _rotation_handles.resize(_rotation_types.size() * 2);

        setup_handles();

        for (Draggable & d : _size_handles)
        {
            d.set_parent(this);
        }

        for (Draggable & d : _position_handles)
        {
            d.set_parent(this);
        }

        for (Draggable & d : _rotation_handles)
        {
            d.set_parent(this);
        }

        int i = 0;

        for (auto const prop_iter : _properties)
        {
            Parameter const* property = prop_iter.second;

            float const slider_range_3d = _slider_movement_range * 2.0f;
            float const normalized_current_value = (property->get_value<float>() - property->get_min<float>()) / (property->get_max<float>() - property->get_min<float>());

            assert(normalized_current_value >= 0.0f && normalized_current_value <= 1.0f);

            Eigen::Vector3f const center_position(0.0f, -_extent_2[1], -3.6f - 3.0f * i);

            Draggable_point p;
            p.set_parent(this);
            p.set_position(center_position + Eigen::Vector3f(normalized_current_value * slider_range_3d - slider_range_3d / 2.0f, 0.0f, 0.0f));

            p.add_constraint(new Range_constraint(center_position - Eigen::Vector3f(slider_range_3d / 2.0f, 0.0f, 0.0f), center_position + Eigen::Vector3f(slider_range_3d / 2.0f, 0.0f, 0.0f)));

            _property_handles[property->get_name()] = p;

            ++i;
        }
    }

    Eigen::Vector3f _extent_2;
    Eigen::Transform<float, 3, Eigen::Isometry> _transform;

    std::vector<Draggable_point> _size_handles;
    std::vector<Draggable_disc> _position_handles;
    std::vector<Draggable_disc> _rotation_handles;
    std::unordered_map<std::string, Draggable_point> _property_handles;
    std::unordered_map<std::string, Eigen::Vector2f> _property_ranges;

    Parameter_list _properties;

    std::vector< std::vector<Corner_type> > _corner_types;
    std::vector< Plane > _position_types;
    std::vector< Plane > _rotation_types;
};


#endif // DRAGGABLE_H
