#ifndef KDTreeRenderer_h
#define KDTreeRenderer_h

#include "KDTreeMap.h"

class KDTreeRenderer
{
public:
    struct Vertex
    {
        int m_X;
        int m_Y;
    };

    struct Wall
    {
        Vertex m_VertexFrom;
        Vertex m_VertexTo;

        const KDMapData::Wall *m_pKDWall;
    };

public:
    KDTreeRenderer(const KDTreeMap &iMap);

public:
    const unsigned char* GetFrameBuffer() const;
    unsigned char *GetFrameBuffer();

    void RefreshFrameBuffer();
    void ClearBuffers();

    inline void SetPlayerPosition(int iX, int iY, int iDirection);

protected:
    void FillFrameBufferWithColor(unsigned char r, unsigned char g, unsigned char b);
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

protected:
    int ComputeZ();
    int RecursiveComputeZ(KDTreeNode *pNode);

protected:
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;

    Vertex m_PlayerPosition;
    int m_PlayerDirection;
    int m_PlayerFOV;
    int m_PlayerHeight;
};

void KDTreeRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

void KDTreeRenderer::SetPlayerPosition(int iX, int iY, int iDirection)
{
    m_PlayerPosition.m_X = iX;
    m_PlayerPosition.m_Y = iY;
    m_PlayerDirection = iDirection;
}

#endif