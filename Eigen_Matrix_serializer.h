#ifndef EIGEN_MATRIX_SERIALIZER_H
#define EIGEN_MATRIX_SERIALIZER_H

#include <Eigen/Core>

namespace boost
{
    template<class Archive, typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
    inline void serialize(
        Archive & ar, Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> & t, const unsigned int /* file_version */)
    {
        for (int i = 0; i < t.size(); i++)
        {
            ar & boost::serialization::make_nvp("v", t.data()[i]);
//            ar & BOOST_SERIALIZATION_NVP(t.data()[i]);
        }
    }
}

#endif // EIGEN_MATRIX_SERIALIZER_H
