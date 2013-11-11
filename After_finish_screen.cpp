#include "After_finish_screen.h"

#include "My_viewer.h"
#include "Statistics_screen.h"
#include "Before_start_screen.h"

After_finish_screen::After_finish_screen(My_viewer &viewer, Core &core) : Menu_screen(viewer, core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void After_finish_screen::init()
{
    // After finish screen
    if (_core.get_progress().last_level < _core.get_level_names().size())
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Next Level",  std::bind(&After_finish_screen::load_next_level, this));
//        _next_level_button = boost::shared_ptr<Draggable_button>(button);
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Show Statistics", std::bind(&After_finish_screen::change_to_statistics, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.75f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back to Main Menu", std::bind(&After_finish_screen::change_to_main_menu, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.8f, 0.3f), "Level finished!");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _renderer.generate_label_texture(label.get());
    }

    int const score_count = _core.get_sensor_data().calculate_score(_core.get_level_data()._score_time_factor);

    Score score;
    score.final_score = score_count;
    score.sensor_data = _core.get_sensor_data();

    if (_core.get_progress().last_level < _core.get_level_names().size())
    {
        _core.get_progress().last_level += 1;
    }

    _core.get_progress().scores[_core.get_current_level_name()].push_back(score);

//    _next_level_button->set_visible(_core.get_progress().last_level < _core.get_level_names().size());

    _core.save_progress();

    _score_particle_system = Targeted_particle_system(3.0f);
    _score_particle_system.generate(QString("%1").arg(score_count, 8, 10, QChar('0')).toStdString(), _viewer.get_particle_font(), QRectF(0.0f, 0.5f, 1.0f, 0.3f));
}


void After_finish_screen::load_next_level()
{
    _core.load_next_level();

    for (Targeted_particle & p : _score_particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    kill();
}

void After_finish_screen::change_to_main_menu()
{
    for (Targeted_particle & p : _score_particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _viewer.add_screen(new Main_menu_screen(_viewer, _core));

    kill();
}

void After_finish_screen::update_event(const float time_step)
{
    _score_particle_system.animate(time_step);
}

void After_finish_screen::change_to_statistics()
{
//    for (Targeted_particle & p : _score_particle_system.get_particles())
//    {
//        p.target = Eigen::Vector3f::Random().normalized();
//        p.target *= 1.5f;
//    }

    _viewer.add_screen(new Statistics_screen(_viewer, _core, this));

//    kill();
    pause();
}

void After_finish_screen::draw()
{
    if (get_state() == State::Paused) return;

    Menu_screen::draw();

    _renderer.draw_particle_system(_score_particle_system, _viewer.height());
}
