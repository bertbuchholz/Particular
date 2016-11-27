#include "Picking.h"

Picking* Picking::instance = NULL;


void set_pick_index(int index)
{
    Picking::getInstance()->set_index(index);

}
