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

#endif