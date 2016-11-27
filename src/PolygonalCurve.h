#ifndef POLYGONALCURVE_H
#define POLYGONALCURVE_H

#include <vector>\

#include <Eigen/Core>

class Polygonal_curve
{
public:
    typedef Eigen::Vector3f Point;

    int add_vertex_at_back(const Point& vertex);

    int addVertexAtFront(const Point& vertex);

    void insert_vertex(int const offset, const Point& vertex);

    // return the 3d point on the curve at uniform curve length s
    Point get_pos_on_curve(float const s) const;
    Point get_pos_on_curve_y_over_x(const float s) const;

    // return the point on the curve at uniform curve length s
    void get_vertex_and_offset_at_length(float s, int & vertexIndex, float & offset) const;
    void get_vertex_and_offset_at_length_y_over_x(float s, int &vertexIndex, float &offset) const;

    // return the point on the curve at uniform curve length s
    int getClosestVertexIndexOnCurve(float s) const;

    Point compute_tangent(float const s) const;

    void finish();

    void calcLength();

    float get_length() const;
    float get_length_y_over_x() const;

    float get_uniform_length_at_vertex(int const index) const;
    float get_uniform_length_at_vertex_y_over_x(const int index) const;

    float get_absolute_length_at_vertex(int const index) const;
    float get_absolute_length_at_uniform_length(float const s) const;
    float get_absolute_length_at_uniform_length_y_over_x(float const s) const;

    float distance_between_vertices(int index1, int index2) const;

    std::vector<Point> const& get_vertices() const { return _vertices; }

    std::vector<Point> & get_vertices() { return _vertices; }

    void clear();

private:
    std::vector<Point> _vertices;

    std::vector<float> _lengths_at_points;
    float _length;

    std::vector<float> _lengths_at_points_y_over_x; // assume a 2d curve
    float _length_y_over_x;
};


#endif // POLYGONALCURVE_H
