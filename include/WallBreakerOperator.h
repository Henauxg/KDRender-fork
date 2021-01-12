#ifndef WallBreakerOperator_h
#define WallBreakerOperator_h

class SectorInclusions;

#include "KDTreeBuilderData.h"

class WallBreakerOperator
{
public:
    WallBreakerOperator(const SectorInclusions &iSectorInclusions);
    virtual ~WallBreakerOperator();

public:
    KDBData::Error Run(const std::vector<KDBData::Wall> &iWalls, std::vector<KDBData::Wall> &oWalls);
    KDBData::Error RunOnCluster(const std::vector<KDBData::Wall> &iWalls, std::vector<KDBData::Wall> &oWalls);

protected:
    bool SplitWall(KDBData::Wall iWall, const KDBData::Vertex &iVertex,
                   KDBData::Wall &oMinWall, KDBData::Wall &oMaxWall) const;

protected:
    const SectorInclusions &m_SectorInclusions;
};

#endif