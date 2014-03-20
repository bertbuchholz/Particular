#include "Menu_screen.h"

#include "My_viewer.h"

#include <GL/gl.h>

//#define GL_GLEXT_PROTOTYPES
//#include <GL/glext.h>


Menu_screen::Menu_screen(My_viewer & viewer, Core & core) : Screen(viewer), _core(core), _renderer(_viewer.get_renderer()), _hover_index(-1), _time(0.0f)
{
//    _renderer.init(_viewer.context(), _viewer.size());

    _picking.init(_viewer.context());

    _gl_functions.init(_viewer.context());

    _heat_program = std::unique_ptr<QGLShaderProgram>(init_program(_viewer.context(),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/temperature.vert"),
                                                                          Data_config::get_instance()->get_absolute_qfilename("shaders/heat.frag")));
}

void Menu_screen::draw_hovered_button(Draggable_button const* b, float const time, float const alpha)
{
//    std::cout << __FUNCTION__ << std::endl;

    _heat_program->bind();

    _heat_program->setUniformValue("scene_texture", 0);
    _heat_program->setUniformValue("repetition_ratio", b->get_extent()[0] / b->get_extent()[1]);
    _heat_program->setUniformValue("alpha", alpha);
    _gl_functions.glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, bg_texture);

    _heat_program->setUniformValue("screen_size", QSize(_viewer.width(), _viewer.height()));
    _heat_program->setUniformValue("time", time);

    _viewer.draw_button(b, false, alpha);

    _heat_program->release();
}

void Menu_screen::update_event(const float timestep)
{
    _time += timestep;
}

void Menu_screen::draw()
{
    if (get_state() == State::Paused) return;

    float alpha = 1.0f;

    if (get_state() == State::Killing || get_state() == State::Resuming || get_state() == State::Pausing)
    {
        if (get_state() == State::Killing || get_state() == State::Pausing)
        {
            alpha = 1.0f - _transition_progress;
        }
        else if (get_state() == State::Resuming)
        {
            alpha = _transition_progress;
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        if (_buttons[i]->is_visible())
        {
            boost::shared_ptr<Draggable_button> const& button = _buttons[i];
            if (int(i) == _hover_index)
            {
                draw_hovered_button(button.get(), _time, alpha);
            }
            else
            {
                _viewer.draw_button(button.get(), false, alpha);
            }
        }
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        if (label->is_visible())
        {
            _viewer.draw_label(label.get(), alpha);
        }
    }

    _viewer.stop_normalized_screen_coordinates();
}

bool Menu_screen::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int picked_index = _picking.do_pick(
                    event->pos().x() / float(_viewer.camera()->screenWidth()),
                    (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                    std::bind(&Menu_screen::draw_draggables_for_picking, this));

        if (picked_index > -1)
        {
            _buttons[picked_index]->clicked();
        }
    }

    return true;
}

bool Menu_screen::mouseMoveEvent(QMouseEvent * event)
{
    int const new_picking_index = _picking.do_pick(event->pos().x() / float(_viewer.camera()->screenWidth()), (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                                     std::bind(&Menu_screen::draw_draggables_for_picking, this));

    if (new_picking_index != _hover_index)
    {
        if (_hover_index != -1)
        {
            // stopped hovering over item
            std::cout << __FUNCTION__ << " stopped hovering" << std::endl;

            _hover_index = -1;
        }

        if (new_picking_index != -1)
        {
            // entered new picked item

            _hover_index = new_picking_index;
            std::cout << __FUNCTION__ << " started hovering" << std::endl;
        }
    }

    return true;
}

void Menu_screen::resize(QSize const& /* size */)
{
    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _renderer.generate_label_texture(label.get());
    }
}

void Menu_screen::draw_draggables_for_picking()
{
    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        if (_buttons[i]->is_visible())
        {
            _picking.set_index(int(i));
            _viewer.draw_button(_buttons[i].get(), true);
        }
    }

    _viewer.stop_normalized_screen_coordinates();
}
