#include "KDTreeMap.h"

#include <cstdint>
#include <cstring>

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
    streamSize += 2 * sizeof(KDMapData::Vertex); // m_AABBMin & m_AABBMax

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

    *(reinterpret_cast<unsigned int*>(ioData))
        = static_cast<unsigned int>(m_Walls.size());
    ioData += sizeof(unsigned int);

    for (unsigned int i = 0; i < m_Walls.size(); i++)
    {
        *(reinterpret_cast<KDMapData::Wall *>(ioData)) = m_Walls[i];
        ioData += sizeof(KDMapData::Wall);
    }

    *ioData = m_SplitPlane == SplitPlane::XConst ? 0x0 : (m_SplitPlane == SplitPlane::YConst ? 0x1 : 0x2);
    ioData += sizeof(char);

    *(reinterpret_cast<int *>(ioData)) = m_SplitOffset;
    ioData += sizeof(int);

    *(reinterpret_cast<KDMapData::Vertex *>(ioData)) = m_AABBMin;
    ioData += sizeof(KDMapData::Vertex);

    *(reinterpret_cast<KDMapData::Vertex *>(ioData)) = m_AABBMax;
    ioData += sizeof(KDMapData::Vertex);

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
    else if (splitPlaneInt == 2)
        m_SplitPlane = KDTreeNode::SplitPlane::None;

    m_SplitOffset = *(reinterpret_cast<const int *>(ipData));
    ipData += sizeof(unsigned int);

    m_AABBMin = *(reinterpret_cast<const KDMapData::Vertex *>(ipData));
    ipData += sizeof(KDMapData::Vertex);

    m_AABBMax = *(reinterpret_cast<const KDMapData::Vertex *>(ipData));
    ipData += sizeof(KDMapData::Vertex);

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

int KDTreeNode::ComputeDepth() const
{
    return RecursiveComputeDepth(this);
}

int KDTreeNode::RecursiveComputeDepth(const KDTreeNode *ipNode) const
{
    if(!ipNode)
        return 0;
    else
        return 1 + std::max(RecursiveComputeDepth(ipNode->m_NegativeSide), RecursiveComputeDepth(ipNode->m_PositiveSide));
}

void KDTreeNode::GetAABB(KDMapData::Vertex &oAABBMin, KDMapData::Vertex &oAABBMax) const
{
    oAABBMin = m_AABBMin;
    oAABBMax = m_AABBMax;
}

KDTreeMap::KDTreeMap() : m_RootNode(nullptr)
{
    
}

KDTreeMap::~KDTreeMap()
{
    if(m_RootNode)
        delete m_RootNode;
    m_RootNode = nullptr;

    for (unsigned int i = 0; i < m_Textures.size(); i++)
    {
        if(m_Textures[i].m_pData)
            delete m_Textures[i].m_pData;
        m_Textures[i].m_pData = nullptr;
    }
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

        memcpy(pData, m_ColorPalette, sizeof(m_ColorPalette));
        pData += sizeof(m_ColorPalette);

        *(reinterpret_cast<unsigned int *>(pData)) = m_Textures.size();
        pData += sizeof(unsigned int);

        for (unsigned int i = 0; i < m_Textures.size(); i++)
        {
            *(reinterpret_cast<unsigned int *>(pData)) = m_Textures[i].m_Height;
            pData += sizeof(unsigned int);

            *(reinterpret_cast<unsigned int *>(pData)) = m_Textures[i].m_Width;
            pData += sizeof(unsigned int);

            unsigned int length = sizeof(uint32_t) * (1u << (m_Textures[i].m_Height + m_Textures[i].m_Width));
            if(length)
            {
                memcpy(pData, m_Textures[i].m_pData, length);
                pData += length;
            }
        }

        *(reinterpret_cast<unsigned int *>(pData)) = m_Sectors.size();
        pData += sizeof(unsigned int);

        for (unsigned i = 0; i < m_Sectors.size(); i++)
        {
            *(reinterpret_cast<int *>(pData)) = m_Sectors[i].floor;
            pData += sizeof(int);

            *(reinterpret_cast<int *>(pData)) = m_Sectors[i].ceiling;
            pData += sizeof(int);

            *(reinterpret_cast<int *>(pData)) = m_Sectors[i].floorTexId;
            pData += sizeof(int);

            *(reinterpret_cast<int *>(pData)) = m_Sectors[i].ceilingTexId;
            pData += sizeof(int);

            if(m_Sectors[i].m_pLight) // Should always be true
            {
                unsigned int dummy;
                m_Sectors[i].m_pLight->Stream(pData, dummy);
            }
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

    memcpy(m_ColorPalette, iData, sizeof(m_ColorPalette));
    iData += sizeof(m_ColorPalette);

    unsigned nbTextures = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    for (unsigned int i = 0; i < nbTextures; i++)
    {
        KDMapData::Texture texture;

        texture.m_Height = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        texture.m_Width = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        texture.m_pData = nullptr;
        unsigned int length = sizeof(uint32_t) * (1u << (texture.m_Width + texture.m_Height));
        if(length)
        {
            texture.m_pData = new unsigned char[length];
            memcpy(texture.m_pData, iData, length);
            iData += length;
        }

        m_Textures.push_back(texture);
    }

    unsigned nbSectors = *(reinterpret_cast<const int *>(iData));
    iData += sizeof(int);

    for (unsigned int i = 0; i < nbSectors; i++)
    {
        KDMapData::Sector sector;

        sector.floor = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        sector.ceiling = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        sector.floorTexId = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        sector.ceilingTexId = *(reinterpret_cast<const int *>(iData));
        iData += sizeof(int);

        unsigned int read = 0u;
        sector.m_pLight = std::shared_ptr<Light>(UnstreamLight(iData, read));
        iData += read;

        m_Sectors.push_back(sector);
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

    ComputeDynamicColorPalettes();
}

unsigned int KDTreeMap::ComputeStreamSize() const
{
    unsigned int streamSize = 0;

    streamSize += 3 * sizeof(int); // m_PlayerStartX, m_PlayerStartY, m_PlayerStartDirection

    streamSize += sizeof(m_ColorPalette);

    streamSize += sizeof(unsigned int); // m_Textures.size()

    for(unsigned int i = 0; i < m_Textures.size(); i++)
    {
        streamSize +=  2 * sizeof(unsigned int); // m_Height and m_Width
        streamSize += (1u << (m_Textures[i].m_Height + m_Textures[i].m_Width)) * sizeof(uint32_t); // RGBA assumed
    }

    streamSize += sizeof(unsigned int); // m_Sectors.size()
    streamSize += 4 * m_Sectors.size() * sizeof(int); // sector's floor, ceiling, floorTexId and ceilingTexId

    // sector light
    for (unsigned int i = 0; i < m_Sectors.size(); i++)
    {
        if(m_Sectors[i].m_pLight) // Should *always* be true
            streamSize += m_Sectors[i].m_pLight->ComputeStreamSize();
    }

    if (m_RootNode)
        streamSize += m_RootNode->ComputeStreamSize();
    
    return streamSize;
}

CType KDTreeMap::GetPlayerStartX() const
{
    return CType(m_PlayerStartX) / POSITION_SCALE;
}

CType KDTreeMap::GetPlayerStartY() const
{
    return CType(m_PlayerStartY) / POSITION_SCALE;
}

int KDTreeMap::GetPlayerStartDirection() const
{
    return m_PlayerStartDirection;
}

int KDTreeMap::ComputeDepth() const
{
    if(m_RootNode)
        return m_RootNode->ComputeDepth();
    else
        return 0;
}

void KDTreeMap::ComputeDynamicColorPalettes()
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int factor = 15u;
        for (unsigned int j = 0; j < 16; j++)
        {
            const unsigned char *pSrcColPtr = reinterpret_cast<const unsigned char *>(&m_ColorPalette[i]);
            unsigned char *pDestColPtr = reinterpret_cast<unsigned char *>(&m_DynamicColorPalettes[j][i]);
            for(unsigned int k = 0; k < 3; k++)
            {
                unsigned int srcComp = static_cast<unsigned int>(*pSrcColPtr++);
                unsigned int destComp = (srcComp * factor) >> 8u;
                *pDestColPtr++ = destComp;
            }
            *pDestColPtr = 255u;
            factor += 15u;
        }
    }
}
