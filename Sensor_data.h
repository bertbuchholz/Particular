#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <vector>

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#endif

#include <Parameter.h>

class Sensor_data
{
public:
    enum class Type { ColMol = 0, RelMol, AvgTemp, EnergyCon };

    Sensor_data() : _check_interval(0.2f)
    {
        _data.resize(4);
        _energy_bonus.push_back(500000);
    }

    void add_value(Type const type, float const value)
    {
        _data[int(type)].push_back(value);
    }

    std::vector<float> const& get_data(Type const type) const
    {
        return _data[int(type)];
    }

    void update_energy_bonus()
    {
        float const energy_consumption = _data[int(Type::EnergyCon)].back();

        int const penalty_per_second = 5000;

        int const penalty = std::round(penalty_per_second * std::max(0.0f, energy_consumption - 1.0001f) * _check_interval / 100.0f) * 100;

        int const new_energy_bonus = std::max(0, _energy_bonus.back() - penalty);

        _energy_bonus.push_back(new_energy_bonus);
    }

    void clear()
    {
        for (auto & v : _data)
        {
            v.clear();
        }

        _energy_bonus.clear();
        _energy_bonus.push_back(500000);
    }

    int get_num_data_types() const
    {
        return int(_data.size());
    }

    float get_check_interval() const
    {
        return _check_interval;
    }

    void set_game_field_volume(float const volume)
    {
        _game_field_volume = volume;
    }

    std::vector<int> const& get_energy_bonus() const
    {
        return _energy_bonus;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_data);
    }

private:
    std::vector< std::vector<float> > _data;

    std::vector<int> _energy_bonus;

    float _game_field_volume;
    float _check_interval;
};

#endif // SENSOR_DATA_H
