#ifndef KDTreeRenderer_h
#define KDTreeRenderer_h

#include "KDTreeMap.h"
#include "Consts.h"
#include <cstring>

class KDTreeRenderer
{
public:
    struct Vertex
    {
        int m_X;
        int m_Y;

        Vertex LShift(unsigned int iShift)
        {
            Vertex ret;
            ret.m_X = m_X << iShift;
            ret.m_Y = m_Y << iShift;
            return ret;
        }

        Vertex RShift(unsigned int iShift)
        {
            Vertex ret;
            ret.m_X = ARITHMETIC_SHIFT(m_X, iShift);
            ret.m_Y = ARITHMETIC_SHIFT(m_Y, iShift);
            return ret;
        }
    };

    struct Wall
    {
        Vertex m_VertexFrom;
        Vertex m_VertexTo;

        const KDMapData::Wall *m_pKDWall;
    };

    // Totally doom-inspired (Doom call these 'Visplanes')
    // See Fabien Sanglard's really good book about the Doom Engine :)
    struct Surface
    {
        Surface()
        {
            memset(m_MinY, WINDOW_HEIGHT, sizeof(int) * WINDOW_WIDTH);
            memset(m_MaxY, 0, sizeof(int) * WINDOW_WIDTH);
        }

        int m_MinY[WINDOW_WIDTH];
        int m_MaxY[WINDOW_WIDTH];

        int m_Height;
        int m_SectorIdx;
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
    inline void ComputeRenderParameters(int iX, int iMinX, int iMaxX,
                                        int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                        int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                        int &oT, int &oMinY, int &oMaxY) const;
    inline void RenderColumn(int iT, int iMinVertexColor, int iMaxVertexColor,
                             int iMinY, int iMaxY, int iX,
                             int iR, int iG, int iB);

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

void KDTreeRenderer::ComputeRenderParameters(int iX, int iMinX, int iMaxX,
                                             int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                             int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                             int &oT, int &oMinY, int &oMaxY) const
{
    oT = ((iX - iMinX) * (1 << DECIMAL_SHIFT)) / (iMaxX - iMinX);
    oMinY = ARITHMETIC_SHIFT((((1 << DECIMAL_SHIFT) - oT) * iMinVertexBottomPixel + oT * iMaxVertexBottomPixel), DECIMAL_SHIFT);
    oMaxY = ARITHMETIC_SHIFT((((1 << DECIMAL_SHIFT) - oT) * iMinVertexTopPixel + oT * iMaxVertexTopPixel), DECIMAL_SHIFT);
    oMinY = std::max<int>(oMinY, m_pBottomOcclusionBuffer[iX]);
    oMaxY = std::min<int>(oMaxY, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[iX]);
}

void KDTreeRenderer::RenderColumn(int iT, int iMinVertexColor, int iMaxVertexColor,
                                  int iMinY, int iMaxY, int iX,
                                  int iR, int iG, int iB)
{
    int color = ARITHMETIC_SHIFT((iMinVertexColor * ((1 << DECIMAL_SHIFT) - iT)) + iT * iMaxVertexColor, DECIMAL_SHIFT);
    // int color = ARITHMETIC_SHIFT(iT * 255, DECIMAL_SHIFT);
    for (unsigned int y = iMinY; y <= iMaxY; y++)
    {
        WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + iX, color * iR, color * iG, color * iB);
    }
}

#endif