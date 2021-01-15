#include "WallBreakerOperator.h"

#include "SectorInclusionOperator.h"

#include <algorithm>

WallBreakerOperator::WallBreakerOperator(const SectorInclusions &iSectorInclusions, std::vector<KDBData::Sector> &iSectors) :
    m_SectorInclusions(iSectorInclusions),
    m_Sectors(iSectors)
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

    if (iWalls.size() <= 1)
    {
        oWalls = iWalls;
        return ret;
    }

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

    // Merge walls
    // Texture merging rules are really shady, I should have definitely thought about it
    // before making the choice of considering each sector as a totally independant entity

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

        // Need to merge brokenWalls[i] with uniqueWalls[found]
        if (found >= 0)
        {
            int innerMostSector = m_SectorInclusions.GetInnermostSector(brokenWalls[i].m_InSector, uniqueWalls[found].m_InSector);
            int outerMostSector = m_SectorInclusions.GetOutermostSector(brokenWalls[i].m_OutSector, uniqueWalls[found].m_OutSector);

            // If only one wall has a texture Id, newWall inherits from it
            // If both walls have a texture Id, the one with the innermost InSector wins
            if ((brokenWalls[i].m_TexId != -1 && uniqueWalls [found].m_TexId == -1) ||
                (brokenWalls[i].m_TexId != -1 && brokenWalls[i].m_InSector == innerMostSector))
            {
                uniqueWalls[found].m_TexId = brokenWalls[i].m_TexId;
                uniqueWalls[found].m_TexUOffset = brokenWalls[i].m_TexUOffset;
                uniqueWalls[found].m_TexVOffset = brokenWalls[i].m_TexVOffset;
            }

            uniqueWalls[found].m_InSector = innerMostSector;
            uniqueWalls[found].m_OutSector = outerMostSector;
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
        {
            // If no texture yet, oWalls[found] inherits from uniqueWalls[i]'s TexId
            if(oWalls[found].m_TexId == -1)
                oWalls[found].m_TexId = uniqueWalls[i].m_TexId;
            // Else, the shortest support wall in inSectors assigns its texture id
            // Shady rule, sucks quite a bit
            // Hopefully it works out in practice and doesn't make level design a living hell :/
            else if(uniqueWalls[i].m_TexId != -1)
            {
                KDBData::Wall oldSupportWall, newSupportWall;
                bool foundOldSupport = FindSupportWallInSector(oWalls[found], oWalls[found].m_InSector, oldSupportWall);
                bool foundNewSupport = FindSupportWallInSector(uniqueWalls[i], uniqueWalls[i].m_InSector, newSupportWall);

                if(!foundOldSupport || !foundNewSupport)
                {
                    ret = KDBData::Error::CANNOT_BREAK_WALL;
                    break;
                }

                int constCoordOld = oldSupportWall.GetConstCoordinate();
                int constCoordNew = newSupportWall.GetConstCoordinate();

                if(constCoordOld != constCoordNew)
                {
                    ret = KDBData::Error::CANNOT_BREAK_WALL;
                    break;
                }

                int varCoord = (1 + constCoordNew) % 2;
                int oldSupportWallLength = std::abs(oldSupportWall.m_VertexTo.GetCoord(varCoord) - oldSupportWall.m_VertexFrom.GetCoord(varCoord));
                int newSupportWallLength = std::abs(newSupportWall.m_VertexTo.GetCoord(varCoord) - newSupportWall.m_VertexFrom.GetCoord(varCoord));

                if(newSupportWallLength < oldSupportWallLength)
                {
                    oWalls[found].m_TexId = uniqueWalls[i].m_TexId;
                    oWalls[found].m_TexUOffset = uniqueWalls[i].m_TexUOffset;
                    oWalls[found].m_TexVOffset = uniqueWalls[i].m_TexVOffset;
                }
            }

            oWalls[found].m_OutSector = uniqueWalls[i].m_InSector;
        }
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

bool WallBreakerOperator::FindSupportWallInSector(const KDBData::Wall &iWall, int iSector, KDBData::Wall &oSupportWall) const
{
    if(iSector == -1)
        return false;

    bool found = false;

    int constCoord = iWall.GetConstCoordinate();
    int varCoord = (constCoord + 1) % 2;

    int minInputVal = std::min(iWall.m_VertexFrom.GetCoord(varCoord), iWall.m_VertexTo.GetCoord(varCoord));
    int maxInputVal = std::max(iWall.m_VertexFrom.GetCoord(varCoord), iWall.m_VertexTo.GetCoord(varCoord));

    for (const KDBData::Wall &supportWall : m_Sectors[iSector].m_Walls)
    {
        if (supportWall.GetConstCoordinate() == constCoord)
        {
            int minSupportVal = std::min(supportWall.m_VertexFrom.GetCoord(varCoord), supportWall.m_VertexTo.GetCoord(varCoord));
            int maxSupportVal = std::max(supportWall.m_VertexFrom.GetCoord(varCoord), supportWall.m_VertexTo.GetCoord(varCoord));

            if(minSupportVal <= minInputVal && maxInputVal <= maxSupportVal)
            {
                found = true;
                oSupportWall = supportWall;
                break;
            }
        }
    }

    return found;
}
