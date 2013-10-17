#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <QImage>
#include <QFont>
#include <QtGui>
#include <Eigen/Core>
#include <functional>
#include <vector>
#include <boost/serialization/version.hpp>

#include <Utilities.h>
#include <Color.h>
#include <Frame_buffer.h>

struct Particle
{
    Particle() :
        position(Eigen::Vector3f::Zero()),
        speed(Eigen::Vector3f::Zero()),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        age(0.0f),
        size_factor(1.0f)
    { }

    Eigen::Vector3f position;
    Eigen::Vector3f speed;
    Color4 color;
    float age; // between 0 and 1
    float size_factor;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(position);
        ar & BOOST_SERIALIZATION_NVP(speed);
        ar & BOOST_SERIALIZATION_NVP(color);
        ar & BOOST_SERIALIZATION_NVP(age);
    }
};

//BOOST_CLASS_VERSION(Particle, 1)

struct Targeted_particle : public Particle
{
    Targeted_particle() : Particle(),
        target(Eigen::Vector3f::Zero()),
        initial_speed(Eigen::Vector3f::Zero())
    { }

    Eigen::Vector3f target;
    Eigen::Vector3f initial_speed;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Particle);
        ar & BOOST_SERIALIZATION_NVP(initial_speed);
        ar & BOOST_SERIALIZATION_NVP(target);
    }
};

class Particle_system
{
public:
    void animate(std::function<void(Particle&, float const)> particle_function, float const timestep)
    {
        for (Particle & p : _particles)
        {
            particle_function(p, timestep);
        }
    }

private:
    std::vector<Particle> _particles;
};

class Targeted_particle_system
{
public:
    Targeted_particle_system(float const total_duration = 10.0f) : _total_duration(total_duration), _age(0.0f)
    { }

    // rect in 0..1 coordinates
    void generate(std::string const& text, QFont const& main_font, QRectF const& rect)
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

        img.save("/Users/bert/Desktop/text.png");

        generate(img);
    }

    void generate(QImage const& image)
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
                    p.size_factor = std::rand() / float(RAND_MAX) * 0.5f + 0.5f;
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

        std::cout << __PRETTY_FUNCTION__ << " created " << _particles.size() << " particles" << std::endl;
    }

    void init(std::vector<Targeted_particle> const& particles)
    {
        _particles = particles;

        for (Targeted_particle & p : _particles)
        {
            p.initial_speed = (0.05f + 2.5f) * (p.target - p.position) / _total_duration; // 2.5f is the 1/integral(wendland_2_1)
        }
    }

    void animate(float const timestep)
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
                    p.speed += timestep * to_target.normalized() * 0.01f;
                }
                else
                {
                    p.speed += timestep * Eigen::Vector3f::Random() * 0.01f;
                }

                p.position += p.speed * timestep;
            }
        }
    }

    static bool is_dead(Targeted_particle_system const& p)
    {
        return p._age > p._total_duration;
    }

    std::vector<Targeted_particle> const& get_particles() const
    {
        return _particles;
    }

private:
    std::vector<Targeted_particle> _particles;

    float _total_duration;
    float _age;
};

void draw_particle_system(Targeted_particle_system const& system, int const height);

#endif // PARTICLE_SYSTEM_H
