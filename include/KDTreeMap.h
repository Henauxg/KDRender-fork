#ifndef KDTreeMap_h
#define KDTreeMap_h

#include "Consts.h"

#include <vector>

namespace KDMapData
{
    struct Sector
    {
        int floor;
        int ceiling;
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
        unsigned int m_Height;
        unsigned int m_Width;
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

private:
    friend class KDTreeBuilder;
    friend class KDTreeRenderer;
    friend class WallRenderer;
};

#endif