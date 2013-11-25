#include "Draggable.h"


Draggable_slider::Draggable_slider(const Eigen::Vector3f &position, const Eigen::Vector2f &size, Parameter *parameter, std::function<void ()> callback) :
    Draggable_label(position, size, ""), _callback(callback), _parameter(parameter)
{
    init();
}


void Draggable_slider::update()
{
    Draggable_screen_point const& p_handle = _property_handle;

    Parameter * p = _parameter;

    float new_value = into_range(p_handle.get_position()[0], -_slider_movement_range, _slider_movement_range);
    new_value = (new_value + _slider_movement_range) / (2.0f * _slider_movement_range) * (p->get_max<float>() - p->get_min<float>()) + p->get_min<float>();

    p->set_value(new_value);

    if (_callback) _callback();
}

std::vector<Draggable *> Draggable_slider::get_draggables(const Level_element::Edit_type)
{
    std::vector<Draggable*> result;

    result.push_back(&_property_handle);

    return result;
}

void Draggable_slider::set_slider_marker_texture(GLuint const texture)
{
    _slider_marker_texture = texture;
}

Eigen::Transform<float, 3, Eigen::Affine> Draggable_slider::get_transform() const
{
    return Eigen::Translation3f(get_position()) * Eigen::Scaling(get_extent()[0] * 0.5f, get_extent()[1] * 0.5f, 1.0f);
}

void Draggable_slider::notify()
{
    float const slider_range_3d = _slider_movement_range * 2.0f;
    float const normalized_current_value = (_parameter->get_value<float>() - _parameter->get_min<float>()) / (_parameter->get_max<float>() - _parameter->get_min<float>());

    assert(normalized_current_value >= 0.0f && normalized_current_value <= 1.0f);

    _property_handle.set_position(Eigen::Vector3f(normalized_current_value * slider_range_3d - slider_range_3d / 2.0f, 0.0f, 0.0f));
}

GLuint Draggable_slider::get_slider_marker_texture() const
{
    return _slider_marker_texture;
}

const Draggable_screen_point &Draggable_slider::get_slider_marker() const
{
    return _property_handle;
}

void Draggable_slider::init()
{
    //        int i = 0;

    //        for (auto const prop_iter : _properties)
    {
        _parameter->add_observer(this);

        Parameter const* property = _parameter;


//        _slider_movement_range = get_extent()[0] * 0.5f;
        _slider_movement_range = 1.0f;
        float const slider_range_3d = _slider_movement_range * 2.0f;
        float const normalized_current_value = (property->get_value<float>() - property->get_min<float>()) / (property->get_max<float>() - property->get_min<float>());

        assert(normalized_current_value >= 0.0f && normalized_current_value <= 1.0f);

        Eigen::Vector3f const center_position = Eigen::Vector3f::Zero();

        Draggable_screen_point p;
        p.set_parent(this);
        p.set_position(center_position + Eigen::Vector3f(normalized_current_value * slider_range_3d - slider_range_3d / 2.0f, 0.0f, 0.0f));

        p.add_constraint(new Range_constraint(center_position - Eigen::Vector3f(slider_range_3d / 2.0f, 0.0f, 0.0f), center_position + Eigen::Vector3f(slider_range_3d / 2.0f, 0.0f, 0.0f)));

        //            _property_handles[property->get_name()] = p;

        _property_handle = p;

        //            ++i;
    }
}

Draggable_slider::~Draggable_slider()
{
    _parameter->remove_observer(this);
}


Draggable_tooltip::~Draggable_tooltip()
{
    std::cout << __FUNCTION__ << std::endl;
}

void Draggable_tooltip::start_fade_in()
{
    _transition_progress = 0.0f;

    _state = State::Fading_in;
}

void Draggable_tooltip::start_fade_out()
{
    _transition_progress = 0.0f;

    _state = State::Fading_in;
}

void Draggable_tooltip::animate(const float timestep)
{
    if (_state == State::Fading_in)
    {
        _transition_progress += timestep / _fade_in_time;

        _alpha = _transition_progress;

        if (_transition_progress >= 1.0f)
        {
            _state = State::None;
        }
    }
    else if (_state == State::Fading_out)
    {
        _transition_progress += timestep / _fade_out_time;

        _alpha = 1.0f - _transition_progress;

        if (_transition_progress >= 1.0f)
        {
            _state = State::None;

            // notify about death
        }
    }
}


Draggable::~Draggable()
{
    if (_level_element)
    {
        _level_element->remove_observer(this);
    }
}

void Draggable::update()
{
    set_changed(true);

    if (_parent)
    {
        _parent->update();
    }

    set_changed(false);
}

const Eigen::Vector3f &Draggable::get_position() const
{
    return _position;
}

Eigen::Vector3f &Draggable::get_position()
{
    return _position;
}

void Draggable::set_position(const Eigen::Vector3f &position)
{
    _position = position;
}

void Draggable::set_position_from_world(const Eigen::Vector3f &position)
{
    //        _position = get_transform().inverse() * position;
    // _transform should contain the position as well, so the subtraction is not necessary. this would make this more general
    _position = get_transform().inverse() * (position - get_parent()->get_position());

    for (Constraint * c : _constraints)
    {
        _position = c->apply(_position);
    }
}

void Draggable::set_changed(const bool c)
{
    _changed = c;
}

bool Draggable::is_changed() const
{
    return _changed;
}

void Draggable::set_parent(Draggable *p)
{
    _parent = p;
}

std::vector<Draggable *> Draggable::get_draggables(const Level_element::Edit_type)
{
    return std::vector<Draggable *>();
}

Eigen::Transform<float, 3, Eigen::Affine> Draggable::get_transform() const
{
    if (!_parent)
    {
        return Eigen::Transform<float, 3, Eigen::Affine>::Identity();
    }
    else
    {
        return _parent->get_transform();
    }
}

const Draggable *Draggable::get_parent() const
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

void Draggable::add_constraint(Constraint *c)
{
    _constraints.push_back(c);
}

void Draggable::notify()
{
    set_position(_level_element->get_position());
}

bool Draggable::is_draggable() const
{
    return _draggable;
}

void Draggable::set_draggable(const bool draggable)
{
    _draggable = draggable;
}

bool Draggable::is_visible() const
{
    return _visible;
}

void Draggable::set_visible(const bool v)
{
    _visible = v;
}

const std::string &Draggable::get_tooltip_text() const
{
    return _tooltip_text;
}

void Draggable::set_tooltip_text(const std::string &tooltip_text)
{
    _tooltip_text = tooltip_text;
}

Draggable *Draggable::get_parent()
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


Draggable_label::Draggable_label(const Eigen::Vector3f &position, const Eigen::Vector2f &size, const std::string &text) :
    Draggable(position), _alpha(1.0f), _texture(0), _extent(size), _text(text)
{
    _draggable = false;
}


std::vector<Draggable *> Draggable_label::get_draggables(const Level_element::Edit_type)
{
    std::vector<Draggable *> elements;

    elements.push_back(this);

    return elements;
}

GLuint Draggable_label::get_texture() const
{
    return _texture;
}

void Draggable_label::set_texture(GLuint texture)
{
    _texture = texture;
}

const std::string &Draggable_label::get_text()
{
    return _text;
}

const Eigen::Vector2f &Draggable_label::get_extent() const
{
    return _extent;
}

float Draggable_label::get_alpha() const
{
    return _alpha;
}

void Draggable_label::set_alpha(const float alpha)
{
    _alpha = alpha;
}


Draggable_atom_label::Draggable_atom_label(const Eigen::Vector3f &position, const Eigen::Vector2f &size, const std::string &text, Level_element *atom, qglviewer::Camera *camera) :
    Draggable_label(position, size, text), _current_time(0.0f), _camera(camera)
{
    _level_element = atom;
    _level_element->add_observer(this);

    _alpha = 0.0f;
}

void Draggable_atom_label::animate(const float timestep)
{
    _current_time += timestep;

    float const blend_duration = 5.0f;

    if (_current_time < blend_duration)
    {
        _alpha = std::min(_current_time / blend_duration, 1.0f);
    }
}

void Draggable_atom_label::notify()
{
    Eigen::Vector3f projected_position = QGLV2Eigen(_camera->projectedCoordinatesOf(Eigen2QGLV(_level_element->get_position())));

    if (QGLV2Eigen(_camera->position()).dot(_level_element->get_position()) < 0.0f) // behind the camera
    {
        projected_position[0] = 1000.0f;
        projected_position[1] = 1000.0f;
    }
    else
    {
        projected_position[0] /= float(_camera->screenWidth());
        projected_position[1] /= float(_camera->screenHeight());
        projected_position[1] = 1.0f - projected_position[1];
        projected_position[2] = 0.0f;
    }

    set_position(projected_position);
}


Draggable_button::Draggable_button(const Eigen::Vector3f &position, const Eigen::Vector2f &size, const std::string &text, std::function<void ()> callback) :
    Draggable_label(position, size, text), _callback(callback), _is_pressable(false), _is_pressed(false), _parameter(nullptr)
{ }

Draggable_button::Draggable_button(const Eigen::Vector3f &position, const Eigen::Vector2f &size, const std::string &text, std::function<void (const std::string &)> callback, const std::string &callback_data) :
    Draggable_label(position, size, text), _callback_with_data(callback), _callback_data(callback_data), _is_pressable(false), _is_pressed(false), _parameter(nullptr)
{ }

Draggable_button::~Draggable_button()
{
    if (_parameter)
    {
        _parameter->remove_observer(this);
    }
}

void Draggable_button::set_pressable(const bool is_pressable)
{
    _is_pressable = is_pressable;
}

void Draggable_button::set_pressed(const bool pressed)
{
    _is_pressed = pressed;
}

bool Draggable_button::is_pressed() const
{
    return _is_pressed;
}

void Draggable_button::reset()
{
    _is_pressed = false;
}

void Draggable_button::set_parameter(Parameter *p)
{
    _parameter = p;
    p->add_observer(this);
}

void Draggable_button::notify()
{
    set_pressed(_parameter->get_value<bool>());
}

void Draggable_button::clicked()
{
    if (_is_pressable)
    {
        _is_pressed = !_is_pressed;
    }

    if (_callback_with_data) _callback_with_data(_callback_data);
    else if (_callback) _callback();
}

bool Draggable_button::is_pressable() const
{
    return _is_pressable;
}


Draggable_statistics::Draggable_statistics(const Eigen::Vector3f &position, const Eigen::Vector2f &size, const std::string &text) :
    Draggable_label(position, size, text), _animation_start(0.0f), _animation_duration(10.0f)
{ }


void Draggable_statistics::animate(const float timestep)
{
    _current_time += timestep;
}

float Draggable_statistics::get_normalized_time() const
{
    return into_range((_current_time - _animation_start) / _animation_duration, 0.0f, 1.0f);
}

void Draggable_statistics::set_values(const std::vector<float> &values)
{
    _values = values;

    auto iter = std::max_element(_values.begin(), _values.end());

    _max_value = 0.0f;

    if (iter != _values.end())
    {
        _max_value = *iter;
    }

    iter = std::min_element(_values.begin(), _values.end());

    _min_value = 0.0f;

    if (iter != _values.end())
    {
        _min_value = *iter;
    }
}

const std::vector<float> &Draggable_statistics::get_values() const
{
    return _values;
}

float Draggable_statistics::get_max_value() const
{
    return _max_value;
}

float Draggable_statistics::get_min_value() const
{
    return _min_value;
}

void Draggable_statistics::reset_animation()
{
    _current_time = 0.0f;
}


Draggable_box::Draggable_box(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const std::vector<std::vector<Draggable_box::Corner_type> > &corner_types, const std::vector<Draggable_box::Plane> &position_types, const std::vector<Draggable_box::Plane> &rotation_types, const Parameter_list &properties, Level_element *level_element) :
    Draggable(level_element),
    _properties(properties),
    _corner_types(corner_types),
    _position_types(position_types),
    _rotation_types(rotation_types)
{
    set_position((min + max) * 0.5f);
    _extent_2 = (max - min) * 0.5f;
    _transform = Eigen::Transform<float, 3, Eigen::Affine>::Identity();

    init();
}


Draggable_box::Draggable_box(const Eigen::Vector3f &position, const Eigen::Vector3f &extent, const Eigen::Transform<float, 3, Eigen::Affine> &transform, const std::vector<std::vector<Draggable_box::Corner_type> > &corner_types, const std::vector<Draggable_box::Plane> &position_types, const std::vector<Draggable_box::Plane> &rotation_types, const Parameter_list &properties, Level_element *level_element) :
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


Draggable_box::Draggable_box(const Eigen::Vector3f &position, const Eigen::Vector3f &extent, const Eigen::Transform<float, 3, Eigen::Affine> &transform, const Parameter_list &properties, Level_element *level_element) :
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


Draggable_box::Draggable_box(const Eigen::Vector3f &min, const Eigen::Vector3f &max, const Parameter_list &properties, Level_element *level_element) :
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
    _transform = Eigen::Transform<float, 3, Eigen::Affine>::Identity();

    init();
}


void Draggable_box::setup_handles()
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

void Draggable_box::update()
{
    Eigen::Vector3f new_extent = _extent_2;
    Eigen::Vector3f new_position = Eigen::Vector3f::Zero();
    Eigen::Transform<float, 3, Eigen::Affine> new_transform = _transform;

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
            int const rotation_type_index = int(i) / 2;

            float angle = 0.0f;

            Eigen::Vector3f rotation_axis = Eigen::Vector3f::Zero();

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

            if ((i % 2) == 0) angle += float(M_PI);
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

const std::vector<Draggable_point> &Draggable_box::get_corners() const
{
    return _size_handles;
}

const std::vector<Draggable_disc> &Draggable_box::get_position_points() const
{
    return _position_handles;
}

const std::vector<Draggable_disc> &Draggable_box::get_rotation_handles() const
{
    return _rotation_handles;
}

const std::unordered_map<std::string, Draggable_point> &Draggable_box::get_property_handles() const
{
    return _property_handles;
}

Eigen::Vector3f Draggable_box::get_min() const
{
    return get_position() - _extent_2;
}

Eigen::Vector3f Draggable_box::get_max() const
{
    return get_position() + _extent_2;
}

std::vector<Draggable *> Draggable_box::get_draggables(const Level_element::Edit_type edit_type)
{
    std::vector<Draggable*> result;

    if (int(edit_type) & int(Level_element::Edit_type::Scale))
    {
        for (Draggable_point & d : _size_handles)
        {
            result.push_back(&d);
        }
    }

    if (int(edit_type) & int(Level_element::Edit_type::Translate))
    {
        for (Draggable_disc & d : _position_handles)
        {
            result.push_back(&d);
        }
    }

    if (int(edit_type) & int(Level_element::Edit_type::Rotate))
    {
        for (Draggable_disc & d : _rotation_handles)
        {
            result.push_back(&d);
        }
    }

    if (int(edit_type) & int(Level_element::Edit_type::Property))
    {
        for (auto & iter : _property_handles)
        {
            result.push_back(&iter.second);
        }
    }

    return result;
}

void Draggable_box::set_transform(const Eigen::Transform<float, 3, Eigen::Affine> &transform)
{
    _transform = transform;
    setup_handles();
}

Eigen::Transform<float, 3, Eigen::Affine> Draggable_box::get_transform() const
{
    return _transform;
}

void Draggable_box::visit(Tractor_barrier *b) const
{
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::init()
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

void Draggable_box::visit(Molecule_releaser *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Plane_barrier *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_extent(Eigen::Vector2f(_extent_2[0] * 2.0f, _extent_2[1] * 2.0f));
    b->set_property_values(_properties);
}

void Draggable_box::visit(Sphere_portal *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Box_portal *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Blow_barrier *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Charged_barrier *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Box_barrier *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Moving_box_barrier *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}

void Draggable_box::visit(Brownian_box *b) const
{
    std::cout << __FUNCTION__ << std::endl;
    b->set_transform(_transform);
    b->set_position(get_position());
    b->set_size(_extent_2 * 2.0f);
    b->set_property_values(_properties);
}


Draggable_spinbox::Draggable_spinbox(const Eigen::Vector3f &position, const Eigen::Vector2f &size, Parameter *parameter, std::function<void ()> callback) :
    Draggable_label(position, size, ""), _callback(callback), _parameter(parameter)
{
    init();
}


void Draggable_spinbox::init()
{
    _parameter->add_observer(this);

    _increment_draggable.set_parent(this);
    _increment_draggable.set_position({0.0f, 0.75f, 0.0f});
    _increment_draggable.set_draggable(false);

    _decrement_draggable.set_parent(this);
    _decrement_draggable.set_position({0.0f, -0.75f, 0.0f});
    _decrement_draggable.set_draggable(false);
}

void Draggable_spinbox::update()
{
    std::cout << __FUNCTION__ << " implement!" << std::endl;

    int new_value = _parameter->get_value<int>();

    if (_increment_draggable.is_changed())
    {
        new_value += 1;
    }
    else if (_decrement_draggable.is_changed())
    {
        new_value -= 1;
    }

    _parameter->set_value(into_range(new_value, _parameter->get_min<int>(), _parameter->get_max<int>()));

    if (_callback) _callback();
}

Draggable_spinbox::~Draggable_spinbox()
{
    _parameter->remove_observer(this);
}

void Draggable_spinbox::notify()
{ }

std::vector<Draggable *> Draggable_spinbox::get_draggables(const Level_element::Edit_type)
{
    std::vector<Draggable*> result;

    result.push_back(&_increment_draggable);
    result.push_back(&_decrement_draggable);

    return result;
}

Eigen::Transform<float, 3, Eigen::Affine> Draggable_spinbox::get_transform() const
{
    return Eigen::Translation3f(get_position()) * Eigen::Scaling(get_extent()[0] * 0.5f, get_extent()[1] * 0.5f, 1.0f);
}

int Draggable_spinbox::get_value() const
{
    return _parameter->get_value<int>();
}
