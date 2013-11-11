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
    void visit(Plane_barrier * b) const override
    {
        if (b->get_extent())
        {
            glDisable(GL_LIGHTING);

            glLineWidth(3.0f);

            glPushMatrix();

            glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
            glMultMatrixf(b->get_transform().data());

//            glColor3f(1.0f, 0.0f, 0.0f);
//            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitX()));
//            glColor3f(0.0f, 1.0f, 0.0f);
//            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitY()));
//            glColor3f(0.0f, 0.0f, 1.0f);
//            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()));

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

    void visit(Charged_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        float charge = b->get_charge();
        charge /= 50.0f;

        if (charge < 0.0f)
        {
            charge *= -1.0f;
            Color c = charge * Atom::atom_colors[int(Atom::Type::Cl)] + (1.0f - charge) * Color(0.8f, 0.8f, 0.8f);
            glColor3fv(c.data());
        }
        else
        {
            Color c = charge * Atom::atom_colors[int(Atom::Type::Na)] + (1.0f - charge) * Color(0.8f, 0.8f, 0.8f);
            glColor3fv(c.data());
        }

//        glColor3f(0.7f, 0.7f, 0.7f);
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
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor4f(0.3f, 0.35f, 0.7f, 0.6f);
        draw_box(b->get_box().min(), b->get_box().max(), 1.0f, true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDisable(GL_LIGHTING);

        glPointSize(24.0f * _scale_factor);

        glBegin(GL_POINTS);
        for (Targeted_particle_system const& p_system : b->get_particle_systems())
        {
            for (Targeted_particle const& p : p_system.get_particles())
            {
                glColor4fv(p.color.data());
                glVertex3fv(p.position.data());
            }
        }
        glEnd();

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        float const unreleased_ratio = 1.0f - b->get_num_released_molecules() / float(b->get_num_max_molecules());

        if (b->get_num_max_molecules() - b->get_num_released_molecules() > 0)
        {
            float animation_position = b->get_animation_count();

            QEasingCurve curve(QEasingCurve::InOutQuad);

            if (animation_position < 0.5f)
            {
                animation_position = curve.valueForProgress(animation_position * 2.0f);
            }
            else
            {
                animation_position = curve.valueForProgress(1.0f - (animation_position - 0.5f) * 2.0f);
            }

            Eigen::Vector3f arrow_start = Eigen::Vector3f::Zero();
            arrow_start[0] = b->get_extent()[0] * 0.5f * animation_position;

            Eigen::Vector3f arrow_end = arrow_start;
            arrow_end[0] += 10.0f * unreleased_ratio + 2.0f;

            glDisable(GL_LIGHTING);

            glLineWidth(3.0f);
            glColor4f(0.3f, 0.35f, 0.7f, 0.6f);

            glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            draw_arrow_z_plane_bold(Eigen2OM(arrow_start), Eigen2OM(arrow_end), 1.0f - std::max(0.0f, unreleased_ratio - 0.2f));
            glPopMatrix();
        }

        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

//    void visit(Molecule_releaser * b) const override
//    {
//        glPushMatrix();

//        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
//        glMultMatrixf(b->get_transform().data());


//        glDisable(GL_LIGHTING);

//        glPointSize(24.0f * _scale_factor);

////        draw_box(b->get_box().min(), b->get_box().max());
//        glBegin(GL_POINTS);
//        for (Targeted_particle_system const& p_system : b->get_particle_systems())
//        {
//            for (Targeted_particle const& p : p_system.get_particles())
//            {
//                Color4 color(p.color, 1.0f);
//                glColor4fv(color.data());
//                glVertex3fv(p.position.data());
//            }
//        }
//        glEnd();

////        _texture_program->bind();

//        glEnable(GL_LIGHTING);


//        glEnable(GL_TEXTURE_2D);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, _molecule_releaser_tex);
////        glColor4f(0.8f, 0.8f, 0.6f, 1.0f);
//        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//        glPushMatrix();
////        glScalef(7.9f, 7.9f, 7.9f);
//        glScalef(b->get_box().sizes()[0], b->get_box().sizes()[1] * 0.95f, b->get_box().sizes()[2]);

//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//        draw_mesh(_molecule_releaser_mesh);
//        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

//        glBindTexture(GL_TEXTURE_2D, 0);
////        _texture_program->release();

//        glPopMatrix();

//        glDisable(GL_TEXTURE_2D);

////        draw_box(b->get_box().min(), b->get_box().max());


//        if (b->is_selected())
//        {
//            glColor3f(1.0f, 1.0f, 0.7f);
//            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
//            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//        }

//        Eigen::Vector3f arrow_start = Eigen::Vector3f::Zero();
//        arrow_start[0] = b->get_extent()[0] * 0.5f;

//        Eigen::Vector3f arrow_end = arrow_start;
//        arrow_end[0] += 15.0f;

//        glDisable(GL_LIGHTING);

//        glLineWidth(3.0f);
//        glColor3f(1.0f, 1.0f, 1.0f);

//        glPushMatrix();
//        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
//        draw_arrow_z_plane_bold(Eigen2OM(arrow_start), Eigen2OM(arrow_end));
//        glPopMatrix();

//        glEnable(GL_LIGHTING);

//        glPopMatrix();
//    }

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

        glPointSize(12.0f * _scale_factor);

//        draw_box(b->get_box().min(), b->get_box().max());
        glBegin(GL_POINTS);
        for (Particle const& p : b->get_particles())
        {
            glVertex3fv(p.position.data());
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);

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

    void visit(Tractor_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glDisable(GL_LIGHTING);

        glPointSize(24.0f * _scale_factor);

//        draw_box(b->get_box().min(), b->get_box().max());
        glBegin(GL_POINTS);
        for (Particle const& p : b->get_particles())
        {
            Color4 color(p.color.rgb(), 1.0f - p.age);
            glColor4fv(color.data());
            glVertex3fv(p.position.data());
        }
        glEnd();

        glEnable(GL_LIGHTING);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glColor3f(0.3f, 0.3f, 0.3f);
//        draw_box(b->get_box().min(), b->get_box().max(), 1.0f, true);

        glPushMatrix();
        glScalef(b->get_box().sizes()[0], b->get_box().sizes()[1] * 0.95f, b->get_box().sizes()[2]);
        glRotatef(b->get_rotation_angle(), 1.0f, 0.0f, 0.0f);
        glRotatef(90, 0.0f, 1.0f, 0.0f);
        draw_mesh(_tractor_circle_mesh);
        glPopMatrix();

        if (b->is_selected())
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
        }

        glPopMatrix();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void visit(Box_portal * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor4f(0.3f, 0.8f, 0.4f, 0.6f);
        draw_box(b->get_box().min(), b->get_box().max(), 1.0f, true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDisable(GL_LIGHTING);

        glPushMatrix();
        if (b->get_box().sizes()[0] < b->get_box().sizes()[2])
        {
            glTranslatef(b->get_box().sizes()[0] * 0.5f + 1.5f, -b->get_box().sizes()[1] * 0.5f, 0.0f);
            glScalef(1.0f, 1.0f, b->get_box().sizes()[2]);
        }
        else
        {
            glTranslatef(0.0f, -b->get_box().sizes()[1] * 0.5f, -b->get_box().sizes()[2] * 0.5f - 1.5f);
            glScalef(b->get_box().sizes()[0], 1.0f, 1.0f);
            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        }
        glTranslatef(-0.5f, 0.0f, -0.5f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        int const num_capped_molecules = b->get_condition().get_num_captured_molecules();
        int const min_capped_molecules = b->get_condition().get_min_captured_molecules();

        float const rect_size_x = 1.0f;
        float const rect_size_y = 1.0f / float(min_capped_molecules);

        for (int i = 0; i < min_capped_molecules; ++i)
        {
            if (i < num_capped_molecules)
            {
                glColor4f(0.4f, 1.0f, 0.6f, 0.6f);
            }
            else
            {
                glColor4f(0.4f, 0.5f, 0.4f, 0.4f);
            }

            draw_rect_z_plane(Eigen::Vector3f(0.0f, 0.0f, 0.0f), Eigen::Vector3f(rect_size_x, rect_size_y * 0.9f, 0.0f));

            glTranslatef(0.0f, rect_size_y, 0.0f);
        }

        glPopMatrix();

        glPointSize(24.0f * _scale_factor);

        glBegin(GL_POINTS);
        for (Particle const& p : b->get_particles())
        {
            glColor4fv(p.color.data());
            glVertex3fv(p.position.data());
        }
        glEnd();

        if (b->is_selected())
        {
            glColor3f(0.3f, 1.0f, 0.4f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.05f, true);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glPopMatrix();
    }

    void visit(Sphere_portal * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor4f(0.3f, 0.8f, 0.4f, 0.6f);
        glPushMatrix();

        Eigen::Vector3f const& extent = b->get_extent();

        glScalef(extent[0] * 0.5f, extent[1] * 0.5f, extent[2] * 0.5f);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        draw_mesh(_icosphere_3_mesh);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        int const num_capped_molecules = b->get_condition().get_num_captured_molecules();
        int const min_capped_molecules = b->get_condition().get_min_captured_molecules();

        float const arc_fraction = 1.0f / float(min_capped_molecules);

        glDisable(GL_LIGHTING);

        for (int i = 0; i < min_capped_molecules; ++i)
        {
            if (i < num_capped_molecules)
            {
                glColor4f(0.4f, 1.0f, 0.6f, 0.6f);

            }
            else
            {
                glColor4f(0.4f, 0.5f, 0.4f, 0.4f);
            }

            draw_arc<Eigen::Vector3f>(1.1f, 1.0f, arc_fraction * 0.9f, std::max(64, min_capped_molecules * 6));

            glRotatef(arc_fraction * 360.0f, 0.0f, 0.0f, 1.0f);
        }

        glPopMatrix();

        glPopMatrix();

        glPointSize(24.0f * _scale_factor);

        glBegin(GL_POINTS);
        for (Particle const& p : b->get_particles())
        {
            glColor4fv(p.color.data());
            glVertex3fv(p.position.data());
        }
        glEnd();

        if (b->is_selected())
        {
            glColor3f(0.3f, 1.0f, 0.4f);
            draw_box(b->get_box().min(), b->get_box().max(), 1.0f, true);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

//        glColor3f(0.3f, 0.8f, 0.4f);
//        draw_box(b->get_box().min(), b->get_box().max());

        glPopMatrix();
    }

    void visit(Particle_system_element * system) const override
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);

        float const lifetime_percentage = system->get_life_percentage();

        float alpha = 1.0f;

        if (lifetime_percentage > 0.5f)
        {
            alpha = (1.0f - lifetime_percentage) * 2.0f;
            alpha *= alpha;
        }

        glPointSize(24.0f * _scale_factor);

        glBegin(GL_POINTS);
        for (Particle const& p : system->get_particles())
        {
            Color4 color(p.color.rgb(), alpha);
            glColor4fv(color.data());
            glVertex3fv(p.position.data());
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);

        glEnable(GL_LIGHTING);
    }

    void init(QGLContext const* context, QSize const& size)
    {
        Frame_buffer<Color> molecule_releaser_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/molecule_releaser.png")));
        _molecule_releaser_tex = create_texture(molecule_releaser_tex_fb);

        Frame_buffer<Color4> brownian_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/brownian_panel.png")));
        _brownian_panel_tex = create_texture(brownian_panel_tex_fb);

        _molecule_releaser_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/molecule_releaser.obj"));

//        typename MyMesh::ConstVertexIter vIt(_molecule_releaser_mesh.vertices_begin()), vEnd(_molecule_releaser_mesh.vertices_end());

//        for (; vIt!=vEnd; ++vIt)
//        {
//            MyMesh::TexCoord2D const& coord = _molecule_releaser_mesh.texcoord2D(vIt.handle());

//            std::cout << vIt.handle() << " tex2d " << coord << std::endl;
//            std::cout << vIt.handle() << " normal " << _molecule_releaser_mesh.normal(vIt.handle()) << std::endl;
//        }

        _tractor_circle_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/tractor_circle.obj"));

        _icosphere_3_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/icosphere_3.obj"));

        _texture_program = std::unique_ptr<QGLShaderProgram>(init_program(context, Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"), Data_config::get_instance()->get_absolute_qfilename("shaders/test.frag")));

        Frame_buffer<Color4> particle_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/particle.png")));
        _particle_tex = create_texture(particle_tex_fb);

        resize(size);
    }

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

    void init(QGLContext const* /* context */)
    {
        Frame_buffer<Color4> brownian_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/brownian_panel.png")));
        _brownian_panel_tex = create_texture(brownian_panel_tex_fb);

        Frame_buffer<Color4> tractor_panel_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/tractor_panel.png")));
        _tractor_panel_tex = create_texture(tractor_panel_tex_fb);
    }


private:
    GLuint _brownian_panel_tex;
    GLuint _tractor_panel_tex;
};


#endif // BARRIER_DRAW_VISITOR_H
