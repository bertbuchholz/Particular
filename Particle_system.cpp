#include "Particle_system.h"

#include <QtOpenGL>

void Targeted_particle_system::generate(const std::string &text, const QFont &main_font, const QRectF &rect)
{

    int const scale = 500;

    QRect scaled_rect(rect.left() * scale,
                      rect.top() * scale,
                      rect.width() * scale,
                      rect.height() * scale);

    //        QImage img(text.size() * 30, text.size() * 30, QImage::Format_RGB32);
    QImage img(scale, scale, QImage::Format_RGB32);
    img.fill(QColor(0, 0, 0));

    QPainter p(&img);

    QFont font = main_font;
    //        font.setWeight(QFont::Bold);
    font.setPixelSize(70);
    font.setLetterSpacing(QFont::PercentageSpacing, 115.0f);
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));
    p.setBrush(Qt::NoBrush);

    //        p.drawText(QRect(15, 15, img.width() - 30, img.height() - 30), Qt::AlignCenter, QString::fromStdString(text));

    //        QSize b_size = p.boundingRect(QRect(0, 0, 500, 500), Qt::AlignLeft | Qt::AlignTop, QString::fromStdString(text)).size();


    p.drawText(scaled_rect, Qt::AlignCenter, QString::fromStdString(text));

//    img.save("/Users/bert/Desktop/text.png");

    generate(img);
}


void Targeted_particle_system::generate(const QImage &image)
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
                p.position = Eigen::Vector3f::Random();
                p.position[2] = 0.0f;
                p.target = Eigen::Vector3f(  x / float(image.width())  * 2.0f - 1.0f,
                                             -(y / float(image.height()) * 2.0f - 1.0f),
                                             0.0f);

                p.initial_speed = (0.05f + 2.5f) * (p.target - p.position) / _total_duration; // 2.5f is the 1/integral(wendland_2_1)

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


Curved_particle_system::Curved_particle_system() : _total_time(1.0f), _current_time(0.0f)
{ }


Curved_particle_system::Curved_particle_system(std::vector<Eigen::Vector3f> const& curve_points, float const total_time) :
    _total_time(total_time),
    _current_time(0.0f),
    _effect_spread(1.0f),
    _particle_size(1.0f),
    _tail_dissipation_speed(0.5f),
    _curve_color(1.0f, 0.0f, 0.0f, 1.0f),
    _effect_color(0.0f, 1.0f, 0.0f, 1.0f)
{
    std::random_device rd;
    _rng = std::mt19937(rd());

    for (auto const& p : curve_points)
    {
        _curve.add_vertex_at_back(p);
    }

    _curve.finish();
}

void Curved_particle_system::animate(float const timestep)
{
    // animate current particles, then generate new ones
    for (Particle & p : _curve_particles)
    {
        p.age = std::min(1.0f, p.age + timestep);
        p.speed += Eigen::Vector3f::Random() * 0.0001f;
//        p.position += p.speed * timestep;
    }

    for (Particle & p : _effect_particles)
    {
//        p.age = std::min(1.001f, p.age + timestep);
        p.age = p.age + timestep;
        p.position += p.speed * timestep;
        p.color.a = 1.0f - (p.age * _tail_dissipation_speed);
        p.speed += Eigen::Vector3f(0.0f, -0.001f * timestep, 0.0f);
    }

    if (_current_time <= _total_time)
    {
        float const normalized_current_position = _current_time / _total_time;

        float t = normalized_current_position;

        _current_time += timestep;

        float const normalized_next_position = std::min(_current_time / _total_time, 1.0f);

        float const normalized_travel_distance = 0.5f * (normalized_next_position - normalized_current_position);

        while (t < normalized_next_position)
        {
            Eigen::Vector3f const tangent = _curve.compute_tangent(t);
            Eigen::Vector3f const orthogonal(tangent[1], -tangent[0], 0.0f);

            Eigen::Vector3f const new_particle_position = _curve.get_pos_on_curve(t);

            Particle p;
            p.position = new_particle_position + orthogonal * (_rng() / float(_rng.max()) - 0.5f) * 0.01f;
//            p.color = Color4(1.0f, 0.0f, 0.0f, h3.getNext() * 0.4f + 0.6f);
//            p.size_factor = h2.getNext() * 2.0f + 0.3f;
            p.color = _curve_color;
            p.color.a = _rng() / float(_rng.max()) * 0.6f + 0.4f;
            p.size_factor = _rng() / float(_rng.max()) * 2.0f + 0.3f;
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
                p.size_factor = _rng() / float(_rng.max()) * 2.0f + 0.1f;
                p.size_factor *= _particle_size;

                Eigen::Vector3f speed = tangent * 0.1f * _tangent_speed_factor;
                speed += Eigen::Vector3f::Random() * 0.05f * _effect_spread;
                p.speed = speed;

                _effect_particles.push_back(p);
            }
        }
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

void Curved_particle_system::setEffect_color(Color4 const& effect_color)
{
    _effect_color = effect_color;
}




