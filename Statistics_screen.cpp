#include "Statistics_screen.h"

#include "After_finish_screen.h"
#include "My_viewer.h"

void Statistics_screen::init()
{
    _statistics.resize(_core.get_sensor_data().get_num_data_types());

    {
        Draggable_statistics stat(Eigen::Vector3f(0.25f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Released Molecules");
        _statistics[int(Sensor_data::Type::RelMol)] = stat;
    }

    {
        Draggable_statistics stat(Eigen::Vector3f(0.75f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Collected Molecules");
        _statistics[int(Sensor_data::Type::ColMol)] = stat;
    }

    {
        Draggable_statistics stat(Eigen::Vector3f(0.25f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Avg. Temperature");
        _statistics[int(Sensor_data::Type::AvgTemp)] = stat;
    }

    {
        Draggable_statistics stat(Eigen::Vector3f(0.75f, 0.2f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Energy Consumption");
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

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _viewer.generate_button_texture(button.get());
    }

//    for (boost::shared_ptr<Draggable_label> const& label : _labels)
//    {
//        _viewer.generate_label_texture(label.get());
//    }

    for (Draggable_statistics & stat : _statistics)
    {
        _viewer.generate_statistics_texture(stat);
    }

    setup_statistics(_core.get_sensor_data());
}

void Statistics_screen::draw()
{
    float alpha = 1.0f;

    if (get_state() == State::Killing || get_state() == State::Resuming)
    {
        if (get_state() == State::Killing)
        {
            alpha = 1.0f - _transition_progress;
        }
        else if (get_state() == State::Resuming)
        {
            alpha = _transition_progress;
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    _viewer.start_normalized_screen_coordinates();

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
//            if (button->is_visible())
        {
            _viewer.draw_button(button.get(), false, alpha);
        }
    }

    for (Draggable_statistics const& stat : _statistics)
    {
            _viewer.draw_statistic(stat);
    }

    _viewer.stop_normalized_screen_coordinates();
}

void Statistics_screen::draw_draggables_for_picking()
{
    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        _picking.set_index(i);
        _viewer.draw_button(_buttons[i].get(), true);
    }

    _viewer.stop_normalized_screen_coordinates();
}

void Statistics_screen::update_event(const float time_step)
{
    for (Draggable_statistics & stat : _statistics)
    {
        stat.animate(time_step);
    }
}

void Statistics_screen::setup_statistics(const Sensor_data &sensor_data)
{
    for (int i = 0; i < sensor_data.get_num_data_types(); ++i)
    {
        _statistics[i].set_values(sensor_data.get_data(Sensor_data::Type(i)));
    }
}

void Statistics_screen::exit()
{
//    _viewer.add_screen(new After_finish_screen(_viewer, _core));
    _calling_screen->resume();

    kill();
}

void Statistics_screen::repeat()
{
    for (Draggable_statistics & stat : _statistics)
    {
            stat.reset_animation();
    }
}


Statistics_screen::Statistics_screen(My_viewer & viewer, Core & core, Screen *calling_screen) : Screen(viewer), _core(core), _calling_screen(calling_screen)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

bool Statistics_screen::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int picked_index = _picking.do_pick(
                    event->pos().x() / float(_viewer.camera()->screenWidth()),
                    (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                    std::bind(&Statistics_screen::draw_draggables_for_picking, this));

        if (picked_index > -1)
        {
            _buttons[picked_index]->clicked();
        }
    }

    return true;
}
