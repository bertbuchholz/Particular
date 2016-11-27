#include "Geometry_utils.h"

Eigen::ParametrizedLine<float, 3> plane_plane_intersection(const Eigen::Hyperplane<float, 3> &p1, const Eigen::Hyperplane<float, 3> &p2)
{
    Eigen::Vector3f direction = p1.normal().cross(p2.normal());

    // calc x and z -> coeffs 0 (x), 2 (z), 3 (d)

    float const x = (p1.coeffs()[2] * p2.coeffs()[3] - p2.coeffs()[2] * p1.coeffs()[3]) /
            (p1.coeffs()[0] * p2.coeffs()[2] - p2.coeffs()[0] * p1.coeffs()[2]);

    float const z = (p2.coeffs()[0] * p1.coeffs()[3] - p1.coeffs()[0] * p2.coeffs()[3]) /
            (p1.coeffs()[0] * p2.coeffs()[2] - p2.coeffs()[0] * p1.coeffs()[2]);

    Eigen::Vector3f origin(x, 0.0f, z);

    return Eigen::ParametrizedLine<float, 3>(origin, direction);
}
