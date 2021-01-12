#ifndef KDTreeBuilder_h
#define KDTreeBuilder_h

#include "KDTreeMap.h"
#include "KDTreeBuilderData.h"

class Map;
class KDTreeMap;

class KDTreeBuilder
{
public:
    KDTreeBuilder(const Map &iMap);
    virtual ~KDTreeBuilder();

public:
    bool Build();
    KDTreeMap* GetOutputTree();

protected:
    MapBuildData::ErrorCode BuildSectors(const Map &iMap);
    MapBuildData::ErrorCode BuildKDTree(KDTreeMap *&oKDTree);

protected:
    MapBuildData::ErrorCode BuildSector(const Map::Data::Sector &iMapSector, MapBuildData::Sector &oSector);
    MapBuildData::ErrorCode BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<MapBuildData::Wall> &oWalls);
    MapBuildData::ErrorCode RecursiveBuildKDTree(std::list<MapBuildData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&oKDTree);
    void SplitWallSet(std::list<MapBuildData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane,
                      int &oSplitOffset, std::list<MapBuildData::Wall> &oPositiveSide, std::list<MapBuildData::Wall> &oNegativeSide,
                      std::list<MapBuildData::Wall> &oWithinSplitPlane);
    bool IsWallSetConvex(const std::list<MapBuildData::Wall> &iWalls);

protected:
    // Inputs/outputs
    const Map &m_Map;
    KDTreeMap *m_pKDTree;

    // Intermediate structure
    std::vector<MapBuildData::Sector> m_Sectors;
};

#endif