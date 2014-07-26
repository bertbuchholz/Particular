#ifndef BARRIER_DRAW_VISITOR_H
#define BARRIER_DRAW_VISITOR_H

#include <Draw_functions.h>
#include <GL_utilities.h>

#include "Visitor.h"
#include "Level_element.h"
#include "Molecule_releaser.h"
#include "Data_config.h"
#include "Color_utilities.h"

class Level_element_draw_visitor : public Level_element_visitor
{
public:
    void visit(Plane_barrier * b) const override;
    void visit(Box_barrier * b) const override;
    void visit(Charged_barrier * b) const override;
    void visit(Moving_box_barrier * b) const override;
    void visit(Blow_barrier * b) const override;
    void visit(Molecule_releaser * b) const override;

    void visit(Brownian_box * b) const override;
    void visit(Brownian_plane * p_element) const override;

    void visit(Tractor_barrier * b) const override;

    void visit(Box_portal * b) const override;
    void visit(Sphere_portal * b) const override;

    void visit(Particle_system_element * system) const override;

    void init(QGLContext const* context, QSize const& size);

    void resize(QSize const& size)
    {
         _scale_factor = (size.height()) / (768.0f);
    }

private:
    float _scale_factor; // changes with screen size changes

    std::unique_ptr<QGLShaderProgram> _texture_program;

    GLuint _brownian_panel_tex;

    MyMesh _molecule_releaser_mesh;
    GLuint _molecule_releaser_tex;

    MyMesh _tractor_circle_mesh;
    MyMesh _icosphere_3_mesh;

    GLuint _particle_tex;

    std::unique_ptr<QGLShaderProgram> _particle_distance_shader;
};


class Level_element_ui_draw_visitor : public Level_element_visitor
{
public:
    void visit(Brownian_box * b) const override
    {
        if (!(int(b->is_user_editable()) & int(Level_element::Edit_type::Property))) return;

        glEnable(GL_TEXTURE_2D);

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glTranslatef(0.0f, -b->get_extent()[1] * 0.5f - 0.01f, -5.0f);
        glScalef(8.24f, 1.0f, 3.47f);

        glRotatef(90, 1.0, 0.0, 0.0);
        glBindTexture(GL_TEXTURE_2D, _brownian_panel_tex);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopMatrix();
    }

    void visit(Tractor_barrier * b) const override
    {
        if (!(int(b->is_user_editable()) & int(Level_element::Edit_type::Property))) return;

        glEnable(GL_TEXTURE_2D);

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glTranslatef(0.0f, -b->get_extent()[1] * 0.5f - 0.01f, -5.0f);
        glScalef(8.24f, 1.0f, 3.47f);

        glRotatef(90, 1.0, 0.0, 0.0);
        glBindTexture(GL_TEXTURE_2D, _tractor_panel_tex);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopMatrix();
    }

    void visit(Charged_barrier * b) const override
    {
        if (!(int(b->is_user_editable()) & int(Level_element::Edit_type::Property))) return;

        glEnable(GL_TEXTURE_2D);

        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glTranslatef(0.0f, -b->get_extent()[1] * 0.5f - 0.01f, -5.0f);
        glScalef(8.24f, 1.0f, 3.47f);

        glRotatef(90, 1.0, 0.0, 0.0);
        glBindTexture(GL_TEXTURE_2D, _panel_charged_barrier_tex);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        draw_quad_with_tex_coords();
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopMatrix();
    }

    void init()
    {
        GL_functions f;
        f.init();

        Frame_buffer<Color4> brownian_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/brownian_panel.png")));
        _brownian_panel_tex = f.create_texture(brownian_panel_tex_fb);

        Frame_buffer<Color4> tractor_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/tractor_panel.png")));
        _tractor_panel_tex = f.create_texture(tractor_panel_tex_fb);

        Frame_buffer<Color4> panel_charged_barrier_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/panel_charged_barrier.png")));
        _panel_charged_barrier_tex = f.create_texture(panel_charged_barrier_tex_fb);
    }


private:
    GLuint _brownian_panel_tex;
    GLuint _tractor_panel_tex;
    GLuint _panel_charged_barrier_tex;
};


#endif // BARRIER_DRAW_VISITOR_H
