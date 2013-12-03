#include "Help_screen.h"

#include "Core.h"
#include "My_viewer.h"

Help_screen::Help_screen(My_viewer &viewer, Core &core, Screen *calling_state) : Menu_screen(viewer, core), _calling_screen(calling_state)
{
    _type = Screen::Type::Modal;

    init();

    _picking.init(_viewer.context());
}

void Help_screen::init()
{

}
