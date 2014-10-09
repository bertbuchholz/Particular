#include "Before_start_screen.h"

#include "My_viewer.h"

#include "Main_menu_screen.h"


Before_start_screen::Before_start_screen(My_viewer &viewer, Core &core) : Menu_screen(viewer, core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void Before_start_screen::draw()
{
    Menu_screen::draw();

    _renderer.draw_particle_system(_particle_system, _viewer.height());
}


void Before_start_screen::init()
{
    // start level screen
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.35f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Start Level",  std::bind(&Before_start_screen::start_level, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.15f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Back to Main Menu",  std::bind(&Before_start_screen::return_to_main_menu, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    _particle_system = Targeted_particle_system(3.0f);
    _particle_system.generate(QString("LEVEL %1").arg(_core.get_progress().last_level + 1).toStdString(), _viewer.get_particle_font(), QRectF(0.0f, 0.1f, 1.0f, 0.3f), _renderer.get_aspect_ratio());
}

void Before_start_screen::return_to_main_menu()
{
    for (Targeted_particle & p : _particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _viewer.add_screen(new Main_menu_screen(_viewer, _core));

    kill();
}

void Before_start_screen::start_level()
{
    _core.start_level();

    for (Targeted_particle & p : _particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    kill();
}

void Before_start_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    _particle_system.animate(time_step);
}
