#ifndef MapBuildData_h
#define MapBuildData_h

#include "Map.h"

#include <vector>
#include <list>

namespace MapBuildData
{
    enum class ErrorCode
    {
        OK,
        INVALID_POLYGON,
        SECTOR_INTERSECTION,
        CANNOT_BREAK_WALL,
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
            else
                return iLeft.m_OutSector < iRight.m_OutSector;
        }

        bool IsGeomEqual(const Wall &iOtherWall);
        bool IsGeomOpposite(const Wall &iOtherWall) const;

        unsigned int GetConstCoordinate() const;

        int m_InSector;
        int m_OutSector;
    };

    struct Sector
    {
        std::list<Wall> m_Walls;

        int m_Ceiling;
        int m_Floor;

        enum class Relationship
        {
            INSIDE,
            PARTIAL, // "hard" intersection
            UNDETERMINED
        };

        // Returns the relationship between this and iSector2 (in this order)
        Relationship FindRelationship(const Sector &iSector2) const;
    };
};

#endif