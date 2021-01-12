#include "KDTreeRenderer.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include <cstring>

namespace
{
    KDRData::Wall GetWallFromNode(KDTreeNode *ipNode, unsigned int iWallIdx)
    {
        KDRData::Wall wall;

        if(ipNode)
        {
            wall = ipNode->BuildXYScaledWall<KDRData::Wall>(iWallIdx);
            wall.m_pKDWall = ipNode->GetWall(iWallIdx);
        }

        return wall;
    }

    KDRData::Sector GetSectorFromKDSector(const KDMapData::Sector &iSector)
    {
        KDRData::Sector sector;

        sector.m_pKDSector = &iSector;
        sector.m_Ceiling = CType(iSector.ceiling) / POSITION_SCALE;
        sector.m_Floor = CType(iSector.floor) / POSITION_SCALE;

        return sector;
    }
}

KDTreeRenderer::KDTreeRenderer(const KDTreeMap &iMap) :
    m_Map(iMap),
    m_pFrameBuffer(new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4u]),
    m_PlayerHorizontalFOV(90 * (1 << ANGLE_SHIFT)),
    m_PlayerVerticalFOV((m_PlayerHorizontalFOV * WINDOW_HEIGHT) / WINDOW_WIDTH),
    m_PlayerHeight(CType(30) / POSITION_SCALE),
    m_MaxColorInterpolationDist(CType(500) / POSITION_SCALE),
    m_HorizontalDistortionCst(1 / (2 * tanInt(m_PlayerHorizontalFOV / 2))),
    m_VerticalDistortionCst(1 / (2 * tanInt(m_PlayerVerticalFOV / 2)))
{
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
        positiveSide = m_PlayerPosition.m_X > (CType(pNode->m_SplitOffset) / POSITION_SCALE);
    else if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::YConst)
        positiveSide = m_PlayerPosition.m_Y > (CType(pNode->m_SplitOffset) / POSITION_SCALE);

    if (positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
    else if (!positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);

    for (unsigned int i = 0; i < pNode->m_Walls.size(); i++)
    {
        KDRData::Wall wall(GetWallFromNode(pNode, i));

        bool vertexFromIsBehindPlayer = DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexFrom) <= 0;
        bool vertexToIsBehindPlayer = DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexTo) <= 0;

        if ((WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom) <= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexTo) <= 0) ||
            (WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom) >= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexTo) >= 0) ||
            (vertexFromIsBehindPlayer && vertexToIsBehindPlayer))
        {
            // Culling (wall is entirely outside frustum)
            continue;
        }
        else
        {
            // Clip against frustum
            int minAngle = m_PlayerHorizontalFOV / 2, maxAngle = -m_PlayerHorizontalFOV / 2;
            KDRData::Vertex minVertex, maxVertex;

            bool vertexFromInsideFrustum = isInsideFrustum(wall.m_VertexFrom);
            bool vertexToInsideFrustum = isInsideFrustum(wall.m_VertexTo);

            if (!vertexFromInsideFrustum || !vertexToInsideFrustum)
            {
                KDRData::Vertex intersectionVertex;
                if (HalfLineSegmentIntersection<KDRData::Vertex>(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                {
                    minVertex = intersectionVertex;
                    minAngle = -m_PlayerHorizontalFOV / 2;
                }
                if (HalfLineSegmentIntersection<KDRData::Vertex>(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                {
                    maxVertex = intersectionVertex;
                    maxAngle = m_PlayerHorizontalFOV / 2;
                }
            }

            auto updateMinMaxAnglesAndVertices = [&](int iAngle, const KDRData::Vertex &iVertex) {
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

void KDTreeRenderer::RenderWall(const KDRData::Wall &iWall, const KDRData::Vertex &iMinVertex, const KDRData::Vertex &iMaxVertex, int iMinAngle, int iMaxAngle)
{
    // TODO: there has to be (multiple) way(s) to refactor this harder

    // Need to correct for horizontal distortions around the edges of the projection screen
    // I'm leaving the old formulas as comments since they are much more intuitive

    // See below
    // Also see https://stackoverflow.com/questions/24173966/raycasting-engine-rendering-creating-slight-distortion-increasing-towards-edges

    // Without horizontal distortion correction
    // int minX = ((iMinAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV;
    // int maxX = ((iMaxAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV - 1;
    
    // Horizontal distortion correction
    // int minX = WINDOW_WIDTH / 2 + tanInt(iMinAngle) / tanInt(m_PlayerHorizontalFOV / 2) * (WINDOW_WIDTH / 2);
    // int maxX = WINDOW_WIDTH / 2 + tanInt(iMaxAngle) / tanInt(m_PlayerHorizontalFOV / 2) * (WINDOW_WIDTH / 2);

    // Same as above but with little refacto
    int minX = WINDOW_WIDTH / 2 + WINDOW_WIDTH * tanInt(iMinAngle) * m_HorizontalDistortionCst;
    int maxX = WINDOW_WIDTH / 2 + WINDOW_WIDTH * tanInt(iMaxAngle) * m_HorizontalDistortionCst;

    if(minX > maxX)
        return;

    minX = Clamp(minX, 0, WINDOW_WIDTH - 1);
    maxX = Clamp(maxX, 0, WINDOW_WIDTH - 1);

    // We need further precision for this ratio
    CType InvMinMaxXRange = maxX == minX ? static_cast<CType>(0) : (1 << 7u) / CType(maxX - minX);

    CType minDist = DistInt(m_PlayerPosition, iMinVertex) * cosInt(iMinAngle); // Correction for vertical distortion
    CType maxDist = DistInt(m_PlayerPosition, iMaxVertex) * cosInt(iMaxAngle); // Correction for vertical distortion

    // TODO: perform actual clipping
    // Dirty hack
    if(maxDist <= 0)
        return;
    minDist = minDist <= CType(0) ? CType(1) : minDist;

    char r = iWall.m_pKDWall->m_InSector % 3 == 0 ? 1 : 0;
    char g = iWall.m_pKDWall->m_InSector % 3 == 1 ? 1 : 0;
    char b = iWall.m_pKDWall->m_InSector % 3 == 2 ? 1 : 0;
    int maxColorRange = iWall.m_VertexFrom.m_X == iWall.m_VertexTo.m_X ? 230 : 180;
    int minColorClamp = iWall.m_VertexFrom.m_X == iWall.m_VertexTo.m_X ? 55 : 50;

    int whichSide = WhichSide(iWall.m_VertexFrom, iWall.m_VertexTo, m_PlayerPosition);

    int inSectorIdx = iWall.m_pKDWall->m_InSector;
    const KDRData::Sector &inSector = GetSectorFromKDSector(m_Map.m_Sectors[inSectorIdx]);

    int outSectorIdx = iWall.m_pKDWall->m_OutSector;
    const KDRData::Sector &outSector = GetSectorFromKDSector(m_Map.m_Sectors[outSectorIdx]);

    int minVertexColor = ((m_MaxColorInterpolationDist - minDist) * maxColorRange) / m_MaxColorInterpolationDist;
    int maxVertexColor = ((m_MaxColorInterpolationDist - maxDist) * maxColorRange) / m_MaxColorInterpolationDist;
    minVertexColor = Clamp(minVertexColor, minColorClamp, maxColorRange);
    maxVertexColor = Clamp(maxVertexColor, minColorClamp, maxColorRange);

    CType t;
    int minY, maxY, minYUnclamped, maxYUnclamped;

    // Hard wall: no outer sector, which means nothing will be drawn behind this wall
    if (outSectorIdx == -1 && whichSide > 0)
    {
        CType eyeToTop = inSector.m_Ceiling - m_PlayerZ;
        CType eyeToBottom = m_PlayerZ - inSector.m_Floor;

        // Same here, need to correct for distortion
        // I'm leaving the old formulas as comments since they are much more intuitive

        // Without vertical distortion
        // int minVertexBottomPixel = ((-atanInt(eyeToBottom / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        // int minVertexTopPixel = ((atanInt(eyeToTop / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        // int maxVertexBottomPixel = ((-atanInt(eyeToBottom / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        // int maxVertexTopPixel = ((atanInt(eyeToTop / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

        int minAngleEyeToBottom = atanInt(eyeToBottom / minDist);
        int minAngleEyeToTop = atanInt(eyeToTop / minDist);
        int maxAngleEyeToBottom = atanInt(eyeToBottom / maxDist);
        int maxAngleEyeToTop = atanInt(eyeToTop / maxDist);

        int minVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToBottom) * m_VerticalDistortionCst;
        int minVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToTop) * m_VerticalDistortionCst;
        int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToBottom) * m_VerticalDistortionCst;
        int maxVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToTop) * m_VerticalDistortionCst;

        KDRData::FlatSurface floorSurface;
        floorSurface.m_MinX = minX;
        floorSurface.m_MaxX = maxX;
        floorSurface.m_SectorIdx = inSectorIdx;
        floorSurface.m_Height = inSector.m_Floor;

        KDRData::FlatSurface ceilingSurface(floorSurface);
        ceilingSurface.m_Height = inSector.m_Ceiling;

        bool addFloorSurface = false;
        bool addCeilingSurface = false;

        for (int x = minX; x <= maxX; x++)
        {
            // TODO: optimize divisions
            if (!m_pHorizOcclusionBuffer[x])
            {
                ComputeRenderParameters(x, minX, maxX, InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

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
                CType eyeToTopFloor = m_PlayerZ - std::max(inSector.m_Floor, outSector.m_Floor);
                CType eyeToBottomFloor = m_PlayerZ - std::min(inSector.m_Floor, outSector.m_Floor);

                // Same here, need to correct for distortion
                // I'm leaving the old formulas as comments since they are much more intuitive

                // Without vertical distortion
                // int minVertexBottomPixel = ((-atanInt(eyeToBottomFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int minVertexTopPixel = ((-atanInt(eyeToTopFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int maxVertexBottomPixel = ((-atanInt(eyeToBottomFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int maxVertexTopPixel = ((-atanInt(eyeToTopFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                int minAngleEyeToBottomFloor = atanInt(eyeToBottomFloor / minDist);
                int minAngleEyeToTopFloor = atanInt(eyeToTopFloor / minDist);
                int maxAngleEyeToBottomFloor = atanInt(eyeToBottomFloor / maxDist);
                int maxAngleEyeToTopFloor = atanInt(eyeToTopFloor / maxDist);

                int minVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToBottomFloor) * m_VerticalDistortionCst;
                int minVertexTopPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToTopFloor) * m_VerticalDistortionCst;
                int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToBottomFloor) * m_VerticalDistortionCst;
                int maxVertexTopPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToTopFloor) * m_VerticalDistortionCst;

                KDRData::FlatSurface floorSurface;
                floorSurface.m_MinX = minX;
                floorSurface.m_MaxX = maxX;
                floorSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
                floorSurface.m_Height = whichSide > 0 ? inSector.m_Floor : outSector.m_Floor;

                bool wallIsVisible = (whichSide > 0 && inSector.m_Floor < outSector.m_Floor) ||
                                     (whichSide < 0 && outSector.m_Floor < inSector.m_Floor);
                bool addFloorSurface = false;

                for (unsigned int x = minX; x <= maxX; x++)
                {
                    // TODO: optimize divisions
                    if (!m_pHorizOcclusionBuffer[x])
                    {
                        ComputeRenderParameters(x, minX, maxX, InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

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
                CType eyeToTopCeiling = std::max(inSector.m_Ceiling, outSector.m_Ceiling) - m_PlayerZ;
                CType eyeToBottomCeiling = std::min(inSector.m_Ceiling, outSector.m_Ceiling) - m_PlayerZ;

                // Same here, need to correct for distortion
                // I'm leaving the old formulas as comments since they are much more intuitive

                // int minVertexBottomPixel = ((atanInt(eyeToBottomCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int minVertexTopPixel = ((atanInt(eyeToTopCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int maxVertexBottomPixel = ((atanInt(eyeToBottomCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
                // int maxVertexTopPixel = ((atanInt(eyeToTopCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

                int minAngleEyeToBottomCeiling = atanInt(eyeToBottomCeiling / minDist);
                int minAngleEyeToTopCeiling = atanInt(eyeToTopCeiling / minDist);
                int maxAngleEyeToBottomCeiling = atanInt(eyeToBottomCeiling / maxDist);
                int maxAngleEyeToTopCeiling = atanInt(eyeToTopCeiling / maxDist);

                int minVertexBottomPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToBottomCeiling) * m_VerticalDistortionCst;
                int minVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToTopCeiling) * m_VerticalDistortionCst;
                int maxVertexBottomPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToBottomCeiling) * m_VerticalDistortionCst;
                int maxVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToTopCeiling) * m_VerticalDistortionCst;

                KDRData::FlatSurface ceilingSurface;
                ceilingSurface.m_MinX = minX;
                ceilingSurface.m_MaxX = maxX;
                ceilingSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
                ceilingSurface.m_Height = whichSide > 0 ? inSector.m_Ceiling : outSector.m_Ceiling;

                bool wallIsVisible = (whichSide > 0 && inSector.m_Ceiling > outSector.m_Ceiling) ||
                                     (whichSide < 0 && outSector.m_Ceiling > inSector.m_Ceiling);

                bool addCeilingSurface = false;

                for (unsigned int x = minX; x <= maxX; x++)
                {
                    // TODO: optimize divisions
                    if (!m_pHorizOcclusionBuffer[x])
                    {
                        ComputeRenderParameters(x, minX, maxX, InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

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

        if ((whichSide > 0 && outSector.m_Ceiling > inSector.m_Ceiling) ||
            (whichSide < 0 && outSector.m_Ceiling < inSector.m_Ceiling))
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
    int vFovRad = ARITHMETIC_SHIFT(m_PlayerVerticalFOV, ANGLE_SHIFT) * (314 << FP_SHIFT) / (180 * 100);

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

                int theta = (m_PlayerVerticalFOV * (2 * y - WINDOW_HEIGHT)) / (2 * WINDOW_HEIGHT);
                CType sinTheta = sinInt(theta);

                if (sinTheta != 0)
                {
                    CType dist = (((m_PlayerZ - CType(currentSurface.m_Height))) / sinTheta);
                    dist = dist < 0 ? -dist : dist;
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
                    int theta = (m_PlayerVerticalFOV * (2 * iY - WINDOW_HEIGHT)) / (2 * WINDOW_HEIGHT);
                    CType sinTheta = sinInt(theta);

                    if (sinTheta != 0)
                    {
                        dist = (m_PlayerZ - currentSurface.m_Height) / sinTheta;
                        dist = dist < 0 ? -dist : dist;
                        dist = std::min(dist, m_MaxColorInterpolationDist);
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
        positiveSide = m_PlayerPosition.m_X > (CType(pNode->m_SplitOffset) / POSITION_SCALE);
    else if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::YConst)
        positiveSide = m_PlayerPosition.m_Y > (CType(pNode->m_SplitOffset) / POSITION_SCALE);

    // We are on a terminal node
    // The node contains exactly one wall (or multiple walls oriented similarly and refering
    // to the same sectors), which we are going to use to find out in which sector the player is
    if ((positiveSide && !pNode->m_PositiveSide) ||
        (!positiveSide && !pNode->m_NegativeSide))
    {
        if(pNode->GetNbOfWalls()) // Should always be true
        {
            oZ = m_PlayerHeight;
            KDRData::Wall wall(GetWallFromNode(pNode, 0));
            int whichSide = WhichSide(wall.m_VertexFrom, wall.m_VertexTo, m_PlayerPosition);
            if(whichSide < 0)
            {
                int outSectorIdx = wall.m_pKDWall->m_OutSector;
                if (outSectorIdx >= 0)
                {
                    KDRData::Sector outSector = GetSectorFromKDSector(m_Map.m_Sectors[outSectorIdx]);
                    oZ += outSector.m_Floor;
                    oZ = Clamp(oZ, outSector.m_Floor, outSector.m_Ceiling);
                }
            }
            else
            {
                int inSectorIdx = wall.m_pKDWall->m_InSector;
                if (inSectorIdx >= 0)
                {
                    KDRData::Sector inSector = GetSectorFromKDSector(m_Map.m_Sectors[inSectorIdx]);
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

bool KDTreeRenderer::isInsideFrustum(const KDRData::Vertex &iVertex) const
{
    return (WhichSide(m_PlayerPosition, m_FrustumToLeft, iVertex) >= 0) &&
           (WhichSide(m_PlayerPosition, m_FrustumToRight, iVertex) <= 0);
}

void KDTreeRenderer::SetPlayerCoordinates(const KDRData::Vertex &iPosition, int iDirection)
{
    m_PlayerPosition = iPosition;
    m_PlayerDirection = iDirection;

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerHorizontalFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerHorizontalFOV / 2, m_FrustumToRight);
    GetVector(m_PlayerPosition, m_PlayerDirection, m_Look);
}

KDRData::Vertex KDTreeRenderer::GetPlayerPosition() const
{
    return m_PlayerPosition;
}

int KDTreeRenderer::GetPlayerDirection() const
{
    return m_PlayerDirection;
}

KDRData::Vertex KDTreeRenderer::GetLook() const
{
    return m_Look;
}