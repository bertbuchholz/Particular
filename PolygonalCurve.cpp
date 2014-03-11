#include "PolygonalCurve.h"


int Polygonal_curve::add_vertex_at_back(const Polygonal_curve::Point &vertex)
{
    _vertices.push_back(vertex);
    return _vertices.size() - 1;
}


int Polygonal_curve::addVertexAtFront(const Polygonal_curve::Point &vertex)
{
    _vertices.insert(_vertices.begin(), vertex);
    return _vertices.size() - 1;
}


void Polygonal_curve::insert_vertex(const int offset, const Polygonal_curve::Point &vertex)
{
    _vertices.insert(_vertices.begin() + offset, vertex);
}


Polygonal_curve::Point Polygonal_curve::get_pos_on_curve(const float s) const
{
    // std::cout << "PolygonalCurve::getPosOnCurve(): " << _lengthsAtPoints.size() << " " << s << std::endl;
    assert(s >= 0.0f && s <= 1.0f);

    if (s < 0.0001f) return _vertices.front();

    int i = std::lower_bound(_lengths_at_points.begin(), _lengths_at_points.end(), _length * s) - _lengths_at_points.begin();

    if (i > int(_vertices.size()) - 1) return _vertices.back();

    assert(i < int(_vertices.size()));

    float s0 = get_uniform_length_at_vertex(i - 1);
    float s1 = get_uniform_length_at_vertex(i);

    assert(s >= s0 && s <= s1);

    float offsetFromS1 = (s1 - s) / (s1 - s0);

    return (_vertices[i - 1] * offsetFromS1 + _vertices[i] * (1.0f - offsetFromS1));
}


Polygonal_curve::Point Polygonal_curve::get_pos_on_curve_y_over_x(const float s) const
{
    // std::cout << "PolygonalCurve::getPosOnCurve(): " << _lengthsAtPoints.size() << " " << s << std::endl;
    assert(s >= 0.0f && s <= 1.0f);

    if (s < 0.0001f) return _vertices.front();

    int i = std::lower_bound(_lengths_at_points_y_over_x.begin(), _lengths_at_points_y_over_x.end(), _length_y_over_x * s) - _lengths_at_points_y_over_x.begin();

    if (i > int(_vertices.size()) - 1) return _vertices.back();

    assert(i < int(_vertices.size()));

    float s0 = get_uniform_length_at_vertex_y_over_x(i - 1);
    float s1 = get_uniform_length_at_vertex_y_over_x(i);

    assert(s >= s0 && s <= s1);

    float offsetFromS1 = (s1 - s) / (s1 - s0);

    return (_vertices[i - 1] * offsetFromS1 + _vertices[i] * (1.0f - offsetFromS1));
}


void Polygonal_curve::get_vertex_and_offset_at_length(float s, int &vertexIndex, float &offset) const
{
    // std::cout << "PolygonalCurve::getPosOnCurve(): " << _lengthsAtPoints.size() << " " << s << std::endl;
    assert(s >= 0.0f && s <= 1.0f);

    vertexIndex = 0;
    offset = 0.0f;

    if (s < 0.0001f) return;

    while (vertexIndex < int(_vertices.size()) - 1 && get_uniform_length_at_vertex(vertexIndex) <= s)
    {
        ++vertexIndex;
    }

    if (vertexIndex >= int(_vertices.size()) - 1)
    {
        vertexIndex = _vertices.size() - 1;
        return;
    }

    assert(vertexIndex < int(_vertices.size()));

    float s0 = get_uniform_length_at_vertex(vertexIndex - 1);
    float s1 = get_uniform_length_at_vertex(vertexIndex);

    assert(s >= s0 && s <= s1);

    // offset as seen from s0
    offset = 1.0f - ((s1 - s) / (s1 - s0));
}


void Polygonal_curve::get_vertex_and_offset_at_length_y_over_x(float s, int & vertexIndex, float & offset) const
{
    // std::cout << "PolygonalCurve::getPosOnCurve(): " << _lengthsAtPoints.size() << " " << s << std::endl;
    assert(s >= 0.0f && s <= 1.0f);

    vertexIndex = 0;
    offset = 0.0f;

    if (s < 0.0001f) return;

    while (vertexIndex < int(_vertices.size()) - 1 && get_uniform_length_at_vertex_y_over_x(vertexIndex) <= s)
    {
        ++vertexIndex;
    }

    if (vertexIndex >= int(_vertices.size()) - 1)
    {
        vertexIndex = _vertices.size() - 1;
        return;
    }

    assert(vertexIndex < int(_vertices.size()));

    float s0 = get_uniform_length_at_vertex_y_over_x(vertexIndex - 1);
    float s1 = get_uniform_length_at_vertex_y_over_x(vertexIndex);

    assert(s >= s0 && s <= s1);

    // offset as seen from s0
    offset = 1.0f - ((s1 - s) / (s1 - s0));
}


int Polygonal_curve::getClosestVertexIndexOnCurve(float s) const
{
    // std::cout << "PolygonalCurve::getPosOnCurve(): " << _lengthsAtPoints.size() << " " << s << std::endl;
    assert(s >= 0.0f && s <= 1.0f);

    if (s < 0.0001f) return 0;

    unsigned int i = 0;
    while (i < _vertices.size() - 1 && get_uniform_length_at_vertex(i) <= s)
    {
        ++i;
    }

    if (i >= _vertices.size() - 1) return _vertices.size() - 1;

    assert(i < _vertices.size());

    float s0 = get_uniform_length_at_vertex(i - 1);
    float s1 = get_uniform_length_at_vertex(i);

    assert(s >= s0 && s <= s1);

    float offsetFromS1 = (s1 - s) / (s1 - s0);

    if (offsetFromS1 > 0.5f) return (i - 1);

    return i;

    // return (vertices.at(i - 1).getPos() * offsetFromS1 + vertices.at(i).getPos() * (1.0f - offsetFromS1));
}

Polygonal_curve::Point Polygonal_curve::compute_tangent(const float s) const
{
    int index;
    float offset;

    get_vertex_and_offset_at_length(s, index, offset);

    if (index == 0)
    {
        return (_vertices[1] - _vertices[0]).normalized();
    }

    return (_vertices[index] - _vertices[index - 1]).normalized();
}


void Polygonal_curve::finish()
{
    calcLength();
}


void Polygonal_curve::calcLength()
{
    _lengths_at_points.clear();

    _length = 0.0f;
    _lengths_at_points.push_back(0.0f);

    _length_y_over_x = 0.0f;
    _lengths_at_points_y_over_x.push_back(0.0f);

    for (unsigned int i = 1; i < _vertices.size(); ++i)
    {
        _length += (_vertices[i] - _vertices[i - 1]).norm();
        _lengths_at_points.push_back(_length);

        _length_y_over_x += _vertices[i][0] - _vertices[i - 1][0];
        _lengths_at_points_y_over_x.push_back(_length_y_over_x);
    }

    assert(_lengths_at_points.size() == _vertices.size());
}


float Polygonal_curve::get_length() const
{
    return _length;
}


float Polygonal_curve::get_length_y_over_x() const
{
    return _length_y_over_x;
}


float Polygonal_curve::get_uniform_length_at_vertex(const int index) const
{
    return _lengths_at_points[index] / _length;
}

float Polygonal_curve::get_uniform_length_at_vertex_y_over_x(const int index) const
{
    return _lengths_at_points_y_over_x[index] / _length_y_over_x;
}

float Polygonal_curve::get_absolute_length_at_vertex(const int index) const
{
    return _lengths_at_points[index];
}

float Polygonal_curve::get_absolute_length_at_uniform_length(const float s) const
{
    int index;
    float offset;

    get_vertex_and_offset_at_length(s, index, offset);

    if (index == _lengths_at_points.size() - 1)
    {
        return _length;
    }

    return _lengths_at_points[index] * offset + _lengths_at_points[index + 1] * (1.0f - offset);
}


float Polygonal_curve::get_absolute_length_at_uniform_length_y_over_x(const float s) const
{
    int index;
    float offset;

    get_vertex_and_offset_at_length_y_over_x(s, index, offset);

    if (index == _lengths_at_points_y_over_x.size() - 1)
    {
        return _length_y_over_x;
    }

    return _lengths_at_points_y_over_x[index] * offset + _lengths_at_points_y_over_x[index + 1] * (1.0f - offset);
}


float Polygonal_curve::distance_between_vertices(int index1, int index2) const
{
    float l1 = _lengths_at_points[index1];
    float l2 = _lengths_at_points[index2];

    if (l1 > l2)
    {
        std::swap(l1, l2);
    }

    float d1 = l2 - l1;

    return d1;
}


void Polygonal_curve::clear()
{
    _vertices.clear();
    _lengths_at_points.clear();
}
