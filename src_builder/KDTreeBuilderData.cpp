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

KDBData::Sector::Relationship KDBData::Sector::FindRelationship(const Sector &iSector2) const
{
    // TODO: there are smarter ways to work out inclusion
    Relationship ret = Relationship::UNDETERMINED;

    unsigned int nbOnOutline = 0;
    unsigned int nbInside = 0;
    unsigned int nbOutside = 0;
    bool invalidLineIntersection = true;

    // Had to ensure that the intersection half-line that is used to find out whether thisVertex is in the polygon
    // doesn't cross a vertex of the polygon.
    for (int deltaYIntersectionLine = 0; deltaYIntersectionLine < 30 && invalidLineIntersection; deltaYIntersectionLine++)
    {
        Relationship ret = Relationship::UNDETERMINED;
        nbOnOutline = 0;
        nbInside = 0;
        nbOutside = 0;
        invalidLineIntersection = false;

        // Count the number of intersections between an input half line (that has thisVertex as origin)
        // and the other sector
        // If even: thisVertex is outside the other sector
        // If odd: thisVerteix is inside the other sector
        for (const Wall &thisWall : m_Walls)
        {
            const Vertex &thisVertex = thisWall.m_VertexFrom;
            bool onOutline = false;

            // TODO: sort segments (in order to perform a binary search instead of linear)
            unsigned int nbIntersectionsAlongHalfLine = 0;
            for (const Wall &otherWall : iSector2.m_Walls)
            {
                // TODO: "hard" intersection criterion won't work in every case as soon as
                // decimation takes textures into account
                // Leave it as is or find a better criterion?
                // TODO: since lines are either oX or oY aligned, calling generic SegmentSegment intersection
                // is a bit overkill
                // Note: SegmentSegment intersection won't return true if input segments are colinear, even if they
                // do intersect, so calling this function works here
                Vertex intersection;
                if (SegmentSegmentIntersection<KDBData::Vertex, double>(thisWall.m_VertexFrom, thisWall.m_VertexTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection) &&
                    !(intersection == thisWall.m_VertexFrom) && !(intersection == thisWall.m_VertexTo) &&
                    !(intersection == otherWall.m_VertexFrom) && !(intersection == otherWall.m_VertexTo))
                {
                    ret = Relationship::PARTIAL;
                    break;
                }

                if (VertexOnXYAlignedSegment(otherWall.m_VertexFrom, otherWall.m_VertexTo, thisVertex))
                {
                    nbOnOutline++;
                    onOutline = true;
                    break;
                }
                else
                {
                    Vertex halfLineTo = thisVertex;
                    halfLineTo.m_X += 100;
                    halfLineTo.m_Y += (50 + deltaYIntersectionLine);

                    Vertex intersection;
                    if (HalfLineSegmentIntersection<KDBData::Vertex, double>(thisVertex, halfLineTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection))
                    {
                        if (intersection == thisWall.m_VertexFrom || intersection == thisWall.m_VertexTo ||
                            intersection == otherWall.m_VertexFrom || intersection == otherWall.m_VertexTo)
                        {
                            invalidLineIntersection = true;
                            break;
                        }

                        nbIntersectionsAlongHalfLine++;
                    }
                }
            }

            if (!onOutline)
            {
                if (nbIntersectionsAlongHalfLine % 2 == 1)
                    nbInside++;
                else
                    nbOutside++;
            }

            if (nbInside > 0 && nbOutside > 0)
                ret = Relationship::PARTIAL;

            if (ret == Relationship::PARTIAL)
                break;
        }
    }

    if ((nbInside + nbOnOutline) == m_Walls.size())
        ret = Relationship::INSIDE;
    else if ((nbOutside + nbOnOutline) == m_Walls.size())
        ret = Relationship::UNDETERMINED;
    else
        ret = Relationship::PARTIAL;

    return ret;
}