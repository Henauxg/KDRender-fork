#ifndef KDTreeRendererData_h
#define KDTreeRendererData_h

#include "Consts.h"
#include "FP32.h"
#include "KDTreeMap.h"

#include <list>
#include <vector>

namespace KDRData
{
    struct Vertex
    {
        CType m_X;
        CType m_Y;
    };

    struct Wall
    {
        Vertex m_VertexFrom;
        Vertex m_VertexTo;

        const KDMapData::Wall *m_pKDWall;
    };

    struct Sector
    {
        CType m_Floor;
        CType m_Ceiling;

        const KDMapData::Sector *m_pKDSector;
    };

    // Totally doom-inspired (Doom calls these 'Visplanes')
    // See Fabien Sanglard's really good book about the Doom Engine :)
    class FlatSurface
    {
    public:
        FlatSurface();
        FlatSurface(const FlatSurface &iOther);

    public:
        bool Absorb(const FlatSurface &iOther);
        void Tighten();

    public:
        int m_MinX;
        int m_MaxX;

        int m_MinY[WINDOW_WIDTH];
        int m_MaxY[WINDOW_WIDTH];

        CType m_Height;
        int m_SectorIdx;
        int m_TexId;
    };

    // For sprite clipping
    // Doom-inspired as well
    class SpriteClippingSegment
    {
    public:
        SpriteClippingSegment();
        virtual ~SpriteClippingSegment();

    public:
        enum class BoundaryType
        {
            NONE,
            TOP,
            BOTTOM
        };

    public:
        int m_LeftX;
        int m_RightX;

        int m_LeftDist;
        int m_RightDist;

        BoundaryType m_Boundary;
        int m_LeftYBoundary;
        int m_RightYBoundary;
    };

    // For horizontal occlusion
    class HorizontalScreenSegments
    {
    public:
        HorizontalScreenSegments();
        virtual ~HorizontalScreenSegments();

    public:
        void AddScreenSegment(unsigned int iMinX, unsigned int iMaxX);
        bool IsScreenEntirelyDrawn() const;
        void Clear();

    protected:
        struct IntervalEnd
        {
            unsigned int m_X;
            int m_Direction; // +1 or -1

            friend bool operator < (const IntervalEnd &iEnd1, const IntervalEnd &iEnd2)
            {
                if(iEnd1.m_X != iEnd2.m_X)
                    return iEnd1.m_X < iEnd2.m_X;
                else
                    return iEnd1.m_Direction > iEnd2.m_Direction; // Yup, this way
            }
        };

    protected:
        void InsertIntervalEnd(const IntervalEnd &iEnd);

    protected:
        std::list<IntervalEnd> m_Segments;
    };

    struct Settings
    {
        int m_PlayerHorizontalFOV;
        int m_PlayerVerticalFOV;
        CType m_PlayerHeight;
        CType m_HorizontalDistortionCst;
        CType m_VerticalDistortionCst;
    };

    struct State
    {
        KDRData::Vertex m_PlayerPosition;
        CType m_PlayerZ;
        int m_PlayerDirection;
        KDRData::Vertex m_FrustumToLeft;
        KDRData::Vertex m_FrustumToRight;
        KDRData::Vertex m_Look;
    };

    Wall GetWallFromNode(KDTreeNode *ipNode, unsigned int iWallIdx);
    void GetAABBFromNode(KDTreeNode *ipNode, KDRData::Vertex &oAABBMin,  KDRData::Vertex &oAABBMax);
    Sector GetSectorFromKDSector(const KDMapData::Sector &iSector);
} // namespace KDRData

#endif