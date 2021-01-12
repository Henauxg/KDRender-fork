#include "WallBreakerOperator.h"

#include "SectorInclusionOperator.h"

#include <algorithm>

WallBreakerOperator::WallBreakerOperator(const SectorInclusions &iSectorInclusions) : m_SectorInclusions(iSectorInclusions)
{
}

WallBreakerOperator::~WallBreakerOperator()
{
}

KDBData::Error WallBreakerOperator::Run(const std::vector<KDBData::Wall> &iWalls, std::vector<KDBData::Wall> &oWalls)
{
    KDBData::Error ret = KDBData::Error::OK;

    std::vector<KDBData::Wall> xConstWalls;
    std::vector<KDBData::Wall> yConstWalls;

    for (unsigned int i = 0; i < iWalls.size(); i++)
    {
        if (iWalls[i].GetConstCoordinate() == 0)
            xConstWalls.push_back(iWalls[i]);
        else if (iWalls[i].GetConstCoordinate() == 1)
            yConstWalls.push_back(iWalls[i]);
        else
            ret = KDBData::Error::CANNOT_BREAK_WALL;
    }

    if (ret != KDBData::Error::OK)
        return ret;

    // Build cluster of walls that have the same const coordinate
    std::sort(xConstWalls.begin(), xConstWalls.end());
    std::sort(yConstWalls.begin(), yConstWalls.end());

    std::vector<std::vector<KDBData::Wall> *> pCurrentWalls;
    pCurrentWalls.push_back(&xConstWalls);
    pCurrentWalls.push_back(&yConstWalls);

    for (unsigned int w = 0; w < pCurrentWalls.size(); w++)
    {
        std::vector<KDBData::Wall> &walls = *pCurrentWalls[w];
        if (!walls.empty())
        {
            int constCoor = walls[0].GetConstCoordinate();
            int constVal = walls[0].m_VertexFrom.GetCoord(constCoor);
            unsigned int i = 0;
            while (i < walls.size())
            {
                std::vector<KDBData::Wall> wallCluster;
                std::vector<KDBData::Wall> brokenWallsCluster;
                while (i < walls.size() && walls[i].m_VertexFrom.GetCoord(constCoor) == constVal)
                {
                    wallCluster.push_back(walls[i]);
                    i++;
                }

                KDBData::Error localErr;
                if ((localErr = RunOnCluster(wallCluster, brokenWallsCluster)) != KDBData::Error::OK)
                    ret = localErr;
                else // Everything went fine
                    oWalls.insert(oWalls.end(), brokenWallsCluster.begin(), brokenWallsCluster.end());

                if (i < walls.size())
                    constVal = walls[i].m_VertexFrom.GetCoord(constCoor);
            }
        }
    }

    return ret;
}

KDBData::Error WallBreakerOperator::RunOnCluster(const std::vector<KDBData::Wall> &iWalls, std::vector<KDBData::Wall> &oWalls)
{
    KDBData::Error ret = KDBData::Error::OK;

    if (iWalls.empty())
        return ret;

    unsigned int constCoord, varCoord;
    constCoord = iWalls[0].GetConstCoordinate();
    varCoord = (constCoord + 1) % 2;
    if (constCoord == 2) // Invalid result
        return KDBData::Error::CANNOT_BREAK_WALL;

    std::vector<KDBData::Vertex> sortedVertices;
    sortedVertices.push_back(iWalls[0].m_VertexFrom);
    sortedVertices.push_back(iWalls[0].m_VertexTo);

    for (unsigned int i = 1; i < iWalls.size(); i++)
    {
        if (iWalls[i].m_VertexFrom.GetCoord(constCoord) != iWalls[i].m_VertexTo.GetCoord(constCoord) ||
            iWalls[i].m_VertexFrom.GetCoord(constCoord) != iWalls[0].m_VertexFrom.GetCoord(constCoord))
            ret = KDBData::Error::CANNOT_BREAK_WALL;

        sortedVertices.push_back(iWalls[i].m_VertexFrom);
        sortedVertices.push_back(iWalls[i].m_VertexTo);
    }

    if (ret != KDBData::Error::OK)
        return ret;

    std::sort(sortedVertices.begin(), sortedVertices.end());
    auto last = std::unique(sortedVertices.begin(), sortedVertices.end());
    sortedVertices.erase(last, sortedVertices.end());

    std::vector<KDBData::Wall> brokenWalls;

    for (unsigned int i = 0; i < iWalls.size(); i++)
    {
        KDBData::Wall currentWall = iWalls[i];
        for (unsigned int j = 1; j < sortedVertices.size(); j++)
        {
            KDBData::Wall minWall, maxWall;
            if (SplitWall(currentWall, sortedVertices[j], minWall, maxWall))
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
    std::vector<KDBData::Wall> uniqueWalls;
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

        if (found >= 0)
        {
            KDBData::Wall newWall = brokenWalls[i];
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

bool WallBreakerOperator::SplitWall(KDBData::Wall iWall, const KDBData::Vertex &iVertex,
                                    KDBData::Wall &oMinWall, KDBData::Wall &oMaxWall) const
{
    bool hasBeenSplit = false;

    unsigned int constCoord, varCoord;
    constCoord = iWall.GetConstCoordinate();
    varCoord = (constCoord + 1) % 2;
    if (constCoord == 2) // Invalid result, should never happen
        return false;

    // Should never happen as well
    if (iVertex.GetCoord(constCoord) != iWall.m_VertexFrom.GetCoord(constCoord) ||
        iVertex.GetCoord(constCoord) != iWall.m_VertexTo.GetCoord(constCoord))
        return false;

    bool reverse = iWall.m_VertexFrom.GetCoord(varCoord) > iWall.m_VertexTo.GetCoord(varCoord);
    if (reverse)
        std::swap(iWall.m_VertexFrom, iWall.m_VertexTo);

    // Vertex is strictly within input wall, break it
    if (iWall.m_VertexFrom.GetCoord(varCoord) < iVertex.GetCoord(varCoord) &&
        iVertex.GetCoord(varCoord) < iWall.m_VertexTo.GetCoord(varCoord))
    {
        oMinWall = KDBData::Wall(iWall);
        oMaxWall = KDBData::Wall(iWall);

        oMinWall.m_VertexTo.SetCoord(varCoord, iVertex.GetCoord(varCoord));
        oMaxWall.m_VertexFrom.SetCoord(varCoord, iVertex.GetCoord(varCoord));

        if (reverse)
        {
            std::swap(oMinWall.m_VertexFrom, oMinWall.m_VertexTo);
            std::swap(oMaxWall.m_VertexFrom, oMaxWall.m_VertexTo);
        }

        hasBeenSplit = true;
    }

    return hasBeenSplit;
}
