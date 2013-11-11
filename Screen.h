#ifndef STATE_H
#define STATE_H

#include <QtGui>
#include <memory>

class My_viewer;

class Screen : public QObject
{
    Q_OBJECT

public:
    enum class Type { Modal = 0x01, Fullscreen = 0x02 };
    enum class State { Paused = 0, Resuming, Pausing, Running, Fading_out, Fading_in, Faded_out, Killing, Killed };

    Screen(My_viewer & viewer) :
        _viewer(viewer),
        _transition_progress(0.0f),
        _state(State::Resuming)
    { }

    virtual ~Screen() {}

    Type get_type() const
    {
        return _type;
    }

    State get_state() const
    {
        return _state;
    }

    virtual bool mousePressEvent(QMouseEvent *) { return false; }
    virtual bool mouseMoveEvent(QMouseEvent *) { return false; }
    virtual bool mouseReleaseEvent(QMouseEvent * ) { return false; }
    virtual bool keyPressEvent(QKeyEvent *) { return false; }
    virtual bool wheelEvent(QWheelEvent *) { return false; }


    virtual void resize(QSize const& /* size */) { }

//    virtual void draw(qglviewer::Camera const* camera, Parameter_list const& parameters);
    virtual void draw() { }

    void pause();
    void resume();
    void kill();

    virtual void deactivate() { }
    virtual void activate() { }

    void update(float const time_step);

    virtual void update_event(float const /* time_step */) { }
    virtual void state_changed_event(State const /* current_state */, State const /* previous_state */)
    { }

    static bool is_dead(std::unique_ptr<Screen> const& s)
    {
        return (s->_state == State::Killed);
    }

protected:
    My_viewer & _viewer;

    Type _type;

    float _transition_progress;

private:
    State _state;
};



#endif // STATE_H
