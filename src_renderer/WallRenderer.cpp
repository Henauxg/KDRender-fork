#include "WallRenderer.h"

#include "GeomUtils.h"

#include <algorithm>
#include <cstring>

WallRenderer::WallRenderer(KDRData::Wall &iWall, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap):
    m_Wall(iWall),
    m_State(iState),
    m_Settings(iSettings),
    m_Map(iMap)
{
    // For debug purposes
    r = iWall.m_pKDWall->m_InSector % 3 == 0 ? 1 : 0;
    g = iWall.m_pKDWall->m_InSector % 3 == 1 ? 1 : 0;
    b = iWall.m_pKDWall->m_InSector % 3 == 2 ? 1 : 0;
}

WallRenderer::~WallRenderer()
{

}

void WallRenderer::SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer)
{
    m_pFrameBuffer = ipFrameBuffer;
    m_pHorizOcclusionBuffer = ipHorizOcclusionBuffer;
    m_pTopOcclusionBuffer = ipTopOcclusionBuffer;
    m_pBottomOcclusionBuffer = ipBottomOcclusionBuffer;
}

void WallRenderer::Render(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    // Clip against frustum
    minAngle = m_Settings.m_PlayerHorizontalFOV / 2;
    maxAngle = -m_Settings.m_PlayerHorizontalFOV / 2;

    bool vertexFromInsideFrustum = isInsideFrustum(m_Wall.m_VertexFrom);
    bool vertexToInsideFrustum = isInsideFrustum(m_Wall.m_VertexTo);

    if (!vertexFromInsideFrustum || !vertexToInsideFrustum)
    {
        KDRData::Vertex intersectionVertex;
        if (HalfLineSegmentIntersection<KDRData::Vertex>(m_State.m_PlayerPosition, m_State.m_FrustumToLeft, m_Wall.m_VertexFrom, m_Wall.m_VertexTo, intersectionVertex))
        {
            minVertex = intersectionVertex;
            minAngle = -m_Settings.m_PlayerHorizontalFOV / 2;
        }
        if (HalfLineSegmentIntersection<KDRData::Vertex>(m_State.m_PlayerPosition, m_State.m_FrustumToRight, m_Wall.m_VertexFrom, m_Wall.m_VertexTo, intersectionVertex))
        {
            maxVertex = intersectionVertex;
            maxAngle = m_Settings.m_PlayerHorizontalFOV / 2;
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

    if (vertexFromInsideFrustum)
    {
        int angle = Angle(m_State.m_PlayerPosition, m_State.m_Look, m_Wall.m_VertexFrom);
        updateMinMaxAnglesAndVertices(angle, m_Wall.m_VertexFrom);
    }

    if (vertexToInsideFrustum)
    {
        int angle = Angle(m_State.m_PlayerPosition, m_State.m_Look, m_Wall.m_VertexTo);
        updateMinMaxAnglesAndVertices(angle, m_Wall.m_VertexTo);
    }

    // Should always be true
    if (minAngle <= maxAngle)
        RenderWall(oGeneratedFlats);
}

void WallRenderer::RenderWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    // TODO: there has to be (multiple) way(s) to refactor this harder

    // Need to correct for horizontal distortions around the edges of the projection screen
    // I'm leaving the old formulas as comments since they are much more intuitive

    // See below
    // Also see https://stackoverflow.com/questions/24173966/raycasting-engine-rendering-creating-slight-distortion-increasing-towards-edges

    // Without horizontal distortion correction
    // int minX = ((minAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV;
    // int maxX = ((maxAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV - 1;

    // Horizontal distortion correction
    // int minX = WINDOW_WIDTH / 2 + tanInt(minAngle) / tanInt(m_PlayerHorizontalFOV / 2) * (WINDOW_WIDTH / 2);
    // int maxX = WINDOW_WIDTH / 2 + tanInt(maxAngle) / tanInt(m_PlayerHorizontalFOV / 2) * (WINDOW_WIDTH / 2);

    // Same as above but with little refacto
    minX = WINDOW_WIDTH / 2 + WINDOW_WIDTH * tanInt(minAngle) * m_Settings.m_HorizontalDistortionCst;
    maxX = WINDOW_WIDTH / 2 + WINDOW_WIDTH * tanInt(maxAngle) * m_Settings.m_HorizontalDistortionCst;

    if (minX > maxX)
        return;

    minX = Clamp(minX, 0, WINDOW_WIDTH - 1);
    maxX = Clamp(maxX, 0, WINDOW_WIDTH - 1);

    // We need further precision for this ratio
    InvMinMaxXRange = maxX == minX ? static_cast<CType>(0) : (1 << 7u) / CType(maxX - minX);

    minDist = DistInt(m_State.m_PlayerPosition, minVertex) * cosInt(minAngle); // Correction for vertical distortion
    maxDist = DistInt(m_State.m_PlayerPosition, maxVertex) * cosInt(maxAngle); // Correction for vertical distortion

    // TODO: perform actual clipping
    // Dirty hack
    if (maxDist <= 0)
        return;
    minDist = minDist <= CType(0) ? CType(1) : minDist;

    maxColorRange = m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X ? 230 : 180;
    minColorClamp = m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X ? 55 : 50;

    whichSide = WhichSide(m_Wall.m_VertexFrom, m_Wall.m_VertexTo, m_State.m_PlayerPosition);

    inSectorIdx = m_Wall.m_pKDWall->m_InSector;
    inSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[inSectorIdx]);

    outSectorIdx = m_Wall.m_pKDWall->m_OutSector;
    outSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[outSectorIdx]);

    minVertexColor = ((m_Settings.m_MaxColorInterpolationDist - minDist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
    maxVertexColor = ((m_Settings.m_MaxColorInterpolationDist - maxDist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
    minVertexColor = Clamp(minVertexColor, minColorClamp, maxColorRange);
    maxVertexColor = Clamp(maxVertexColor, minColorClamp, maxColorRange);

    if (outSectorIdx == -1 && whichSide > 0)
    {
        RenderHardWall(oGeneratedFlats);
    }
    else if (outSectorIdx != -1)
    {
        // Avoid occlusion failure
        if ((whichSide > 0 && outSector.m_Ceiling > inSector.m_Ceiling) ||
            (whichSide < 0 && outSector.m_Ceiling < inSector.m_Ceiling))
        {
            RenderSoftWallTop(oGeneratedFlats);
            RenderSoftWallBottom(oGeneratedFlats);
        }
        else
        {
            RenderSoftWallBottom(oGeneratedFlats);
            RenderSoftWallTop(oGeneratedFlats);
        }
    }
}

void WallRenderer::RenderHardWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    CType eyeToTop = inSector.m_Ceiling - m_State.m_PlayerZ;
    CType eyeToBottom = m_State.m_PlayerZ - inSector.m_Floor;

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

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToBottom) * m_Settings.m_VerticalDistortionCst;
    int minVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToTop) * m_Settings.m_VerticalDistortionCst;
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToBottom) * m_Settings.m_VerticalDistortionCst;
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToTop) * m_Settings.m_VerticalDistortionCst;

    KDRData::FlatSurface floorSurface;
    floorSurface.m_MinX = minX;
    floorSurface.m_MaxX = maxX;
    floorSurface.m_SectorIdx = inSectorIdx;
    floorSurface.m_Height = inSector.m_Floor;

    KDRData::FlatSurface ceilingSurface(floorSurface);
    ceilingSurface.m_Height = inSector.m_Ceiling;

    bool addFloorSurface = false;
    bool addCeilingSurface = false;

    CType t;
    int minY, maxY, minYUnclamped, maxYUnclamped;
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

            if (minY <= maxY)
                RenderColumn(t, minVertexColor, maxVertexColor, minY, maxY, x, r, g, b);
        }
    }
    // Nothing will be drawn behind this wall
    memset(m_pHorizOcclusionBuffer + minX, 1u, maxX - minX + 1);

    if (addFloorSurface)
        oGeneratedFlats.push_back(floorSurface);
    if (addCeilingSurface)
        oGeneratedFlats.push_back(ceilingSurface);
}

void WallRenderer::RenderSoftWallTop(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    CType eyeToTopCeiling = std::max(inSector.m_Ceiling, outSector.m_Ceiling) - m_State.m_PlayerZ;
    CType eyeToBottomCeiling = std::min(inSector.m_Ceiling, outSector.m_Ceiling) - m_State.m_PlayerZ;

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

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToBottomCeiling) * m_Settings.m_VerticalDistortionCst;
    int minVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(minAngleEyeToTopCeiling) * m_Settings.m_VerticalDistortionCst;
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToBottomCeiling) * m_Settings.m_VerticalDistortionCst;
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 + WINDOW_HEIGHT * tanInt(maxAngleEyeToTopCeiling) * m_Settings.m_VerticalDistortionCst;

    KDRData::FlatSurface ceilingSurface;
    ceilingSurface.m_MinX = minX;
    ceilingSurface.m_MaxX = maxX;
    ceilingSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
    ceilingSurface.m_Height = whichSide > 0 ? inSector.m_Ceiling : outSector.m_Ceiling;

    bool wallIsVisible = (whichSide > 0 && inSector.m_Ceiling > outSector.m_Ceiling) ||
                         (whichSide < 0 && outSector.m_Ceiling > inSector.m_Ceiling);

    bool addCeilingSurface = false;

    CType t;
    int minY, maxY, minYUnclamped, maxYUnclamped;
    for (unsigned int x = minX; x <= maxX; x++)
    {
        // TODO: optimize divisions
        if (!m_pHorizOcclusionBuffer[x])
        {
            ComputeRenderParameters(x, minX, maxX, InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

            if (wallIsVisible)
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

    if (addCeilingSurface)
        oGeneratedFlats.push_back(ceilingSurface);
}

void WallRenderer::RenderSoftWallBottom(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    CType eyeToTopFloor = m_State.m_PlayerZ - std::max(inSector.m_Floor, outSector.m_Floor);
    CType eyeToBottomFloor = m_State.m_PlayerZ - std::min(inSector.m_Floor, outSector.m_Floor);

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

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToBottomFloor) * m_Settings.m_VerticalDistortionCst;
    int minVertexTopPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(minAngleEyeToTopFloor) * m_Settings.m_VerticalDistortionCst;
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToBottomFloor) * m_Settings.m_VerticalDistortionCst;
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 - WINDOW_HEIGHT * tanInt(maxAngleEyeToTopFloor) * m_Settings.m_VerticalDistortionCst;

    KDRData::FlatSurface floorSurface;
    floorSurface.m_MinX = minX;
    floorSurface.m_MaxX = maxX;
    floorSurface.m_SectorIdx = whichSide > 0 ? inSectorIdx : outSectorIdx;
    floorSurface.m_Height = whichSide > 0 ? inSector.m_Floor : outSector.m_Floor;

    bool wallIsVisible = (whichSide > 0 && inSector.m_Floor < outSector.m_Floor) ||
                         (whichSide < 0 && outSector.m_Floor < inSector.m_Floor);
    bool addFloorSurface = false;

    CType t;
    int minY, maxY, minYUnclamped, maxYUnclamped;
    for (unsigned int x = minX; x <= maxX; x++)
    {
        // TODO: optimize divisions
        if (!m_pHorizOcclusionBuffer[x])
        {
            ComputeRenderParameters(x, minX, maxX, InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, t, minY, maxY, minYUnclamped, maxYUnclamped);

            if (wallIsVisible)
                floorSurface.m_MaxY[x] = std::min(minYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
            else
                floorSurface.m_MaxY[x] = std::min(maxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
            floorSurface.m_MinY[x] = m_pBottomOcclusionBuffer[x];

            if (!addFloorSurface && floorSurface.m_MinY[x] < floorSurface.m_MaxY[x])
                addFloorSurface = true;

            // We need to fill the occlusion buffer even if we don't draw there, since it will be
            // used for floor and ceiling surfaces
            m_pBottomOcclusionBuffer[x] = std::max(m_pBottomOcclusionBuffer[x], maxY);
            if (minY <= maxY && wallIsVisible)
                RenderColumn(t, minVertexColor, maxVertexColor, minY, maxY, x, r, g, b);
        }
    }

    if (addFloorSurface)
        oGeneratedFlats.push_back(floorSurface);
}

bool WallRenderer::isInsideFrustum(const KDRData::Vertex &iVertex) const
{
    return (WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToLeft, iVertex) >= 0) &&
           (WhichSide(m_State.m_PlayerPosition, m_State.m_FrustumToRight, iVertex) <= 0);
}