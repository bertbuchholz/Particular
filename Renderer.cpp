#include <Renderer.h>


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

float get_scale(QSize const& b_size, QSize const& target_size)
{
    float const width_ratio = target_size.width() / float(b_size.width());
    float const height_ratio = target_size.height() / float(b_size.height());

    return std::min(width_ratio, height_ratio);
}

void draw_cube()
{
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, -1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -1.0f);
    glRotatef(180.0f, 0.0f, -1.0f, 0.0f);
    draw_quad_with_tex_coords(10.0f);
    glPopMatrix();
}



void World_renderer::init(QGLContext const* context, QSize const& size)
{
    initializeGLFunctions(context);

    _screen_size = size;

    _aspect_ratio = size.width() / float(size.height());
}

void World_renderer::setup_gl_points(bool const distance_dependent) const
{
    if (distance_dependent)
    {
        float quadratic[] =  { 0.0f, 0.0f, 0.001f };
        glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, quadratic);

        //        glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

        //        //Tell it the max and min sizes we can use using our pre-filled array.
    }
    else
    {
        float quadratic[] =  { 1.0f, 0.0f, 0.0f };
        glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, quadratic);
        //        glPointParameterf(GL_POINT_SIZE_MIN, 1.0f);
        //        glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);
        //        glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
        //        glPointSize(8.0f);

    }

    glDisable(GL_POINT_SPRITE);
    glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
    glPointParameterf(GL_POINT_SIZE_MIN, 2.0f);
    glPointParameterf(GL_POINT_SIZE_MAX, 32.0f);

    glEnable(GL_POINT_SMOOTH);
}

void World_renderer::draw_particle_system(Targeted_particle_system const& system, int const height) const
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

    for (Targeted_particle const& p : system.get_particles())
    {
        glPointSize(p.size_factor * 4.0f * height / (768.0f));
        glBegin(GL_POINTS);
        glColor4fv(p.color.data());
        glVertex3fv(p.position.data());
        glEnd();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void World_renderer::draw_textured_quad(const GLuint tex_id) const
{
    glBindTexture(GL_TEXTURE_2D, tex_id);
//    draw_quad_with_tex_coords();
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,   0.0f);
    glVertex2f  (-0.99f, -0.99f);
    glTexCoord2f( 0.99f,  0.0f);
    glVertex2f  ( 0.99f, -0.99f);
    glTexCoord2f( 0.99f,  0.99f);
    glVertex2f  ( 0.99f,  0.99f);
    glTexCoord2f( 0.0f,   0.99f);
    glVertex2f  (-0.99f,  0.99f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Ui_renderer::setup_fonts()
{
    int id = QFontDatabase::addApplicationFont(Data_config::get_instance()->get_absolute_qfilename("fonts/Matiz.ttf"));
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    _main_font = QFont(family);
}

void Ui_renderer::init(const QGLContext *context, const QSize &size)
{
    World_renderer::init(context, size);

    setup_fonts();

    for (int i = 0; i < 10; ++i)
    {
        QImage text_image(100, 100, QImage::Format_ARGB32);
        text_image.fill(QColor(0, 0, 0, 0));

        QPainter p;
        p.begin(&text_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QFont font = get_main_font();
        font.setPointSizeF(0.05f * float(size.width()));
//        font.setPointSizeF(40.0f);
        p.setFont(font);

        p.setPen(QColor(255, 255, 255));

        QRect text_bb;
        p.drawText(text_image.rect(), Qt::AlignCenter, QString("%1").arg(i), &text_bb);
        p.end();

//        text_image.save(QString("/tmp/text_image_%1.png").arg(i));

        QImage final_text_image(text_bb.size() + QSize(4, 4), QImage::Format_ARGB32);
        final_text_image.fill(QColor(0, 0, 0, 0));

        p.begin(&final_text_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        p.drawImage(2, 2, text_image, text_bb.left(), text_bb.top());
        p.end();

        Frame_buffer<Color4> number_tex_fb = convert<QRgb_to_Color4_converter, Color4>(final_text_image);
        _number_textures.push_back(create_texture(number_tex_fb));

//        final_text_image.save(QString("/tmp/number%1.png"));
    }

    Frame_buffer<Color4> arrowup_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/spinbox_arrowup.png")));
    _spinbox_arrowup_texture = create_texture(arrowup_tex_fb);

    Frame_buffer<Color4> arrowdown_tex_fb = convert<QRgb_to_Color4_converter, Color4>(QImage(Data_config::get_instance()->get_absolute_qfilename("textures/spinbox_arrowdown.png")));
    _spinbox_arrowdown_texture = create_texture(arrowdown_tex_fb);
}

void Ui_renderer::draw_spinbox(const Draggable_spinbox &s, const bool for_picking, const float alpha) const
{
    glPushMatrix();

    glColor4f(1.0f, 1.0f, 1.0f, s.get_alpha() * alpha);

    Eigen::Transform<float, 3, Eigen::Affine> const spinbox_system = s.get_transform();

    Eigen::Vector3f const number_pos = spinbox_system * Eigen::Vector3f::Zero();

    glPushMatrix();
    glTranslatef(number_pos[0], number_pos[1], number_pos[2]);
    glScalef(0.018f / _aspect_ratio, 0.018f, 1.0f); // draw a square
//    glColor3f(1.0f, 1.0f, 1.0f);
    draw_textured_quad(_number_textures[s.get_value()]);
    glPopMatrix();


    Eigen::Vector3f const decrement_pos = spinbox_system * s.get_decrement_draggable().get_position();

    glPushMatrix();
    glTranslatef(decrement_pos[0], decrement_pos[1], decrement_pos[2]);
    glScalef(0.01f, 0.01f * _aspect_ratio, 1.0f); // draw a square
//    glColor4f(1.0f, 1.0f, 0.0f, s.get_alpha() * alpha);

    if (for_picking)
    {
        draw_quad_with_tex_coords();
    }
    else
    {
//        draw_quad_with_tex_coords();
        draw_textured_quad(_spinbox_arrowdown_texture);
    }

    glPopMatrix();

    Eigen::Vector3f const increment_pos = spinbox_system * s.get_increment_draggable().get_position();

    glPushMatrix();
    glTranslatef(increment_pos[0], increment_pos[1], increment_pos[2]);
    glScalef(0.01f, 0.01f * _aspect_ratio, 1.0f); // draw a square
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
//    glColor4f(1.0f, 0.0f, 1.0f, s.get_alpha() * alpha);

    if (for_picking)
    {
        draw_quad_with_tex_coords();
    }
    else
    {
//        draw_quad_with_tex_coords();
        draw_textured_quad(_spinbox_arrowup_texture);
    }

    glPopMatrix();


    glPopMatrix();
}

void Ui_renderer::generate_button_texture(Draggable_button *b) const
{
    std::cout << __PRETTY_FUNCTION__ << " constructing button texture" << std::endl;

    QSize const pixel_size(_screen_size.width() * b->get_extent()[0], _screen_size.height() * b->get_extent()[1]);

    QImage img(pixel_size, QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));

    QPainter p;
    p.begin(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QPen pen;
    pen.setWidth(5);
    pen.setColor(QColor(20, 133, 204));
    p.setPen(pen);

    p.setBrush(QBrush(QColor(76, 153, 204, 120)));

    //    p.drawRoundedRect(QRect(5, 5, pixel_size.width() - 10, pixel_size.height() - 10), pixel_size.width() * 0.1f, pixel_size.width() * 0.1f);
    float const radius = std::min((pixel_size.width() - 10) * 0.5f, (pixel_size.height() - 10) * 0.5f);
    p.drawRoundedRect(QRect(5, 5, pixel_size.width() - 10, pixel_size.height() - 10), radius, radius);

    QFont font = _main_font;
    //        font.setWeight(QFont::Bold);
    font.setPointSizeF(10.0f);
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));

    QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.5f);

    QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

    font.setPointSizeF(10.0f * get_scale(b_size, text_size));
    p.setFont(font);

    p.drawText(img.rect(), Qt::AlignCenter, QString::fromStdString(b->get_text()));

    p.end();

    delete_texture(b->get_texture());
    Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(img);
    b->set_texture(create_texture(texture_fb));

//    deleteTexture(b->get_texture());
//    b->set_texture(bindTexture(img));
}

void Ui_renderer::generate_label_texture(Draggable_label *b) const
{
    std::cout << __PRETTY_FUNCTION__ << " constructing label texture" << std::endl;

    QSize const pixel_size(_screen_size.width() * b->get_extent()[0], _screen_size.height() * b->get_extent()[1]);

    QImage img(pixel_size, QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));

    QPainter p;
    p.begin(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QFont font = _main_font;
    //        font.setWeight(QFont::Bold);
    font.setPointSizeF(10.0f);
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));

    QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.8f);

    QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

    //        font.setPointSizeF(10.0f * text_size.height() / float(b_size.height())); // always use height ratio to ensure same size text for buttons of same height
    font.setPointSizeF(10.0f * get_scale(b_size, text_size));
    p.setFont(font);

    p.drawText(img.rect(), Qt::AlignCenter, QString::fromStdString(b->get_text()));

    p.end();

    delete_texture(b->get_texture());
    Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(img);
    b->set_texture(create_texture(texture_fb));

//    deleteTexture(b->get_texture());
//    b->set_texture(bindTexture(img));
}

void Ui_renderer::generate_statistics_texture(Draggable_statistics &b) const
{
    std::cout << __PRETTY_FUNCTION__ << " constructing statistic texture" << std::endl;

    QSize const pixel_size(_screen_size.width() * b.get_extent()[0], _screen_size.height() * b.get_extent()[1]);

    QImage img(pixel_size, QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));

    QPainter p;
    p.begin(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QPen pen;
    pen.setWidth(2);
    pen.setColor(QColor(20, 133, 204));
    p.setPen(pen);

    p.setBrush(QBrush(QColor(76, 153, 204, 120)));

    p.drawRoundedRect(QRect(2, 2, pixel_size.width() - 4, pixel_size.height() - 4), pixel_size.width() * 0.02f, pixel_size.width() * 0.02f);


    QFont font = _main_font;
    //        font.setWeight(QFont::Bold);
    font.setPixelSize(0.1f * pixel_size.height());
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));

    //        QSize text_size(pixel_size.width() * 0.8f, pixel_size.height() * 0.8f);

    //        QSize b_size = p.boundingRect(QRect(QPoint(0, 0), text_size), Qt::AlignCenter, QString::fromStdString(b->get_text())).size();

    //        font.setPointSizeF(10.0f * get_scale(b_size, text_size));
    //        p.setFont(font);

    p.drawText(0.2f * pixel_size.width(), 0.15f * pixel_size.height(), QString::fromStdString(b.get_text()));

    pen.setWidthF(0.5f);
    pen.setColor(QColor(255, 255, 255, 100));
    p.setPen(pen);

    p.drawLine(0.2f * pixel_size.width(), 0.8f * pixel_size.height(), 0.9f * pixel_size.width(), 0.8f * pixel_size.height());

    for (int i = 1; i < 5; ++i)
    {
        float const height = 0.8f - 0.6f * i / 4.0f;
        p.drawLine(0.2f * pixel_size.width(), height * pixel_size.height(), 0.9f * pixel_size.width(), height * pixel_size.height());
    }

    p.end();

    delete_texture(b.get_texture());
    Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(img);
    b.set_texture(create_texture(texture_fb));

//    deleteTexture(b.get_texture());
//    b.set_texture(bindTexture(img));
}

Draggable_tooltip *Ui_renderer::generate_tooltip(const Eigen::Vector3f &screen_pos, const Eigen::Vector3f &element_extent, const std::string &text) const
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    QSize const pixel_size(_screen_size.width() * 0.3f, _screen_size.height() * 0.5f);

    QImage text_image(pixel_size, QImage::Format_ARGB32);
    text_image.fill(QColor(0, 0, 0, 0));

    QPainter p(&text_image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QFont font = _main_font;
    font.setPointSizeF(0.02f * float(_screen_size.width()));
    p.setFont(font);

    p.setPen(QColor(255, 255, 255));

    // draw off of the edges of the image and cut out with padding to avoid UV imprecisions to cause texture artifacts (bad wrap arounds)
    QRect final_text_bb;
    QRect inset_text_bb = text_image.rect();
    inset_text_bb.setWidth(inset_text_bb.width() - 40);
    inset_text_bb.setHeight(inset_text_bb.height() - 40);
    inset_text_bb.setTopLeft({20, 20});
    p.drawText(inset_text_bb, Qt::AlignLeft | Qt::TextWordWrap, QString::fromStdString(text), &final_text_bb);
    p.end();

    final_text_bb.setWidth(final_text_bb.width() + 4);
    final_text_bb.setHeight(final_text_bb.height() + 4);
    final_text_bb.setLeft(final_text_bb.left() - 2);
    final_text_bb.setTop(final_text_bb.top() - 2);
    text_image = text_image.copy(final_text_bb);

    Eigen::Vector2f const uniform_bb(final_text_bb.width() / float(_screen_size.width()),
                                     final_text_bb.height() / float(_screen_size.height()));

    Eigen::Vector3f final_screen_pos = screen_pos;

    float const min_y_distance = 0.5f * uniform_bb[1] + 0.5f * element_extent[1] + 0.01f;

    if (screen_pos[1] - min_y_distance - 0.5f * uniform_bb[1] >= 0.0f) // tooltip fits below the object
    {
        final_screen_pos[1] = screen_pos[1] - min_y_distance;
    }
    else
    {
        final_screen_pos[1] = screen_pos[1] + min_y_distance;
    }

    final_screen_pos[0] = std::max(uniform_bb[0] * 0.5f + 0.05f, final_screen_pos[0]);
    final_screen_pos[0] = std::min(1.0f - (uniform_bb[0] * 0.5f + 0.05f), final_screen_pos[0]);

    Draggable_tooltip * tooltip = new Draggable_tooltip(final_screen_pos, uniform_bb, "");

//    tooltip->set_texture(bindTexture(text_image));

    Frame_buffer<Color4> texture_fb = convert<QRgb_to_Color4_converter, Color4>(text_image);
    tooltip->set_texture(create_texture(texture_fb));

    return tooltip;
}


Shader_renderer::~Shader_renderer()
{
    glDeleteBuffers(1, &_ice_texture);
    glDeleteBuffers(1, &_backdrop_texture);
    glDeleteBuffers(1, &_blurred_backdrop_texture);
    glDeleteBuffers(1, &_background_grid_texture);
    glDeleteBuffers(2, _tmp_screen_texture);
    glDeleteBuffers(1, &_depth_texture);
}

void Shader_renderer::init(const QGLContext *context, const QSize &size)
{
    World_renderer::init(context, size);

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

void Shader_renderer::resize(const QSize &size)
{
    //        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));

    glGenTextures(1, &_depth_texture); // FIXME: need to delete first
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
    _scene_fbo->release();

    _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

    _level_element_draw_visitor.resize(size);
}

void Shader_renderer::update(const Level_data &level_data)
{
    glDeleteTextures(1, &_backdrop_texture);
    QImage background(Data_config::get_instance()->get_absolute_qfilename("textures/" + QString::fromStdString(level_data._background_name)));
    Frame_buffer<Color> backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(background);
    _backdrop_texture = create_texture(backdrop_tex_fb);

    QImage blurred_background = background.scaled(background.size() * 0.05f);
    Frame_buffer<Color> blurred_backdrop_tex_fb = convert<QColor_to_Color_converter, Color>(blurred_background);
    _blurred_backdrop_texture = create_texture(blurred_backdrop_tex_fb);
}

void Shader_renderer::draw_atom(const Atom &atom, const float scale, const float alpha)
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

void Shader_renderer::draw_molecule(const Molecule &molecule, const float scale, const float alpha)
{
    for (Atom const& atom : molecule._atoms)
    {
        draw_atom(atom, scale, alpha);
    }
}

float Shader_renderer::get_brownian_strength(const Eigen::Vector3f &pos, const std::vector<Brownian_element *> &elements, const float general_temperature) const
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

void Shader_renderer::draw_temperature_mesh(const MyMesh &mesh, const Level_data &level_data, const GLuint bg_texture, const QSize &screen_size, const float time)
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

void Shader_renderer::draw_temperature(const Level_data &level_data) const
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

void Shader_renderer::draw_level_elements(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Shader_renderer::draw_particle_systems(const Level_data &level_data) const
{
    for (Particle_system_element * element : level_data._particle_system_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Shader_renderer::draw_elements_ui(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_ui_draw_visitor);
    }
}

void Shader_renderer::draw_backdrop_quad() const
{
    glPushMatrix();

    glTranslatef(0.0f, 300.0f, 0.0f);

    glScalef(300.0f, 1.0f, 300.0f);

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    draw_quad_with_tex_coords();

    glPopMatrix();
}

void Shader_renderer::render(QGLFramebufferObject *main_fbo, const Level_data &level_data, const float time, const qglviewer::Camera *camera)
{
    glEnable(GL_DEPTH_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    QSize screen_size(camera->screenWidth(), camera->screenHeight());

    _scene_fbo->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    assert(_scene_fbo->isValid());

    // draw the complete scene into an FB

    //    glDisable(GL_TEXTURE_2D);

    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);
    glDisable(GL_LIGHTING);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _background_grid_texture);
    //            draw_backdrop_quad();

    glPushMatrix();
    //    glTranslatef(0.0f, 200.0f, 0.0f);
    glScalef(200.0f, 200.0f, 200.0f);
    //            draw_mesh(_bg_hemisphere_mesh);
    draw_cube();
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    std::list<Molecule> const& molecules = level_data._molecules;

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
        _molecule_program->setUniformValue("bg_texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _blurred_backdrop_texture);

        for (Molecule const& molecule : molecules)
        {
            draw_molecule(molecule, _scale);
        }
    }
    _molecule_program->release();

    glDisable(GL_TEXTURE_2D);

    draw_level_elements(level_data);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    draw_particle_systems(level_data);

    _scene_fbo->release();

    _post_fbo->bind();


    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tmp_screen_texture[0], 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _blur_program->bind();
    _blur_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _scene_fbo->texture());
    _blur_program->setUniformValue("depth_texture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
    _blur_program->setUniformValue("clip_distances", QVector2D(camera->zNear(), camera->zFar()));
    _blur_program->setUniformValue("tex_size", screen_size);
//    _blur_program->setUniformValue("focus_distance", float(camera->position()[1]));
    _blur_program->setUniformValue("focus_distance", float(camera->position().norm()));
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


//    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
//                                          _post_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
//                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    return;


    //        draw_temperature(level_data);

    // distort/"freeze" the scene texture by using the temperature on the front game field plane

    _temperature_fbo->bind();


    QGLFramebufferObject::blitFramebuffer(_temperature_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _post_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_temperature_mesh(_grid_mesh, level_data, _tmp_screen_texture[1], screen_size, time);

    _temperature_fbo->release();

    main_fbo->bind();

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glDisable(GL_DEPTH_TEST);
    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    _screen_quad_program->bind();
    _screen_quad_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _temperature_fbo->texture());
    draw_quad_with_tex_coords();
    _screen_quad_program->release();

    glPopAttrib();

    // put the depth buffer from the scene drawing into the returned buffer to draw the ui elements correctly
    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    draw_elements_ui(level_data);

    glEnable(GL_DEPTH_TEST);

    main_fbo->release();
}


Editor_renderer::~Editor_renderer()
{
    glDeleteBuffers(1, &_ice_texture);
    glDeleteBuffers(1, &_backdrop_texture);
    glDeleteBuffers(2, _tmp_screen_texture);
    glDeleteBuffers(1, &_depth_texture);
}

void Editor_renderer::init(const QGLContext *context, const QSize &size)
{
    World_renderer::init(context, size);

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

void Editor_renderer::resize(const QSize &size)
{
    //        _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _scene_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size));


    glGenTextures(1, &_depth_texture); // FIXME: need to delete first
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
    _scene_fbo->release();

    _post_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));
    _temperature_fbo = std::unique_ptr<QGLFramebufferObject>(new QGLFramebufferObject(size, QGLFramebufferObject::Depth));

    _level_element_draw_visitor.resize(size);
}

void Editor_renderer::draw_atom(const Atom &atom, const float scale, const float alpha)
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

void Editor_renderer::draw_molecule(const Molecule &molecule, const float scale, const float alpha)
{
    for (Atom const& atom : molecule._atoms)
    {
        draw_atom(atom, scale, alpha);
    }
}

float Editor_renderer::get_brownian_strength(const Eigen::Vector3f &pos, const std::vector<Brownian_element *> &elements, const float general_temperature) const
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

void Editor_renderer::draw_temperature_mesh(const MyMesh &mesh, const Level_data &level_data, const GLuint bg_texture, const QSize &screen_size, const float time)
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

void Editor_renderer::draw_temperature(const Level_data &level_data) const
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

void Editor_renderer::draw_level_elements(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Editor_renderer::draw_particle_systems(const Level_data &level_data) const
{
    for (Particle_system_element * element : level_data._particle_system_elements)
    {
        element->accept(&_level_element_draw_visitor);
    }
}

void Editor_renderer::draw_elements_ui(const Level_data &level_data) const
{
    for (boost::shared_ptr<Level_element> const& element : level_data._level_elements)
    {
        element->accept(&_level_element_ui_draw_visitor);
    }
}

void Editor_renderer::draw_backdrop_quad() const
{
    glPushMatrix();

    glTranslatef(0.0f, 300.0f, 0.0f);

    glScalef(300.0f, 1.0f, 300.0f);

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    draw_quad_with_tex_coords();

    glPopMatrix();
}

void Editor_renderer::render(QGLFramebufferObject *main_fbo, const Level_data &level_data, const float time, const qglviewer::Camera *camera)
{
    glEnable(GL_DEPTH_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    QSize screen_size(camera->screenWidth(), camera->screenHeight());

    _scene_fbo->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    assert(_scene_fbo->isValid());

    glDisable(GL_TEXTURE_2D);

    //        glDisable(GL_LIGHTING);

    //        glColor3f(1.0f, 1.0f, 1.0f);

    //        draw_backdrop_quad();

    //        glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_LIGHTING);

    std::list<Molecule> const& molecules = level_data._molecules;

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
            draw_molecule(molecule, _scale);
        }
    }
    _molecule_program->release();

    draw_level_elements(level_data);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    draw_particle_systems(level_data);

    _scene_fbo->release();

    // distort/"freeze" the scene texture by using the temperature on the front game field plane

    _temperature_fbo->bind();

    QGLFramebufferObject::blitFramebuffer(_temperature_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_temperature_mesh(_grid_mesh, level_data, _scene_fbo->texture(), screen_size, time);

    _temperature_fbo->release();

    main_fbo->bind();

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glDisable(GL_DEPTH_TEST);
    //        glViewport(0.0f, 0.0f, camera->screenWidth(), camera->screenHeight());

    _screen_quad_program->bind();
    _screen_quad_program->setUniformValue("texture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _temperature_fbo->texture());
    draw_quad_with_tex_coords();
    _screen_quad_program->release();

    glPopAttrib();

    // put the depth buffer from the scene drawing into the returned buffer to draw the ui elements correctly
    QGLFramebufferObject::blitFramebuffer(main_fbo, QRect(0, 0, screen_size.width(), screen_size.height()),
                                          _scene_fbo.get(), QRect(0, 0, screen_size.width(), screen_size.height()),
                                          GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    draw_elements_ui(level_data);

    glEnable(GL_DEPTH_TEST);

    main_fbo->release();
}
