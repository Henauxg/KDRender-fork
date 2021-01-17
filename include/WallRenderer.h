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

    // TODO: Refactor!
    inline void ComputeRenderParameters(int iX, int iMinX, int iMaxX, CType iInvMinMaxXRange,
                                        int iMinVertexBottomPixel, int iMaxVertexBottomPixel,
                                        int iMinVertexTopPixel, int iMaxVertexTopPixel,
                                        CType iBottomZ, CType iTopZ,
                                        CType &oT, int &oMinY, int &oMaxY,
                                        int &oMinYUnclamped, int &oMaxYUnclamped,
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
    int m_MinColorClamp;
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
                                           CType iBottomZ, CType iTopZ,
                                           CType &oT, int &oMinY, int &oMaxY,
                                           int &oMinYUnclamped, int &oMaxYUnclamped,
                                           int &oTexelXClamped, CType &oMinTexelY, CType &oMaxTexelY) const
{
    // For extra precision, the inverse range has been shifted. Need to shift it back
    oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) >> 7u;
    // oT = iMaxX == iMinX ? CType(0) : (static_cast<CType>(iX - iMinX) * iInvMinMaxXRange) / (1 << 7u); // For float/double

    oMinYUnclamped = (1 - oT) * iMinVertexBottomPixel + oT * iMaxVertexBottomPixel;
    oMaxYUnclamped = (1 - oT) * iMinVertexTopPixel + oT * iMaxVertexTopPixel;
    oMinY = std::max<int>(oMinYUnclamped, m_pBottomOcclusionBuffer[iX]);
    oMaxY = std::min<int>(oMaxYUnclamped, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[iX]);

    
    // Affine mapping (nausea-inducing)
    // CType texelX = (1 - oT) * m_MinTexelX + oT * m_MaxTexelX;
    // Perspective correct
    CType texelX = ((1 - oT) * (m_MinTexelX / m_MinDist) + oT * (m_MaxTexelX / m_MaxDist)) / ((1 - oT) / m_MinDist + oT / m_MaxDist);
    // oTexelXClamped = static_cast<int>(Mod(texelX, CType(int(m_pTexture->m_Width))));
    oTexelXClamped = texelX - ((texelX >> (7u + FP_SHIFT)) << (7u + FP_SHIFT));
    oTexelXClamped = oTexelXClamped >= m_pTexture->m_Width ? m_pTexture->m_Width - 1 : oTexelXClamped;
    oMinTexelY = ((iBottomZ + m_TexVOffset) * CType(int(m_pTexture->m_Height)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
    oMaxTexelY = ((iTopZ + m_TexVOffset) * CType(int(m_pTexture->m_Height)) * CType(POSITION_SCALE)) / CType(TEXEL_SCALE);
    if (m_pTexture && oMaxYUnclamped != oMinYUnclamped)
    {
        // Clamp
        CType tMin = (CType(oMinY) - CType(oMinYUnclamped)) / (CType(oMaxYUnclamped - oMinYUnclamped));
        CType minTexelYBackup = oMinTexelY;
        oMinTexelY = (1 - tMin) * oMinTexelY + tMin * oMaxTexelY;
        CType tMax = (CType(oMaxYUnclamped) - CType(oMaxY)) / (CType(oMaxYUnclamped - oMinYUnclamped));
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
    CType tY, texelY;
    int texelYClamped;
    CType invMinMaxYRange = iMaxY == iMinY ? CType(1 << 7u) : (1 << 7u) / CType(iMaxY - iMinY);
    for (unsigned int y = iMinY; y <= iMaxY; y++)
    {
        tY = (CType(int(y)) - iMinY) * invMinMaxYRange >> 7u;
        texelY = (1 - tY) * iMinTexelY + tY * iMaxTexelY;
        // texelYClamped = static_cast<int>(Mod(texelY, CType(int(m_pTexture->m_Height))));
        texelYClamped = texelY - ((texelY >> (7u + FP_SHIFT)) << (7u + FP_SHIFT));
        texelYClamped = texelYClamped >= m_pTexture->m_Height ? m_pTexture->m_Height - 1 : texelYClamped;

        unsigned char r = m_pTexture->m_pData[iTexelXClamped * m_pTexture->m_Height * 4u + texelYClamped * 4u + 0];
        unsigned char g = m_pTexture->m_pData[iTexelXClamped * m_pTexture->m_Height * 4u + texelYClamped * 4u + 1];
        unsigned char b = m_pTexture->m_pData[iTexelXClamped * m_pTexture->m_Height * 4u + texelYClamped * 4u + 2];

        WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + iX, (light * r) >> 8u, (light * g) >> 8u, (light * b) >> 8u);
    }
}

#endif
