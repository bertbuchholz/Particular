#ifndef PROGRESS_H
#define PROGRESS_H

#include <vector>

#include "Score.h"

struct Progress
{
    Progress() : last_level(0), sandbox_warning_seen(false)
    { }

    void reset()
    {
        last_level = 0;
    }

    int last_level;
    std::map< std::string, std::vector<Score> > scores; // level name to scores
    bool sandbox_warning_seen;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(last_level);
        ar & BOOST_SERIALIZATION_NVP(scores);

        if (version > 1)
        {
            ar & BOOST_SERIALIZATION_NVP(sandbox_warning_seen);
        }
    }
};

BOOST_CLASS_VERSION(Progress, 2)

#endif // PROGRESS_H
