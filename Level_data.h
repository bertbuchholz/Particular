#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include "Atom.h"
#include "Level_elements.h"
#include "End_condition.h"

class Level_data
{
public:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
    //        ar & _molecules;
        ar & _game_field_borders;
        ar & _barriers;
        ar & _portals;
        ar & _molecule_releasers;
        ar & _end_conditions;
    }

    std::vector<Molecule> _molecules;

    std::vector<Barrier*> _game_field_borders;

    std::vector<Barrier*> _barriers;
    std::vector<Portal*> _portals;
    std::vector<Brownian_element*> _brownian_elements;
    std::vector<Molecule_releaser*> _molecule_releasers;

    std::vector<End_condition*> _end_conditions;
};



#endif // LEVEL_DATA_H