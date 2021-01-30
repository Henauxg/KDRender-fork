#ifndef KDTreeBuilderData_h
#define KDTreeBuilderData_h

#include "Map.h"
#include "Light.h"

#include <vector>
#include <list>
#include <memory>

namespace KDBData
{
    enum class Error
    {
        OK,
        INVALID_POLYGON,
        SECTOR_INTERSECTION,
        CANNOT_BREAK_WALL,
        CANNOT_LOAD_TEXTURE,
        CANNOT_LOAD_SPRITE,
        UNKNOWN_FAILURE
    };

    struct Vertex
    {
        Vertex();
        Vertex(const Map::Data::Sector::Vertex &iV);

        int m_X;
        int m_Y;

        int GetCoord(unsigned int i) const;
        void SetCoord(unsigned int i, int iVal);

        friend bool operator<(const Vertex &iLeft, const Vertex &iRight)
        {
            if (iLeft.m_X != iRight.m_X)
                return iLeft.m_X < iRight.m_X;
            else
                return iLeft.m_Y < iRight.m_Y;
        }

        friend bool operator==(const Vertex &iLeft, const Vertex &iRight)
        {
            return iLeft.m_X == iRight.m_X && iLeft.m_Y == iRight.m_Y;
        }
    };

    struct Wall
    {
        Wall() {}

        Vertex m_VertexFrom;
        Vertex m_VertexTo;

        friend bool operator<(const Wall &iLeft, const Wall &iRight)
        {
            if (!(iLeft.m_VertexFrom == iRight.m_VertexFrom))
                return iLeft.m_VertexFrom < iRight.m_VertexFrom;
            else if (!(iLeft.m_VertexTo == iRight.m_VertexTo))
                return iLeft.m_VertexTo < iRight.m_VertexTo;
            else if (iLeft.m_InSector != iRight.m_InSector)
                return iLeft.m_InSector < iRight.m_InSector;
            else if (iLeft.m_OutSector != iRight.m_OutSector)
                return iLeft.m_OutSector < iRight.m_OutSector;
            else // Should normally be useless
                return iLeft.m_TexId < iRight.m_TexId;
        }

        bool IsGeomEqual(const Wall &iOtherWall);
        bool IsGeomOpposite(const Wall &iOtherWall) const;

        unsigned int GetConstCoordinate() const;

        int m_InSector;
        int m_OutSector;

        int m_TexId;
        int m_TexUOffset;
        int m_TexVOffset;
    };

    struct Sector
    {
        std::list<Wall> m_Walls;

        int m_Ceiling;
        int m_Floor;

        int m_CeilingTexId;
        int m_FloorTexId;

        std::shared_ptr<Light> m_pLight;

        enum class Relationship
        {
            INSIDE,
            PARTIAL, // "hard" intersection
            UNDETERMINED
        };
    };
};

#endif