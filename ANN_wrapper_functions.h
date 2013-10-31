#ifndef ANN_WRAPPER_H
#define ANN_WRAPPER_H

#include <vector>
#include <cassert>
#include <ANN/ANN.h>

#include "Atom.h"

class ANN_wrapper
{
public:
    ANN_wrapper() : _tree(nullptr), _dataPts(nullptr)
    { }

    ~ANN_wrapper()
    {
        if (_tree)
        {
            annDeallocPts(_dataPts);
            delete _tree;
        }
    }

    void generate_tree_from_molecules(std::list<Molecule> const& molecules);
    std::vector<Atom const*> find_closest_atoms(Atom const& atom) const;

private:
    ANNkd_tree * _tree;
    ANNpointArray _dataPts;

    std::vector<Atom const*> _index_to_atom;
};

#endif
