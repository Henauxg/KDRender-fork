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
    m_PlayerHorizontalFOV(60),
    m_PlayerVerticalFOV((m_PlayerHorizontalFOV * WINDOW_HEIGHT) / WINDOW_WIDTH),
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

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerHorizontalFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerHorizontalFOV / 2, m_FrustumToRight);
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
            // Render walls within split plane
            for (unsigned int i = 0; i < pNode->m_Walls.size(); i++)
            {
                int minAngle = m_PlayerHorizontalFOV / 2, maxAngle = -m_PlayerHorizontalFOV / 2;
                Vertex minVertex, maxVertex;

                bool vertexFromInsideFrustum = isInsideFrustum(wall.m_VertexFrom);
                bool vertexToInsideFrustum = isInsideFrustum(wall.m_VertexTo);

                if (!vertexFromInsideFrustum || !vertexToInsideFrustum)
                {
                    Vertex intersectionVertex;
                    if (HalfLineSegmentIntersection(m_PlayerPosition, m_FrustumToLeft, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                    {
                        minVertex = intersectionVertex;
                        minAngle = -m_PlayerHorizontalFOV / 2;
                    }
                    if (HalfLineSegmentIntersection(m_PlayerPosition, m_FrustumToRight, wall.m_VertexFrom, wall.m_VertexTo, intersectionVertex))
                    {
                        maxVertex = intersectionVertex;
                        maxAngle = m_PlayerHorizontalFOV / 2;
                    }
                }

                auto updateMinMaxAnglesAndVertices = [&](int iAngle, const Vertex &iVertex)
                {
                    if (iAngle < minAngle)
                    {
                        minAngle = iAngle;
                        minVertex = iVertex;
                    }
                    if (iAngle > maxAngle)
                    {
                        maxAngle = iAngle;
                        maxVertex = iVertex;
                    }
                };

                if(vertexFromInsideFrustum)
                {
                    int angle = Angle(m_PlayerPosition, m_Look, wall.m_VertexFrom);
                    updateMinMaxAnglesAndVertices(angle, wall.m_VertexFrom);
                }

                if (vertexToInsideFrustum)
                {
                    int angle = Angle(m_PlayerPosition, m_Look, wall.m_VertexTo);
                    updateMinMaxAnglesAndVertices(angle, wall.m_VertexTo);
                }

                // Should always be true
                if(minAngle <= maxAngle)
                    RenderWall(wall, minVertex, maxVertex, minAngle, maxAngle);
            }
        }
    }

    if (positiveSide && pNode->m_NegativeSide)
        RenderNode(pNode->m_NegativeSide);
    else if (!positiveSide && pNode->m_PositiveSide)
        RenderNode(pNode->m_PositiveSide);
}

void KDTreeRenderer::RenderWall(const Wall &iWall, const Vertex &iMinVertex, const Vertex &iMaxVertex, int iMinAngle, int iMaxAngle)
{
    int minX = ((iMinAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV;
    int maxX = ((iMaxAngle + m_PlayerHorizontalFOV / 2) * WINDOW_WIDTH) / m_PlayerHorizontalFOV;
    minX = Clamp(minX, 0, WINDOW_WIDTH - 1);
    maxX = Clamp(maxX, 0, WINDOW_WIDTH - 1);

    int minDist = DistInt(m_PlayerPosition, iMinVertex) * cosInt(iMinAngle) / DECIMAL_MULT; // Correction for distortion
    int maxDist = DistInt(m_PlayerPosition, iMaxVertex) * cosInt(iMaxAngle) / DECIMAL_MULT; // Correction for distortion

    // Hard wall
    if (iWall.m_pKDWall->m_OutSector == -1 && WhichSide(iWall.m_VertexFrom, iWall.m_VertexTo, m_PlayerPosition) > 0)
    {
        int eyeToTop = m_Map.m_Sectors[iWall.m_pKDWall->m_InSector].ceiling - m_PlayerZ;
        int eyeToBottom = m_Map.m_Sectors[iWall.m_pKDWall->m_InSector].floor + m_PlayerZ;

        int minVertexBottomPixel = ((-atanInt(DECIMAL_MULT * eyeToBottom / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        int minVertexTopPixel = ((atanInt(DECIMAL_MULT * eyeToTop / minDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        Clamp(minVertexBottomPixel, 0, WINDOW_HEIGHT - 1);
        Clamp(minVertexTopPixel, 0, WINDOW_HEIGHT - 1);

        int maxVertexBottomPixel = ((-atanInt(DECIMAL_MULT * eyeToBottom / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        int maxVertexTopPixel = ((atanInt(DECIMAL_MULT * eyeToTop / maxDist) + m_PlayerVerticalFOV / 2) * WINDOW_HEIGHT) / m_PlayerVerticalFOV;
        Clamp(maxVertexBottomPixel, 0, WINDOW_HEIGHT - 1);
        Clamp(maxVertexTopPixel, 0, WINDOW_HEIGHT - 1);

        for (unsigned int x = minX; x < maxX; x++)
        {
            if (!m_pHorizOcclusionBuffer[x])
            {
                unsigned int t = ((x - minX) * DECIMAL_MULT ) / (maxX - minX);
                unsigned int minY = ((DECIMAL_MULT - t) * minVertexBottomPixel + t * maxVertexBottomPixel) / DECIMAL_MULT;
                unsigned int maxY = ((DECIMAL_MULT - t) * minVertexTopPixel + t * maxVertexTopPixel) / DECIMAL_MULT;
                minY = std::max<unsigned int>(minY, m_pBottomOcclusionBuffer[x]);
                maxY = std::min<unsigned int>(maxY, WINDOW_HEIGHT - 1 - m_pTopOcclusionBuffer[x]);

                int color = (255u * (DECIMAL_MULT - t)) / DECIMAL_MULT;
                for (unsigned int y = minY; y <= maxY; y++)
                {
                    WriteFrameBuffer((WINDOW_HEIGHT - 1 - y) * WINDOW_WIDTH + x, color, 0u, 0u);
                }
            }
        }
        memset(m_pHorizOcclusionBuffer + minX, 1u, maxX - minX);
    }
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

bool KDTreeRenderer::isInsideFrustum(const Vertex &iVertex) const
{
    return (WhichSide(m_PlayerPosition, m_FrustumToLeft, iVertex) >= 0) &&
           (WhichSide(m_PlayerPosition, m_FrustumToRight, iVertex) <= 0);
}

void KDTreeRenderer::SetPlayerCoordinates(const KDTreeRenderer::Vertex &iPosition, int iDirection)
{
    m_PlayerPosition = iPosition;
    m_PlayerDirection = iDirection;

    GetVector(m_PlayerPosition, m_PlayerDirection - m_PlayerHorizontalFOV / 2, m_FrustumToLeft);
    GetVector(m_PlayerPosition, m_PlayerDirection + m_PlayerHorizontalFOV / 2, m_FrustumToRight);
    GetVector(m_PlayerPosition, m_PlayerDirection, m_Look);
}

KDTreeRenderer::Vertex KDTreeRenderer::GetPlayerPosition() const
{
    return m_PlayerPosition;
}

int KDTreeRenderer::GetPlayerDirection() const
{
    return m_PlayerDirection;
}

KDTreeRenderer::Vertex KDTreeRenderer::GetLook() const
{
    return m_Look;
}