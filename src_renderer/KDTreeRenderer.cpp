#include "KDTreeRenderer.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include "WallRenderer.h"

#include <cstring>

KDTreeRenderer::KDTreeRenderer(const KDTreeMap &iMap) :
    m_Map(iMap),
    m_pFrameBuffer(new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4u])
{
    m_Settings.m_PlayerHorizontalFOV = 90 * (1 << ANGLE_SHIFT);
    m_Settings.m_PlayerVerticalFOV = (m_Settings.m_PlayerHorizontalFOV * WINDOW_HEIGHT) / WINDOW_WIDTH;
    m_Settings.m_PlayerHeight = CType(30) / POSITION_SCALE;
    m_Settings.m_MaxColorInterpolationDist = CType(500) / POSITION_SCALE;
    m_Settings.m_HorizontalDistortionCst = 1 / (2 * tanInt(m_Settings.m_PlayerHorizontalFOV / 2));
    m_Settings.m_VerticalDistortionCst = 1 / (2 * tanInt(m_Settings.m_PlayerVerticalFOV / 2));

    m_FlatSurfaces.reserve(20);
    memset(m_pFrameBuffer, 255u, sizeof(unsigned char) * 4u * WINDOW_HEIGHT * WINDOW_WIDTH);
    ClearBuffers();
}

const unsigned char* KDTreeRenderer::GetFrameBuffer() const
{
    return m_pFrameBuffer;
}

unsigned char *KDTreeRenderer::GetFrameBuffer()
{
    return m_pFrameBuffer;
}

void KDTreeRenderer::FillFrameBufferWithColor(unsigned char r, unsigned char g, unsigned char b)
{
    // Loop because memset can't take anything bigger than a char as an input
    for (unsigned int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT * 4u; i+= 4u)
    {
        m_pFrameBuffer[i + 0] = r;
        m_pFrameBuffer[i + 1] = g;
        m_pFrameBuffer[i + 2] = b;
    }
}

void KDTreeRenderer::ClearBuffers()
{
    memset(m_pHorizOcclusionBuffer, 0u, sizeof(unsigned char) * WINDOW_WIDTH);
    memset(m_pTopOcclusionBuffer, 0, sizeof(int) * WINDOW_WIDTH);
    memset(m_pBottomOcclusionBuffer, 0, sizeof(int) * WINDOW_WIDTH);
    m_FlatSurfaces.clear();
}

void KDTreeRenderer::RefreshFrameBuffer()
{
    // Useful when debugging, useless and costly otherwise
    // FillFrameBufferWithColor(0u, 0u, 0u);
    Render();
}

void KDTreeRenderer::Render()
{
    m_State.m_PlayerZ = ComputeZ();

    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection - m_Settings.m_PlayerHorizontalFOV / 2, m_State.m_FrustumToLeft);
    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection + m_Settings.m_PlayerHorizontalFOV / 2, m_State.m_FrustumToRight);
    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection, m_State.m_Look);

    RenderNode(m_Map.m_RootNode);
    RenderFlatSurfaces();
}

void KDTreeRenderer::RenderNode(KDTreeNode *pNode)
{
    // TODO: culling

    bool positiveSide;
    if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_State.m_PlayerPosition.m_X > (CType(pNode->m_SplitOffset) / POSITION_SCALE);
    else if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::YConst)
        positiveSide = m_State.m_PlayerPosition.m_Y > (CType(pNode->m_SplitOffset) / POSITION_SCALE);

    if (positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
    else if (!positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);

    for (unsigned int i = 0; i < pNode->m_Walls.size(); i++)
    {
        KDRData::Wall wall(KDRData::GetWallFromNode(pNode, i));

        bool vertexFromIsBehindPlayer = DotProduct(m_State.m_PlayerPosition, m_State.m_Look, m_State.m_PlayerPosition, wall.m_VertexFrom) <= 0;
        bool vertexToIsBehindPlayer = DotProduct(m_State.m_PlayerPosition, m_State.m_Look, m_State.m_PlayerPosition, wall.m_VertexTo) <= 0;

        if ((WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToLeft, wall.m_VertexFrom) <= 0 &&
             WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToLeft, wall.m_VertexTo) <= 0) ||
            (WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToRight, wall.m_VertexFrom) >= 0 &&
             WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToRight, wall.m_VertexTo) >= 0) ||
            (vertexFromIsBehindPlayer && vertexToIsBehindPlayer))
        {
            // Culling (wall is entirely outside frustum)
            continue;
        }
        else
        {
            std::vector<KDRData::FlatSurface> generatedFlats;
            WallRenderer wallRenderer(wall, m_State, m_Settings, m_Map);
            wallRenderer.SetBuffers(m_pFrameBuffer, m_pHorizOcclusionBuffer, m_pTopOcclusionBuffer, m_pBottomOcclusionBuffer);
            wallRenderer.Render(generatedFlats);

            for(const KDRData::FlatSurface &flat : generatedFlats)
                AddFlatSurface(flat);
        }
    }

    if (positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);
    else if (!positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
}

bool KDTreeRenderer::AddFlatSurface(const KDRData::FlatSurface &iFlatSurface)
{
    bool hasBeenAbsorbed = false;
    for(auto &k : m_FlatSurfaces)
    {
        for (KDRData::FlatSurface &flatSurface : k.second)
        {
            if (flatSurface.Absorb(iFlatSurface))
                hasBeenAbsorbed = true;
        }
    }

    if(!hasBeenAbsorbed)
        m_FlatSurfaces[iFlatSurface.m_Height].push_back(iFlatSurface);

    return true;
}

void KDTreeRenderer::RenderFlatSurfacesLegacy()
{
    int vFovRad = ARITHMETIC_SHIFT(m_Settings.m_PlayerVerticalFOV, ANGLE_SHIFT) * (314 << FP_SHIFT) / (180 * 100);

    for(const auto &keyVal : m_FlatSurfaces)
    {
        const std::vector<KDRData::FlatSurface> &currentSurfaces = keyVal.second;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            const KDRData::FlatSurface &currentSurface = currentSurfaces[i];

            char r = currentSurface.m_SectorIdx % 3 == 0 ? 1 : 0;
            char g = currentSurface.m_SectorIdx % 3 == 1 ? 1 : 0;
            char b = currentSurface.m_SectorIdx % 3 == 2 ? 1 : 0;
            int maxColorRange = 150;

            int minY = WINDOW_HEIGHT;
            int maxY = 0;
            for (int x = currentSurface.m_MinX; x <= currentSurface.m_MaxX; x++)
            {
                minY = std::min(currentSurface.m_MinY[x], minY);
                maxY = std::max(currentSurface.m_MaxY[x], maxY);
            }

            for (int y = minY; y < maxY; y++)
            {
                // double vFovRadD = ARITHMETIC_SHIFT(m_PlayerVerticalFOV, ANGLE_SHIFT) * M_PI / 180.0;
                // double thetaD = (vFovRadD * (2.0 * y - WINDOW_HEIGHT)) / (2.0 * WINDOW_HEIGHT);
                // double distD = std::abs(static_cast<double>(m_PlayerZ - m_FlatSurfaces[i].m_Height) / sin(thetaD));
                // int colorD = ((m_MaxColorInterpolationDist - distD) * maxColorRange) / m_MaxColorInterpolationDist;

                int theta = (m_Settings.m_PlayerVerticalFOV * (2 * y - WINDOW_HEIGHT)) / (2 * WINDOW_HEIGHT);
                CType sinTheta = sinInt(theta);

                if (sinTheta != 0)
                {
                    CType dist = (((m_State.m_PlayerZ - CType(currentSurface.m_Height))) / sinTheta);
                    dist = dist < 0 ? -dist : dist;
                    dist = std::min(dist, m_Settings.m_MaxColorInterpolationDist);
                    int color = ((m_Settings.m_MaxColorInterpolationDist - dist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
                    color = std::max(45, color);

                    for (int x = currentSurface.m_MinX; x <= currentSurface.m_MaxX; x++)
                    {
                        if (y <= currentSurface.m_MaxY[x] && y >= currentSurface.m_MinY[x])
                        {
                            WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + x, color * r, color * g, color * b);
                        }
                    }
                }
            }
        }
    }
}

void KDTreeRenderer::RenderFlatSurfaces()
{
    // {
    //     unsigned int totalSize = 0;
    //     for (const auto &k : m_FlatSurfaces)
    //         totalSize += k.second.size();
    //     std::cout << "Number of flat surfaces = " << totalSize << std::endl;
    // }

    for(unsigned int i = 0; i < WINDOW_HEIGHT; i++)
        m_LinesXStart[i] = -1;

    for(const auto &keyVal : m_FlatSurfaces)
    {
        const CType currentHeight = keyVal.first;
        const std::vector<KDRData::FlatSurface> &currentSurfaces = keyVal.second;

        for(unsigned int i = 0; i < WINDOW_HEIGHT; i++)
            m_DistYCache[i] = -1;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            const KDRData::FlatSurface &currentSurface = currentSurfaces[i];

            char r = currentSurface.m_SectorIdx % 3 == 0 ? 1 : 0;
            char g = currentSurface.m_SectorIdx % 3 == 1 ? 1 : 0;
            char b = currentSurface.m_SectorIdx % 3 == 2 ? 1 : 0;
            int maxColorRange = 150;

            auto DrawLine = [&](int iY, int iMinX, int iMaxX) {
                CType dist = -1;
                int color;

                if (m_DistYCache[iY] >= 0)
                    dist = m_DistYCache[iY];
                else
                {
                    int theta = (m_Settings.m_PlayerVerticalFOV * (2 * iY - WINDOW_HEIGHT)) / (2 * WINDOW_HEIGHT);
                    CType sinTheta = sinInt(theta);

                    if (sinTheta != 0)
                    {
                        dist = (m_State.m_PlayerZ - currentSurface.m_Height) / sinTheta;
                        dist = dist < 0 ? -dist : dist;
                        dist = std::min(dist, m_Settings.m_MaxColorInterpolationDist);
                        m_DistYCache[iY] = dist;
                    }
                }

                if(dist >= 0)
                {
                    color = ((m_Settings.m_MaxColorInterpolationDist - dist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
                    color = std::max(45, color);

                    for (int x = iMinX; x <= iMaxX; x++)
                    {
                        WriteFrameBuffer((WINDOW_HEIGHT - 1 - iY) * WINDOW_WIDTH + x, color * r, color * g, color * b);
                    }
                }
            };

            // Jump to start of the drawable part of the surface
            int minXDrawable = currentSurface.m_MinX;
            for(; currentSurface.m_MinY[minXDrawable] > currentSurface.m_MaxY[minXDrawable]; minXDrawable++);
            int maxXDrawable = currentSurface.m_MaxX;
            for (; currentSurface.m_MinY[maxXDrawable] > currentSurface.m_MaxY[maxXDrawable]; maxXDrawable--);

            for (int y = currentSurface.m_MinY[minXDrawable]; y <= currentSurface.m_MaxY[minXDrawable]; y++)
            {
                m_LinesXStart[y] = minXDrawable;
            }

            for (int x = minXDrawable + 1; x <= maxXDrawable; x++)
            {
                // End of contiguous block
                if (currentSurface.m_MinY[x] > currentSurface.m_MaxY[x] ||
                    currentSurface.m_MinY[x] > currentSurface.m_MaxY[x - 1] ||
                    currentSurface.m_MaxY[x] < currentSurface.m_MinY[x - 1])
                {
                    for (int y = currentSurface.m_MinY[x - 1]; y <= currentSurface.m_MaxY[x - 1]; y++)
                    {
                        DrawLine(y, m_LinesXStart[y], x - 1);
                    }

                    // Jump to next drawable part of the surface
                    for (; x + 1 <= maxXDrawable && currentSurface.m_MinY[x + 1] > currentSurface.m_MaxY[x + 1]; x++);

                    for (int y = currentSurface.m_MinY[x]; y <= currentSurface.m_MaxY[x]; y++)
                        m_LinesXStart[y] = x;
                }
                else
                {
                    int deltaBottom = currentSurface.m_MinY[x] - currentSurface.m_MinY[x - 1];
                    if (deltaBottom < 0)
                    {
                        for (int y = currentSurface.m_MinY[x]; y < currentSurface.m_MinY[x - 1]; y++)
                        {
                            m_LinesXStart[y] = x;
                        }
                    }
                    else if (deltaBottom > 0)
                    {
                        for (int y = currentSurface.m_MinY[x - 1]; y < currentSurface.m_MinY[x]; y++)
                        {
                            DrawLine(y, m_LinesXStart[y], x - 1);
                        }
                    }

                    int deltaTop = currentSurface.m_MaxY[x] - currentSurface.m_MaxY[x - 1];
                    if (deltaTop > 0)
                    {
                        for (int y = currentSurface.m_MaxY[x - 1] + 1; y <= currentSurface.m_MaxY[x]; y++)
                        {
                            m_LinesXStart[y] = x;
                        }
                    }
                    else if (deltaTop < 0)
                    {
                        for (int y = currentSurface.m_MaxY[x]; y < currentSurface.m_MaxY[x - 1]; y++)
                        {
                            DrawLine(y, m_LinesXStart[y], x - 1);
                        }
                    }
                }
            }

            for (int y = currentSurface.m_MinY[maxXDrawable]; y <= currentSurface.m_MaxY[maxXDrawable]; y++)
            {
                DrawLine(y, m_LinesXStart[y], maxXDrawable);
            }
        }
    }
}

CType KDTreeRenderer::ComputeZ()
{
    return RecursiveComputeZ(m_Map.m_RootNode);
}

CType KDTreeRenderer::RecursiveComputeZ(KDTreeNode *pNode)
{
    static unsigned pouet = 0;

    if(!pNode)
        return 0; // Should not happen

    CType oZ = 0;

    bool positiveSide;
    if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_State.m_PlayerPosition.m_X > (CType(pNode->m_SplitOffset) / POSITION_SCALE);
    else if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::YConst)
        positiveSide = m_State.m_PlayerPosition.m_Y > (CType(pNode->m_SplitOffset) / POSITION_SCALE);

    // We are on a terminal node
    // The node contains exactly one wall (or multiple walls oriented similarly and refering
    // to the same sectors), which we are going to use to find out in which sector the player is
    if ((positiveSide && !pNode->m_PositiveSide) ||
        (!positiveSide && !pNode->m_NegativeSide))
    {
        if(pNode->GetNbOfWalls()) // Should always be true
        {
            oZ = m_Settings.m_PlayerHeight;
            KDRData::Wall wall(KDRData::GetWallFromNode(pNode, 0));
            int whichSide = WhichSide(wall.m_VertexFrom, wall.m_VertexTo, m_State.m_PlayerPosition);
            if(whichSide < 0)
            {
                int outSectorIdx = wall.m_pKDWall->m_OutSector;
                if (outSectorIdx >= 0)
                {
                    KDRData::Sector outSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[outSectorIdx]);
                    oZ += outSector.m_Floor;
                    oZ = Clamp(oZ, outSector.m_Floor, outSector.m_Ceiling);
                }
            }
            else
            {
                int inSectorIdx = wall.m_pKDWall->m_InSector;
                if (inSectorIdx >= 0)
                {
                    KDRData::Sector inSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[inSectorIdx]);
                    oZ += inSector.m_Floor;
                    oZ = Clamp(oZ, inSector.m_Floor, inSector.m_Ceiling);
                }
            }
        }
    }
    // Recursive run
    else
    {
        if(positiveSide)
            return RecursiveComputeZ(pNode->m_PositiveSide);
        else
            return RecursiveComputeZ(pNode->m_NegativeSide);
    }

    return oZ;
}

void KDTreeRenderer::SetPlayerCoordinates(const KDRData::Vertex &iPosition, int iDirection)
{
    m_State.m_PlayerPosition = iPosition;
    m_State.m_PlayerDirection = iDirection;

    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection - m_Settings.m_PlayerHorizontalFOV / 2, m_State.m_FrustumToLeft);
    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection + m_Settings.m_PlayerHorizontalFOV / 2, m_State.m_FrustumToRight);
    GetVector(m_State.m_PlayerPosition, m_State.m_PlayerDirection, m_State.m_Look);
}

KDRData::Vertex KDTreeRenderer::GetPlayerPosition() const
{
    return m_State.m_PlayerPosition;
}

int KDTreeRenderer::GetPlayerDirection() const
{
    return m_State.m_PlayerDirection;
}

KDRData::Vertex KDTreeRenderer::GetLook() const
{
    return m_State.m_Look;
}