#include "Atom.h"

std::unordered_map< std::string, std::function<Molecule(Eigen::Vector3f const& position)> > Molecule::_molecule_factory_map =
{
    {"O2",     Molecule::create_oxygen},
    {"H2O",    Molecule::create_water },
    {"SDS",    Molecule::create_sulfate},
    {"Na",     Molecule::create_charged_natrium},
    {"Cl",     Molecule::create_charged_chlorine},
    {"Dipole", Molecule::create_dipole}
};
