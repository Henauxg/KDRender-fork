#include "FlatSurfacesRenderer.h"

#include "GeomUtils.h"
#include "Light.h"

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
            m_PaletteCache[i] = -1;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            // Debug
            m_RDbg = count % 3 == 0 ? 160 : 0;
            m_GDbg = count % 3 == 1 ? 160 : 0;
            m_BDbg = count % 3 == 2 ? 160 : 0;
            count++;

            // TODO textures
            m_CurrSectorR = 255;
            m_CurrSectorG = 255;
            m_CurrSectorB = 255;

            m_SectorLightValue = m_Map.m_Sectors[currentSurfaces[i].m_SectorIdx].m_pLight->GetValue();
            m_MaxLight = m_SectorLightValue * 90 / 100;
            m_MinLight = LightTools::GetMinLight(m_MaxLight) * 90 / 100;
            m_MaxColorInterpolationDist = LightTools::GetMaxInterpolationDist(m_MaxLight);

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

    char palette;
    CType deltaTexelX, deltaTexelY;
    KDRData::Vertex leftmostTexel;
    int light;
    unsigned int lightClamped;

    if (m_PaletteCache[iY] >= 0)
    {
        palette = m_PaletteCache[iY];
        leftmostTexel = m_LeftmostTexelCache[iY];
        deltaTexelX = m_DeltaTexelXCache[iY];
        deltaTexelY = m_DeltaTexelYCache[iY];
    }
    else
    {
        CType den = (CType(iY) / WINDOW_HEIGHT - CType(1) / CType(2));
        if(!den)
            return;
        
        CType dist = m_Settings.m_VerticalDistortionCst * ((m_State.m_PlayerZ - iSurface.m_Height) / den);
        dist = dist < 0 ? -dist : dist;

        int light = ((m_MaxColorInterpolationDist - dist) * m_MaxLight) / m_MaxColorInterpolationDist;
        light = Clamp(light, m_MinLight, m_MaxLight);
        palette = light >> 4u;
        m_PaletteCache[iY] = palette;

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

    if (palette >= 0)
    {
        unsigned int xOffsetFrameBuffer = (WINDOW_HEIGHT - 1 - iY) * WINDOW_WIDTH + iMinX;
        // m_MinVertexColor = ((maxColorInterpolationDist - m_MinDist) * maxLightVal) / maxColorInterpolationDist;
        

        if (iSurface.m_TexId >= 0)
        {
            const uint32_t *pPalette = m_Map.m_DynamicColorPalettes[palette];
            const KDMapData::Texture &texture = m_Map.m_Textures[iSurface.m_TexId];

            CType currTexelX = leftmostTexel.m_X + CType(iMinX) * deltaTexelX;
            CType currTexelY = leftmostTexel.m_Y + CType(iMinX) * deltaTexelY;
            int currTexelXInt = currTexelX.GetRawValue();
            int currTexelYInt = currTexelY.GetRawValue();

            unsigned int yModShift = texture.m_Height + FP_SHIFT;
            unsigned int xModShift = texture.m_Width + FP_SHIFT;
            int currTexelXClamped, currTexelYClamped;
            unsigned texIdx;
            unsigned int r, g, b;
            const uint32_t *src;
            uint32_t *dest = reinterpret_cast<uint32_t*>(m_pFrameBuffer) + xOffsetFrameBuffer;
            for (unsigned int x = iMaxX - iMinX + 1; x; --x)
            {
                currTexelXClamped = currTexelX - ((currTexelX >> (xModShift)) << (xModShift));
                currTexelYClamped = currTexelY - ((currTexelY >> (yModShift)) << (yModShift));

                texIdx = (currTexelXClamped << texture.m_Height) + currTexelYClamped;
                src = &pPalette[texture.m_pData[texIdx]];
                *dest++ = *src;

                currTexelX = currTexelX + deltaTexelX;
                currTexelY = currTexelY + deltaTexelY;
            }
        }
    }
}