#include "Main_menu_screen.h"

#include "My_viewer.h"
#include "Before_start_screen.h"
#include "Editor_screen.h"

Main_menu_screen::Main_menu_screen(My_viewer &viewer, Core &core) : Screen(viewer), _core(core)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}


bool Main_menu_screen::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int picked_index = _picking.do_pick(
                    event->pos().x() / float(_viewer.camera()->screenWidth()),
                    (_viewer.camera()->screenHeight() - event->pos().y())  / float(_viewer.camera()->screenHeight()),
                    std::bind(&Main_menu_screen::draw_draggables_for_picking, this));

        if (picked_index > -1)
        {
            _buttons[picked_index]->clicked();
        }
    }

    return true;
}


void Main_menu_screen::draw()
{
    float alpha = 1.0f;

    if (get_state() == State::Killing || get_state() == State::Resuming)
    {
        if (get_state() == State::Killing)
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

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
//            if (button->is_visible())
        {
            _viewer.draw_button(button.get(), false, alpha);
        }
    }

    _viewer.stop_normalized_screen_coordinates();


//    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

//    _viewer.start_normalized_screen_coordinates();

//    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
//    {
//        _viewer.draw_button(button.get(), false);
//    }

//    _viewer.stop_normalized_screen_coordinates();

    setup_gl_points(false);

    draw_particle_system(_game_name_system, _viewer.height());
}


void Main_menu_screen::draw_draggables_for_picking()
{
    _viewer.start_normalized_screen_coordinates();

    for (size_t i = 0; i < _buttons.size(); ++i)
    {
        _picking.set_index(i);
        _viewer.draw_button(_buttons[i].get(), true);
    }

    _viewer.stop_normalized_screen_coordinates();
}


void Main_menu_screen::state_changed_event(const Screen::State new_state, const Screen::State previous_state)
{
    std::cout << __PRETTY_FUNCTION__ << " " << int(new_state) << " " << int(previous_state) << std::endl;

    //        if (new_state == State::Killed)
    //        {
    //            _calling_screen->resume();
    //        }
}


void Main_menu_screen::init()
{
    // main menu
    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.6f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Start New Game",  std::bind(&Main_menu_screen::start_new_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.4f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Continue Game",  std::bind(&Main_menu_screen::continue_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.2f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Quit", std::bind(&Main_menu_screen::quit_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.85f, 0.1f, 0.0f), Eigen::Vector2f(0.2f, 0.06f), "Editor", std::bind(&Main_menu_screen::start_editor, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _viewer.generate_button_texture(button.get());
    }

    _game_name_system.generate("PARTICULAR", _viewer.get_particle_font(), QRectF(0.0f, 0.05f, 1.0f, 0.3f));
}


void Main_menu_screen::start_new_game()
{
    // set level to 0 (including introduction part) and start game

    _core.get_progress().reset();

    _core.clear();

    for (Targeted_particle & p : _game_name_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _core.change_level_state(Main_game_screen::Level_state::Intro);

    kill();
}


void Main_menu_screen::continue_game()
{
    // load current progress and start game
    _core.load_next_level();

    for (Targeted_particle & p : _game_name_system.get_particles())
    {
        p.target = Eigen::Vector3f::Random().normalized();
        p.target *= 1.5f;
    }

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    kill();
}


void Main_menu_screen::quit_game()
{
    _core.save_progress();

    _viewer.quit_game();
}

void Main_menu_screen::start_editor()
{
    _viewer.camera()->frame()->setConstraint(nullptr);

    _core.load_level_defaults();
    _core.set_simulation_state(false);

    _viewer.replace_screens(new Editor_screen(_viewer, _core));
}

void Main_menu_screen::update_event(const float time_step)
{
    _game_name_system.animate(time_step);
}
