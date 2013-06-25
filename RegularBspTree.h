#ifndef REGULARBSPTREE_H
#define REGULARBSPTREE_H

#include <iostream>
#include <vector>
#include <cmath>
#include <queue>

#include <boost/optional.hpp>

// - Dimensions, size of each dimension
// - Max depth or max amount of points per cell
// - Enter a point into the tree
// - When adding a point, check first if the max amount of points is
//   excessed:
//   - If not, add the point to the cell.
//   - If yes, split the cell and redistribute the points into the new
//     cells.

template < class Point, size_t dim, class Data = int >
class Regular_bsp_tree
{
public:
    Regular_bsp_tree(Point const& min, Point const& max, int maxDepth, int maxPoints) :
        _cell_min(min),
        _cell_max(max),
        _parent(NULL),
        _isLeaf(true),
        _depth(0),
        _maxDepth(maxDepth),
        _maxPoints(maxPoints)
    { }

    Regular_bsp_tree() :
        _parent(NULL),
        _isLeaf(true),
        _depth(0),
        _maxDepth(10),
        _maxPoints(10)
    { }

    enum class Side { Positiv, Negativ };

    void get_Leafs(std::vector<Regular_bsp_tree const*>& leafs) const
    {
        if (!_isLeaf)
        {
            for (int i = 0; i < _childrenCount; ++i)
            {
                _children[i].get_leafs(leafs);
            }
        }
        else
        {
            leafs.push_back(this);
        }
    }

    bool is_empty() const
    {
        return (_points.size() == 0);
    }

    bool has_children() const
    {
        return (_children.size() > 0);
    }

    void setDepth(int d)
    {
        _depth = d;
    }

    int getDepth() const
    {
        return _depth;
    }

    bool get_is_leaf() const
    {
        return _isLeaf;
    }

    void set_min(Point const& min)
    {
        _cell_min = min;
    }

    void set_max(Point const& max)
    {
        _cell_max = max;
    }

    Point const& get_min() const
    {
        return _cell_min;
    }

    Point const& get_max() const
    {
        return _cell_max;
    }

    std::vector<Regular_bsp_tree> const& get_children() const
    {
        return _children;
    }

    std::vector<Point> const& get_points() const
    {
        return _points;
    }

    std::vector<Data const*> & get_data()
    {
        return _data;
    }

    std::vector<Data const*> const& get_data() const
    {
        return _data;
    }

    float get_radius() const
    {
        return (_cell_max - _cell_min).norm() * 0.5f;
    }

    Point get_center() const
    {
        return (_cell_max + _cell_min) * 0.5f;
    }

    Regular_bsp_tree<Point, dim, Data> & add_point(Point const& point, Data const* data = nullptr)
    {
        // std::cout << "Depth: " << _depth << " adding point: " << point << std::endl;

        if (_isLeaf)
        {
            // add point into cell directly or if full, split the cell
            // and add then
            if ((int)_points.size() >= _maxPoints && _depth < _maxDepth)
            {
                split_new();
//                split();

                return add_point(point, data);
            }
            else
            {
                // std::cout << "Leaf okay, pushing." << std::endl;
                _points.push_back(point);
                _data.push_back(data);

                assert(_points.size() == _data.size());
            }
        }
        else
        {
            // find the right child and add point there
            // std::cout << "Not a leaf, adding to child." << std::endl;
//            for (int i = 0; i < _childrenCount; ++i)
//            {
//                if (_children[i].is_point_in(point))
//                {
//                    return _children[i].add_point(point, data);
//                }
//            }

            int const child_index = get_child_index_containing_point(point, get_split_pos());
            return _children[child_index].add_point(point, data);
        }

        return *this;
    }


    bool is_point_in(Point const& point) const
    {
        // return (point >= _cellMin && point <= _cellMax);

        for (unsigned int i = 0; i < dim; ++i)
        {
            if (point[i] < _cell_min[i] || point[i] > _cell_max[i]) return false;
        }

        return true;
    }

    void split()
    {
        if (!_isLeaf)
        {
            std::cout << "split(): Error, trying to split non-leaf." << std::endl;
            return;
        }

        _children.resize(_childrenCount, Regular_bsp_tree(_cell_min, _cell_max, _maxDepth, _maxPoints));

        _isLeaf = false;

        _children[0].setDepth(_depth + 1);

        for (unsigned int d = 1; d <= dim; ++d)
        {
            int spacing = _childrenCount / std::pow(2, d);

            for (unsigned int j = 0; j < std::pow(2, d - 1); ++j)
            {
                int parentIndex = j * spacing * 2;
                int childIndex = parentIndex + spacing;
                Regular_bsp_tree const& parent = _children[parentIndex];

                Regular_bsp_tree leftChild = parent;
                leftChild._children.clear();
                Regular_bsp_tree rightChild = parent;
                rightChild._children.clear();

                leftChild._cell_max[d - 1]  -= 0.5f * (parent._cell_max[d - 1] - parent._cell_min[d - 1]);
                rightChild._cell_min[d - 1] += 0.5f * (parent._cell_max[d - 1] - parent._cell_min[d - 1]);

                /*
    std::cout <<
     "d: " << d <<
     " j: " << j <<
     " spacing: " << spacing <<
     " from: " << parentIndex <<
     " to: " << parentIndex << ", " << childIndex << std::endl;
    */

                _children[parentIndex] = leftChild;
                _children[childIndex] = rightChild;
            }
        }

        for (unsigned int i = 0; i < _points.size(); ++i)
        {
            Point const& point = _points[i];
            Data const* data = _data[i];

            for (int j = 0; j < _childrenCount; ++j)
            {
                if (_children[j].is_point_in(point))
                {
                    _children[j].add_point(point, data);
                    break;
                }
            }
        }

        for (unsigned int i = 0; i < _children.size(); ++i)
        {
            _children[i]._parent = this;
        }

        _points.clear();
        _data.clear();
    }

    static int get_child_index_containing_point(Point const& point, Point const& split_pos)
    {
        int index = _childrenCount;
        int level = _childrenCount;

        for (int i = dim - 1; i >= 0; --i)
        {
            level /= 2;

            if (point[i] < split_pos[i])
            {
                index -= level;
            }
        }

        return index - 1;
    }

    static std::vector<Side> get_split_planes_for_child_index(int child_index)
    {
        std::vector<Side> result(dim);

        int level = 1;

        ++child_index; // run from 1-8 instead of 0-7 for dim = 3

        for (unsigned int i = 0; i < dim; ++i)
        {
            level *= 2;

            if (child_index % level != 0)
            {
                result[i] = Side::Negativ;
                child_index = child_index + level / 2;
            }
            else
            {
                result[i] = Side::Positiv;
            }
        }

        return result;
    }

    void split_new()
    {
        if (!_isLeaf)
        {
            std::cout << "split(): Error, trying to split non-leaf." << std::endl;
            return;
        }

        _children.resize(_childrenCount, Regular_bsp_tree(_cell_min, _cell_max, _maxDepth, _maxPoints));

        _isLeaf = false;

        _split_pos = get_center();

        for (int i = 0; i < _childrenCount; ++i)
        {
            Regular_bsp_tree & child = _children[i];
            child.setDepth(_depth + 1);

            std::vector<Side> split_sides = get_split_planes_for_child_index(i);

            for (unsigned int d = 0; d < dim; ++d)
            {
                if (split_sides[d] == Side::Negativ)
                {
                    child._cell_max[d] = _split_pos[d];
                }
                else
                {
                    child._cell_min[d] = _split_pos[d];
                }
            }
        }

        for (unsigned int i = 0; i < _points.size(); ++i)
        {
            Point const& point = _points[i];
            Data const* data = _data[i];

            for (int j = 0; j < _childrenCount; ++j)
            {
                if (_children[j].is_point_in(point))
                {
                    _children[j].add_point(point, data);
                    break;
                }
            }
        }

        for (unsigned int i = 0; i < _children.size(); ++i)
        {
            _children[i]._parent = this;
        }

        _points.clear();
        _data.clear();
    }

    friend std::ostream& operator<< (std::ostream& stream, Regular_bsp_tree const& q)
    {
        // stream << q._depth << " " << q._cellMin << " " << q._cellMax << std::endl;
        stream << q._depth << " " << q._children.size() << std::endl;

        /*
        for (unsigned int i = 0; i < q._points.size(); ++i)
        {
            std::cout << "[" << q._points[i] << "] Data: " << q._data[i] << std::endl;
        }
        */

        if (!q._isLeaf)
        {
            std::cout << "Children:" << std::endl;

            for (int i = 0; i < q._childrenCount; ++i)
            {
                stream << q._children[i] << std::endl;
            }
        }

        return stream;
    }

    Regular_bsp_tree const& query(Point const& point) const
    {
        if (!_isLeaf)
        {
//            for (int j = 0; j < _childrenCount; ++j)
//            {
//                if (_children[j].is_point_in(point))
//                {
//                    return _children[j].query(point);
//                }
//            }

            int const child_index = get_child_index_containing_point(point, get_split_pos());
            return _children[child_index].query(point);
        }

        return *this;
    }

    static void getPostOrderQueue(std::vector< Regular_bsp_tree<Point, dim, Data>* > & list, Regular_bsp_tree<Point, dim, Data>* node)
    {
        for (unsigned int i = 0; i < node->_children.size(); ++i)
        {
            getPostOrderQueue(list, &(node->_children[i]));
        }

        list.push_back(node);
    }

    void printAveraged() const
    {
        std::cout << "p: d: " << _depth << ", " << _isLeaf << ", " << _data.size() << ")" << std::endl;

        for (unsigned int i = 0; i < _children.size(); ++i)
        {
            _children[i].printAveraged();
        }
    }

    template <typename Averager>
    static void average_data(Regular_bsp_tree* root, Averager const& averager)
    {
        std::vector<Regular_bsp_tree*> nodeQueue;
        getPostOrderQueue(nodeQueue, root);

//        std::cout << __PRETTY_FUNCTION__ << " postorder.size: " << nodeQueue.size() << std::endl;

        for (unsigned int i = 0; i < nodeQueue.size(); ++i)
        {
            Regular_bsp_tree & node = *nodeQueue[i];

            // std::cout << "node depth: " << node._depth << " node: " << &node << std::endl;

            if (node.get_is_leaf())
            {
                if (node._data.size() > 0)
                {
                    node._averaged_data = averager(node._data);
                }
            }
            else
            {
                std::vector<Data const*> leaf_data;

                for (unsigned int j = 0; j < node._children.size(); ++j)
                {
                    if (!node._children[j]._averaged_data) continue;

                    leaf_data.push_back(&node._children[j]._averaged_data.get());
                }

                if (leaf_data.size() > 0)
                {
                    node._averaged_data = boost::optional<Data>(averager(leaf_data));
                }

                // node._clusteredData->pos = (node._cellMin + node._cellMax) * 0.5f;
            }
        }
    }

    bool has_averaged_data() const
    {
        return _averaged_data.is_initialized();
    }

    Data const& get_averaged_data() const
    {
        return _averaged_data.get();
    }

    std::vector<Regular_bsp_tree const*> get_all_nodes() const
    {
        std::vector<Regular_bsp_tree const*> result;

        std::queue<Regular_bsp_tree const*> queue;
        queue.push(this);

        while (!queue.empty())
        {
            Regular_bsp_tree const* node = queue.front();
            queue.pop();

            result.push_back(node);

            for (unsigned int i = 0; i < node->_children.size(); ++i)
            {
                queue.push(&(node->_children[i]));
            }
        }

        return result;
    }

    std::vector<Regular_bsp_tree const*> get_nodes(int depth) const
    {
        std::vector<Regular_bsp_tree const*> result;

        std::queue<Regular_bsp_tree const*> queue;
        queue.push(this);

        while (!queue.empty())
        {
            Regular_bsp_tree const* node = queue.front();
            queue.pop();

            if (node->_depth < depth)
            {
                for (unsigned int i = 0; i < node->_children.size(); ++i)
                {
                    queue.push(&(node->_children[i]));
                }
            }
            else if (node->_depth == depth)
            {
                result.push_back(node);
            }

            assert(node->_depth <= depth);
        }

        return result;
    }

    int getMaxDepth() const
    {
        int maxDepth = -1;

        std::queue<Regular_bsp_tree const*> queue;
        queue.push(this);

        while (!queue.empty())
        {
            Regular_bsp_tree const* node = queue.front();
            queue.pop();

            if (node->_depth > maxDepth)
            {
                maxDepth = node->_depth;
            }

            for (unsigned int i = 0; i < node->_children.size(); ++i)
            {
                queue.push(&(node->_children[i]));
            }
        }

        return maxDepth;
    }

    // the leaf closest to the root
    int getMinLeafDepth() const
    {
        int minDepth = 10000;

        std::queue<Regular_bsp_tree const*> queue;
        queue.push(this);

        while (!queue.empty())
        {
            Regular_bsp_tree const* node = queue.front();
            queue.pop();

            if (node->getIsLeaf() && node->_depth < minDepth)
            {
                minDepth = node->_depth;
            }

            for (unsigned int i = 0; i < node->_children.size(); ++i)
            {
                queue.push(&(node->_children[i]));
            }
        }

        return minDepth;
    }

    int getNodeCount() const
    {
        int count = 0;

        std::queue<Regular_bsp_tree const*> queue;
        queue.push(this);

        while (!queue.empty())
        {
            Regular_bsp_tree const* node = queue.front();
            queue.pop();

            ++count;

            for (unsigned int i = 0; i < node->_children.size(); ++i)
            {
                queue.push(&(node->_children[i]));
            }
        }

        return count;
    }

    Point const& get_split_pos() const
    {
        return _split_pos;
    }

//    Data const* find_closest_in_radius(Point const& p, float const radius) const
//    {

//    }

    template <typename Reject_condition>
    Data const* find_closest(Point const& p, boost::optional<Reject_condition> const& reject_condition = boost::optional<Reject_condition>()) const
    {
        Regular_bsp_tree node = query(p);

        std::vector<Point> const& points = node.get_points();
        std::vector<Data const*> const& data = node.get_data();

        int closest_index = -1;
        float closest_dist = 1e10f;

        for (size_t i = 0; i < points.size(); ++i)
        {
            if (reject_condition && reject_condition.get()(*data[i])) continue;

            float const dist = (points[i] - p).norm();

            if (dist < closest_dist)
            {
                closest_dist = dist;
                closest_index = i;
            }
        }

//        return find_closest_in_radius(p, closest_dist);

        assert(closest_index >= 0);

        return data[closest_index];
    }


//    void clear()
//    {
//        _children.clear();
//    }

private:
    Point _cell_min;
    Point _cell_max;
    Point _split_pos;
    std::vector< Regular_bsp_tree<Point, dim, Data> > _children;
    Regular_bsp_tree<Point, dim, Data>* _parent;

    std::vector<Point> _points;
    std::vector<Data const*> _data;

    boost::optional<Data> _averaged_data;

    bool _isLeaf;
    int _depth;
    int _maxDepth;
    int _maxPoints;

    static int const _childrenCount;
};


template < class Point, size_t dim, class Data >
int const Regular_bsp_tree<Point, dim, Data>::_childrenCount = std::pow(2, dim);


/*
template <class Point>
void draw(QPainter & p, RegularBspTree<Point, 2> const& quadTree, Point const& min, Point const& max)
{
    // std::cout << "Drawing" << std::endl;

    p.setPen(Qt::black);

    float xRange = max[0] - min[0];
    float yRange = max[1] - min[1];

    QRect rect(
                std::floor((quadTree.getMin()[0] - min[0]) / xRange * p.window().width() + 0.5f),
                std::floor((quadTree.getMin()[1] - min[1]) / yRange * p.window().height() + 0.5f),
                std::floor((quadTree.getMax()[0] - quadTree.getMin()[0]) / xRange * p.window().width() + 0.5f),
                std::floor((quadTree.getMax()[1] - quadTree.getMin()[1]) / yRange * p.window().width() + 0.5f)
                );

    p.drawRect(rect);

    std::vector<Point> const& points = quadTree.getPoints();

    p.setPen(Qt::red);

    for (unsigned int i = 0; i < points.size(); ++i)
    {
        p.drawEllipse(
                    (points[i][0] - min[0]) / xRange * p.window().width(),
                    (points[i][1] - min[1]) / xRange * p.window().width(),
                    2, 2);
    }

    if (!quadTree.getIsLeaf())
    {
        std::vector<RegularBspTree<Point, 2> > const& children = quadTree.getChildren();

        for (unsigned int i = 0; i < children.size(); ++i)
        {
            draw(p, children[i], min, max);
        }
    }
}
*/

#endif // REGULARBSPTREE_H
