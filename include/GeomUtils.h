#ifndef GeomUtils_h
#define GeomUtils_h

#include <cmath>
#include <algorithm>
#include <cstdint>

#include "Consts.h"
#include "FP32.h"

template<typename Vertex, typename Intermediate = CType>
inline int WhichSide(const Vertex &iV1, const Vertex &iV2, const Vertex &iP)
{
    Intermediate normalX = iV2.m_Y - iV1.m_Y;
    Intermediate normalY = iV1.m_X - iV2.m_X;

    Intermediate prod = (iP.m_X - iV1.m_X) * normalX + (iP.m_Y - iV1.m_Y) * normalY;
    if(prod == 0)
        return 0;
    else if(prod < 0)
        return -1;
    else
        return 1;
}

inline CType cosInt(int iAngle)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    float iAngleRad = static_cast<float>(iAngle) / (180.0 * static_cast<float>((1 << ANGLE_SHIFT))) * M_PI;
    float res = cos(iAngleRad);
    return static_cast<CType>(res);
}

inline CType sinInt(int iAngle)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    double iAngleRad = static_cast<float>(iAngle) / (180.0 * static_cast<float>((1 << ANGLE_SHIFT))) * M_PI;
    float res = sin(iAngleRad);
    return static_cast<CType>(res);
}

inline CType tanInt(int iAngle)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    double iAngleRad = static_cast<float>(iAngle) / (180.0 * static_cast<float>((1 << ANGLE_SHIFT))) * M_PI;
    float res = tan(iAngleRad);
    return static_cast<CType>(res);
}

inline int atanInt(CType iX)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    float x = static_cast<float>(iX);
    double atanVal = atan(x) * (180.0 * static_cast<float>((1 << ANGLE_SHIFT))) / M_PI;
    return static_cast<int>(atanVal);
}

template <typename Vertex>
inline void GetVector(const Vertex &iOrigin, int iDirection, Vertex &oTo)
{
    CType cosDirMult = cosInt(iDirection);
    CType sinDirMult = sinInt(iDirection);

    oTo.m_X = iOrigin.m_X + sinDirMult;
    oTo.m_Y = iOrigin.m_Y + cosDirMult;
}

template <typename Vertex, typename RetType = CType>
inline RetType DotProduct(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    return (v1.m_X - v0.m_X) * (v3.m_X - v2.m_X) + (v1.m_Y - v0.m_Y) * (v3.m_Y - v2.m_Y);
}

template <typename Vertex>
inline CType Det(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    return (v1.m_X - v0.m_X) * (v3.m_Y - v2.m_Y) - (v1.m_Y - v0.m_Y) * (v3.m_X - v2.m_X);
}

// TODO: re-enable
// From Wikipedia's article on integer square root
// FP32<FP_SHIFT> SqrtInt(const FP32<FP_SHIFT> &s)
// {
//     FP32<FP_SHIFT> x0 = s >> 1; // Initial estimate

//     // Sanity check
//     if (x0)
//     {
//         long x1 = (x0 + s / x0) >> 1; // Update

//         while (x1 < x0) // This also checks for cycle
//         {
//             x0 = x1;
//             x1 = (x0 + s / x0) >> 1;
//         }

//         return x0;
//     }
//     else
//     {
//         return s;
//     }
// }

template <typename Vertex>
inline CType SquareDist(const Vertex &iV1, const Vertex &iV2)
{
    return (iV2.m_X - iV1.m_X) * (iV2.m_X - iV1.m_X) + (iV2.m_Y - iV1.m_Y) * (iV2.m_Y - iV1.m_Y);
}

template <typename Vertex>
inline CType DistInt(const Vertex &iV1, const Vertex &iV2)
{
    return std::sqrt(static_cast<float>(SquareDist(iV1, iV2)));
}

template <typename Vertex>
inline int Angle(const Vertex &iFrom, const Vertex &iTo1, const Vertex &iTo2)
{
    CType dot = DotProduct(iFrom, iTo1, iFrom, iTo2);
    CType det = Det(iFrom, iTo1, iFrom, iTo2);
    // Extremely non-optimized
    // TODO: implement lookup table
    double angle = -atan2(static_cast<float>(det), static_cast<float>(dot));
    return static_cast<int>(angle / M_PI * (180.0 * static_cast<float>((1 << ANGLE_SHIFT))));
}

// Thanks to http://flassari.is/2008/11/line-line-intersection-in-cplusplus/ for writing
// this boiler-plate piece of code for me
// Note: this function returns false if lines are colinear, even if they are the same
template <typename Vertex, typename Intermediate = CType>
inline bool LineLineIntersection(const Vertex &iV1, const Vertex &iV2, const Vertex &iV3, const Vertex &iV4, Vertex &oIntersection)
{
    // Store the values for fast access and easy
    // equations-to-code conversion
    Intermediate x1 = iV1.m_X, x2 = iV2.m_X, x3 = iV3.m_X, x4 = iV4.m_X;
    Intermediate y1 = iV1.m_Y, y2 = iV2.m_Y, y3 = iV3.m_Y, y4 = iV4.m_Y;

    Intermediate d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    // If d is zero, there is no intersection
    if (d == 0)
        return false;

    // Get the x and y
    Intermediate pre = (x1 * y2 - y1 * x2), post = (x3 * y4 - y3 * x4);
    Intermediate x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
    Intermediate y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

    // Return the point of intersection
    oIntersection.m_X = x;
    oIntersection.m_Y = y;
    return true;
}

// Really dirty, just wanted to try this out
// TODO: code a real intersection solver
template <typename Vertex, typename Intermediate = CType>
inline bool HalfLineSegmentIntersection(const Vertex &iHalfLineFrom, const Vertex &iHalfLineTo, const Vertex &iV1, const Vertex &iV2, Vertex &oIntersection)
{
    if (!LineLineIntersection<Vertex, Intermediate>(iHalfLineFrom, iHalfLineTo, iV1, iV2, oIntersection))
        return false;
    else
    {
        if (DotProduct<Vertex, Intermediate>(iHalfLineFrom, oIntersection, iHalfLineFrom, iHalfLineTo) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iV1, oIntersection, iV1, iV2) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iV2, oIntersection, iV2, iV1) < 0)
            return false;

        return true;
    }
}

// As shitty as above
// TODO: code a real intersection solver
template <typename Vertex, typename Intermediate = CType>
inline bool LineSegmentIntersection(const Vertex &iLineV1, const Vertex &iLineV2, const Vertex &iSegV1, const Vertex &iSegV2, Vertex &oIntersection)
{
    if (!LineLineIntersection<Vertex, Intermediate>(iLineV1, iLineV2, iSegV1, iSegV2, oIntersection))
        return false;
    else
    {
        if (DotProduct<Vertex, Intermediate>(iSegV1, oIntersection, iSegV1, iSegV2) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iLineV2, oIntersection, iSegV1, iSegV2) < 0)
            return false;

        return true;
    }
}

// As shitty as above
// TODO: code a real intersection solver
template <typename Vertex, typename Intermediate = CType>
inline bool SegmentSegmentIntersection(const Vertex &iV1, const Vertex &iV2, const Vertex &iV3, const Vertex &iV4, Vertex &oIntersection)
{
    if (!LineLineIntersection<Vertex, Intermediate>(iV1, iV2, iV3, iV4, oIntersection))
        return false;
    else
    {
        if (DotProduct<Vertex, Intermediate>(iV1, oIntersection, iV1, iV2) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iV2, oIntersection, iV2, iV1) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iV3, oIntersection, iV3, iV4) < 0)
            return false;

        if (DotProduct<Vertex, Intermediate>(iV4, oIntersection, iV4, iV3) < 0)
            return false;

        return true;
    }
}

template <typename Vertex>
inline bool VertexOnXYAlignedSegment(Vertex iV1, Vertex iV2, const Vertex &iPoint)
{
    if (iV1.m_X == iV2.m_X && iPoint.m_X == iV1.m_X)
    {
        if(iV1.m_Y > iV2.m_Y)
            std::swap(iV1, iV2);
        return iV1.m_Y <= iPoint.m_Y && iPoint.m_Y <= iV2.m_Y;
    }
    else if (iV1.m_Y == iV2.m_Y && iPoint.m_Y == iV1.m_Y)
    {
        if (iV1.m_X > iV2.m_X)
            std::swap(iV1, iV2);
        return iV1.m_X <= iPoint.m_X && iPoint.m_X <= iV2.m_X;
    }
    else
        return false;
}

template <typename Number>
inline Number Clamp(Number iVal, Number iMin, Number iMax)
{
    return (iVal < iMin ? iMin : (iVal > iMax ? iMax : iVal));
}

// iDiv is assumed positive
template <typename Number>
inline Number Mod(const Number &iVal, const Number &iDiv)
{
    return iVal % iDiv;
}

template<>
inline float Mod<float>(const float &iVal, const float &iDiv)
{
    if (iVal >= 0)
        return std::fmod(iVal, iDiv);
    else
        return iDiv + std::fmod(iVal, iDiv);
}

template <>
inline double Mod<double>(const double &iVal, const double &iDiv)
{
    if(iVal >= 0)
        return std::fmod(iVal, iDiv);
    else
        return iDiv + std::fmod(iVal, iDiv);
}

template <>
inline FP32<FP_SHIFT> Mod<FP32<FP_SHIFT>>(const FP32<FP_SHIFT> &iVal, const FP32<FP_SHIFT> &iDiv)
{
    return iVal - (static_cast<int>(iVal / iDiv) * iDiv);
}

#endif