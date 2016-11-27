#pragma once

#include <cmath>

#include <QGLViewer/qglviewer.h>
#include <QOpenGLFunctions_3_3_Core>
#include <Eigen/Core>

#include "Icosphere.h"
#include "MyOpenMesh.h"
#include "Color.h"
#include "Frame_buffer.h"
#include "Geometry_utils.h"

void draw_disc_simple(int const resolution = 10);
void draw_disc_simple_silhouette(int const resolution = 10);

//void set_color(yafaray::color_t const& c);
//void set_color(yafaray::colorA_t const& c);

void set_color(QColor const& c);
void set_color(Color const& c);
void set_color(Color const& c, float const alpha);

//GLuint create_texture(Frame_buffer<Color> const& frame);
//GLuint create_texture(Frame_buffer<Color4> const& frame);
//GLuint create_texture(int const width, int const height);
GLuint bind_framebuffer(Frame_buffer<Color> const& frame);
//void delete_texture(GLuint const id);

inline void draw_quad_with_tex_coords()
{
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,  0.0f);
    glVertex2f  (-1.0f, -1.0f);
    glTexCoord2f( 1.0f,  0.0f);
    glVertex2f  ( 1.0f, -1.0f);
    glTexCoord2f( 1.0f,  1.0f);
    glVertex2f  ( 1.0f,  1.0f);
    glTexCoord2f( 0.0f,  1.0f);
    glVertex2f  (-1.0f,  1.0f);
    glEnd();
}


inline void draw_quad_with_tex_coords(float const uv_scale)
{
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,  0.0f);
    glVertex2f  (-1.0f, -1.0f);
    glTexCoord2f( 1.0f * uv_scale, 0.0f * uv_scale);
    glVertex2f  ( 1.0f, -1.0f);
    glTexCoord2f( 1.0f * uv_scale,  1.0f * uv_scale);
    glVertex2f  ( 1.0f,  1.0f);
    glTexCoord2f( 0.0f * uv_scale,  1.0f * uv_scale);
    glVertex2f  (-1.0f,  1.0f);
    glEnd();
}


template <class Point>
void gl_apply_coordinate_system(Point const& normal, Point const& tangent, Point const& bitangent)
{
    GLfloat m[16];

    m[0] = tangent[0];
    m[1] = tangent[1];
    m[2] = tangent[2];
    m[3] = 0;

    m[4] = bitangent[0];
    m[5] = bitangent[1];
    m[6] = bitangent[2];
    m[7] = 0;

    m[8]  = normal[0];
    m[9]  = normal[1];
    m[10] = normal[2];
    m[11] = 0;

    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;

    glMultMatrixf(m);
}

template <class Point>
void gl_create_and_apply_coordinate_system(Point const& normal)
{
    Point tangent, bitangent;
    create_orthogonal_cs(normal, tangent, bitangent);
    gl_apply_coordinate_system(normal, tangent, bitangent);
}


template <typename Point>
void drawPoint(Point const& point, QColor const& color)
{
    glPointSize(5.0f);

    glBegin(GL_POINTS);

    glColor3f(color.redF(), color.greenF(), color.blueF());
    glVertex3fv(point.data());

    glEnd();
}

template <typename Point>
void draw_point(Point const& point, float const size = 2.0f)
{
    glPointSize(size);

    glBegin(GL_POINTS);

    glVertex3fv(point.data());

    glEnd();
}


template <typename MyMesh>
void draw_point_mesh(MyMesh const& mesh)
{
    glPointSize(1.0f);

    glBegin(GL_POINTS);

    for (typename MyMesh::ConstVertexIter iter = mesh.vertices_begin(); iter != mesh.vertices_end(); ++iter)
    {
        glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
        glVertex3fv(mesh.point(iter).data());
    }

    glEnd();
}



template <typename Point, typename Color>
void drawIcoSphere(Point const& center, float radius, Color const& color, IcoSphere<Point, Color> & sphere)
{
    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glScalef(radius, radius, radius);

    if (radius < 0.0f)
    {
        glColor3f(.8f, 0.8f, 0.4f);
        radius = 1.0f;
    }
    else
    {
        // glColor3f(color[0] * 100.0f, color[1] * 100.0f, color[2] * 100.0f);
        glColor3f(color.R, color.G, color.B);
    }

    // radius = .1f;
    // gluDisk(quadric, 0, radius, 16, 1);

    sphere.draw();

    glPopMatrix();


    glPointSize(2.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

    glBegin(GL_POINTS);

    glVertex3f(center[0], center[1], center[2]);

    glEnd();
}


template <typename Point>
void draw_sphere_ico(Point const& center, float radius, bool const draw_silhouette = false)
{
    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);

    glScalef(radius, radius, radius);

    if (draw_silhouette)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    IcoSphere<Point, int> sphere(2);
    sphere.draw();

    glPopMatrix();
}


template <typename Point, typename Col>
void draw_sphere_ico(Point const& center, float radius, Col const& color, bool const draw_silhouette = false)
{
    set_color(color);

    draw_sphere_ico(center, radius, draw_silhouette);
}


template <typename Point>
void draw_disc(Point const& center, Point const& normal, float const radius, bool const draw_silhouette = false, int const resolution = 10)
{
    Point tangent, bitangent;

    create_orthogonal_cs(normal, tangent, bitangent);

    GLfloat m[16];

    m[0] = tangent[0];
    m[1] = tangent[1];
    m[2] = tangent[2];
    m[3] = 0;

    m[4] = bitangent[0];
    m[5] = bitangent[1];
    m[6] = bitangent[2];
    m[7] = 0;

    m[8]  = normal[0];
    m[9]  = normal[1];
    m[10] = normal[2];
    m[11] = 0;

    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;

    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glScalef(radius, radius, radius);
    glMultMatrixf(m);

    if (draw_silhouette)
    {
        draw_disc_simple_silhouette(resolution);
    }
    else
    {
        draw_disc_simple(resolution);
    }

    glPopMatrix();
}

template <typename Point, typename Color>
void draw_disc(Point const& center, Point const& normal, float const radius, Color const& color, bool const draw_silhouette = false, int const resolution = 10)
{
    set_color(color);

    draw_disc(center, normal, radius, draw_silhouette, resolution);
}


template <typename Point>
void draw_arc(float const outer_radius, float const inner_radius, float const fraction, int const resolution)
{
    int const num_segments = std::max(2, int(resolution * fraction));

    glBegin(GL_TRIANGLE_STRIP);

    for (int i = 0; i < num_segments; ++i)
    {
        float const degrees = 2.0f * M_PI * i / float(num_segments) * fraction;

        float const x = std::cos(degrees);
        float const y = std::sin(degrees);

        glVertex3f(x * inner_radius, y * inner_radius, 0.0f);
        glVertex3f(x * outer_radius, y * outer_radius, 0.0f);
    }

    glEnd();
}


template <typename Point, typename Color>
void drawRect(Point const& center, Point const& normal, float const size_x, float const size_y, Color const& color, bool const draw_contour = false)
{
    Point tangent, bitangent;

    create_orthogonal_cs(normal, tangent, bitangent);

    GLfloat m[16];

    m[0] = tangent[0];
    m[1] = tangent[1];
    m[2] = tangent[2];
    m[3] = 0;

    m[4] = bitangent[0];
    m[5] = bitangent[1];
    m[6] = bitangent[2];
    m[7] = 0;

    m[8]  = normal[0];
    m[9]  = normal[1];
    m[10] = normal[2];
    m[11] = 0;

    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;

    if (draw_contour)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glMultMatrixf(m);
    glScalef(size_x, size_y, 1.0f);

    glColor3f(color.R, color.G, color.B);

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f);
    glVertex2f(-1.0f,  1.0f);
    glEnd();

    glPopMatrix();
}

template <typename Point, typename Color>
void drawRect(Point const& center, Point const& normal, float const radius, Color const& color, bool const draw_contour = false)
{
    drawRect(center, normal, radius, radius, color, draw_contour);
}

template <typename Point>
void draw_rect_z_plane(Point const& center, float const size_x, float const size_y)
{
    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glScalef(size_x, size_y, 1.0f);

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f);
    glVertex2f(-1.0f,  1.0f);
    glEnd();

    glPopMatrix();
}

template <typename Point>
void draw_rect_z_plane(Point const& min, Point const& max)
{
    Point center = 0.5f * (min + max);

    draw_rect_z_plane(center, 0.5f * (max[0] - min[0]), 0.5f * (max[1] - min[1]));
}


template <class Point>
void drawLine(Point const& p1, Point const& p2)
{
    glBegin(GL_LINES);

    glVertex3fv(p1.data());
    glVertex3fv(p2.data());

    glEnd();
}

template <class Point>
void draw_arrow_z_plane(Point const& p1, Point const& p2)
{
    float const relative_head_size = 0.2f;

    drawLine(p1, p2);

    float const length = (p2 - p1).length();

    Point const a_0 = p1 + (p2 - p1) * (1.0f - relative_head_size);

    Point orth_vector = (p2 - p1);
    orth_vector.normalize();
    orth_vector = Point(orth_vector[1], -orth_vector[0], orth_vector[2]);

    Point const a_l = a_0 + length * relative_head_size * orth_vector;
    Point const a_r = a_0 - length * relative_head_size * orth_vector;

    drawLine(p2, a_l);
    drawLine(p2, a_r);
}


template <class Point>
void draw_arrow_z_plane_bold(Point const& p1, Point const& p2, float const relative_head_length = 0.2f)
{
    glPushMatrix();

    float const relative_body_width = 0.6f;

    Point v = p2 - p1;
    float const length = v.length();

    float const angle = std::atan2(v[1], v[0]);

    glRotatef(angle * M_2_PI * 360.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(p1[0] + length * 0.5f, p1[1], 0.0f);
    glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
    glScalef(length, length, 1.0f);
    glTranslatef(-0.5f, -0.5f, 0.0f);

    glBegin(GL_LINE_LOOP);

    glVertex3fv(Point(0.5f - relative_body_width * 0.5f, 0.0f, 0.0f).data());
    glVertex3fv(Point(0.5f + relative_body_width * 0.5f, 0.0f, 0.0f).data());
    glVertex3fv(Point(0.5f + relative_body_width * 0.5f, 1.0f - relative_head_length, 0.0f).data());
    glVertex3fv(Point(1.0f, 1.0f - relative_head_length, 0.0f).data());
    glVertex3fv(Point(0.5f, 1.0f, 0.0f).data());
    glVertex3fv(Point(0.0f, 1.0f - relative_head_length, 0.0f).data());
    glVertex3fv(Point(0.5f - relative_body_width * 0.5f, 1.0f - relative_head_length, 0.0f).data());

    glEnd();

    glPopMatrix();
}


template <class Point>
void draw_axis_aligned_plane(Point const& min, Point const& max, int const axis, float const scale = 1.0f)
{
    Point diff = (max - min);
    Point center = (max + min) * 0.5f;

    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glScalef(diff[0] * scale, diff[1] * scale, diff[2] * scale);

    glTranslatef(-0.5f, -0.5f, -0.5f);

    glBegin(GL_QUADS);

    if (axis == 0)
    {
        glNormal3f(1.0f, 0.0f,0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);
    }
    else if (axis == 1)
    {
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);
    }
    else if (axis == 2)
    {
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
    }

    glEnd();

    glPopMatrix();
}


template <class Point>
void draw_box_from_center(Point const& center, Point const& edge_lengths, float const scale = 1.0f, bool const draw_contour = false)
{
    glPushMatrix();

    glTranslatef(center[0], center[1], center[2]);
    glScalef(edge_lengths[0] * scale, edge_lengths[1] * scale, edge_lengths[2] * scale);

    glTranslatef(-0.5f, -0.5f, -0.5f);

    if (draw_contour)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBegin(GL_QUADS);

    glNormal3f(0.0f,-1.0f,0.0f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(0.0f,0.0f,1.0f);
    glVertex3f(1.0f,0.0f,1.0f);
    glVertex3f(1.0f,0.0f,0.0f);

    glNormal3f(0.0f,0.0f,-1.0f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(1.0f,0.0f,0.0f);
    glVertex3f(1.0f,1.0f,0.0f);
    glVertex3f(0.0f,1.0f,0.0f);

    glNormal3f(0.0f,1.0f,0.0f);
    glVertex3f(1.0f,1.0f,0.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(0.0f,1.0f,1.0f);
    glVertex3f(0.0f,1.0f,0.0f);

    glNormal3f(0.0f,0.0f,1.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(1.0f,0.0f,1.0f);
    glVertex3f(0.0f,0.0f,1.0f);
    glVertex3f(0.0f,1.0f,1.0f);

    glNormal3f(-1.0f,0.0f,0.0f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(0.0f,1.0f,0.0f);
    glVertex3f(0.0f,1.0f,1.0f);
    glVertex3f(0.0f,0.0f,1.0f);

    glNormal3f(1.0f,0.0f,0.0f);
    glVertex3f(1.0f,0.0f,0.0f);
    glVertex3f(1.0f,0.0f,1.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(1.0f,1.0f,0.0f);

    glEnd();

    glPopMatrix();
}


template <class Point>
void draw_box(Point const& min, Point const& max, float const scale = 1.0f, bool const draw_contour = false)
{
    Point diff = (max - min);
    Point center = (max + min) * 0.5f;

    draw_box_from_center(center, diff, scale, draw_contour);
}



template <class Mesh>
void draw_mesh_immediate(Mesh const& mesh)
{
    if (mesh.vertices_empty())
        return;

    typename Mesh::ConstFaceIter fIt(mesh.faces_begin()), fEnd(mesh.faces_end());

    glBegin(GL_TRIANGLES);
    for (; fIt!=fEnd; ++fIt) {
        typename Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());

        // glColor3ubv(mesh.color(fIt.handle()).data());
//        glColor3f(1.0f, 1.0f, 1.0f);

//        glNormal3fv(mesh.normal(fIt.handle()).data());

        if (mesh.has_vertex_normals())
        {
            glNormal3fv(mesh.normal(fvIt.handle()).data());
        }
        if (mesh.has_vertex_texcoords2D())
        {
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        }
        glVertex3fv(mesh.point(fvIt.handle()).data());

        ++fvIt;
        if (mesh.has_vertex_normals())
        {
            glNormal3fv(mesh.normal(fvIt.handle()).data());
        }
        if (mesh.has_vertex_texcoords2D())
        {
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        }
        glVertex3fv(mesh.point(fvIt.handle()).data());

        ++fvIt;
        if (mesh.has_vertex_normals())
        {
            glNormal3fv(mesh.normal(fvIt.handle()).data());
        }
        if (mesh.has_vertex_texcoords2D())
        {
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
        }
        glVertex3fv(mesh.point(fvIt.handle()).data());
    }
    glEnd();
}


template <class Mesh>
void draw_mesh(Mesh const& mesh)
{
    if (mesh.vertices_empty())
        return;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, mesh.points());

    if (mesh.has_vertex_normals())
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, mesh.vertex_normals());
    }

    if (mesh.has_vertex_colors())
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_UNSIGNED_BYTE, 0, mesh.vertex_colors());
    }

    if (mesh.has_vertex_texcoords2D())
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, mesh.texcoords2D());
    }

    typename Mesh::ConstFaceIter
            fIt(mesh.faces_begin()),
            fEnd(mesh.faces_end());

    glBegin(GL_TRIANGLES);
    for (; fIt != fEnd; ++fIt)
    {
        typename Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(*fIt);
        glArrayElement(fvIt->idx());
        ++fvIt;
        glArrayElement(fvIt->idx());
        ++fvIt;
        glArrayElement(fvIt->idx());
    }
    glEnd();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


template <class Mesh>
void draw_mesh_geometry_only(Mesh const& mesh)
{
    if (mesh.vertices_empty())
        return;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, mesh.points());

    typename Mesh::ConstFaceIter
            fIt(mesh.faces_begin()),
            fEnd(mesh.faces_end());

    glBegin(GL_TRIANGLES);
    for (; fIt != fEnd; ++fIt)
    {
        typename Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());
        glArrayElement(fvIt.handle().idx());
        ++fvIt;
        glArrayElement(fvIt.handle().idx());
        ++fvIt;
        glArrayElement(fvIt.handle().idx());
    }
    glEnd();

    glDisableClientState(GL_VERTEX_ARRAY);
}

template <typename MyMesh>
void draw_wireframe(MyMesh const& mesh)
{
    if (mesh.vertices_empty()) return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_mesh(mesh);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


inline void draw_cube_grid(int res)
{
    int res2 = res / 2;

    glPushMatrix();

    glScalef(1.0f / float(res2), 1.0f / float(res2), 1.0f / float(res2));

    glDisable(GL_LIGHTING);

    glBegin(GL_LINES);

    for (int y = -res2; y <= res2; ++y)
    {
        glVertex3f(res2, y, -res2);
        glVertex3f(res2, y,  res2);

        glVertex3f(-res2, -y, -res2);
        glVertex3f(-res2, -y,  res2);

        glVertex3f(-res2, -y, res2);
        glVertex3f( res2, -y, res2);

        glVertex3f(-res2, y, -res2);
        glVertex3f( res2, y, -res2);
    }

    for (int x = -res2; x <= res2; ++x)
    {
        glVertex3f( x,  res2, -res2);
        glVertex3f( x,  res2,  res2);

        glVertex3f(-x, -res2, -res2);
        glVertex3f(-x, -res2,  res2);

        glVertex3f( x, -res2,  res2);
        glVertex3f( x,  res2,  res2);

        glVertex3f(-x, -res2, -res2);
        glVertex3f(-x,  res2, -res2);
    }

    for (int z = -res2; z <= res2; ++z)
    {
        glVertex3f(res2, -res2, z);
        glVertex3f(res2,  res2, z);

        glVertex3f(-res2, -res2, -z);
        glVertex3f(-res2,  res2, -z);

        glVertex3f(-res2, -res2, -z);
        glVertex3f( res2, -res2, -z);

        glVertex3f(-res2, res2, z);
        glVertex3f( res2, res2, z);
    }

    glEnd();

    glPopMatrix();
}

template <class Point>
void draw_line(Point const& p1, Point const& p2)
{
    glBegin(GL_LINES);

    glVertex3fv(p1.data());
    glVertex3fv(p2.data());

    glEnd();
}


template <class Point>
void draw_line(Point const& start, Point const& dir, float const length)
{
    glBegin(GL_LINES);

    glVertex3fv(start.data());
    glVertex3fv((start + dir * length).data());

    glEnd();
}
