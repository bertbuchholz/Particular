#ifndef BARRIER_DRAW_VISITOR_H
#define BARRIER_DRAW_VISITOR_H

#include <Draw_functions.h>

#include "Visitor.h"
#include "Level_elements.h"


class Level_element_draw_visitor : public Level_element_visitor
{
public:
    void visit(Plane_barrier * b) const override
    {
        if (b->get_extent())
        {
            glDisable(GL_LIGHTING);

            glLineWidth(3.0f);

            glPushMatrix();

            glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
            glMultMatrixf(b->get_transform().data());

            glColor3f(1.0f, 0.0f, 0.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitX()));
            glColor3f(0.0f, 1.0f, 0.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitY()));
            glColor3f(0.0f, 0.0f, 1.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()));

            glColor3f(0.7f, 0.7f, 0.7f);
            drawRect(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()),
                     b->get_extent().get()[0] * 0.5f, b->get_extent().get()[1] * 0.5f, Color(0.7f), true);

            if (b->is_selected())
            {
                drawRect(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()),
                         b->get_extent().get()[0] * 0.55f, b->get_extent().get()[1] * 0.55f, Color(1.0f, 1.0f, 0.7f), true);
            }

            glPopMatrix();

            glEnable(GL_LIGHTING);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    void visit(Box_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.7f, 0.7f, 0.7f);
        draw_box(b->get_box().min(), b->get_box().max());

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glPopMatrix();
    }

    void visit(Moving_box_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.7f, 0.7f, 0.7f);
        draw_box(b->get_box().min(), b->get_box().max());

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        glPopMatrix();
    }

    void visit(Blow_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.5f, 0.6f, 0.8f);
        draw_box(b->get_box().min(), b->get_box().max());

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        Eigen::Vector3f arrow_start = Eigen::Vector3f::Zero();
        arrow_start[int(b->get_direction())] = b->get_extent()[int(b->get_direction())] * 0.5f;

        Eigen::Vector3f arrow_end = arrow_start;
        arrow_end[int(b->get_direction())] += 15.0f;

        glDisable(GL_LIGHTING);

        glLineWidth(3.0f);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        draw_arrow_z_plane(Eigen2OM(arrow_start), Eigen2OM(arrow_end));
        glPopMatrix();

        glPopMatrix();

        glEnable(GL_LIGHTING);
    }

    void visit(Molecule_releaser * b) const override
    {
        glEnable(GL_LIGHTING);

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());


//        _texture_program->bind();

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _molecule_releaser_tex);
//        glColor4f(0.8f, 0.8f, 0.6f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glPushMatrix();
//        glScalef(7.9f, 7.9f, 7.9f);
        glScalef(b->get_box().sizes()[0], b->get_box().sizes()[1] * 0.95f, b->get_box().sizes()[2]);

        draw_mesh(_molecule_releaser_mesh);

        glBindTexture(GL_TEXTURE_2D, 0);
//        _texture_program->release();

        glPopMatrix();

        glDisable(GL_TEXTURE_2D);

//        draw_box(b->get_box().min(), b->get_box().max());


        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        Eigen::Vector3f arrow_start = Eigen::Vector3f::Zero();
        arrow_start[0] = b->get_extent()[0] * 0.5f;

        Eigen::Vector3f arrow_end = arrow_start;
        arrow_end[0] += 15.0f;

        glDisable(GL_LIGHTING);

        glLineWidth(3.0f);
        glColor3f(1.0f, 1.0f, 1.0f);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        draw_arrow_z_plane(Eigen2OM(arrow_start), Eigen2OM(arrow_end));
        glPopMatrix();

        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    void visit(Brownian_box * b) const override
    {
        float const max_strength = 50.0f;

        float const strength = into_range(b->get_strength() / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

        Color c(strength, 0.0f, 1.0f - strength);
        glColor3fv(c.data());

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glDisable(GL_LIGHTING);

//        draw_box(b->get_box().min(), b->get_box().max());
        glBegin(GL_POINTS);
        for (Particle const& p : b->get_particles())
        {
            glVertex3fv(p.position.data());
        }
        glEnd();

        glEnable(GL_LIGHTING);

        glColor4f(c[0], c[1], c[2], 0.4f);
        draw_box(b->get_box().min(), b->get_box().max(), 1.0f, true);

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        glPopMatrix();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void visit(Brownian_plane * p_element) const override
    {
        float const max_strength = 50.0f;

        float const strength = into_range(p_element->get_strength() / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

        Color c(strength, 0.0f, 1.0f - strength);

        drawRect(Eigen::Vector3f(p_element->get_plane().offset() * p_element->get_plane().normal()),
                 Eigen::Vector3f(p_element->get_plane().normal()), 20.0f, c);
    }

    void visit(Box_portal * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.3f, 0.8f, 0.4f);
        draw_box(b->get_box().min(), b->get_box().max());

        if (b->is_selected())
        {
            glColor3f(0.3f, 1.0f, 0.4f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        glPopMatrix();
    }

    void visit(Particle_system_element * system) const override
    {
        glDisable(GL_LIGHTING);

        float const lifetime_percentage = system->get_life_percentage();

        float alpha = 1.0f;

        if (lifetime_percentage > 0.5f)
        {
            alpha = (1.0f - lifetime_percentage) * 2.0f;
            alpha *= alpha;
        }

        glBegin(GL_POINTS);
        for (Particle const& p : system->get_particles())
        {
//            glColor4f(1.0f, 0.7f, 0.6f, alpha);
            Color4 color(p.color, alpha);
            glColor4fv(color.data());
            glVertex3fv(p.position.data());
        }
        glEnd();
    }

    void init(QGLContext const* context)
    {

        Frame_buffer<Color> molecule_releaser_tex_fb = convert<QColor_to_Color_converter, Color>(QImage("data/textures/molecule_releaser.png"));
        _molecule_releaser_tex = create_texture(molecule_releaser_tex_fb);

        Frame_buffer<Color4> brownian_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage("data/textures/brownian_panel.png"));
        _brownian_panel_tex = create_texture(brownian_panel_tex_fb);

        _molecule_releaser_mesh = load_mesh<MyMesh>("data/meshes/molecule_releaser.obj");

        typename MyMesh::ConstVertexIter vIt(_molecule_releaser_mesh.vertices_begin()), vEnd(_molecule_releaser_mesh.vertices_end());

        for (; vIt!=vEnd; ++vIt)
        {
            MyMesh::TexCoord2D const& coord = _molecule_releaser_mesh.texcoord2D(vIt.handle());

            std::cout << vIt.handle() << " tex2d " << coord << std::endl;
            std::cout << vIt.handle() << " normal " << _molecule_releaser_mesh.normal(vIt.handle()) << std::endl;
        }

        _texture_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/temperature.vert", "data/shaders/test.frag"));
    }

private:
    std::unique_ptr<QGLShaderProgram> _texture_program;

    GLuint _brownian_panel_tex;

    MyMesh _molecule_releaser_mesh;
    GLuint _molecule_releaser_tex;
};


class Level_element_ui_draw_visitor : public Level_element_visitor
{
public:
    void visit(Brownian_box * b) const override
    {
        glEnable(GL_TEXTURE_2D);

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glTranslatef(0.0f, -b->get_extent()[1] * 0.5f - 0.01f, -5.0f);
        glScalef(8.24f, 1.0f, 3.47f);

        glRotatef(90, 1.0, 0.0, 0.0);
        glBindTexture(GL_TEXTURE_2D, _brownian_panel_tex);
        glColor3f(1.0f, 1.0f, 1.0f);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopMatrix();
    }

    void init(QGLContext const* /* context */)
    {
        Frame_buffer<Color4> brownian_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage("data/textures/brownian_panel.png"));
        _brownian_panel_tex = create_texture(brownian_panel_tex_fb);
    }


private:
    GLuint _brownian_panel_tex;
};


#endif // BARRIER_DRAW_VISITOR_H
