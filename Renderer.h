#ifndef RENDERER_H
#define RENDERER_H

#include <Draw_functions.h>
#include <Icosphere.h>

#include "Atom.h"

class Molecule_renderer
{
public:
    ~Molecule_renderer() {}

    virtual void render(std::vector<Molecule> const& molecules) const = 0;

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
    void draw_molecule(Molecule const& m) const
    {
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

    void render(std::vector<Molecule> const& molecules) const override
    {
        glDisable(GL_LIGHTING);

        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);

        for (Molecule const& molecule : molecules)
        {
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

    void draw_molecule(Molecule const& molecule, float const scale) const
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
            else if (atom._type == Atom::Type::C)
            {
                set_color(Color(0.3f, 0.3f, 0.4f));
            }
            else if (atom._type == Atom::Type::S)
            {
                set_color(Color(0.8f, 0.7f, 0.2f));
            }
            else if (atom._type == Atom::Type::N)
            {
                set_color(Color(0.8f, 0.3f, 0.8f));
            }
            else if (atom._type == Atom::Type::Charge)
            {
                set_color(Color(0.3f, 0.2f, 0.7f));
                radius = 0.3f;
            }

            glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

            glScalef(radius, radius, radius);

            _icosphere.draw();

            glPopMatrix();
        }
    }

    void render(std::vector<Molecule> const& molecules) const override
    {
        glEnable(GL_LIGHTING);

        for (Molecule const& molecule : molecules)
        {
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


#endif // RENDERER_H
