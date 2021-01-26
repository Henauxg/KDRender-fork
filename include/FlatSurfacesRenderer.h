#ifndef FlatSurfacesRenderer_h
#define FlatSurfacesRenderer_h

#include "KDTreeRendererData.h"

#include <vector>
#include <map>

class FlatSurfacesRenderer
{
public:
    FlatSurfacesRenderer(const std::map<CType, std::vector<KDRData::FlatSurface>> &iFlatSurfaces, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap);
    virtual ~FlatSurfacesRenderer();

public:
    void SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer);

public:
    void Render();

protected:
    void DrawLine(int iY, int iMinX, int iMaxX, const KDRData::FlatSurface &iSurface);
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

protected:
    const std::map<CType, std::vector<KDRData::FlatSurface>> &m_FlatSurfaces;
    const KDRData::State &m_State;
    const KDRData::Settings &m_Settings;
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;

    // Caches
    int m_LinesXStart[WINDOW_HEIGHT];
    CType m_DistYCache[WINDOW_HEIGHT];
    KDRData::Vertex m_LeftmostTexelCache[WINDOW_HEIGHT];
    CType m_DeltaTexelYCache[WINDOW_HEIGHT];
    CType m_DeltaTexelXCache[WINDOW_HEIGHT];

    // TODO textures
    unsigned char m_CurrSectorR;
    unsigned char m_CurrSectorG;
    unsigned char m_CurrSectorB;

    int m_RDbg, m_GDbg, m_BDbg;
};

void FlatSurfacesRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

#endif