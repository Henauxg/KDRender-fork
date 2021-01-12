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
    SectorInclusionOperator(std::vector<KDBData::Sector> &ioSectors);
    virtual ~SectorInclusionOperator();

public:
    KDBData::Error Run();
    const SectorInclusions &GetResult() const { return m_Result; }

protected:
    void ResetIsInsideMatrix();

    // Returns the relationship (inclusion, "hard" intersection or undetermined) between iSector1 and iSector2 (in this order)
    KDBData::Sector::Relationship FindRelationship(const KDBData::Sector &iSector1, const KDBData::Sector &iSector2) const;

protected:
    std::vector<KDBData::Sector> &m_Sectors;
    std::vector<std::vector<bool>> m_IsInsideMatrix;
    SectorInclusions m_Result;
};

#endif
