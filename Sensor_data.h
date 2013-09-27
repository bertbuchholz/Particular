#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <vector>

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
        return _data.size();
    }

    float get_check_interval()
    {
        return _check_interval;
    }

    int calculate_score(float const time_factor) const
    {
        float score = 0.0f;

        for (size_t i = 0; i < _data[int(Type::ColMol)].size(); ++i)
        {
            score += std::exp(-i / time_factor) * _data[int(Type::ColMol)][i];
        }

        score *= _check_interval;

        return score * 100;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(_data);
    }

private:
    std::vector< std::vector<float> > _data;
    float _check_interval;
};

#endif // SENSOR_DATA_H
