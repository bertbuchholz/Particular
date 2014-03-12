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
    }

    void add_value(Type const type, float const value)
    {
        _data[int(type)].push_back(value);
    }

    std::vector<float> const& get_data(Type const type) const
    {
        return _data[int(type)];
    }

    void clear()
    {
        for (auto & v : _data)
        {
            v.clear();
        }
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

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_data);
    }

private:
    std::vector< std::vector<float> > _data;

    std::vector<float> _cumulative_energy;
    std::vector<float> _energy_score;

    float _game_field_volume;
    float _check_interval;
};

#endif // SENSOR_DATA_H
