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

protected:
    std::vector<KDMapData::Wall> m_Walls;

    SplitPlane m_SplitPlane;
    int m_SplitOffset;

    KDTreeNode *m_PositiveSide;
    KDTreeNode *m_NegativeSide;

private:
    friend class MapBuildData;
};

class KDTreeMap
{
public:
    KDTreeMap();
    virtual ~KDTreeMap();

public:
    void Stream(char *&oData);
    void UnStream(const char *iData);

protected:
    unsigned int ComputeStreamSize();

protected:
    KDTreeNode *m_RootNode;

private:
    friend class MapBuildData;
};

#endif