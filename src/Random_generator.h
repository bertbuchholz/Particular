#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <random>
#include <Eigen/Core>

struct Random_generator
{
    Random_generator()
    {
//        mt19937::result_type seed = time(0);
        std::mt19937::result_type seed = 0;
        _generator = std::minstd_rand(seed);
        _distribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    }

    Eigen::Vector3f generator_unit_vector()
    {
        return Eigen::Vector3f(_distribution(_generator), _distribution(_generator), _distribution(_generator)).normalized();
    }


//    std::mt19937 _generator;
    std::minstd_rand _generator;
    std::uniform_real_distribution<float> _distribution;
};

#endif // RANDOM_GENERATOR_H
