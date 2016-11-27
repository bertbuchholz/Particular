#include "Options_viewer.h"

#include "Geometry_utils.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

void Options_viewer::draw_decal(const Decal &d)
{
    _screen_label_program->bind();

    QVector2D aspect(1.0f, 1.0f);
    if (d._aspect == Decal::Keep_height)
    {
        aspect.setX(1.0f / camera()->aspectRatio());
    }
    else if (d._aspect == Decal::Keep_width)
    {
        aspect.setY(camera()->aspectRatio());
    }

    _screen_label_program->setUniformValue("fb_aspect", aspect);
    _screen_label_program->setUniformValue("size", Eigen2QVec(d._size));
    _screen_label_program->setUniformValue("position", Eigen2QVec(d._screen_pos));
    _screen_label_program->setUniformValue("tint", Eigen2QVec(d._tint));
    _screen_label_program->setUniformValue("bg_alpha", d._bg_alpha);

    _screen_label_program->setUniformValue("tex", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d._texture_id);

    _screen_quad.draw();

    _screen_label_program->release();
}

void Options_viewer::postDraw()
{
    Base::postDraw();

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    if (_draw_info_labels)
    {
        for (auto const& l : _info_labels)
        {
            if (!l._text.empty())
            {
                draw_decal(l);
            }
        }
    }
}


void Options_viewer::init()
{
    const QString screen_label_vert = GLSL(330,
    uniform vec2 size;
    uniform vec2 position;
    uniform vec2 fb_aspect;

    layout(location = 0) in vec2 in_vertex_pos;
    layout(location = 2) in vec2 in_uv_coord;

    out vec2 uv_coord;

    void main(void)
    {
        gl_Position = vec4(in_vertex_pos.xy * size.xy * fb_aspect.xy + position.xy, 0.0, 1.0);
        //    gl_Position = vec4(in_vertex_pos.xy * size.xy + position.xy, 0.0, 1.0);
        gl_Position.xy = gl_Position.xy * 2.0 - vec2(1.0);
        uv_coord = in_uv_coord;
    });

    const QString screen_label_frag = GLSL(330,
    uniform sampler2D tex;
    uniform vec4 tint;
    uniform float bg_alpha;

    in vec2 uv_coord;

    out vec4 frag_color;

    void main(void)
    {
        frag_color = texture(tex, uv_coord) * tint;
        frag_color.a = max(frag_color.a, bg_alpha);
//        frag_color = vec4(uv_coord, 0.0, 1.0);
    });

    _screen_label_program = std::unique_ptr<QOpenGLShaderProgram>(init_program_from_code(screen_label_vert, screen_label_frag));

    MyMesh mesh;

    MyMesh::VertexHandle vhandle[4];

    mesh.request_vertex_texcoords2D();

    vhandle[0] = mesh.add_vertex(MyMesh::Point(0, 0,  0));
    vhandle[1] = mesh.add_vertex(MyMesh::Point(1, 0,  0));
    vhandle[2] = mesh.add_vertex(MyMesh::Point(1, 1,  0));
    vhandle[3] = mesh.add_vertex(MyMesh::Point(0, 1,  0));

    mesh.set_texcoord2D(vhandle[0], MyMesh::TexCoord2D(0, 0));
    mesh.set_texcoord2D(vhandle[1], MyMesh::TexCoord2D(1, 0));
    mesh.set_texcoord2D(vhandle[2], MyMesh::TexCoord2D(1, 1));
    mesh.set_texcoord2D(vhandle[3], MyMesh::TexCoord2D(0, 1));

    std::vector<MyMesh::VertexHandle>  face_vhandles;
    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[3]);
    MyMesh::FaceHandle fh = mesh.add_face(face_vhandles);

    mesh.request_face_normals();
    mesh.request_vertex_normals();

    mesh.set_normal(fh, MyMesh::Normal(0, 0, 1));

    mesh.triangulate();

    mesh.update_face_normals();
    mesh.update_normals();

    _screen_quad.init(mesh);
}
