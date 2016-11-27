#include "Particle_system.h"

#include <QtOpenGL>

#include <cmath>

void Targeted_particle_system::generate(const std::string &text, const QFont &main_font, const QRectF &rect, float const aspect_ratio)
{

    int const scale = 500;

    QRect scaled_rect(rect.left() * scale,
                      rect.top() * scale,
                      rect.width() * scale,
                      rect.height() * scale);

    //        QImage img(text.size() * 30, text.size() * 30, QImage::Format_RGB32);
    QImage image(scale, scale, QImage::Format_RGB32);
    image.fill(QColor(0, 0, 0));

    QPainter p(&image);

    QFont font = main_font;
    //        font.setWeight(QFont::Bold);
    font.setPointSizeF(10.0f);
//    font.setLetterSpacing(QFont::PercentageSpacing, 115.0f);
    p.setFont(font);

    float const x_factor = scaled_rect.width() / float(p.fontMetrics().width(QString::fromStdString(text)));
    float const y_factor = scaled_rect.height() / float(p.fontMetrics().height());
    font.setPointSizeF(9.5f * std::min(x_factor, y_factor));
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));
    p.setBrush(Qt::NoBrush);

    p.drawText(scaled_rect, Qt::AlignCenter, QString::fromStdString(text));

//    p.drawRect(0, 0, scale - 1, scale - 1);
//    p.drawRect(scaled_rect);

    p.end();

//    img.save("/Users/bert/Desktop/text.png");

//    img = img.scaled(img.width(), img.height());

    int const neighborhood_size = 1;

    QImage tmp_image = image;

    for (int x = 0; x < image.width(); ++x)
    {
        for (int y = 0; y < image.height(); ++y)
        {
            int neighborhood_average = 0;

            for (int u = -neighborhood_size; u <= neighborhood_size; ++u)
            {
                for (int v = -neighborhood_size; v <= neighborhood_size; ++v)
                {
                    neighborhood_average += qRed(image.pixel(into_range(x + u, 0, image.width() - 1), into_range(y + v, 0, image.height() - 1)));
                }
            }

            neighborhood_average /= (2 * neighborhood_size + 1) * (2 * neighborhood_size + 1);

            tmp_image.setPixel(x, y, QColor(std::abs(neighborhood_average - qRed(image.pixel(x, y))), 0, 0).rgb());
        }
    }

    generate(tmp_image, QVector2D(1.0f / aspect_ratio, 1.0f));
}


void Targeted_particle_system::generate(const QImage &image, QVector2D const& scale)
{
    _particles.reserve(10000);

    for (int x = 0; x < image.width(); ++x)
    {
        for (int y = 0; y < image.height(); ++y)
        {
            if (qRed(image.pixel(x, y)) > 10)
            {
                Targeted_particle p;
                p.age = 0.0f;
                p.size_factor = std::rand() / float(RAND_MAX) * 2.5f + 0.5f;
                float const alpha = std::rand() / float(RAND_MAX) * 0.7f + 0.3f;
                //                    p.color = Color4(1.0f, 0.33f, 0.05f, alpha);
                float const green = std::rand() / float(RAND_MAX) * 0.26f + 0.2f;
                p.color = Color4(1.0f, green, 0.05f, alpha);

//                p.position = Eigen::Vector3f::Random();

                float const s = std::rand() / float(RAND_MAX) * 2.0f * M_PI;
                p.position = { std::cos(s) * 1.5f,
                               std::sin(s) * 1.5f,
                               0.0f };

//                p.position = { 1.0f, 1.0f, 0.0f };

                p.position[2] = 0.0f;
                p.target = Eigen::Vector3f(scale.x() *  (x / float(image.width())  * 2.0f - 1.0f),
                                           scale.y() * -(y / float(image.height()) * 2.0f - 1.0f),
                                             0.0f);

                p.initial_speed = (0.05f + 2.5f) * (p.target - p.position) / _total_duration; // 2.5f is the 1/integral(wendland_2_1), add 0.05 to overshoot a bit

                p.speed[2] = 0.0f;

                _particles.push_back(p);
            }
        }
    }

    std::cout << __FUNCTION__ << " created " << _particles.size() << " particles" << std::endl;
}

void Targeted_particle_system::init(const std::vector<Targeted_particle> &particles)
{
    _particles = particles;

    for (Targeted_particle & p : _particles)
    {
        p.initial_speed = (0.05f + 2.5f) * (p.target - p.position) / _total_duration; // 2.5f is the 1/integral(wendland_2_1)
    }
}

void Targeted_particle_system::animate(const float timestep)
{
    _age += timestep;

    for (Targeted_particle & p : _particles)
    {
        if (p.age < 1.0f)
        {
            p.age += timestep / _total_duration;
            p.speed = p.initial_speed * wendland_2_1(p.age);
            p.position += p.speed * timestep;
        }
        else
        {
            Eigen::Vector3f const to_target = p.target - p.position;

            float const distance = to_target.norm();

            if (distance > 0.005f)
            {
                p.speed += timestep * to_target * std::max(0.01f, distance * distance * 5.0f) * rand() / (RAND_MAX * distance);
            }
            else
            {
                p.speed += timestep * Eigen::Vector3f::Random() * 0.01f;
            }

            p.position += p.speed * timestep;
        }
    }
}


Curved_particle_system::Curved_particle_system() : _total_time(1.0f), _current_time(0.0f), _infinite_loop(false)
{ }


Curved_particle_system::Curved_particle_system(std::vector<Eigen::Vector3f> const& curve_points, float const total_time) :
    _total_time(total_time),
    _current_time(0.0f),
    _effect_spread(1.0f),
    _particle_size(1.0f),
    _tail_dissipation_speed(0.5f),
    _curve_color(1.0f, 0.0f, 0.0f, 1.0f),
    _effect_color(0.0f, 1.0f, 0.0f, 1.0f),
    _display_ratio(1.0f),
    _use_as_diagram(false),
    _infinite_loop(false)
{
    std::random_device rd;
    _rng = std::mt19937(rd());

    for (auto const& p : curve_points)
    {
        _curve.add_vertex_at_back(p);
    }

    _curve.finish();
}

Color4 color_aging(Color4 const& c, float const age)
{
    return Color4(c.rgb() + Color(1.0f) * (1.0f - age), c.a);
}

void Curved_particle_system::animate(float const timestep)
{
    // animate current particles, then generate new ones
    for (Particle & p : _curve_particles)
    {
        p.age = std::min(1.0f, p.age + timestep);
        p.speed += Eigen::Vector3f::Random().normalized() * 0.0001f;
        p.speed[2] = 0.0f;
        p.current_color = color_aging(p.color, p.age);
//        p.speed += Eigen::Vector3f::Zero();
        p.position += p.speed * timestep;
    }

    for (Particle & p : _effect_particles)
    {
//        p.age = std::min(1.001f, p.age + timestep);
        p.age = p.age + timestep;
        p.position += p.speed * timestep;
        p.color.a = 1.0f - (p.age * _tail_dissipation_speed);
        p.current_color = color_aging(p.color, p.age);
        p.speed += Eigen::Vector3f(0.0f, -0.001f * timestep, 0.0f);
    }

//    float const l_x = _curve.get_length_y_over_x();
//    float const l   = _curve.get_length();

//    int const num_points = 100 * l / l_x;

    if (_current_time <= _total_time)
    {
        float const normalized_current_position = _current_time / _total_time;

        float t = normalized_current_position;

        _current_time += timestep;

        float const normalized_next_position = std::min(_current_time / _total_time, 1.0f);

        Eigen::Vector3f start_pos, end_pos;

        if (_use_as_diagram)
        {
            start_pos = _curve.get_pos_on_curve_y_over_x(normalized_current_position);
            end_pos   = _curve.get_pos_on_curve_y_over_x(normalized_next_position);
        }
        else
        {
            start_pos = _curve.get_pos_on_curve(normalized_current_position);
            end_pos   = _curve.get_pos_on_curve(normalized_next_position);
        }

        float const step_length = (end_pos - start_pos).norm();

//        Eigen::Vector3f const step_vector = end_pos - start_pos;
//        float const step_ratio = std::abs(step_vector[1] / step_vector[0]);

//        float const travel_dist_factor = 1.0f + std::pow(std::abs((end_pos - start_pos).normalized()[1]), 5.0f) * _display_ratio;
//        float const travel_dist_factor = 1.0f + std::cos(0.5f * M_PI * std::abs((end_pos - start_pos).normalized()[0])) * _display_ratio;

//        float const normalized_travel_distance = 0.1f * _display_ratio * step_length / _curve.get_length();
//        float const normalized_travel_distance = 0.01f * travel_dist_factor * _curve.get_length();

//        if (normalized_travel_distance > (normalized_next_position - normalized_current_position)) return;

        float const normalized_travel_distance = 0.03f * (normalized_next_position - normalized_current_position) / step_length;
//        float const normalized_travel_distance = std::max(0.01f, step_ratio / _display_ratio * 0.01f *
//                (_curve.get_absolute_length_at_uniform_length(normalized_next_position) - _curve.get_absolute_length_at_uniform_length(normalized_current_position)) /
//                _curve.get_length());

        while (t < normalized_next_position)
        {
            Eigen::Vector3f const tangent = _curve.compute_tangent(t);
            Eigen::Vector3f const orthogonal(tangent[1], -tangent[0], 0.0f);

            Eigen::Vector3f new_particle_position;

            if (_use_as_diagram)
            {
                new_particle_position = _curve.get_pos_on_curve_y_over_x(t);
            }
            else
            {
                float const alpha = _rng() / float(_rng.max());
                new_particle_position = alpha * _curve.get_pos_on_curve(t) + (1.0f - alpha) * _curve.get_pos_on_curve(std::min(1.0f, t + normalized_travel_distance));
            }

            Particle p;
            p.position = new_particle_position + orthogonal * (_rng() / float(_rng.max()) - 0.5f) * 0.01f;
            p.color = _curve_color;
            p.color.a = _rng() / float(_rng.max()) * 0.6f + 0.4f;
            p.current_color = color_aging(p.color, p.age);
            p.size_factor = _rng() / float(_rng.max()) * 2.0f + 0.3f;
//            p.size_factor = 1.0f;
            p.size_factor *= _particle_size;

            _curve_particles.push_back(p);

            t += normalized_travel_distance;

            int const spawned_particles = 2;

            for (int i = 0; i < spawned_particles; ++i)
            {
                Particle p;
                p.position = new_particle_position;
                p.color = _effect_color;
                p.color.a = _rng() / float(_rng.max()) * 0.6f + 0.4f;
                p.current_color = color_aging(p.color, p.age);
                p.size_factor = _rng() / float(_rng.max()) * 2.0f + 0.1f;
                p.size_factor *= _particle_size;

                Eigen::Vector3f speed = tangent * 0.1f * _tangent_speed_factor;
                speed += Eigen::Vector3f::Random() * 0.05f * _effect_spread;
                p.speed = speed;

                _effect_particles.push_back(p);
            }
        }
    }
    else if (_infinite_loop)
    {
        _current_time = 0.0f;
    }
}

void Curved_particle_system::set_tail_dissipation_speed(float const tail_dissipation_speed)
{
    _tail_dissipation_speed = tail_dissipation_speed;
}


void Curved_particle_system::set_tangent_speed_factor(float const tangent_speed_factor)
{
    _tangent_speed_factor = tangent_speed_factor;
}

Color4 const& Curved_particle_system::get_curve_color() const
{
    return _curve_color;
}

void Curved_particle_system::set_curve_color(Color4 const& curve_color)
{
    _curve_color = curve_color;
}

Color4 const& Curved_particle_system::get_effect_color() const
{
    return _effect_color;
}

void Curved_particle_system::set_effect_color(Color4 const& effect_color)
{
    _effect_color = effect_color;
}

void Curved_particle_system::reset()
{
    _current_time = 0.0f;

    _curve_particles.clear();
    _effect_particles.clear();
}




