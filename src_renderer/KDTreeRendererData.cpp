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
        if (m_MinY[x] != WINDOW_HEIGHT || m_MaxY[x] != 0)
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