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
    bool AddFlatSurface(const KDRData::FlatSurface &iFlatSurface);
    void RenderFlatSurfacesLegacy();
    void RenderFlatSurfaces();

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

    
    KDRData::State m_State;
    KDRData::Settings m_Settings;
};

void KDTreeRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

#endif