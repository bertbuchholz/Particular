#ifndef DRAGGABLE_H
#define DRAGGABLE_H

#include <vector>
#include <functional>
#include <QVariantAnimation>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <QGLViewer/camera.h>

#include "Level_element.h"
#include "Molecule_releaser.h"
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
        _parent(nullptr), _position(position), _changed(false), _level_element(level_element), _draggable(true), _slider_movement_range(3.9f), _visible(true)
    { }

    virtual ~Draggable();

    virtual void animate(float const /* timestep */) {}

    virtual void update();

    Eigen::Vector3f const& get_position() const;

    Eigen::Vector3f & get_position();

    virtual void set_position(Eigen::Vector3f const& position);

    void set_position_from_world(Eigen::Vector3f const& position);

    void set_changed(bool const c);

    bool is_changed() const;

    void set_parent(Draggable * p);

    virtual std::vector<Draggable *> get_draggables(Level_element::Edit_type const /* edit_type */);

    virtual Eigen::Transform<float, 3, Eigen::Affine> get_transform() const;

    Draggable * get_parent();

    Draggable const* get_parent() const;

    void set_callback_self(std::function<void(Draggable * self)> const& callback)
    {
        _callback_self = callback;
    }

    void add_constraint(Constraint * c);

    void notify() override;

    bool is_draggable() const;
    void set_draggable(bool const draggable);

    bool is_visible() const;
    void set_visible(bool const v);

    std::string const& get_tooltip_text() const;
    void set_tooltip_text(std::string const& tooltip_text);

    virtual void clicked()
    {
        if (_callback_self)
        {
            _callback_self(this);
        }
    }

protected:
    Draggable * _parent;

    Eigen::Vector3f _position;

    std::vector<Constraint*> _constraints;

    bool _changed;

    Level_element * _level_element;

    bool _draggable;

    float _slider_movement_range;

    bool _visible;

    std::string _tooltip_text;

    std::function<void(Draggable *)> _callback_self;
};

class Draggable_screen_point : public Draggable
{

};

class Draggable_point : public Draggable
{

};

class Draggable_disc : public Draggable
{

};

class Draggable_label : public Draggable
{
public:
    Draggable_label() : _texture(0) {}

    Draggable_label(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text);

    ~Draggable_label();

    std::vector<Draggable *> get_draggables(Level_element::Edit_type const /* edit_type */) override;

    GLuint get_texture() const;
    void set_texture(GLuint texture);

    void set_text(std::string const& text);
    std::string const& get_text() const;

    Eigen::Vector2f const& get_extent() const;
    void set_extent(Eigen::Vector2f const& extent);

    float get_alpha() const;
    void set_alpha(float const alpha);

    Color4 const& get_color() const;
    void set_color(const Color4 &color);

protected:
    float _alpha;

private:
    GLuint _texture;

    Eigen::Vector2f _extent;

    std::string _text;

    Color4 _color;
};


class Draggable_tooltip : public Draggable_label
{
public:
    enum class State { None, Fading_in, Fading_out };

    Draggable_tooltip(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text) :
        Draggable_label(position, size, text), _fade_in_time(0.3f), _fade_out_time(0.3f), _transition_progress(0.0f), _state(State::None)
    { }

    ~Draggable_tooltip();

    void start_fade_in();

    void start_fade_out();

    void animate(const float timestep) override;


protected:
    float _fade_in_time;
    float _fade_out_time;

    float _transition_progress;

    State _state;
};

class Draggable_atom_label : public Draggable_label
{
public:
    Draggable_atom_label(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text, Level_element * atom, qglviewer::Camera * camera);

    void animate(const float timestep) override;

    void notify() override;

private:
    float _current_time;
    qglviewer::Camera const* _camera;
};


class Draggable_button : public Draggable_label
{
public:
    Draggable_button(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text, std::function<void(void)> callback);

    Draggable_button(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text, std::function<void(std::string const&)> callback, std::string const& callback_data);

    ~Draggable_button();

    void set_pressable(bool const is_pressable);
    bool is_pressable() const;

    void set_pressed(bool const pressed);
    bool is_pressed() const;

    void reset();

    void set_parameter(Parameter * p);

    void notify() override;

    void clicked() override;

private:
    std::function<void(void)> _callback;
    std::function<void(std::string const&)> _callback_with_data;

    std::string _callback_data;

    bool _is_pressable;
    bool _is_pressed;

    Parameter * _parameter;
};

class Draggable_statistics : public Draggable_label
{
public:
    enum class Display_type { Int, Float, Percentage };

    Draggable_statistics() {}

    Draggable_statistics(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text);

    void animate(const float timestep) override;

    float get_normalized_time() const;

    void set_values(std::vector<float> const& values, boost::optional<float> const& min_value = boost::optional<float>(), boost::optional<float> const& max_value = boost::optional<float>());
    std::vector<float> const& get_values() const;

    float get_max_value() const;
    float get_min_value() const;

    void reset_animation();

    void set_duration(float const duration);

    Curved_particle_system const& get_particle_system() const { return _particle_system; }
    Curved_particle_system & get_particle_system() { return _particle_system; }

    void set_display_type(Display_type const display_type) { _display_type = display_type; }
    Display_type get_display_type() const { return _display_type; }

private:
    std::string _x_label;
    std::string _y_label;

    float _current_time;
    float _animation_start;
    float _animation_duration;

    std::vector<float> _values;
    float _max_value;
    float _min_value;

    Curved_particle_system _particle_system;

    Display_type _display_type;

    std::function<void(void)> _finish_callback;
};


class Draggable_slider : public Draggable_label
{
public:

    Draggable_slider() {}

//    Draggable_slider(Eigen::Vector3f const& position, Eigen::Vector2f const& size, std::string const& text, std::function<void(void)> callback) :
    Draggable_slider(Eigen::Vector3f const& position, Eigen::Vector2f const& size, Parameter * parameter, std::function<void(void)> callback = std::function<void(void)>());

    ~Draggable_slider();

    void update() override;

    std::vector<Draggable*> get_draggables(Level_element::Edit_type const /* edit_type */) override;

    const Draggable_screen_point &get_slider_marker() const;

    GLuint get_slider_marker_texture() const;
    void set_slider_marker_texture(GLuint const texture);

    Eigen::Transform<float, 3, Eigen::Affine> get_transform() const override;

    void notify() override;

private:
    void init();

    Draggable_screen_point  _property_handle;

    std::function<void(void)> _callback;

    Parameter * _parameter;

    GLuint _slider_marker_texture;
};


class Draggable_spinbox : public Draggable_label
{
public:

    Draggable_spinbox() {}

    Draggable_spinbox(Eigen::Vector3f const& position, Eigen::Vector2f const& size, Parameter * parameter, std::function<void(void)> callback = std::function<void(void)>());

    ~Draggable_spinbox();

    void update() override;

    std::vector<Draggable*> get_draggables(Level_element::Edit_type const /* edit_type */) override;

    Draggable_screen_point const& get_increment_draggable() const { return _increment_draggable; }
    Draggable_screen_point const& get_decrement_draggable() const { return _decrement_draggable; }

//    GLuint get_slider_marker_texture() const;
//    void set_slider_marker_texture(GLuint const texture);

    Eigen::Transform<float, 3, Eigen::Affine> get_transform() const override;

    int get_value() const;

    void notify() override;

private:
    void init();

    Draggable_screen_point _increment_draggable;
    Draggable_screen_point _decrement_draggable;

    std::function<void(void)> _callback;

    Parameter * _parameter;

//    GLuint _slider_marker_texture;
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
                  Level_element * level_element = nullptr);

    Draggable_box(Eigen::Vector3f const& position,
                  Eigen::Vector3f const& extent,
                  Eigen::Transform<float, 3, Eigen::Affine> const& transform,
                  std::vector< std::vector<Corner_type> > const& corner_types,
                  std::vector<Plane> const& position_types,
                  std::vector<Plane> const& rotation_types,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr);

    Draggable_box(Eigen::Vector3f const& position,
                  Eigen::Vector3f const& extent,
                  Eigen::Transform<float, 3, Eigen::Affine> const& transform,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr);

    Draggable_box(Eigen::Vector3f const& min, Eigen::Vector3f const& max,
                  Parameter_list const& properties = Parameter_list(),
                  Level_element * level_element = nullptr);

    void setup_handles();

    void update() override;

    std::vector<Draggable_point> const& get_corners() const;
    std::vector<Draggable_disc> const& get_position_points() const;
    std::vector<Draggable_disc> const& get_rotation_handles() const;
    std::unordered_map<std::string, Draggable_point> const& get_property_handles() const;
    Draggable_disc const& get_setting_handle() const;
    Draggable_disc      & get_setting_handle();


    Eigen::Vector3f get_min() const;
    Eigen::Vector3f get_max() const;

    std::vector<Draggable*> get_draggables(Level_element::Edit_type const edit_type) override;

    void set_transform(Eigen::Transform<float, 3, Eigen::Affine> const& transform);
    Eigen::Transform<float, 3, Eigen::Affine> get_transform() const override;

    void visit(Brownian_box * b) const override;
    void visit(Moving_box_barrier * b) const override;
    void visit(Box_barrier * b) const override;
    void visit(Charged_barrier * b) const override;
    void visit(Blow_barrier * b) const override;
    void visit(Box_portal * b) const override;
    void visit(Sphere_portal * b) const override;
    void visit(Plane_barrier * b) const override;
    void visit(Molecule_releaser * b) const override;
    void visit(Tractor_barrier * b) const override;

private:
    void init();

    Eigen::Vector3f _extent_2;
    Eigen::Transform<float, 3, Eigen::Affine> _transform;

    std::vector<Draggable_point> _size_handles;
    std::vector<Draggable_disc> _position_handles;
    std::vector<Draggable_disc> _rotation_handles;
    Draggable_disc _setting_handle;
    std::unordered_map<std::string, Draggable_point> _property_handles;
    std::unordered_map<std::string, Eigen::Vector2f> _property_ranges;

    Parameter_list _properties;

    std::vector< std::vector<Corner_type> > _corner_types;
    std::vector< Plane > _position_types;
    std::vector< Plane > _rotation_types;
};


#endif // DRAGGABLE_H

