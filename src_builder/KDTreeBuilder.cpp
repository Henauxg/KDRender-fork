#include "KDTreeBuilder.h"

#include "GeomUtils.h"
#include "Map.h"
#include "KDTreeMap.h"
#include "SectorInclusionOperator.h"
#include "WallBreakerOperator.h"
#include "ImageFromFileOperator.h"

#include <vector>
#include <list>
#include <iterator>
#include <memory>
#include <algorithm>

KDBData::Error KDTreeBuilder::BuildSectors(const Map &iMap)
{
    const Map::Data *pMapData = &iMap.GetData();
    for (unsigned int i = 0; i < pMapData->m_Sectors.size(); i++)
    {
        KDBData::Sector sector;

        KDBData::Error err = BuildSector(pMapData->m_Sectors[i], sector);
        if (err != KDBData::Error::OK)
            return err;

        for (KDBData::Wall &wall : sector.m_Walls)
        {
            wall.m_InSector = i;
            wall.m_OutSector = -1;
        }
        sector.m_Ceiling = pMapData->m_Sectors[i].m_Ceiling;
        sector.m_Floor = pMapData->m_Sectors[i].m_Floor;

        m_Sectors.push_back(sector);
    }

    
    return KDBData::Error::OK;
}

KDBData::Error KDTreeBuilder::BuildSector(const Map::Data::Sector &iMapSector, KDBData::Sector &oSector)
{
    KDBData::Error error = KDBData::Error::OK;

    std::vector<KDBData::Wall> outlineWalls;
    error = BuildPolygon(iMapSector.m_Outline, 1, oSector.m_Walls);

    if (error != KDBData::Error::OK)
        return error;

    for (unsigned int i = 0; i < iMapSector.m_Holes.size(); i++)
    {
        error = BuildPolygon(iMapSector.m_Holes[i], -1, oSector.m_Walls);
        if (error != KDBData::Error::OK)
            break;
    }

    return error;
}

template<typename Iterator>
void FillWalls(Iterator iBegin, Iterator iEnd, std::list<KDBData::Wall> &oWalls)
{
    Iterator vit(iBegin);
    Iterator vitNext;
    for (; vit != iEnd; ++vit)
    {
        vitNext = std::next(vit);
        if(vitNext == iEnd)
            vitNext = iBegin;

        KDBData::Wall wall;
        wall.m_VertexFrom = *vit;
        wall.m_VertexTo = *vitNext;
        oWalls.push_back(wall);
    }
}

KDBData::Error KDTreeBuilder::BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<KDBData::Wall> &oWalls)
{
    KDBData::Error error = KDBData::Error::OK;

    if(iPolygon.empty())
        return KDBData::Error::INVALID_POLYGON;

    std::list<KDBData::Vertex> decimatedVertices;
    KDBData::Vertex prevVertex(iPolygon[0]);
    int previousLineOrientation = 0; // 1 for horizontal line, -1 for vertical line

    for(unsigned int i = 0; i < iPolygon.size(); i++)
    {
        KDBData::Vertex currVertex(iPolygon[(i + 1) % iPolygon.size()]);
        int deltaX = currVertex.m_X - prevVertex.m_X;
        int deltaY = currVertex.m_Y - prevVertex.m_Y;

        if((deltaX && deltaY) || (!deltaX && !deltaY))
        {
            error = KDBData::Error::INVALID_POLYGON;
            break;
        }

        decimatedVertices.push_back(prevVertex);
        prevVertex = currVertex;
    }

    if (error != KDBData::Error::OK)
        return error;

    // Decimate vertices
    // This is useful in order to detect "hard" sector intersections and raise an
    // error when they occur
    // TODO: When textures are supported, this step will have to be performed inside
    // the inclusion solver only (we might accidentally remove information otherwise)

    std::list<KDBData::Vertex>::iterator vit(decimatedVertices.begin());
    std::list<KDBData::Vertex>::iterator vitNext;
    std::list<KDBData::Vertex>::iterator vitPrev;
    // while (vit != decimatedVertices.end())
    // {
    //     vitNext = std::next(vit);
    //     if(vitNext == decimatedVertices.end())
    //         vitNext = decimatedVertices.begin();

    //     if(vit == decimatedVertices.begin())
    //         vitPrev = std::prev(decimatedVertices.end());
    //     else
    //         vitPrev = std::prev(vit);

    //     const KDBData::Vertex &vPrev = *vitPrev;
    //     const KDBData::Vertex &vCurr = *vit;
    //     const KDBData::Vertex &vNext = *vitNext;

    //     if ((vPrev.m_X == vCurr.m_X && vCurr.m_X == vNext.m_X) ||
    //         (vPrev.m_Y == vCurr.m_Y && vCurr.m_Y == vNext.m_Y))
    //         vit = decimatedVertices.erase(vit);
    //     else
    //         vit++;
    // }

    if(decimatedVertices.size() < 4)
        return KDBData::Error::INVALID_POLYGON;

    vitPrev = decimatedVertices.begin();
    vit = std::next(vitPrev);
    vitNext = std::next(vit);

    bool reverseIter = iDesiredOrientation * WhichSide(*vitPrev, *vit, *vitNext) < 0;
    if(reverseIter)
        FillWalls<std::reverse_iterator<std::list<KDBData::Vertex>::iterator>>(decimatedVertices.rbegin(), decimatedVertices.rend(), oWalls);
    else
        FillWalls<std::list<KDBData::Vertex>::iterator>(decimatedVertices.begin(), decimatedVertices.end(), oWalls);

    // Report texture info
    if(reverseIter)
    {
        // The map designer had direct orientation in mind, which is why we
        // must be careful when reporting the texture coordinate from the current
        // vertex to the matching wall
        int i = iPolygon.size() - 2;
        for (KDBData::Wall &wall : oWalls)
        {
            i = i == -1 ? iPolygon.size() - 1 : i;
            wall.m_TexId = iPolygon[i].m_TexId;
            wall.m_TexUOffset = iPolygon[i].m_TexUOffset;
            wall.m_TexVOffset = iPolygon[i].m_TexVOffset;
            i--;
        }
    }
    else
    {
        unsigned int i = 0;
        for (KDBData::Wall &wall : oWalls)
        {
            wall.m_TexId = iPolygon[i].m_TexId;
            wall.m_TexUOffset = iPolygon[i].m_TexUOffset;
            wall.m_TexVOffset = iPolygon[i].m_TexVOffset;
            i++;
        }
    }

    return error;
}

KDBData::Error KDTreeBuilder::BuildKDTree(KDTreeMap *&oKDTree)
{
    SectorInclusionOperator inclusionOper(m_Sectors);
    KDBData::Error ret = inclusionOper.Run();

    if (ret == KDBData::Error::OK)
    {
        if (ret == KDBData::Error::OK)
        {
            // Walls are broken so no walls overlap
            // Wall breaking & in/out sector resolution is handled by WallBreakerOperator
            std::vector<KDBData::Wall> allWallsToBreak;
            for (unsigned int i = 0; i < m_Sectors.size(); i++)
            {
                for (const KDBData::Wall &wall : m_Sectors[i].m_Walls)
                {
                    allWallsToBreak.push_back(wall);
                }
            }

            std::vector<KDBData::Wall> allWalls;
            WallBreakerOperator wallBreakerOper(inclusionOper.GetResult(), m_Sectors);
            ret = wallBreakerOper.Run(allWallsToBreak, allWalls);

            if(ret == KDBData::Error::OK)
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
                std::list<KDBData::Wall> allWallsList(allWalls.begin(), allWalls.end());
                allWalls.clear();
                ret = RecursiveBuildKDTree(allWallsList, KDTreeNode::SplitPlane::XConst, oKDTree->m_RootNode);
            }
        }
    }

    return ret;
}

bool KDTreeBuilder::IsWallSetConvex(const std::list<KDBData::Wall> &iWalls)
{
    bool isConvex = true;

    if(iWalls.empty())
        return true;

    // Only walls that separate the inside and the outside of the map
    // can be considered a convex set
    int inSector = iWalls.begin()->m_InSector;
    for (const KDBData::Wall &wall : iWalls)
    {
        if(wall.m_OutSector != -1)
            isConvex = false;
        if(wall.m_InSector != inSector)
            isConvex = false;
    }

    if(!isConvex)
        return false;

    // Compute the axis-aligned bounding box
    KDBData::Vertex bboxMin = iWalls.begin()->m_VertexFrom;
    KDBData::Vertex bboxMax = bboxMin;
    for (const KDBData::Wall &wall : iWalls)
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
    for (const KDBData::Wall &wall : iWalls)
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

KDBData::Error KDTreeBuilder::RecursiveBuildKDTree(std::list<KDBData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&ioKDTreeNode)
{
    if(!ioKDTreeNode)
        return KDBData::Error::UNKNOWN_FAILURE;

    // No need to divide the set of wall any further
    if(IsWallSetConvex(iWalls))
    {
        ioKDTreeNode->m_SplitPlane = KDTreeNode::SplitPlane::None;
        ioKDTreeNode->m_SplitOffset = 0; // Will never be used

        for (const KDBData::Wall &wall : iWalls)
        {
            KDMapData::Wall kdWall;

            kdWall.m_From.m_X = wall.m_VertexFrom.m_X;
            kdWall.m_From.m_Y = wall.m_VertexFrom.m_Y;

            kdWall.m_To.m_X = wall.m_VertexTo.m_X;
            kdWall.m_To.m_Y = wall.m_VertexTo.m_Y;

            kdWall.m_InSector = wall.m_InSector;
            kdWall.m_OutSector = wall.m_OutSector;

            kdWall.m_TexId = wall.m_TexId;
            kdWall.m_TexUOffset = wall.m_TexUOffset;
            kdWall.m_TexVOffset = wall.m_TexVOffset;

            ioKDTreeNode->m_Walls.push_back(kdWall);
        }

        return KDBData::Error::OK;
    }
    else
    {
        int splitOffset;
        std::list<KDBData::Wall> positiveSide, negativeSide, withinPlane;
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
            for (const KDBData::Wall &wall : withinPlane)
            {
                KDMapData::Wall kdWall;

                kdWall.m_From.m_X = wall.m_VertexFrom.m_X;
                kdWall.m_From.m_Y = wall.m_VertexFrom.m_Y;

                kdWall.m_To.m_X = wall.m_VertexTo.m_X;
                kdWall.m_To.m_Y = wall.m_VertexTo.m_Y;

                kdWall.m_InSector = wall.m_InSector;
                kdWall.m_OutSector = wall.m_OutSector;

                kdWall.m_TexId = wall.m_TexId;
                kdWall.m_TexUOffset = wall.m_TexUOffset;
                kdWall.m_TexVOffset = wall.m_TexVOffset;

                ioKDTreeNode->m_Walls.push_back(kdWall);
            }
        }

        if (!positiveSide.empty())
        {
            ioKDTreeNode->m_PositiveSide = new KDTreeNode;
            KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
            KDBData::Error ret = RecursiveBuildKDTree(positiveSide, newSplitPlane, ioKDTreeNode->m_PositiveSide);
            if (ret != KDBData::Error::OK)
                return ret;
        }

        if (!negativeSide.empty())
        {
            ioKDTreeNode->m_NegativeSide = new KDTreeNode;
            KDTreeNode::SplitPlane newSplitPlane = iSplitPlane == KDTreeNode::SplitPlane::XConst ? KDTreeNode::SplitPlane::YConst : KDTreeNode::SplitPlane::XConst;
            KDBData::Error ret = RecursiveBuildKDTree(negativeSide, newSplitPlane, ioKDTreeNode->m_NegativeSide);
            if (ret != KDBData::Error::OK)
                return ret;
        }

        return KDBData::Error::OK;
    }
}

void KDTreeBuilder::SplitWallSet(std::list<KDBData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::list<KDBData::Wall> &oPositiveSide, std::list<KDBData::Wall> &oNegativeSide, std::list<KDBData::Wall> &oWithinSplitPlane)
{
    int splitPlaneInt = iSplitPlane == KDTreeNode::SplitPlane::XConst ? 0 : (iSplitPlane == KDTreeNode::SplitPlane::YConst ? 1 : 2);

    std::vector<KDBData::Wall> WallsParallelToSplitPlane;
    for (const KDBData::Wall &wall : ioWalls)
    {
        if (wall.m_VertexFrom.GetCoord(splitPlaneInt) == wall.m_VertexTo.GetCoord(splitPlaneInt))
            WallsParallelToSplitPlane.push_back(wall);
    }

    if (WallsParallelToSplitPlane.empty())
        return;

    std::sort(WallsParallelToSplitPlane.begin(), WallsParallelToSplitPlane.end());
    oSplitOffset = WallsParallelToSplitPlane[(WallsParallelToSplitPlane.size() - 1) / 2].m_VertexFrom.GetCoord(splitPlaneInt);
    WallsParallelToSplitPlane.clear();

    std::list<KDBData::Wall>::iterator sit(ioWalls.begin());
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
            KDBData::Vertex splitVertex;
            splitVertex.SetCoord(splitPlaneInt, oSplitOffset);
            splitVertex.SetCoord((splitPlaneInt + 1) % 2, sit->m_VertexFrom.GetCoord((splitPlaneInt + 1) % 2));

            KDBData::Wall newWall1(*sit);
            KDBData::Wall newWall2(*sit);
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
    bool success = BuildSectors(m_Map) == KDBData::Error::OK;
    if(!success)
        return false;

    KDBData::Error err = BuildKDTree(m_pKDTree);
    if(err != KDBData::Error::OK || !m_pKDTree)
        return false;

    return true;
}

KDTreeMap* KDTreeBuilder::GetOutputTree()
{
    KDTreeMap *pRet = m_pKDTree;
    m_pKDTree = nullptr;
    return pRet;
}