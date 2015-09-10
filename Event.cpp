#include "Event.h"

#include "Core.h"
#include "My_viewer.h"
#include "Help_screen.h"


Eigen::AlignedBox2f get_projected_outlines_bb(My_viewer const& viewer, Eigen::AlignedBox3f const& aabb)
{
    Eigen::Vector3f max_with_min_y = aabb.max();
    max_with_min_y[1] = aabb.min()[1];

    Eigen::Vector2f const aabb_projected_min = viewer.get_projected_coordinates(aabb.min());
    Eigen::Vector2f const aabb_projected_max = viewer.get_projected_coordinates(aabb.max());
    Eigen::Vector2f const aabb_projected_max_with_min_y = viewer.get_projected_coordinates(max_with_min_y);

    Eigen::Vector2f const aabb_projected_rect_max({std::max(aabb_projected_max[0], aabb_projected_max_with_min_y[0]),
                                                   std::max(aabb_projected_max[1], aabb_projected_max_with_min_y[1])});

    return Eigen::AlignedBox2f(aabb_projected_min, aabb_projected_rect_max);
}

bool Molecule_releaser_event::trigger()
{
    if (_core.get_current_time() < 5.0f) return false;

    if (_core.get_level_data()._molecule_releasers.size() != 1) return false;

    Molecule_releaser const* m = _core.get_level_data()._molecule_releasers.back();

    Eigen::AlignedBox3f aabb = m->get_world_aabb();

    Eigen::AlignedBox2f projected_aabb = get_projected_outlines_bb(_viewer, aabb);
    Eigen::Vector2f const aabb_projected_min = projected_aabb.min();
    Eigen::Vector2f const aabb_projected_max = projected_aabb.max();

    Eigen::Vector2f const projected_position = projected_aabb.center();

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._position = projected_position;
    item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
            0.5f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
    item._text = QString("Molecule Releaser\nThis device releases molecules over time into the container.");
    item._text_rect_position = projected_position + Eigen::Vector2f(item._radius[0], 0.1f);
    item._text_rect_size = Eigen::Vector2f(1.0f - projected_position[0] - 0.2f, 1.0f - projected_position[1] - 0.2f);
    item._use_particle_system = true;

    help_screen->_help_items.push_back(item);

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Portal_event::trigger()
{
    if (_core.get_current_time() < 9.0f) return false;

    assert(_core.get_level_data()._portals.size() == 1);

    Portal const* p = _core.get_level_data()._portals.back();

    Eigen::AlignedBox3f aabb = p->get_world_aabb();

    Eigen::AlignedBox2f projected_aabb = get_projected_outlines_bb(_viewer, aabb);
    Eigen::Vector2f const aabb_projected_min = projected_aabb.min();
    Eigen::Vector2f const aabb_projected_max = projected_aabb.max();

    Eigen::Vector2f const projected_position = projected_aabb.center();

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_size = Eigen::Vector2f(projected_position[0] - 0.2f, 1.0f - projected_position[1] - 0.2f);
    item._use_particle_system = true;

    {
        item._position = projected_position;
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
                0.5f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
        item._text_rect_position = Eigen::Vector2f(projected_position[0] - item._text_rect_size[0] - item._radius[0],
                projected_position[1] + 0.1f);
        item._text = QString("Collector\nThis device collects molecules. To complete a level, you need to gather a certain amount of molecules in the portal.");

        help_screen->_help_items.push_back(item);
    }

    {
        item._position = projected_position;
        item._position[1] = aabb_projected_min[1] - 0.02f;
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]), 0.05f);
        item._text = QString("Collector\nThe amount is indicated by the green progress bar attached to the portal.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Heat_button_event::trigger()
{
    if (_core.get_current_time() < 5.0f) return false;

    Eigen::Vector2f heat_button_pos(0.05f, 0.95f); // from void Main_game_screen::update_level_element_buttons()
    Eigen::Vector2f heat_button_size(0.04f, 0.04f * _viewer.camera()->aspectRatio());

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_size = Eigen::Vector2f(0.4f, 0.4f);
    item._text_rect_position = Eigen::Vector2f(0.05f, 0.87f);
    item._use_particle_system = true;

    {
        item._position = heat_button_pos + Eigen::Vector2f(0.05f, 0.0f);
        item._radius = heat_button_size.cwiseProduct(Eigen::Vector2f(2.0f, 0.7f));
        item._text = QString("Available Elements\n"
                             "Up here, you find the game elements that you can freely use to manipulate the molecules in each level.");

        help_screen->_help_items.push_back(item);
    }

    {
        item._position = heat_button_pos;
        item._radius = heat_button_size * 0.7f;
        item._text = QString("Temperature Element\n"
                             "This one is the temperature element from the intro. "
                             "Click the button and then click in the container to place the element.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Static_existing_heat_element_event::trigger()
{
    if (_core.get_current_time() < 13.0f) return false;

    Brownian_element const* b = _core.get_level_data()._brownian_elements.back();

    Eigen::AlignedBox3f aabb = b->get_world_aabb();

    Eigen::AlignedBox2f projected_aabb = get_projected_outlines_bb(_viewer, aabb);
    Eigen::Vector2f const aabb_projected_min = projected_aabb.min();
    Eigen::Vector2f const aabb_projected_max = projected_aabb.max();

    Eigen::Vector2f const projected_position = projected_aabb.center();

    Eigen::Vector3f front_center = b->get_position();
    front_center[1] = aabb.min()[1];

    Eigen::Vector2f const projected_front_center = _viewer.get_projected_coordinates(front_center);

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_position = Eigen::Vector2f(0.15f, (projected_position[1] < 0.5f) ? 0.9f : 0.4f);
    item._text_rect_size = Eigen::Vector2f(0.7f, 0.4f);
    item._use_particle_system = true;

    {
        item._position = projected_position;
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
                0.5f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
        item._text = QString("Temperature element\nHeat is motion of atoms and molecules. If you heat up an area, molecules will start to move around faster, "
                             "if you cool it down, they move slower.");

        help_screen->_help_items.push_back(item);
    }

    {
        item._position = projected_front_center + Eigen::Vector2f(0.0f, -projected_aabb.sizes()[1] * 0.1f);
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
                0.1f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
        item._text = QString("Temperature element\nThe slider below the center of the element allows you to change the temperature. "
                             "Try now to increase the temperature to agitate the molecules.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Movable_existing_heat_element_event::trigger()
{
    if (_core.get_current_time() < 5.0f) return false;

    Brownian_element const* b = _core.get_level_data()._brownian_elements.back();

    Eigen::AlignedBox3f aabb = b->get_world_aabb();

    Eigen::AlignedBox2f projected_aabb = get_projected_outlines_bb(_viewer, aabb);
    Eigen::Vector2f const aabb_projected_min = projected_aabb.min();
    Eigen::Vector2f const aabb_projected_max = projected_aabb.max();

    Eigen::Vector2f const projected_position = projected_aabb.center();

    Eigen::Vector3f front_center = b->get_position();
    front_center[1] = aabb.min()[1];

    Eigen::Vector2f const projected_front_center = _viewer.get_projected_coordinates(front_center);

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_position = Eigen::Vector2f(0.15f, (projected_position[1] < 0.5f) ? 0.9f : 0.4f);
    item._text_rect_size = Eigen::Vector2f(0.7f, 0.4f);
    item._use_particle_system = true;

    {
        item._position = projected_position;
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
                0.5f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
        item._text = QString("Temperature element\nThe element is already hot. Move and scale the element with the buttons in its center and the "
                             "corners so that it agitates the molecules.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Heat_element_placed_event::trigger()
{
    if (_core.get_level_data()._brownian_elements.size() == 0) return false;

    Brownian_element const* b = _core.get_level_data()._brownian_elements.back();

    Eigen::AlignedBox3f aabb = b->get_world_aabb();

    Eigen::AlignedBox2f projected_aabb = get_projected_outlines_bb(_viewer, aabb);
    Eigen::Vector2f const aabb_projected_min = projected_aabb.min();
    Eigen::Vector2f const aabb_projected_max = projected_aabb.max();

    Eigen::Vector2f const projected_position = projected_aabb.center();

    Eigen::Vector3f front_center = b->get_position();
    front_center[1] = aabb.min()[1];

    Eigen::Vector2f const projected_front_center = _viewer.get_projected_coordinates(front_center);

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_position = Eigen::Vector2f(0.15f, (projected_position[1] < 0.5f) ? 0.9f : 0.4f);
    item._text_rect_size = Eigen::Vector2f(0.7f, 0.4f);
    item._use_particle_system = true;

    {
        item._position = projected_position;
        item._radius = Eigen::Vector2f(0.5f * std::abs(aabb_projected_max[0] - aabb_projected_min[0]),
                0.5f * std::abs(aabb_projected_max[1] - aabb_projected_min[1]));
        item._text = QString("Well done. Now as before, change the temperature, location and size of the element to bring "
                             "the molecules into the green portal on the right.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}


bool Heat_turned_up_event::trigger()
{
    assert(_core.get_level_data()._brownian_elements.size() == 1);

    Brownian_element const* b = _core.get_level_data()._brownian_elements.back();

    float const strength_threshold = 0.6f * b->get_parameters()["strength"]->get_max<float>();

    if (b->get_strength() < strength_threshold)
    {
        _last_time_too_low = _core.get_current_time();
        return false;
    }

    if (_core.get_current_time() - _last_time_too_low < 1.0f) return false;

    _core.set_simulation_state(false);

    Help_screen * help_screen = new Help_screen(_viewer, _core, _calling_screen);

    Help_screen::Help_item item;
    item._text_rect_size = Eigen::Vector2f(0.4f, 0.4f);
    item._text_rect_position = Eigen::Vector2f(0.3f, 0.3f);
    item._use_particle_system = false;

    {
        item._text = QString("Well done! The molecules will now move faster in the hot area and diffuse into the green portal.");

        help_screen->_help_items.push_back(item);
    }

    help_screen->init();

    _viewer.add_screen_delayed(help_screen);

    return true;
}
