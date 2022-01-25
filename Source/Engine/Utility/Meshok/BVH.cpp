// Bounding Volume Hierarchy
// Based on https://github.com/brandonpelfrey/Fast-BVH
#include "stdafx.h"
#pragma hdrstop
#include <algorithm>	// std::min
#include <Core/Memory.h>
#include <Meshok/Meshok.h>
#include <Meshok/BVH.h>

namespace Meshok
{

BVH::BVH(AllocatorI& allocator)
	: m_nodes(allocator)
	, m_indices(allocator)
{
}

/*! Build the BVH, given an input data set
 *  - Handling our own stack is quite a bit faster than the recursive style.
 *  - Each build stack entry's parent field eventually stores the offset
 *    to the parent of that node. Before that is finally computed, it will
 *    equal exactly three other values. (These are the magic values Untouched,
 *    Untouched-1, and TouchedTwice).
 *  - The partition here was also slightly faster than std::partition.
 */
ERet BVH::Build(
				const TriMeshI& _mesh,
				const BuildOptions& build_options
				, AllocatorI & scratchpad
				)
{
	const UINT numPrimitives = _mesh.numTriangles();

	/// temporary struct used for building a BVH
	struct Primitive
	{
		AABBf	bounds;
		V3f		center;	//!< precached center of the bounding box - for sorting along axes
	};
	DynamicArray< Primitive >	primitives( scratchpad );
	mxDO(primitives.setCountExactly(numPrimitives));

	mxDO(m_indices.setCountExactly(numPrimitives));

	// Compute the bounding box of the whole scene.
	AABBf sceneBounds;
	AABBf_Clear(&sceneBounds);

	// Precompute bounding boxes for all objects to store.
	for( UINT iTriangle = 0; iTriangle < numPrimitives; iTriangle++ )
	{
		V3f V1, V2, V3;
		_mesh.getTriangleAtIndex( iTriangle, V1, V2, V3 );

		//
		Primitive & primitive = primitives._data[ iTriangle ];

		primitive.bounds.min_corner = V3_Mins( V3_Mins( V1, V2 ), V3 );
		primitive.bounds.max_corner = V3_Maxs( V3_Maxs( V1, V2 ), V3 );
		primitive.center = AABBf_Center( primitive.bounds );

		// Increase the size of the bounding box to avoid numerical issues.
		primitive.bounds = AABBf_GrowBy( primitive.bounds, 1e-4f );

		sceneBounds = AABBf_Merge( sceneBounds, primitive.bounds );

		m_indices._data[iTriangle] = iTriangle;
	}

	m_bounds = sceneBounds;


	struct BVHBuildEntry {
		/// If non-zero then this is the index of the parent (used in offsets).
		U32 parent;
		/// The range of objects in the object list covered by this node.
		U32 start, end;
	};
	BVHBuildEntry todo[128];
	U32 iStackTop = 0;
	const U32 Untouched    = 0xFFFFFFFF;
	const U32 TouchedTwice = 0xFFFFFFFD;
	const U32 RootNodeID	= 0xFFFFFFFC;

	// Push the root
	todo[iStackTop].start = 0;
	todo[iStackTop].end = numPrimitives;
	todo[iStackTop].parent = RootNodeID;
	iStackTop++;

	mxDO(m_nodes.ReserveExactly( numPrimitives ));

//	UINT nodeCount = 0;	//!< number of (internal) nodes
	UINT leafCount = 0;	//!< number of (terminal) leaves

	// In-place quick sort-like construction algorithm with global heuristic
	while( iStackTop > 0 )
	{
		// Pop the next item off of the stack
		const BVHBuildEntry& bnode = todo[--iStackTop];
		const U32 start = bnode.start;
		const U32 end = bnode.end;
		const U32 nPrims = end - start;

		Node node;
		node.count = 0;
		node.iRightChild = Untouched;	// children will decrement this field later during building

		// Build a bounding box from the primitives' centroids
		AABBf bc = {
			primitives._data[ m_indices._data[start] ].center,
			primitives._data[ m_indices._data[start] ].center
		};
		// Calculate the bounding box for this node
		{
			// bounding box from the primitives' boxes
			AABBf bb( primitives._data[ m_indices._data[ start ] ].bounds );
			for( U32 p = start+1; p < end; ++p ) {
				AABBf_AddAABB( &bb, primitives._data[ m_indices._data[p] ].bounds );
				AABBf_AddPoint( &bc, primitives._data[ m_indices._data[p] ].center );
			}
			node.mins = bb.min_corner;
			node.maxs = bb.max_corner;
		}

		// If the number of primitives at this point is less than the leaf
		// size, then this will become a  (Signified by iRightChild == 0)
		if( nPrims <= build_options.maxTrisPerLeaf ) {
			node.count = nPrims;
			node.start = start;
			leafCount++;
		}

		const U32 iThisNode = m_nodes.num();	// index of the new node
		m_nodes.add(node);

		// Child touches parent...
		// Special case: Don't do this for the root.
		const U32 parentIndex = bnode.parent;
		if( parentIndex != RootNodeID )
		{
			m_nodes._data[ parentIndex ].iRightChild--;

			// When this is the second touch, this is the right child.
			// The right child sets up the offset for the flat tree.
			if( m_nodes._data[ parentIndex ].iRightChild == TouchedTwice ) {
				m_nodes._data[ parentIndex ].iRightChild = iThisNode;
			}
		}

		// If this is a leaf, no need to subdivide.
		if( node.IsLeaf() ) {
			continue;
		}

		// Partition the list of objects into two halves.
		U32 mid = start;
		{
			// Split on the center of the longest axis
			const U32 splitAxis = AABBf_LongestAxis( bc );
			const float split_coord = (bc.min_corner[splitAxis] + bc.max_corner[splitAxis]) * 0.5f;
			// Partition the list of objects on this split
			for( U32 i = start; i < end; ++i )
			{
				if( primitives._data[ m_indices._data[i] ].center[ splitAxis ] < split_coord ) {
					TSwap( m_indices._data[i], m_indices._data[mid] );
					++mid;
				}
			}
		}

		// If we get a bad split, just choose the center...
		if( mid == start || mid == end ) {
			mid = start + (end - start) / 2;
		}

		// Push the right child first: when the right child is popped off the stack (after the left child),
		// it will Initialize the 'right child offset' in the parent.
		todo[iStackTop].start = mid;
		todo[iStackTop].end = end;
		todo[iStackTop].parent = iThisNode;
		iStackTop++;

		// Then push the left child.
		todo[iStackTop].start = start;
		todo[iStackTop].end = mid;
		todo[iStackTop].parent = iThisNode;
		iStackTop++;
	}

	return ALL_OK;
}

bool AABBf_Intersect_Ray(
						const V3f& _mins, const V3f& _maxs,
						const V3f& _ro, const V3f& _rd, const V3f& _invDir,
						float& _tnear, float& _tfar
						)
{

#if 1

	return -1 != intersectRayAABB( _mins, _maxs, _ro, _rd, _tnear, _tfar );

#else

	// SSE disabled, because args must be 16-byte aligned

// http://www.flipcode.com/archives/SSE_RayBox_Intersection_Test.shtml
// turn those verbose intrinsics into something readable.
#define loadps(mem)		_mm_load_ps((const float * const)(mem))
#define storess(ss,mem)		_mm_store_ss((float * const)(mem),(ss))
#define minss			_mm_min_ss
#define maxss			_mm_max_ss
#define minps			_mm_min_ps
#define maxps			_mm_max_ps
#define mulps			_mm_mul_ps
#define subps			_mm_sub_ps
#define rotatelps(ps)		_mm_shuffle_ps((ps),(ps), 0x39)	// a,b,c,d -> b,c,d,a
#define muxhps(low,high)	_mm_movehl_ps((low),(high))	// low{a,b,c,d}|high{e,f,g,h} = {c,d,g,h}
static const float flt_plus_inf = -logf(0);	// let's keep C and C++ compilers happy.
static const float __declspec(align(16))
  ps_cst_plus_inf[4] = {  flt_plus_inf,  flt_plus_inf,  flt_plus_inf,  flt_plus_inf },
  ps_cst_minus_inf[4] = { -flt_plus_inf, -flt_plus_inf, -flt_plus_inf, -flt_plus_inf };

  // you may already have those values hanging around somewhere
  const __m128
				 plus_inf	= loadps(ps_cst_plus_inf),
              minus_inf	= loadps(ps_cst_minus_inf);

  // use whatever's appropriate to load.
  const __m128
			box_min	= loadps(&_mins),
            box_max	= loadps(&_maxs),
            pos	= loadps(&_ro),
            inv_dir	= loadps(&_invDir);

  // use a div if inverted directions aren't available
  const __m128 l1 = mulps(subps(box_min, pos), inv_dir);
  const __m128 l2 = mulps(subps(box_max, pos), inv_dir);

  // the order we use for those min/max is vital to filter out
  // NaNs that happens when an inv_dir is +/- inf and
  // (box_min - pos) is 0. inf * 0 = NaN
  const __m128 filtered_l1a = minps(l1, plus_inf);
  const __m128 filtered_l2a = minps(l2, plus_inf);

  const __m128 filtered_l1b = maxps(l1, minus_inf);
  const __m128 filtered_l2b = maxps(l2, minus_inf);

  // now that we're back on our feet, test those slabs.
  __m128 lmax = maxps(filtered_l1a, filtered_l2a);
  __m128 lmin = minps(filtered_l1b, filtered_l2b);

  // unfold back. try to hide the latency of the shufps & co.
  const __m128 lmax0 = rotatelps(lmax);
  const __m128 lmin0 = rotatelps(lmin);
  lmax = minss(lmax, lmax0);
  lmin = maxss(lmin, lmin0);

  const __m128 lmax1 = muxhps(lmax,lmax);
  const __m128 lmin1 = muxhps(lmin,lmin);
  lmax = minss(lmax, lmax1);
  lmin = maxss(lmin, lmin1);

  const bool ret = _mm_comige_ss(lmax, _mm_setzero_ps()) & _mm_comige_ss(lmax,lmin);

  storess(lmin, &_tnear);
  storess(lmax, &_tfar);

  return  ret;

#endif

}

namespace
{
	//! Node for storing state information during traversal.
	struct BVHTraversal {
		U32 iNode; // Node
		float mint; // Minimum hit time for this node.
	public:
		mxFORCEINLINE BVHTraversal()
		{}
		mxFORCEINLINE BVHTraversal(U32 _i, float _mint)
			: iNode(_i), mint(_mint)
		{ }
	};
}//namespace

U32 BVH::FindAllIntersections(
							  const V3f& _O,
							  const V3f& _D,
							  const float _tmax,
							  TriangleCallback* triangle_callback,
							  void* triangle_callback_data
							  ) const
{
	const CV3f invDir(
		(mmAbs(_D.x) > 1e-4f) ? mmRcp(_D.x) : BIG_NUMBER,
		(mmAbs(_D.y) > 1e-4f) ? mmRcp(_D.y) : BIG_NUMBER,
		(mmAbs(_D.z) > 1e-4f) ? mmRcp(_D.z) : BIG_NUMBER
	);

	U32	objectsHit = 0;

	// Working set
	BVHTraversal todo[64];

	// "Push" on the root node to the working set
	todo[0].iNode = 0;
	todo[0].mint = -9999999.f;

	int iStackTop = 0;

	while( iStackTop >= 0 )
	{
		// Pop off the next node to work on.
		const int iNode = todo[iStackTop].iNode;
		const float tnear = todo[iStackTop].mint;
		iStackTop--;

		if( tnear > _tmax ) {
			continue;
		}

		const Node& node = m_nodes._data[ iNode ];

		if( node.IsLeaf() )
		{
			for( U32 iPrimitive = 0; iPrimitive < node.count; ++iPrimitive )
			{
				(*triangle_callback)(
					m_indices._data[ node.start + iPrimitive ]
					, triangle_callback_data
				);
			}
			objectsHit += node.count;
		}
		else
		{
			const U32 iChild0 = iNode + 1;	// the first (left) child is stored right after the node
			const U32 iChild1 = node.iRightChild;
			const Node& child0 = m_nodes._data[iChild0];
			const Node& child1 = m_nodes._data[iChild1];

			float tnear0, tfar0;
			float tnear1, tfar1;
			const bool hitc0 = AABBf_Intersect_Ray( child0.mins, child0.maxs, _O, _D, invDir, tnear0, tfar0 );
			const bool hitc1 = AABBf_Intersect_Ray( child1.mins, child1.maxs, _O, _D, invDir, tnear1, tfar1 );

			// Did we hit both nodes?
			if( hitc0 && hitc1 )
			{
				// We assume that the left child is a closer hit...
				U32 closer = iChild0;
				U32 further = iChild1;

				// ... If the right child was actually closer, swap the relevant values.
				if( tnear1 < tnear0 )
				{
					TSwap( tnear0, tnear1 );
//					TSwap( tfar0, tfar1 );	// unused
					TSwap( closer, further );
				}

				// It's possible that the nearest object is still in the other side,
				// but we'll check the further-away node later...

				// Push the farther first
				todo[++iStackTop] = BVHTraversal(further, tnear1);

				// And now the closer (with overlap test)
				todo[++iStackTop] = BVHTraversal(closer, tnear0);
			}
			else if (hitc0)
			{
				todo[++iStackTop] = BVHTraversal(iChild0, tnear0);
			}
			else if(hitc1)
			{
				todo[++iStackTop] = BVHTraversal(iChild1, tnear1);
			}
		}
	}
	return objectsHit;
}

bool BVH::FindClosestIntersection(
	const V3f& _O,
	const V3f& _D,
	const float _tmax,
	const TriMesh& _mesh,
	Intersection &_result
	) const
{
	const CV3f invDir(
		(mmAbs(_D.x) > 1e-4f) ? mmRcp(_D.x) : BIG_NUMBER,
		(mmAbs(_D.y) > 1e-4f) ? mmRcp(_D.y) : BIG_NUMBER,
		(mmAbs(_D.z) > 1e-4f) ? mmRcp(_D.z) : BIG_NUMBER
	);

	_result.iTriangle = ~0;
	_result.minT = BIG_NUMBER;

	// Working set
	BVHTraversal todo[64];

	// "Push" on the root node to the working set
	todo[0].iNode = 0;
	todo[0].mint = -BIG_NUMBER;

	int iStackTop = 0;

	while( iStackTop >= 0 )
	{
		// Pop off the next node to work on.
		const int iNode = todo[iStackTop].iNode;
		const float tnear = todo[iStackTop].mint;
		iStackTop--;

		// If this node is further than the closest found intersection, continue
		if( tnear > _result.minT ) {
			continue;
		}

		const Node& node = m_nodes._data[ iNode ];

		if( node.IsLeaf() )
		{
			for( U32 iPrimitive = 0; iPrimitive < node.count; ++iPrimitive )
			{
				const U32 triangleIndex = m_indices._data[ node.start + iPrimitive ];
				const TriMesh::Triangle& triangle = _mesh.triangles._data[ triangleIndex ];

				const Vertex& v0 = _mesh.vertices._data[triangle.a[0]];
				const Vertex& v1 = _mesh.vertices._data[triangle.a[1]];
				const Vertex& v2 = _mesh.vertices._data[triangle.a[2]];

				float t;
				if( RayTriangleIntersection( v0.xyz, v1.xyz, v2.xyz, _O, _D, &t ) )
				{
					// keep the closest intersection only
					if( t < _result.minT ) {
						_result.minT = t;
						_result.iTriangle = triangleIndex;
					}
				}
			}
		}
		else
		{
			const U32 iChild0 = iNode + 1;	// the first (left) child is stored right after the node
			const U32 iChild1 = node.iRightChild;
			const Node& child0 = m_nodes._data[iChild0];
			const Node& child1 = m_nodes._data[iChild1];

			float tnear0, tfar0;
			float tnear1, tfar1;
			const bool hitc0 = AABBf_Intersect_Ray( child0.mins, child0.maxs, _O, _D, invDir, tnear0, tfar0 );
			const bool hitc1 = AABBf_Intersect_Ray( child1.mins, child1.maxs, _O, _D, invDir, tnear1, tfar1 );

			// Did we hit both nodes?
			if( hitc0 && hitc1 )
			{
				// We assume that the left child is a closer hit...
				U32 closer = iChild0;
				U32 further = iChild1;

				// ... If the right child was actually closer, swap the relevant values.
				if( tnear1 < tnear0 )
				{
					TSwap( tnear0, tnear1 );
//					TSwap( tfar0, tfar1 );	// unused
					TSwap( closer, further );
				}

				// It's possible that the nearest object is still in the other side,
				// but we'll check the further-away node later...

				// Push the farther first
				todo[++iStackTop] = BVHTraversal(further, tnear1);

				// And now the closer (with overlap test)
				todo[++iStackTop] = BVHTraversal(closer, tnear0);
			}
			else if (hitc0)
			{
				todo[++iStackTop] = BVHTraversal(iChild0, tnear0);
			}
			else if(hitc1)
			{
				todo[++iStackTop] = BVHTraversal(iChild1, tnear1);
			}
		}
	}
	return _result.iTriangle != ~0;
}

U32 BVH::FindAllPrimitivesInBox(
								const AABBf& _bounds,
								U32 *_primitives,
								U32 _bufferSize
								) const
{
	UNDONE;
	return 0;
}

void BVH::DebugDraw(
					ADebugDraw & renderer
					)
{
#if 0
	for( UINT i = 0; i < m_nodes.num(); i++ )
	{
		renderer.DrawAABB( m_nodes[i].bbox, RGBAf::WHITE );
	}
#else
	// the top-most level isn't rendered
	static const U32 s_NodeColors[8] = {
		RGBAi::RED,
		RGBAi::ORANGE,
		RGBAi::YELLOW,
		RGBAi::GREEN,
		RGBAi::LIGHTBLUE,
		RGBAi::BLUE,
		RGBAi::VIOLET,
		RGBAi::WHITE,
	};

	TStaticArray< U32, 64 >	stack;
	// "Push" the root node.
	stack[0] = 0;
	UINT stackPtr = 1;

	while( stackPtr > 0 )
	{
		// Pop off the next node to work on.
		const U32 iNode = stack[--stackPtr];
		const Node& node = m_nodes[iNode];		

		const UINT treeDepth = stackPtr;
		const int colorIndex = smallest( treeDepth, mxCOUNT_OF(s_NodeColors)-1 );
		renderer.DrawAABB( node.mins, node.maxs, RGBAf::fromRgba32(s_NodeColors[colorIndex]) );

		if( !node.IsLeaf() )
		{
			stack[ stackPtr++ ] = iNode + 1;	// left
			stack[ stackPtr++ ] = node.iRightChild;	// right
		}
	}
#endif
}

void BVH::dbg_show(
	VX::ADebugView & renderer
	, const M44f& transform
	)
{
	// the top-most level isn't rendered
	static const U32 s_NodeColors[8] = {
		RGBAi::RED,
		RGBAi::ORANGE,
		RGBAi::YELLOW,
		RGBAi::GREEN,
		RGBAi::LIGHTBLUE,
		RGBAi::BLUE,
		RGBAi::VIOLET,
		RGBAi::WHITE,
	};

	TStaticArray< U32, 64 >	stack;
	// "Push" the root node.
	stack[0] = 0;
	UINT stackPtr = 1;

	while( stackPtr > 0 )
	{
		// Pop off the next node to work on.
		const U32 iNode = stack[--stackPtr];
		const Node& node = m_nodes[iNode];		

		const UINT treeDepth = stackPtr;
		const int colorIndex = smallest( treeDepth, mxCOUNT_OF(s_NodeColors)-1 );

		renderer.addBox(
			AABBf::make( node.mins, node.maxs ).transformed( transform ),
			RGBAf::fromRgba32(s_NodeColors[colorIndex])
			);

		if( !node.IsLeaf() )
		{
			stack[ stackPtr++ ] = iNode + 1;	// left
			stack[ stackPtr++ ] = node.iRightChild;	// right
		}
	}
}

}//namespace Meshok

/*
Implementations:
https://github.com/brandonpelfrey/Fast-BVH
https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c
*/
