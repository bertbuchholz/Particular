#ifndef PROGRESS_H
#define PROGRESS_H

#include <vector>

#include "Score.h"

struct Progress
{
    Progress() : last_level(0)
    { }

    int last_level;
    std::map< std::string, std::vector<Score> > scores; // level name to scores

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* version */)
    {
        ar & BOOST_SERIALIZATION_NVP(last_level);
        ar & BOOST_SERIALIZATION_NVP(scores);
    }
};

#endif // PROGRESS_H
