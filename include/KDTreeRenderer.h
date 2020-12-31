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

    void SetPlayerCoordinates(const Vertex &iPosition, int iDirection);

    Vertex GetPlayerPosition() const;
    int GetPlayerDirection() const;

    Vertex GetLook() const;

protected:
    void FillFrameBufferWithColor(unsigned char r, unsigned char g, unsigned char b);
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

protected:
    int ComputeZ();
    int RecursiveComputeZ(KDTreeNode *pNode);

    void Render();
    void RenderNode(KDTreeNode *pNode);
    void RenderWall(const Wall &iWall, const Vertex &iMinVertex, const Vertex &iMaxVertex, int iMinAngle, int iMaxAngle);

protected:
    bool isInsideFrustum(const Vertex &iVertex) const;

protected:
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;

    Vertex m_PlayerPosition;
    int m_PlayerZ;
    int m_PlayerDirection;
    Vertex m_FrustumToLeft;
    Vertex m_FrustumToRight;
    Vertex m_Look;

    const int m_PlayerHorizontalFOV;
    const int m_PlayerVerticalFOV;
    const int m_PlayerHeight;
    const int m_MaxColorInterpolationDist;
};

void KDTreeRenderer::WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b)
{
    idx = idx << 2u;
    m_pFrameBuffer[idx] = r;
    m_pFrameBuffer[idx + 1u] = g;
    m_pFrameBuffer[idx + 2u] = b;
}

#endif