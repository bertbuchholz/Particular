#include "Help_screen.h"

#include "Core.h"
#include "My_viewer.h"

Help_screen::Help_screen(My_viewer &viewer, Core &core, Screen *calling_state) : Menu_screen(viewer, core),
    _calling_screen(calling_state),
    _current_item_index(0)
{
    _type = Screen::Type::Modal;

    _picking.init(_viewer.context());
}

void Help_screen::init()
{
    int index = 0;

    for (Help_item & item : _help_items)
    {
        Eigen::Vector2f const button_size(0.15f, 0.08f);

        {
            Draggable_label * label = new Draggable_label(
                        Eigen::Vector3f(item._text_rect_position[0] + item._text_rect_size[0] * 0.5f, item._text_rect_position[1] - item._text_rect_size[1] * 0.5f, 0.0f),
                    item._text_rect_size, item._text.toStdString());

            item._text_rect_size = _renderer.generate_flowing_text_label(label, item._text_rect_size[0]);
            label->set_position(Eigen::Vector3f(item._text_rect_position[0] + item._text_rect_size[0] * 0.5f,
                    item._text_rect_position[1] - item._text_rect_size[1] * 0.5f, 0.0f));

            label->set_visible(false);

            _labels.push_back(boost::shared_ptr<Draggable_label>(label));
        }

        {
            float const extra_button_v_distance = 0.01f;

            Eigen::Vector3f const pos(item._text_rect_position[0] + item._text_rect_size[0] - button_size[0] * 0.5f,
                    item._text_rect_position[1] - item._text_rect_size[1] - button_size[1] * 0.5f - extra_button_v_distance,
                    0.0f);

            item._text_rect_size[1] += button_size[1] + extra_button_v_distance;

            Draggable_button * button;

            if (index == int(_help_items.size() - 1))
            {
                button = new Draggable_button(pos, button_size, "Got it!",  std::bind(&Help_screen::continue_game, this));
            }
            else
            {
                button = new Draggable_button(pos, button_size, "Next",  std::bind(&Help_screen::next_help, this));
            }

            button->set_visible(false);

            _renderer.generate_button_texture(button);

            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        if (item._use_particle_system)
        {
            std::vector<Eigen::Vector3f> points;

            float length = 0.0f;

            int const num_points = 20;

            for (int i = 0; i < num_points; ++i)
            {
                points.push_back({item._position[0] + item._radius[0] * std::cos(i / float(num_points - 1) * 2.0f * M_PI),
                                  item._position[1] + item._radius[1] * std::sin(i / float(num_points - 1) * 2.0f * M_PI),
                                  0.0f}
                                 );
            }

            for (int i = 0; i < num_points - 1; ++i)
            {
                length += (points[i] - points[i + 1]).norm();
            }

            item._particle_system = Curved_particle_system(points, 6.0f * length);
            item._particle_system.set_tangent_speed_factor(0.2f);
            item._particle_system.set_particle_size(1.0f);

            float const green = std::rand() / float(RAND_MAX) * 0.26f + 0.2f;
            Color4 curve_color = Color4(1.0f, green, 0.05f, 1.0f);

            item._particle_system.set_curve_color(curve_color);

            item._particle_system.set_effect_color(curve_color);
        }

        ++index;
    }

    _buttons[_current_item_index]->set_visible(true);
    _labels[_current_item_index]->set_visible(true);

    assert(_buttons.size() == _help_items.size() && _labels.size() == _help_items.size());
}

void Help_screen::draw()
{
    Help_item const& item = _help_items[_current_item_index];

    _viewer.start_normalized_screen_coordinates();

    if (item._use_particle_system)
    {
        _viewer.get_renderer().draw_curved_particle_system_in_existing_coord_sys(item._particle_system, _viewer.camera()->screenHeight());
    }

    glColor4f(0.3f, 0.3f, 0.3f, 0.2f);

    Eigen::Vector2f const padding = {0.01f, 0.01f * _viewer.camera()->aspectRatio()};

    glBegin(GL_QUADS);

    glVertex2f(item._text_rect_position[0] - padding[0],                           item._text_rect_position[1] + padding[1]);
    glVertex2f(item._text_rect_position[0] + item._text_rect_size[0] + padding[0], item._text_rect_position[1] + padding[1]);
    glVertex2f(item._text_rect_position[0] + item._text_rect_size[0] + padding[0], item._text_rect_position[1] - item._text_rect_size[1] - padding[1]);
    glVertex2f(item._text_rect_position[0] - padding[0],                           item._text_rect_position[1] - item._text_rect_size[1] - padding[1]);

    glEnd();

    _viewer.stop_normalized_screen_coordinates();

    Menu_screen::draw();
}

void Help_screen::next_help()
{
    _buttons[_current_item_index]->set_visible(false);
    _labels[_current_item_index]->set_visible(false);

    ++_current_item_index;

    assert(_current_item_index < int(_help_items.size()));

    _buttons[_current_item_index]->set_visible(true);
    _labels[_current_item_index]->set_visible(true);
}

void Help_screen::continue_game()
{
    kill();

    _core.set_simulation_state(true);
}

Help_screen *Help_screen::test(My_viewer & viewer, Core & core)
{
    Help_screen * help_screen = new Help_screen(viewer, core, viewer.get_current_screen());

    Help_item item;

    item._text = QString("Molecule Releaser\nThis device releases molecules over time into the container. Blabla blablabla bla blablablabla bla.");
    item._text_rect_position = Eigen::Vector2f(0.2f, 0.8f);
    item._text_rect_size = Eigen::Vector2f(0.5f, 0.5f);
    item._position = {0.8f, 0.8f};
    item._radius = {0.1f, 0.1f};

    help_screen->_help_items.push_back(item);
    help_screen->init();

    return help_screen;
}


void Help_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    if (_help_items[_current_item_index]._use_particle_system)
    {
        _help_items[_current_item_index]._particle_system.animate(time_step);
    }
}
