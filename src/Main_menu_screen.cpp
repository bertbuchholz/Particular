#include "Main_menu_screen.h"

#include "My_viewer.h"
#include "Before_start_screen.h"
#include "Editor_screen.h"
#include "level_picker_screen.h"

Main_menu_screen::Main_menu_screen(My_viewer &viewer, Core &core) : Menu_screen(viewer, core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void Main_menu_screen::draw()
{
    Menu_screen::draw();

    _renderer.setup_gl_points(false);

    _renderer.draw_particle_system(_game_name_system, _viewer.height());
}

void Main_menu_screen::init()
{
    // main menu
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.65f, 0.0f), Eigen::Vector2f(0.5f, 0.12f), "Start New Game",  std::bind(&Main_menu_screen::start_new_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    if (_core.get_progress().last_level < _core.get_level_names().size())
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.5f, 0.0f), Eigen::Vector2f(0.5f, 0.12f), "Continue Game",  std::bind(&Main_menu_screen::continue_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }
    else
    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.5f, 0.5f, 0.0f), Eigen::Vector2f(0.5f, 0.12f), "All levels finished, try the sandbox!");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.35f, 0.0f), Eigen::Vector2f(0.5f, 0.12f), "Pick Level", std::bind(&Main_menu_screen::pick_level, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.2f, 0.0f), Eigen::Vector2f(0.5f, 0.12f), "Quit", std::bind(&Main_menu_screen::quit_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.85f, 0.1f, 0.0f), Eigen::Vector2f(0.2f, 0.06f), "Sandbox", std::bind(&Main_menu_screen::start_editor, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _renderer.generate_label_texture(label.get());
    }

    _game_name_system.generate("PARTICULAR", _viewer.get_particle_font(), QRectF(0.0f, 0.1f, 1.0f, 0.2f), _renderer.get_aspect_ratio());
}


void Main_menu_screen::start_new_game()
{
    // set level to 0 (including introduction part) and start game

    _core.get_progress().reset();

    _core.clear();

    _core.load_default_simulation_settings();

    for (Targeted_particle & p : _game_name_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

//    _core.change_level_state(Main_game_screen::Level_state::Intro);
    _core.load_level(0);

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    _core.change_level_state(Main_game_screen::Level_state::Running); // remove intro

    kill();
}


void Main_menu_screen::continue_game()
{
    // load current progress and start game
//    _core.set_current_level_index(_core.get_progress().last_level);

    _core.load_level(_core.get_progress().last_level);

    _core.load_default_simulation_settings();

    for (Targeted_particle & p : _game_name_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    kill();
}


void Main_menu_screen::quit_game()
{
    _core.save_progress();

    _viewer.quit_game();
}

void Main_menu_screen::start_editor()
{
    if (_core.get_progress().last_level < _core.get_level_names().size() && !_core.get_progress().sandbox_warning_seen)
    {
        QMessageBox::StandardButton button = QMessageBox::warning(&_viewer, "Starting Sandbox", "You haven't finished the game levels yet. Using the sandbox might need knowledge gained during those levels.\nStart the sandbox anyway?", QMessageBox::Yes | QMessageBox::No);

        if (button == QMessageBox::Yes)
        {
            _core.get_progress().sandbox_warning_seen = true;
            _core.save_progress();
        }
    }

    if (_core.get_progress().last_level == _core.get_level_names().size() || _core.get_progress().sandbox_warning_seen)
    {
        _viewer.replace_screens(new Editor_screen(_viewer, _core));

        _core.load_level_defaults();
        _core.set_simulation_state(false);

        _core.load_simulation_settings();

        _core.set_new_game_state(Core::Game_state::Unstarted);

    }
}

void Main_menu_screen::pick_level()
{
    _viewer.add_screen(new Level_picker_screen(_viewer, _core, this));

    kill();
}

void Main_menu_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    _game_name_system.animate(time_step);
}
