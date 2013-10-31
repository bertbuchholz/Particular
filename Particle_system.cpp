#include "Particle_system.h"

#include <QtOpenGL>

void draw_particle_system(Targeted_particle_system const& system, int const height)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0f, 1.0f, 0.0f, 1.0, 1.0f, -1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(0.5f, 0.5f, 0.0f);

    glScalef(0.5f, 0.5f, 1.0f);

    //        glBindTexture(GL_TEXTURE_2D, _particle_tex);

    for (Targeted_particle const& p : system.get_particles())
    {
        glPointSize(p.size_factor * 4.0f * height / (768.0f));
        glBegin(GL_POINTS);
        glColor4fv(p.color.data());
        glVertex3fv(p.position.data());
        glEnd();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


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

    img.save("/Users/bert/Desktop/text.png");

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

    std::cout << __PRETTY_FUNCTION__ << " created " << _particles.size() << " particles" << std::endl;
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
