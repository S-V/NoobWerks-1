// Binary Space Partitioning
// TODO: fix degenerate normal and dist
#pragma once

#include <Base/Base.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>	// TInplaceArray
#include <Core/Core.h>

// for VertexDescription
#include <GPU/Public/graphics_types.h>
#include <Utility/MeshLib/TriMesh.h>
#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/Volumes.h>

namespace Meshok
{
namespace BSP
{

/// Prefer double precision for BSP-based CSG.
//typedef float Real;
typedef double Real;


MAKE_ALIAS( HFace, U32 );	//!< face index
MAKE_ALIAS( HNode, U32 );	//!< node index - upper two bits describe the type of the node
MAKE_ALIAS( HPlane, U32 );	//!< plane index

const HFace NIL_FACE(~0);
const HNode NIL_NODE(~0);

enum { BSP_MAX_NODES = (1U<<30)-1 };
enum { BSP_MAX_DEPTH = 32 };	// size of temporary stack storage (we try to avoid recursion)
enum { BSP_MAX_PLANES = MAX_UINT32-1 };	// maximum allowed number of planes in a single tree
enum { BSP_MAX_POLYS = MAX_UINT32-1 };


struct BSP_Vertex : public Meshok::Vertex
{
public:
	mxDECLARE_CLASS(BSP_Vertex,CStruct);
	mxDECLARE_REFLECTION;
};
mxSTATIC_ASSERT_ISPOW2(sizeof(BSP_Vertex));


struct BSP_Face: CStruct
{
	TArray< BSP_Vertex >	vertices;	//!< Vertices
	HFace					next;		//!< Next coplanar polygon, if any (also used during building)
	BSP_Vertex				buffer[3];	//!< small embedded storage to avoid memory allocations
public:
	mxDECLARE_CLASS(BSP_Face,CStruct);
	mxDECLARE_REFLECTION;
	BSP_Face()
	{
		vertices.initializeWithExternalStorageAndCount( buffer, mxCOUNT_OF(buffer) );
		next = NIL_FACE;
	}
};

/// This enum describes the allowed BSP node types.
enum NODE_TYPE
{
	INTERNAL_NODE	= 0,	//!< An internal (auto-)partitioning node (with polygon-aligned splitting plane).
	NodeType_Split	= 1,	//!< An internal partitioning node (with arbitrary splitting plane).
	//PSEUDO_LEAF		= 1,	//!< Level of Detail BSP (Used by progressive BSP trees).
	SOLID_LEAF		= 2,	//!< An incell leaf node ( representing solid matter ).
	EMPTY_LEAF		= 3,	//!< An outcell leaf node ( representing empty space ).
	NodeType_MAX	// Marker. Do not use.
};

inline bool IS_LEAF( UINT nodeID ) {
	return (nodeID & (~0U<<30)) != 0;
}
inline bool IS_INTERNAL( UINT nodeID ) {
	return (nodeID & (~0U<<30)) == 0;
}
inline HNode MAKE_LEAF_ID( UINT payload, NODE_TYPE type ) {
	return HNode( (type<<30) | payload );
}
inline NODE_TYPE GET_TYPE( UINT nodeID ) {
	return NODE_TYPE((nodeID >> 30) & 3);
}
inline UINT16 GET_PAYLOAD( UINT nodeID ) {
	return nodeID & ~(~0U<<30);
}
inline bool IS_SOLID_LEAF( UINT nodeID ) {
	return GET_TYPE(nodeID) == SOLID_LEAF;
}
inline bool IS_EMPTY_LEAF( UINT nodeID ) {
	return GET_TYPE(nodeID) == EMPTY_LEAF;
}

// NOTE: it's possible to reduce the size of each Node to just 4 bytes,
// e.g., by storing the second child right after the first one (useful for static BSP without polygons).
struct BSP_Node: CStruct
{
	HPlane	plane;	//«4 Hyperplane of the node (index into array of planes).
	HNode	back;	//«4 Index of the left child (negative subspace, behind the plane).
	HNode	front;	//«4 Index of the right child (positive subspace, in front of plane).
	HFace	faces;	//«4 linked list of polygons lying on this node's plane (optional).
public:
	mxDECLARE_CLASS(BSP_Node,CStruct);
	mxDECLARE_REFLECTION;
	BSP_Node()
	{
		plane = HPlane(~0);
		back = MAKE_LEAF_ID( ~0, SOLID_LEAF );	// the back side always points at solid space
		front = MAKE_LEAF_ID( ~0, EMPTY_LEAF );	// the front side always points at empty space
		faces = NIL_FACE;
	}
};
mxSTATIC_ASSERT(sizeof(BSP_Node)==16);


//#define THICK_PLANE_SIDE_EPSILON	(0.25f)
//#define NORMAL_PLANE_SIDE_EPSILON	(0.1f)
//#define PRECISE_PLANE_SIDE_EPSILON	(0.01f)

/// The global thickness value for all BSP planes - any vertex within that distance is classified as "on".
#define	ON_EPSILON			0.1f
#define	NORMAL_EPSILON		0.00001f
#define	DIST_EPSILON		0.01f

struct SplittingCriteria
{
	float	splitCost;

	// ratio balance of front/back polygons versus split polygons,
	// must be in range [0..1],
	// 1 - prefer balanced tree, 0 - avoid splitting polygons
	//
	float	balanceVsCuts;

	// slack value for testing points wrt planes
	float	planeDistanceEpsilon;

	float	planeNormalEpsilon;	//!< for comparing plane normals when building a tree

public:
	SplittingCriteria()
	{
		splitCost = 1.0f;
		balanceVsCuts = 0.6f;
		planeDistanceEpsilon = 0.01f;
		planeNormalEpsilon = 0.00001f;
	}
	void PresetsForCollisionDetection()
	{
		splitCost = 0;	// for collision detection we don't need to store polygons
		balanceVsCuts = 1.0f;	// we prefer balanced trees
		planeDistanceEpsilon = 0.01f;
		planeNormalEpsilon = 0.00001f;
	}
};

/*
-----------------------------------------------------------------------------
	stats for testing & debugging
-----------------------------------------------------------------------------
*/
class BspStats {
public:
	U32		m_polysBefore;
	U32		m_polysAfter;	// number of resulting polygons
	U32		m_numSplits;	// number of cuts caused by BSP

	U32		m_numInternalNodes;
	U32		m_numPlanes;
	//U32		depth;
	U32		m_numSolidLeaves, m_numEmptyLeaves;

	U32		m_bytesAllocated;
public:
	BspStats();
	void Reset();
	void Print( U32 elapsedTimeMSec );
};

/*
-----------------------------------------------------------------------------
	Polygon-aligned (auto-partitioning) node-storing solid leaf-labeled BSP tree.
	Leaf nodes are not explicitly stored.
	It can be used for ray casting, collision detection and CSG operations.
-----------------------------------------------------------------------------
*/
struct BSP_Tree: CStruct
{
	DynamicArray< BSP_Node >	m_nodes;	//!< tree nodes (0 = root index) (16 bytes per each)
	DynamicArray< Vector4 >		m_planes;	//!< unique splitting plane equations (16 bytes per plane)
	DynamicArray< BSP_Face >	m_faces;	//!< convex polygons
	float				m_epsilon;	//!< plane 'thickness' epsilon used for building the tree

public:
	mxDECLARE_CLASS(BSP_Tree,CStruct);
	mxDECLARE_REFLECTION;
	BSP_Tree( AllocatorI & _allocator );

public:	// Construction.
	struct Settings : SplittingCriteria
	{
		bool	store_polygons;
	};

	ERet Build( const TriMesh& mesh, const Settings& settings = Settings() );

	ERet Build2( const TriMesh& mesh, AllocatorI & scratch, const Settings& settings = Settings() );

	// Returns the total amount of occupied memory in bytes.
	size_t BytesAllocated() const;

	bool IsEmpty() const;

	void RemoveFaceData();

public:	// Testing & Debugging.
	bool Debug_Validate() const;

public:	// Serialization.

	struct Header_d {
		U32	num_nodes;
		U32	num_planes;
		F32	plane_epsilon;	//!< plane 'thickness' epsilon used for building the tree
		U32	unused_padding;
	};

	/// Header_d, nodes, planes
	ERet SaveToBlob(
		NwBlobWriter & _stream
	) const;

public:	//
	void Translate( const V3f& _translation );
	void Scale( const float scale );
	/// Transforms all planes by an angle preserving transform.
	void TransformBy( const M44f& _matrix );
	void Transform( const V3f& _translation, const float _scaling );

	void DbgPrint();

public:	// Point-In-Solid

	bool PointInSolid( const V3f& point ) const;

	// Ray casting

	/// returns no intersection if the ray is fully inside or outside the solid.
	bool CastRay(
		const V3f& start, const V3f& direction,
		float tmin, float tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
		float *thit, V3f *hitNormal,
		const bool startInSolid = false
	) const;

	// Line/Intersection testing

	struct Intersection
	{
		float	time;	// [0..1] if hit
		HPlane	plane;
	};

	//bool IntersectsLine(
	//	const V3f& _start, const V3f& _end,
	//	Intersection &_result
	//	) const
	//{
	//	const V3f direction = V3_Normalized(_end - _start);
	//	const float tmin = 0.0f;
	//	const float tmax = 1.0f;
	//	return IntersectsLineR(_start, direction, 0.0f, 1.0f, HNode(0), _result);
	//}
	struct Intersection2
	{
		float	tmin;
		float	tmax;
		HPlane	plane;
	public:
		Intersection2()
		{
			tmin = +BIG_NUMBER;
			tmax = -BIG_NUMBER;
			plane = HPlane(~0);
		}
	};

#if 0
	mxSTOLEN("Serious engine, BSPTree<Type, iDimensions>::FindLineMinMax()");
	void IntersectsLineR(
		const V3f& v0, const V3f& v1,
		const float t0, const float t1,
		const HNode nodeIndex,
		Intersection2 &_result
		) const
	{
		if( IS_LEAF( nodeIndex ) )
		{
			if( IS_SOLID_LEAF( nodeIndex ) )
			{
				_result.tmin = minf(_result.tmin, t0);
				_result.tmax = maxf(_result.tmax, t1);
			}
		}
		else
		{
			const BSP_Node& node = this->m_nodes[ nodeIndex ];
			const Vector4& plane = this->m_planes[ node.plane ];
			// get the distances of each point from the splitting plane
			const float d0 = Plane_PointDistance( plane, v0 );
			const float d1 = Plane_PointDistance( plane, v1 );

			_result.plane = node.plane;

			if( d0 >= 0 && d1 >= 0 )
			{
				// both points are in front of the plane
				IntersectsLineR( v0, v1, t0, t1, node.front, _result );
			}
			else if( d0 < 0 && d1 < 0 )
			{
				// both points are behind the plane
				IntersectsLineR( v0, v1, t0, t1, node.back, _result );
			}
			else
			{
				// the line spans the splitting plane - check both children of the node
				const float tm = d0 / ( d0 - d1 );
				const V3f mid = v0 + (v1 - v0) * tm;

				const int nearIndex = (d0 >= 0);	// child index for half-space containing the start point: 1 - front, 0 - back
				const HNode sides[2] = { node.back, node.front };

				IntersectsLineR( v0, mid, t0, tm, sides[nearIndex], _result );
				IntersectsLineR( mid, v1, tm, t1, sides[nearIndex^1], _result );
			}
		}
	}
#endif

#if 0
	// returns > 0, if outside
	// negative distance - the point is inside solid region
	float DistanceToPoint( const V3f& point, float epsilon = ON_EPSILON ) const
	{
		HNode	iNode(0);// root
		HNode	iParent(NIL_NODE);
		float	minimumDistance = BIG_NUMBER;
		while( IS_INTERNAL( iNode ) )
		{
			const BSP_Node& node = m_nodes[ iNode ];
			const Vector4& plane = m_planes[ node.plane ];
			// find which side of the node we are on
			float distance = Plane_PointDistance( mxTEMP(V4f&)plane, point );
			const int nearIndex = distance > 0.0f;
			const HNode sides[2] = { node.back, node.front };
			// Go down the appropriate side
			iParent = iNode;
			iNode = sides[ nearIndex ];
			if( fabs(distance) < fabs(minimumDistance) ) {
				minimumDistance = distance;
			}
		}
		return minimumDistance;
	}
#endif

#if 0
	bool CastRay2(
		const V3f& start, const V3f& direction,
		float tmin, float tmax,
		float *tHit, V3f *hitNormal
		) const
	{
		mxASSERT(m_nodes.num());

		enum { StackSize = 256 };
		struct StackEntry
		{
			HNode node;
			HNode parent;
			float time;
		};
		StackEntry stack[StackSize];

		V3f normal;
		int stackp = 0;

		float PlaneThickness = ON_EPSILON;
		float IntersectEpsilon = 0;//2.0f * PlaneThickness;

/*
CenterPos - in world coordinate system
		sRay ray = inRay;
		ray.Start -= CenterPos;
*/

		HNode	iNode(0);// root
		HNode	iParent(NIL_NODE);
		for(;;)
		{
			if( IS_INTERNAL( iNode ) )
			{
				const BSP_Node& node = m_nodes[ iNode ];
				const Vector4& plane = m_planes[ node.plane ];

				float dist = Plane_PointDistance( plane, start );
				int nearIndex = dist < 0.0f;
				const HNode sides[2] = { node.back, node.front };

				float denom = V3_Dot( Plane_GetNormal(plane), direction );

				if(denom != 0.0f)
				{
					float t = -dist / denom;
					if(0.0f <= t && t <= tmax+IntersectEpsilon)
					{
						if(t >= tmin-IntersectEpsilon)
						{
							stack[stackp].node = sides[1-nearIndex];
							stack[stackp].time = tmax;
							stack[stackp].parent = iNode;
							stackp++;
							tmax = t;
						}
						else
							nearIndex = 1 - nearIndex;
					}
				}
				else if(mmAbs(dist) < IntersectEpsilon)
				{
					stack[stackp].node = sides[1-nearIndex];
					stack[stackp].time = tmax;
					stackp++;
				}

				iParent = iNode;
				iNode = sides[nearIndex];
			}
			else
			{
				if( IS_SOLID_LEAF( iNode ) )
				{
					*hitNormal = normal;
					*tHit = tmin;
					return true;
				}

				if(!stackp)
					break;

				normal = Plane_GetNormal( m_planes[ m_nodes[iParent].plane ] );
				tmin = tmax;
				stackp--;
				iNode = stack[stackp].node;
				tmax = stack[stackp].time;
				iParent = stack[stackp].parent;
			}
		}

		return false;
	}
#endif

	// overlap testing

	// sweep tests

	// oriented capsule collision detection

public:	// Hermite data

	class Sampler : public VX::AVolumeSampler
	{
		const BSP_Tree &	m_tree;
		/*const*/ V3f	m_cellSize;	//!< the size of each dual cell
		/*const*/ V3f	m_offset;	//!< the world-space offset of each sample

	public:
		Sampler( const BSP_Tree& _tree, const AABBf& _bounds, const Int3& _resolution );

		virtual bool IsInside(
			int iCellX, int iCellY, int iCellZ
		) const override;

		/// returns no intersection if the ray is fully inside or outside the solid.
		bool CastRay(
			const V3f& start, const V3f& direction,
			float tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
			float *thit, V3f *hitNormal,
			const bool startInSolid = false
		) const;

		virtual U32 SampleHermiteData(
			int iCellX, int iCellY, int iCellZ,
			VX::HermiteDataSample & _sample
		) const override;
	};

private:
	ERet Build_Traditional( const TriMesh& mesh, const Settings& settings );
	ERet Build_Without_Polygons( const TriMesh& mesh, const Settings& settings );

	const HNode BuildTreeRecursive( const HFace polygons, const Settings& settings );

	const HFace FindBestSplitter(
		const HFace polygons,
		const SplittingCriteria& options
	);

	void PartitionPolygons(
		const Vector4& partitioner,	// splitting plane
		const HFace polygons,	// linked list
		HFace *frontFaces,	// linked list
		HFace *backFaces,	// linked list
		HFace *coplanar,	// linked list
		UINT faceCounts[4],	// stats (poly counts for each EPlaneSide)
		const float epsilon = ON_EPSILON
	);

	const HPlane AddUniquePlane(
		const Vector4& plane,
		const float normalEps = NORMAL_EPSILON,
		const float distEps = DIST_EPSILON
	);
};

#if 0
	// This enum describes the allowed BSP node types.
	enum ENodeType
	{
		BN_Polys,	// An internal (auto-)partitioning node (with polygon-aligned splitting plane).
		BN_Solid,	// An incell leaf node ( representing solid space ).
		BN_Empty,	// An outcell leaf node ( representing empty space ).
		BN_Split,	// An internal partitioning node (with arbitrary splitting plane).
		// Used by progressive BSP trees.
		BN_Undecided,
		//// For axis-aligned splitting planes.
		//BN_Plane_YZ,	// X axis
		//BN_Plane_XY,	// Z axis
		//BN_Plane_XZ,	// Y axis

		BN_MAX	// Marker. Do not use.
	};

	// generic BSP tree used as intermediate representation
	// for constructing specific optimized trees
	//
	struct genNode
	{
		TPtr< genNode >	pos;
		TPtr< genNode >	neg;
	};

	struct genTree
	{
		TPtr< genNode >	root;

		void Build( pxTriangleMeshInterface* mesh );
	};

	//
	//	ENodeState
	//
	enum ENodeState
	{
		Unchanged,	// Node has not been modified.
		LeftChild,	// The left child has not been modified.
		RightChild,	// The right child has not been modified.
	};
#endif

}//namespace BSP
}//namespace Meshok

//HACK: Reflection - must be in the global scope!
mxREFLECT_AS_BUILT_IN_INTEGER(Meshok::BSP::HFace);
mxREFLECT_AS_BUILT_IN_INTEGER(Meshok::BSP::HNode);
mxREFLECT_AS_BUILT_IN_INTEGER(Meshok::BSP::HPlane);

/*
some stats for "cow.ply" (17412 vertices, 5804 triangles):
"cow.ply" : 173 KiB
"cow.bsp" : 52.8 KiB (built in ~300 seconds)
"cow.bsp" with polys : 387 KiB (built in ~306 seconds)
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
