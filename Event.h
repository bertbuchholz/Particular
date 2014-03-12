#ifndef EVENT_H
#define EVENT_H

class Core;
class My_viewer;

class Event
{
public:
    Event(Core & core, My_viewer & viewer) : _core(core), _viewer(viewer)
    { }

    virtual bool trigger() = 0;

protected:
    Core & _core;
    My_viewer & _viewer;
};

// First explain goal:
// Event 1
// - disable controls
// - show molecule releaser
// Hide Event 1, create time delayed Event 2
// - wait for some molecules to be released
// Event 2, create immediate Event 3
// - show portal
// Event 3, create Event 4 catching the placement of the heat element
// - show button for heat element, enable controls
// - let user place heat element
// Event 4
// - stop, explain the control elements on that
// Event 5
// -


class Molecule_releaser_event : public Event
{
public:
    Molecule_releaser_event(Core & core, My_viewer & viewer) : Event(core, viewer)
    { }

    bool trigger() override;
};

class Portal_event : public Event
{
public:
    Portal_event(Core & core, My_viewer & viewer) : Event(core, viewer)
    { }

    bool trigger() override;
};

class Heat_button_event : public Event
{
public:
    Heat_button_event(Core & core, My_viewer & viewer) : Event(core, viewer)
    { }

    bool trigger() override;
};

class Heat_element_placed_event : public Event
{
public:
    Heat_element_placed_event(Core & core, My_viewer & viewer) : Event(core, viewer)
    { }

    bool trigger() override;
};

class Heat_turned_up_event : public Event
{
public:
    Heat_turned_up_event(Core & core, My_viewer & viewer) : Event(core, viewer)
    { }

    bool trigger() override;

private:
    float _last_time_too_low;
};


#endif // EVENT_H