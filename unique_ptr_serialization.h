#ifndef UNIQUE_PTR_SERIALIZATION_H
#define UNIQUE_PTR_SERIALIZATION_H

template <typename T>
struct Ptr_contains_predicate
{
    Ptr_contains_predicate(T* pPtr) :
        mPtr(pPtr)
    {}

//    template <typename P>
    bool operator()(boost::shared_ptr<T> const& pPtr) const
    {
        return pPtr.get() == mPtr;
    }

    T* mPtr;
};

namespace boost {
    namespace serialization {

        template <class Archive, class T>
        inline void save
            (Archive &archive,
            std::unique_ptr<T> const& subtree,
            const unsigned int file_version)
        {
            // only the raw pointer has to be saved
            const T *const  subtree_x = subtree.get();

            archive << subtree_x;
        }

        template <class Archive, class T>
        inline void load
            (Archive &archive,
            std::unique_ptr<T> &subtree,
            const unsigned int file_version)
        {

            T *p_subtree;

            archive >> p_subtree;

            #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
                subtree.release();
                subtree = std::unique_ptr< T >(p_subtree);
            #else
                subtree.reset(p_subtree);
            #endif
        }

        template <class Archive, class T>
        inline void serialize
            (Archive &archive,
            std::unique_ptr<T> &subtree,
            const unsigned int file_version)
        {
            boost::serialization::split_free(archive, subtree, file_version);
        }

    } // namespace serialization
} // namespace boost


#endif // UNIQUE_PTR_SERIALIZATION_H
