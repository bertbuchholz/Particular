#include "Draw_functions.h"

//#define GL_GLEXT_PROTOTYPES 1
//#include <GL/glext.h>

void set_color(QColor const& c)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

void set_color(Color const& c)
{
    glColor3f(c.R, c.G, c.B);
}

void set_color(Color const& c, float const alpha)
{
    glColor4f(c.R, c.G, c.B, alpha);
}

void draw_disc_simple(int const resolution)
{
    float x1 = 1.0f;
    float y1 = 0.0f;

    glBegin(GL_TRIANGLES);
    for(int i = 1; i <= resolution; ++i)
    {
        float angle = 2.0f * M_PI * i / float(resolution);
        float x2 = std::cos(angle);
        float y2 = std::sin(angle);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(  x1,   y1);
        glVertex2f(  x2,   y2);
        y1 = y2;
        x1 = x2;
    }
    glEnd();
}

void draw_disc_simple_silhouette(int const resolution)
{
    glBegin(GL_LINE_STRIP);

    for(int i = 0; i <= resolution; ++i)
    {
        float angle = 2.0f * M_PI * i / float(resolution);
        float x = std::cos(angle);
        float y = std::sin(angle);
        glVertex2f(x, y);
    }

    glEnd();
}

//GLuint create_texture(Frame_buffer<Color> const& frame)
//{
//    GLuint texture_index;

//    glGenTextures(1, &texture_index);

//    glBindTexture(GL_TEXTURE_2D, texture_index);

//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.get_width(), frame.get_height(), 0,
//                 GL_RGB, GL_FLOAT, frame.get_raw_data());

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

////    glGenerateMipmap(GL_TEXTURE_2D);

//    glBindTexture(GL_TEXTURE_2D, 0);

//    return texture_index;
//}

//void delete_texture(GLuint const id)
//{
//    glDeleteTextures(1, &id);
//}

//GLuint create_texture(Frame_buffer<Color4> const& frame)
//{
//    GLuint texture_index;

//    glGenTextures(1, &texture_index);

//    glBindTexture(GL_TEXTURE_2D, texture_index);

//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.get_width(), frame.get_height(), 0,
//                 GL_RGBA, GL_FLOAT, frame.get_raw_data());

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

////    glGenerateMipmap(GL_TEXTURE_2D);

//    glBindTexture(GL_TEXTURE_2D, 0);

//    return texture_index;
//}

//GLuint create_texture(int const width, int const height)
//{
//    GLuint texture_index;

//    glGenTextures(1, &texture_index);

//    glBindTexture(GL_TEXTURE_2D, texture_index);

//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
//                 GL_RGBA, GL_FLOAT, nullptr);

////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
////    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

//    glBindTexture(GL_TEXTURE_2D, 0);

//    return texture_index;
//}

GLuint bind_framebuffer(Frame_buffer<Color> const& frame)
{
    GLuint texture_index;
    glGenTextures(1, &texture_index);

    glBindTexture(GL_TEXTURE_2D, texture_index);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, frame.get_width(), frame.get_height(), 0,
                 GL_RGB, GL_FLOAT, frame.get_raw_data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture_index;
}

