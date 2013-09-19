#include "My_viewer.h"

#include <Atom.h>

void My_viewer::setup_intro()
{
    _my_camera->setUpVector(qglviewer::Vec(0.0f, 0.0f, 1.0f));
    _my_camera->setViewDirection(qglviewer::Vec(0.0f, 1.0f, 0.0f));
    _my_camera->setPosition(qglviewer::Vec(0.0f, -80.0f, 0.0f));

    Eigen::Vector3f grid_start(-10.0f, -10.0f, -10.0f);
    Eigen::Vector3f grid_end  ( 10.0f,  10.0f, 10.0f);

    int const resolution = 5;

    // add bunch of water molecules
    for (float x = grid_start[0]; x < grid_end[0]; x += resolution)
    {
        for (float y = grid_start[1]; y < grid_end[1]; y += resolution)
        {
            for (float z = grid_start[2]; z < grid_end[2]; z += resolution)
            {
                _core.add_molecule(Molecule::create_water(Eigen::Vector3f(x, y, z)));
            }
        }
    }

    _parameters["Core/rotation_fluctuation"]->set_value(0.5f);
    _parameters["Core/translation_fluctuation"]->set_value(2.0f);

    _parameters["Core/rotation_damping"]->set_value(0.5f);
    _parameters["Core/translation_damping"]->set_value(0.1f);

    _parameters["Core/mass_factor"]->set_value(0.1f);

    _parameters["game_field_left"]->set_value(-50.0f);
    _parameters["game_field_right"]->set_value(50.0f);
    _parameters["game_field_front"]->set_value(-50.0f);
    _parameters["game_field_back"]->set_value(50.0f);
    _parameters["game_field_bottom"]->set_value(-50.0f);
    _parameters["game_field_top"]->set_value(50.0f);

    for (int i = 0; i < 100; ++i)
    {
        _core.update(0.05f);
    }

    qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
    kfi->addKeyFrame(*camera()->frame(), 0.0f);
    camera()->setKeyFrameInterpolator(0, kfi);

    qglviewer::Frame front_view = *camera()->frame();
    front_view.setPosition(0.0f, -50.0f, 0.0f);
    kfi->addKeyFrame(front_view, 10.0f);

    qglviewer::Frame side_view = *camera()->frame();
    qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, -0.57735f, -0.57735f), 2.094f);
    side_view.setOrientation(orientation);
    side_view.setPosition(0.0f, -40.0f, 0.0f);
    kfi->addKeyFrame(side_view, 16.0f);

    kfi->startInterpolation();

    connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));


    _intro_time = 0.0f;
    _intro_state = Intro_state::Beginning;

    set_simulation_state(true);
}

void My_viewer::update_intro(const float timestep)
{
    _intro_time += timestep;

    if (_intro_state == Intro_state::Beginning)
    {
        if (_intro_time > 10.0f)
        {
            _parameters["Core/rotation_fluctuation"]->set_value(0.0f);
            _parameters["Core/translation_fluctuation"]->set_value(0.0f);

//            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
//            kfi->addKeyFrame(*camera()->frame());
//            camera()->setKeyFrameInterpolator(0, kfi);

//            qglviewer::Frame side_view = *camera()->frame();
//            qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, -0.57735f, -0.57735f), 2.094f);
//            side_view.setOrientation(orientation);
//            side_view.setPosition(0.0f, -40.0f, 0.0f);

//            kfi->addKeyFrame(side_view);
//            kfi->setInterpolationTime(5.0f);
//            kfi->startInterpolation();
//            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));

            Molecule m = Molecule::create_water(Eigen::Vector3f(20.0f, -40.0f, 0.0f));
            Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
            t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));
            t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
            m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));

            _core.add_molecule(m);

            _intro_state = Intro_state::Single_molecule;
            _intro_time = 0.0f;
        }
    }
    else if (_intro_state == Intro_state::Single_molecule)
    {
        if (_intro_time > 8.0f)
        {
            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
            kfi->addKeyFrame(*camera()->frame());
            camera()->setKeyFrameInterpolator(0, kfi);
            qglviewer::Frame translated_view = *camera()->frame();
            translated_view.translate(qglviewer::Vec(-5.0f, 5.0f, 0.0f));
            kfi->addKeyFrame(translated_view);
            kfi->setInterpolationTime(4.0f);
            kfi->startInterpolation();

            _core.add_molecule(Molecule::create_water(Eigen::Vector3f(20.0f, -30.0f, 0.0f)));
            Molecule & m = _core.get_molecules().back();
            Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
            t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));
            t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
            m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));

            for (Atom & a : m._atoms)
            {
                std::string sign = "-";

                if (a._charge > 0.0f)
                {
                    sign = "+";
                }

                Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, camera());
                generate_label_texture(label);
                _labels[int(Level_state::Intro)].push_back(boost::shared_ptr<Draggable_label>(label));
            }

//            set_simulation_state(false);

            _intro_state = Intro_state::Two_molecules_0;
            _intro_time = 0.0f;
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_0)
    {
        if ((_core.get_molecules().begin()->_x - (++_core.get_molecules().begin())->_x).norm() < 4.0f)
        {
            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_1;
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_1)
    {
        if (_intro_time > 5.0f)
        {
            for (boost::shared_ptr<Draggable_label> & label : _labels[int(Level_state::Intro)])
            {
                label->set_alpha(0.0f);
            }

            for (int i = 0; i < 2; ++i)
            {
                _core.add_molecule(Molecule::create_water(Eigen::Vector3f(-20.0f, -33.0f - i * 4.0f, 0.0f)));
                Molecule & m = _core.get_molecules().back();
                Eigen::Transform<float, 3, Eigen::Affine> t = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
                t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 1.0f, 0.0f)));

                if (i == 0)
                {
                    t.rotate(Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
                }
                else
                {
                    t.rotate(Eigen::AngleAxisf(-M_PI * 0.5f, Eigen::Vector3f(0.0f, 0.0f, 1.0f)));
                }

                m.apply_orientation(Eigen::Quaternion<float>(t.rotation()));

                for (Atom & a : m._atoms)
                {
                    std::string sign = "-";

                    if (a._charge > 0.0f)
                    {
                        sign = "+";
                    }

                    Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, camera());
                    generate_label_texture(label);
                    _labels[int(Level_state::Intro)].push_back(boost::shared_ptr<Draggable_label>(label));
                }
            }

            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
            kfi->addKeyFrame(*camera()->frame());
            camera()->setKeyFrameInterpolator(0, kfi);

            qglviewer::Frame side_view = *camera()->frame();
            qglviewer::Quaternion orientation(qglviewer::Vec(0.57735f, 0.57735f, 0.57735f), 2.094f);
            side_view.setOrientation(orientation);
            side_view.setPosition(qglviewer::Vec(5.0f, -35.0f, 0.0f));

            kfi->addKeyFrame(side_view, 8.0f);

            kfi->startInterpolation();
//            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam1_end_reached()));

            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_2;

            // damp a lot to force standstill, but don't disable the simulation to keep getting notification for the labels' position updates
            _parameters["Core/rotation_damping"]->set_value(100.0f);
            _parameters["Core/translation_damping"]->set_value(100.0f);
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_2)
    {
        if (_intro_time > 8.0f)
        {
            // remove excessive damping
            _parameters["Core/rotation_damping"]->set_value(0.5f);
            _parameters["Core/translation_damping"]->set_value(0.1f);

            _intro_time = 0.0f;
            _intro_state = Intro_state::Two_molecules_3;
        }
    }
    else if (_intro_state == Intro_state::Two_molecules_3)
    {
//        auto iter = _core.get_molecules().begin();
//        std::advance(iter, 2);
//        Molecule const& m0 = *iter;
//        std::advance(iter, 1);
//        Molecule const& m1 = *iter;

//        if ((m0._x - m1._x).norm() < 4.0f)
        if (_intro_time > 10.0f)
        {
            _intro_time = 0.0f;
            _intro_state = Intro_state::Finishing;
        }
    }
    else if (_intro_state == Intro_state::Finishing)
    {
        if (_intro_time > 4.0f)
        {
            float const z = 0.5f * (_core.get_level_data()._game_field_borders[Level_data::Plane::Pos_Z]->get_position()[2]
                    + _core.get_level_data()._game_field_borders[Level_data::Plane::Neg_Z]->get_position()[2]);

            qglviewer::Vec level_start_position(0.0f, -80.0f, z);

            qglviewer::KeyFrameInterpolator * kfi = new qglviewer::KeyFrameInterpolator(camera()->frame());
            kfi->addKeyFrame(*camera()->frame());
            camera()->setKeyFrameInterpolator(0, kfi);

            qglviewer::Quaternion orientation(qglviewer::Vec(1.0f, 0.0f, 0.0f), M_PI * 0.5f);
            qglviewer::Frame front_view = *camera()->frame();
            front_view.setOrientation(orientation);
            front_view.setPosition(level_start_position);

            kfi->addKeyFrame(front_view);
            kfi->setInterpolationTime(2.0f);
            kfi->startInterpolation();

            connect(kfi, SIGNAL(endReached()), this, SLOT(intro_cam2_end_reached()));

            _intro_state = Intro_state::Finished;
        }
    }
}

void My_viewer::intro_cam1_end_reached()
{
    _core.get_molecules().erase(_core.get_molecules().begin(), --_core.get_molecules().end());

    Molecule & m = _core.get_molecules().front();

    for (Atom & a : m._atoms)
    {
        std::string sign = "-";

        if (a._charge > 0.0f)
        {
            sign = "+";
        }

        Draggable_label * label = new Draggable_atom_label(Eigen::Vector3f(0.5f, 0.8f, 0.0f), Eigen::Vector2f(0.1f, 0.1f), sign, &a, camera());
        generate_label_texture(label);
        _labels[int(Level_state::Intro)].push_back(boost::shared_ptr<Draggable_label>(label));
    }
}

void My_viewer::intro_cam2_end_reached()
{
    load_next_level();
}
