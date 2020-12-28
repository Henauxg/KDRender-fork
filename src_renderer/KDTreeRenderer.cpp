#include "KDTreeRenderer.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "GeomUtils.h"

#include <cstring>

namespace
{
    KDTreeRenderer::Wall GetWallFromNode(KDTreeNode *ipNode, unsigned int iWallIdx)
    {
        KDTreeRenderer::Wall wall;

        if(ipNode)
        {
            const KDMapData::Wall &kdWall = ipNode->GetWall(iWallIdx);
            wall.m_pKDWall = &kdWall;

            if(ipNode->GetSplitPlane() == KDTreeNode::SplitPlane::XConst)
            {
                wall.m_VertexFrom.m_X = ipNode->GetSplitOffset();
                wall.m_VertexTo.m_X = wall.m_VertexFrom.m_X;

                wall.m_VertexFrom.m_Y = kdWall.m_From;
                wall.m_VertexTo.m_Y = kdWall.m_To;
            }
            else
            {
                wall.m_VertexFrom.m_Y = ipNode->GetSplitOffset();
                wall.m_VertexTo.m_Y = wall.m_VertexFrom.m_Y;

                wall.m_VertexFrom.m_X = kdWall.m_From;
                wall.m_VertexTo.m_X = kdWall.m_To;
            }
        }

        return wall;
    }
}

KDTreeRenderer::KDTreeRenderer(const KDTreeMap &iMap) :
    m_Map(iMap),
    m_pFrameBuffer(new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4u]),
    m_pHorizOcclusionBuffer(new unsigned char[WINDOW_WIDTH]),
    m_pTopOcclusionBuffer(new int[WINDOW_WIDTH]),
    m_pBottomOcclusionBuffer(new int[WINDOW_WIDTH]),
    m_PlayerFOV(90),
    m_PlayerHeight(30)
{
    ClearBuffers();
}

const unsigned char* KDTreeRenderer::GetFrameBuffer() const
{
    return m_pFrameBuffer;
}

unsigned char *KDTreeRenderer::GetFrameBuffer() 
{
    return m_pFrameBuffer;
}

void KDTreeRenderer::FillFrameBufferWithColor(unsigned char r, unsigned char g, unsigned char b)
{
    // Loop because memset can't take anything bigger than a char as an input
    for (unsigned int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT * 4u; i+= 4u)
    {
        m_pFrameBuffer[i + 0] = r;
        m_pFrameBuffer[i + 1] = g;
        m_pFrameBuffer[i + 2] = b;
    }
}

void KDTreeRenderer::ClearBuffers()
{
    memset(m_pFrameBuffer, 255u, sizeof(unsigned char) * 4u * WINDOW_HEIGHT * WINDOW_WIDTH);
    memset(m_pHorizOcclusionBuffer, 0u, sizeof(unsigned char) * WINDOW_WIDTH);
    memset(m_pTopOcclusionBuffer, 0, sizeof(int) * WINDOW_WIDTH);
    memset(m_pBottomOcclusionBuffer, 0, sizeof(int) * WINDOW_WIDTH);
}

void KDTreeRenderer::RefreshFrameBuffer()
{
    FillFrameBufferWithColor(200u, 200u, 200u);

    Render();
}

void KDTreeRenderer::Render()
{
    m_PlayerZ = ComputeZ();

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerFOV / 2, m_FrustumToRight);
    GetVector(m_PlayerPosition, m_PlayerDirection, m_Look);

    RenderNode(m_Map.m_RootNode);
}

void KDTreeRenderer::RenderNode(KDTreeNode *pNode)
{
    // TODO: culling

    bool positiveSide;
    if (pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_PlayerPosition.m_X > pNode->m_SplitOffset;
    else
        positiveSide = m_PlayerPosition.m_Y > pNode->m_SplitOffset;

    if (positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
    else if (!positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);

    for (unsigned int i = 0; i < pNode->m_Walls.size(); i++)
    {
        KDTreeRenderer::Wall wall(GetWallFromNode(pNode, i));
        if ((WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom) <= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexTo) <= 0) ||
            (WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom) >= 0 &&
             WhichSide(m_PlayerPosition, m_FrustumToRight, wall.m_VertexTo) >= 0) ||
            (DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexFrom) <= 0 &&
             DotProduct(m_PlayerPosition, m_Look, m_PlayerPosition, wall.m_VertexTo) <= 0))
        {
            // Culling (wall is entirely outside frustum)
            continue;
        }
        else
        {
        }
    }

    if (positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);
    else if (!positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
}

int KDTreeRenderer::ComputeZ()
{
    return RecursiveComputeZ(m_Map.m_RootNode) + m_PlayerHeight;
}

int KDTreeRenderer::RecursiveComputeZ(KDTreeNode *pNode)
{
    if(!pNode)
        return 0; // Should not happen

    int oZ = 0;

    bool positiveSide;
    if(pNode->m_SplitPlane == KDTreeNode::SplitPlane::XConst)
        positiveSide = m_PlayerPosition.m_X > pNode->m_SplitOffset;
    else
        positiveSide = m_PlayerPosition.m_Y > pNode->m_SplitOffset;

    // We are on a terminal node
    // The node contains exactly one wall, which we are going to use
    // to find out in which sector the player is
    if ((positiveSide && !pNode->m_PositiveSide) ||
        (!positiveSide && !pNode->m_NegativeSide))
    {
        if(pNode->GetNbOfWalls()) // Should always be true
        {
            Wall wall(GetWallFromNode(pNode, 0));
            int whichSide = WhichSide(wall.m_VertexFrom, wall.m_VertexTo, m_PlayerPosition);
            if(whichSide < 0)
            {
                int outSector = wall.m_pKDWall->m_OutSector;
                if (outSector >= 0)
                    oZ = m_Map.m_Sectors[outSector].floor;
            }
            else
            {
                int inSector = wall.m_pKDWall->m_InSector;
                if (inSector >= 0)
                    oZ = m_Map.m_Sectors[inSector].floor;
            }
        }
    }
    // Recursive run
    else
    {
        if(positiveSide)
            return RecursiveComputeZ(pNode->m_PositiveSide);
        else
            return RecursiveComputeZ(pNode->m_NegativeSide);
    }

    return oZ;
}