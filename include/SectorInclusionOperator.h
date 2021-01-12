#ifndef SectorInclusionOperator_h
#define SectorInclusionOperator_h

#include "KDTreeBuilderData.h"

#include <vector>

class SectorInclusions
{
public:
    SectorInclusions() {}
    virtual ~SectorInclusions() {}

public:
    int GetInnermostSector(int iSector1, int iSector2) const
    {
        if (iSector1 == -1)
            return iSector2;
        else if (iSector2 == -1)
            return iSector1;
        else if (m_NbOfContainingSectors[iSector1] < m_NbOfContainingSectors[iSector2])
            return iSector2;
        else
            return iSector1;
    }

    int GetOutermostSector(int iSector1, int iSector2) const
    {
        if (iSector1 == -1)
            return iSector1;
        else if (iSector2 == -1)
            return iSector2;
        else if (m_NbOfContainingSectors[iSector1] < m_NbOfContainingSectors[iSector2])
            return iSector1;
        else
            return iSector2;
    }

protected:
    std::vector<int> m_SmallestContainingSectors;
    std::vector<int> m_NbOfContainingSectors;

private:
    friend class SectorInclusionOperator;
};

class SectorInclusionOperator
{
public:
    SectorInclusionOperator(std::vector<MapBuildData::Sector> &ioSectors);
    virtual ~SectorInclusionOperator();

public:
    MapBuildData::ErrorCode Run();
    const SectorInclusions &GetResult() const { return m_Result; }

protected:
    void ResetIsInsideMatrix();

protected:
    std::vector<MapBuildData::Sector> &m_Sectors;
    std::vector<std::vector<bool>> m_IsInsideMatrix;
    SectorInclusions m_Result;
};

#endif