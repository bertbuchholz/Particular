#ifndef RENDERER_H
#define RENDERER_H

#include <QGLShaderProgram>
#include <QGLFramebufferObject>

#include <QGLFunctions>

#include <MyOpenMesh.h>
#include <Icosphere.h>

#include "Draggable.h"
#include "Level_data.h"
#include "Level_element_draw_visitor.h"
#include "GL_texture.h"

//void setup_gl_points(bool const distance_dependent);

class World_renderer : public QGLFunctions
{
public:
    virtual ~World_renderer() {}

    virtual void init(QGLContext const* context, QSize const& size);

    virtual void resize(QSize const& /* size */);

//    virtual void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const = 0;
    virtual void render(QGLFramebufferObject * /* main_fbo */, Level_data const& /* level_data */,
                        float const /* time */, qglviewer::Camera const* = nullptr) {}

    virtual void update(Level_data const& /* level_data */) {}

    void setup_gl_points(bool const distance_dependent) const;

    void draw_particle_system(Targeted_particle_system const& system, int const height) const;
    void draw_curved_particle_system(Curved_particle_system const& system, int const height) const;
    void draw_curved_particle_system_in_existing_coord_sys(Curved_particle_system const& system, int const height) const;


    void draw_textured_quad(const GLuint tex_id) const;

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }


protected:
    QGLContext const* _context;

    GL_functions _gl_functions;

    float _aspect_ratio;

    QSize _screen_size;
};


class Ui_renderer : public World_renderer
{
public:
    virtual void init(QGLContext const* context, QSize const& size);

    void draw_spinbox(Draggable_spinbox const& s, const bool for_picking, float const alpha = 1.0f) const;

    void generate_button_texture(Draggable_button *b) const;
    void generate_label_texture(Draggable_label *b, const int text_alignment = Qt::AlignCenter) const;
    void generate_statistics_texture(Draggable_statistics &b) const;
    Eigen::Vector2f generate_flowing_text_label(Draggable_label *label, float const text_width) const;
    Draggable_tooltip * generate_tooltip(Eigen::Vector3f const& screen_pos, Eigen::Vector3f const& element_extent, std::string const& text) const;

    void setup_fonts();

    QFont const& get_main_font() const { return _main_font; }


protected:
    std::vector<GLuint> _number_textures;

    GLuint _spinbox_arrowup_texture;
    GLuint _spinbox_arrowdown_texture;

    QFont _main_font;
};


class Editor_renderer : public World_renderer
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



class Shader_renderer : public World_renderer
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
    void draw_temperature_cube(const MyMesh &mesh, const Level_data &level_data, const GLuint bg_texture, const QSize &screen_size, const float time);
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
    MyMesh _cube_grid_mesh;
    MyMesh _bg_hemisphere_mesh;

    GLuint _ice_texture;
    GLuint _backdrop_texture;
    GLuint _blurred_backdrop_texture;
    GLuint _background_grid_texture;
    GLuint _depth_texture;
    GL_texture _tmp_screen_texture[2];

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

#endif // RENDERER_H
