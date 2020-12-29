#ifndef GeomUtils_h
#define GeomUtils_h

#include <cmath>

#define DECIMAL_MULT 1000

template<typename Vertex>
int WhichSide(const Vertex &iV1, const Vertex &iV2, const Vertex &iP)
{
    int normalX = iV2.m_Y - iV1.m_Y;
    int normalY = iV1.m_X - iV2.m_X;

    int prod = (iP.m_X - iV1.m_X) * normalX + (iP.m_Y - iV1.m_Y) * normalY;
    if(prod == 0)
        return 0;
    else if(prod < 0)
        return -1;
    else
        return 1;
}

int cosInt(int iAngle)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    double iAngleRad = static_cast<double>(iAngle) / 180.0 * M_PI;
    double mult = static_cast<double>(DECIMAL_MULT);
    double res = cos(iAngleRad) * mult;
    return static_cast<int>(res);
}

int sinInt(int iAngle)
{
    // Extremely non-optimized
    // TODO: implement lookup table
    double iAngleRad = static_cast<double>(iAngle) / 180.0 * M_PI;
    double mult = static_cast<double>(DECIMAL_MULT);
    double res = sin(iAngleRad) * mult;
    return static_cast<int>(res);
}

template<typename Vertex>
void GetVector(const Vertex &iOrigin, int iDirection, Vertex &oTo)
{
    int cosDirMult = cosInt(iDirection);
    int sinDirMult = sinInt(iDirection);

    oTo.m_X = iOrigin.m_X + sinDirMult;
    oTo.m_Y = iOrigin.m_Y + cosDirMult;
}

template<typename Vertex>
int DotProduct(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    return (v1.m_X - v0.m_X) * (v3.m_X - v2.m_X) + (v1.m_Y - v0.m_Y) * (v3.m_Y - v2.m_Y);
}

template<typename Vertex>
int Det(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    return (v1.m_X - v0.m_X) * (v3.m_Y - v2.m_Y) + (v1.m_Y - v0.m_Y) * (v3.m_X - v2.m_X);
}

// From Wikipedia's article on integer square root
unsigned int sqrtInt(unsigned int s)
{
    unsigned int x0 = s >> 1; // Initial estimate

    // Sanity check
    if (x0)
    {
        unsigned long x1 = (x0 + s / x0) >> 1; // Update

        while (x1 < x0) // This also checks for cycle
        {
            x0 = x1;
            x1 = (x0 + s / x0) >> 1;
        }

        return x0;
    }
    else
    {
        return s;
    }
}

template <typename Vertex>
unsigned int SquareDist(const Vertex &iV1, const Vertex &iV2)
{
    return iV1.m_X * iV2.m_X + iV1.m_Y * iV2.m_Y;
}

template <typename Vertex>
unsigned int DistInt(const Vertex &iV1, const Vertex &iV2)
{
    return SqrtInt(SquareDist(iV1, iV2));
}

template <typename Vertex>
int Angle(const Vertex &iFrom, const Vertex &iTo1, const Vertex &iTo2)
{
    int dot = DotProduct(iFrom, iTo1, iFrom, iTo2);
    int det = Det(iFrom, iTo1, iFrom, iTo2);
    // Extremely non-optimized
    // TODO: implement lookup table
    double angle = atan2(det, dot);
    return static_cast<int>(angle / M_PI * 180.0);
}

// Thanks to http://flassari.is/2008/11/line-line-intersection-in-cplusplus/ for writing
// this boiler-plate piece of code for me
template <typename Vertex>
bool LineLineIntersection(const Vertex &iV1, const Vertex &iV2, const Vertex &iV3, const Vertex &iV4, Vertex &oIntersection)
{
    // Store the values for fast access and easy
    // equations-to-code conversion
    int x1 = iV1.m_X, x2 = iV2.m_X, x3 = iV3.m_X, x4 = iV4.m_X;
    int y1 = iV1.m_Y, y2 = iV2.m_Y, y3 = iV3.m_Y, y4 = iV4.m_Y;

    int d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    // If d is zero, there is no intersection
    if (d == 0)
        return false;

    // Get the x and y
    int pre = (x1 * y2 - y1 * x2), post = (x3 * y4 - y3 * x4);
    int x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
    int y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

    // Return the point of intersection
    oIntersection.m_X = x;
    oIntersection.m_Y = y;
    return true;
}

#endif