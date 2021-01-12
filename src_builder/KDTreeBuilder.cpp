#include "KDTreeBuilder.h"

#include "GeomUtils.h"
#include "Map.h"
#include "KDTreeMap.h"
#include "SectorInclusionOperator.h"
#include "WallBreakerOperator.h"

#include <vector>
#include <list>
#include <iterator>
#include <memory>
#include <algorithm>

MapBuildData::ErrorCode KDTreeBuilder::BuildSectors(const Map &iMap)
{
    const Map::Data *pMapData = &iMap.GetData();
    for (unsigned int i = 0; i < pMapData->m_Sectors.size(); i++)
    {
        MapBuildData::Sector sector;

        MapBuildData::ErrorCode err = BuildSector(pMapData->m_Sectors[i], sector);
        if (err != MapBuildData::ErrorCode::OK)
            return err;

        for (const MapBuildData::Wall &wall : sector.m_Walls)
        {
            MapBuildData::Wall &mutableWall = const_cast<MapBuildData::Wall &>(wall); // Ugly but screw this, I'm not at work
            mutableWall.m_InSector = i;
            mutableWall.m_OutSector = -1;
        }
        sector.m_Ceiling = pMapData->m_Sectors[i].m_Ceiling;
        sector.m_Floor = pMapData->m_Sectors[i].m_Floor;

        m_Sectors.push_back(sector);
    }

    
    return MapBuildData::ErrorCode::OK;
}

MapBuildData::ErrorCode KDTreeBuilder::BuildSector(const Map::Data::Sector &iMapSector, MapBuildData::Sector &oSector)
{
    MapBuildData::ErrorCode error = MapBuildData::ErrorCode::OK;

    std::vector<MapBuildData::Wall> outlineWalls;
    error = BuildPolygon(iMapSector.m_Outline, 1, oSector.m_Walls);

    if (error != MapBuildData::ErrorCode::OK)
        return error;

    for (unsigned int i = 0; i < iMapSector.m_Holes.size(); i++)
    {
        error = BuildPolygon(iMapSector.m_Holes[i], -1, oSector.m_Walls);
        if (error != MapBuildData::ErrorCode::OK)
            break;
    }

    return error;
}

template<typename Iterator>
void FillWalls(Iterator iBegin, Iterator iEnd, std::list<MapBuildData::Wall> &oWalls)
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
        oWalls.push_back(wall);
    }
}

MapBuildData::ErrorCode KDTreeBuilder::BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<MapBuildData::Wall> &oWalls)
{
    MapBuildData::ErrorCode error = MapBuildData::ErrorCode::OK;

    if(iPolygon.empty())
        return MapBuildData::ErrorCode::INVALID_POLYGON;

    std::list<MapBuildData::Vertex> decimatedVertices;
    MapBuildData::Vertex prevVertex(iPolygon[0]);
    int previousLineOrientation = 0; // 1 for horizontal line, -1 for vertical line

    for(unsigned int i = 0; i < iPolygon.size(); i++)
    {
        MapBuildData::Vertex currVertex(iPolygon[(i + 1) % iPolygon.size()]);
        int deltaX = currVertex.m_X - prevVertex.m_X;
        int deltaY = currVertex.m_Y - prevVertex.m_Y;

        if((deltaX && deltaY) || (!deltaX && !deltaY))
        {
            error = MapBuildData::ErrorCode::INVALID_POLYGON;
            break;
        }

        decimatedVertices.push_back(prevVertex);
        prevVertex = currVertex;
    }

    if (error != MapBuildData::ErrorCode::OK)
        return error;

    // Decimate vertices
    // This is useful in order to detect "hard" sector intersections and raise an
    // error when they occur
    // TODO: When textures are supported, this step will have to be performed inside
    // the inclusion solver only (we might accidentally remove information otherwise)

    std::list<MapBuildData::Vertex>::iterator vit(decimatedVertices.begin());
    std::list<MapBuildData::Vertex>::iterator vitNext;
    std::list<MapBuildData::Vertex>::iterator vitPrev;
    while (vit != decimatedVertices.end())
    {
        vitNext = std::next(vit);
        if(vitNext == decimatedVertices.end())
            vitNext = decimatedVertices.begin();

        if(vit == decimatedVertices.begin())
            vitPrev = std::prev(decimatedVertices.end());
        else
            vitPrev = std::prev(vit);

        const MapBuildData::Vertex &vPrev = *vitPrev;
        const MapBuildData::Vertex &vCurr = *vit;
        const MapBuildData::Vertex &vNext = *vitNext;

        if ((vPrev.m_X == vCurr.m_X && vCurr.m_X == vNext.m_X) ||
            (vPrev.m_Y == vCurr.m_Y && vCurr.m_Y == vNext.m_Y))
            vit = decimatedVertices.erase(vit);
        else
            vit++;
    }

    if(decimatedVertices.size() < 4)
        return MapBuildData::ErrorCode::INVALID_POLYGON;

    vitPrev = decimatedVertices.begin();
    vit = std::next(vitPrev);
    vitNext = std::next(vit);

    bool reverseIter = iDesiredOrientation * WhichSide(*vitPrev, *vit, *vitNext) < 0;
    if(reverseIter)
        FillWalls<std::reverse_iterator<std::list<MapBuildData::Vertex>::iterator>>(decimatedVertices.rbegin(), decimatedVertices.rend(), oWalls);
    else
        FillWalls<std::list<MapBuildData::Vertex>::iterator>(decimatedVertices.begin(), decimatedVertices.end(), oWalls);

    return error;
}

MapBuildData::ErrorCode KDTreeBuilder::BuildKDTree(KDTreeMap *&oKDTree)
{
    SectorInclusionOperator inclusionOper(m_Sectors);
    MapBuildData::ErrorCode ret = inclusionOper.Run();

    if (ret == MapBuildData::ErrorCode::OK)
    {
        if (ret == MapBuildData::ErrorCode::OK)
        {
            // Walls are broken so no walls overlap
            // Wall breaking & in/out sector resolution is handled by WallBreakerOperator
            std::vector<MapBuildData::Wall> allWallsToBreak;
            for (unsigned int i = 0; i < m_Sectors.size(); i++)
            {
                for (const MapBuildData::Wall &wall : m_Sectors[i].m_Walls)
                {
                    allWallsToBreak.push_back(wall);
                }
            }

            std::vector<MapBuildData::Wall> allWalls;
            WallBreakerOperator wallBreakerOper(inclusionOper.GetResult());
            ret = wallBreakerOper.Run(allWallsToBreak, allWalls);

            if(ret == MapBuildData::ErrorCode::OK)
            {
                oKDTree = new KDTreeMap;

                oKDTree->m_PlayerStartX = m_Map.GetData().m_PlayerStartPosition.first;
                oKDTree->m_PlayerStartY = m_Map.GetData().m_PlayerStartPosition.second;
                oKDTree->m_PlayerStartDirection = m_Map.GetData().m_PlayerStartDirection;

                oKDTree->m_RootNode = new KDTreeNode;

                for (unsigned int i = 0; i < m_Sectors.size(); i++)
                {
                    KDMapData::Sector sector;
                    sector.ceiling = m_Sectors[i].m_Ceiling;
                    sector.floor = m_Sectors[i].m_Floor;
                    oKDTree->m_Sectors.push_back(sector);
                }

                // Damn I really suck at writing consistent code :(
                std::list<MapBuildData::Wall> allWallsList(allWalls.begin(), allWalls.end());
                allWalls.clear();
                ret = RecursiveBuildKDTree(allWallsList, KDTreeNode::SplitPlane::XConst, oKDTree->m_RootNode);
            }
        }
    }

    return ret;
}

bool KDTreeBuilder::IsWallSetConvex(const std::list<MapBuildData::Wall> &iWalls)
{
    bool isConvex = true;

    if(iWalls.empty())
        return true;

    // Only walls that separate the inside and the outside of the map
    // can be considered a convex set
    int inSector = iWalls.begin()->m_InSector;
    for (const MapBuildData::Wall &wall : iWalls)
    {
        if(wall.m_OutSector != -1)
            isConvex = false;
        if(wall.m_InSector != inSector)
            isConvex = false;
    }

    if(!isConvex)
        return false;

    // Compute the axis-aligned bounding box
    MapBuildData::Vertex bboxMin = iWalls.begin()->m_VertexFrom;
    MapBuildData::Vertex bboxMax = bboxMin;
    for (const MapBuildData::Wall &wall : iWalls)
    {
        for(unsigned int coord = 0; coord < 2; coord++)
        {
            if (wall.m_VertexFrom.GetCoord(coord) < bboxMin.GetCoord(coord))
                bboxMin.SetCoord(coord, wall.m_VertexFrom.GetCoord(coord));
            if (wall.m_VertexTo.GetCoord(coord) < bboxMin.GetCoord(coord))
                bboxMin.SetCoord(coord, wall.m_VertexTo.GetCoord(coord));

            if (wall.m_VertexFrom.GetCoord(coord) > bboxMax.GetCoord(coord))
                bboxMax.SetCoord(coord, wall.m_VertexFrom.GetCoord(coord));
            if (wall.m_VertexTo.GetCoord(coord) > bboxMax.GetCoord(coord))
                bboxMax.SetCoord(coord, wall.m_VertexTo.GetCoord(coord));
        }
    }

    // Due to the X/Y-alignment property of our polygons, the only possible convex hull
    // is the axis-aligned bounding box of the input walls
    // We therefore have to check that every input wall is on the border of the bounding box
    for (const MapBuildData::Wall &wall : iWalls)
    {
        // If one wall isn't on the bbox, return false
        int constCoord = wall.GetConstCoordinate();
        if (wall.m_VertexFrom.GetCoord(constCoord) != bboxMin.GetCoord(constCoord) &&
            wall.m_VertexFrom.GetCoord(constCoord) != bboxMax.GetCoord(constCoord))
            isConvex = false;

        // Check orientation of the wall to make sure the set is convex
        // (The wall may be part of a hole)
        if (constCoord == 0 && bboxMax.m_X > bboxMin.m_X)
        {
            if (wall.m_VertexFrom.m_X == bboxMax.m_X && 
                wall.m_VertexFrom.m_Y < wall.m_VertexTo.m_Y)
                isConvex = false;
            else if (wall.m_VertexFrom.m_X == bboxMin.m_X &&
                     wall.m_VertexFrom.m_Y > wall.m_VertexTo.m_Y)
                isConvex = false;
        }
        else if (constCoord == 1 && bboxMax.m_Y > bboxMin.m_Y)
        {
            if (wall.m_VertexFrom.m_Y == bboxMax.m_Y &&
                wall.m_VertexFrom.m_X > wall.m_VertexTo.m_X)
                isConvex = false;
            else if (wall.m_VertexFrom.m_Y == bboxMin.m_Y &&
                     wall.m_VertexFrom.m_X < wall.m_VertexTo.m_X)
                isConvex = false;
        }
    }

    return isConvex;
}

MapBuildData::ErrorCode KDTreeBuilder::RecursiveBuildKDTree(std::list<MapBuildData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&ioKDTreeNode)
{
    if(!ioKDTreeNode)
        return MapBuildData::ErrorCode::UNKNOWN_FAILURE;

    // No need to divide the set of wall any further
    if(IsWallSetConvex(iWalls))
    {
        ioKDTreeNode->m_SplitPlane = KDTreeNode::SplitPlane::None;
        ioKDTreeNode->m_SplitOffset = 0; // Will never be used

        for (const MapBuildData::Wall &wall : iWalls)
        {
            KDMapData::Wall kdWall;

            kdWall.m_From.m_X = wall.m_VertexFrom.m_X;
            kdWall.m_From.m_Y = wall.m_VertexFrom.m_Y;

            kdWall.m_To.m_X = wall.m_VertexTo.m_X;
            kdWall.m_To.m_Y = wall.m_VertexTo.m_Y;

            kdWall.m_InSector = wall.m_InSector;
            kdWall.m_OutSector = wall.m_OutSector;

            ioKDTreeNode->m_Walls.push_back(kdWall);
        }

        return MapBuildData::ErrorCode::OK;
    }
    else
    {
        int splitOffset;
        std::list<MapBuildData::Wall> positiveSide, negativeSide, withinPlane;
        SplitWallSet(iWalls, iSplitPlane, splitOffset, positiveSide, negativeSide, withinPlane);

        // TODO: remove?
        if (withinPlane.empty())
        {
            positiveSide.clear();
            negativeSide.clear();
            iSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
            SplitWallSet(iWalls, iSplitPlane, splitOffset, positiveSide, negativeSide, withinPlane);
        }

        if (!withinPlane.empty())
        {
            ioKDTreeNode->m_SplitPlane = iSplitPlane;
            ioKDTreeNode->m_SplitOffset = splitOffset;
            for (const MapBuildData::Wall &wall : withinPlane)
            {
                KDMapData::Wall kdWall;

                kdWall.m_From.m_X = wall.m_VertexFrom.m_X;
                kdWall.m_From.m_Y = wall.m_VertexFrom.m_Y;

                kdWall.m_To.m_X = wall.m_VertexTo.m_X;
                kdWall.m_To.m_Y = wall.m_VertexTo.m_Y;

                kdWall.m_InSector = wall.m_InSector;
                kdWall.m_OutSector = wall.m_OutSector;

                ioKDTreeNode->m_Walls.push_back(kdWall);
            }
        }

        if (!positiveSide.empty())
        {
            ioKDTreeNode->m_PositiveSide = new KDTreeNode;
            KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
            MapBuildData::ErrorCode ret = RecursiveBuildKDTree(positiveSide, newSplitPlane, ioKDTreeNode->m_PositiveSide);
            if (ret != MapBuildData::ErrorCode::OK)
                return ret;
        }

        if (!negativeSide.empty())
        {
            ioKDTreeNode->m_NegativeSide = new KDTreeNode;
            KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
            MapBuildData::ErrorCode ret = RecursiveBuildKDTree(negativeSide, newSplitPlane, ioKDTreeNode->m_NegativeSide);
            if (ret != MapBuildData::ErrorCode::OK)
                return ret;
        }

        return MapBuildData::ErrorCode::OK;
    }
}

void KDTreeBuilder::SplitWallSet(std::list<MapBuildData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::list<MapBuildData::Wall> &oPositiveSide, std::list<MapBuildData::Wall> &oNegativeSide, std::list<MapBuildData::Wall> &oWithinSplitPlane)
{
    int splitPlaneInt = iSplitPlane == KDTreeNode::SplitPlane::XConst ? 0 : (iSplitPlane == KDTreeNode::SplitPlane::YConst ? 1 : 2);

    std::vector<MapBuildData::Wall> WallsParallelToSplitPlane;
    for (const MapBuildData::Wall &wall : ioWalls)
    {
        if (wall.m_VertexFrom.GetCoord(splitPlaneInt) == wall.m_VertexTo.GetCoord(splitPlaneInt))
            WallsParallelToSplitPlane.push_back(wall);
    }

    if (WallsParallelToSplitPlane.empty())
        return;

    std::sort(WallsParallelToSplitPlane.begin(), WallsParallelToSplitPlane.end());
    oSplitOffset = WallsParallelToSplitPlane[(WallsParallelToSplitPlane.size() - 1) / 2].m_VertexFrom.GetCoord(splitPlaneInt);
    WallsParallelToSplitPlane.clear();

    std::list<MapBuildData::Wall>::iterator sit(ioWalls.begin());
    while(sit != ioWalls.end())
    {
        if (sit->m_VertexFrom.GetCoord(splitPlaneInt) == oSplitOffset &&
            sit->m_VertexTo.GetCoord(splitPlaneInt) == oSplitOffset)
        {
            oWithinSplitPlane.push_back(*sit);
        }
        else if (sit->m_VertexFrom.GetCoord(splitPlaneInt) >= oSplitOffset &&
                 sit->m_VertexTo.GetCoord(splitPlaneInt) >= oSplitOffset)
        {
            oPositiveSide.push_back(*sit);
        }
        else if (sit->m_VertexFrom.GetCoord(splitPlaneInt) <= oSplitOffset &&
                 sit->m_VertexTo.GetCoord(splitPlaneInt) <= oSplitOffset)
        {
            oNegativeSide.push_back(*sit);
        }
        else
        {
            MapBuildData::Vertex splitVertex;
            splitVertex.SetCoord(splitPlaneInt, oSplitOffset);
            splitVertex.SetCoord((splitPlaneInt + 1) % 2, sit->m_VertexFrom.GetCoord((splitPlaneInt + 1) % 2));

            MapBuildData::Wall newWall1(*sit);
            MapBuildData::Wall newWall2(*sit);
            newWall1.m_VertexTo = splitVertex;
            newWall2.m_VertexFrom = splitVertex;

            if (newWall1.m_VertexFrom.GetCoord(splitPlaneInt) < oSplitOffset)
            {
                oNegativeSide.push_back(newWall1);
                oPositiveSide.push_back(newWall2);
            }
            else
            {
                oNegativeSide.push_back(newWall2);
                oPositiveSide.push_back(newWall1);
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
    bool success = BuildSectors(m_Map) == MapBuildData::ErrorCode::OK;
    if(!success)
        return false;

    MapBuildData::ErrorCode err = BuildKDTree(m_pKDTree);
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