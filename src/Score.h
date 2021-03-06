#ifndef SCORE_H
#define SCORE_H

#include "Sensor_data.h"

#include <cmath>

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>
#endif

#include "Color.h"

struct Score
{
    int final_score;
    float full_time;
    Sensor_data sensor_data;
    std::vector< std::pair<float, int> > score_at_time;
    std::vector< std::pair<float, int> > penalty_at_time;
    int num_molecules_to_capture;
    int _penalty;
    int _energy_bonus;

    static Color4 score_color;
    static Color4 energy_bonus_color;
    static Color4 score_green;
    static Color4 score_red;

    static bool score_comparer(Score const& score1, Score const& score2)
    {
        return score1.final_score < score2.final_score;
    }

    void calculate_score(float const time_factor, int const num_molecules_to_capture);

    std::vector< std::pair<float, int> > const& get_score_at_time() const;

    float get_full_time() const { return full_time; }

    static float get_score_multiplier(float const time, const float time_threshold)
    {
        if (time < time_threshold)
        {
            return 1.0f;
        }

        return std::exp(-(time - time_threshold) / time_threshold);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(final_score);
        ar & BOOST_SERIALIZATION_NVP(full_time);
        ar & BOOST_SERIALIZATION_NVP(sensor_data);
        ar & BOOST_SERIALIZATION_NVP(score_at_time);
        ar & BOOST_SERIALIZATION_NVP(penalty_at_time);
        ar & BOOST_SERIALIZATION_NVP(num_molecules_to_capture);
        ar & BOOST_SERIALIZATION_NVP(_penalty);
        ar & BOOST_SERIALIZATION_NVP(_energy_bonus);
    }
};

BOOST_CLASS_VERSION(Score, 1)

#endif // SCORE_H
