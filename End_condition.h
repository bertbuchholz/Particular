#ifndef END_CONDITION_H
#define END_CONDITION_H

#include "Atom.h"

class End_condition
{
public:
    enum class Type { And, Or };

    enum class State { Not_finished, Finished };

    virtual ~End_condition() {}

    virtual State check_state(std::vector<Molecule> const& /*molecules*/) const = 0;

    void set_type(Type type)
    {
        _type = type;
    }

    Type get_type() const
    {
        return _type;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & _type;
    }

private:
    Type _type;
};

class Molecule_capture_condition : public End_condition
{
public:
    Molecule_capture_condition() {}

    Molecule_capture_condition(int const min_captured_molecules) :
        _min_captured_molecules(min_captured_molecules)
    { }

    State check_state(std::vector<Molecule> const& /*molecules*/) const override
    {
        if (_num_captured_molecules < _min_captured_molecules) return State::Not_finished;

        return State::Finished;
    }

    void set_min_captured_molecules(int const min_captured_molecules)
    {
        _min_captured_molecules = min_captured_molecules;
    }

    int get_min_captured_molecules() const
    {
        return _min_captured_molecules;
    }

    void set_num_captured_molecules(int const num_captured_molecules)
    {
        _num_captured_molecules = num_captured_molecules;
    }

    int get_num_captured_molecules() const
    {
        return _num_captured_molecules;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<End_condition>(*this);
        ar & _min_captured_molecules;
    }

private:
    int _min_captured_molecules;
    int _num_captured_molecules;
};

class Molecule_count_condition : public End_condition
{
public:
    Molecule_count_condition() {}

    Molecule_count_condition(float const min_ratio_active, int const min_num_molecules) :
        _min_ratio_active(min_ratio_active), _min_num_molecules(min_num_molecules)
    { }

    State check_state(std::vector<Molecule> const& molecules) const override
    {
        if (_min_num_molecules < int(molecules.size())) return State::Not_finished;

        int num_active_molecules = 0;

        for (Molecule const& m : molecules)
        {
            if (m._active)
            {
                ++num_active_molecules;
            }
        }

        float const ratio = num_active_molecules / float(molecules.size());

        if (ratio < _min_ratio_active)
        {
            return State::Finished;
        }
        else
        {
            return State::Not_finished;
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & boost::serialization::base_object<End_condition>(*this);
        ar & _min_ratio_active;
        ar & _min_num_molecules;
    }

private:
    float _min_ratio_active;
    int _min_num_molecules;
};

#endif // END_CONDITION_H
