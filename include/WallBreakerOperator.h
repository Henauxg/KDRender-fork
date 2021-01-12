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
    MapBuildData::ErrorCode Run(const std::vector<MapBuildData::Wall> &iWalls, std::vector<MapBuildData::Wall> &oWalls);
    MapBuildData::ErrorCode RunOnCluster(const std::vector<MapBuildData::Wall> &iWalls, std::vector<MapBuildData::Wall> &oWalls);

protected:
    bool SplitWall(MapBuildData::Wall iWall, const MapBuildData::Vertex &iVertex,
                   MapBuildData::Wall &oMinWall, MapBuildData::Wall &oMaxWall) const;

protected:
    const SectorInclusions &m_SectorInclusions;
};

#endif