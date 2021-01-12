#ifndef KDTreeRenderer_h
#define KDTreeRenderer_h

#include "KDTreeRendererData.h"
#include "KDTreeMap.h"
#include "Consts.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <cstring>
#include <algorithm>

class KDTreeRenderer
{
public:
    KDTreeRenderer(const KDTreeMap &iMap);

public:
    const unsigned char* GetFrameBuffer() const;
    unsigned char *GetFrameBuffer();

    void RefreshFrameBuffer();
    void ClearBuffers();

    void SetPlayerCoordinates(const KDRData::Vertex &iPosition, int iDirection);

    KDRData::Vertex GetPlayerPosition() const;
    int GetPlayerDirection() const;

    KDRData::Vertex GetLook() const;

protected:
    void FillFrameBufferWithColor(unsigned char r, unsigned char g, unsigned char b);
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

protected:
    CType ComputeZ();
    CType RecursiveComputeZ(KDTreeNode *pNode);

    void Render();
    void RenderNode(KDTreeNode *pNode);
    void RenderWall(const KDRData::Wall &iWall, const KDRData::Vertex &iMinVertex, const KDRData::Vertex &iMaxVertex, int iMinAngle, int iMaxAngle);
    inline void ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
                                        int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                        int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                        CType &oT, int &oMinY, int &oMaxY,
                                        int &oMinYUnclamped, int &oMaxYUnclamped) const;
    inline void RenderColumn(CType iT, int iMinVertexColor, int iMaxVertexColor,
                             int iMinY, int iMaxY, int iX,
                             int iR, int iG, int iB);
    bool AddFlatSurface(const KDRData::FlatSurface &iFlatSurface);
    void RenderFlatSurfacesLegacy();
    void RenderFlatSurfaces();

protected:
    bool isInsideFrustum(const KDRData::Vertex &iVertex) const;

protected:
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char m_pHorizOcclusionBuffer[WINDOW_WIDTH];
    int m_pTopOcclusionBuffer[WINDOW_WIDTH];
    int m_pBottomOcclusionBuffer[WINDOW_WIDTH];
    
    // Data used to render floors and ceilings
    std::unordered_map<int, std::vector<KDRData::FlatSurface>> m_FlatSurfaces; // Flat surfaces are stored int the map according to their height
    int m_LinesXStart[WINDOW_HEIGHT];
    CType m_DistYCache[WINDOW_HEIGHT];

    KDRData::Vertex m_PlayerPosition;
    CType m_PlayerZ;
    int m_PlayerDirection;
    KDRData::Vertex m_FrustumToLeft;
    KDRData::Vertex m_FrustumToRight;
    KDRData::Vertex m_Look;

    const int m_PlayerHorizontalFOV;
    const int m_PlayerVerticalFOV;
    const CType m_PlayerHeight;
    const CType m_MaxColorInterpolationDist;
    const CType m_HorizontalDistortionCst;
    const CType m_VerticalDistortionCst;
};

void KDTreeRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

void KDTreeRenderer::ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
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

void KDTreeRenderer::RenderColumn(CType iT, int iMinVertexColor, int iMaxVertexColor,
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