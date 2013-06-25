#ifndef RENDERER_H
#define RENDERER_H

#include <Draw_functions.h>
#include <Icosphere.h>
#include <StandardCamera.h>
#include <Geometry_utils.h>

#include "Atom.h"

class Molecule_renderer
{
public:
    ~Molecule_renderer() {}

    virtual void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const = 0;

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }
};

class Stick_renderer : Molecule_renderer
{
public:
    void draw_molecule(Molecule const& m, float const alpha = 1.0f) const
    {
        glColor4f(1.0f, 1.0f, 1.0f, alpha);

        for (int i = 0; i < int(m._connectivity.size()); ++i)
        {
            std::vector<int> connections = m._connectivity[i];
            Atom const& connector = m._atoms[i];

            for (int const connected_index : connections)
            {
                Atom const& connected = m._atoms[connected_index];

                draw_line(connector._r, connected._r);
            }
        }
    }

    void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const override
    {
        glDisable(GL_LIGHTING);

        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);

        for (Molecule const& molecule : molecules)
        {
            if (!molecule._active) continue;

            draw_molecule(molecule);
        }
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

    static std::string name()
    {
        return "Stick Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Stick_renderer;
    }
};


REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Stick_renderer);

class Ball_renderer : Molecule_renderer
{
public:
    Ball_renderer()
    {
        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);
    }

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f) const
    {
        glPushMatrix();

        float radius = scale * atom._radius;

        if (atom._type == Atom::Type::H)
        {
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
        }
        else if (atom._type == Atom::Type::O)
        {
            glColor4f(0.9f, 0.2f, 0.2f, alpha);
        }
        else if (atom._type == Atom::Type::C)
        {
            glColor4f(0.3f, 0.3f, 0.4f, alpha);
        }
        else if (atom._type == Atom::Type::S)
        {
            glColor4f(0.8f, 0.7f, 0.2f, alpha);
        }
        else if (atom._type == Atom::Type::N)
        {
            glColor4f(0.8f, 0.3f, 0.8f, alpha);
        }
        else if (atom._type == Atom::Type::Na)
        {
            glColor4f(0.8f, 0.3f, 0.8f, alpha);
        }
        else if (atom._type == Atom::Type::Cl)
        {
            glColor4f(0.5f, 0.8f, 0.3f, alpha);
        }
        else if (atom._type == Atom::Type::Charge)
        {
            glColor4f(0.3f, 0.2f, 0.7f, alpha);
            radius = 0.3f;
        }

        glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

        glScalef(radius, radius, radius);

        _icosphere.draw();

        glPopMatrix();
    }

    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f) const
    {
        for (Atom const& atom : molecule._atoms)
        {
            draw_atom(atom, scale, alpha);
        }
    }

    void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const override
    {
        glEnable(GL_LIGHTING);

        for (Molecule const& molecule : molecules)
        {
            if (!molecule._active) continue;

            draw_molecule(molecule, _scale);
        }
    }

    void set_parameters(Parameter_list const& parameters) override
    {
        _scale = parameters["scale"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("scale", 1.0f, 0.1f, 10.0f));
        return parameters;
    }

    static std::string name()
    {
        return "Ball Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Ball_renderer;
    }

private:
    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;

    float _scale;
};

REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Ball_renderer);


class Distance_renderer : Molecule_renderer
{
public:
    void render(std::vector<Molecule> const& molecules, StandardCamera const* camera) const override
    {
        glLineWidth(2.0f);

        std::vector< std::pair<float, int> > distance_indices;

        for (size_t i = 0; i < molecules.size(); ++i)
        {
            if (!molecules[i]._active) continue;

            float const distance = (QGLV2Eigen(camera->position()) - molecules[i]._x).norm();
            distance_indices.push_back(std::pair<float, int>(distance, i));
        }

        std::sort(distance_indices.begin(), distance_indices.end());
        std::reverse(distance_indices.begin(), distance_indices.end());

        //for (Molecule const& molecule : molecules)
        for (std::pair<float, int> const& distance_index : distance_indices)
        {
            Molecule const& molecule = molecules[distance_index.second];
            float const distance = distance_index.first;

//            float const distance = (QGLV2Eigen(camera->position()) - molecule._x).norm();

//            float const normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());

            float normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());
            normalized_distance = std::abs(normalized_distance * 2.0f - 1.0f);

            float const alpha_stick = wendland_2_1(normalized_distance);

//            normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());
//            normalized_distance = (normalized_distance - 0.4f) * 5.0f;
//            normalized_distance = std::abs(normalized_distance * 2.0f - 1.0f);

            float alpha_ball = wendland_2_1(into_range(normalized_distance * 7.0f, 0.0f, 1.0f));

            if (alpha_stick > 0.0f && alpha_stick <= 1.0f)
            {
                glDisable(GL_LIGHTING);
                stick_renderer.draw_molecule(molecule, alpha_stick);
            }

            if (alpha_ball > 0.0f && alpha_ball <= 1.0f)
            {
                glEnable(GL_LIGHTING);
                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
            }

//            if (normalized_distance > 0.0f && normalized_distance < 0.25f)
//            {
//                float const alpha = normalized_distance / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha);
//            }
//            else if (normalized_distance >= 0.25f && normalized_distance < 0.5f)
//            {
//                float const alpha_ball  =        (normalized_distance - 0.25f) / 0.25f;
//                float const alpha_stick = 1.0f - (normalized_distance - 0.25f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
//            }
//            else if (normalized_distance >= 0.5f && normalized_distance < 0.75f)
//            {
//                float const alpha_ball  = 1.0f - (normalized_distance - 0.5f) / 0.25f;
//                float const alpha_stick =        (normalized_distance - 0.5f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
//            }
//            else if (normalized_distance >= 0.75f && normalized_distance < 1.0f)
//            {
//                float const alpha_stick = 1.0f - (normalized_distance - 0.75f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//            }
        }
    }

    void set_parameters(Parameter_list const& parameters) override
    {
        _scale = parameters["scale"]->get_value<float>();
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        parameters.add_parameter(new Parameter("scale", 1.0f, 0.1f, 10.0f));
        return parameters;
    }

    static std::string name()
    {
        return "Distance Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Distance_renderer;
    }

private:
    float _scale;

    Ball_renderer ball_renderer;
    Stick_renderer stick_renderer;
};

REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Distance_renderer);


#endif // RENDERER_H
