#include "KDTreeMap.h"

KDTreeNode::KDTreeNode():
    m_PositiveSide(nullptr),
    m_NegativeSide(nullptr)
{
    
}

KDTreeNode::~KDTreeNode()
{
    if(m_PositiveSide)
        delete m_PositiveSide;
    m_PositiveSide = nullptr;

    if(m_NegativeSide)
        delete m_NegativeSide;
    m_NegativeSide = nullptr;
}

unsigned int KDTreeNode::ComputeStreamSize() const
{
    unsigned int streamSize = 0;

    streamSize += sizeof(unsigned int); // Size of m_Walls
    streamSize += m_Walls.size() * sizeof(KDMapData::Wall);
    streamSize += sizeof(char); // m_SplitPlane
    streamSize += sizeof(int); // m_SplitOffset

    streamSize += sizeof(char); // Field that indicates whether there is a positive child
    if (m_PositiveSide)
        streamSize += m_PositiveSide->ComputeStreamSize();

    streamSize += sizeof(char); // Field that indicates whether there is a negative child
    if (m_NegativeSide)
        streamSize += m_NegativeSide->ComputeStreamSize();

    return streamSize;
}

void KDTreeNode::Stream(char *&ioData, unsigned int &oNbBytesWritten) const
{
    if (!ioData)
        return;

    *(reinterpret_cast<unsigned int *>(ioData)) = m_Walls.size();
    ioData += sizeof(unsigned int);

    for (unsigned int i = 0; i < m_Walls.size(); i++)
    {
        *(reinterpret_cast<KDMapData::Wall *>(ioData)) = m_Walls[i];
        ioData += sizeof(KDMapData::Wall);
    }

    *ioData = m_SplitPlane == SplitPlane::XConst ? 0x0 : 0x1;
    ioData += sizeof(char);

    *(reinterpret_cast<int *>(ioData)) = m_SplitOffset;
    ioData += sizeof(int);

    *ioData = m_PositiveSide ? 0x1 : 0x0;
    ioData += sizeof(char);

    if (m_PositiveSide)
        m_PositiveSide->Stream(ioData, oNbBytesWritten);

    *ioData = m_NegativeSide ? 0x1 : 0x0;
    ioData += sizeof(char);

    if (m_NegativeSide)
        m_NegativeSide->Stream(ioData, oNbBytesWritten);
}

void KDTreeNode::UnStream(const char *ipData, unsigned int &oNbBytesRead)
{
    if(!ipData)
        return;

    const char *pDataInit = ipData;
    oNbBytesRead = 0;

    unsigned int nbWalls = *(reinterpret_cast<const unsigned int *>(ipData));
    ipData += sizeof(unsigned int);

    for (unsigned int i = 0; i < nbWalls; i++)
    {
        m_Walls.push_back(*(reinterpret_cast<const KDMapData::Wall *>(ipData)));
        ipData += sizeof(KDMapData::Wall);
    }

    char splitPlaneInt = *ipData;
    ipData += sizeof(char);
    if (splitPlaneInt == 0)
        m_SplitPlane = KDTreeNode::SplitPlane::XConst;
    else if (splitPlaneInt == 1)
        m_SplitPlane = KDTreeNode::SplitPlane::YConst;

    m_SplitOffset = *(reinterpret_cast<const int *>(ipData));
    ipData += sizeof(unsigned int);

    char positiveSide = *ipData;
    ipData += sizeof(char);

    oNbBytesRead += ipData - pDataInit;

    if (positiveSide)
    {
        m_PositiveSide = new KDTreeNode;
        if(m_PositiveSide)
        {
            unsigned int nbBytesRead = 0;
            m_PositiveSide->UnStream(ipData, nbBytesRead);
            oNbBytesRead += nbBytesRead;
            ipData += nbBytesRead;
        }
    }

    char negativeSide = *ipData;
    ipData += sizeof(char);
    oNbBytesRead += sizeof(char);

    if (negativeSide)
    {
        m_NegativeSide = new KDTreeNode;
        if (m_NegativeSide)
        {
            unsigned int nbBytesRead = 0;
            m_NegativeSide->UnStream(ipData, nbBytesRead);
            oNbBytesRead += nbBytesRead;
            ipData += nbBytesRead;
        }
    }
}

KDTreeMap::KDTreeMap() : m_RootNode(nullptr)
{
    
}

KDTreeMap::~KDTreeMap()
{
    if(m_RootNode)
        delete m_RootNode;
    m_RootNode = nullptr;
}

void KDTreeMap::Stream(char *&oData, unsigned int &oSize) const
{
    oSize = ComputeStreamSize();
    oData = new char[oSize];
    if(oData)
    {
        char *pData = oData;

        *(reinterpret_cast<int *>(pData)) = m_PlayerStartX;
        pData += sizeof(int);

        *(reinterpret_cast<int *>(pData)) = m_PlayerStartY;
        pData += sizeof(int);

        *(reinterpret_cast<int *>(pData)) = m_PlayerStartDirection;
        pData += sizeof(int);

        *(reinterpret_cast<unsigned int *>(pData)) = m_Sectors.size();
        pData += sizeof(unsigned int);

        for (unsigned i = 0; i < m_Sectors.size(); i++)
        {
            *(reinterpret_cast<KDMapData::Sector *>(pData)) = m_Sectors[i];
            pData += sizeof(KDMapData::Sector);
        }

        unsigned int dummy;
        m_RootNode->Stream(pData, dummy);
    }
    else
        oSize = 0;
}

void KDTreeMap::UnStream(const char *iData, unsigned int &oNbBytesRead)
{
    if(!iData)
        return;

    const char *pDataInit = iData;
    oNbBytesRead = 0;

    m_PlayerStartX = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    m_PlayerStartY = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    m_PlayerStartDirection = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    unsigned nbSectors = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    for (unsigned int i = 0; i < nbSectors; i++)
    {
        KDMapData::Sector sector = *(reinterpret_cast<const KDMapData::Sector *>(iData));
        m_Sectors.push_back(sector);
        iData += sizeof(KDMapData::Sector);
    }

    if (m_RootNode)
        delete m_RootNode;
    m_RootNode = new KDTreeNode;

    oNbBytesRead += (iData - pDataInit);

    if (m_RootNode)
    {
        unsigned int nbBytesRead = 0;
        m_RootNode->UnStream(iData, nbBytesRead);
        oNbBytesRead += nbBytesRead;
    }
}

unsigned int KDTreeMap::ComputeStreamSize() const
{
    unsigned int streamSize = 0;

    streamSize += 3 * sizeof(int); // m_PlayerStartX, m_PlayerStartY, m_PlayerStartDirection

    streamSize += sizeof(unsigned int); // m_Sectors.size()
    streamSize += m_Sectors.size() * sizeof(KDMapData::Sector);

    if (m_RootNode)
        streamSize += m_RootNode->ComputeStreamSize();
    
    return streamSize;
}

int KDTreeMap::GetPlayerStartX() const
{
    return m_PlayerStartX;
}

int KDTreeMap::GetPlayerStartY() const
{
    return m_PlayerStartY;
}

int KDTreeMap::GetPlayerStartDirection() const
{
    return m_PlayerStartDirection;
}