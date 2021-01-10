#ifndef FP32_h
#define FP32_h

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <string>
#include <ostream>

// #define FP32_DEBUG_ENABLED

#define FP_SHIFT 12

#define FP32_HIGH(a, p) ((a) >> (p))

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
    constexpr operator float() const
    {
        return (static_cast<float>(m_Val) / static_cast<float>(1 << P));
    }

    constexpr operator int() const
    {
        return FP32_HIGH(m_Val, P);
    }

    constexpr operator bool() const
    {
        return m_Val;
    }

    /* Quite a lot of redundancy, but I wanted the code to be fast enough even when
    compiler optimizations are turned off. I therefore tried to avoid making unecessary
    objets or function calls */

    constexpr FP32<P> operator-() const
    {
        FP32<P> ret;
        ret.m_Val = -m_Val;
#ifdef FP32_DEBUG_ENABLED
        ret.m_ValStr = ret.GetString();
#endif
        return ret;
    }

    friend constexpr FP32<P> operator+(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(iN1.m_Val + iN2.m_Val);
    }

    friend constexpr FP32<P> operator+(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal(iN1.m_Val + (iN2 << P));
    }

    friend constexpr FP32<P> operator+(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN1 << P) + iN2.m_Val);
    }

    friend constexpr FP32<P> operator-(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return FromFPVal(iN1.m_Val - iN2.m_Val);
    }

    friend constexpr FP32<P> operator-(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal(iN1.m_Val - (iN2 << P));
    }

    friend constexpr FP32<P> operator-(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN1 << P) - iN2.m_Val);
    }

    friend constexpr FP32<P> operator*(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        int64_t mul = static_cast<int64_t>(iN1.m_Val) * static_cast<int64_t>(iN2.m_Val);
        return FromFPVal(static_cast<int32_t>(mul >> P));
    }

    friend constexpr FP32<P> operator*(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal((iN1.m_Val * iN2));
    }

    friend constexpr FP32<P> operator*(const int &iN1, const FP32<P> &iN2)
    {
        return FromFPVal((iN2.m_Val * iN1));
    }

    friend constexpr FP32<P> operator/(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        int64_t num = static_cast<int64_t>(iN1.m_Val) << P;
        int64_t den = static_cast<int64_t>(iN2.m_Val);
        return FromFPVal(static_cast<int32_t>((num / den)));
    }

    friend constexpr FP32<P> operator/(const FP32<P> &iN1, const int &iN2)
    {
        return FromFPVal((iN1.m_Val / iN2));
    }

    friend constexpr FP32<P> operator/(const int &iN1, const FP32<P> &iN2)
    {
        int64_t num = static_cast<int64_t>(iN1) << (P * 2u);
        int64_t den = static_cast<int64_t>(iN2.m_Val);
        return FromFPVal(static_cast<int32_t>((num / den)));
    }

    FP32<P> &operator+=(const FP32<P> &iIncrement)
    {

        this->m_Val += iIncrement.m_Val;
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
        return *this;
    }

    FP32<P> &operator+=(int &iIncrement)
    {

        this->m_Val += iIncrement << P;
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
        return *this;
    }

    FP32<P> &operator-=(const FP32<P> &iIncrement)
    {

        this->m_Val -= iIncrement;
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
        return *this;
    }

    FP32<P> &operator-=(int &iIncrement)
    {

        this->m_Val -= iIncrement << P;
#ifdef FP32_DEBUG_ENABLED
        m_ValStr = GetString();
#endif
        return *this;
    }

    friend constexpr bool operator==(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val == iN2.m_Val;
    }

    friend constexpr bool operator==(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val == (iN2 << P);
    }

    friend constexpr bool operator==(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val == (iN1 << P);
    }

    friend constexpr bool operator!=(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val != iN2.m_Val;
    }

    friend constexpr bool operator!=(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val != (iN2 << P);
    }

    friend constexpr bool operator!=(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val != (iN1 << P);
    }

    friend constexpr bool operator<(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val < iN2.m_Val;
    }

    friend constexpr bool operator<(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val < (iN2 << P);
    }

    friend constexpr bool operator<(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val > (iN1 << P);
    }

    friend constexpr bool operator<=(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val <= iN2.m_Val;
    }

    friend constexpr bool operator<=(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val <= (iN2 << P);
    }

    friend constexpr bool operator<=(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val >= (iN1 << P);
    }

    friend constexpr bool operator>(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val > iN2.m_Val;
    }

    friend constexpr bool operator>(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val > (iN2 << P);
    }

    friend constexpr bool operator>(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val < (iN1 << P);
    }

    friend constexpr bool operator>=(const FP32<P> &iN1, const FP32<P> &iN2)
    {
        return iN1.m_Val >= iN2.m_Val;
    }

    friend constexpr bool operator>=(const FP32<P> &iN1, const int &iN2)
    {
        return iN1.m_Val >= (iN2 << P);
    }

    friend constexpr bool operator>=(const int &iN1, const FP32<P> iN2)
    {
        return iN2.m_Val <= (iN1 << P);
    }

public:
    std::string GetString() const
    {
        // Inaccurate, we'll see if we need something better in the future
        return std::to_string(static_cast<float>(*this));
    }

    int32_t GetRawValue() const
    {
        return m_Val;
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