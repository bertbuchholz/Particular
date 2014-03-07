#include "Statistics_screen.h"

#include "After_finish_screen.h"
#include "My_viewer.h"

void Statistics_screen::init()
{
    _statistics.resize(_core.get_sensor_data().get_num_data_types());

    {
        boost::shared_ptr<Draggable_statistics> stat(new Draggable_statistics(Eigen::Vector3f(0.25f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Released Molecules"));
        stat->set_duration(_stat_anim_duration);
        _statistics[int(Sensor_data::Type::RelMol)] = stat;
    }

    {
        boost::shared_ptr<Draggable_statistics> stat(new Draggable_statistics(Eigen::Vector3f(0.75f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Collected Molecules"));
        stat->set_duration(_stat_anim_duration);
        _statistics[int(Sensor_data::Type::ColMol)] = stat;
    }

    {
        boost::shared_ptr<Draggable_statistics> stat(new Draggable_statistics(Eigen::Vector3f(0.25f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Avg. Temperature"));
        stat->set_duration(_stat_anim_duration);
        _statistics[int(Sensor_data::Type::AvgTemp)] = stat;
    }

    {
        boost::shared_ptr<Draggable_statistics> stat(new Draggable_statistics(Eigen::Vector3f(0.75f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Energy Consumption"));
        stat->set_duration(_stat_anim_duration);
        _statistics[int(Sensor_data::Type::EnergyCon)] = stat;
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.50f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Back",  std::bind(&Statistics_screen::exit, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.25f, 0.1f, 0.0f), Eigen::Vector2f(0.25f, 0.1f), "Repeat",  std::bind(&Statistics_screen::repeat, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    _collected_score_label = boost::shared_ptr<Draggable_label>(new Draggable_label({ 0.25f, 0.6f + 0.35f * 0.5f, 0.0f }, { 0.3f, 0.2f }, "ColScore"));
    _collected_score_label->set_alpha(0.0f);
//    _renderer.generate_label_texture(_collected_score_label.get());
    _labels.push_back(_collected_score_label);

    _power_score_label = boost::shared_ptr<Draggable_label>(new Draggable_label({ 0.75f, 0.6f + 0.35f * 0.5f, 0.0f }, { 0.3f, 0.2f }, "PowerScore"));
    _power_score_label->set_alpha(0.0f);
//    _renderer.generate_label_texture(_power_score_label.get());
    _labels.push_back(_power_score_label);

    boost::shared_ptr<Draggable_event> collected_label_event(new Draggable_event(_collected_score_label, Draggable_event::Type::Fade_in));
    _events.push_back(collected_label_event);

    boost::shared_ptr<Draggable_event> power_label_event(new Draggable_event(_power_score_label, Draggable_event::Type::Fade_in));
    _events.push_back(power_label_event);

    {
        boost::shared_ptr<Draggable_event> e(new Draggable_event(_statistics[int(Sensor_data::Type::RelMol)], Draggable_event::Type::Animate));
        e->set_duration(_stat_anim_duration);
        e->trigger();
        _events.push_back(e);
    }

    {
        boost::shared_ptr<Draggable_event> e(new Draggable_event(_statistics[int(Sensor_data::Type::ColMol)], Draggable_event::Type::Animate));
        e->set_duration(_stat_anim_duration);
        e->trigger();
        e->set_finish_function(std::bind(&Draggable_event::trigger, collected_label_event.get()));
        _events.push_back(e);
    }

    {
        boost::shared_ptr<Draggable_event> e(new Draggable_event(_statistics[int(Sensor_data::Type::AvgTemp)], Draggable_event::Type::Animate));
        e->set_duration(_stat_anim_duration);
        e->trigger();
        _events.push_back(e);
    }

    {
        boost::shared_ptr<Draggable_event> e(new Draggable_event(_statistics[int(Sensor_data::Type::EnergyCon)], Draggable_event::Type::Animate));
        e->set_duration(_stat_anim_duration);
        e->trigger();
        e->set_finish_function(std::bind(&Draggable_event::trigger, power_label_event.get()));
        _events.push_back(e);
    }

    setup_statistics(_core.get_sensor_data(), _core.get_progress().scores[_core.get_current_level_name()].back()); // this MUST be before the generation of the textures!

    for (boost::shared_ptr<Draggable_statistics> const& stat : _statistics)
    {
        _renderer.generate_statistics_texture(*stat);
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _renderer.generate_label_texture(label.get());
    }
}

void Statistics_screen::draw()
{
    _viewer.start_normalized_screen_coordinates();

    for (boost::shared_ptr<Draggable_statistics> const& stat : _statistics)
    {
        _viewer.draw_statistic(*stat);
    }

    _viewer.stop_normalized_screen_coordinates();

    Menu_screen::draw();
}


void Statistics_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    for (boost::shared_ptr<Draggable_event> & e : _events)
    {
        e->update(_time, time_step);
    }
}

void Statistics_screen::setup_statistics(Sensor_data const& sensor_data, Score const& score)
{
    for (int i = 0; i < sensor_data.get_num_data_types(); ++i)
    {
        _statistics[i]->set_values(sensor_data.get_data(Sensor_data::Type(i)));
    }

    Draggable_statistics * stat = _statistics[int(Sensor_data::Type::ColMol)].get();

    int score_index = 0;

    for (std::pair<float, int> const& score_time : score.get_score_at_time())
    {
        ++score_index;

        float const time = score_time.first;
        float const normalized_time = time / score.get_full_time();

        float const stretched_curve_time = stat->get_particle_system().get_curve().get_absolute_length_at_uniform_length(normalized_time) /
                stat->get_particle_system().get_curve().get_absolute_length_at_uniform_length_y_over_x(normalized_time);

        assert(stretched_curve_time >= 1.0f);

        float const event_time = _stat_anim_duration * stat->get_particle_system().get_curve().get_absolute_length_at_uniform_length(normalized_time) / stat->get_particle_system().get_curve().get_length();

//        float const event_time = normalized_time * _stat_anim_duration * stretched_curve_time;

        Eigen::Vector3f bottom_left_pos = stat->get_position();
        bottom_left_pos -= Eigen::Vector3f(stat->get_extent()[0] * 0.5f, stat->get_extent()[1] * 0.5f, 0.0f);
        bottom_left_pos[0] += 0.1f * stat->get_extent()[0];
        bottom_left_pos[1] += 0.15f * stat->get_extent()[1];

        Eigen::Vector3f top_right_pos = bottom_left_pos;
        top_right_pos[0] += 0.8f * stat->get_extent()[0];
        top_right_pos[1] += 0.75f * stat->get_extent()[1];



        Eigen::Vector3f const curve_pos = stat->get_particle_system().get_curve().get_pos_on_curve_y_over_x(normalized_time);

        Eigen::Vector3f const event_pos(curve_pos[0] * (top_right_pos[0] - bottom_left_pos[0]) + bottom_left_pos[0],
                curve_pos[1] * (top_right_pos[1] - bottom_left_pos[1]) + bottom_left_pos[1], 0.0f);


        Draggable_label * label = new Draggable_label(event_pos, Eigen::Vector2f(0.1f, 0.05f), QString("%1").arg(score_time.second).toStdString());
        label->set_color({147 / 255.0f, 232 / 255.0f, 112 / 255.0f, 1.0f});
        label->set_alpha(0.0f);
        _labels.push_back(boost::shared_ptr<Draggable_label>(label));

        {
            boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Fade_in, event_time));
            e->set_duration(0.1f);
            e->trigger();
            _events.push_back(e);
        }

        {
            boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Move, event_time, QEasingCurve::OutCubic));
            e->set_duration(1.0f);
            e->trigger();
            e->make_move_event(event_pos, event_pos + Eigen::Vector3f(0.0f, 0.05f, 0.0f));
            _events.push_back(e);
        }

        {
            boost::shared_ptr<Draggable_event> e(new Draggable_event(_labels.back(), Draggable_event::Type::Fade_out, event_time + 1.0f));
            e->set_duration(1.0f);
            e->trigger();
            _events.push_back(e);
        }
    }
}

void Statistics_screen::exit()
{
    _calling_screen->resume();

    kill();
}

void Statistics_screen::repeat()
{
    for (boost::shared_ptr<Draggable_statistics> & stat : _statistics)
    {
        stat->reset_animation();
    }

    for (boost::shared_ptr<Draggable_event> & e : _events)
    {
        e->reset();
    }

    _time = 0.0f;
}

Statistics_screen::Statistics_screen(My_viewer & viewer, Core & core, Screen *calling_screen) :
    Menu_screen(viewer, core), _calling_screen(calling_screen), _stat_anim_duration(10.0f)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}
