#ifndef KDTreeRendererData_h
#define KDTreeRendererData_h

#include "Consts.h"
#include "FP32.h"
#include "KDTreeMap.h"

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

    public:
        int m_MinX;
        int m_MaxX;

        int m_MinY[WINDOW_WIDTH];
        int m_MaxY[WINDOW_WIDTH];

        CType m_Height;
        int m_SectorIdx;
    };
}

#endif