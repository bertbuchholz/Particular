#include "Main_menu_screen.h"

#include "My_viewer.h"


Main_menu_screen::Main_menu_screen(My_viewer &viewer, Core &core) : Screen(viewer), _core(core) // , _calling_screen(calling_state)
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
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    _viewer.start_normalized_screen_coordinates();

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _viewer.draw_button(button.get(), false);
    }

    _viewer.stop_normalized_screen_coordinates();

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
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.85f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Start New Game",  std::bind(&Main_menu_screen::start_new_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.65f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Continue Game",  std::bind(&Main_menu_screen::continue_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.45f, 0.0f), Eigen::Vector2f(0.5f, 0.15f), "Quit", std::bind(&Main_menu_screen::quit_game, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }

    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _viewer.generate_button_texture(button.get());
    }

    _game_name_system.generate("I NEED A NAME", _viewer.get_particle_font(), QRectF(0.0f, 0.6f, 1.0f, 0.3f));
}


void Main_menu_screen::start_new_game()
{
    // set level to 0 (including introduction part) and start game

    _core.get_progress().reset();

    _viewer.change_level_state(My_viewer::Level_state::Intro);

    _core.clear();

    _viewer.setup_intro();

    _state = State::Killing;
}


void Main_menu_screen::continue_game()
{
    // load current progress and start game
    _viewer.load_next_level();

    _state = State::Killing;
}


void Main_menu_screen::quit_game()
{
    _core.save_progress();

    _viewer.quit_game();
}

void Main_menu_screen::update_event(const float time_step)
{
    _game_name_system.animate(time_step);
}
