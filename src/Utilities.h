#ifndef UTILITIES_H
#define UTILITIES_H

#include <Eigen/Core>

#ifdef WIN32
#define __func__ __FUNCTION__
#endif

#ifdef __linux__
#define __func__ __PRETTY_FUNCTION__
#endif

template <typename T>
T into_range(T const& value, T const& min, T const& max)
{
    return std::min(max, std::max(min, value));
}

template <typename T>
bool is_in_range(T const& value, T const& min, T const& max)
{
    return (value >= min && value <= max);
}

template <typename T>
T square(T const& value)
{
    return value * value;
}

inline float wendland_2_1(float const x)
{
    float a = 1.0f - x;
    a = a * a * a; // (1-x)**3
    return a * (3.0f * x + 1.0f);
}

//float dot(Eigen::Vector3f const& v1, Eigen::Vector3f const& v2)
//{
//    return v1.dot(v2);
//}

#endif // UTILITIES_H
