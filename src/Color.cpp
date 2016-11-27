#include <iostream>
#include "Color.h"


std::ostream & operator << (std::ostream & out, Color const& c)
{
	out<<"["<<c.R<<" "<<c.G<<" "<<c.B<<"]";
	return out;
}

