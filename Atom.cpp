#include "Atom.h"

#include <Color.h>

std::vector<Color> Atom::atom_colors =
{
    Color(0.3f, 0.2f, 0.7f), // Atom::Type::Charge
    Color(1.0f, 1.0f, 1.0f), // Atom::Type::H
    Color(0.9f, 0.2f, 0.2f), // Atom::Type::O
    Color(0.3f, 0.3f, 0.4f), // Atom::Type::C
    Color(0.8f, 0.7f, 0.2f), // Atom::Type::S
    Color(0.8f, 0.3f, 0.8f), // Atom::Type::N
    Color(0.8f, 0.3f, 0.8f), // Atom::Type::Na
    Color(0.5f, 0.8f, 0.3f)  // Atom::Type::Cl
};

std::unordered_map< std::string, std::function<Molecule(Eigen::Vector3f const& position)> > Molecule::_molecule_factory_map =
{
    {"O2",     Molecule::create_oxygen},
    {"H2O",    Molecule::create_water },
    {"SDS",    Molecule::create_sulfate},
    {"Na",     Molecule::create_charged_natrium},
    {"Cl",     Molecule::create_charged_chlorine},
    {"Dipole", Molecule::create_dipole}
};
