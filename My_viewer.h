#ifndef TEMPLATE_VIEWER_H
#define TEMPLATE_VIEWER_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <iostream>

#include <Options_viewer.h>
#include <Draw_functions.h>


#include <Low_discrepancy_sequences.h>


#include <Registry_parameters.h>

#include "Core.h"
#include "Atom.h"

inline float erf(float const x)
{
    //return std::acos(x);
    return x * x * x;

    float const a = 8.0f * (M_PI - 3.0f) / (3.0f * M_PI * (4.0f - M_PI));

    float const result = std::sqrt(1.0f - std::exp(-(x*x)*(4.0f / M_PI + a * x*x) / (1.0f + a * x*x)));

    return (x < 0.0f) ? -result : result;
}


template <typename Vec3>
class Sample_generator
{
public:
    virtual ~Sample_generator() {}

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

    virtual std::vector<Vec3> generate_samples(int const num_samples, int const start) = 0;
};


typedef Sample_generator<qglviewer::Vec> Sample_generator_Vec;

template <typename Vec3>
class Sample_generator_halton : public Sample_generator<Vec3>
{
public:
    std::vector<Vec3> generate_samples(int const num_samples, int const start)
    {
        std::vector<Vec3> samples;

        Halton halton2(2);
        Halton halton3(3);
        Halton halton5(5);

        halton2.setStart(start);
        halton3.setStart(start);
        halton5.setStart(start);

        for (int i = 0; i < num_samples; ++i)
        {
            Vec3 v(halton2.getNext(), halton3.getNext(), halton5.getNext());

            samples.push_back(v);
        }

        return samples;
    }

    static std::string name()
    {
        return "Sample_generator_halton";
    }

    static Sample_generator<Vec3> * create()
    {
        return new Sample_generator_halton;
    }
};

typedef Sample_generator_halton<qglviewer::Vec> Sample_generator_halton_Vec;

REGISTER_CLASS_WITH_PARAMETERS(Sample_generator_Vec, Sample_generator_halton_Vec);

template <typename Vec3>
class Sample_generator_rand : public Sample_generator<Vec3>
{
public:
    std::vector<Vec3> generate_samples(int const num_samples, int const start)
    {
        std::vector<Vec3> samples;

        srand(start);

        for (int i = 0; i < num_samples; ++i)
        {
            Vec3 v(rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX));

            samples.push_back(v);
        }

        return samples;
    }

    static std::string name()
    {
        return "Sample_generator_rand";
    }

    static Sample_generator<Vec3> * create()
    {
        return new Sample_generator_rand;
    }
};


typedef Sample_generator_rand<qglviewer::Vec> Sample_generator_rand_Vec;

REGISTER_CLASS_WITH_PARAMETERS(Sample_generator_Vec, Sample_generator_rand_Vec);


template <typename Vec3>
class Sample_generator_dummy : public Sample_generator<Vec3>
{
public:
    std::vector<Vec3> generate_samples(int const , int const )
    {
        std::vector<Vec3> samples;
        return samples;
    }

    static std::string name()
    {
        return "Sample_generator_dummy";
    }

    static Sample_generator<Vec3> * create()
    {
        return new Sample_generator_dummy;
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("Bla 1", true));
        parameters.add_parameter(new Parameter("Bla 2", true));
        parameters.add_parameter(new Parameter("Bla 3", true));
        parameters.add_parameter(new Parameter("Bla 4", true));
        return parameters;
    }
};


typedef Sample_generator_dummy<qglviewer::Vec> Sample_generator_dummy_Vec;

REGISTER_CLASS_WITH_PARAMETERS(Sample_generator_Vec, Sample_generator_dummy_Vec);



class My_viewer : public Options_viewer
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    My_viewer()
    {
        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

//        _parameters.add_parameter(new Parameter("num_samples", 1, 1, 100000, update));
//        _parameters.add_parameter(new Parameter("start", 0, 0, 100000, update));
//        _parameters.add_parameter(new Parameter("num_dimensions", 1, 1, 3, update));

        _parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&My_viewer::update_physics_timestep, this)));
        _parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update));
        _parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));

        std::vector<std::string> particle_types = { "H", "O", "H2O" };

        _parameters.add_parameter(new Parameter("particle_type", 0, particle_types, update));

        _parameters.add_parameter(Parameter::create_button("Toggle simulation", std::bind(&My_viewer::toggle_simulation, this)));
        _parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));
        _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));

//        Parameter_registry<Sample_generator_Vec>::create_single_select_instance(&_parameters, "Sample_type", update);
//        Parameter_registry<Sample_generator_Vec>::create_multi_select_instance(&_parameters, "Multi Sample_type", update);
    }

    void init() override
    {
        Base::init();

        restoreStateFromFile();

        glPointSize(5.0f);

        _physics_timer = new QTimer;
        _physics_timer->setInterval(_parameters["physics_timestep_ms"]->get_value<int>());

        connect(_physics_timer, SIGNAL(timeout()), this, SLOT(update_physics()));
//        _physics_timer->start();
    }

    void draw() override
    {
        float const scale = _parameters["global_scale"]->get_value<float>();

        Eigen::Vector3f normal_z = Eigen::Vector3f::UnitZ();

        for (Atom const& atom : _core.get_atoms())
        {
            draw_disc(atom._r, normal_z, scale * atom._radius);
        }

        for (Molecule const& molecule : _core.get_molecules())
        {
            glPushMatrix();

//            glTranslatef(molecule._x[0], molecule._x[1], molecule._x[2]);

            for (Atom const& atom : molecule._atoms)
            {
                draw_disc(atom._r, normal_z, scale * atom._radius);
            }

            glPopMatrix();
        }
    }

    void update_physics_timestep()
    {
        _physics_timer->setInterval(_parameters["physics_timestep_ms"]->get_value<int>());
    }

    float ray_plane_intersection(Vec const& ray_origin, Vec const& ray_dir, Vec const& plane_position, Vec const& plane_normal)
    {
        return OpenMesh::dot(plane_normal, (plane_position - ray_origin)) / OpenMesh::dot(ray_dir, plane_normal);
    }

    OpenMesh::Vec3f QGLV2OM (qglviewer::Vec const& p)
    {
        return OpenMesh::Vec3f(p.x, p.y, p.z);
    }

    qglviewer::Vec OM2QGLV(OpenMesh::Vec3f const& p)
    {
        return qglviewer::Vec(p[0], p[1], p[2]);
    }

    void mousePressEvent(QMouseEvent *event)
    {
        if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier)
        {
            qglviewer::Vec qglv_origin;
            qglviewer::Vec qglv_dir;

            camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

            Vec origin = QGLV2OM(qglv_origin);
            Vec dir    = QGLV2OM(qglv_dir);

            float const t = ray_plane_intersection(origin, dir, Vec(0.0f, 0.0f, 0.0f), Vec(0.0f, 0.0f, 1.0f));

            if (t > 0)
            {
                Eigen::Vector3f const intersect_pos = OM2Eigen(origin + t * dir);

//                Atom const atom(Vec(0.0f), intersect_pos, 1.0f, 1.0f, 1.0f);

                std::string const particle_type = _parameters["particle_type"]->get_value<std::string>();

                // FIXME: add back in
//                if (particle_type == std::string("H"))
//                {
//                    _core.add_atom(Atom::create_hydrogen(intersect_pos));
//                }
//                else if (particle_type == std::string("O"))
//                {
//                    _core.add_atom(Atom::create_oxygen(intersect_pos));
//                }
//                else if (particle_type == std::string("H2O"))
//                {
//                    _core.add_molecule(Molecule::create_water(intersect_pos));
//                }

                _core.add_molecule(Molecule::create_water(intersect_pos));
            }
        }
        else
        {
            Base::mousePressEvent(event);
        }
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        Base::keyPressEvent(event);
    }

    void do_physics_timestep()
    {
        update_physics();

        std::cout << __PRETTY_FUNCTION__ << std::endl;

        for (Molecule const& m : _core.get_molecules())
        {
            std::cout << "x: " << m._x << " F: " << m._force << " v: " << m._v << std::endl;
        }

        update();
    }

    void clear()
    {
        _core.clear();
    }

    void toggle_simulation()
    {
        if (_physics_timer->isActive())
        {
            _physics_timer->stop();
        }
        else
        {
            _physics_timer->start();
        }
    }

    void load_defaults() override
    {
        _core.add_molecule(Molecule::create_water(Eigen::Vector3f(3.0f, 3.0f, 0.0f)));
        _core.add_molecule(Molecule::create_water(Eigen::Vector3f(1.0f, 3.0f, 0.0f)));
        update();
    }

public Q_SLOTS:
    void update_physics()
    {
//        std::cout << _parameters["physics_timestep_ms"]->get_value<int>() << " " << QTime::currentTime().toString("ss:zzz").toStdString() << "\n";
        _core.update(_parameters["physics_timestep_ms"]->get_value<int>() / 1000.0f * _parameters["physics_speed"]->get_value<float>());
    }

private:
    QTimer * _physics_timer;

    Core _core;
};


#endif
