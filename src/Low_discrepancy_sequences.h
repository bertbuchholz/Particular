#ifndef LOW_DISCREPANCY_SEQUENCES_H
#define LOW_DISCREPANCY_SEQUENCES_H

float inline radicalInverse(int n, int base)
{
    float value = 0.0f;
    float invBase = 1.0f/(float)(base), invBi = invBase;

    while (n > 0)
    {
        int d_i = (n % base);
        value += d_i * invBi;
        n /= base;
        invBi *= invBase;

    }

    return value;
}

template <typename Vec3>
Vec3 halton_3(int n)
{
    Vec3 result;

    result[0] = radicalInverse(n, 2);
    result[1] = radicalInverse(n, 3);
    result[2] = radicalInverse(n, 5);

    return result;
}


template <typename Vec3>
Vec3 hammersley_3(int const n, int const n_max)
{
    Vec3 result;

    result[0] = n / float(n_max);
    result[1] = radicalInverse(n, 2);
    result[2] = radicalInverse(n, 3);

    return result;
}


class Halton
{
public:

    Halton()
    {
        //Empty
    }

    Halton(int base)
    {
        setBase(base);
    }

    void setBase(int base)
    {
        mBase = base;
        mInvBase = 1.0 / (double) base;
        mValue = 0;
    }

    void reset()
    {
        mValue=0.0;
    }

    inline void setStart(unsigned int i)
    {
        double factor = mInvBase;

        mValue = 0.0;

        while (i > 0)
        {
            mValue += (double) (i % mBase) * factor;
            i /= mBase;
            factor *= mInvBase;
        }
    }

    inline float getNext()
    {
        double r = 0.9999999999 - mValue;
        if (mInvBase < r)
        {
            mValue += mInvBase;
        }
        else
        {
            double hh, h = mInvBase;
            while (h >= r)
            {
                hh = h;
                h *= mInvBase;
            }

            mValue += hh + h - 1.0;
        }
        return mValue;
    }

private:
    unsigned int mBase;
    double mInvBase;
    double mValue;
};


#endif // LOW_DISCREPANCY_SEQUENCES_H
