#ifndef GEOMETRY_UTILS_H
#define GEOMETRY_UTILS_H

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <QGLViewer/vec.h>
#include <glm/glm.hpp>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

inline Eigen::Vector3f cross(Eigen::Vector3f const& v_0, Eigen::Vector3f const& v_1)
{
    return v_0.cross(v_1);
}

inline OpenMesh::Vec3f QGLV2OM(qglviewer::Vec const& p)
{
    return OpenMesh::Vec3f(p.x, p.y, p.z);
}

inline QVector3D QGLV2QVec(qglviewer::Vec const& p)
{
    return QVector3D(p.x, p.y, p.z);
}

inline qglviewer::Vec OM2QGLV(OpenMesh::Vec3f const& p)
{
    return qglviewer::Vec(p[0], p[1], p[2]);
}

inline qglviewer::Vec Eigen2QGLV(Eigen::Vector3f const& p)
{
    return qglviewer::Vec(p[0], p[1], p[2]);
}

inline Eigen::Vector3f QGLV2Eigen(qglviewer::Vec const& p)
{
    return Eigen::Vector3f(p.x, p.y, p.z);
}

inline OpenMesh::Vec3f Eigen2OM(Eigen::Vector3f const& p)
{
    return OpenMesh::Vec3f(p[0], p[1], p[2]);
}

inline Eigen::Vector3f OM2Eigen(OpenMesh::Vec3f const& p)
{
    return Eigen::Vector3f(p[0], p[1], p[2]);
}

inline Eigen::Vector3f glm2Eigen(glm::vec3 const& p)
{
    return Eigen::Vector3f(p[0], p[1], p[2]);
}

inline glm::vec3 Eigen2glm(Eigen::Vector3f const& p)
{
    return glm::vec3(p[0], p[1], p[2]);
}

inline QVector2D Eigen2QVec(Eigen::Vector2f const& p)
{
    return QVector2D(p[0], p[1]);
}

inline QVector3D Eigen2QVec(Eigen::Vector3f const& p)
{
    return QVector3D(p[0], p[1], p[2]);
}

inline QVector4D Eigen2QVec(Eigen::Vector4f const& p)
{
    return QVector4D(p[0], p[1], p[2], p[3]);
}

inline float ray_plane_intersection(OpenMesh::Vec3f const& ray_origin, OpenMesh::Vec3f const& ray_dir, OpenMesh::Vec3f const& plane_position, OpenMesh::Vec3f const& plane_normal)
{
    return OpenMesh::dot(plane_normal, (plane_position - ray_origin)) / OpenMesh::dot(ray_dir, plane_normal);
}

Eigen::ParametrizedLine<float, 3> plane_plane_intersection(Eigen::Hyperplane<float, 3> const& p1, Eigen::Hyperplane<float, 3> const& p2);

template <class Vector>
void create_orthogonal_cs(const Vector &N, Vector &T, Vector &B)
{
    if ((N[0] == 0) && (N[1] == 0))
    {
        if (N[2] < 0)
            T = Vector(-1, 0, 0);
        else
            T = Vector(1, 0, 0);

        B = Vector(0, 1, 0);
    }
    else
    {
        // Note: The root cannot become zero if
        // N.x==0 && N.y==0.
        const float d = 1.0 / std::sqrt(N[1] * N[1] + N[0] * N[0]);
        T = Vector(N[1] * d, -N[0] * d, 0.0f);
        B = cross(N, T);
    }
}

template <class Point>
int get_longest_axis(Point const& dir)
{
    int longest_axis = 0;
    if (std::abs(dir[1]) > std::abs(dir[0]) && std::abs(dir[1]) > std::abs(dir[2])) longest_axis = 1;
    else if (std::abs(dir[2]) > std::abs(dir[0])) longest_axis = 2;
    return longest_axis;
}

template <class Point>
int get_secondary_axis(Point const& dir, int const longest_axis)
{
    int secondary_axis = (longest_axis + 1) % 3;
    if (std::abs(dir[secondary_axis]) > std::abs(dir[(secondary_axis + 1) % 3])) return secondary_axis;

    return (secondary_axis + 1) % 3;
}

#endif // GEOMETRY_UTILS_H
