#include "KDTreeRenderer.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include <cstring>

namespace
{
    KDTreeRenderer::Wall GetWallFromNode(KDTreeNode *ipNode, unsigned int iWallIdx)
    {
        KDTreeRenderer::Wall wall;

        if(ipNode)
        {
            const KDMapData::Wall &kdWall = ipNode->GetWall(iWallIdx);
            wall.m_pKDWall = &kdWall;

            if(ipNode->GetSplitPlane() == KDTreeNode::SplitPlane::XConst)
            {
                wall.m_VertexFrom.m_X = ipNode->GetSplitOffset();
                wall.m_VertexTo.m_X = wall.m_VertexFrom.m_X;

                wall.m_VertexFrom.m_Y = kdWall.m_From;
                wall.m_VertexTo.m_Y = kdWall.m_To;
            }
            else
            {
                wall.m_VertexFrom.m_Y = ipNode->GetSplitOffset();
                wall.m_VertexTo.m_Y = wall.m_VertexFrom.m_Y;

                wall.m_VertexFrom.m_X = kdWall.m_From;
                wall.m_VertexTo.m_X = kdWall.m_To;
            }
        }

        return wall;
    }
}

KDTreeRenderer::KDTreeRenderer(const KDTreeMap &iMap) :
    m_Map(iMap),
    m_pFrameBuffer(new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4u]),
    m_pHorizOcclusionBuffer(new unsigned char[WINDOW_WIDTH]),
    m_pTopOcclusionBuffer(new int[WINDOW_WIDTH]),
    m_pBottomOcclusionBuffer(new int[WINDOW_WIDTH]),
    m_PlayerHorizontalFOV(80 * (1 << ANGLE_SHIFT)),
    m_PlayerVerticalFOV((m_PlayerHorizontalFOV * WINDOW_HEIGHT) / WINDOW_WIDTH),
    m_PlayerHeight(300),
    m_MaxColorInterpolationDist(5000)
{
    m_FlatSurfaces.reserve(256);
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
    m_PlayerZ = ComputeZ();

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerHorizontalFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerHorizontalFOV / 2, m_FrustumToRight);
    GetVector(m_PlayerPosition, m_PlayerDirection, m_Look);

    RenderNode(m_Map.m_RootNode);
    RenderFlatSurfaces();
}

void KDTreeRenderer::RenderNode(KDTreeNode *pNode)
{
    // TODO: culling

    bool positiveSide;
    if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_PlayerPosition.m_X > pNode->m_SplitOffset;
    else
        positiveSide = m_PlayerPosition.m_Y > pNode->m_SplitOffset;

    if (positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
    else if (!positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);

    for (unsigned int i = 0; i < pNode->m_Walls.size(); i++)
    {
        KDTreeRenderer::Wall wall(GetWallFromNode(pNode, i));
        if ((WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom) <= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexTo) <= 0) ||
            (WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom) >= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexTo) >= 0) ||
            (DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexFrom) <= 0 &&
             DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexTo) <= 0))
        {
            // Culling (wall is entirely outside frustum)
            continue;
        }
        else
        {
            int minAngle = m_PlayerHorizontalFOV / 2, maxAngle = -m_PlayerHorizontalFOV / 2;
            Vertex minVertex, maxVertex;

            bool vertexFromInsideFrustum = isInsideFrustum(wall.m_VertexFrom);
            bool vertexToInsideFrustum = isInsideFrustum(wall.m_VertexTo);

            if (!vertexFromInsideFrustum || !vertexToInsideFrustum)
            {
                Vertex intersectionVertex;
                if (HalfLineSegmentIntersection<Vertex>(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                {
                    minVertex = intersectionVertex;
                    minAngle = -m_PlayerHorizontalFOV / 2;
                }
                if (HalfLineSegmentIntersection<Vertex>(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                {
                    maxVertex = intersectionVertex;
                    maxAngle = m_PlayerHorizontalFOV / 2;
                }
            }

            auto updateMinMaxAnglesAndVertices = [&](int iAngle, const Vertex &iVertex)
            {
                if (iAngle < minAngle)
                {
                    minAngle = iAngle;
                    minVertex = iVertex;
                }
                if (iAngle > maxAngle)
                {
                    maxAngle = iAngle;
                    maxVertex = iVertex;
                }
            };

            if(vertexFromInsideFrustum)
            {
                int angle = Angle(m_PlayerPosition, m_Look, wall.m_VertexFrom);
                updateMinMaxAnglesAndVertices(angle, wall.m_VertexFrom);
            }

            if (vertexToInsideFrustum)
            {
                int angle = Angle(m_PlayerPosition, m_Look, wall.m_VertexTo);
                updateMinMaxAnglesAndVertices(angle, wall.m_VertexTo);
            }

            // Should always be true
            if(minAngle <= maxAngle)
                RenderWall(wall, minVertex, maxVertex, minAngle, maxAngle);
        }
    }

    if (positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);
    else if (!positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
}

void KDTreeRenderer::RenderWall(const Wall &iWall, const Vertex &iMinVertex, const Vertex &iMaxVertex, int iMinAngle, int iMaxAngle)
{
    // TODO: there has to be (multiple) way(s) to refactor this harder
    // TODO: computations are ugly, need to write a clean fixed-point arithmetic class instead of doing
    // shady things

    int minX = ((iMinAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV;
    int maxX = ((iMaxAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV - 1;

    if(minX > maxX)
        return;

    minX = Clamp(minX, 0, WINDOW_WIDTH - 1);
    maxX = Clamp(maxX, 0, WINDOW_WIDTH - 1);

    int minDist = ARITHMETIC_SHIFT(DistInt(m_PlayerPosition, iMinVertex) * cosInt(iMinAngle), DECIMAL_SHIFT); // Correction for distortion
    int maxDist = ARITHMETIC_SHIFT(DistInt(m_PlayerPosition, iMaxVertex) * cosInt(iMaxAngle), DECIMAL_SHIFT);   // Correction for distortion

    // TODO: perform actual clipping
    // Dirty hack
    if(maxDist <= 0)
        return;
    minDist = minDist <= 0 ? 1 : minDist;

    char r = iWall.m_pKDWall->m_InSector % 3 == 0 ? 1 : 0;
    char g = iWall.m_pKDWall->m_InSector % 3 == 1 ? 1 : 0;
    char b = iWall.m_pKDWall->m_InSector % 3 == 2 ? 1 : 0;
    int maxColorRange = iWall.m_VertexFrom.m_X == iWall.m_VertexTo.m_X ? 230 : 180;
    int minColorClamp = iWall.m_VertexFrom.m_X == iWall.m_VertexTo.m_X ? 55 : 50;

    int whichSide = WhichSide(iWall.m_VertexFrom, iWall.m_VertexTo, m_PlayerPosition);

    int inSectorIdx = iWall.m_pKDWall->m_InSector;
    const KDMapData::Sector &inSector = m_Map.m_Sectors[inSectorIdx];

    int outSectorIdx = iWall.m_pKDWall->m_OutSector;
    const KDMapData::Sector &outSector = m_Map.m_Sectors[outSectorIdx];

    int minVertexColor = ((m_MaxColorInterpolationDist - minDist) * maxColorRange) / m_MaxColorInterpolationDist;
    int maxVertexColor = ((m_MaxColorInterpolationDist - maxDist) * maxColorRange) / m_MaxColorInterpolationDist;
    minVertexColor = Clamp(minVertexColor, minColorClamp, maxColorRange);
    maxVertexColor = Clamp(maxVertexColor, minColorClamp, maxColorRange);

    int t, minY, maxY, minYUnclamped, maxYUnclamped;

    // Hard wall: no outer sector, which means nothing will be drawn behind this wall
    if (outSectorIdx == -1 && whichSide > 0)
    {
        int eyeToTop = inSector.ceiling - m_PlayerZ;
        int eyeToBottom = m_PlayerZ - inSector.floor;

        int minVertexBottomPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToBottom / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        int minVertexTopPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToTop / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

        int maxVertexBottomPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToBottom / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        int maxVertexTopPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToTop / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

        FlatSurface floorSurface;
        floorSurface.m_MinX = minX;
        floorSurface.m_MaxX = maxX;
        floorSurface.m_SectorIdx = inSectorIdx;
        floorSurface.m_Height = m_Map.m_Sectors[inSectorIdx].floor;

        FlatSurface ceilingSurface(floorSurface);
        ceilingSurface.m_Height = m_Map.m_Sectors[inSectorIdx].ceiling;

        bool addFloorSurface = false;
        bool addCeilingSurface = false;

        for (int x = minX; x <= maxX; x++)
        {
            // TODO: optimize divisions
            if (!m_pHorizOcclusionBuffer[x])
            {
                ComputeRenderParameters(x, minX, maxX, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

                floorSurface.m_MaxY[x] = std::min(minYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
                floorSurface.m_MinY[x] = m_pBottomOcclusionBuffer[x];
                if (!addFloorSurface && floorSurface.m_MinY[x] < floorSurface.m_MaxY[x])
                    addFloorSurface = true;

                ceilingSurface.m_MinY[x] = std::max(maxYUnclamped, m_pBottomOcclusionBuffer[x]);
                ceilingSurface.m_MaxY[x] = WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x];
                if (!addCeilingSurface && ceilingSurface.m_MinY[x] < ceilingSurface.m_MaxY[x])
                    addCeilingSurface = true;

                if(minY <= maxY)
                    RenderColumn(t, minVertexColor, maxVertexColor, minY, maxY, x, r, g, b);
            }
        }
        // Nothing will be drawn behind this wall
        memset(m_pHorizOcclusionBuffer + minX, 1u, maxX - minX + 1);

        if(addFloorSurface)
            AddFlatSurface(floorSurface);
        if(addCeilingSurface)
            AddFlatSurface(ceilingSurface);
    }
    // Soft wall, what I believe the Doom engine calls "portals"
    else if (outSectorIdx != -1)
    {
        auto RenderBottom = [&]() {
            // Render bottom part of the sector
            // TODO: /!\ CAREFUL when adding flat surfaces, need to add them even if inSector.floor == outSector.floor
            // if (inSector.floor != outSector.floor)
            {
                int eyeToTopFloor = m_PlayerZ - std::max(inSector.floor, outSector.floor);
                int eyeToBottomFloor = m_PlayerZ - std::min(inSector.floor, outSector.floor);

                int minVertexBottomPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToBottomFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                int minVertexTopPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToTopFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                int maxVertexBottomPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToBottomFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                int maxVertexTopPixel = ((-atanInt((1 << DECIMAL_SHIFT) * eyeToTopFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                FlatSurface floorSurface;
                floorSurface.m_MinX = minX;
                floorSurface.m_MaxX = maxX;
                floorSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
                floorSurface.m_Height = m_Map.m_Sectors[floorSurface.m_SectorIdx].floor;

                bool wallIsVisible = (whichSide > 0 && inSector.floor < outSector.floor) ||
                                     (whichSide < 0 && outSector.floor < inSector.floor);
                bool addFloorSurface = false;

                for (unsigned int x = minX; x <= maxX; x++)
                {
                    // TODO: optimize divisions
                    if (!m_pHorizOcclusionBuffer[x])
                    {
                        ComputeRenderParameters(x, minX, maxX, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

                        if(wallIsVisible)
                            floorSurface.m_MaxY[x] = std::min(minYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
                        else
                            floorSurface.m_MaxY[x] = std::min(maxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
                        floorSurface.m_MinY[x] = m_pBottomOcclusionBuffer[x];

                        if(!addFloorSurface && floorSurface.m_MinY[x] < floorSurface.m_MaxY[x])
                            addFloorSurface = true;

                        // We need to fill the occlusion buffer even if we don't draw there, since it will be
                        // used for floor and ceiling surfaces
                        m_pBottomOcclusionBuffer[x] = std::max(m_pBottomOcclusionBuffer[x], maxY);
                        if (minY <= maxY && wallIsVisible)
                            RenderColumn(t, minVertexColor, maxVertexColor, minY, maxY, x, r, g, b);
                    }
                }

                if(addFloorSurface)
                    AddFlatSurface(floorSurface);
            }
        };

        auto RenderTop = [&]() {
            // Render top part of the sector
            // TODO: /!\ CAREFUL when adding flat surfaces, need to add them even if inSector.ceiling == outSector.ceiling
            // if (inSector.ceiling != outSector.ceiling)
            {
                int eyeToTopCeiling = std::max(inSector.ceiling, outSector.ceiling) - m_PlayerZ;
                int eyeToBottomCeiling = std::min(inSector.ceiling, outSector.ceiling) - m_PlayerZ;

                int minVertexBottomPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToBottomCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                int minVertexTopPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToTopCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                int maxVertexBottomPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToBottomCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                int maxVertexTopPixel = ((atanInt((1 << DECIMAL_SHIFT) * eyeToTopCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                FlatSurface ceilingSurface;
                ceilingSurface.m_MinX = minX;
                ceilingSurface.m_MaxX = maxX;
                ceilingSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
                ceilingSurface.m_Height = m_Map.m_Sectors[ceilingSurface.m_SectorIdx].ceiling;

                bool wallIsVisible = (whichSide > 0 && inSector.ceiling > outSector.ceiling) ||
                                     (whichSide < 0 && outSector.ceiling > inSector.ceiling);

                bool addCeilingSurface = false;

                for (unsigned int x = minX; x <= maxX; x++)
                {
                    // TODO: optimize divisions
                    if (!m_pHorizOcclusionBuffer[x])
                    {
                        ComputeRenderParameters(x, minX, maxX, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

                        if(wallIsVisible)
                            ceilingSurface.m_MinY[x] = std::max(maxYUnclamped, m_pBottomOcclusionBuffer[x]);
                        else
                            ceilingSurface.m_MinY[x] = std::max(minYUnclamped, m_pBottomOcclusionBuffer[x]);
                        ceilingSurface.m_MaxY[x] = WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x];

                        if (!addCeilingSurface && ceilingSurface.m_MinY[x] < ceilingSurface.m_MaxY[x])
                            addCeilingSurface = true;

                        // We need to fill the occlusion buffer even if we don't draw there, since it will be
                        // used for floor and ceiling surfaces
                        m_pTopOcclusionBuffer[x] = std::max(WINDOW_HEIGHT - 1 - minYUnclamped, m_pTopOcclusionBuffer[x]);
                        if (minY <= maxY && wallIsVisible)
                            RenderColumn(t, minVertexColor, maxVertexColor, minY, maxY, x, r, g, b);
                    }
                }

                if(addCeilingSurface)
                    AddFlatSurface(ceilingSurface);
            }
        };

        if ((whichSide > 0 && outSector.ceiling > inSector.ceiling) ||
            (whichSide < 0 && outSector.ceiling < inSector.ceiling))
        {
            RenderTop();
            RenderBottom();
        }
        else
        {
            RenderBottom();
            RenderTop();
        }
    }
}

bool KDTreeRenderer::AddFlatSurface(const FlatSurface &iFlatSurface)
{
    bool hasBeenAbsorbed = false;
    for(auto &k : m_FlatSurfaces)
    {
        for (FlatSurface &flatSurface : k.second)
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
    int vFovRad = ARITHMETIC_SHIFT(m_PlayerVerticalFOV, ANGLE_SHIFT) * (314 << DECIMAL_SHIFT) / (180 * 100);

    for(const auto &keyVal : m_FlatSurfaces)
    {
        const std::vector<FlatSurface> &currentSurfaces = keyVal.second;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            const FlatSurface &currentSurface = currentSurfaces[i];

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

                int theta = (m_PlayerVerticalFOV * (2.0 * y - WINDOW_HEIGHT)) / (2.0 * WINDOW_HEIGHT);
                int sinTheta = sinInt(theta);

                if (sinTheta != 0)
                {
                    int dist = std::abs(((m_PlayerZ - currentSurface.m_Height) << DECIMAL_SHIFT) / sinTheta); // no need to right shift, sinTheta is already shifted
                    dist = std::min(dist, m_MaxColorInterpolationDist);
                    int color = ((m_MaxColorInterpolationDist - dist) * maxColorRange) / m_MaxColorInterpolationDist;
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
    {
        m_LinesXStart[i] = -1;
        m_HeightYCache[i] = 0;
        m_DistYCache[i] = -1;
    }

    for(const auto &keyVal : m_FlatSurfaces)
    {
        const std::vector<FlatSurface> &currentSurfaces = keyVal.second;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            const FlatSurface &currentSurface = currentSurfaces[i];

            char r = currentSurface.m_SectorIdx % 3 == 0 ? 1 : 0;
            char g = currentSurface.m_SectorIdx % 3 == 1 ? 1 : 0;
            char b = currentSurface.m_SectorIdx % 3 == 2 ? 1 : 0;
            int maxColorRange = 150;

            auto DrawLine = [&](int iY, int iMinX, int iMaxX) {
                int dist = -1;
                int color;

                if (m_HeightYCache[iY] == currentSurface.m_Height && m_DistYCache[iY] >= 0)
                {
                    dist = m_DistYCache[iY];
                }
                else
                {
                    int theta = (m_PlayerVerticalFOV * (2.0 * iY - WINDOW_HEIGHT)) / (2.0 * WINDOW_HEIGHT);
                    int sinTheta = sinInt(theta);

                    if (sinTheta != 0)
                    {
                        dist = std::abs(((m_PlayerZ - currentSurface.m_Height) << DECIMAL_SHIFT) / sinTheta); // no need to right shift, sinTheta is already shifted
                        dist = std::min(dist, m_MaxColorInterpolationDist);
                        m_HeightYCache[iY] = currentSurface.m_Height;
                        m_DistYCache[iY] = dist;
                    }
                }

                if(dist >= 0)
                {
                    color = ((m_MaxColorInterpolationDist - dist) * maxColorRange) / m_MaxColorInterpolationDist;
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

int KDTreeRenderer::ComputeZ()
{
    return RecursiveComputeZ(m_Map.m_RootNode);
}

int KDTreeRenderer::RecursiveComputeZ(KDTreeNode *pNode)
{
    if(!pNode)
        return 0; // Should not happen

    int oZ = 0;

    bool positiveSide;
    if(pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_PlayerPosition.m_X > pNode->m_SplitOffset;
    else
        positiveSide = m_PlayerPosition.m_Y > pNode->m_SplitOffset;

    // We are on a terminal node
    // The node contains exactly one wall (or multiple walls oriented similarly and refering
    // to the same sectors), which we are going to use to find out in which sector the player is
    if ((positiveSide && !pNode->m_PositiveSide) ||
        (!positiveSide && !pNode->m_NegativeSide))
    {
        if(pNode->GetNbOfWalls()) // Should always be true
        {
            Wall wall(GetWallFromNode(pNode, 0));
            int whichSide = WhichSide(wall.m_VertexFrom, wall.m_VertexTo, m_PlayerPosition);
            if(whichSide < 0)
            {
                int outSector = wall.m_pKDWall->m_OutSector;
                if (outSector >= 0)
                    oZ = m_Map.m_Sectors[outSector].floor + m_PlayerHeight;
                oZ = Clamp(oZ, m_Map.m_Sectors[outSector].floor, m_Map.m_Sectors[outSector].ceiling);
            }
            else
            {
                int inSector = wall.m_pKDWall->m_InSector;
                if (inSector >= 0)
                    oZ = m_Map.m_Sectors[inSector].floor + m_PlayerHeight;
                oZ = Clamp(oZ, m_Map.m_Sectors[inSector].floor, m_Map.m_Sectors[inSector].ceiling);
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

bool KDTreeRenderer::isInsideFrustum(const Vertex &iVertex) const
{
    return (WhichSide(m_PlayerPosition, m_FrustumToLeft, iVertex) >= 0) &&
           (WhichSide(m_PlayerPosition, m_FrustumToRight, iVertex) <= 0);
}

void KDTreeRenderer::SetPlayerCoordinates(const KDTreeRenderer::Vertex &iPosition, int iDirection)
{
    m_PlayerPosition = iPosition;
    m_PlayerDirection = iDirection;

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerHorizontalFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerHorizontalFOV / 2, m_FrustumToRight);
    GetVector(m_PlayerPosition, m_PlayerDirection, m_Look);
}

KDTreeRenderer::Vertex KDTreeRenderer::GetPlayerPosition() const
{
    return m_PlayerPosition;
}

int KDTreeRenderer::GetPlayerDirection() const
{
    return m_PlayerDirection;
}

KDTreeRenderer::Vertex KDTreeRenderer::GetLook() const
{
    return m_Look;
}