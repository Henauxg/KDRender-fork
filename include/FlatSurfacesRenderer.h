#ifndef FlatSurfacesRenderer_h
#define FlatSurfacesRenderer_h

#include "KDTreeRendererData.h"

#include <vector>
#include <map>

class FlatSurfacesRenderer
{
public:
    FlatSurfacesRenderer(const std::map<CType, std::vector<KDRData::FlatSurface>> &iFlatSurfaces, const KDRData::State &iState, const KDRData::Settings &iSettings);
    virtual ~FlatSurfacesRenderer();

public:
    void SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer);

public:
    void Render();
    void RenderLegacy();

protected:
    void DrawLine(int iY, int iMinX, int iMaxX, const KDRData::FlatSurface &iSurface);
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

protected:
    const std::map<CType, std::vector<KDRData::FlatSurface>> &m_FlatSurfaces;
    const KDRData::State &m_State;
    const KDRData::Settings &m_Settings;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;

// Cache
    int m_LinesXStart[WINDOW_HEIGHT];
    CType m_DistYCache[WINDOW_HEIGHT];
};

void FlatSurfacesRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

#endif