#include <Renderer.h>

void setup_gl_points(const bool distance_dependent)
{
    if (distance_dependent)
    {

        float quadratic[] =  { 0.0f, 0.0f, 0.001f };
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION, quadratic);

        //        glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

        //        //Tell it the max and min sizes we can use using our pre-filled array.
    }
    else
    {
        float quadratic[] =  { 1.0f, 0.0f, 0.0f };
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION, quadratic);
//        glPointParameterf(GL_POINT_SIZE_MIN, 1.0f);
//        glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);
//        glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
//        glPointSize(8.0f);

    }

    glDisable(GL_POINT_SPRITE);
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
    glPointParameterf(GL_POINT_SIZE_MIN, 2.0f);
    glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);

    glEnable(GL_POINT_SMOOTH);
}
