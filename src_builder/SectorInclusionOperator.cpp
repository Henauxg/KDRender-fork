#include "SectorInclusionOperator.h"

#include "GeomUtils.h"

SectorInclusionOperator::SectorInclusionOperator(std::vector<KDBData::Sector> &ioSectors) : m_Sectors(ioSectors),
                                                                                                 m_IsInsideMatrix(ioSectors.size())
{
}

SectorInclusionOperator::~SectorInclusionOperator()
{
}

void SectorInclusionOperator::ResetIsInsideMatrix()
{
    for (unsigned int i = 0; i < m_IsInsideMatrix.size(); i++)
    {
        m_IsInsideMatrix[i].clear();
        m_IsInsideMatrix[i].resize(m_IsInsideMatrix.size(), false);
    }
}

KDBData::Error SectorInclusionOperator::Run()
{
    KDBData::Error ret = KDBData::Error::OK;

    ResetIsInsideMatrix();
    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        for (unsigned int j = 0; j < m_Sectors.size(); j++)
        {
            if (j == i)
                continue;

            KDBData::Sector::Relationship rel = FindRelationship(m_Sectors[i], m_Sectors[j]);
            if (rel == KDBData::Sector::Relationship::PARTIAL)
                ret = KDBData::Error::SECTOR_INTERSECTION;

            if (rel == KDBData::Sector::Relationship::INSIDE)
                m_IsInsideMatrix[i][j] = true;
        }
    }

    if (ret != KDBData::Error::OK)
        return ret;

    m_Result.m_NbOfContainingSectors.resize(m_Sectors.size(), 0u);
    m_Result.m_SmallestContainingSectors.resize(m_Sectors.size(), -1);

    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        for (unsigned j = 0; j < m_Sectors.size(); j++)
        {
            if (m_IsInsideMatrix[i][j])
                m_Result.m_NbOfContainingSectors[i]++;
        }
    }

    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        if (m_Result.m_NbOfContainingSectors[i] > 0)
        {
            for (unsigned int j = 0; j < m_Sectors.size(); j++)
            {
                if (m_IsInsideMatrix[i][j] &&
                    (m_Result.m_NbOfContainingSectors[j] == m_Result.m_NbOfContainingSectors[i] - 1))
                {
                    for (const KDBData::Wall &wall : m_Sectors[i].m_Walls)
                    {
                        KDBData::Wall &mutableWall = const_cast<KDBData::Wall &>(wall);
                        mutableWall.m_OutSector = j;
                        m_Result.m_SmallestContainingSectors[i] = j;
                    }
                }
            }
        }
    }

    return ret;
}

KDBData::Sector::Relationship SectorInclusionOperator::FindRelationship(const KDBData::Sector &iSector1, const KDBData::Sector &iSector2) const
{
    // TODO: there are smarter ways to work out inclusion
    KDBData::Sector::Relationship ret = KDBData::Sector::Relationship::UNDETERMINED;

    unsigned int nbOnOutline = 0;
    unsigned int nbInside = 0;
    unsigned int nbOutside = 0;
    bool invalidLineIntersection = true;

    // Had to ensure that the intersection half-line that is used to find out whether thisVertex is in the polygon
    // doesn't cross a vertex of the polygon.
    for (int deltaYIntersectionLine = 0; deltaYIntersectionLine < 30 && invalidLineIntersection; deltaYIntersectionLine++)
    {
        KDBData::Sector::Relationship ret = KDBData::Sector::Relationship::UNDETERMINED;
        nbOnOutline = 0;
        nbInside = 0;
        nbOutside = 0;
        invalidLineIntersection = false;

        // Count the number of intersections between an input half line (that has thisVertex as origin)
        // and the other sector
        // If even: thisVertex is outside the other sector
        // If odd: thisVerteix is inside the other sector
        for (const KDBData::Wall &thisWall : iSector1.m_Walls)
        {
            const KDBData::Vertex &thisVertex = thisWall.m_VertexFrom;
            bool onOutline = false;

            // TODO: sort segments (in order to perform a binary search instead of linear)
            unsigned int nbIntersectionsAlongHalfLine = 0;
            for (const KDBData::Wall &otherWall : iSector2.m_Walls)
            {
                // TODO: "hard" intersection criterion won't work in every case as soon as
                // decimation takes textures into account
                // Leave it as is or find a better criterion?
                // TODO: since lines are either oX or oY aligned, calling generic SegmentSegment intersection
                // is a bit overkill
                // Note: SegmentSegment intersection won't return true if input segments are colinear, even if they
                // do intersect, so calling this function works here
                KDBData::Vertex intersection;
                if (SegmentSegmentIntersection<KDBData::Vertex, double>(thisWall.m_VertexFrom, thisWall.m_VertexTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection) &&
                    !(intersection == thisWall.m_VertexFrom) && !(intersection == thisWall.m_VertexTo) &&
                    !(intersection == otherWall.m_VertexFrom) && !(intersection == otherWall.m_VertexTo))
                {
                    ret = KDBData::Sector::Relationship::PARTIAL;
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
                    KDBData::Vertex halfLineTo = thisVertex;
                    halfLineTo.m_X += 100;
                    halfLineTo.m_Y += (50 + deltaYIntersectionLine);

                    KDBData::Vertex intersection;
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
                ret = KDBData::Sector::Relationship::PARTIAL;

            if (ret == KDBData::Sector::Relationship::PARTIAL)
                break;
        }
    }

    if ((nbInside + nbOnOutline) == iSector1.m_Walls.size())
        ret = KDBData::Sector::Relationship::INSIDE;
    else if ((nbOutside + nbOnOutline) == iSector1.m_Walls.size())
        ret = KDBData::Sector::Relationship::UNDETERMINED;
    else
        ret = KDBData::Sector::Relationship::PARTIAL;

    return ret;
}