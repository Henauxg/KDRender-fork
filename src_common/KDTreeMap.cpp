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

KDTreeMap::KDTreeMap():
    m_RootNode(nullptr)
{
    
}

KDTreeMap::~KDTreeMap()
{
    if(m_RootNode)
        delete m_RootNode;
    m_RootNode = nullptr;
}

void KDTreeMap::Stream(char *&oData)
{
    // TODO
}

void KDTreeMap::UnStream(const char *iData)
{
    // TODO
}

unsigned int KDTreeMap::ComputeStreamSize()
{
    // TODO
    return 0;
}