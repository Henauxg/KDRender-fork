#include "WallRenderer.h"
#include <algorithm>
#include <cstring>

WallRenderer::WallRenderer(KDRData::Wall &iWall, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap):
    m_Wall(iWall),
    m_State(iState),
    m_Settings(iSettings),
    m_Map(iMap),
    m_pTexture(nullptr),
    m_TexUOffset(0),
    m_TexVOffset(0)
{
    // For debug purposes
    if(m_Wall.m_pKDWall->m_TexId == 0)
    {
        r = 0;
        g = 1;
        b = 1;
    }
    else if(m_Wall.m_pKDWall->m_TexId == 1)
    {
        r = 1;
        g = 0;
        b = 1;
    }
    else
    {
        r = 1; // iWall.m_pKDWall->m_InSector % 3 == 0 ? 1 : 0;
        g = 1; // iWall.m_pKDWall->m_InSector % 3 == 1 ? 1 : 0;
        b = 1; // iWall.m_pKDWall->m_InSector % 3 == 2 ? 1 : 0;
    }

    if(m_Wall.m_pKDWall->m_TexId != -1)
    {
        m_pTexture = &m_Map.m_Textures[m_Wall.m_pKDWall->m_TexId];
        m_TexUOffset = m_Wall.m_pKDWall->m_TexUOffset;
        m_TexVOffset = m_Wall.m_pKDWall->m_TexVOffset;
    }
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
    m_MinAngle = m_Settings.m_PlayerHorizontalFOV / 2;
    m_MaxAngle = -m_Settings.m_PlayerHorizontalFOV / 2;

    bool vertexFromInsideFrustum = isInsideFrustum(m_Wall.m_VertexFrom);
    bool vertexToInsideFrustum = isInsideFrustum(m_Wall.m_VertexTo);

    if (!vertexFromInsideFrustum || !vertexToInsideFrustum)
    {
        KDRData::Vertex intersectionVertex;
        if (HalfLineSegmentIntersection<KDRData::Vertex>(m_State.m_PlayerPosition, m_State.m_FrustumToLeft, m_Wall.m_VertexFrom, m_Wall.m_VertexTo, intersectionVertex))
        {
            // Avoid losing precision
            m_MinVertex = m_Wall.m_VertexFrom;
            if (m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X)
                m_MinVertex.m_Y = intersectionVertex.m_Y;
            else
                m_MinVertex.m_X = intersectionVertex.m_X;

            m_MinAngle = -m_Settings.m_PlayerHorizontalFOV / 2;
        }

        if (HalfLineSegmentIntersection<KDRData::Vertex>(m_State.m_PlayerPosition, m_State.m_FrustumToRight, m_Wall.m_VertexFrom, m_Wall.m_VertexTo, intersectionVertex))
        {
            m_MaxVertex = m_Wall.m_VertexFrom;
            // Avoid losing precision
            if (m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X)
                m_MaxVertex.m_Y = intersectionVertex.m_Y;
            else
                m_MaxVertex.m_X = intersectionVertex.m_X;

            m_MaxAngle = m_Settings.m_PlayerHorizontalFOV / 2;
        }
    }

    auto updateMinMaxAnglesAndVertices = [&](int iAngle, const KDRData::Vertex &iVertex) {
        if (iAngle < m_MinAngle)
        {
            m_MinAngle = iAngle;
            m_MinVertex = iVertex;
        }
        if (iAngle > m_MaxAngle)
        {
            m_MaxAngle = iAngle;
            m_MaxVertex = iVertex;
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
    if (m_MinAngle <= m_MaxAngle)
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
    m_MinX = WINDOW_WIDTH / 2 + MultiplyIntFpToInt(WINDOW_WIDTH, tanInt(m_MinAngle) * m_Settings.m_HorizontalDistortionCst);
    m_maxX = WINDOW_WIDTH / 2 + MultiplyIntFpToInt(WINDOW_WIDTH, tanInt(m_MaxAngle) * m_Settings.m_HorizontalDistortionCst) - 1;

    if (m_MinX >= m_maxX)
        return;

    m_MinX = Clamp(m_MinX, 0, WINDOW_WIDTH - 1);
    m_maxX = Clamp(m_maxX, 0, WINDOW_WIDTH - 1);

    // We need further precision for this ratio
    m_InvMinMaxXRange = m_maxX == m_MinX ? static_cast<CType>(0) : (1 << 7u) / CType(m_maxX - m_MinX);

    // Texture U coordinate range calculation
    if(m_pTexture)
    {
        if (m_MinVertex.m_Y == m_MaxVertex.m_Y)
        {
            m_MinTexelX = ((m_MinVertex.m_X + m_TexUOffset) * CType(int(1u << m_pTexture->m_Width)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
            m_MaxTexelX = ((m_MaxVertex.m_X + m_TexUOffset) * CType(int(1u << m_pTexture->m_Width)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
        }
        else
        {
            m_MinTexelX = ((m_MinVertex.m_Y + m_TexUOffset) * CType(int(1u << m_pTexture->m_Width)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
            m_MaxTexelX = ((m_MaxVertex.m_Y + m_TexUOffset) * CType(int(1u << m_pTexture->m_Width)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
        }
    }
    else
    {
        // Useless
        m_MinTexelX = 0;
        m_MaxTexelX = 1;
    }

    m_MinDist = DistInt(m_State.m_PlayerPosition, m_MinVertex) * cosInt(m_MinAngle); // Correction for vertical distortion
    m_MaxDist = DistInt(m_State.m_PlayerPosition, m_MaxVertex) * cosInt(m_MaxAngle); // Correction for vertical distortion

    // TODO: perform actual clipping
    // Dirty hack
    if (m_MaxDist <= 0)
        return;
    m_MinDist = m_MinDist <= CType(0) ? CType(1) : m_MinDist;

    m_MaxColorRange = m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X ? 230 : 180;
    m_MinColorRange = m_Wall.m_VertexFrom.m_X == m_Wall.m_VertexTo.m_X ? 55 : 50;

    m_WhichSide = WhichSide(m_Wall.m_VertexFrom, m_Wall.m_VertexTo, m_State.m_PlayerPosition);

    m_InSectorIdx = m_Wall.m_pKDWall->m_InSector;
    m_InSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[m_InSectorIdx]);

    m_OutSectorIdx = m_Wall.m_pKDWall->m_OutSector;
    m_OutSector = KDRData::GetSectorFromKDSector(m_Map.m_Sectors[m_OutSectorIdx]);

    m_MinVertexColor = ((m_Settings.m_MaxColorInterpolationDist - m_MinDist) * m_MaxColorRange) / m_Settings.m_MaxColorInterpolationDist;
    m_MaxVertexColor = ((m_Settings.m_MaxColorInterpolationDist - m_MaxDist) * m_MaxColorRange) / m_Settings.m_MaxColorInterpolationDist;
    m_MinVertexColor = Clamp(m_MinVertexColor, m_MinColorRange, m_MaxColorRange);
    m_MaxVertexColor = Clamp(m_MaxVertexColor, m_MinColorRange, m_MaxColorRange);

    if(m_pTexture)
        m_YModShift = m_pTexture->m_Height + FP_SHIFT;

    if (m_OutSectorIdx == -1 && m_WhichSide > 0)
    {
        RenderHardWall(oGeneratedFlats);
    }
    else if (m_OutSectorIdx != -1)
    {
        // Avoid occlusion failure
        if ((m_WhichSide > 0 && m_OutSector.m_Ceiling > m_InSector.m_Ceiling) ||
            (m_WhichSide < 0 && m_OutSector.m_Ceiling < m_InSector.m_Ceiling))
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
    CType eyeToTop = m_InSector.m_Ceiling - m_State.m_PlayerZ;
    CType eyeToBottom = m_State.m_PlayerZ - m_InSector.m_Floor;

    // Same here, need to correct for distortion
    // I'm leaving the old formulas as comments since they are much more intuitive

    // Without vertical distortion
    // int minVertexBottomPixel = ((-atanInt(eyeToBottom / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int minVertexTopPixel = ((atanInt(eyeToTop / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexBottomPixel = ((-atanInt(eyeToBottom / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexTopPixel = ((atanInt(eyeToTop / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottom / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int minVertexTopPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTop / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottom / m_MaxDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTop / m_MaxDist) * m_Settings.m_VerticalDistortionCst));

    KDRData::FlatSurface floorSurface;
    floorSurface.m_MinX = m_MinX;
    floorSurface.m_MaxX = m_maxX;
    floorSurface.m_SectorIdx = m_InSectorIdx;
    floorSurface.m_TexId = m_InSector.m_pKDSector->floorTexId;
    floorSurface.m_Height = m_InSector.m_Floor;

    KDRData::FlatSurface ceilingSurface(floorSurface);
    ceilingSurface.m_TexId = m_InSector.m_pKDSector->ceilingTexId;
    ceilingSurface.m_Height = m_InSector.m_Ceiling;

    bool addFloorSurface = false;
    bool addCeilingSurface = false;

    CType t, minTexelY, maxTexelY;
    int minY, maxY, minYUnclamped, maxYUnclamped;
    int texelXClamped;
    for (int x = m_MinX; x <= m_maxX; x++)
    {
        // TODO: optimize divisions
        if (!m_pHorizOcclusionBuffer[x])
        {
            ComputeRenderParameters(x, m_MinX, m_maxX, m_InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, m_InSector.m_Floor, m_InSector.m_Ceiling, t, minY, maxY, minYUnclamped, maxYUnclamped, texelXClamped, minTexelY, maxTexelY);

            floorSurface.m_MaxY[x] = std::min(minYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);
            floorSurface.m_MinY[x] = m_pBottomOcclusionBuffer[x];
            if (!addFloorSurface && floorSurface.m_MinY[x] < floorSurface.m_MaxY[x])
                addFloorSurface = true;

            ceilingSurface.m_MinY[x] = std::max(maxYUnclamped, m_pBottomOcclusionBuffer[x]);
            ceilingSurface.m_MaxY[x] = WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x];
            if (!addCeilingSurface && ceilingSurface.m_MinY[x] < ceilingSurface.m_MaxY[x])
                addCeilingSurface = true;

            if (minY <= maxY)
            {
                if(m_pTexture)
                    RenderColumnWithTexture(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, texelXClamped, minTexelY, maxTexelY);
                else
                    RenderColumn(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, r, g, b);
            }
        }
    }
    // Nothing will be drawn behind this wall
    memset(m_pHorizOcclusionBuffer + m_MinX, 1u, m_maxX - m_MinX + 1);

    if (addFloorSurface)
        oGeneratedFlats.push_back(floorSurface);
    if (addCeilingSurface)
        oGeneratedFlats.push_back(ceilingSurface);
}

void WallRenderer::RenderSoftWallTop(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    CType topCeiling = std::max(m_InSector.m_Ceiling, m_OutSector.m_Ceiling);
    CType bottomCeiling = std::min(m_InSector.m_Ceiling, m_OutSector.m_Ceiling);
    CType eyeToTopCeiling = topCeiling - m_State.m_PlayerZ;
    CType eyeToBottomCeiling = bottomCeiling - m_State.m_PlayerZ;

    // Same here, need to correct for distortion
    // I'm leaving the old formulas as comments since they are much more intuitive

    // int minVertexBottomPixel = ((atanInt(eyeToBottomCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int minVertexTopPixel = ((atanInt(eyeToTopCeiling / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexBottomPixel = ((atanInt(eyeToBottomCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexTopPixel = ((atanInt(eyeToTopCeiling / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottomCeiling / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int minVertexTopPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTopCeiling / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottomCeiling / m_MaxDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 + MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTopCeiling / m_MaxDist) * m_Settings.m_VerticalDistortionCst));

    KDRData::FlatSurface ceilingSurface;
    ceilingSurface.m_MinX = m_MinX;
    ceilingSurface.m_MaxX = m_maxX;
    ceilingSurface.m_SectorIdx = m_WhichSide > 0 ? m_InSectorIdx : m_OutSectorIdx;
    ceilingSurface.m_TexId = m_WhichSide > 0 ? m_InSector.m_pKDSector->ceilingTexId : m_OutSector.m_pKDSector->ceilingTexId;
    ceilingSurface.m_Height = m_WhichSide > 0 ? m_InSector.m_Ceiling : m_OutSector.m_Ceiling;

    bool wallIsVisible = (m_WhichSide > 0 && m_InSector.m_Ceiling > m_OutSector.m_Ceiling) ||
                         (m_WhichSide < 0 && m_OutSector.m_Ceiling > m_InSector.m_Ceiling);

    bool addCeilingSurface = false;

    CType t, minTexelY, maxTexelY;
    int minY, maxY, minYUnclamped, maxYUnclamped;
    int texelXClamped;
    for (unsigned int x = m_MinX; x <= m_maxX; x++)
    {
        // TODO: optimize divisions
        if (!m_pHorizOcclusionBuffer[x])
        {
            ComputeRenderParameters(x, m_MinX, m_maxX, m_InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, bottomCeiling, topCeiling, t, minY, maxY, minYUnclamped, maxYUnclamped, texelXClamped, minTexelY, maxTexelY);

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
            {
                if (m_pTexture)
                    RenderColumnWithTexture(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, texelXClamped, minTexelY, maxTexelY);
                else
                    RenderColumn(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, r, g, b);
            }
        }
    }

    if (addCeilingSurface)
        oGeneratedFlats.push_back(ceilingSurface);
}

void WallRenderer::RenderSoftWallBottom(std::vector<KDRData::FlatSurface> &oGeneratedFlats)
{
    CType topFloor = std::max(m_InSector.m_Floor, m_OutSector.m_Floor);
    CType bottomFloor = std::min(m_InSector.m_Floor, m_OutSector.m_Floor);
    CType eyeToTopFloor = m_State.m_PlayerZ - topFloor;
    CType eyeToBottomFloor = m_State.m_PlayerZ - bottomFloor;

    // Same here, need to correct for distortion
    // I'm leaving the old formulas as comments since they are much more intuitive

    // Without vertical distortion
    // int minVertexBottomPixel = ((-atanInt(eyeToBottomFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int minVertexTopPixel = ((-atanInt(eyeToTopFloor / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexBottomPixel = ((-atanInt(eyeToBottomFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
    // int maxVertexTopPixel = ((-atanInt(eyeToTopFloor / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;

    int minVertexBottomPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottomFloor / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int minVertexTopPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTopFloor / m_MinDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexBottomPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToBottomFloor / m_MaxDist) * m_Settings.m_VerticalDistortionCst));
    int maxVertexTopPixel = WINDOW_HEIGHT / 2 - MultiplyIntFpToInt(WINDOW_HEIGHT, ((eyeToTopFloor / m_MaxDist) * m_Settings.m_VerticalDistortionCst));

    KDRData::FlatSurface floorSurface;
    floorSurface.m_MinX = m_MinX;
    floorSurface.m_MaxX = m_maxX;
    floorSurface.m_SectorIdx = m_WhichSide > 0 ? m_InSectorIdx : m_OutSectorIdx;
    floorSurface.m_TexId = m_WhichSide > 0 ? m_InSector.m_pKDSector->floorTexId : m_OutSector.m_pKDSector->floorTexId;
    floorSurface.m_Height = m_WhichSide > 0 ? m_InSector.m_Floor : m_OutSector.m_Floor;

    bool wallIsVisible = (m_WhichSide > 0 && m_InSector.m_Floor < m_OutSector.m_Floor) ||
                         (m_WhichSide < 0 && m_OutSector.m_Floor < m_InSector.m_Floor);
    bool addFloorSurface = false;

    CType t, minTexelY, maxTexelY;
    int minY, maxY, minYUnclamped, maxYUnclamped;
    int texelXClamped;
    for (unsigned int x = m_MinX; x <= m_maxX; x++)
    {
        // TODO: optimize divisions
        if (!m_pHorizOcclusionBuffer[x])
        {
            ComputeRenderParameters(x, m_MinX, m_maxX, m_InvMinMaxXRange, minVertexBottomPixel, maxVertexBottomPixel, minVertexTopPixel, maxVertexTopPixel, bottomFloor, topFloor, t, minY, maxY, minYUnclamped, maxYUnclamped, texelXClamped, minTexelY, maxTexelY);

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
            {
                if (m_pTexture)
                    RenderColumnWithTexture(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, texelXClamped, minTexelY, maxTexelY);
                else
                    RenderColumn(t, m_MinVertexColor, m_MaxVertexColor, minY, maxY, x, r, g, b);
            }
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