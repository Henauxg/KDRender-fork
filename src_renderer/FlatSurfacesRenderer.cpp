#include "FlatSurfacesRenderer.h"

#include "GeomUtils.h"

FlatSurfacesRenderer::FlatSurfacesRenderer(const std::map<CType, std::vector<KDRData::FlatSurface>> &iFlatSurfaces, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap):
    m_FlatSurfaces(iFlatSurfaces),
    m_State(iState),
    m_Settings(iSettings),
    m_Map(iMap)
{
}

FlatSurfacesRenderer::~FlatSurfacesRenderer()
{
}

void FlatSurfacesRenderer::SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer)
{
    m_pFrameBuffer = ipFrameBuffer;
    m_pHorizOcclusionBuffer = ipHorizOcclusionBuffer;
    m_pTopOcclusionBuffer = ipTopOcclusionBuffer;
    m_pBottomOcclusionBuffer = ipBottomOcclusionBuffer;
}

void FlatSurfacesRenderer::RenderLegacy()
{
    int vFovRad = ARITHMETIC_SHIFT(m_Settings.m_PlayerVerticalFOV, ANGLE_SHIFT) * (314 << FP_SHIFT) / (180 * 100);

    for (const auto &keyVal : m_FlatSurfaces)
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

// #include <iostream>
void FlatSurfacesRenderer::Render()
{
    // {
    //     unsigned int totalSize = 0;
    //     for (const auto &k : m_FlatSurfaces)
    //         totalSize += k.second.size();
    //     std::cout << "Number of flat surfaces = " << totalSize << std::endl;
    // }

    for (unsigned int i = 0; i < WINDOW_HEIGHT; i++)
        m_LinesXStart[i] = -1;

    unsigned count = 0;
    for (const auto &keyVal : m_FlatSurfaces)
    {
        const CType currentHeight = keyVal.first;
        const std::vector<KDRData::FlatSurface> &currentSurfaces = keyVal.second;

        for (unsigned int i = 0; i < WINDOW_HEIGHT; i++)
            m_DistYCache[i] = -1;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            m_RDbg = count % 3 == 0 ? 160 : 0;
            m_GDbg = count % 3 == 1 ? 160 : 0;
            m_BDbg = count % 3 == 2 ? 160 : 0;
            count++;

            // TODO textures
            m_CurrSectorR = 255;
            m_CurrSectorG = 255;
            m_CurrSectorB = 255;
            
            if(currentSurfaces[i].m_TexId != -1)
            {
                unsigned int hIdx = m_Map.m_Textures[currentSurfaces[i].m_TexId].m_Height;
                unsigned int wIdx = m_Map.m_Textures[currentSurfaces[i].m_TexId].m_Width;

                m_CurrSectorR = m_Map.m_Textures[currentSurfaces[i].m_TexId].m_pData[(1u << (hIdx + wIdx + 1u)) + 0];
                m_CurrSectorG = m_Map.m_Textures[currentSurfaces[i].m_TexId].m_pData[(1u << (hIdx + wIdx + 1u)) + 1];
                m_CurrSectorB = m_Map.m_Textures[currentSurfaces[i].m_TexId].m_pData[(1u << (hIdx + wIdx + 1u)) + 2];
            }

            const KDRData::FlatSurface &currentSurface = currentSurfaces[i];

            // Jump to start of the drawable part of the surface
            int minXDrawable = currentSurface.m_MinX;
            for (; currentSurface.m_MinY[minXDrawable] > currentSurface.m_MaxY[minXDrawable]; minXDrawable++);
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
                        DrawLine(y, m_LinesXStart[y], x - 1, currentSurface);
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
                            DrawLine(y, m_LinesXStart[y], x - 1, currentSurface);
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
                            DrawLine(y, m_LinesXStart[y], x - 1, currentSurface);
                        }
                    }
                }
            }

            for (int y = currentSurface.m_MinY[maxXDrawable]; y <= currentSurface.m_MaxY[maxXDrawable]; y++)
            {
                DrawLine(y, m_LinesXStart[y], maxXDrawable, currentSurface);
            }
        }
    }
}

void FlatSurfacesRenderer::DrawLine(int iY, int iMinX, int iMaxX, const KDRData::FlatSurface &iSurface)
{
    if(iY == WINDOW_HEIGHT / 2)
        return;

    int maxColorRange = 160;

    CType dist = -1;
    CType deltaTexelX, deltaTexelY;
    KDRData::Vertex leftmostTexel;
    int light;

    if (m_DistYCache[iY] >= 0)
    {
        dist = m_DistYCache[iY];
        leftmostTexel = m_LeftmostTexelCache[iY];
        deltaTexelX = m_DeltaTexelXCache[iY];
        deltaTexelY = m_DeltaTexelYCache[iY];
    }
    else
    {
        dist = m_Settings.m_VerticalDistortionCst * ((m_State.m_PlayerZ - iSurface.m_Height) / (CType(iY) / WINDOW_HEIGHT - CType(1) / CType(2)));
        dist = dist < 0 ? -dist : dist;
        m_DistYCache[iY] = dist;

        const KDMapData::Texture &texture = m_Map.m_Textures[iSurface.m_TexId];
        CType length = dist / cosInt(m_Settings.m_PlayerHorizontalFOV / 2);

        leftmostTexel.m_X = (m_State.m_FrustumToLeft.m_X - m_State.m_PlayerPosition.m_X) * length + m_State.m_PlayerPosition.m_X;
        leftmostTexel.m_Y = (m_State.m_FrustumToLeft.m_Y - m_State.m_PlayerPosition.m_Y) * length + m_State.m_PlayerPosition.m_Y;

        leftmostTexel.m_X = leftmostTexel.m_X * CType(int(1u << texture.m_Width)) * CType(POSITION_SCALE) / CType(TEXEL_SCALE);
        leftmostTexel.m_Y = leftmostTexel.m_Y * CType(int(1u << texture.m_Height)) * CType(POSITION_SCALE) / CType(TEXEL_SCALE);

        m_LeftmostTexelCache[iY] = leftmostTexel;

        KDRData::Vertex rightmostTexel;
        rightmostTexel.m_X = (m_State.m_FrustumToRight.m_X - m_State.m_PlayerPosition.m_X) * length + m_State.m_PlayerPosition.m_X;
        rightmostTexel.m_Y = (m_State.m_FrustumToRight.m_Y - m_State.m_PlayerPosition.m_Y) * length + m_State.m_PlayerPosition.m_Y;

        rightmostTexel.m_X = rightmostTexel.m_X * CType(int(1u << texture.m_Width)) * CType(POSITION_SCALE) / CType(TEXEL_SCALE);
        rightmostTexel.m_Y = rightmostTexel.m_Y * CType(int(1u << texture.m_Width)) * CType(POSITION_SCALE) / CType(TEXEL_SCALE);

        deltaTexelX = (rightmostTexel.m_X - leftmostTexel.m_X) / CType(WINDOW_WIDTH);
        deltaTexelY = (rightmostTexel.m_Y - leftmostTexel.m_Y) / CType(WINDOW_WIDTH);

        m_DeltaTexelXCache[iY] = deltaTexelX;
        m_DeltaTexelYCache[iY] = deltaTexelY;
    }

    if (dist >= 0)
    {
        light = ((m_Settings.m_MaxColorInterpolationDist - dist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
        light = Clamp(light, 45, maxColorRange);

        if (iSurface.m_TexId >= 0)
        {
            const KDMapData::Texture &texture = m_Map.m_Textures[iSurface.m_TexId];

            CType currTexelX = leftmostTexel.m_X + CType(iMinX) * deltaTexelX;
            CType currTexelY = leftmostTexel.m_Y + CType(iMinX) * deltaTexelY;

            unsigned int yModShift = texture.m_Height + FP_SHIFT;
            unsigned int xModShift = texture.m_Width + FP_SHIFT;
            int currTexelXClamped, currTexelYClamped;
            unsigned int r, g, b;
            for (int x = iMinX; x <= iMaxX; x++)
            {
                currTexelXClamped = currTexelX - ((currTexelX >> (xModShift)) << (xModShift));
                currTexelYClamped = currTexelY - ((currTexelY >> (yModShift)) << (yModShift));

                // (iTexelXClamped * (1u << m_pTexture->m_Height)) << 2u
                r = texture.m_pData[((currTexelXClamped * (1u << texture.m_Height)) << 2u) + (currTexelYClamped << 2u) + 0];
                g = texture.m_pData[((currTexelXClamped * (1u << texture.m_Height)) << 2u) + (currTexelYClamped << 2u) + 1];
                b = texture.m_pData[((currTexelXClamped * (1u << texture.m_Height)) << 2u) + (currTexelYClamped << 2u) + 2];

                // r = m_RDbg == 0 ? r : m_RDbg;
                // g = m_GDbg == 0 ? g : m_GDbg;
                // b = m_BDbg == 0 ? b : m_BDbg;

                WriteFrameBuffer((WINDOW_HEIGHT - 1 - iY) * WINDOW_WIDTH + x, (light * r) >> 8u, (light * g) >> 8u, (light * b) >> 8u);

                currTexelX = currTexelX + deltaTexelX;
                currTexelY = currTexelY + deltaTexelY;
            }
        }
        else
        {
            for (int x = iMinX; x <= iMaxX; x++)
                WriteFrameBuffer((WINDOW_HEIGHT - 1 - iY) * WINDOW_WIDTH + x, (light * m_CurrSectorR) >> 8u, (light * m_CurrSectorG) >> 8u, (light * m_CurrSectorB) >> 8u);
        }
    }
}