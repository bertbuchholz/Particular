#include "Experiment_screen.h"

#include "My_viewer.h"

Experiment_screen::Experiment_screen(My_viewer &viewer, Core &core) : Menu_screen(viewer, core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void Experiment_screen::draw()
{
    Menu_screen::draw();

    _viewer.start_normalized_screen_coordinates();
    _viewer.draw_statistic(_statistic);
    _viewer.stop_normalized_screen_coordinates();

    _renderer.setup_gl_points(false);

    _renderer.draw_curved_particle_system(_curved_system, _viewer.height());

}

void Experiment_screen::init()
{
    boost::shared_ptr<Draggable_label> adv_options_label(
                new Draggable_label(Eigen::Vector3f(0.9f, 0.71f, 0.0f),
                                     Eigen::Vector2f(0.15f, 0.07f),
                                     "EXPERIMENT"));
    _renderer.generate_label_texture(adv_options_label.get());

    _labels.push_back(adv_options_label);

    _statistic = Draggable_statistics(Eigen::Vector3f(0.25f, 0.6f + 0.35f * 0.5f, 0.0f), Eigen::Vector2f(0.45f, 0.35f), "Released Molecules");

    std::vector<float> stat_values;

    stat_values.push_back(-10.0f);

    for (int i = 0; i < 10; ++i)
    {
        stat_values.push_back(rand() / float(RAND_MAX) * 5.0f - 7.0f);
    }

    stat_values.push_back(10.0f);

    _statistic.set_values(stat_values, -10, 10);
    _renderer.generate_statistics_texture(_statistic);

    std::vector<Eigen::Vector3f> points;

//    for (int i = 0; i < 100; ++i)
//    {
//        Eigen::Vector3f p(std::cos(2.0f * float(M_PI) * i / 100.0f),
//                          std::sin(2.0f * float(M_PI) * i / 100.0f),
//                          0.0f);

//        p *= 0.5f;
//        p += Eigen::Vector3f(0.5f, 0.5f, 0.0f);

//        points.push_back(p);
//    }

    points.push_back(Eigen::Vector3f(0.0f, 0.0f, 0.0f));
    points.push_back(Eigen::Vector3f(0.1f, 0.9f, 0.0f));
    points.push_back(Eigen::Vector3f(0.9f, 0.9f, 0.0f));

    _curved_system = Curved_particle_system(points, 10.0f);
    _curved_system.set_effect_spread(1.0f);
}

void Experiment_screen::update_event(const float time_step)
{
    Menu_screen::update_event(time_step);

    _curved_system.animate(time_step);

    _statistic.animate(time_step);
}
