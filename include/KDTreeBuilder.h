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
    KDBData::Error BuildSectors(const Map &iMap);
    KDBData::Error BuildKDTree(KDTreeMap *&oKDTree);

protected:
    KDBData::Error BuildSector(const Map::Data::Sector &iMapSector, KDBData::Sector &oSector);
    KDBData::Error BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<KDBData::Wall> &oWalls);
    KDBData::Error RecursiveBuildKDTree(std::list<KDBData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&oKDTree);
    void SplitWallSet(std::list<KDBData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane,
                      int &oSplitOffset, std::list<KDBData::Wall> &oPositiveSide, std::list<KDBData::Wall> &oNegativeSide,
                      std::list<KDBData::Wall> &oWithinSplitPlane);
    bool IsWallSetConvex(const std::list<KDBData::Wall> &iWalls) const;
    void ComputeWallSetAABB(const std::list<KDBData::Wall> &iWalls, KDMapData::Vertex &oAABBMin, KDMapData::Vertex &oAABBMax) const;

protected:
    // Inputs/outputs
    const Map &m_Map;
    KDTreeMap *m_pKDTree;

    // Intermediate structure
    std::vector<KDBData::Sector> m_Sectors;
};

#endif