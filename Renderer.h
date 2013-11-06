#ifndef RENDERER_H
#define RENDERER_H

#include <QGLShaderProgram>
#include <QGLFramebufferObject>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Draw_functions.h>
#include <MyOpenMesh.h>
#include <GL_utilities.h>
#include <Icosphere.h>
#include <StandardCamera.h>
#include <Geometry_utils.h>
#include <Color_utilities.h>

#include "Level_data.h"
#include "Level_element_draw_visitor.h"
#include "Data_config.h"

class World_renderer
{
public:
    ~World_renderer() {}

    virtual void init(QGLContext const* /* context */, QSize const& /* size */) {}

    virtual void resize(QSize const& /* size */) {}

//    virtual void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const = 0;
    virtual void render(QGLFramebufferObject * main_fbo, Level_data const& level_data, float const time, qglviewer::Camera const* = nullptr) = 0;

    virtual void update(Level_data const& /* level_data */) {}

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }
};


class Editor_renderer : public World_renderer, public QGLFunctions
{
public:
    Editor_renderer()
    {
        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);
    }

    void init(QGLContext const* context, QSize const& size) override
    {
        initializeGLFunctions(context);

        _molecule_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                           Data_config::get_instance()->get_absolute_qfilename("shaders/simple.vert"),
                                                                           Data_config::get_instance()->get_absolute_qfilename("shaders/molecule.frag")));
        _temperature_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.frag")));
        _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/simple_texture.frag")));
        _post_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/post.frag")));
        _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/depth_blur_1D.frag")));

        _sphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/icosphere_3.obj"));
        _grid_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/grid_10x10.obj"));

        typename MyMesh::ConstVertexIter vIt(_grid_mesh.vertices_begin()), vEnd(_grid_mesh.vertices_end());

        for (; vIt!=vEnd; ++vIt)
        {
            MyMesh::Point const& v = _grid_mesh.point(vIt.handle());

            MyMesh::TexCoord2D t(v[0] * 0.5f + 0.5f, v[2] * 0.5f + 0.5f);
            _grid_mesh.set_texcoord2D(vIt.handle(), t);
        }

        Frame_buffer<Color> ice_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/ice_texture.png")));
        _ice_texture = create_texture(ice_tex_fb);

        Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/iss_interior_1.png")));
        _backdrop_texture = create_texture(backdrop_tex_fb);

        resize(size);

        _level_element_draw_visitor.init(context, size);
        _level_element_ui_draw_visitor.init(context);
    }

    void resize(QSize const& size) override
    {
//        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));


        glGenTextures(1, &_depth_tex); // FIXME: need to delete first
        glBindTexture(GL_TEXTURE_2D, _depth_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.width(), size.height(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // FIXME: need to delete first
        _tmp_screen_texture[0] = create_texture(size.width(), size.height());
        _tmp_screen_texture[1] = create_texture(size.width(), size.height());

        _scene_fbo->bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_tex, 0);
        _scene_fbo->release();

        _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
        _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

        _level_element_draw_visitor.resize(size);
    }

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f)
    {
        float radius = scale * atom._radius;

        Color4 color(Atom::atom_colors[int(atom._type)], alpha);

        if (atom._type == Atom::Type::Charge)
        {
            radius = 0.3f;
        }

        glm::mat4x4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, glm::vec3(atom.get_position()[0], atom.get_position()[1], atom.get_position()[2]));
        model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, radius));

        glUniformMatrix4fv(_molecule_program->uniformLocation("m_model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

        glUniform4fv(_molecule_program->uniformLocation("color"), 1, color.data());

        draw_mesh(_sphere_mesh);
    }

    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f)
    {
        for (Atom const& atom : molecule._atoms)
        {
            draw_atom(atom, scale, alpha);
        }
    }

    float get_brownian_strength(Eigen::Vector3f const& pos, std::vector<Brownian_element*> const& elements, float const general_temperature) const
    {
        float factor = general_temperature;

        for (Brownian_element const* element : elements)
        {
            factor += element->get_brownian_motion_factor(pos);
        }

        float const max_strength = 50.0f;

        float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

        return strength;
    }

    void draw_temperature_mesh(MyMesh const& mesh, Level_data const& level_data, GLuint const bg_texture, QSize const& screen_size, const float time)
    {
        if (level_data._game_field_borders.size() != 6)
        {
            std::cout << __PRETTY_FUNCTION__ << " no game field borders or too many/few, not drawing temperature mesh: " << level_data._game_field_borders.size() << std::endl;
            return;
        }

        float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

        glDisable(GL_DEPTH_TEST);

        _temperature_program->bind();

        _temperature_program->setUniformValue("ice_texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _ice_texture);

        _temperature_program->setUniformValue("scene_texture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bg_texture);

        _temperature_program->setUniformValue("screen_size", screen_size);
        _temperature_program->setUniformValue("time", time);

        std::vector<Brownian_element*> const& elements = level_data._brownian_elements;

        auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

        assert(front_face_iter != level_data._game_field_borders.end());

        Plane_barrier const* front_face = front_face_iter->second;
        Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

        typename MyMesh::ConstFaceIter fIt(mesh.faces_begin()), fEnd(mesh.faces_end());

        glBegin(GL_TRIANGLES);
        for (; fIt!=fEnd; ++fIt)
        {
            typename MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());

            Eigen::Vector3f p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
        }
        glEnd();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        _temperature_program->release();
    }

    void draw_temperature(Level_data const& level_data) const
    {
        auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

        assert(front_face_iter != level_data._game_field_borders.end());

        float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

        Plane_barrier const* front_face = front_face_iter->second;
        Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

        Eigen::Vector3f grid_start = front_face->get_position() - extent;
        Eigen::Vector3f grid_end = front_face->get_position() + extent;

        float resolution = 1.0f;

        std::cout << __PRETTY_FUNCTION__ << " " << grid_start << " " << grid_end << std::endl;

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
            {
                Eigen::Vector3f pos(x, front_face->get_position()[1], z);

                float const strength = get_brownian_strength(pos, level_data._brownian_elements, general_temperature);

                Color c(strength, 0.0f, 1.0f - strength);

                glColor3fv(c.data());

                glBegin(GL_POINTS);
                glVertex3fv(pos.data());
                glEnd();
            }
        }
    }

    void draw_level_elements(Level_data const& level_data) const
    {
        for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
        {
            element->accept(&_level_element_draw_visitor);
        }
    }

    void draw_particle_systems(Level_data const& level_data) const
    {
        for (Particle_system_element * element : level_data._particle_system_elements)
        {
            element->accept(&_level_element_draw_visitor);
        }
    }

    void draw_elements_ui(Level_data const& level_data) const
    {
        for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
        {
            element->accept(&_level_element_ui_draw_visitor);
        }
    }

    void draw_backdrop_quad() const
    {
        glPushMatrix();

        glTranslatef(0.0f, 300.0f, 0.0f);

        glScalef(300.0f, 1.0f, 300.0f);

        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        draw_quad_with_tex_coords();

        glPopMatrix();
    }

    void render(QGLFramebufferObject * main_fbo, Level_data const& level_data, float const time, qglviewer::Camera const* camera) override;


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
        return "Editor Renderer";
    }

    static World_renderer * create()
    {
        return new Editor_renderer;
    }

private:
    float _scale;

    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;
    MyMesh _sphere_mesh;
    MyMesh _grid_mesh;

    GLuint _ice_texture;
    GLuint _backdrop_texture;
    GLuint _tmp_screen_texture[2];


    std::unique_ptr<QGLShaderProgram> _molecule_program;
    std::unique_ptr<QGLShaderProgram> _temperature_program;
    std::unique_ptr<QGLShaderProgram> _screen_quad_program;
    std::unique_ptr<QGLShaderProgram> _post_program;
    std::unique_ptr<QGLShaderProgram> _blur_program;


    GLuint _depth_tex;

    std::unique_ptr<QGLFramebufferObject> _scene_fbo;
    std::unique_ptr<QGLFramebufferObject> _post_fbo;
    std::unique_ptr<QGLFramebufferObject> _temperature_fbo;

    Level_element_draw_visitor _level_element_draw_visitor;
    Level_element_ui_draw_visitor _level_element_ui_draw_visitor;
};

REGISTER_CLASS_WITH_PARAMETERS(World_renderer, Editor_renderer);



class Shader_renderer : public World_renderer, public QGLFunctions
{
public:
    Shader_renderer()
    {
        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);
    }

    void init(QGLContext const* context, QSize const& size) override
    {
        initializeGLFunctions(context);

        _molecule_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                           Data_config::get_instance()->get_absolute_qfilename("shaders/simple.vert"),
                                                                           Data_config::get_instance()->get_absolute_qfilename("shaders/molecule.frag")));
        _temperature_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.frag")));
        _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                              Data_config::get_instance()->get_absolute_qfilename("shaders/simple_texture.frag")));
        _post_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/post.frag")));
        _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(context,
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/fullscreen_square.vert"),
                                                                       Data_config::get_instance()->get_absolute_qfilename("shaders/depth_blur_1D.frag")));

        _sphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/icosphere_3.obj"));
        _grid_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/grid_10x10.obj"));
        _bg_hemisphere_mesh = load_mesh<MyMesh>(Data_config::get_instance()->get_absolute_filename("meshes/bg_hemisphere.obj"));

        typename MyMesh::ConstVertexIter vIt(_grid_mesh.vertices_begin()), vEnd(_grid_mesh.vertices_end());

        for (; vIt!=vEnd; ++vIt)
        {
            MyMesh::Point const& v = _grid_mesh.point(vIt.handle());

            MyMesh::TexCoord2D t(v[0] * 0.5f + 0.5f, v[2] * 0.5f + 0.5f);
            _grid_mesh.set_texcoord2D(vIt.handle(), t);
        }

        Frame_buffer<Color> ice_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/ice_texture.png")));
        _ice_texture = create_texture(ice_tex_fb);

        Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/iss_interior_1.png")));
        _backdrop_texture = create_texture(backdrop_tex_fb);

        Frame_buffer<Color> blurred_backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/iss_interior_1_blurred.png")));
        _blurred_backdrop_texture = create_texture(blurred_backdrop_tex_fb);

        Frame_buffer<Color> backdrop_grid_tex_fb = convert<QColor_to_Color_converter, Color>(QImage(Data_config::get_instance()->get_absolute_qfilename("/textures/background_grid.png")));
        _background_grid_texture = create_texture(backdrop_grid_tex_fb);

        resize(size);

        _level_element_draw_visitor.init(context, size);
        _level_element_ui_draw_visitor.init(context);
    }

    void resize(QSize const& size) override
    {
//        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));

        glGenTextures(1, &_depth_tex); // FIXME: need to delete first
        glBindTexture(GL_TEXTURE_2D, _depth_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.width(), size.height(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // FIXME: need to delete first
        _tmp_screen_texture[0] = create_texture(size.width(), size.height());
        _tmp_screen_texture[1] = create_texture(size.width(), size.height());

        _scene_fbo->bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_tex, 0);
        _scene_fbo->release();

        _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
        _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

        _level_element_draw_visitor.resize(size);
    }

    void update(Level_data const& level_data) override
    {
        glDeleteTextures(1, &_backdrop_texture);
        QImage background(Data_config::get_instance()->get_absolute_qfilename("textures/" + QString::fromStdString(level_data._background_name)));
        Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(background);
        _backdrop_texture = create_texture(backdrop_tex_fb);

        QImage blurred_background = background.scaled(background.size() * 0.05f);
        Frame_buffer<Color> blurred_backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(blurred_background);
        _blurred_backdrop_texture = create_texture(blurred_backdrop_tex_fb);
    }

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f)
    {
        float radius = scale * atom._radius;

        Color4 color(Atom::atom_colors[int(atom._type)], alpha);

        if (atom._type == Atom::Type::Charge)
        {
            radius = 0.3f;
        }

        glm::mat4x4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, glm::vec3(atom.get_position()[0], atom.get_position()[1], atom.get_position()[2]));
        model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, radius));

        glUniformMatrix4fv(_molecule_program->uniformLocation("m_model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

        glUniform4fv(_molecule_program->uniformLocation("color"), 1, color.data());

        draw_mesh(_sphere_mesh);
    }

    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f)
    {
        for (Atom const& atom : molecule._atoms)
        {
            draw_atom(atom, scale, alpha);
        }
    }

    float get_brownian_strength(Eigen::Vector3f const& pos, std::vector<Brownian_element*> const& elements, float const general_temperature) const
    {
        float factor = general_temperature;

        for (Brownian_element const* element : elements)
        {
            factor += element->get_brownian_motion_factor(pos);
        }

        float const max_strength = 50.0f;

        float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

        return strength;
    }

    void draw_temperature_mesh(MyMesh const& mesh, Level_data const& level_data, GLuint const bg_texture, QSize const& screen_size, const float time)
    {
        if (level_data._game_field_borders.size() != 6)
        {
            std::cout << __PRETTY_FUNCTION__ << " no game field borders or too many/few, not drawing temperature mesh" << std::endl;
            return;
        }

        float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

        glDisable(GL_DEPTH_TEST);

        _temperature_program->bind();

        _temperature_program->setUniformValue("ice_texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _ice_texture);

        _temperature_program->setUniformValue("scene_texture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bg_texture);

        _temperature_program->setUniformValue("screen_size", screen_size);
        _temperature_program->setUniformValue("time", time);

//        glColor3f(0.5f, 0.5f, 0.5f);
//        draw_backdrop_quad();

        std::vector<Brownian_element*> const& elements = level_data._brownian_elements;

        auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

        assert(front_face_iter != level_data._game_field_borders.end());

        Plane_barrier const* front_face = front_face_iter->second;
        Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

        typename MyMesh::ConstFaceIter fIt(mesh.faces_begin()), fEnd(mesh.faces_end());

        glBegin(GL_TRIANGLES);
        for (; fIt!=fEnd; ++fIt)
        {
            typename MyMesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(fIt.handle());

            Eigen::Vector3f p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements, general_temperature)).data());
            glVertex3fv(p.data());
        }
        glEnd();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        _temperature_program->release();
    }

    void draw_temperature(Level_data const& level_data) const
    {
        auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

        assert(front_face_iter != level_data._game_field_borders.end());

        float const general_temperature = 0.5f * (level_data._translation_fluctuation + level_data._rotation_fluctuation);

        Plane_barrier const* front_face = front_face_iter->second;
        Eigen::Vector3f extent(front_face->get_extent().get()[0] * 0.5f, 0.0f, front_face->get_extent().get()[1] * 0.5f);

        Eigen::Vector3f grid_start = front_face->get_position() - extent;
        Eigen::Vector3f grid_end = front_face->get_position() + extent;

        float resolution = 1.0f;

        std::cout << __PRETTY_FUNCTION__ << " " << grid_start << " " << grid_end << std::endl;

        for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
        {
            for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
            {
                Eigen::Vector3f pos(x, front_face->get_position()[1], z);

                float const strength = get_brownian_strength(pos, level_data._brownian_elements, general_temperature);

                Color c(strength, 0.0f, 1.0f - strength);

                glColor3fv(c.data());

                glBegin(GL_POINTS);
                glVertex3fv(pos.data());
                glEnd();
            }
        }
    }

    void draw_level_elements(Level_data const& level_data) const
    {
        for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
        {
            element->accept(&_level_element_draw_visitor);
        }
    }

    void draw_particle_systems(Level_data const& level_data) const
    {
        for (Particle_system_element * element : level_data._particle_system_elements)
        {
            element->accept(&_level_element_draw_visitor);
        }
    }

    void draw_elements_ui(Level_data const& level_data) const
    {
        for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
        {
            element->accept(&_level_element_ui_draw_visitor);
        }
    }

    void draw_backdrop_quad() const
    {
        glPushMatrix();

        glTranslatef(0.0f, 300.0f, 0.0f);

        glScalef(300.0f, 1.0f, 300.0f);

        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        draw_quad_with_tex_coords();

        glPopMatrix();
    }

    void render(QGLFramebufferObject * main_fbo, Level_data const& level_data, float const time, qglviewer::Camera const* camera) override;


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
        return "Shader Renderer";
    }

    static World_renderer * create()
    {
        return new Shader_renderer;
    }

private:
    float _scale;

    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;
    MyMesh _sphere_mesh;
    MyMesh _grid_mesh;
    MyMesh _bg_hemisphere_mesh;

    GLuint _ice_texture;
    GLuint _backdrop_texture;
    GLuint _blurred_backdrop_texture;
    GLuint _background_grid_texture;
    GLuint _tmp_screen_texture[2];

    std::unique_ptr<QGLShaderProgram> _molecule_program;
    std::unique_ptr<QGLShaderProgram> _temperature_program;
    std::unique_ptr<QGLShaderProgram> _screen_quad_program;
    std::unique_ptr<QGLShaderProgram> _post_program;
    std::unique_ptr<QGLShaderProgram> _blur_program;


    GLuint _depth_tex;

    std::unique_ptr<QGLFramebufferObject> _scene_fbo;
    std::unique_ptr<QGLFramebufferObject> _post_fbo;
    std::unique_ptr<QGLFramebufferObject> _temperature_fbo;

    Level_element_draw_visitor _level_element_draw_visitor;
    Level_element_ui_draw_visitor _level_element_ui_draw_visitor;
};

REGISTER_CLASS_WITH_PARAMETERS(World_renderer, Shader_renderer);

void setup_gl_points(bool const distance_dependent);

#endif // RENDERER_H
