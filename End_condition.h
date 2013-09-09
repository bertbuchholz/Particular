#ifndef END_CONDITION_H
#define END_CONDITION_H

#include "Atom.h"

class End_condition
{
public:
    enum class Type { And, Or };

    enum class State { Not_finished, Finished };

    End_condition() : _type(Type::Or)
    { }

    virtual ~End_condition() {}

    virtual State check_state(std::list<Molecule> const& /*molecules*/) const = 0;

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
        ar & BOOST_SERIALIZATION_NVP(_type);
    }

private:
    Type _type;
};

class Molecule_capture_condition : public End_condition
{
public:
    Molecule_capture_condition() : _min_captured_molecules(10), _num_captured_molecules(0)
    { }

    Molecule_capture_condition(int const min_captured_molecules) :
        _min_captured_molecules(min_captured_molecules), _num_captured_molecules(0)
    { }

    State check_state(std::list<Molecule> const& /*molecules*/) const override
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
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(End_condition);
        ar & BOOST_SERIALIZATION_NVP(_min_captured_molecules);
    }

private:
    int _min_captured_molecules;
    int _num_captured_molecules;
};

#endif // END_CONDITION_H
