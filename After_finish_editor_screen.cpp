#include "After_finish_editor_screen.h"

#include "My_viewer.h"
#include "Statistics_screen.h"
#include "Editor_screen.h"

After_finish_editor_screen::After_finish_editor_screen(My_viewer &viewer, Core &core) : Menu_screen(viewer, core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void After_finish_editor_screen::init()
{
    // After finish screen
    if (_core.get_progress().last_level < _core.get_level_names().size())
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Play Again",  std::bind(&After_finish_editor_screen::play_again, this));
//        _next_level_button = boost::shared_ptr<Draggable_button>(button);
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Show Statistics", std::bind(&After_finish_editor_screen::change_to_statistics, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.75f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back to Editor", std::bind(&After_finish_editor_screen::return_to_editor, this));
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

    int num_molecules_to_capture = 0;

    for (Portal * p : _core.get_level_data()._portals)
    {
        num_molecules_to_capture += p->get_condition().get_min_captured_molecules();
    }

    Score score;
    score.sensor_data = _core.get_sensor_data();
    score.calculate_score(_core.get_level_data()._score_time_factor, num_molecules_to_capture);

    _score_particle_system = Targeted_particle_system(3.0f);
    _score_particle_system.generate(QString("%1").arg(score.final_score, 8, 10, QChar('0')).toStdString(), _viewer.get_particle_font(), QRectF(0.0f, 0.5f, 1.0f, 0.3f));
}


void After_finish_editor_screen::play_again()
{
    for (Targeted_particle & p : _score_particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _core.reset_level();

    _core.start_level();

    kill();
}

void After_finish_editor_screen::return_to_editor()
{
    for (Targeted_particle & p : _score_particle_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _core.reset_level();

    _viewer.camera()->frame()->setConstraint(nullptr);

    _core.set_simulation_state(false);

    _viewer.replace_screens(new Editor_screen(_viewer, _core));

    kill();
}

void After_finish_editor_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    _score_particle_system.animate(time_step);
}

void After_finish_editor_screen::change_to_statistics()
{
    _viewer.add_screen(new Statistics_screen(_viewer, _core, this));

    pause();
}

void After_finish_editor_screen::draw()
{
    if (get_state() == State::Paused) return;

    Menu_screen::draw();

    _renderer.draw_particle_system(_score_particle_system, _viewer.height());
}
