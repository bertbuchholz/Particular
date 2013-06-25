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

            glPushMatrix();

            glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
            glMultMatrixf(b->get_transform().data());

            //        drawRect(Eigen::Vector3f(b->get_plane().offset() * b->get_plane().normal()),
            //                 Eigen::Vector3f(b->get_plane().normal()), 20.0f, Color(0.7f), true);

            glColor3f(1.0f, 0.0f, 0.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitX()));
            glColor3f(0.0f, 1.0f, 0.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitY()));
            glColor3f(0.0f, 0.0f, 1.0f);
            drawLine(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()));

            glColor3f(0.7f, 0.7f, 0.7f);
            drawRect(Eigen::Vector3f(Eigen::Vector3f::Zero()), Eigen::Vector3f(Eigen::Vector3f::UnitZ()),
                     b->get_extent().get()[0] * 0.5f, b->get_extent().get()[1] * 0.5f, Color(0.7f), true);

            glPopMatrix();

            glEnable(GL_LIGHTING);
        }
    }

    void visit(Box_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.7f, 0.7f, 0.7f);
        draw_box(b->get_box().min(), b->get_box().max());

        glPopMatrix();
    }

    void visit(Moving_box_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.7f, 0.7f, 0.7f);
        draw_box(b->get_box().min(), b->get_box().max());

        glPopMatrix();
    }

    void visit(Blow_barrier * b) const override
    {
        glPushMatrix();

        glTranslatef(b->get_position()[0], b->get_position()[1], b->get_position()[2]);
        glMultMatrixf(b->get_transform().data());

        glColor3f(0.5f, 0.6f, 0.8f);
        draw_box(b->get_box().min(), b->get_box().max());

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

        glColor3f(0.5f, 0.6f, 0.8f);
        draw_box(b->get_box().min(), b->get_box().max());

        Eigen::Vector3f arrow_start = Eigen::Vector3f::Zero();
        arrow_start[0] = b->get_extent()[0] * 0.5f;

        Eigen::Vector3f arrow_end = arrow_start;
        arrow_end[0] += 15.0f;

        glDisable(GL_LIGHTING);

        glLineWidth(3.0f);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        draw_arrow_z_plane(Eigen2OM(arrow_start), Eigen2OM(arrow_end));
        glPopMatrix();

        glPopMatrix();

        glEnable(GL_LIGHTING);
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

        draw_box(b->get_box().min(), b->get_box().max());

        glPopMatrix();
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

        glPopMatrix();
    }
};


#endif // BARRIER_DRAW_VISITOR_H
