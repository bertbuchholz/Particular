#ifndef SCORE_H
#define SCORE_H

#include "Sensor_data.h"

struct Score
{
    int final_score;
    Sensor_data sensor_data;

    static bool score_comparer(Score const& score1, Score const& score2)
    {
        return score1.final_score < score2.final_score;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(final_score);
        ar & BOOST_SERIALIZATION_NVP(sensor_data);
    }
};

BOOST_CLASS_VERSION(Score, 1)

#endif // SCORE_H
