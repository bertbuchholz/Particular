#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <QImage>
#include <Eigen/Core>
#include <functional>
#include <vector>


#include <Utilities.h>
#include <Color.h>
#include <Frame_buffer.h>

struct Particle
{
    Eigen::Vector3f position;
    Eigen::Vector3f speed;
    Color color;
    float age; // between 0 and 1

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & position;
        ar & speed;
        ar & color;
        ar & age;
    }
};

struct Targeted_particle : public Particle
{
    Eigen::Vector3f target;
    Eigen::Vector3f initial_speed;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<Particle>(*this);
        ar & initial_speed;
        ar & target;
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
    Targeted_particle_system(float const total_duration = 10.0f) : _total_duration(total_duration)
    { }

    void generate(std::string const& text)
    {

        QImage img(text.size() * 30, 100, QImage::Format_RGB32);
        img.fill(QColor(0, 0, 0));

        QPainter p(&img);

        QFont font;
        font.setWeight(QFont::Bold);
        font.setPixelSize(25);
        p.setFont(font);

        p.setPen(QColor(255, 255, 255));
        p.setBrush(Qt::NoBrush);

        p.drawText(QRect(15, 15, img.width() - 30, img.height() - 30), Qt::AlignCenter, QString::fromStdString(text));

//        img.save("/Users/bert/Desktop/text.png");

        generate(img);
    }

    void generate(QImage const& image)
    {
        _particles.reserve(10000);

        for (int x = 0; x < image.width(); ++x)
        {
            for (int y = 0; y < image.height(); ++y)
            {
                if (qRed(image.pixel(x, y)) > 128)
                {
                    Targeted_particle p;
                    p.age = 0.0f;
                    p.color = Color(1.0f, 0.0f, 0.0f);
                    p.position = Eigen::Vector3f::Random();
                    p.position[2] = 0.0f;
                    p.target = Eigen::Vector3f(  x / float(image.width())  * 2.0f - 1.0f,
                                               -(y / float(image.height()) * 2.0f - 1.0f),
                                               0.0f);

//                    p.speed = Eigen::Vector3f::Random();

                    p.initial_speed = 2.5f * (p.target - p.position) / _total_duration; // 2.5f is the 1/integral(wendland_2_1)

                    p.speed[2] = 0.0f;


                    _particles.push_back(p);
                }
            }
        }

        std::cout << __PRETTY_FUNCTION__ << " created " << _particles.size() << " particles" << std::endl;
    }

    void animate(float const timestep)
    {
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

    std::vector<Targeted_particle> const& get_particles() const
    {
        return _particles;
    }

private:
    std::vector<Targeted_particle> _particles;

    float _total_duration;
};

#endif // PARTICLE_SYSTEM_H
