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

    _callback();
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
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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
