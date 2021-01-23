#include "KDTreeRendererData.h"

#include <cstring>

KDRData::FlatSurface::FlatSurface()
{
    // std::fill(m_MinY.begin(), m_MinY.end(), WINDOW_HEIGHT);
    // std::fill(m_MaxY.begin(), m_MaxY.end(), 0);
    for (unsigned int x = 0; x < WINDOW_WIDTH; x++)
    {
        m_MinY[x] = WINDOW_HEIGHT;
        m_MaxY[x] = 0;
    }
}

KDRData::FlatSurface::FlatSurface(const FlatSurface &iOther) : 
    m_MinX(iOther.m_MinX),
    m_MaxX(iOther.m_MaxX),
    m_Height(iOther.m_Height),
    m_SectorIdx(iOther.m_SectorIdx),
    m_TexId(iOther.m_TexId)
{
    memcpy(m_MinY, iOther.m_MinY, sizeof(int) * WINDOW_WIDTH);
    memcpy(m_MaxY, iOther.m_MaxY, sizeof(int) * WINDOW_WIDTH);
}

bool KDRData::FlatSurface::Absorb(const FlatSurface &iOther)
{
    if (iOther.m_Height != m_Height || iOther.m_SectorIdx != m_SectorIdx)
        return false;

    bool doAbsorb = true;
    for (unsigned int x = iOther.m_MinX; x <= iOther.m_MaxX; x++)
    {
        // There is actual data here, cannot absorb
        if (m_MinY[x] <= m_MaxY[x])
        {
            doAbsorb = false;
            break;
        }
    }

    if(doAbsorb)
    {
        memcpy(m_MinY + iOther.m_MinX, iOther.m_MinY + iOther.m_MinX, (iOther.m_MaxX - iOther.m_MinX + 1) * sizeof(int));
        memcpy(m_MaxY + iOther.m_MinX, iOther.m_MaxY + iOther.m_MinX, (iOther.m_MaxX - iOther.m_MinX + 1) * sizeof(int));

        m_MinX = std::min(iOther.m_MinX, m_MinX);
        m_MaxX = std::max(iOther.m_MaxX, m_MaxX);
    }

    return doAbsorb;
}

void KDRData::FlatSurface::Tighten()
{
    for (unsigned int x = m_MinX; x <= m_MaxX; x++)
    {
        if (m_MinY[x] <= m_MaxY[x])
        {
            m_MinX = x;
            break;
        }
    }

    for (unsigned int x = m_MaxX; x >= m_MinX; x--)
    {
        if (m_MinY[x] <= m_MaxY[x])
        {
            m_MaxX = x;
            break;
        }
    }
}

KDRData::Wall KDRData::GetWallFromNode(KDTreeNode *ipNode, unsigned int iWallIdx)
{
    KDRData::Wall wall;

    if (ipNode)
    {
        wall = ipNode->BuildXYScaledWall<KDRData::Wall>(iWallIdx);
        wall.m_pKDWall = ipNode->GetWall(iWallIdx);
    }

    return wall;
}

KDRData::Sector KDRData::GetSectorFromKDSector(const KDMapData::Sector &iSector)
{
    KDRData::Sector sector;

    sector.m_pKDSector = &iSector;
    sector.m_Ceiling = CType(iSector.ceiling) / POSITION_SCALE;
    sector.m_Floor = CType(iSector.floor) / POSITION_SCALE;

    return sector;
}

KDRData::HorizontalScreenSegments::HorizontalScreenSegments()
{
}

KDRData::HorizontalScreenSegments::~HorizontalScreenSegments()
{
}

void KDRData::HorizontalScreenSegments::AddScreenSegment(unsigned int iMinX, unsigned int iMaxX)
{
    InsertIntervalEnd({iMinX, 1});
    InsertIntervalEnd({iMaxX, -1});

    // Merge overlapping intervals
    int stack = 0;
    int previousDirection = 1;
    auto it = m_Segments.begin();
    while(it != m_Segments.end() && stack >= 0) // stack >= should always be true
    {
        if (it->m_Direction == 1 && previousDirection == 1)
            stack++;
        else if(it->m_Direction == -1 && previousDirection == -1)
            stack--;
        previousDirection = it->m_Direction;

        if (stack >= 2)
            it = m_Segments.erase(it);
        else
            it++;
    }

    // Merge consecutive intervals
    it = m_Segments.begin();
    previousDirection = 1;
    int previousValue = 0;
    while (it != m_Segments.end())
    {
        bool doMerge = false;
        if(previousDirection == -1 && previousValue == it->m_X - 1)
            doMerge = true;

        previousValue = it->m_X;
        previousDirection = it->m_Direction;

        if (doMerge)
            it = m_Segments.erase(it);
        else
            it++;
    }
}

void KDRData::HorizontalScreenSegments::InsertIntervalEnd(const IntervalEnd &iEnd)
{
    // Insert in sorted list
    auto it = m_Segments.begin();
    while (it != m_Segments.end() && *it < iEnd)
        it++;
    m_Segments.insert(it, iEnd);
}

bool KDRData::HorizontalScreenSegments::IsScreenEntirelyDrawn() const
{
    return m_Segments.size() == 2 && m_Segments.back().m_X == 0 && m_Segments.front().m_X == WINDOW_WIDTH - 1;
}