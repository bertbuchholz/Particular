#include "ANN_wrapper_functions.h"

void ANN_wrapper::generate_tree_from_molecules(std::list<Molecule> const& molecules)
{
    if (molecules.size() == 0) return;

    int const dim = 3; // dimension
    int num_atoms = 0;

    for (Molecule const& m : molecules)
    {
        num_atoms += m._atoms.size();
    }

    _index_to_atom.resize(num_atoms);

    _dataPts = annAllocPts(num_atoms, dim); // allocate data points

    int atom_index = 0;

    for (Molecule const& m : molecules)
    {
        for (Atom const& a : m._atoms)
        {
            _index_to_atom[atom_index] = &a;

            for (size_t i = 0; i < 3; ++i)
            {
                _dataPts[atom_index][i] = a.get_position()[i];
            }

            ++atom_index;
        }

    }

    assert(atom_index == num_atoms);

    _tree = new ANNkd_tree(_dataPts, // the data points
                            num_atoms, // number of points
                            dim); // dimension of space
}

std::vector<const Atom *> ANN_wrapper::find_closest_atoms(Atom const& atom) const
{
    int const k = std::min(15, int(_index_to_atom.size())); // number of nearest neighbors
    int const dim = 3; // dimension
    double const eps = 0; // error boun

    ANNpoint queryPt; // query point
    ANNidxArray nnIdx; // near neighbor indices
    ANNdistArray dists; // near neighbor distances

    queryPt = annAllocPt(dim); // allocate query point

    for (size_t i = 0; i < 3; ++i)
    {
        queryPt[i] = atom.get_position()[i];
    }

//    std::vector<int> neighbor_indices;
    nnIdx = new ANNidx[k]; // allocate near neigh indices
    dists = new ANNdist[k]; // allocate near neighbor dists

    _tree->annkSearch(queryPt, // query point
                      k, // number of near neighbors
                      nnIdx, // nearest neighbors (returned)
//                      neighbor_indices.data(), // nearest neighbors (returned)
                      dists, // distance (returned)
                      eps); // error boun

    std::vector<const Atom *> closest_atoms;

    for (int i = 0; i < k; ++i)
    {
        Atom const* close_atom = _index_to_atom[nnIdx[i]];

        if (atom._parent_id != close_atom->_parent_id)
        {
            closest_atoms.push_back(close_atom);
        }
    }

    delete [] nnIdx;
    delete [] dists;

    annDeallocPt(queryPt);

//    return neighbor_indices;
    return closest_atoms;

}
