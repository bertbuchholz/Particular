#include "After_finish_screen.h"

#include "My_viewer.h"
#include "Statistics_screen.h"
#include "Before_start_screen.h"
#include "Main_menu_screen.h"
#include "Editor_screen.h"

After_finish_screen::After_finish_screen(My_viewer &viewer, Core &core, Main_game_screen::Ui_state const ui_state) : Menu_screen(viewer, core), _animate_score_time(-1.0f)
{
    _type = Screen::Type::Modal;

    init(ui_state);

    _picking.init(_viewer.context());
}

void After_finish_screen::init(Main_game_screen::Ui_state const ui_state)
{
    // After finish screen
    if (ui_state == Main_game_screen::Ui_state::Editor_playing)
    {
        if (_core.get_progress().last_level < _core.get_level_names().size())
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Play Again",  std::bind(&After_finish_screen::play_again, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.75f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back to Editor", std::bind(&After_finish_screen::return_to_editor, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }
    else
    {
        if (_core.get_progress().last_level < _core.get_level_names().size() - 1)
        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Next Level",  std::bind(&After_finish_screen::load_next_level, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }

        {
            Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.75f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back to Main Menu", std::bind(&After_finish_screen::change_to_main_menu, this));
            _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
        }
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Show Statistics", std::bind(&After_finish_screen::change_to_statistics, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }



    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.8f, 0.3f), "Level finished!");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));

        boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move));
        e->set_duration(1.0f);
        e->trigger();
        e->set_finish_function(std::bind(&After_finish_screen::start_score_animation, this));
        e->make_move_event(Eigen::Vector3f(0.5f, 1.3f, 0.0f), Eigen::Vector3f(0.5f, 0.8f, 0.0f));
        _events.push_back(e);
    }

    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.25f, 0.6f, 0.0f), Eigen::Vector2f(0.3f, 0.1f), "Portal Score");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));

        boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move));
        e->set_duration(1.0f);
        e->trigger();
        e->make_move_event(Eigen::Vector3f(-0.3f, 0.6f, 0.0f), Eigen::Vector3f(0.25f, 0.6f, 0.0f));
        _events.push_back(e);
    }

    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.75f, 0.6f, 0.0f), Eigen::Vector2f(0.3f, 0.1f), "Energy Penalty");
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));

        boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move));
        e->set_duration(1.0f);
        e->trigger();
        e->make_move_event(Eigen::Vector3f(1.3f, 0.6f, 0.0f), Eigen::Vector3f(0.75f, 0.6f, 0.0f));
        _events.push_back(e);
    }

    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.25f, 0.5f, 0.0f), Eigen::Vector2f(0.3f, 0.1f), "0000000");
        label->set_color({147 / 255.0f, 232 / 255.0f, 112 / 255.0f, 1.0f});
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
        _score_label = _labels.back();

        boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move));
        e->set_duration(1.0f);
        e->trigger();
        e->make_move_event(Eigen::Vector3f(0.25f, -0.2f, 0.0f), Eigen::Vector3f(0.25f, 0.5f, 0.0f));
        _events.push_back(e);
    }

    {
        Draggable_label * label = new Draggable_label(Eigen::Vector3f(0.75f, 0.5f, 0.0f), Eigen::Vector2f(0.3f, 0.1f), "0000000");
        label->set_color({255 / 255.0f, 121 / 255.0f, 54 / 255.0f, 1.0f});
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));
        _penalty_label = _labels.back();

        boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move));
        e->set_duration(1.0f);
        e->trigger();
        e->make_move_event(Eigen::Vector3f(0.75f, -0.2f, 0.0f), Eigen::Vector3f(0.75f, 0.5f, 0.0f));
        _events.push_back(e);
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
        button->set_visible(false);
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

    _score.sensor_data = _core.get_sensor_data();
    _score.calculate_score(_core.get_level_data()._score_time_factor, num_molecules_to_capture);

    if (ui_state == Main_game_screen::Ui_state::Playing)
    {
        if (_core.get_progress().last_level < _core.get_level_names().size())
        {
            _core.get_progress().last_level += 1;
        }

        std::vector<Score> & scores = _core.get_progress().scores[_core.get_current_level_name()];

        bool is_new_highscore = false;

        if (!scores.empty())
        {
            auto iter = std::max_element(scores.begin(), scores.end(), Score::score_comparer);

            if (_score.final_score > iter->final_score)
            {
                is_new_highscore = true;
            }
        }
        else
        {
            is_new_highscore = true;
        }

        if (is_new_highscore)
        {
            std::cout << __FUNCTION__ << " new highscore" <<  std::endl;
        }

        scores.push_back(_score);

        _core.save_progress();
    }
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

void After_finish_screen::play_again()
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

void After_finish_screen::return_to_editor()
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

void After_finish_screen::change_to_statistics()
{
    _viewer.add_screen(new Statistics_screen(_viewer, _core, this, _score));

    pause();
}

void After_finish_screen::start_score_animation()
{
    _animate_score_time = _time;
}

void After_finish_screen::add_particle_system()
{
    int const score = _score.final_score - _score._penalty;

    _score_particle_system = Targeted_particle_system(3.0f);
    _score_particle_system.generate(QString("%1").arg(score, 7, 10, QChar('0')).toStdString(), _viewer.get_particle_font(), QRectF(0.0f, 0.5f, 1.0f, 0.3f));

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        button->set_visible(true);
    }
}

void After_finish_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    if (_animate_score_time > 0.0f && _animate_score_time < 100.0f)
    {
        float const duration = 3.0f;

        float const normalized_time = (_time - _animate_score_time) / duration;

        int const score = _score.final_score;
        int interpolated_score = score * normalized_time;

        int const penalty = _score._penalty;
        int interpolated_penalty = penalty * normalized_time;

        if (normalized_time >= 1.0f)
        {
            interpolated_score = score;
            interpolated_penalty = penalty;
            _animate_score_time = 101.0f;
            add_particle_system();
        }

        _score_label->set_text(QString("%1").arg(interpolated_score, 7, 10, QChar('0')).toStdString());
        _renderer.generate_label_texture(_score_label.get());

        _penalty_label->set_text(QString("%1").arg(interpolated_penalty, 7, 10, QChar('0')).toStdString());
        _renderer.generate_label_texture(_penalty_label.get());
    }

    _score_particle_system.animate(time_step);

    for (boost::shared_ptr<Draggable_event> & e : _events)
    {
        e->update(_time, time_step);
    }
}

void After_finish_screen::draw()
{
    if (get_state() == State::Paused) return;

    Menu_screen::draw();

    _renderer.draw_particle_system(_score_particle_system, _viewer.height());
}
