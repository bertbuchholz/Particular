#include "level_picker_screen.h"

#include "My_viewer.h"

#include "Before_start_screen.h"
#include "Main_menu_screen.h"
#include "Statistics_screen.h"

Level_picker_screen::Level_picker_screen(My_viewer & viewer, Core & core, Screen *calling_screen) :
    Menu_screen(viewer, core), _calling_screen(calling_screen)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}


void Level_picker_screen::init()
{
    QStringList level_names = _core.get_level_names();

//    int const num_level_per_page = 3;

    for (int i = 0; i < level_names.size(); ++i)
    {
        QString const& level_name = level_names[i];

        int const index_on_page = i % 3;

        boost::shared_ptr<Draggable_label> level_name_label(new Draggable_label({0.25f + index_on_page * 0.25f, 0.7f, 0.0f}, { 0.2f, 0.1f }, level_name.toStdString()));
//        _collected_score_label->set_color(Score::score_color);
        _renderer.generate_label_texture(level_name_label.get());
        _level_name_labels.push_back(level_name_label);

        boost::shared_ptr<Draggable_button> play_button(new Draggable_button(Eigen::Vector3f(0.25f + index_on_page * 0.25f, 0.55f, 0.0f), Eigen::Vector2f(0.2f, 0.1f), "Play",
                                                                             std::bind(&Level_picker_screen::play_level, this, std::placeholders::_1), level_name.toStdString()));
        _play_buttons.push_back(play_button);

        boost::shared_ptr<Draggable_button> stats_button(new Draggable_button(Eigen::Vector3f(0.25f + index_on_page * 0.25f, 0.4f, 0.0f), Eigen::Vector2f(0.2f, 0.1f), "Stats",
                                                                              std::bind(&Level_picker_screen::show_stats, this, std::placeholders::_1), level_name.toStdString()));
        _statistic_buttons.push_back(stats_button);

        _labels.push_back(level_name_label);
        _buttons.push_back(play_button);
        _buttons.push_back(stats_button);
    }

    {
        boost::shared_ptr<Draggable_button> left_arrow_button (new Draggable_button(Eigen::Vector3f(0.06f, 0.5f, 0.0f), Eigen::Vector2f(0.04f, 0.7f), "L",
                                                               std::bind(&Level_picker_screen::change_page, this, std::placeholders::_1), "left"));
        _page_arrow_buttons.push_back(left_arrow_button);

        boost::shared_ptr<Draggable_button> right_arrow_button(new Draggable_button(Eigen::Vector3f(0.94f, 0.5f, 0.0f), Eigen::Vector2f(0.04f, 0.7f), "R",
                                                               std::bind(&Level_picker_screen::change_page, this, std::placeholders::_1), "right"));
        _page_arrow_buttons.push_back(right_arrow_button);

        _buttons.push_back(left_arrow_button);
        _buttons.push_back(right_arrow_button);
    }

    {
        Draggable_button * button = new Draggable_button(Eigen::Vector3f(0.5f, 0.15f, 0.0f), Eigen::Vector2f(0.5f, 0.2f), "Back to Main Menu",  std::bind(&Level_picker_screen::return_to_main_menu, this));
        _buttons.push_back(boost::shared_ptr<Draggable_button>(button));
    }


    for (boost::shared_ptr<Draggable_button> const& button : _buttons)
    {
        _renderer.generate_button_texture(button.get());
    }

    for (boost::shared_ptr<Draggable_label> const& label : _labels)
    {
        _renderer.generate_label_texture(label.get());
    }

    show_page(0);
}

void Level_picker_screen::play_level(std::string const& level_name)
{
    std::cout << __FUNCTION__ << " name: " << level_name << std::endl;

    int const level_index = _core.get_level_names().indexOf(QString::fromStdString(level_name));

    assert(level_index != -1);

    _core.load_level(level_index);
    _core.load_default_simulation_settings();

    _viewer.add_screen(new Before_start_screen(_viewer, _core));

    kill();
}

void Level_picker_screen::show_stats(std::string const& level_name)
{
    assert(_core.get_progress().scores.find(level_name) != _core.get_progress().scores.end());

    std::vector<Score> const& scores = _core.get_progress().scores[level_name];

    assert(!scores.empty());

    auto iter = std::max_element(scores.begin(), scores.end(), Score::score_comparer);

    assert(iter != scores.end());

    _viewer.add_screen(new Statistics_screen(_viewer, _core, this, *iter));

    pause();
}

void Level_picker_screen::change_page(std::string const& left_or_right)
{
    if (left_or_right == std::string("left"))
    {
        _current_page -= 1;
    }
    else
    {
        _current_page += 1;
    }

    int const num_levels_per_page = 3;
    int const num_levels = int(_level_name_labels.size());

    _current_page = into_range(_current_page, 0, num_levels / num_levels_per_page);

    show_page(_current_page);
}

void Level_picker_screen::show_page(int const page_index)
{
    int const num_levels_per_page = 3;
    int const num_levels = int(_level_name_labels.size());

    _current_page = page_index;

    for (int i = 0; i < num_levels; ++i)
    {
        _level_name_labels[i]->set_visible(false);
        _play_buttons[i]->set_visible(false);
        _statistic_buttons[i]->set_visible(false);
    }

    for (int i = page_index * num_levels_per_page; i < page_index * num_levels_per_page + 3; ++i)
    {
        _level_name_labels[i]->set_visible(true);

        if (i <= _core.get_progress().last_level)
        {
            _play_buttons[i]->set_visible(true);
        }

        if (_core.get_progress().scores.find(_core.get_level_base_name(i)) != _core.get_progress().scores.end())
        {
            _statistic_buttons[i]->set_visible(true);
        }
    }

    _page_arrow_buttons[0]->set_visible(true);
    _page_arrow_buttons[1]->set_visible(true);

    if (page_index == 0)
    {
        _page_arrow_buttons[0]->set_visible(false);
    }

    if (num_levels / num_levels_per_page == page_index + 1)
    {
        _page_arrow_buttons[1]->set_visible(false);
    }
}



//void Level_picker_screen::draw()
//{
//    _viewer.start_normalized_screen_coordinates();

//    for (boost::shared_ptr<Draggable_statistics> const& stat : _statistics)
//    {
//        _viewer.draw_statistic(*stat);
//    }

//    _viewer.stop_normalized_screen_coordinates();

//    Menu_screen::draw();
//}

void Level_picker_screen::return_to_main_menu()
{
    _viewer.add_screen(new Main_menu_screen(_viewer, _core));

    kill();
}

