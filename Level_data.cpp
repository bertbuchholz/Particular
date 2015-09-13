#include "Level_data.h"

#include "End_condition.h"
#include "Main_options_window.h"


Level_data::Level_data()
{
    std::cout << __FUNCTION__ << std::endl;

    std::function<void(void)> update_variables = std::bind(&Level_data::update_variables, this);

    _parameters.add_parameter(new Parameter("Game Field Width",  80.0f, 40.0f, 200.0f, std::bind(&Level_data::change_game_field_borders, this)));
    _parameters.add_parameter(new Parameter("Game Field Height", 40.0f, 20.0f, 200.0f, std::bind(&Level_data::change_game_field_borders, this)));
    _parameters.add_parameter(new Parameter("Game Field Depth",  40.0f, 5.0f,  100.0f, std::bind(&Level_data::change_game_field_borders, this)));

    _parameters.add_parameter(new Parameter("score_time_factor", 60.0f, 1.0f, 3000.0f, update_variables));

    _parameters.add_parameter(new Parameter("rotation_damping", 0.3f, 0.1f, 1.0f, update_variables));
    _parameters.add_parameter(new Parameter("translation_damping", 0.3f, 0.1f, 1.0f, update_variables));
    _parameters.add_parameter(new Parameter("rotation_fluctuation", 0.0f, -50.0f, 50.0f, update_variables));
    _parameters.add_parameter(new Parameter("translation_fluctuation", 0.0f, -50.0f, 50.0f, update_variables));
    _parameters.add_parameter(new Parameter("Temperature", 0.0f, -50.0f, 50.0f, update_variables));
    _parameters.add_parameter(new Parameter("Damping", 0.3f, 0.1f, 1.0f, update_variables));

    _parameters.add_parameter(new Parameter("gravity", 0.0f, 0.0f, 10.0f, update_variables));

    _parameters.add_parameter(new Parameter("background_name", std::string("iss_interior_1.png")));

    Parameter_list * available_list = _parameters.add_child("Available elements");
    available_list->add_parameter(new Parameter("Box_barrier", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Brownian_box", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Box_portal", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Sphere_portal", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Molecule_releaser", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Charged_barrier", 0, 0, 9, update_variables));
    available_list->add_parameter(new Parameter("Tractor_barrier", 0, 0, 9, update_variables));

    _temperature_grid.set_size(10, 10);

    update_variables();
}

bool Level_data::validate_elements()
{
    std::cout << __FUNCTION__ << " "
              << _level_elements.size() << " "
              << _barriers.size() << " "
              << _portals.size() << " "
              << _molecule_releasers.size() << " "
              << _brownian_elements.size()
              << std::endl;

    std::cout << "LE: " << _level_elements << "\n"
              << "BA: " << _barriers << "\n"
              << "PO: " << _portals << "\n"
              << "MR: " << _molecule_releasers << "\n"
              << "BE: " << _brownian_elements << "\n";

    for (auto const& e : _level_elements)
    {
        Level_element const* e_ptr = e.get();

        if (std::find(_barriers.begin(), _barriers.end(), e_ptr) == _barriers.end() &&
                std::find(_portals.begin(), _portals.end(), e_ptr) == _portals.end() &&
                std::find(_molecule_releasers.begin(), _molecule_releasers.end(), e_ptr) == _molecule_releasers.end() &&
                std::find(_brownian_elements.begin(), _brownian_elements.end(), e_ptr) == _brownian_elements.end()
                )
        {
            assert(false);
            return false;
        }

    }

    for (auto const& iter : _game_field_borders)
    {
        Barrier const* b = iter.second;

        if (std::find(_barriers.begin(), _barriers.end(), b) == _barriers.end())
        {
            assert(false);
            return false;
        }
    }

    return true;
}

void Level_data::load_defaults()
{
    _parameters["Game Field Width"]->set_value(80.0f);
    _parameters["Game Field Height"]->set_value(40.0f);
    _parameters["Game Field Depth"]->set_value(40.0f);

    _parameters["Temperature"]->set_value(0.0f);
    _parameters["Damping"]->set_value(0.3f);
    _parameters["gravity"]->set_value(0.0f);

    update_variables();
}

void Level_data::update_variables()
{
    _score_time_factor = _parameters["score_time_factor"]->get_value<float>();
    _background_name = _parameters["background_name"]->get_value<std::string>();

//    _rotation_damping = _parameters["rotation_damping"]->get_value<float>();
//    _translation_damping = _parameters["translation_damping"]->get_value<float>();
//    _rotation_fluctuation = _parameters["rotation_fluctuation"]->get_value<float>();
//    _translation_fluctuation = _parameters["translation_fluctuation"]->get_value<float>();

    _rotation_fluctuation = _parameters["Temperature"]->get_value<float>();
    _translation_fluctuation = _parameters["Temperature"]->get_value<float>();

    _rotation_damping = _parameters["Damping"]->get_value<float>();
    _translation_damping = _parameters["Damping"]->get_value<float>();

    //        _gravity = _parameters["gravity"]->get_value<float>();
    _external_forces["gravity"]._force[2] = -_parameters["gravity"]->get_value<float>();

    _game_field_width = _parameters["Game Field Width"]->get_value<float>();
    _game_field_height = _parameters["Game Field Height"]->get_value<float>();

    Parameter_list const* available_list = _parameters.get_child("Available elements");

    for (auto const& iter : *available_list)
    {
        _available_elements[iter.first] = iter.second->get_value<int>();
    }

    update_parameters(); // FIXME: need to update parameters in case one of the meta params has been changed so UI is updated for those values depending on them
}

void Level_data::update_parameters()
{
    _parameters["score_time_factor"]->set_value_no_update(_score_time_factor);
    _parameters["background_name"]->set_value_no_update(_background_name);

    _parameters["rotation_damping"]->set_value_no_update(_rotation_damping);
    _parameters["translation_damping"]->set_value_no_update(_translation_damping);
    _parameters["rotation_fluctuation"]->set_value_no_update(_rotation_fluctuation);
    _parameters["translation_fluctuation"]->set_value_no_update(_translation_fluctuation);

    _parameters["Temperature"]->set_value_no_update((_rotation_fluctuation + _translation_fluctuation) * 0.5f);
    _parameters["Damping"]->set_value_no_update((_rotation_damping + _translation_damping) * 0.5f);

    //        _parameters["gravity"]->set_value_no_update(_gravity);
    _parameters["gravity"]->set_value_no_update(-_external_forces["gravity"]._force[2]);

    if (_game_field_borders.size() == 6)
    {
        _parameters["Game Field Width"]->set_value_no_update(_game_field_borders[Plane::Pos_X]->get_position()[0] -
                _game_field_borders[Plane::Neg_X]->get_position()[0]);
        _parameters["Game Field Depth"]->set_value_no_update(_game_field_borders[Plane::Pos_Y]->get_position()[1] -
                _game_field_borders[Plane::Neg_Y]->get_position()[1]);
        _parameters["Game Field Height"]->set_value_no_update(_game_field_borders[Plane::Pos_Z]->get_position()[2] -
                _game_field_borders[Plane::Neg_Z]->get_position()[2]);
    }

    Parameter_list * available_list = _parameters.get_child("Available elements");

    for (auto & iter : *available_list)
    {
        iter.second->set_value_no_update(_available_elements[iter.first]);
    }
}

float Level_data::get_temperature(Eigen::Vector3f const& world_pos) const
{
    return _temperature_grid.get_interpolated_value(
            into_range(world_pos[0] / (_game_field_width * 0.5f) * 0.5f + 0.5f, 0.0f, 1.0f),
            into_range(world_pos[2] / (_game_field_height * 0.5f) * 0.5f + 0.5f, 0.0f, 1.0f));
}

void Level_data::add_molecule_releaser(Molecule_releaser *molecule_releaser)
{
    _molecule_releasers.push_back(molecule_releaser);
    _level_elements.push_back(boost::shared_ptr<Level_element>(molecule_releaser));
}

void Level_data::add_portal(Portal *portal)
{
    _portals.push_back(portal);
    _level_elements.push_back(boost::shared_ptr<Level_element>(portal));
}

void Level_data::add_brownian_element(Brownian_element *element)
{
    _brownian_elements.push_back(element);
    _level_elements.push_back(boost::shared_ptr<Level_element>(element));
}

void Level_data::add_barrier(Barrier *barrier)
{
    _barriers.push_back(barrier);
    _level_elements.push_back(boost::shared_ptr<Level_element>(barrier));
}

void Level_data::change_game_field_borders()
{
    //        Eigen::Vector3f min(_parameters["game_field_left"]->get_value<float>(),
    //                _parameters["game_field_front"]->get_value<float>(),
    //                _parameters["game_field_bottom"]->get_value<float>());

    Eigen::Vector3f min(-0.5f * _parameters["Game Field Width"]->get_value<float>(),
            -0.5f * _parameters["Game Field Depth"]->get_value<float>(),
            -0.5f * _parameters["Game Field Height"]->get_value<float>());

    Eigen::Vector3f max = -min;

    //        Eigen::Vector3f max(_parameters["game_field_right"]->get_value<float>(),
    //                _parameters["game_field_back"]->get_value<float>(),
    //                _parameters["game_field_top"]->get_value<float>());

    set_game_field_borders(min, max);
}

void Level_data::set_game_field_borders(const Eigen::Vector3f &min, const Eigen::Vector3f &max)
{
    for (auto const& e : _game_field_borders)
    {
        Level_element * b = e.second;
        delete_level_element(b);
    }

    _game_field_borders.clear();

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
            _game_field_borders[plane] = b;
            add_barrier(b);
        }
    }
}

void Level_data::delete_level_element(Level_element *level_element)
{
    _molecule_releasers.erase(std::remove(_molecule_releasers.begin(), _molecule_releasers.end(), level_element), _molecule_releasers.end());
    _barriers.erase(std::remove(_barriers.begin(), _barriers.end(), level_element), _barriers.end());
    _portals.erase(std::remove(_portals.begin(), _portals.end(), level_element), _portals.end());
    _brownian_elements.erase(std::remove(_brownian_elements.begin(), _brownian_elements.end(), level_element), _brownian_elements.end());

    _level_elements.erase(std::remove_if(_level_elements.begin(), _level_elements.end(), Ptr_contains_predicate<Level_element>(level_element)), _level_elements.end());

    assert(_level_elements.size() == _barriers.size() +
           _brownian_elements.size() +
           _portals.size() +
           _molecule_releasers.size());
}

void Level_data::reset_level_elements()
{
    for (boost::shared_ptr<Level_element> const& e : _level_elements)
    {
        e->reset();
    }
}

void Level_data::use_unstable_options()
{
    _parameters["Damping"]->set_min_max(0.0f, 1.0f);
    _parameters["rotation_damping"]->set_min_max(0.0f, 1.0f);
    _parameters["translation_damping"]->set_min_max(0.0f, 1.0f);
}
