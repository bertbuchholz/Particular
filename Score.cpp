#include "Score.h"

#include <cmath>
#include <cassert>

int Score::calculate_score(const float time_factor, const int num_molecules_to_capture)
{
    float score = 0.0f;

    full_time = sensor_data.get_data(Sensor_data::Type::ColMol).size() * sensor_data.get_check_interval();

//    float const max_power = _game_field_volume * 50.0f;

//    _cumulative_energy.clear();
//    _cumulative_energy.push_back(0.0f);

//    for (size_t i = 1; i < _data[int(Type::EnergyCon)].size(); ++i)
//    {
//        _cumulative_energy.push_back(_cumulative_energy.back() + _data[int(Type::EnergyCon)][i] * _check_interval);
//    }

    _num_molecules_to_capture = num_molecules_to_capture;

    int const points_per_molecule = 1e6 / _num_molecules_to_capture;

    int highest_num_captured = 0;

    std::vector<float> const& collected_molecules = sensor_data.get_data(Sensor_data::Type::ColMol);

    for (size_t i = 0; i < collected_molecules.size(); ++i)
    {
        float const current_time = i * sensor_data.get_check_interval();

        int const current_captured = collected_molecules[i];

        if (current_captured > highest_num_captured)
        {
            highest_num_captured = current_captured;

            if (current_time < time_factor)
            {
                score_at_time.push_back(std::pair<float, int>(current_time, points_per_molecule));
                score += points_per_molecule;
            }
            else
            {
                int const new_score = points_per_molecule * std::exp(-current_time / time_factor);
                score += new_score;
                score_at_time.push_back(std::pair<float, int>(current_time, new_score));
            }
        }

    }

    return score;
}

std::vector<std::pair<float, int> > const& Score::get_score_at_time() const
{
    return score_at_time;
}
