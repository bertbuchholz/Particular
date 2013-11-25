#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include <vector>

#ifndef Q_MOC_RUN
#include <boost/optional.hpp>
#endif

#include <Eigen/Core>

template <typename Vec>
unsigned int hash_function(Vec const& point, const int n, const float cell_size)
{
    /*
    hash(x,y,z) = ( x p1 xor y p2 xor z p3) mod n
    where p1, p2, p3 are large prime numbers, in
    our case 73856093, 19349663, 83492791
    */

    // float bb_size = 6.0f;

    float const inv_cell_size = 1.0f / cell_size;

    unsigned int i_x = std::floor(point[0] * inv_cell_size);
    unsigned int i_y = std::floor(point[1] * inv_cell_size);
    unsigned int i_z = std::floor(point[2] * inv_cell_size);

    // std::cout << "i_x: " << (i_x % n) << " sample.x: " << sample.x << std::endl;

    unsigned int hash_value = ((i_x * 73856093u) ^ (i_y * 19349663u) ^ (i_z * 83492791u)) % unsigned(n);
    // unsigned int hash_value = (i_x + i_y + i_z) % n;

    return hash_value;
}

template <typename Vec, typename Data>
class Spatial_hash
{
public:
    struct Point_data
    {
        Point_data(Vec const& p, Data const& d) : point(p), data(d)
        { }

        Vec point;
        Data data;
    };

    struct Index_distance
    {
        Index_distance(int const i, float const d) : index(i), distance(d)
        { }

        int index;
        float distance;
    };

    Spatial_hash(int const num_bins, float const cell_size)
    {
        _cell_size = cell_size;

        _bins.resize(num_bins);
    }

    void add_point(Vec const& position, Data const& data)
    {
        Point_data const point(position, data);

        int const bin_index = hash_function(position, int(_bins.size()), _cell_size);

        _bins[bin_index].push_back(point);

//        std::cout << __FUNCTION__ << " Added " << data << " " << position << " to bin " << bin_index << std::endl;
    }

    struct Null_condition
    {
        bool operator() (Data const& /* d */) const
        {
            return false;
        }
    };

    template <typename Reject_condition>
    Index_distance get_closest_point_in_bin(std::vector<Point_data> const& bin, Vec const& point, Reject_condition const& reject_condition) const
    {
        Index_distance closest_data(-1, 1e10f);

        for (size_t i = 0; i < bin.size(); ++i)
        {
            if (reject_condition(bin[i].data)) continue;

            float const dist = (bin[i].point - point).norm();

            if (dist < closest_data.distance)
            {
                closest_data.distance = dist;
                closest_data.index = int(i);
            }
        }

        return closest_data;
    }

    template <typename Reject_condition = Null_condition>
    boost::optional<Point_data const&> get_closest_point(Vec const& point, Reject_condition const& reject_condition = Null_condition()) const
    {
        int closest_point_index = -1;
        int closest_bin_index = -1;
        float closest_dist = 1e10f;

        float const offsets[] { -_cell_size, 0.0f, _cell_size };

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                for (int z = -1; z <= 1; ++z)
                {
                    int const bin_index = hash_function(point + Vec(offsets[x + 1], offsets[y + 1], offsets[z + 1]), int(_bins.size()), _cell_size);

                    Index_distance const index_distance = get_closest_point_in_bin(_bins[bin_index], point, reject_condition);

                    if (index_distance.distance < closest_dist && index_distance.distance <= _cell_size)
                    {
                        closest_dist = index_distance.distance;
                        closest_point_index = index_distance.index;
                        closest_bin_index = bin_index;
                    }
                }
            }
        }

//        assert(closest_bin_index < int(_bins.size()) && closest_point_index < int(_bins[closest_bin_index].size()));

        if (closest_bin_index < 0) return boost::optional<Point_data const&>();

        return boost::optional<Point_data const&>(_bins[closest_bin_index][closest_point_index]);
    }

    std::vector< std::vector<Point_data> const* > get_neighborhood(Vec const& point) const
    {
        std::vector< std::vector<Point_data> const* > result;

        float const offsets[] { -_cell_size, 0.0f, _cell_size };

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                for (int z = -1; z <= 1; ++z)
                {
                    int const bin_index = hash_function(point + Vec(offsets[x + 1], offsets[y + 1], offsets[z + 1]), _bins.size(), _cell_size);

                    result.push_back(&_bins[bin_index]);
                }
            }
        }

        return result;
    }

    void clear()
    {
        for (std::vector<Point_data> & bin : _bins)
        {
            bin.clear();
        }
    }

private:
    float _cell_size;
    std::vector< std::vector<Point_data> > _bins;
};


inline void spatial_hash_test()
{
    typedef Eigen::Vector3f MyVec;

    MyVec const search_point(0.0f, 0.0f, 0.0f);

    Spatial_hash<MyVec, int> sh(10, 0.2f);

    std::vector<MyVec> points;

    int closest_index = -1;
    float closest_dist = 1e10f;

    for (int i = 0; i < 50; ++i)
    {
        MyVec p = MyVec::Random();
        points.push_back(p);
//        std::cout << p << std::endl;

        sh.add_point(p, i);

        float const dist = (p - search_point).norm();

        if (dist < closest_dist)
        {
            closest_index = i;
            closest_dist = dist;
        }
    }

    std::cout << "Closest point " << closest_index << " " << closest_dist << std::endl;

    boost::optional<Spatial_hash<MyVec, int>::Point_data const&> opt_pd = sh.get_closest_point(search_point);

    if (!opt_pd)
    {
        std::cout << "No point found" << std::endl;
    }
    else
    {
        std::cout << "Found point " << opt_pd->data << std::endl;
        assert(opt_pd->data == closest_index);
    }

}

#endif // SPATIAL_HASH_H
