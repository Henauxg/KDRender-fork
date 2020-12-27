#ifndef GeomUtils_h
#define GeomUtils_h

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

#endif