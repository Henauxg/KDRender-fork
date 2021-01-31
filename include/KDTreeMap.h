#ifndef KDTreeMap_h
#define KDTreeMap_h

#include "Consts.h"
#include "Light.h"

#include <vector>
#include <memory>

namespace KDMapData
{
    // This will probably end up as a full-fledged class
    struct Sector
    {
        int floor;
        int ceiling;

        int floorTexId;
        int ceilingTexId;

        std::shared_ptr<Light> m_pLight;
    };

    struct Vertex
    {
        int m_X;
        int m_Y;
    };

    class Wall
    {
    public:
        Wall() {}
        virtual ~Wall() {}

    public:
        Vertex m_From;
        Vertex m_To;

    public:
        int m_InSector;
        int m_OutSector;

    public:
        int m_TexId;
        int m_TexUOffset;
        int m_TexVOffset;
    };

    struct Texture
    {
        unsigned int m_Height; // Height as a power of 2, in order to shift
        unsigned int m_Width; // Same
        unsigned char *m_pData; // RGBA assumed
    };
}

class KDTreeNode
{
public:
    enum class SplitPlane
    {
        XConst,
        YConst,
        None
    };

public:
    KDTreeNode();
    virtual ~KDTreeNode();

public:
    unsigned int ComputeStreamSize() const;
    void Stream(char *&ioData, unsigned int &oNbBytesWritten) const;
    void UnStream(const char *ipData, unsigned int &oNbBytesRead);

public:
    SplitPlane GetSplitPlane() const { return m_SplitPlane; }
    int GetSplitOffset() const { return m_SplitOffset; }
    unsigned int GetNbOfWalls() const { return m_Walls.size(); }
    const KDMapData::Wall *GetWall(unsigned int iWallIdx) const { return &m_Walls[iWallIdx]; }
    void GetAABB(KDMapData::Vertex &oAABBMin, KDMapData::Vertex &oAABBMax) const;

    template <typename WallType>
    WallType BuildXYScaledWall(unsigned iWallIdx) const
    {
        const KDMapData::Wall &kdWall = m_Walls[iWallIdx];
        WallType ret;

        ret.m_VertexFrom.m_X = static_cast<CType>(kdWall.m_From.m_X) / POSITION_SCALE;
        ret.m_VertexFrom.m_Y = static_cast<CType>(kdWall.m_From.m_Y) / POSITION_SCALE;

        ret.m_VertexTo.m_X = static_cast<CType>(kdWall.m_To.m_X) / POSITION_SCALE;
        ret.m_VertexTo.m_Y = static_cast<CType>(kdWall.m_To.m_Y) / POSITION_SCALE;

        return ret;
    }

public:
    // For debugging purpose
    int ComputeDepth() const;

protected:
    int RecursiveComputeDepth(const KDTreeNode *ipNode) const;

protected:
    std::vector<KDMapData::Wall> m_Walls;

    SplitPlane m_SplitPlane;
    int m_SplitOffset;

    // Axis-aligned bounding box of the node
    // Used by renderer for frustum culling
    // For a KD-tree, it is easy to determine an AABB upon traversal. However,
    // I want to make switching to generic BSPs not too hard if I decide to do it
    // one day :)
    KDMapData::Vertex m_AABBMin;
    KDMapData::Vertex m_AABBMax;

    KDTreeNode *m_PositiveSide;
    KDTreeNode *m_NegativeSide;

private:
    friend class KDTreeBuilder;
    friend class KDTreeRenderer;
};

class KDTreeMap
{
public:
    KDTreeMap();
    virtual ~KDTreeMap();

public:
    void Stream(char *&oData, unsigned int &oNbBytesWritten) const;
    void UnStream(const char *ipData, unsigned int &oNbBytesRead);

public:
    void ComputeDynamicColorPalettes();

public:
    CType GetPlayerStartX() const;
    CType GetPlayerStartY() const;
    int GetPlayerStartDirection() const;

public:
    // For debugging purpose only
    int ComputeDepth() const;

protected:
    unsigned int ComputeStreamSize() const;

protected:
    std::vector<KDMapData::Texture> m_Textures;
    std::vector<KDMapData::Sector> m_Sectors;

    KDTreeNode *m_RootNode;

    int m_PlayerStartX;
    int m_PlayerStartY;
    int m_PlayerStartDirection;

    unsigned int m_ColorPalette[256];

    unsigned int m_DynamicColorPalettes[16][256];

private:
    friend class KDTreeBuilder;
    friend class KDTreeRenderer;
    friend class WallRenderer;
    friend class FlatSurfacesRenderer;
};

#endif