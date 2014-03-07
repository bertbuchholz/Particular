#ifndef SCORE_H
#define SCORE_H

#include "Sensor_data.h"

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>
#endif

struct Score
{
    int final_score;
    float full_time;
    Sensor_data sensor_data;
    std::vector< std::pair<float, int> > score_at_time;
    std::vector<float> penalty_at_time;
    int _num_molecules_to_capture;

    static bool score_comparer(Score const& score1, Score const& score2)
    {
        return score1.final_score < score2.final_score;
    }

    int calculate_score(float const time_factor, int const num_molecules_to_capture);

    std::vector< std::pair<float, int> > const& get_score_at_time() const;

    float get_full_time() const { return full_time; }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(final_score);
        ar & BOOST_SERIALIZATION_NVP(sensor_data);
    }
};

BOOST_CLASS_VERSION(Score, 1)

#endif // SCORE_H
