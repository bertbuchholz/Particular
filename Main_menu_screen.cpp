#include "Main_menu_screen.h"

#include "My_viewer.h"
#include "Before_start_screen.h"
#include "Editor_screen.h"

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
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.6f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Start New Game",  std::bind(&Main_menu_screen::start_new_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    if (_core.get_progress().last_level < _core.get_level_names().size() - 1)
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.4f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Continue Game",  std::bind(&Main_menu_screen::continue_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }
    else
    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.5f, 0.4f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "All levels finished, try the sandbox!");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.2f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Quit", std::bind(&Main_menu_screen::quit_game, this));
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

    _game_name_system.generate("PARTICULAR", _viewer.get_particle_font(), QRectF(0.0f, 0.05f, 1.0f, 0.3f));
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
    _core.load_next_level();

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    _core.change_level_state(Main_game_screen::Level_state::Running); // remove intro

    kill();
}


void Main_menu_screen::continue_game()
{
    // load current progress and start game
    _core.load_next_level();

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
    _viewer.camera()->frame()->setConstraint(nullptr);

    _core.load_level_defaults();
    _core.set_simulation_state(false);

    _core.load_simulation_settings();

    _core.set_new_game_state(Core::Game_state::Unstarted);

    _viewer.replace_screens(new Editor_screen(_viewer, _core));
}

void Main_menu_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    _game_name_system.animate(time_step);
}
