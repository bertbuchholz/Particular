#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <boost/serialization/shared_ptr.hpp>

#include "Atom.h"
#include "Level_elements.h"
#include "End_condition.h"

class Level_data
{
public:
    enum class Plane { Neg_X = 0, Neg_Y, Neg_Z, Pos_X, Pos_Y, Pos_Z };

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
    //        ar & _molecules;
        ar & _game_field_borders;
        ar & _barriers;
        ar & _portals;
        ar & _molecule_releasers;
        ar & _brownian_elements;
        ar & _level_elements;
        ar & _end_conditions;
    }

    std::list<Molecule> _molecules;

    std::map<Plane, Plane_barrier*> _game_field_borders;

    std::vector<Barrier*> _barriers;
    std::vector<Portal*> _portals;
    std::vector<Brownian_element*> _brownian_elements;
    std::vector<Molecule_releaser*> _molecule_releasers;
    std::vector<Particle_system_element*> _particle_system_elements;

//    std::vector<Level_element*> _level_elements;
    std::vector< boost::shared_ptr<Level_element> > _level_elements;

    std::vector<End_condition*> _end_conditions;
};



#endif // LEVEL_DATA_H
