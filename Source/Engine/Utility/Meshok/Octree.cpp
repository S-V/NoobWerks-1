#include "stdafx.h"
#pragma hdrstop
#include <Meshok/Meshok.h>
#include <Meshok/Octree.h>

#if 0
/*
Implicit node representations
Linear (hashed) Octrees
https://geidav.wordpress.com/2014/08/18/advanced-octrees-2-node-representations/
*/
size_t Octree::GetNodeTreeDepth(const OctreeNode *node)
{
    assert(node->LocCode); // at least flag bit must be set
    // for (U32_t lc=node->LocCode, depth=0; lc!=1; lc>>=3, depth++);
    // return depth;
 
#if defined(__GNUC__)
    return (31-__builtin_clz(node->LocCode))/3;
#elif defined(_MSC_VER)
    long msb;
	// Search the mask data from most significant bit (MSB) to least significant bit (LSB) for a set bit (1).
    _BitScanReverse(&msb, node->LocCode);
    return msb/3;
#endif
}
#endif

const UINT8* CUBE_GetEdgeIndices()
{
	static const UINT8 edgeIndices[ 12*2 ] =
	{
		// bottom
		0, 2,
		2, 3,
		3, 1,
		1, 0,

		// middle - bottom to top
		0, 4,
		1, 5,
		3, 7,
		2, 6,

		// top
		4, 5,
		5, 7,
		7, 6,
		6, 4
	};
	return edgeIndices;
}
#if 0
const U32 g_bitCounts8[256] =
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

mxDEFINE_CLASS(FeatureVertex);
mxBEGIN_REFLECTION(FeatureVertex)
	mxMEMBER_FIELD(xyz),
	mxMEMBER_FIELD(N),
mxEND_REFLECTION
FeatureVertex::FeatureVertex()
{
}

mxDEFINE_CLASS(MeshOctreeNode);
mxBEGIN_REFLECTION(MeshOctreeNode)
	mxMEMBER_FIELD(features),
	mxMEMBER_FIELD(kids),
mxEND_REFLECTION
MeshOctreeNode::MeshOctreeNode()
{
}

mxDEFINE_CLASS(MeshOctree);
mxBEGIN_REFLECTION(MeshOctree)
	mxMEMBER_FIELD(m_nodes),
mxEND_REFLECTION
MeshOctree::MeshOctree()
{
}

ERet MeshOctree::Build( ATriangleMeshInterface* triangleMesh )
{
	UNDONE;
#if 0
const U32 startTimeMSec = mxGetTimeInMilliseconds();

	struct CollectTriangles : ATriangleIndexCallback
	{
		Tree &	m_tree;
		CollectTriangles( Tree & tree ) : m_tree( tree )
		{}
		virtual void ProcessTriangle( const Vertex& a, const Vertex& b, const Vertex& c ) override
		{
			BspPoly & triangle = m_tree.m_polys.add();
			triangle.vertices.setNum(3);
			BspVertex & v1 = triangle.vertices[0];
			BspVertex & v2 = triangle.vertices[1];
			BspVertex & v3 = triangle.vertices[2];
			v1.xyz = a.xyz;
			v2.xyz = b.xyz;
			v3.xyz = c.xyz;
			v1.st = V2_To_Half2(a.st);
			v2.st = V2_To_Half2(b.st);
			v3.st = V2_To_Half2(c.st);
			triangle.next = NIL_ID;
		}
	};

	CollectTriangles	callback( *this );
	triangleMesh->ProcessAllTriangles( &callback );

	chkRET_X_IF_NOT(m_polys.num() > 0, ERR_INVALID_PARAMETER);

	// setup a linked list of polygons
	for( int iPoly = 0; iPoly < m_polys.num()-1; iPoly++ )
	{
		m_polys[iPoly].next = iPoly+1;
	}

	BspStats	stats;
	stats.m_polysBefore = m_polys.num();

	BuildTree_R( *this, 0, stats );

	stats.m_polysAfter = m_polys.num();

	m_nodes.shrink();
	m_planes.shrink();
	m_polys.shrink();

	stats.m_numInternalNodes = m_nodes.num();
	stats.m_numPlanes = m_planes.num();
	stats.m_bytesAllocated = this->BytesAllocated();

	const U32 currentTimeMSec = mxGetTimeInMilliseconds();
	stats.Print(currentTimeMSec - startTimeMSec);
#endif
	return ALL_OK;
}

// copied from
// http://www.gamedev.net/topic/552906-closest-point-on-triangle/
// which is based on
// http://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
V3f ClosestPointOnTriangle( const V3f triangle[3], const V3f& sourcePosition )
{
    V3f edge0 = triangle[1] - triangle[0];
    V3f edge1 = triangle[2] - triangle[0];
    V3f v0 = triangle[0] - sourcePosition;

    float a = V3_Dot( edge0, edge0 );
    float b = V3_Dot( edge0, edge1 );
    float c = V3_Dot( edge1, edge1 );
    float d = V3_Dot( edge0, v0 );
    float e = V3_Dot( edge1, v0 );

    float det = a*c - b*b;
    float s = b*e - c*d;
    float t = b*d - a*e;

    if ( s + t < det )
    {
        if ( s < 0.f )
        {
            if ( t < 0.f )
            {
                if ( d < 0.f )
                {
                    s = clampf( -d/a, 0.f, 1.f );
                    t = 0.f;
                }
                else
                {
                    s = 0.f;
                    t = clampf( -e/c, 0.f, 1.f );
                }
            }
            else
            {
                s = 0.f;
                t = clampf( -e/c, 0.f, 1.f );
            }
        }
        else if ( t < 0.f )
        {
            s = clampf( -d/a, 0.f, 1.f );
            t = 0.f;
        }
        else
        {
            float invDet = 1.f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        if ( s < 0.f )
        {
            float tmp0 = b+d;
            float tmp1 = c+e;
            if ( tmp1 > tmp0 )
            {
                float numer = tmp1 - tmp0;
                float denom = a-2*b+c;
                s = clampf( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                t = clampf( -e/c, 0.f, 1.f );
                s = 0.f;
            }
        }
        else if ( t < 0.f )
        {
            if ( a+d > b+e )
            {
                float numer = c+e-b-d;
                float denom = a-2*b+c;
                s = clampf( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                s = clampf( -e/c, 0.f, 1.f );
                t = 0.f;
            }
        }
        else
        {
            float numer = c+e-b-d;
            float denom = a-2*b+c;
            s = clampf( numer/denom, 0.f, 1.f );
            t = 1.f - s;
        }
    }

    return triangle[0] + edge0 * s + edge1 * t;
}

//float MeshSDF::DistanceTo( const V3f& _position ) const
//{
//UNDONE;
//return 0;
//}
#endif

/*
https://geidav.wordpress.com/2014/08/18/advanced-octrees-2-node-representations/
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
