#ifndef OPTIONS_VIEWER_H
#define OPTIONS_VIEWER_H

#include <cmath>
#include <fstream>
#include <unordered_map>
#include <QtGui>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QDockWidget>

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/manipulatedCameraFrame.h>

#include "StandardCamera.h"
#include "Parameter.h"
#include "Q_parameter_bridge.h"
#include "Frame_buffer.h"
#include "Color.h"
#include "Color_utilities.h"
#include "GL_utilities.h"
#include "Decal.h"
#include "Geometry_utils.h"

//#ifdef __APPLE__
#define USE_EXTERNAL_OPTIONS_WINDOW
//#endif


class Flyin_widget : public QScrollArea
{
    Q_OBJECT

    typedef QScrollArea Base;

public:
//    Flyin_widget(QWidget * p) : QScrollArea(NULL), _keep_open(false)
    Flyin_widget(QWidget * p) : QScrollArea(p), _keep_open(false)
    {
        _fade_in_animation = new QPropertyAnimation(this, "geometry");
        _fade_in_animation->setDuration(200);
        _fade_in_animation->setDirection(QAbstractAnimation::Backward);
        _fade_in_animation->setEasingCurve(QEasingCurve::InOutQuad);

        setVisible(false);

        setContentsMargins(0, 0, 0, 0);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

//        setStyleSheet("background:transparent;");
//        setAttribute(Qt::WA_TranslucentBackground);
//        setWindowFlags(Qt::FramelessWindowHint);
    }

    void hide_flyin_widget()
    {
        // std::cout << "hide_flyin_widget()" << std::endl;

        if (_keep_open) return;

        if (_fade_in_animation->direction() == QAbstractAnimation::Backward &&
                _fade_in_animation->state() != QAbstractAnimation::Running) return;

        _fade_in_animation->setDirection(QAbstractAnimation::Backward);

        if (_fade_in_animation->state() != QAbstractAnimation::Running)
        {
            _fade_in_animation->start();
        }
    }

    void show_flyin_widget()
    {
        // std::cout << "show_flyin_widget()" << std::endl;

        setVisible(true);

        if (_fade_in_animation->direction() == QAbstractAnimation::Forward &&
                _fade_in_animation->state() != QAbstractAnimation::Running) return;

        _fade_in_animation->setDirection(QAbstractAnimation::Forward);

        if (_fade_in_animation->state() != QAbstractAnimation::Running)
        {
            _fade_in_animation->start();
        }
    }

    void toggle_open()
    {
        _keep_open = !_keep_open;

        if (_keep_open)
        {
            show_flyin_widget();
        }
        else
        {
            hide_flyin_widget();
        }
    }


    void setWidget(QWidget * widget)
    {
        // setMinimumWidth(widget->width() + verticalScrollBar()->width());
        setMinimumWidth(widget->minimumSizeHint().width() + verticalScrollBar()->width());
        setMinimumHeight(parentWidget()->height());

        QRect hidden_rect  = QRect(-width(), 0, width(), height());
        QRect visible_rect = QRect(       0, 0, width(), height());

        _fade_in_animation->setStartValue(hidden_rect);
        _fade_in_animation->setEndValue(visible_rect);

        Base::setWidget(widget);
    }

    void resizeEvent(QResizeEvent * ev) override
    {
        std::cout << __FUNCTION__ << std::endl;

        QRect hidden_rect  = QRect(-width(), 0, width(), height());
        QRect visible_rect = QRect(       0, 0, width(), height());

        _fade_in_animation->setStartValue(hidden_rect);
        _fade_in_animation->setEndValue(visible_rect);

        widget()->adjustSize();
        adjustSize();

        Base::resizeEvent(ev);
    }

    void childEvent(QChildEvent *) override
    {
        std::cout << __FUNCTION__ << " Flyin_widget::childEvent();" << std::endl;
    }

public Q_SLOTS:
    void widget_event(QChildEvent *)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

Q_SIGNALS:
    void leaving();
    void entering();

protected:
    // prevent mouse events being passed to the underlying (qgl)widget
    void mousePressEvent(QMouseEvent *) override {}

    void wheelEvent(QWheelEvent * ev) override
    {
        if (verticalScrollBar()->value() == verticalScrollBar()->minimum() &&
                ev->delta() > 0)
            return;

        if (verticalScrollBar()->value() == verticalScrollBar()->maximum() &&
                ev->delta() < 0)
            return;

        Base::wheelEvent(ev);
    }

    void leaveEvent(QEvent *) override
    {
        // std::cout << "Flyin_widget_real::leaveEvent()" << std::endl;
        hide_flyin_widget();
        Q_EMIT leaving();
    }

    void enterEvent(QEvent *) override
    {
        // std::cout << "Flyin_widget_real::enterEvent()" << std::endl;
        show_flyin_widget();
        Q_EMIT entering();
    }

private:
    QPropertyAnimation * _fade_in_animation;
    bool _keep_open;
};




class Options_viewer : public QGLViewer
{
    Q_OBJECT

public:
    typedef QGLViewer Base;

    Options_viewer(QGLFormat const& format = QGLFormat()) : QGLViewer(format), _is_fullscreen(false)
    {
#ifdef USE_EXTERNAL_OPTIONS_WINDOW
        _menu_frame = new QScrollArea(this);
        _menu_frame->setWindowFlags(_menu_frame->windowFlags() | Qt::Window);
#else
        _menu_frame = new Flyin_widget(this);
#endif

        setCamera(new StandardCamera());
        setMouseTracking(true);

        for (int i = 0; i < 3; ++i)
        {
            float const label_height = 0.03f;

            Info_label l;
            l._screen_pos = {0.02f, 0.02f + label_height * 1.2f * i };
            l._size = { 1.0f, label_height };
            l._aspect = Decal::Keep_height;
            l._bg_alpha = 0.3f;

            _info_labels.push_back(l);
        }

        int id = QFontDatabase::addApplicationFont("data/fonts/SourceSansPro-Semibold.otf");
        id *= QFontDatabase::addApplicationFont("data/fonts/SourceSansPro-Regular.otf");

        if (id >= 0)
        {
    //        QStringList families = QFontDatabase::applicationFontFamilies(id);
            _font.setFamily("Source Sans Pro");
        }
        else
        {
            std::cout << __func__ << " Couldn't load at least one font." << std::endl;
        }
    }

    virtual ~Options_viewer()
    { }

protected:
    void align_camera(int key)
    {
        qglviewer::Vec new_direction;
        qglviewer::Vec new_up(0.0f, 0.0f, 1.0f);
        qglviewer::Vec new_position;

        if (key == Qt::Key_1) // Front view (from pos x)
        {
            new_position  = qglviewer::Vec(1.0f, 0.0f, 0.0f);
            new_direction = -new_position;
        }
        else if (key == Qt::Key_3) // Right view (from pos y)
        {
            new_position  = qglviewer::Vec(0.0f, 1.0f, 0.0f);
            new_direction = -new_position;
        }
        else if (key == Qt::Key_7) // Top view (from pos z)
        {
            new_position  = qglviewer::Vec(0.0f, 0.0f, 1.0f);
            new_direction = -new_position;
            new_up        = qglviewer::Vec(0.0f, 1.0f, 0.0f);
        }

        camera()->setPosition(new_position * sceneRadius() * 4.0f);
        camera()->setUpVector(new_up);
        camera()->setViewDirection(new_direction);
        update();
    }

    void rotate_camera(int key)
    {
        int longest_axis = get_longest_axis(camera()->upVector()); // primary rotation axis with keypad 4 and 6
        // int secondary_axis = get_secondary_axis(camera()->upVector(), longest_axis);

        //qglviewer::Vec rotationAxis[3] = {qglviewer::Vec(1.0f, 0.0f, 0.0f), qglviewer::Vec(0.0f, 1.0f, 0.0f), qglviewer::Vec(0.0f, 0.0f, 1.0f)};

        qglviewer::Vec rotationAxis;

        int rotation_axis_index = longest_axis;

        float direction = 1.0f;

        if (key == Qt::Key_4)
        {
            rotationAxis = camera()->upVector();
        }
        if (key == Qt::Key_6)
        {
            rotationAxis = camera()->upVector();
            direction = -1.0f;
        }
        else if (key == Qt::Key_8)
        {
            rotationAxis = camera()->rightVector();
            //rotation_axis_index = secondary_axis;
        }
        else if (key == Qt::Key_2)
        {
            rotationAxis = camera()->rightVector();
            //rotation_axis_index = secondary_axis;
            direction = -1.0f;
        }

        std::cout << rotation_axis_index << " " << direction << std::endl;

        float rotation_amount = 0.01 * direction;
        float rotation = 2 * M_PI * rotation_amount;

        // const qglviewer::Vec& initPoint = this->camera()->position();
        qglviewer::Vec rotationCenter(0,0,0);
        qglviewer::ManipulatedCameraFrame* camframe =  camera()->frame();
        // qglviewer::Vec localAxis = camframe->localTransformOf(rotationAxis[rotation_axis_index]);
        qglviewer::Vec localAxis = camframe->localTransformOf(rotationAxis);
        qglviewer::Quaternion rotationQuaternion(localAxis, rotation);

        camframe->rotateAroundPoint(rotationQuaternion, rotationCenter);

        update();
    }


    void keyPressEvent(QKeyEvent * event) override
    {
//        std::cout << "Options_viewer::keyPressEvent() " << event->key() << std::endl;

        bool handled = false;

        if (event->key() == Qt::Key_O)
        {
            show_option_menu();
            handled = true;
        }
        else if (event->key() == Qt::Key_B)
        {
            QColor const bg = QColorDialog::getColor(backgroundColor(), this);

            if (bg.isValid())
            {
                setBackgroundColor(bg);
            }

            handled = true;
        }
        else if (event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier)
        {
            int i = 0;
            QString filename = QDir::tempPath() + QString("/qglviewer_screenshot-%1.png").arg(int(i), 3, 10, QChar('0'));

            while (QFile::exists(filename))
            {
                ++i;
                filename = QString(QDir::tempPath() + "/qglviewer_screenshot-%1.png").arg(int(i), 3, 10, QChar('0'));
            }

//            setSnapshotFormat("PNG");
//            saveSnapshot(filename);
//            QImage img = grabFrameBuffer(true);

            Frame_buffer<Color4> buffer(camera()->screenWidth(), camera()->screenHeight());
            glReadPixels(0, 0, camera()->screenWidth(), camera()->screenHeight(), GL_RGBA, GL_FLOAT, buffer.get_raw_data());
            QImage img = convert_with_alpha(buffer);

            img.save(filename);

            std::cout << __FUNCTION__ << " shot saved to " << filename.toStdString() << std::endl;

            if (event->modifiers() & Qt::ShiftModifier)
            {
                std::cout << "Starting gimp ..." << std::endl;

                QProcess process;
                QStringList arguments;
                arguments << filename;

                process.startDetached("gimp", arguments);
            }

            handled = true;
        }
        else if (event->key() == Qt::Key_Return && event->modifiers() & Qt::AltModifier)
        {
            if (_is_fullscreen)
            {
//                setFullScreen(false);
                showNormal();
            }
            else
            {
//                setFullScreen();
                showFullScreen();
            }

            _is_fullscreen = !_is_fullscreen;

            handled = true;
        }
        else if (event->key() == Qt::Key_U && event->modifiers() & Qt::ControlModifier)
        {
            save_parameters_with_check();

            handled = true;
        }
        else if (event->key() == Qt::Key_U && event->modifiers() & Qt::ShiftModifier)
        {
            restore_parameters_with_check();

            handled = true;
        }
        else if (event->modifiers() & Qt::KeypadModifier)
        {
            if (event->key() == Qt::Key_1 || event->key() == Qt::Key_3 || event->key() == Qt::Key_7)
            {
                align_camera(event->key());
                handled = true;
            }
            else if (event->key() == Qt::Key_4 || event->key() == Qt::Key_6 || event->key() == Qt::Key_8 || event->key() == Qt::Key_2)
            {
                rotate_camera(event->key());
                handled = true;
            }
            else if (event->key() == Qt::Key_5)
            {
                if (camera()->type() == qglviewer::Camera::ORTHOGRAPHIC)
                {
                    camera()->setType(qglviewer::Camera::PERSPECTIVE);
                    update();
                }
                else
                {
                    camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
                    update();
                }

                handled = true;
            }
        }
        else if (event->key() == Qt::Key_F9)
        {
            load_defaults();

            handled = true;
        }
        else if (event->key() == Qt::Key_Escape)
        {
            showNormal();
            _is_fullscreen = false;
        }

        if (!handled) QGLViewer::keyPressEvent(event);
    }

    void save_parameters_with_check()
    {
        std::unique_ptr<QMessageBox> msg_box(new QMessageBox(QMessageBox::NoIcon, "Save parameters", "Really save?", QMessageBox::Cancel | QMessageBox::Ok, this));

        int pressed_button = msg_box->exec();

        if (pressed_button == QMessageBox::Ok)
        {
            save_parameters();
        }
    }

    void restore_parameters_with_check()
    {
        std::unique_ptr<QMessageBox> msg_box(new QMessageBox(QMessageBox::NoIcon, "Restore parameters", "Really restore?", QMessageBox::Cancel | QMessageBox::Ok, this));

        int pressed_button = msg_box->exec();

        if (pressed_button == QMessageBox::Ok)
        {
            restore_parameters();
        }
    }

    virtual void load_defaults()
    {
        std::cout << __func__ << " No default function specified." << std::endl;
    }

    QFrame * create_options_widget() const
    {
        std::cout << "create_options_widget()" << std::endl;

        QFrame * frame = new QFrame;

        QFont f = frame->font();
        f.setPointSize(f.pointSize() * 0.9f);
        frame->setFont(f);

        QLayout * menu_layout = new QVBoxLayout(frame);

        draw_instance_list(_parameters, menu_layout);

        for (QWidget * w : _option_widgets)
        {
            menu_layout->addWidget(w);
        }

        menu_layout->setSpacing(0);
        menu_layout->setMargin(0);
        menu_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        frame->setWindowTitle(windowTitle() + " Options");
        frame->setLayout(menu_layout);

        return frame;
    }

    void show_option_menu()
    {
        if (!_menu_frame->widget())
        {
#ifdef USE_EXTERNAL_OPTIONS_WINDOW
//            _menu_dock_widget = new QDockWidget;
//            _menu_dock_widget->setWidget(create_options_widget());
//            _menu_dock_widget->show();
//            create_options_widget()->show();
//            _menu_frame->setWidget(create_options_widget());
//            _menu_frame->show();

            _menu_frame->setWidget(create_options_widget());
            _menu_frame->setWidgetResizable(true);
            _menu_frame->show();
#else
            _menu_frame->setWidget(create_options_widget());
#endif
        }
#ifdef USE_EXTERNAL_OPTIONS_WINDOW
        else
        {
            _menu_frame->show();
            _menu_frame->raise();
        }
#else
        _menu_frame->toggle_open();
#endif
    }

    void update_options()
    {
        _menu_frame->setWidget(create_options_widget());
    }

    void mousePressEvent(QMouseEvent * event) override
    {
        bool handled = false;

        if (event->button() == Qt::RightButton && event->modifiers() & Qt::AltModifier)
        {
            handled = show_context_menu(event->pos());
        }

        if (!handled) Base::mousePressEvent(event);
    }

    void resizeEvent(QResizeEvent * ev) override
    {
        std::cout << __FUNCTION__ << std::endl;

#ifndef USE_EXTERNAL_OPTIONS_WINDOW
        _menu_frame->setFixedHeight(height());
#endif

        Base::resizeEvent(ev);
    }


    /*
    void mouseMoveEvent(QMouseEvent * ev)
    {
        // std::cout << "mouseMoveEvent(); " << ev->x() << " " << ev->y() << std::endl;
        if (ev->pos().x() < 20)
        {
            std::cout << "Options_viewer::mouseMoveEvent() in zone" << std::endl;

            if (!_menu_frame)
            {
                _menu_frame = create_options_widget();
            }

            _menu_frame->show_flyin_widget();
        }
        else
        {
            Base::mouseMoveEvent(ev);
        }
    }
    */

    virtual bool show_context_menu(const QPoint& /* pos */)
    {
        std::cout << __func__ << " Got no context menu." << std::endl;
        return false;
    }

    virtual void restore_parameters()
    {
        setUpdatesEnabled(false);
        _parameters.load(std::string("boost_parameters_") + windowTitle().toStdString());
        setUpdatesEnabled(true);

#ifdef USE_EXTERNAL_OPTIONS_WINDOW

#else
        int const old_value = _menu_frame->verticalScrollBar()->value();

        _menu_frame->setWidget(create_options_widget()); // TODO: can possibly be removed, if parameter callbacks work correctly
        _menu_frame->verticalScrollBar()->setValue(old_value);
#endif
    }

    void save_parameters() const
    {
        _parameters.save(std::string("boost_parameters_") + windowTitle().toStdString());
    }

    void add_widget_to_options(QWidget * w)
    {
        _option_widgets.push_back(w);
    }

    void set_info_text(int const label_num, std::string const& text)
    {
        if (label_num >= int(_info_labels.size())) return;

        _info_labels[label_num].set_text(text, _font);
    }

    void draw_decal(Decal const& d);

    void postDraw() override;

    void init() override;

protected:
#ifdef USE_EXTERNAL_OPTIONS_WINDOW
    QScrollArea * _menu_frame;
#else
    Flyin_widget * _menu_frame;
#endif

    QDockWidget * _menu_dock_widget;

    bool _ignore_leave;

    Parameter_list _parameters;

    std::vector<QWidget*> _option_widgets;

    bool _is_fullscreen;

    std::vector<Info_label> _info_labels;

    QFont _font;

    std::unique_ptr<QOpenGLShaderProgram> _screen_label_program;

    GL_mesh2 _screen_quad;

    bool _draw_info_labels;
};

#endif // OPTIONS_VIEWER_H
