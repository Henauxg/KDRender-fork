#ifndef WallRenderer_h
#define WallRenderer_h

#include "KDTreeRendererData.h"
#include "GeomUtils.h"

#include <vector>

class WallRenderer
{
public:
    WallRenderer(KDRData::Wall &iWall, const KDRData::State &iState, const KDRData::Settings &iSettings, const KDTreeMap &iMap);
    virtual ~WallRenderer();

public:
    void SetBuffers(unsigned char *ipFrameBuffer, unsigned char *ipHorizOcclusionBuffer, 
                    KDRData::HorizontalScreenSegments *ipHorizDrawnSegs, int *ipTopOcclusionBuffer, int *ipBottomOcclusionBuffer);
    void Render(std::vector<KDRData::FlatSurface> &oGeneratedFlats);

protected:
    void RenderWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderHardWall(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderSoftWallTop(std::vector<KDRData::FlatSurface> &oGeneratedFlats);
    void RenderSoftWallBottom(std::vector<KDRData::FlatSurface> &oGeneratedFlats);

protected:
    bool isInsideFrustum(const KDRData::Vertex &iVertex) const;
    inline void WriteFrameBuffer(unsigned int idx, unsigned char r, unsigned char g, unsigned char b);

    // TODO: Refactor!
    inline void ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
                                        int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                        int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                        CType &oT, int &oMinY, int &oMaxY,
                                        int &oMinYUnclamped, int &oMaxYUnclamped) const;
    inline void ComputeTextureParameters(CType iT, int iMinY, int iMaxY,
                                         CType iBottomZ, CType iTopZ,
                                         int iMinYUnclamped, int iMaxYUnclamped,
                                         int &oTexelXClamped, CType &oMinTexelY, CType &oMaxTexelY) const;
    inline void RenderColumn(CType iT, int iMinVertexColor, int iMaxVertexColor,
                                   int iMinY, int iMaxY, int iX,
                                   int iR, int iG, int iB);
    inline void RenderColumnWithTexture(CType iT, int iMinVertexLight, int iMaxVertexLight,
                                        int iMinY, int iMaxY, int iX,
                                        int iTexelXClamped, CType iMinTexelY, CType iMaxTexelY);

protected:
    const KDRData::Wall &m_Wall;
    const KDRData::State &m_State;
    const KDRData::Settings &m_Settings;
    const KDTreeMap &m_Map;

    unsigned char *m_pFrameBuffer;
    unsigned char *m_pHorizOcclusionBuffer;
    KDRData::HorizontalScreenSegments *m_pHorizDrawnSegs; // TODO: makes m_pHorizOcclusionBuffer, get rid of m_pHorizOcclusionBuffer
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

    CType m_MinTexelX;
    CType m_MaxTexelX;

    CType m_MinDist;
    CType m_MaxDist;

    int m_MaxColorRange;
    int m_MinColorRange;
    int m_MinVertexColor;
    int m_MaxVertexColor;

    int m_WhichSide;

    int m_InSectorIdx;
    KDRData::Sector m_InSector;
    int m_OutSectorIdx;
    KDRData::Sector m_OutSector;

    const KDMapData::Texture *m_pTexture;
    int m_TexUOffset;
    int m_TexVOffset;
    unsigned int m_YModShift;

protected:
    // Debug only
    char r,
        g,
        b;
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
    oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) >> 7u;
    // oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) / (1 << 7u); // For float/double

    oMinYUnclamped = MultiplyIntFpToInt(iMinVertexBottomPixel, 1 - oT) + MultiplyIntFpToInt(iMaxVertexBottomPixel, oT);
    oMaxYUnclamped = MultiplyIntFpToInt(iMinVertexTopPixel, 1 - oT) + MultiplyIntFpToInt(iMaxVertexTopPixel, oT);
    oMinY = std::max<int>(oMinYUnclamped, m_pBottomOcclusionBuffer[iX]);
    oMaxY = std::min<int>(oMaxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[iX]);
}

void WallRenderer::ComputeTextureParameters(CType iT, int iMinY, int iMaxY,
                                            CType iBottomZ, CType iTopZ,
                                            int iMinYUnclamped, int iMaxYUnclamped,
                                            int &oTexelXClamped, CType &oMinTexelY, CType &oMaxTexelY) const
{
    if (!m_pTexture)
        return;

    // Affine mapping (nausea-inducing)
    // CType texelX = (1 - oT) * m_MinTexelX + oT * m_MaxTexelX;
    // Perspective correct
    CType texelX = ((1 - iT) * (m_MinTexelX / m_MinDist) + iT * (m_MaxTexelX / m_MaxDist)) / ((1 - iT) / m_MinDist + iT / m_MaxDist);
    unsigned int xModShift = m_pTexture->m_Width + FP_SHIFT;
    oTexelXClamped = texelX - ((texelX >> xModShift) << xModShift);
    oMinTexelY = ((iBottomZ + m_TexVOffset) * CType(int(1u << m_pTexture->m_Height)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
    oMaxTexelY = ((iTopZ + m_TexVOffset) * CType(int(1u << m_pTexture->m_Height)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
    if (m_pTexture && iMaxYUnclamped != iMinYUnclamped)
    {
        // Clamp
        CType tMin = (CType(iMinY) - CType(iMinYUnclamped)) / (CType(iMaxYUnclamped - iMinYUnclamped));
        CType minTexelYBackup = oMinTexelY;
        oMinTexelY = (1 - tMin) * oMinTexelY + tMin * oMaxTexelY;
        CType tMax = (CType(iMaxYUnclamped) - CType(iMaxY)) / (CType(iMaxYUnclamped - iMinYUnclamped));
        oMaxTexelY = (1 - tMax) * oMaxTexelY + tMax * minTexelYBackup;
    }
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

void WallRenderer::RenderColumnWithTexture(CType iT, int iMinVertexLight, int iMaxVertexLight,
                                           int iMinY, int iMaxY, int iX,
                                           int iTexelXClamped, CType iMinTexelY, CType iMaxTexelY)
{
    int light = (iMinVertexLight * (1 - iT)) + iT * iMaxVertexLight;
    CType tY, texelY = iMinTexelY;
    int texelYClamped;
    CType deltaTexelY = iMaxY == iMinY ? CType(1) : (iMaxTexelY - iMinTexelY) / CType(iMaxY - iMinY);

    unsigned int textureIdxX = (iTexelXClamped * (1u << m_pTexture->m_Height)) << 2u;
    unsigned int textureIdxY;
    unsigned char r, g, b;
    for (unsigned int y = iMinY; y <= iMaxY; y++)
    {
        texelY = texelY + deltaTexelY;
        texelYClamped = texelY - ((texelY >> (m_YModShift)) << (m_YModShift));
        textureIdxY = textureIdxX + (texelYClamped << 2u);

        r = m_pTexture->m_pData[textureIdxY];
        g = m_pTexture->m_pData[textureIdxY + 1];
        b = m_pTexture->m_pData[textureIdxY + 2];

        // TODO: get rid of integer multiplications (way too expensive), use CLUT instead
        // Well, current hardware doesn't support palette as far as I know, so a CLUT might slow things down actually
        WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + iX, (light * r) >> 8u, (light * g) >> 8u, (light * b) >> 8u);
    }
}

#endif
