#ifndef KDTreeMap_h
#define KDTreeMap_h

#include <vector>

namespace KDMapData
{
    struct Sector
    {
        int floor;
        int ceiling;
    };

    class Wall
    {
    public:
        Wall() {}
        virtual ~Wall() {}

    public:
        int m_From;
        int m_To;

        int m_InSector;
        int m_OutSector;
    };
}

class KDTreeNode
{
public:
    enum class SplitPlane
    {
        XConst,
        YConst
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
    const KDMapData::Wall &GetWall(unsigned int iWallIdx) const { return m_Walls[iWallIdx]; }

protected:
    std::vector<KDMapData::Wall> m_Walls;

    SplitPlane m_SplitPlane;
    int m_SplitOffset;

    KDTreeNode *m_PositiveSide;
    KDTreeNode *m_NegativeSide;

private:
    friend class MapBuildData;
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
    int GetPlayerStartX() const;
    int GetPlayerStartY() const;
    int GetPlayerStartDirection() const;

protected:
    unsigned int ComputeStreamSize() const;

protected:
    std::vector<KDMapData::Sector> m_Sectors;
    KDTreeNode *m_RootNode;
    int m_PlayerStartX;
    int m_PlayerStartY;
    int m_PlayerStartDirection;

private:
    friend class MapBuildData;
    friend class KDTreeRenderer;
};

#endif