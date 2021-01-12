#include "SectorInclusionOperator.h"

SectorInclusionOperator::SectorInclusionOperator(std::vector<MapBuildData::Sector> &ioSectors) : m_Sectors(ioSectors),
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

MapBuildData::ErrorCode SectorInclusionOperator::Run()
{
    MapBuildData::ErrorCode ret = MapBuildData::ErrorCode::OK;

    ResetIsInsideMatrix();
    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        for (unsigned int j = 0; j < m_Sectors.size(); j++)
        {
            if (j == i)
                continue;

            MapBuildData::Sector::Relationship rel = m_Sectors[i].FindRelationship(m_Sectors[j]);
            if (rel == MapBuildData::Sector::Relationship::PARTIAL)
                ret = MapBuildData::ErrorCode::SECTOR_INTERSECTION;

            if (rel == MapBuildData::Sector::Relationship::INSIDE)
                m_IsInsideMatrix[i][j] = true;
        }
    }

    if (ret != MapBuildData::ErrorCode::OK)
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
                    for (const MapBuildData::Wall &wall : m_Sectors[i].m_Walls)
                    {
                        MapBuildData::Wall &mutableWall = const_cast<MapBuildData::Wall &>(wall);
                        mutableWall.m_OutSector = j;
                        m_Result.m_SmallestContainingSectors[i] = j;
                    }
                }
            }
        }
    }

    return ret;
}