#ifndef WallRenderer_h
#define WallRenderer_h

#include "KDTreeRendererData.h"

#include <vector>

class WallRenderer
{
public:
    WallRenderer(KDRData::Wall &iWall, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap);
    virtual ~WallRenderer();

public:
    void SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer);
    void Render(std::vector<KDRData::FlatSurface> &oGeneratedFlats);

protected:
    void RenderWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderHardWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderSoftWallTop(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderSoftWallBottom(std::vector<KDRData::FlatSurface> &oGeneratedFlats);

protected:
    bool isInsideFrustum(const KDRData::Vertex &iVertex) const;
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);
    inline void ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
                                        int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                        int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                        CType &oT, int &oMinY, int &oMaxY,
                                        int &oMinYUnclamped, int &oMaxYUnclamped) const;
    inline void RenderColumn(CType iT, int iMinVertexColor, int iMaxVertexColor,
                             int iMinY, int iMaxY, int iX,
                             int iR, int iG, int iB);

protected:
    const KDRData::Wall &m_Wall;
    const KDRData::State &m_State;
    const KDRData::Settings &m_Settings;
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;

protected:
    // Intermediate computations results
    KDRData::Vertex m_MinVertex;
    KDRData::Vertex m_MaxVertex;

    int m_MinAngle;
    int m_MaxAngle;

    int m_MinX;
    int m_maxX;
    CType m_InvMinMaxXRange;

    CType m_MinDist;
    CType m_MaxDist;

    int m_MaxColorRange;
    int m_MinColorClamp;
    int m_MinVertexColor;
    int m_MaxVertexColor;

    int m_WhichSide;

    int m_InSectorIdx;
    KDRData::Sector m_InSector;
    int m_OutSectorIdx;
    KDRData::Sector m_OutSector;

protected:
    // Debug only
    char r, g, b;
};

void WallRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

void WallRenderer::ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
                                           int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                           int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                           CType &oT, int &oMinY, int &oMaxY,
                                           int &oMinYUnclamped, int &oMaxYUnclamped) const
{
    // For extra precision, the inverse range has been shifted. Need to shift it back
    // oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) >> 7u;
    oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) / (1 << 7u); // For float/double
    oMinYUnclamped = ((1 - oT) * iMinVertexBottomPixel + oT * iMaxVertexBottomPixel);
    oMaxYUnclamped = ((1 - oT) * iMinVertexTopPixel + oT * iMaxVertexTopPixel);
    oMinY = std::max<int>(oMinYUnclamped, m_pBottomOcclusionBuffer[iX]);
    oMaxY = std::min<int>(oMaxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[iX]);
}

void WallRenderer::RenderColumn(CType iT, int iMinVertexColor, int iMaxVertexColor,
                                int iMinY, int iMaxY, int iX,
                                int iR, int iG, int iB)
{
    int color = (iMinVertexColor * (1 - iT)) + iT * iMaxVertexColor;
    // int color = 255 * iT;
    for (unsigned int y = iMinY; y <= iMaxY; y++)
    {
        WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + iX, color * iR, color * iG, color * iB);
    }
}

#endif
