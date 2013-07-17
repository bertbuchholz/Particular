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

class Molecule_renderer
{
public:
    ~Molecule_renderer() {}

    virtual void init(QGLContext const* /* context */, QSize const& /* size */) {}

    virtual void resize(QSize const& /* size */) {}

//    virtual void render(std::vector<Molecule> const& molecules, StandardCamera const* = nullptr) const = 0;
    virtual void render(Level_data const& level_data, float const time, StandardCamera const* = nullptr) const = 0;

    virtual void set_parameters(Parameter_list const& /* parameters */)
    { }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }
};

class Stick_renderer : Molecule_renderer
{
public:
    void draw_molecule(Molecule const& m, float const alpha = 1.0f) const
    {
        glColor4f(1.0f, 1.0f, 1.0f, alpha);

        for (int i = 0; i < int(m._connectivity.size()); ++i)
        {
            std::vector<int> connections = m._connectivity[i];
            Atom const& connector = m._atoms[i];

            for (int const connected_index : connections)
            {
                Atom const& connected = m._atoms[connected_index];

                draw_line(connector._r, connected._r);
            }
        }
    }

    void render(Level_data const& level_data, float const /* time */, StandardCamera const* = nullptr) const override
    {
        std::vector<Molecule> const& molecules = level_data._molecules;

        glDisable(GL_LIGHTING);

        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);

        for (Molecule const& molecule : molecules)
        {
            if (!molecule._active) continue;

            draw_molecule(molecule);
        }
    }

    static Parameter_list get_parameters()
    {
        Parameter_list parameters;
        return parameters;
    }

    static std::string name()
    {
        return "Stick Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Stick_renderer;
    }
};


REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Stick_renderer);

class Ball_renderer : Molecule_renderer
{
public:
    Ball_renderer()
    {
        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);
    }

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f) const
    {
        glPushMatrix();

        float radius = scale * atom._radius;

        if (atom._type == Atom::Type::H)
        {
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
        }
        else if (atom._type == Atom::Type::O)
        {
            glColor4f(0.9f, 0.2f, 0.2f, alpha);
        }
        else if (atom._type == Atom::Type::C)
        {
            glColor4f(0.3f, 0.3f, 0.4f, alpha);
        }
        else if (atom._type == Atom::Type::S)
        {
            glColor4f(0.8f, 0.7f, 0.2f, alpha);
        }
        else if (atom._type == Atom::Type::N)
        {
            glColor4f(0.8f, 0.3f, 0.8f, alpha);
        }
        else if (atom._type == Atom::Type::Na)
        {
            glColor4f(0.8f, 0.3f, 0.8f, alpha);
        }
        else if (atom._type == Atom::Type::Cl)
        {
            glColor4f(0.5f, 0.8f, 0.3f, alpha);
        }
        else if (atom._type == Atom::Type::Charge)
        {
            glColor4f(0.3f, 0.2f, 0.7f, alpha);
            radius = 0.3f;
        }

        glTranslatef(atom._r[0], atom._r[1], atom._r[2]);

        glScalef(radius, radius, radius);

        _icosphere.draw();

        glPopMatrix();
    }

    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f) const
    {
        for (Atom const& atom : molecule._atoms)
        {
            draw_atom(atom, scale, alpha);
        }
    }

    void render(Level_data const& level_data, float const /* time */, StandardCamera const* = nullptr) const override
    {
        std::vector<Molecule> const& molecules = level_data._molecules;

        glEnable(GL_LIGHTING);

        for (Molecule const& molecule : molecules)
        {
            if (!molecule._active) continue;

            draw_molecule(molecule, _scale);
        }
    }

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
        return "Ball Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Ball_renderer;
    }

private:
    IcoSphere<OpenMesh::Vec3f, Color> _icosphere;

    float _scale;
};

REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Ball_renderer);


class Distance_renderer : Molecule_renderer
{
public:
    void render(Level_data const& level_data, float const /* time */, StandardCamera const* camera) const override
    {
        std::vector<Molecule> const& molecules = level_data._molecules;

        glLineWidth(2.0f);

        std::vector< std::pair<float, int> > distance_indices;

        for (size_t i = 0; i < molecules.size(); ++i)
        {
            if (!molecules[i]._active) continue;

            float const distance = (QGLV2Eigen(camera->position()) - molecules[i]._x).norm();
            distance_indices.push_back(std::pair<float, int>(distance, i));
        }

        std::sort(distance_indices.begin(), distance_indices.end());
        std::reverse(distance_indices.begin(), distance_indices.end());

        //for (Molecule const& molecule : molecules)
        for (std::pair<float, int> const& distance_index : distance_indices)
        {
            Molecule const& molecule = molecules[distance_index.second];
            float const distance = distance_index.first;

//            float const distance = (QGLV2Eigen(camera->position()) - molecule._x).norm();

//            float const normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());

            float normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());
            normalized_distance = std::abs(normalized_distance * 2.0f - 1.0f);

            float const alpha_stick = wendland_2_1(normalized_distance);

//            normalized_distance = (distance - camera->zNear()) / (camera->zFar() - camera->zNear());
//            normalized_distance = (normalized_distance - 0.4f) * 5.0f;
//            normalized_distance = std::abs(normalized_distance * 2.0f - 1.0f);

            float alpha_ball = wendland_2_1(into_range(normalized_distance * 7.0f, 0.0f, 1.0f));

            if (alpha_stick > 0.0f && alpha_stick <= 1.0f)
            {
                glDisable(GL_LIGHTING);
                stick_renderer.draw_molecule(molecule, alpha_stick);
            }

            if (alpha_ball > 0.0f && alpha_ball <= 1.0f)
            {
                glEnable(GL_LIGHTING);
                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
            }

//            if (normalized_distance > 0.0f && normalized_distance < 0.25f)
//            {
//                float const alpha = normalized_distance / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha);
//            }
//            else if (normalized_distance >= 0.25f && normalized_distance < 0.5f)
//            {
//                float const alpha_ball  =        (normalized_distance - 0.25f) / 0.25f;
//                float const alpha_stick = 1.0f - (normalized_distance - 0.25f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
//            }
//            else if (normalized_distance >= 0.5f && normalized_distance < 0.75f)
//            {
//                float const alpha_ball  = 1.0f - (normalized_distance - 0.5f) / 0.25f;
//                float const alpha_stick =        (normalized_distance - 0.5f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//                ball_renderer.draw_molecule(molecule, _scale, alpha_ball);
//            }
//            else if (normalized_distance >= 0.75f && normalized_distance < 1.0f)
//            {
//                float const alpha_stick = 1.0f - (normalized_distance - 0.75f) / 0.25f;
//                stick_renderer.draw_molecule(molecule, alpha_stick);
//            }
        }
    }

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
        return "Distance Renderer";
    }

    static Molecule_renderer * create()
    {
        return new Distance_renderer;
    }

private:
    float _scale;

    Ball_renderer ball_renderer;
    Stick_renderer stick_renderer;
};

REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Distance_renderer);


class Shader_renderer : Molecule_renderer
{
public:
    Shader_renderer()
    {
        _icosphere = IcoSphere<OpenMesh::Vec3f, Color>(2);
    }

    void init(QGLContext const* context, QSize const& size) override
    {
        _molecule_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/simple.vert", "data/shaders/molecule.frag"));
        _temperature_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/temperature.vert", "data/shaders/temperature.frag"));
        _screen_quad_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/fullscreen_square.vert", "data/shaders/simple_texture.frag"));
        _post_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/fullscreen_square.vert", "data/shaders/post.frag"));
        _blur_program = std::unique_ptr<QGLShaderProgram>(init_program(context, "data/shaders/fullscreen_square.vert", "data/shaders/blur_1D.frag"));

        _sphere_mesh = load_mesh<MyMesh>("data/meshes/icosphere_3.obj");
        _grid_mesh = load_mesh<MyMesh>("data/meshes/grid_10x10.obj");

        typename MyMesh::ConstVertexIter vIt(_grid_mesh.vertices_begin()), vEnd(_grid_mesh.vertices_end());

        for (; vIt!=vEnd; ++vIt)
        {
            MyMesh::Point const& v = _grid_mesh.point(vIt.handle());

            MyMesh::TexCoord2D t(v[0] * 0.5f + 0.5f, v[2] * 0.5f + 0.5f);
            _grid_mesh.set_texcoord2D(vIt.handle(), t);
        }

        Frame_buffer<Color> ice_tex_fb = convert<QColor_to_Color_converter, Color>(QImage("data/textures/ice_texture.jpg"));
        _ice_texture = create_texture(ice_tex_fb);

        Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(QImage("data/textures/iss_interior_1.jpg"));
        _backdrop_texture = create_texture(backdrop_tex_fb);

        resize(size);

        _level_element_draw_visitor.init(context);
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
    }

    void draw_atom(Atom const& atom, float const scale, float const alpha = 1.0f) const
    {
        float radius = scale * atom._radius;

        Color4 color(Atom::atom_colors[int(atom._type)], alpha);

        if (atom._type == Atom::Type::Charge)
        {
            radius = 0.3f;
        }

        glm::mat4x4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, glm::vec3(atom._r[0], atom._r[1], atom._r[2]));
        model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, radius));

        glUniformMatrix4fv(_molecule_program->uniformLocation("m_model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

        glUniform4fv(_molecule_program->uniformLocation("color"), 1, color.data());

        draw_mesh(_sphere_mesh);
    }

    void draw_molecule(Molecule const& molecule, float const scale, float const alpha = 1.0f) const
    {
        for (Atom const& atom : molecule._atoms)
        {
            draw_atom(atom, scale, alpha);
        }
    }

    float get_brownian_strength(Eigen::Vector3f const& pos, std::vector<Brownian_element*> const& elements) const
    {
        float factor = 0.0f;

        for (Brownian_element const* element : elements)
        {
            factor += element->get_brownian_motion_factor(pos);
        }

        float const max_strength = 50.0f;

        float const strength = into_range(factor / max_strength, -1.0f, 1.0f) * 0.5f + 0.5f;

        return strength;
    }

    void draw_temperature_mesh(MyMesh const& mesh, Level_data const& level_data, GLuint const bg_texture, QSize const& screen_size, const float time) const
    {
        if (level_data._game_field_borders.size() != 6)
        {
            std::cout << __PRETTY_FUNCTION__ << " no game field borders or too many/few, not drawing temperature mesh" << std::endl;
            return;
        }

        _temperature_program->bind();

        _temperature_program->setUniformValue("ice_texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _ice_texture);

        _temperature_program->setUniformValue("scene_texture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bg_texture);

        _temperature_program->setUniformValue("screen_size", screen_size);
        _temperature_program->setUniformValue("time", time);

        glColor3f(0.5f, 0.5f, 0.5f);
        draw_backdrop_quad();

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
            glColor3fv(Color(get_brownian_strength(p, elements)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements)).data());
            glVertex3fv(p.data());
            ++fvIt;
            p = OM2Eigen(mesh.point(fvIt.handle()));
            p = p.cwiseProduct(extent) + front_face->get_position();
//            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glTexCoord2fv(mesh.texcoord2D(fvIt.handle()).data());
            glColor3fv(Color(get_brownian_strength(p, elements)).data());
            glVertex3fv(p.data());
        }
        glEnd();

        _temperature_program->release();
    }

    void draw_temperature(Level_data const& level_data) const
    {
        auto front_face_iter = level_data._game_field_borders.find(Level_data::Plane::Neg_Y);

        assert(front_face_iter != level_data._game_field_borders.end());

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

                float const strength = get_brownian_strength(pos, level_data._brownian_elements);

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
        for (Level_element * element : level_data._level_elements)
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
        for (Level_element * element : level_data._level_elements)
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

    void render(Level_data const& level_data, float const time, StandardCamera const* camera) const override
    {
        QSize screen_size(camera->screenWidth(), camera->screenHeight());

        _scene_fbo->bind();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        assert(_scene_fbo->isValid());

        // draw the complete scene into an FB

        glEnable(GL_TEXTURE_2D);

//        glColor3f(1.0f, 0.0f, 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _backdrop_texture);
        draw_backdrop_quad();

        glDisable(GL_TEXTURE_2D);

        std::vector<Molecule> const& molecules = level_data._molecules;

        _molecule_program->bind();
        {
            GLdouble m_projection[16];
            glGetDoublev(GL_PROJECTION_MATRIX, m_projection);

            GLdouble m_view[16];
            glGetDoublev(GL_MODELVIEW_MATRIX, m_view);

            _molecule_program->setUniformValue("m_projection", QMatrix4x4(m_projection).transposed());
            _molecule_program->setUniformValue("m_view", QMatrix4x4(m_view).transposed());
            _molecule_program->setUniformValue("light_pos", QVector3D(-5.0f, 5.0f, 5.0f));
            _molecule_program->setUniformValue("camera_pos", QVector3D(camera->position()[0], camera->position()[1], camera->position()[2]));

            for (Molecule const& molecule : molecules)
            {
                if (!molecule._active) continue;

                draw_molecule(molecule, _scale);
            }
        }
        _molecule_program->release();

        draw_level_elements(level_data);

        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);

        draw_particle_systems(level_data);

        _scene_fbo->release();

        _post_fbo->bind();


        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[0], 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _blur_program->bind();
        _blur_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _scene_fbo->texture());
        _blur_program->setUniformValue("depth_texture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _depth_tex);
        _blur_program->setUniformValue("clip_distances", QVector2D(camera->zNear(), camera->zFar()));
        _blur_program->setUniformValue("tex_size", screen_size);
        _blur_program->setUniformValue("focus_distance", float(camera->position()[1]));
        _blur_program->setUniformValue("direction", QVector2D(1.0, 0.0));

        draw_quad_with_tex_coords();

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[1], 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _blur_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[0]);
        _blur_program->setUniformValue("direction", QVector2D(0.0, 1.0));
        draw_quad_with_tex_coords();

        _blur_program->release();

        _post_fbo->release();


//        draw_temperature(level_data);

        // distort/"freeze" the scene texture by using the temperature on the front game field plane

        _temperature_fbo->bind();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_temperature_mesh(_grid_mesh, level_data, _tmp_screen_texture[1], screen_size, time);

        _temperature_fbo->release();


        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

        glDisable(GL_DEPTH_TEST);
        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

        _screen_quad_program->bind();
        _screen_quad_program->setUniformValue("texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _temperature_fbo->texture());
//        glBindTexture(GL_TEXTURE_2D, _tmp_screen_texture[1]);
        draw_quad_with_tex_coords();
        _screen_quad_program->release();

        glPopAttrib();

        // put the depth buffer from the scene drawing onto the direct FB
        QGLFramebufferObject::blitFramebuffer(0, QRect(0, 0, screen_size.width(), screen_size.height()),
                                              _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                              GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);

        draw_elements_ui(level_data);
    }


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

    static Molecule_renderer * create()
    {
        return new Shader_renderer;
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

REGISTER_CLASS_WITH_PARAMETERS(Molecule_renderer, Shader_renderer);

#endif // RENDERER_H
