#include "Score.h"

#include <cmath>
#include <cassert>

#include <Utilities.h>

Color4 Score::score_color        = Color4(147 / 255.0f, 232 / 255.0f, 112 / 255.0f, 1.0f);
Color4 Score::energy_bonus_color = Color4(124 / 255.0f, 157 / 255.0f, 255 / 255.0f, 1.0f);
Color4 Score::score_red          = Color4(255 / 255.0f, 121 / 255.0f,  54 / 255.0f, 1.0f);
Color4 Score::score_green        = Color4(147 / 255.0f, 232 / 255.0f, 112 / 255.0f, 1.0f);

void Score::calculate_score(const float time_factor, const int num_molecules_to_capture_)
{
    float score = 0.0f;

    full_time = sensor_data.get_data(Sensor_data::Type::ColMol).size() * sensor_data.get_check_interval();

    num_molecules_to_capture = num_molecules_to_capture_;

    int const points_per_molecule = int(1e6 / num_molecules_to_capture);

    int highest_num_captured = 0;

    std::vector<float> const& collected_molecules = sensor_data.get_data(Sensor_data::Type::ColMol);

    for (size_t i = 0; i < collected_molecules.size(); ++i)
    {
        float const current_time = i * sensor_data.get_check_interval();

        int const current_captured = int(collected_molecules[i]);

        if (current_captured > highest_num_captured)
        {
            highest_num_captured = current_captured;

            int const new_score = int(points_per_molecule * get_score_multiplier(current_time, time_factor) / 100) * 100;
            score += new_score;
            score_at_time.push_back(std::pair<float, int>(current_time, new_score));
        }
    }

    std::vector<float> energy_values = sensor_data.get_data(Sensor_data::Type::EnergyCon);

    // energy of 1 or less gets no penalty, energy over 1 gets a penalty of up to 95% of the current time's score possibility
    // 95% equals energy == 5

//    float avg_penalty = 0.0f;
    int penalty_sum = 0;

    for (size_t i = 0; i < energy_values.size(); ++i)
    {
        float const current_time = i * sensor_data.get_check_interval();

        float const penalty = into_range(energy_values[i] - 1.0f, 0.0f, 4.0f) / 4.0f * 0.95f * get_score_multiplier(current_time, time_factor);
//        avg_penalty += penalty;
        int const discrete_penalty = int(penalty * score / (energy_values.size() * 100)) * 100;
        penalty_sum += discrete_penalty;

        penalty_at_time.push_back(std::pair<float, int>(current_time, discrete_penalty));
    }

//    avg_penalty /= float(energy_values.size());

//    {
//        float sum = 0.0f;

//        for (auto p : penalty_at_time)
//        {
//            sum += p.second;
//        }

//        assert(std::abs(sum - avg_penalty * score) < avg_penalty * score * 0.01f);
//    }

    final_score = int(score);
    _penalty = penalty_sum;
    _energy_bonus = sensor_data.get_energy_bonus().back();
}

std::vector<std::pair<float, int> > const& Score::get_score_at_time() const
{
    return score_at_time;
}
