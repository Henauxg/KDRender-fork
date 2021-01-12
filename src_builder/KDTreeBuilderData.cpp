#include "KDTreeBuilderData.h"

#include "KDTreeMap.h"
#include "GeomUtils.h"

int KDBData::Vertex::GetCoord(unsigned int i) const
{
    switch (i)
    {
    case 0:
        return m_X;
    case 1:
        return m_Y;
    default:
        return 0; // Should never happen
    }
}

void KDBData::Vertex::SetCoord(unsigned int i, int iVal)
{
    switch (i)
    {
    case 0:
        m_X = iVal;
        break;
    case 1:
        m_Y = iVal;
        break;
    default:
        break; // Should never happen
    }
}

KDBData::Vertex::Vertex()
{

}

KDBData::Vertex::Vertex(const Map::Data::Sector::Vertex &iV)
{
    m_X = iV.m_X;
    m_Y = iV.m_Y;
}

bool KDBData::Wall::IsGeomEqual(const Wall &iOtherWall)
{
    return this->m_VertexFrom == iOtherWall.m_VertexFrom && this->m_VertexTo == iOtherWall.m_VertexTo;
}

bool KDBData::Wall::IsGeomOpposite(const Wall &iOtherWall) const
{
    return this->m_VertexFrom == iOtherWall.m_VertexTo && this->m_VertexTo == iOtherWall.m_VertexFrom;
}

unsigned int KDBData::Wall::GetConstCoordinate() const
{
    if (m_VertexFrom.m_X == m_VertexTo.m_X)
        return 0;
    else if (m_VertexFrom.m_Y == m_VertexTo.m_Y)
        return 1;
    else
        return 2; // Invalid, should not happen
}