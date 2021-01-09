#ifndef KDTreeRenderer_h
#define KDTreeRenderer_h

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

    // Totally doom-inspired (Doom calls these 'Visplanes')
    // See Fabien Sanglard's really good book about the Doom Engine :)
    struct FlatSurface
    {
        FlatSurface()
        {
            // std::fill(m_MinY.begin(), m_MinY.end(), WINDOW_HEIGHT);
            // std::fill(m_MaxY.begin(), m_MaxY.end(), 0);
            for (unsigned int x = 0; x < WINDOW_WIDTH; x++)
            {
                m_MinY[x] = WINDOW_HEIGHT;
                m_MaxY[x] = 0;
            }
        }
        
        FlatSurface(const FlatSurface &iOther) :
            m_MinX(iOther.m_MinX),
            m_MaxX(iOther.m_MaxX),
            m_Height(iOther.m_Height),
            m_SectorIdx(iOther.m_SectorIdx)
        {
            memcpy(m_MinY, iOther.m_MinY, sizeof(int) * WINDOW_WIDTH);
            memcpy(m_MaxY, iOther.m_MaxY, sizeof(int) * WINDOW_WIDTH);
        }

        bool Absorb(const FlatSurface &iOther)
        {
            if(iOther.m_Height != m_Height || iOther.m_SectorIdx != m_SectorIdx)
                return false;

            if(iOther.m_MinX >= m_MinX && iOther.m_MinX <= m_MaxX)
                return false;

            if(iOther.m_MaxX >= m_MinX && iOther.m_MinX <= m_MaxX)
                return false;

            memcpy(m_MinY + iOther.m_MinX, iOther.m_MinY + iOther.m_MinX, (iOther.m_MaxX - iOther.m_MinX + 1) * sizeof(int));
            memcpy(m_MaxY + iOther.m_MinX, iOther.m_MaxY + iOther.m_MinX, (iOther.m_MaxX - iOther.m_MinX + 1) * sizeof(int));

            m_MinX = std::min(iOther.m_MinX, m_MinX);
            m_MaxX = std::max(iOther.m_MaxX, m_MaxX);

            return true;
        }

        int m_MinX;
        int m_MaxX;

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
                                        int &oT, int &oMinY, int &oMaxY,
                                        int &oMinYUnclamped, int &oMaxYUnclamped) const;
    inline void RenderColumn(int iT, int iMinVertexColor, int iMaxVertexColor,
                             int iMinY, int iMaxY, int iX,
                             int iR, int iG, int iB);
    bool AddFlatSurface(const FlatSurface &iFlatSurface);
    void RenderFlatSurfacesLegacy();
    void RenderFlatSurfaces();

protected:
    bool isInsideFrustum(const Vertex &iVertex) const;

protected:
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    int *m_pTopOcclusionBuffer;
    int *m_pBottomOcclusionBuffer;
    
    // Data used to render floors and ceilings
    std::unordered_map<int, std::vector<FlatSurface>> m_FlatSurfaces; // Flat surfaces are stored int the map according to their height
    int m_LinesXStart[WINDOW_HEIGHT];
    int m_HeightYCache[WINDOW_HEIGHT];
    int m_DistYCache[WINDOW_HEIGHT];

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

#include <iostream>
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
                                             int &oT, int &oMinY, int &oMaxY,
                                             int &oMinYUnclamped, int &oMaxYUnclamped) const
{
    oT = iMaxX == iMinX ? 0 : ((iX - iMinX) * (1 << DECIMAL_SHIFT)) / (iMaxX - iMinX);
    oMinYUnclamped = ARITHMETIC_SHIFT((((1 << DECIMAL_SHIFT) - oT) * iMinVertexBottomPixel + oT * iMaxVertexBottomPixel), DECIMAL_SHIFT);
    oMaxYUnclamped = ARITHMETIC_SHIFT((((1 << DECIMAL_SHIFT) - oT) * iMinVertexTopPixel + oT * iMaxVertexTopPixel), DECIMAL_SHIFT);
    oMinY = std::max<int>(oMinYUnclamped, m_pBottomOcclusionBuffer[iX]);
    oMaxY = std::min<int>(oMaxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[iX]);
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