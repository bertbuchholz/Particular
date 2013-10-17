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

    glBegin(GL_POINTS);

    for (Targeted_particle const& p : system.get_particles())
    {
        glPointSize(p.size_factor * 4.0f * height / (768.0f));
        glColor4fv(p.color.data());
        glVertex3fv(p.position.data());
    }

    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
