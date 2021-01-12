#include "KDTreeBuilder.h"

#include "Map.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include <vector>
#include <list>
#include <iterator>
#include <memory>
#include <algorithm>

// Ugly forward declaration :(
class SectorInclusions;

class MapBuildData
{
public:
    enum class ErrorCode
    {
        OK,
        INVALID_POLYGON,
        SECTOR_INTERSECTION,
        CANNOT_BREAK_WALL,
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
            else if (!(iLeft.m_VertexTo == iRight.m_VertexTo))
                return iLeft.m_VertexTo < iRight.m_VertexTo;
            else if (iLeft.m_InSector != iRight.m_InSector)
                return iLeft.m_InSector < iRight.m_InSector;
            else
                return iLeft.m_OutSector < iRight.m_OutSector;
        }

        bool IsGeomEqual(const Wall &iOtherWall)
        {
            return this->m_VertexFrom == iOtherWall.m_VertexFrom && this->m_VertexTo == iOtherWall.m_VertexTo;
        }

        bool IsGeomOpposite(const Wall &iOtherWall) const
        {
            return this->m_VertexFrom == iOtherWall.m_VertexTo && this->m_VertexTo == iOtherWall.m_VertexFrom;
        }

        unsigned int GetConstCoordinate() const
        {
            if (m_VertexFrom.m_X == m_VertexTo.m_X)
                return 0;
            else if (m_VertexFrom.m_Y == m_VertexTo.m_Y)
                return 1;
            else
                return 2; // Invalid, should not happen
        }

        int m_InSector;
        int m_OutSector;
    };

    struct Sector
    {
        std::list<Wall> m_Walls;

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
            bool invalidLineIntersection = true;

            // Had to ensure that the intersection half-line that is used to find out whether thisVertex is in the polygon
            // doesn't cross a vertex of the polygon. 
            for(int deltaYIntersectionLine = 0; deltaYIntersectionLine < 30 && invalidLineIntersection; deltaYIntersectionLine++)
            {
                Relationship ret = Relationship::UNDETERMINED;
                nbOnOutline = 0;
                nbInside = 0;
                nbOutside = 0;
                invalidLineIntersection = false;

                // Count the number of intersections between an input half line (that has thisVertex as origin)
                // and the other sector
                // If even: thisVertex is outside the other sector
                // If odd: thisVerteix is inside the other sector
                for (const Wall &thisWall : m_Walls)
                {
                    const Vertex &thisVertex = thisWall.m_VertexFrom;
                    bool onOutline = false;

                    // TODO: sort segments (in order to perform a binary search instead of linear)
                    unsigned int nbIntersectionsAlongHalfLine = 0;
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
                        if (SegmentSegmentIntersection<MapBuildData::Vertex, double>(thisWall.m_VertexFrom, thisWall.m_VertexTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection) &&
                            !(intersection == thisWall.m_VertexFrom) && !(intersection == thisWall.m_VertexTo) &&
                            !(intersection == otherWall.m_VertexFrom) && !(intersection == otherWall.m_VertexTo))
                        {
                            ret = Relationship::PARTIAL;
                            break;
                        }

                        if (VertexOnXYAlignedSegment(otherWall.m_VertexFrom, otherWall.m_VertexTo, thisVertex))
                        {
                            nbOnOutline++;
                            onOutline = true;
                            break;
                        }
                        else
                        {
                            Vertex halfLineTo = thisVertex;
                            halfLineTo.m_X += 100;
                            halfLineTo.m_Y += (50 + deltaYIntersectionLine);

                            Vertex intersection;
                            if (HalfLineSegmentIntersection<MapBuildData::Vertex, double>(thisVertex, halfLineTo, otherWall.m_VertexFrom, otherWall.m_VertexTo, intersection))
                            {
                                if(intersection == thisWall.m_VertexFrom || intersection == thisWall.m_VertexTo ||
                                   intersection == otherWall.m_VertexFrom || intersection == otherWall.m_VertexTo)
                                {
                                    invalidLineIntersection = true;
                                    break;
                                }

                                nbIntersectionsAlongHalfLine++;
                            }
                        }
                    }
                    
                    if (!onOutline)
                    {
                        if (nbIntersectionsAlongHalfLine % 2 == 1)
                            nbInside++;
                        else
                            nbOutside++;
                    }

                    if (nbInside > 0 && nbOutside > 0)
                        ret = Relationship::PARTIAL;

                    if (ret == Relationship::PARTIAL)
                        break;
                }
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
    ErrorCode BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<Wall> &oWalls);
    ErrorCode RecursiveBuildKDTree(std::list<Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&oKDTree);
    void SplitWallSet(std::list<Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::list<Wall> &oPositiveSide, std::list<Wall> &oNegativeSide, std::list<Wall> &oWithinSplitPlane);
    bool IsWallSetConvex(const std::list<Wall> &iWalls);

protected:
    std::vector<Sector> m_Sectors;
    const Map::Data *m_pMapData;
};

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
    public : 
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

SectorInclusionOperator::SectorInclusionOperator(std::vector<MapBuildData::Sector> &ioSectors) :
    m_Sectors(ioSectors),
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
                    for(const MapBuildData::Wall &wall : m_Sectors[i].m_Walls)
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

WallBreakerOperator::WallBreakerOperator(const SectorInclusions &iSectorInclusions) :
    m_SectorInclusions(iSectorInclusions)
{
}

WallBreakerOperator::~WallBreakerOperator()
{
}

MapBuildData::ErrorCode WallBreakerOperator::Run(const std::vector<MapBuildData::Wall> &iWalls, std::vector<MapBuildData::Wall> &oWalls)
{
    MapBuildData::ErrorCode ret = MapBuildData::ErrorCode::OK;

    std::vector<MapBuildData::Wall> xConstWalls;
    std::vector<MapBuildData::Wall> yConstWalls;

    for (unsigned int i = 0; i < iWalls.size(); i++)
    {
        if(iWalls[i].GetConstCoordinate() == 0)
            xConstWalls.push_back(iWalls[i]);
        else if (iWalls[i].GetConstCoordinate() == 1)
            yConstWalls.push_back(iWalls[i]);
        else
            ret = MapBuildData::ErrorCode::CANNOT_BREAK_WALL;
    }

    if(ret != MapBuildData::ErrorCode::OK)
        return ret;

    // Build cluster of walls that have the same const coordinate
    std::sort(xConstWalls.begin(), xConstWalls.end());
    std::sort(yConstWalls.begin(), yConstWalls.end());

    std::vector<std::vector<MapBuildData::Wall> *> pCurrentWalls;
    pCurrentWalls.push_back(&xConstWalls);
    pCurrentWalls.push_back(&yConstWalls);

    for (unsigned int w = 0; w < pCurrentWalls.size(); w++)
    {
        std::vector<MapBuildData::Wall> &walls = *pCurrentWalls[w];
        if (!walls.empty())
        {
            int constCoor = walls[0].GetConstCoordinate();
            int constVal = walls[0].m_VertexFrom.GetCoord(constCoor);
            unsigned int i = 0;
            while (i < walls.size())
            {
                std::vector<MapBuildData::Wall> wallCluster;
                std::vector<MapBuildData::Wall> brokenWallsCluster;
                while (i < walls.size() && walls[i].m_VertexFrom.GetCoord(constCoor) == constVal)
                {
                    wallCluster.push_back(walls[i]);
                    i++;
                }

                MapBuildData::ErrorCode localErr;
                if ((localErr = RunOnCluster(wallCluster, brokenWallsCluster)) != MapBuildData::ErrorCode::OK)
                    ret = localErr;
                else // Everything went fine
                    oWalls.insert(oWalls.end(), brokenWallsCluster.begin(), brokenWallsCluster.end());

                if(i < walls.size())
                    constVal = walls[i].m_VertexFrom.GetCoord(constCoor);
            }
        }
    }

    return ret;
}

MapBuildData::ErrorCode WallBreakerOperator::RunOnCluster(const std::vector<MapBuildData::Wall> &iWalls, std::vector<MapBuildData::Wall> &oWalls)
{
    MapBuildData::ErrorCode ret = MapBuildData::ErrorCode::OK;

    if (iWalls.empty())
        return ret;

    unsigned int constCoord, varCoord;
    constCoord = iWalls[0].GetConstCoordinate();
    varCoord = (constCoord + 1) % 2;
    if (constCoord == 2) // Invalid result
        return MapBuildData::ErrorCode::CANNOT_BREAK_WALL;

    std::vector<MapBuildData::Vertex> sortedVertices;
    sortedVertices.push_back(iWalls[0].m_VertexFrom);
    sortedVertices.push_back(iWalls[0].m_VertexTo);

    for (unsigned int i = 1; i < iWalls.size(); i++)
    {
        if (iWalls[i].m_VertexFrom.GetCoord(constCoord) != iWalls[i].m_VertexTo.GetCoord(constCoord) ||
            iWalls[i].m_VertexFrom.GetCoord(constCoord) != iWalls[0].m_VertexFrom.GetCoord(constCoord))
            ret = MapBuildData::ErrorCode::CANNOT_BREAK_WALL;

        sortedVertices.push_back(iWalls[i].m_VertexFrom);
        sortedVertices.push_back(iWalls[i].m_VertexTo);
    }

    if (ret != MapBuildData::ErrorCode::OK)
        return ret;

    std::sort(sortedVertices.begin(), sortedVertices.end());
    auto last = std::unique(sortedVertices.begin(), sortedVertices.end());
    sortedVertices.erase(last, sortedVertices.end());

    std::vector<MapBuildData::Wall> brokenWalls;

    for (unsigned int i = 0; i < iWalls.size(); i++)
    {
        MapBuildData::Wall currentWall = iWalls[i];
        for (unsigned int j = 1; j < sortedVertices.size(); j++)
        {
            MapBuildData::Wall minWall, maxWall;
            if(SplitWall(currentWall, sortedVertices[j], minWall, maxWall))
            {
                brokenWalls.push_back(minWall);
                currentWall = maxWall;
            }
        }
        brokenWalls.push_back(currentWall);
    }

    // Find all subsets of walls that are geometrically equal. Each subset is replaced
    // with a unique wall whose inner sector is the innermost sector of the whole set (same
    // goes for the outer sector)
    std::vector<MapBuildData::Wall> uniqueWalls;
    for (unsigned int i = 0; i < brokenWalls.size(); i++)
    {
        int found = -1;
        for (unsigned int j = 0; j < uniqueWalls.size(); j++)
        {
            if (brokenWalls[i].IsGeomEqual(uniqueWalls[j]))
            {
                found = j;
                break;
            }
        }

        if(found >= 0)
        {
            MapBuildData::Wall newWall = brokenWalls[i];
            newWall.m_InSector = m_SectorInclusions.GetInnermostSector(brokenWalls[i].m_InSector, uniqueWalls[found].m_InSector);
            newWall.m_OutSector = m_SectorInclusions.GetOutermostSector(brokenWalls[i].m_OutSector, uniqueWalls[found].m_OutSector);
            uniqueWalls[found] = newWall;
        }
        else
            uniqueWalls.push_back(brokenWalls[i]);
    }

    // Find all pair of walls that are geometrically opposed. Replace each pair with a unique
    // wall with corresponding inner and outer sectors
    for (unsigned int i = 0; i < uniqueWalls.size(); i++)
    {
        int found = -1;
        for (unsigned int j = 0; j < oWalls.size(); j++)
        {
            if (uniqueWalls[i].IsGeomOpposite(oWalls[j]))
            {
                found = j;
                break;
            }
        }

        if (found >= 0)
            oWalls[found].m_OutSector = uniqueWalls[i].m_InSector;
        else
            oWalls.push_back(uniqueWalls[i]);
    }

    return ret;
}

bool WallBreakerOperator::SplitWall(MapBuildData::Wall iWall, const MapBuildData::Vertex &iVertex,
                                    MapBuildData::Wall &oMinWall, MapBuildData::Wall &oMaxWall) const
{
    bool hasBeenSplit = false;

    unsigned int constCoord, varCoord;
    constCoord = iWall.GetConstCoordinate();
    varCoord = (constCoord + 1) % 2;
    if (constCoord == 2) // Invalid result, should never happen
        return false;

    // Should never happen as well
    if(iVertex.GetCoord(constCoord) != iWall.m_VertexFrom.GetCoord(constCoord) ||
       iVertex.GetCoord(constCoord) != iWall.m_VertexTo.GetCoord(constCoord))
        return false;

    bool reverse = iWall.m_VertexFrom.GetCoord(varCoord) > iWall.m_VertexTo.GetCoord(varCoord);
    if(reverse)
        std::swap(iWall.m_VertexFrom, iWall.m_VertexTo);

    // Vertex is strictly within input wall, break it
    if(iWall.m_VertexFrom.GetCoord(varCoord) < iVertex.GetCoord(varCoord) && 
    iVertex.GetCoord(varCoord) < iWall.m_VertexTo.GetCoord(varCoord))
    {
        oMinWall = MapBuildData::Wall(iWall);
        oMaxWall = MapBuildData::Wall(iWall);

        oMinWall.m_VertexTo.SetCoord(varCoord, iVertex.GetCoord(varCoord));
        oMaxWall.m_VertexFrom.SetCoord(varCoord, iVertex.GetCoord(varCoord));

        if(reverse)
        {
            std::swap(oMinWall.m_VertexFrom, oMinWall.m_VertexTo);
            std::swap(oMaxWall.m_VertexFrom, oMaxWall.m_VertexTo);
        }

        hasBeenSplit = true;
    }

    return hasBeenSplit;
}

MapBuildData::MapBuildData() : m_pMapData(nullptr)
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

MapBuildData::ErrorCode MapBuildData::BuildPolygon(const Map::Data::Sector::Polygon &iPolygon, int iDesiredOrientation, std::list<MapBuildData::Wall> &oWalls)
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

    // Decimate vertices
    // This is useful in order to detect "hard" sector intersections and raise an
    // error when they occur
    // TODO: When textures are supported, this step will have to be performed inside
    // the inclusion solver only (we might accidentally remove information otherwise)

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

    if (ret == MapBuildData::ErrorCode::OK)
    {
        if (ret == MapBuildData::ErrorCode::OK)
        {
            // Walls are broken so no walls overlap
            // Wall breaking & in/out sector resolution is handled by WallBreakerOperator
            std::vector<Wall> allWallsToBreak;
            for (unsigned int i = 0; i < m_Sectors.size(); i++)
            {
                for (const Wall &wall : m_Sectors[i].m_Walls)
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

                // Damn I really suck at writing consistent code :(
                std::list<MapBuildData::Wall> allWallsList(allWalls.begin(), allWalls.end());
                allWalls.clear();
                ret = RecursiveBuildKDTree(allWallsList, KDTreeNode::SplitPlane::XConst, oKDTree->m_RootNode);
            }
        }
    }

    return ret;
}

bool MapBuildData::IsWallSetConvex(const std::list<Wall> &iWalls)
{
    bool isConvex = true;

    if(iWalls.empty())
        return true;

    // Only walls that separate the inside and the outside of the map
    // can be considered a convex set
    int inSector = iWalls.begin()->m_InSector;
    for (const Wall &wall : iWalls)
    {
        if(wall.m_OutSector != -1)
            isConvex = false;
        if(wall.m_InSector != inSector)
            isConvex = false;
    }

    // Compute the axis-aligned bounding box
    Vertex bboxMin = iWalls.begin()->m_VertexFrom;
    Vertex bboxMax = bboxMin;
    for(const Wall &wall : iWalls)
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
    for(const Wall &wall : iWalls)
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

MapBuildData::ErrorCode MapBuildData::RecursiveBuildKDTree(std::list<MapBuildData::Wall> &iWalls, KDTreeNode::SplitPlane iSplitPlane, KDTreeNode *&ioKDTreeNode)
{
    if(!ioKDTreeNode)
        return ErrorCode::UNKNOWN_FAILURE;

    // No need to divide the set of wall any further
    if(IsWallSetConvex(iWalls))
    {
        ioKDTreeNode->m_SplitPlane = KDTreeNode::SplitPlane::None;
        ioKDTreeNode->m_SplitOffset = 0; // Will never be used

        for (const Wall &wall : iWalls)
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

        return ErrorCode::OK;
    }
    else
    {
        int splitOffset;
        std::list<Wall> positiveSide, negativeSide, withinPlane;
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
            for (const Wall &wall : withinPlane)
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

        return ErrorCode::OK;
    }
}

void MapBuildData::SplitWallSet(std::list<MapBuildData::Wall> &ioWalls, KDTreeNode::SplitPlane iSplitPlane, int &oSplitOffset, std::list<MapBuildData::Wall> &oPositiveSide, std::list<MapBuildData::Wall> &oNegativeSide, std::list<MapBuildData::Wall> &oWithinSplitPlane)
{
    int splitPlaneInt = iSplitPlane == KDTreeNode::SplitPlane::XConst ? 0 : (iSplitPlane == KDTreeNode::SplitPlane::YConst ? 1 : 2);

    std::vector<Wall> WallsParallelToSplitPlane;
    for(const Wall &wall : ioWalls)
    {
        if (wall.m_VertexFrom.GetCoord(splitPlaneInt) == wall.m_VertexTo.GetCoord(splitPlaneInt))
            WallsParallelToSplitPlane.push_back(wall);
    }

    if (WallsParallelToSplitPlane.empty())
        return;

    std::sort(WallsParallelToSplitPlane.begin(), WallsParallelToSplitPlane.end());
    oSplitOffset = WallsParallelToSplitPlane[(WallsParallelToSplitPlane.size() - 1) / 2].m_VertexFrom.GetCoord(splitPlaneInt);
    WallsParallelToSplitPlane.clear();

    std::list<Wall>::iterator sit(ioWalls.begin());
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
            Vertex splitVertex;
            splitVertex.SetCoord(splitPlaneInt, oSplitOffset);
            splitVertex.SetCoord((splitPlaneInt + 1) % 2, sit->m_VertexFrom.GetCoord((splitPlaneInt + 1) % 2));

            Wall newWall1(*sit);
            Wall newWall2(*sit);
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