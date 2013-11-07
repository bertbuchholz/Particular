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

    ~Editor_renderer();

    void init(QGLContext const* context, QSize const& size) override;
    void resize(QSize const& size) override;
    float get_brownian_strength(Eigen::Vector3f const& pos, std::vector<Brownian_element*> const& elements, float const general_temperature) const;

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f);
    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f);
    void draw_temperature_mesh(MyMesh const& mesh, Level_data const& level_data, GLuint const bg_texture, QSize const& screen_size, const float time);
    void draw_temperature(Level_data const& level_data) const;
    void draw_level_elements(Level_data const& level_data) const;
    void draw_particle_systems(Level_data const& level_data) const;
    void draw_elements_ui(Level_data const& level_data) const;
    void draw_backdrop_quad() const;

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
    GLuint _depth_texture;

    std::unique_ptr<QGLShaderProgram> _molecule_program;
    std::unique_ptr<QGLShaderProgram> _temperature_program;
    std::unique_ptr<QGLShaderProgram> _screen_quad_program;
    std::unique_ptr<QGLShaderProgram> _post_program;
    std::unique_ptr<QGLShaderProgram> _blur_program;

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

    ~Shader_renderer();

    void init(QGLContext const* context, QSize const& size) override;
    void resize(QSize const& size) override;
    void update(Level_data const& level_data) override;

    float get_brownian_strength(Eigen::Vector3f const& pos, std::vector<Brownian_element*> const& elements, float const general_temperature) const;

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f);
    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f);
    void draw_temperature_mesh(MyMesh const& mesh, Level_data const& level_data, GLuint const bg_texture, QSize const& screen_size, const float time);
    void draw_temperature(Level_data const& level_data) const;
    void draw_level_elements(Level_data const& level_data) const;
    void draw_particle_systems(Level_data const& level_data) const;
    void draw_elements_ui(Level_data const& level_data) const;
    void draw_backdrop_quad() const;

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
    GLuint _depth_texture;

    std::unique_ptr<QGLShaderProgram> _molecule_program;
    std::unique_ptr<QGLShaderProgram> _temperature_program;
    std::unique_ptr<QGLShaderProgram> _screen_quad_program;
    std::unique_ptr<QGLShaderProgram> _post_program;
    std::unique_ptr<QGLShaderProgram> _blur_program;

    std::unique_ptr<QGLFramebufferObject> _scene_fbo;
    std::unique_ptr<QGLFramebufferObject> _post_fbo;
    std::unique_ptr<QGLFramebufferObject> _temperature_fbo;

    Level_element_draw_visitor _level_element_draw_visitor;
    Level_element_ui_draw_visitor _level_element_ui_draw_visitor;
};

REGISTER_CLASS_WITH_PARAMETERS(World_renderer, Shader_renderer);

void setup_gl_points(bool const distance_dependent);

#endif // RENDERER_H
