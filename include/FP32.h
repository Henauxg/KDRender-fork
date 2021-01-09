#ifndef FP32_h
#define FP32_h

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <string>
#include <ostream>

#include "Consts.h"

#define FP32_DEBUG_ENABLED

#define FP32_HIGH(a, p) ((a) >> (p))
#define FP32_LOW(a, p) (((a) << (32 - p)) >> (32 - p))

template<unsigned int P>
class FP32
{
public:
    ~FP32() {}

    FP32() {}

    constexpr FP32(int iInt) :
        m_Val(iInt << P)
    {
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
    }

    constexpr FP32(float iFloat) :
        m_Val(std::roundf(iFloat * (1 << P)))
    {
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
    }

    static constexpr FP32<P> FromFPVal(int iFPVal)
    {
        FP32<P> ret;
        ret.m_Val = iFPVal;
#ifdef FP32_DEBUG_ENABLED
        ret.m_ValStr = ret.GetString();
#endif
        return ret;
    }

public:
    operator float() const
    {
        return (static_cast<float>(m_Val) / static_cast<float>(1 << P));
    }

    operator int() const
    {
        return FP32_HIGH(m_Val, P);
    }

    friend FP32<P> operator+(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(iN1.m_Val + iN2.m_Val);
    }

    friend FP32<P> operator+(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal(iN1.m_Val + (iN2 << P));
    }

    friend FP32<P> operator+(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN1 << P) + iN2.m_Val);
    }

    friend FP32<P> operator-(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(iN1.m_Val - iN2.m_Val);
    }

    friend FP32<P> operator-(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal(iN1.m_Val - (iN2 << P));
    }

    friend FP32<P> operator-(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN1 << P) - iN2.m_Val);
    }

    friend FP32<P> operator*(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(static_cast<int32_t>((static_cast<int64_t>(iN1.m_Val) * static_cast<int64_t>(iN2.m_Val))) >> P);
    }

    friend FP32<P> operator*(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal((iN1.m_Val * iN2));
    }

    friend FP32<P> operator*(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN2.m_Val * iN1));
    }

    friend FP32<P> operator/(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(static_cast<int32_t>(((static_cast<int64_t>(iN1.m_Val) << P) / static_cast<int64_t>(iN2.m_Val))));
    }

    friend FP32<P> operator/(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal((iN1.m_Val / iN2));
    }

    friend FP32<P> operator/(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(static_cast<int32_t>((static_cast<int64_t>(iN1) << (2u * P)) / static_cast<int64_t>(iN2.m_Val)));
    }

public:
    std::string GetString() const
    {
        // Inaccurate, we'll see if we need something better in the future
        return std::to_string(static_cast<float>(*this));
    }

    friend std::ostream &operator<<(std::ostream &oStream, const FP32<P> &iInt)
    {
        oStream << iInt.GetString();
        return oStream;
    }

private:
    int32_t m_Val;
#ifdef FP32_DEBUG_ENABLED
    std::string m_ValStr;
#endif
};

#endif