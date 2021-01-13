#include "FlatSurfacesRenderer.h"

#include "GeomUtils.h"

FlatSurfacesRenderer::FlatSurfacesRenderer(const std::unordered_map<int, std::vector<KDRData::FlatSurface>> &iFlatSurfaces, const KDRData::State &iState, const KDRData::Settings &iSettings):
    m_FlatSurfaces(iFlatSurfaces),
    m_State(iState),
    m_Settings(iSettings)
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

    for (const auto &keyVal : m_FlatSurfaces)
    {
        const CType currentHeight = keyVal.first;
        const std::vector<KDRData::FlatSurface> &currentSurfaces = keyVal.second;

        for (unsigned int i = 0; i < WINDOW_HEIGHT; i++)
            m_DistYCache[i] = -1;

        for (unsigned int i = 0; i < currentSurfaces.size(); i++)
        {
            const KDRData::FlatSurface &currentSurface = currentSurfaces[i];

            // Jump to start of the drawable part of the surface
            int minXDrawable = currentSurface.m_MinX;
            for (; currentSurface.m_MinY[minXDrawable] > currentSurface.m_MaxY[minXDrawable]; minXDrawable++)
                ;
            int maxXDrawable = currentSurface.m_MaxX;
            for (; currentSurface.m_MinY[maxXDrawable] > currentSurface.m_MaxY[maxXDrawable]; maxXDrawable--)
                ;

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
                    for (; x + 1 <= maxXDrawable && currentSurface.m_MinY[x + 1] > currentSurface.m_MaxY[x + 1]; x++)
                        ;

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
    int maxColorRange = 150;

    char r = iSurface.m_SectorIdx % 3 == 0 ? 1 : 0;
    char g = iSurface.m_SectorIdx % 3 == 1 ? 1 : 0;
    char b = iSurface.m_SectorIdx % 3 == 2 ? 1 : 0;

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
            dist = (m_State.m_PlayerZ - iSurface.m_Height) / sinTheta;
            dist = dist < 0 ? -dist : dist;
            dist = std::min(dist, m_Settings.m_MaxColorInterpolationDist);
            m_DistYCache[iY] = dist;
        }
    }

    if (dist >= 0)
    {
        color = ((m_Settings.m_MaxColorInterpolationDist - dist) * maxColorRange) / m_Settings.m_MaxColorInterpolationDist;
        color = std::max(45, color);

        for (int x = iMinX; x <= iMaxX; x++)
        {
            WriteFrameBuffer((WINDOW_HEIGHT - 1 - iY) * WINDOW_WIDTH + x, color * r, color * g, color * b);
        }
    }
}