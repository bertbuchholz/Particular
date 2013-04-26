#ifndef TEMPLATE_VIEWER_H
#define TEMPLATE_VIEWER_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <iostream>

#include <Eigen/Geometry>

#include <Options_viewer.h>
#include <Draw_functions.h>

#include <Picking.h>
#include <Registry_parameters.h>

#include "Core.h"
#include "Atom.h"
#include "Spatial_hash.h"

class My_viewer : public Options_viewer
{
    Q_OBJECT

public:
    typedef Options_viewer Base;

    My_viewer() :
        _is_dragging(false)
    {
        std::function<void(void)> update = std::bind(static_cast<void (My_viewer::*)()>(&My_viewer::update), this);

        _parameters.add_parameter(new Parameter("physics_timestep_ms", 10, 1, 100, std::bind(&My_viewer::update_physics_timestep, this)));
        _parameters.add_parameter(new Parameter("physics_speed", 1.0f, -10.0f, 100.0f, update));
        _parameters.add_parameter(new Parameter("global_scale", 1.0f, 0.01f, 100.0f, update));

        Parameter_registry<Core>::create_normal_instance("Core", &_parameters, std::bind(&My_viewer::change_core_settings, this));

        _parameters.add_parameter(new Parameter("indicator_scale", 0.1f, 0.01f, 10.0f, update));

        std::vector<std::string> particle_types { "O2", "H2O" };

        _parameters.add_parameter(new Parameter("particle_type", 0, particle_types, update));

        _parameters.add_parameter(new Parameter("Toggle simulation", false, std::bind(&My_viewer::toggle_simulation, this)));
        _parameters.add_parameter(Parameter::create_button("Do physics timestep", std::bind(&My_viewer::do_physics_timestep, this)));
        _parameters.add_parameter(Parameter::create_button("Clear", std::bind(&My_viewer::clear, this)));

//        Parameter_registry<Atomic_force>::create_single_select_instance(&_parameters, "Atomic Force Type");

//        change_core_settings();

        _core.set_parameters(*_parameters.get_child("Core"));
    }

    void change_core_settings()
    {
        _core.set_parameters(*_parameters.get_child("Core"));
        update();
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

        qglviewer::ManipulatedCameraFrame * frame = camera()->frame();
        StandardCamera * my_cam = new StandardCamera(0.1f, 1000.0f);
        my_cam->setFrame(frame);
        setCamera(my_cam);

        icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);

        _picking.init(context(), size()); // FIXME: needs to be resized when viewer changes
    }

    void draw_molecule(Molecule const& molecule, Eigen::Vector3f const& normal_z, float const scale)
    {
        for (Atom const& atom : molecule._atoms)
        {
            glPushMatrix();

            float radius = scale * atom._radius;

            if (atom._type == Atom::Type::H)
            {
                set_color(Color(1.0f));
            }
            else if (atom._type == Atom::Type::O)
            {
                set_color(Color(0.9f, 0.2f, 0.2f));
            }
            else if (atom._type == Atom::Type::Charge)
            {
                set_color(Color(0.3f, 0.2f, 0.7f));
                radius = 0.3f;
            }

            glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

            glScalef(radius, radius, radius);

            icosphere.draw();

            glPopMatrix();
        }
    }

    void picking_draw()
    {
        for (Molecule const& molecule : _core.get_molecules())
        {
            int const index = molecule._id;
            _picking.set_index(index);

            for (Atom const& atom : molecule._atoms)
            {
                if (atom._type == Atom::Type::Charge) continue;

                glPushMatrix();

                glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

//                float radius = scale * atom._radius;
                float radius = atom._radius;
                glScalef(radius, radius, radius);

                icosphere.draw();

                glPopMatrix();
            }
        }
    }

    struct Reject_condition
    {
        bool operator() (Core::Molecule_atom_id const& d) const
        {
            return d.m_id == molecule_id;
        }

        int molecule_id;
    };

    void draw() override
    {
        glEnable(GL_LIGHTING);
//        glDisable(GL_LIGHTING);

        float const scale = _parameters["global_scale"]->get_value<float>();

        Eigen::Vector3f normal_z = Eigen::Vector3f::UnitZ();

        for (Molecule const& molecule : _core.get_molecules())
        {
            draw_molecule(molecule, normal_z, scale);
        }

        if (_parameters["Core/use_indicators"]->get_value<bool>())
        {
            float const indicator_scale = _parameters["indicator_scale"]->get_value<float>();

            set_color(Color(1.0f));

            for (Force_indicator const& f : _core.get_force_indicators())
            {
                draw_point(f._atom._r);

                //glLineWidth(into_range(1.0f, 5.0f, f._force.norm()));
                //            draw_arrow_z_plane(Eigen2OM(f._atom._r), Eigen2OM(f._atom._r + f._force.normalized()));

                draw_arrow_z_plane(Eigen2OM(f._atom._r), Eigen2OM(f._atom._r + f._force * indicator_scale));
            }
        }

        // debug displays
        glDisable(GL_LIGHTING);

        Molecule_external_force const& force = _core.get_user_force();

        if (force._end_time > _core.get_current_time())
        {
            glBegin(GL_LINES);

            glVertex3fv(force._origin.data());
            glVertex3fv(Eigen::Vector3f(force._origin + force._force).data());

            glEnd();

            glPushMatrix();

            glTranslatef(force._origin[0], force._origin[1], force._origin[2]);

            //                float radius = scale * atom._radius;
            float radius = 0.2f;
            glScalef(radius, radius, radius);

            icosphere.draw();

            glPopMatrix();
        }

        Reject_condition reject_cond;

        for (Molecule const& molecule : _core.get_molecules())
        {
            reject_cond.molecule_id = molecule._id;

            for (Atom const& a : molecule._atoms)
            {
                boost::optional<Core::Molecule_atom_hash::Point_data const&> opt_pd = _core.get_molecule_hash().get_closest(a._r, reject_cond);

                if (!opt_pd) continue;

                Core::Molecule_atom_hash::Point_data const& pd = opt_pd.get();

                boost::optional<Molecule const&> opt_molecule = _core.get_molecule(pd.data.m_id);

                assert(opt_molecule);

                Atom const& closest_atom = opt_molecule->_atoms[pd.data.a_id];

                Eigen::Vector3f const force = _core.get_atomic_force().get()->calc_force_between_atoms(closest_atom, a);

                if (force.norm() > 1e-6f)
                {
                    if (force.dot(closest_atom._r - a._r) < 0.0f)
                    {
                        glColor3f(0.3f, 0.8f, 0.2f);
                    }
                    else
                    {
                        glColor3f(0.8f, 0.2f, 0.2f);
                    }


                    glLineWidth(into_range(100.0f * force.norm(), 0.5f, 10.0f));
                    draw_line(a._r, closest_atom._r);
                }
            }
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

    OpenMesh::Vec3f QGLV2OM(qglviewer::Vec const& p)
    {
        return OpenMesh::Vec3f(p.x, p.y, p.z);
    }

    Eigen::Vector3f QGLV2Eigen(qglviewer::Vec const& p)
    {
        return Eigen::Vector3f(p.x, p.y, p.z);
    }

    qglviewer::Vec OM2QGLV(OpenMesh::Vec3f const& p)
    {
        return qglviewer::Vec(p[0], p[1], p[2]);
    }

    OpenMesh::Vec3f Eigen2OM(Eigen::Vector3f const& p)
    {
        return OpenMesh::Vec3f(p[0], p[1], p[2]);
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

                std::string const particle_type = _parameters["particle_type"]->get_value<std::string>();

                if (particle_type == std::string("O2"))
                {
                    _core.add_molecule(Molecule::create_oxygen(intersect_pos));
                }
                else if (particle_type == std::string("H2O"))
                {
                    _core.add_molecule(Molecule::create_water(intersect_pos));
                }
            }
        }
        else if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::AltModifier)
        {
            std::cout << __PRETTY_FUNCTION__ << " Drag/Click" << std::endl;
            _dragging_start = event->pos();

            _picked_index = _picking.do_pick(event->pos().x(), height() - event->pos().y(), std::bind(&My_viewer::picking_draw, this));

            std::cout << __PRETTY_FUNCTION__ << " index: " << _picked_index << std::endl;

            if (_picked_index > 0)
            {
                bool found;
                qglviewer::Vec world_pos = camera()->pointUnderPixel(event->pos(), found);

                if (found)
                {
                    boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

                    assert(picked_molecule);

                    Molecule_external_force & f = _core.get_user_force();
                    f._molecule_id = _picked_index;
                    qglviewer::Vec dir = world_pos - camera()->position();
                    dir.normalize();
                    f._force = 1.0f * QGLV2Eigen(dir);
                    f._origin = QGLV2Eigen(world_pos);
                    f._local_origin = picked_molecule->_R.transpose() * (f._origin - picked_molecule->_x);
                    f._end_time = _core.get_current_time();

                    std::cout << "Apply force: " << f._origin << std::endl;
                }
            }
        }
        else
        {
            Base::mousePressEvent(event);
        }
    }

    void mouseMoveEvent(QMouseEvent * event) override
    {
        if (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::AltModifier)
        {
            // update drag indicator

            if (!_is_dragging)
            {
                if (_picked_index > 0 && (_dragging_start - event->pos()).manhattanLength() > 0)
                {
                    _is_dragging = true;
                }
            }

            if (_is_dragging)
            {
                boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

                assert(picked_molecule);

                Molecule_external_force & f = _core.get_user_force();

                Eigen::Hyperplane<float, 3> view_plane(QGLV2Eigen(camera()->viewDirection()), f._origin);

                qglviewer::Vec qglv_origin;
                qglviewer::Vec qglv_dir;

                camera()->convertClickToLine(event->pos(), qglv_origin, qglv_dir);

                Eigen::Vector3f origin = QGLV2Eigen(qglv_origin);
                Eigen::Vector3f dir    = QGLV2Eigen(qglv_dir).normalized();

                Eigen::ParametrizedLine<float, 3> line(origin, dir);

                Eigen::Vector3f new_force_target = line.intersectionPoint(view_plane);

                f._origin = picked_molecule->_R * f._local_origin + picked_molecule->_x;
//                f._force = 1.0f * (new_force_target - f._origin).normalized();
                f._force = new_force_target - f._origin;
                f._end_time = _core.get_current_time() + 0.1f;
            }
        }
        else
        {
            Base::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent * event)
    {
        if (_is_dragging)
        {
            _is_dragging = false;
        }
        else if (event->modifiers() & Qt::AltModifier)
        {
            std::cout << __PRETTY_FUNCTION__ << " click" << std::endl;

            if (_picked_index > 0)
            {
                Molecule_external_force & f = _core.get_user_force();

                f._end_time = _core.get_current_time() + 0.5f;
            }
        }
        else
        {
            Base::mouseReleaseEvent(event);
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

    void animate() override
    {
        if (_is_dragging)
        {
            boost::optional<Molecule const&> picked_molecule = _core.get_molecule(_picked_index);

            assert(picked_molecule);

            Molecule_external_force & f = _core.get_user_force();

            Eigen::Vector3f old_force_target = f._origin + f._force;

            f._origin = picked_molecule->_R * f._local_origin + picked_molecule->_x;
//                f._force = 1.0f * (new_force_target - f._origin).normalized();
            f._force = old_force_target - f._origin;
            f._end_time = _core.get_current_time() + 0.1f;
        }

        Base::animate();
    }

    void clear()
    {
        _core.clear();
        update();
    }

    void toggle_simulation()
    {
        if (!_parameters["Toggle simulation"]->get_value<bool>())
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

        float const strength = 1.0f;
        float const radius   = 0.5f;

        _core.add_barrier(new Line_barrier(Eigen::Vector3f(-5.0f, 0.0f, 0.0f), Eigen::Vector3f( 1.0f, 0.0f, 0.0f), strength, radius));
        _core.add_barrier(new Line_barrier(Eigen::Vector3f( 5.0f, 0.0f, 0.0f), Eigen::Vector3f(-1.0f, 0.0f, 0.0f), strength, radius));

        _core.add_barrier(new Line_barrier(Eigen::Vector3f(0.0f, -5.0f, 0.0f), Eigen::Vector3f(0.0f,  1.0f, 0.0f), strength, radius));
        _core.add_barrier(new Line_barrier(Eigen::Vector3f(0.0f,  5.0f, 0.0f), Eigen::Vector3f(0.0f, -1.0f, 0.0f), strength, radius));

        update();
    }

    static void test()
    {
        Eigen::Vector3f point(-3.0f, 0.0f, 0.0f);
        Line_barrier b(Eigen::Vector3f(-5.0f, 0.0f, 0.0f), Eigen::Vector3f( 1.0f, 0.0f, 0.0f), 1.0f, 1.0f);
        Force_indicator f(point);
        std::cout << b.calc_force(f._atom) << std::endl;

        point = Eigen::Vector3f(-1.0f, 0.0f, 0.0f);
        f = Force_indicator(point);
        std::cout << b.calc_force(f._atom) << std::endl;

        point = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
        f = Force_indicator(point);
        std::cout << b.calc_force(f._atom) << std::endl;
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

    IcoSphere<OpenMesh::Vec3f, Color> icosphere;

    Picking _picking;
    bool _is_dragging;
    QPoint _dragging_start;
    int _picked_index;
};


#endif
