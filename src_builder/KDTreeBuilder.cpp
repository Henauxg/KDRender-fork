#include "KDTreeBuilder.h"

#include "Map.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include <vector>
#include <set>
#include <map>
#include <list>
#include <iterator>
#include <memory>
#include <algorithm>

class MapBuildData
{
public:
    enum class ErrorCode
    {
        OK,
        INVALID_POLYGON,
        SECTOR_INTERSECTION,
        UNKNOWN_FAILURE
    };

    struct Vertex
    {
        Vertex() {}

        int m_X;
        int m_Y;

        int GetCoord(unsigned int i) const
        {
            switch (i)
            {
            case 0:
                return m_X;
            case 1:
                return m_Y;
            default:
                return 0; // Should never happen
            }
        }

        void SetCoord(unsigned int i, int iVal)
        {
            switch (i)
            {
            case 0:
                m_X = iVal;
                break;
            case 1:
                m_Y = iVal;
                break;
            default:
                break; // Should never happen
            }
        }

        Vertex(const Map::Data::Sector::Vertex &iV)
        {
            m_X = iV.m_X;
            m_Y = iV.m_Y;
        }

        friend bool operator<(const Vertex& iLeft, const Vertex& iRight)
        {
            if(iLeft.m_X != iRight.m_X)
                return iLeft.m_X < iRight.m_X;
            else
                return iLeft.m_Y < iRight.m_Y;
        }

        friend bool operator==(const Vertex &iLeft, const Vertex &iRight)
        {
            return iLeft.m_X == iRight.m_X && iLeft.m_Y == iRight.m_Y;
        }
    };

    struct Wall
    {
        Wall() {}

        Vertex m_VertexFrom;
        Vertex m_VertexTo;

        friend bool operator<(const Wall &iLeft, const Wall &iRight)
        {
            if (!(iLeft.m_VertexFrom == iRight.m_VertexFrom))
                return iLeft.m_VertexFrom < iRight.m_VertexFrom;
            else
                return iLeft.m_VertexTo < iRight.m_VertexTo;
        }

        int m_InSector;
        int m_OutSector;
    };

    struct Sector
    {
        std::set<Wall> m_Walls;

        int m_Ceiling;
        int m_Floor;

        enum class Relationship
        {
            INSIDE,
            PARTIAL, // "hard" intersection
            UNDETERMINED
        };

        // Returns the relationship between this and iSector2 (in this order)
        Relationship FindRelationship(const Sector &iSector2) const
        {
            // TODO: there are smarter ways to work out inclusion
            Relationship ret = Relationship::UNDETERMINED;

            unsigned int nbOnOutline = 0;
            unsigned int nbInside = 0;
            unsigned int nbOutside = 0;
            for (const Wall &thisWall : m_Walls)
            {
                const Vertex &thisVertex = thisWall.m_VertexFrom;
                bool onOutline = false;

                // TODO: sort segments (in order to perform a binary search instead of linear)
                unsigned int nbIntersectionsAlongYPositive = 0;
                for (const Wall &otherWall : iSector2.m_Walls)
                {
                    // TODO: "hard" intersection criterion won't work in every case as soon as
                    // decimation takes textures into account
                    // Leave it as is or find a better criterion?
                    // TODO: since lines are either oX or oY aligned, calling generic SegmentSegment intersection
                    // is a bit overkill
                    // Note: SegmentSegment intersection won't return true if input segments are colinear, even if they
                    // do intersect, so calling this function works here
                    Vertex intersection;
                    if (SegmentSegmentIntersection(thisWall.m_VertexFrom, thisWall.m_VertexTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection) &&
                        !(intersection == thisWall.m_VertexFrom) && !(intersection == thisWall.m_VertexTo) &&
                        !(intersection == otherWall.m_VertexFrom) && !(intersection == otherWall.m_VertexTo))
                    {
                        ret = Relationship::PARTIAL;
                        break;
                    }

                    if(VertexOnXYAlignedSegment(otherWall.m_VertexFrom, otherWall.m_VertexTo, thisVertex))
                    {
                        nbOnOutline++;
                        onOutline = true;
                        break;
                    }
                    else
                    {
                        // TODO: Extremely non-robust, implement a voting system or we might miss out on outside vertices
                        Vertex halfLineTo = thisVertex;
                        halfLineTo.m_X += 100;
                        halfLineTo.m_Y += 52;

                        Vertex intersection;
                        if (HalfLineSegmentIntersection(thisVertex, halfLineTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection))
                        {
                            nbIntersectionsAlongYPositive++;
                        }
                    }
                }

                if (!onOutline)
                {
                    if (nbIntersectionsAlongYPositive % 2 == 1)
                        nbInside++;
                    else
                        nbOutside++;
                }

                if(nbInside > 0 && nbOutside > 0)
                    ret = Relationship::PARTIAL;

                if(ret == Relationship::PARTIAL)
                    break;
            }

            if((nbInside + nbOnOutline) == m_Walls.size())
                ret = Relationship::INSIDE;
            else if((nbOutside + nbOnOutline) == m_Walls.size())
                ret = Relationship::UNDETERMINED;
            else
                ret = Relationship::PARTIAL;

            return ret;
        }
    };

public:
    MapBuildData();
    virtual ~MapBuildData();

public:
    ErrorCode SetMap(const Map &iMap);
    ErrorCode BuildKDTree(KDTreeMap *&oKDTree);

protected:
    ErrorCode BuildSector(const Map::Data::Sector &iMapSector, Sector &oSector);
    ErrorCode BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::set<Wall> &oWalls);
    ErrorCode RecursiveBuildKDTree(std::set<Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&oKDTree);
    void SplitWallSet(std::set<Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::set<Wall> &oPositiveSide, std::set<Wall> &oNegativeSide, std::set<Wall> &oWithinSplitPlane);

protected:
    std::vector<Sector> m_Sectors;
    const Map::Data *m_pMapData;
};

class SectorInclusionOperator
{
public:
    SectorInclusionOperator(const std::vector<MapBuildData::Sector> &iSectors);
    virtual ~SectorInclusionOperator();

public:
    MapBuildData::ErrorCode Run();

protected:
    void ResetIsInsideMatrix();

protected:
    const std::vector<MapBuildData::Sector> &m_Sectors;
    std::vector<std::vector<bool>> m_IsInsideMatrix;
};

SectorInclusionOperator::SectorInclusionOperator(const std::vector<MapBuildData::Sector> &iSectors) :
    m_Sectors(iSectors),
    m_IsInsideMatrix(iSectors.size())
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
            if(j == i)
                continue;

            MapBuildData::Sector::Relationship rel = m_Sectors[i].FindRelationship(m_Sectors[j]);
            if (rel == MapBuildData::Sector::Relationship::PARTIAL)
                ret = MapBuildData::ErrorCode::SECTOR_INTERSECTION;

            if (rel == MapBuildData::Sector::Relationship::INSIDE)
                m_IsInsideMatrix[i][j] = true;
        }
    }

    if(ret != MapBuildData::ErrorCode::OK)
        return ret;

    std::vector<unsigned int> nbOfContainingSectors(m_Sectors.size(), 0u);
    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        for (unsigned j = 0; j < m_Sectors.size(); j++)
        {
            if (m_IsInsideMatrix[i][j])
                nbOfContainingSectors[i]++;
        }
    }

    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        if(nbOfContainingSectors[i] > 0)
        {
            for (unsigned int j = 0; j < m_Sectors.size(); j++)
            {
                if (m_IsInsideMatrix[i][j] && (nbOfContainingSectors[j] == nbOfContainingSectors[i] - 1))
                {
                    for(const MapBuildData::Wall &wall : m_Sectors[i].m_Walls)
                    {
                        MapBuildData::Wall &mutableWall = const_cast<MapBuildData::Wall &>(wall);
                        mutableWall.m_OutSector = j;
                    }
                }
            }
        }
    }

    return ret;
}

MapBuildData::MapBuildData() :
    m_pMapData(nullptr)
{
}

MapBuildData::~MapBuildData()
{
    
}

MapBuildData::ErrorCode MapBuildData::SetMap(const Map &iMap)
{
    m_pMapData = &iMap.GetData();
    for (unsigned int i = 0; i < m_pMapData->m_Sectors.size(); i++)
    {
        MapBuildData::Sector sector;

        ErrorCode err = BuildSector(m_pMapData->m_Sectors[i], sector);
        if(err != ErrorCode::OK)
            return err;

        for (const Wall &wall : sector.m_Walls)
        {
            Wall &mutableWall = const_cast<Wall &>(wall); // Ugly but screw this, I'm not at work
            mutableWall.m_InSector = i;
            mutableWall.m_OutSector = -1;
        }
        sector.m_Ceiling = m_pMapData->m_Sectors[i].m_Ceiling;
        sector.m_Floor = m_pMapData->m_Sectors[i].m_Floor;

        m_Sectors.push_back(sector);
    }

    

    return ErrorCode::OK;
}

MapBuildData::ErrorCode MapBuildData::BuildSector(const Map::Data::Sector &iMapSector, MapBuildData::Sector &oSector)
{
    ErrorCode error = ErrorCode::OK;

    std::vector<Wall> outlineWalls;
    error = BuildPolygon(iMapSector.m_Outline, 1, oSector.m_Walls);

    if(error != ErrorCode::OK)
        return error;

    for (unsigned int i = 0; i < iMapSector.m_Holes.size(); i++)
    {
        error = BuildPolygon(iMapSector.m_Holes[i], -1, oSector.m_Walls);
        if(error != ErrorCode::OK)
            break;
    }

    return error;
}

template<typename Iterator>
void FillWalls(Iterator iBegin, Iterator iEnd, std::set<MapBuildData::Wall> &oWalls)
{
    Iterator vit(iBegin);
    Iterator vitNext;
    for (; vit != iEnd; ++vit)
    {
        vitNext = std::next(vit);
        if(vitNext == iEnd)
            vitNext = iBegin;

        MapBuildData::Wall wall;
        wall.m_VertexFrom = *vit;
        wall.m_VertexTo = *vitNext;
        oWalls.insert(wall);
    }
}

MapBuildData::ErrorCode MapBuildData::BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::set<MapBuildData::Wall> &oWalls)
{
    ErrorCode error = ErrorCode::OK;

    if(iPolygon.empty())
        return ErrorCode::INVALID_POLYGON;

    std::list<Vertex> decimatedVertices;
    Vertex prevVertex(iPolygon[0]);
    int previousLineOrientation = 0; // 1 for horizontal line, -1 for vertical line

    for(unsigned int i = 0; i < iPolygon.size(); i++)
    {
        Vertex currVertex(iPolygon[(i + 1) % iPolygon.size()]);
        int deltaX = currVertex.m_X - prevVertex.m_X;
        int deltaY = currVertex.m_Y - prevVertex.m_Y;

        if((deltaX && deltaY) || (!deltaX && !deltaY))
        {
            error = ErrorCode::INVALID_POLYGON;
            break;
        }

        decimatedVertices.push_back(prevVertex);
        prevVertex = currVertex;
    }

    if(error != ErrorCode::OK)
        return error;

    std::list<Vertex>::iterator vit(decimatedVertices.begin());
    std::list<Vertex>::iterator vitNext;
    std::list<Vertex>::iterator vitPrev;
    while (vit != decimatedVertices.end())
    {
        vitNext = std::next(vit);
        if(vitNext == decimatedVertices.end())
            vitNext = decimatedVertices.begin();

        if(vit == decimatedVertices.begin())
            vitPrev = std::prev(decimatedVertices.end());
        else
            vitPrev = std::prev(vit);

        const Vertex &vPrev = *vitPrev;
        const Vertex &vCurr = *vit;
        const Vertex &vNext = *vitNext;

        if ((vPrev.m_X == vCurr.m_X && vCurr.m_X == vNext.m_X) ||
            (vPrev.m_Y == vCurr.m_Y && vCurr.m_Y == vNext.m_Y))
            vit = decimatedVertices.erase(vit);
        else
            vit++;
    }

    if(decimatedVertices.size() < 4)
        return ErrorCode::INVALID_POLYGON;

    vitPrev = decimatedVertices.begin();
    vit = std::next(vitPrev);
    vitNext = std::next(vit);

    bool reverseIter = iDesiredOrientation * WhichSide(*vitPrev, *vit, *vitNext) < 0;
    if(reverseIter)
        FillWalls<std::reverse_iterator<std::list<Vertex>::iterator>>(decimatedVertices.rbegin(), decimatedVertices.rend(), oWalls);
    else
        FillWalls<std::list<Vertex>::iterator>(decimatedVertices.begin(), decimatedVertices.end(), oWalls);

    return error;
}

MapBuildData::ErrorCode MapBuildData::BuildKDTree(KDTreeMap *&oKDTree)
{
    SectorInclusionOperator inclusionOper(m_Sectors);
    MapBuildData::ErrorCode ret = inclusionOper.Run();

    if(ret == MapBuildData::ErrorCode::OK)
    {
        std::set<Wall> allWalls;
        for (unsigned int i = 0; i < m_Sectors.size(); i++)
        {
            for (const Wall &wall : m_Sectors[i].m_Walls)
            {
                allWalls.insert(wall);
            }
        }

        oKDTree = new KDTreeMap;

        oKDTree->m_PlayerStartX = m_pMapData->m_PlayerStartPosition.first;
        oKDTree->m_PlayerStartY = m_pMapData->m_PlayerStartPosition.second;
        oKDTree->m_PlayerStartDirection = m_pMapData->m_PlayerStartDirection;

        oKDTree->m_RootNode = new KDTreeNode;

        for (unsigned int i = 0; i < m_Sectors.size(); i++)
        {
            KDMapData::Sector sector;
            sector.ceiling = m_Sectors[i].m_Ceiling;
            sector.floor = m_Sectors[i].m_Floor;
            oKDTree->m_Sectors.push_back(sector);
        }

        ret = RecursiveBuildKDTree(allWalls, KDTreeNode::SplitPlane::XConst, oKDTree->m_RootNode);
    }

    return ret;
}

MapBuildData::ErrorCode MapBuildData::RecursiveBuildKDTree(std::set<MapBuildData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&ioKDTreeNode)
{
    if(!ioKDTreeNode)
        return ErrorCode::UNKNOWN_FAILURE;

    int splitOffset;
    std::set<Wall> positiveSide, negativeSide, withinPlane;
    SplitWallSet(iWalls, iSplitPlane, splitOffset, positiveSide, negativeSide, withinPlane);

    if (withinPlane.empty())
    {
        positiveSide.clear();
        negativeSide.clear();
        iSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
        SplitWallSet(iWalls, iSplitPlane, splitOffset, positiveSide, negativeSide, withinPlane);
    }

    if(!withinPlane.empty())
    {
        ioKDTreeNode->m_SplitPlane = iSplitPlane;
        ioKDTreeNode->m_SplitOffset = splitOffset;
        for(const Wall &wall : withinPlane)
        {
            KDMapData::Wall kdWall;
            int splitPlaneInt = iSplitPlane == KDTreeNode::SplitPlane::XConst ? 0 : 1;
            kdWall.m_From = wall.m_VertexFrom.GetCoord((splitPlaneInt + 1) % 2);
            kdWall.m_To = wall.m_VertexTo.GetCoord((splitPlaneInt + 1) % 2);
            kdWall.m_InSector = wall.m_InSector;
            kdWall.m_OutSector = wall.m_OutSector;
            ioKDTreeNode->m_Walls.push_back(kdWall);
        }
    }

    if (!positiveSide.empty())
    {
        ioKDTreeNode->m_PositiveSide = new KDTreeNode;
        KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
        RecursiveBuildKDTree(positiveSide, newSplitPlane, ioKDTreeNode->m_PositiveSide);
    }

    if (!negativeSide.empty())
    {
        ioKDTreeNode->m_NegativeSide = new KDTreeNode;
        KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
        RecursiveBuildKDTree(negativeSide, newSplitPlane, ioKDTreeNode->m_NegativeSide);
    }

    return ErrorCode::OK;
}

void MapBuildData::SplitWallSet(std::set<MapBuildData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::set<MapBuildData::Wall> &oPositiveSide, std::set<MapBuildData::Wall> &oNegativeSide, std::set<MapBuildData::Wall> &oWithinSplitPlane)
{
    int splitPlaneInt = iSplitPlane == KDTreeNode::SplitPlane::XConst ? 0 : 1;

    std::vector<Wall> WallsParallelToSplitPlane;
    for(const Wall &wall : ioWalls)
    {
        if (wall.m_VertexFrom.GetCoord(splitPlaneInt) == wall.m_VertexTo.GetCoord(splitPlaneInt))
            WallsParallelToSplitPlane.push_back(wall);
    }

    if (WallsParallelToSplitPlane.empty())
        return;

    auto SortWalls = [&](const Wall &iWall1, const Wall &iWall2) {
        return iWall1.m_VertexFrom.GetCoord(splitPlaneInt) < iWall2.m_VertexFrom.GetCoord(splitPlaneInt);
    };
    std::sort(WallsParallelToSplitPlane.begin(), WallsParallelToSplitPlane.end(), SortWalls);
    oSplitOffset = WallsParallelToSplitPlane[(WallsParallelToSplitPlane.size() - 1) / 2].m_VertexFrom.GetCoord(splitPlaneInt);
    WallsParallelToSplitPlane.clear();

    std::set<Wall>::iterator sit(ioWalls.begin());
    while(sit != ioWalls.end())
    {
        if (sit->m_VertexFrom.GetCoord(splitPlaneInt) == oSplitOffset &&
            sit->m_VertexTo.GetCoord(splitPlaneInt) == oSplitOffset)
        {
            oWithinSplitPlane.insert(*sit);
        }
        else if (sit->m_VertexFrom.GetCoord(splitPlaneInt) >= oSplitOffset &&
                 sit->m_VertexTo.GetCoord(splitPlaneInt) >= oSplitOffset)
        {
            oPositiveSide.insert(*sit);
        }
        else if (sit->m_VertexFrom.GetCoord(splitPlaneInt) <= oSplitOffset &&
                 sit->m_VertexTo.GetCoord(splitPlaneInt) <= oSplitOffset)
        {
            oNegativeSide.insert(*sit);
        }
        else
        {
            Vertex splitVertex;
            splitVertex.SetCoord(splitPlaneInt, oSplitOffset);
            splitVertex.SetCoord((splitPlaneInt + 1) % 2, sit->m_VertexFrom.GetCoord((splitPlaneInt + 1) % 2));

            Wall newWall1(*sit);
            Wall newWall2(*sit);
            newWall1.m_VertexTo = splitVertex;
            newWall2.m_VertexFrom = splitVertex;

            if (newWall1.m_VertexFrom.GetCoord(splitPlaneInt) < oSplitOffset)
            {
                oNegativeSide.insert(newWall1);
                oPositiveSide.insert(newWall2);
            }
            else
            {
                oNegativeSide.insert(newWall2);
                oPositiveSide.insert(newWall1);
            }
        }

        sit = ioWalls.erase(sit);
    }
}

KDTreeBuilder::KDTreeBuilder(const Map &iMap):
    m_Map(iMap),
    m_pKDTree(nullptr)
{
}

KDTreeBuilder::~KDTreeBuilder()
{

    if(m_pKDTree)
        delete m_pKDTree;
    m_pKDTree = nullptr;
}

bool KDTreeBuilder::Build()
{
    MapBuildData mapData;
    
    bool success = mapData.SetMap(m_Map) == MapBuildData::ErrorCode::OK;
    if(!success)
        return false;

    MapBuildData::ErrorCode err = mapData.BuildKDTree(m_pKDTree);
    if(err != MapBuildData::ErrorCode::OK || !m_pKDTree)
        return false;

    return true;
}

KDTreeMap* KDTreeBuilder::GetOutputTree()
{
    KDTreeMap *pRet = m_pKDTree;
    m_pKDTree = nullptr;
    return pRet;
}