// Bounding Interval Hierarchy
#include "stdafx.h"
#pragma hdrstop
#include <Core/Memory.h>
#include <Meshok/Meshok.h>
#include <Meshok/BIH.h>

namespace Meshok
{

namespace
{

/// temporary struct used for building a Kd-tree
struct Primitive
{
	AABBf	bounds;
	V3f		center;	//!< precached center of the bounding box
};

static
const AABBf ComputeBounds(
						 const U32* _primitiveIndices,
						 const UINT primitiveCount,
						 const Primitive* _primitives
				   )
{
	AABBf	bounds;
	AABBf_Clear( &bounds );
	for( UINT i = 0; i < primitiveCount; i++ )
	{
		const UINT iPrimitive = _primitiveIndices[ i ];
		const Primitive& primitive = _primitives[ iPrimitive ];
		AABBf_AddAABB( &bounds, primitive.bounds );
	}
	return bounds;
}

static
ERet BuildTreeRecursive(
						BIH & _tree,
						const UINT nodeIndex,
						const AABBf& nodeAabb,
						const UINT nodeDepth,
						const U32 start_index,
						const U32 index_count,	// primitiveCount
						const Primitive* _primitives,
						const BIH::BuildOptions& _options
						)
{
	mxASSERT(index_count > 0);
	if( index_count <= _options.maxTrisPerLeaf || nodeDepth >= _options.maxTreeDepth )
	{
		// create a leaf node
		BIH::Node & node = _tree.m_nodes[ nodeIndex ];
		node.flags_and_count = index_count;
		node.leaf.start_tri = start_index;
		return ALL_OK;
	}
	mxASSERT(index_count > 1);

	// determine the longest axis of the bounding box
	const V3f aabbSize = AABBf_FullSize( nodeAabb );
	const int longestAxis = Max3Index( aabbSize.x, aabbSize.y, aabbSize.z );

	int	splitAxis;	// the split axis

	AABBf leftBounds;
	AABBf rightBounds;
	UINT primitivesRight;	// the number of primitives assigned to the right child

	// iterate possible split axis choices, starting with the longest axis.
	// if all fail it means all children have the same bounds and we simply split
	// the list in half because each node can only have two children.
	for( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		// pick an axis
		splitAxis = (longestAxis + iAxis) % 3;
		// sort children into left and right lists
		const float splitValue = (nodeAabb.min_corner[splitAxis] + nodeAabb.max_corner[splitAxis]) * 0.5f;

		AABBf_Clear( &leftBounds );
		AABBf_Clear( &rightBounds );
		primitivesRight = 0;

		// Because the bounding interval hierarchy is an object partitioning scheme,
		// all object sorting can be done in place and no temporary memory management is required.
		// The recursive construction procedure only needs two pointers
		// to the left and right objects in the index array similar to the quick-sort algorithm.
		UINT left = start_index;	//!< index of the last left item
		UINT right = start_index + index_count - 1;	//!< index of the first right item

		while( left <= right )
		{
			const UINT iPrimitive = _tree.m_indices[ left ];
			const Primitive& primitive = _primitives[ iPrimitive ];

			// Assign the primitive to the correct side, based on the splitting value.
			if( primitive.center[ splitAxis ] < splitValue )
			{
				++left;
				AABBf_AddAABB( &leftBounds, primitive.bounds );
			}
			else
			{
				primitivesRight++;	// Count primitives assigned to the right child.
				// Indices must be sorted in this order: LeftChild | RightChild.
				TSwap( _tree.m_indices[left], _tree.m_indices[right] );	// move this item to the rightmost position
				--right;
				AABBf_AddAABB( &rightBounds, primitive.bounds );
			}
		}//for each primitive

		// if both sides have some children, it's good enough for us.
		if( primitivesRight < index_count ) {
			break;
		}
	}//for each candidate axis

	// If we get a bad split, just choose the center...
	if( !primitivesRight || primitivesRight == index_count )
	{
		// somewhat common case: no good choice, divide children arbitrarily
		primitivesRight = index_count / 2;

		// calculate extents
		leftBounds = ComputeBounds(
			_tree.m_indices.raw() + start_index,
			index_count - primitivesRight,
			_primitives
			);
		rightBounds = ComputeBounds(
			_tree.m_indices.raw() + start_index + (index_count - primitivesRight),
			primitivesRight,
			_primitives
			);

		splitAxis = longestAxis;
	}

	const float clipL = leftBounds.max_corner[ splitAxis ];	// the maximum bounding value of all objects on the left
	const float clipR = rightBounds.min_corner[ splitAxis ];// the minimum bounding value of those on the right

	const UINT primitivesLeft = index_count - primitivesRight;

	// we now have Left and Right primitives sorted in _tree.m_indices...

	// allocate child nodes
	const UINT iLeftChild = _tree.m_nodes.num();
	const UINT iRightChild = iLeftChild + 1;
	// The right child is stored right after the left child in the nodes array.
	BIH::Node emptyNode;
	mxDO(_tree.m_nodes.add(emptyNode));	
	mxDO(_tree.m_nodes.add(emptyNode));

	// Initialize the parent node
	{
		BIH::Node & node = _tree.m_nodes[ nodeIndex ];
		node.flags_and_count = ((splitAxis + 1) << 30);
		node.inner.children = iLeftChild;
		node.inner.clip[0] = clipL;
		node.inner.clip[1] = clipR;
	}

	BuildTreeRecursive(
		_tree,
		iLeftChild,
		leftBounds,
		nodeDepth+1,
		start_index, primitivesLeft,
		_primitives,
		_options
		);

	BuildTreeRecursive(
		_tree,
		iRightChild,
		rightBounds,
		nodeDepth+1,
		start_index + primitivesLeft, primitivesRight,
		_primitives,
		_options
		);

	return ALL_OK;
}

}//namespace

BIH::BIH()
{
}

ERet BIH::Build(
				   const TriMesh& _mesh,
				   const BuildOptions& _options
				   )
{
	DynamicArray< Primitive >	primitives( MemoryHeaps::temporary() );
	primitives.setCountExactly(_mesh.triangles.num());

	m_indices.setCountExactly(_mesh.triangles.num());

	// Compute the bounding box of the whole scene.
	AABBf sceneBounds;
	AABBf_Clear(&sceneBounds);

	// Precompute bounding boxes for all objects to store.
	for( UINT iTriangle = 0; iTriangle < _mesh.triangles.num(); iTriangle++ )
	{
		const TriMesh::Triangle& T = _mesh.triangles[ iTriangle ];
		const Vertex& V1 = _mesh.vertices[ T.a[0] ];
		const Vertex& V2 = _mesh.vertices[ T.a[1] ];
		const Vertex& V3 = _mesh.vertices[ T.a[2] ];

		Primitive& primitive = primitives[ iTriangle ];

		primitive.bounds.min_corner = V3_Mins( V3_Mins( V1.xyz, V2.xyz ), V3.xyz );
		primitive.bounds.max_corner = V3_Maxs( V3_Maxs( V1.xyz, V2.xyz ), V3.xyz );
		primitive.center = AABBf_Center( primitive.bounds );

		// Increase the size of the bounding box to avoid numerical issues.
		primitive.bounds = AABBf_GrowBy( primitive.bounds, 1e-4f );

		sceneBounds = AABBf_Merge( sceneBounds, primitive.bounds );

		m_indices[iTriangle] = iTriangle;
	}

	m_bounds = sceneBounds;

	// create the root node
	BIH::Node rootNode;
	mxDO(m_nodes.add(rootNode));

	BuildTreeRecursive(
		*this,
		0/*root index*/,
		sceneBounds,
		0/*root depth*/,
		0/*start index*/,
		primitives.num(),		
		primitives.raw(),
		_options
	);

	return ALL_OK;
}

struct RayParameters
{
	V3f	origin;	//!< Ray Origin
	V3f	dir;	//!< Ray Direction
	V3f	invDir;	//!< Inverse of each Ray Direction component
	F32	tmin;
	F32	tmax;
public:
	void Init( const V3f& _O, const V3f& _D, float _tmin = 0.0f, float _tmax = BIG_NUMBER )
	{
#define ZERO_EPSILON 0.0001f
		mxASSERT(V3_IsNormalized(_D));
		origin = _O;
		dir = _D;
		invDir.x = (mmAbs(dir.x) > ZERO_EPSILON) ? mmRcp(dir.x) : BIG_NUMBER;
		invDir.y = (mmAbs(dir.y) > ZERO_EPSILON) ? mmRcp(dir.y) : BIG_NUMBER;
		invDir.z = (mmAbs(dir.z) > ZERO_EPSILON) ? mmRcp(dir.z) : BIG_NUMBER;
		tmin = _tmin;
		tmax = _tmax;
#undef ZERO_EPSILON
	}
};

struct Intersections
{
	U32 *	primitives;
	U32		bufferSize;
	U32		registered;
	U32		totalCount;	//!< total number of potentially intersecting objects
};

static
void FindAllIntersectionsRecursive(
						const BIH& _tree,
						const UINT nodeIndex,
						const RayParameters& _ray,
						Intersections & _results,
						const float _tmin,
						const float _tmax,
						U32 *_primitives,
						U32 _bufferSize
						)
{
	const BIH::Node & node = _tree.m_nodes[ nodeIndex ];
	if( node.IsLeaf() )
	{
		U32 spaceLeft = _results.bufferSize - _results.registered;
		for( UINT i = 0;
			(i < node.PrimitiveCount()) && (i < spaceLeft);
			i++ )
		{
			_results.primitives[ _results.registered++ ] = _tree.m_indices[ node.leaf.start_tri + i ];
		}
		_results.totalCount += node.PrimitiveCount();
	}
	else
	{
		const int axis = node.Axis();
		// it is possible to directly access the child
		// that is closer to the ray origin by the sign of the ray direction;
		// determine the traversal order depending upon the ray direction
		const int farSide = (_ray.dir[axis] >= 0.0f);		
		const int nearSide = farSide ^ 1;	// index of the first child to traverse

		// intersect the children's bounding planes with the ray
		const float tnear = (node.inner.clip[nearSide] - _ray.origin[axis] ) * _ray.invDir[axis];
		const float tfar  = (node.inner.clip[farSide ] - _ray.origin[axis] ) * _ray.invDir[axis];

		const U32 children[2] = { node.inner.children, node.inner.children + 1 };

		if( _tmin > tnear ) {	// if the ray segment doesn't intersect the near child
			if( tfar < _tmax ) { // if the ray hits the far side
				FindAllIntersectionsRecursive( _tree, children[farSide], _ray, _results, maxf( _tmin, tfar ), _tmax, _primitives, _bufferSize );
			} // else: ray passes through empty space between node planes
		} else if( tfar > _tmax ) {	// if the ray segment doesn't intersect the far child
			FindAllIntersectionsRecursive( _tree, children[nearSide], _ray, _results, _tmin,  minf( _tmax, tnear ), _primitives, _bufferSize );
		} else { // both nodes have to be visited
			FindAllIntersectionsRecursive( _tree, children[nearSide], _ray, _results, _tmin,  minf( _tmax, tnear ), _primitives, _bufferSize );

//			if ( !closest.IsEmpty() )
//				_tmax = fminf(closest.getT(), _tmax );

			FindAllIntersectionsRecursive( _tree, children[farSide], _ray, _results, maxf( _tmin, tfar ), _tmax, _primitives, _bufferSize );
		}
	}
}

U32 BIH::FindAllIntersections(
						const V3f& _O,
						const V3f& _D,
						const float _tmax,
						U32 *_primitives,
						U32 _bufferSize
						) const
{
	RayParameters	ray;
	ray.Init( _O, _D, 0.0f, _tmax );

	Intersections	result;
	result.primitives = _primitives;
	result.bufferSize = _bufferSize;
	result.registered = 0;
	result.totalCount = 0;

	float tnear, tfar;
	if( -1 != intersectRayAABB( m_bounds.min_corner, m_bounds.max_corner, _O, _D, tnear, tfar ) )
	{
		FindAllIntersectionsRecursive( *this, 0, ray, result, tnear, tfar, _primitives, _bufferSize );
	}
	return result.totalCount;
}

U32 BIH::FindAllPrimitivesInBox(
								const AABBf& _bounds,
								U32 *_primitives,
								U32 _bufferSize
								) const
{
UNDONE;
return 0;
}

void BIH::DebugDraw(
			   ADebugDraw & renderer
			   )
{
	/// Struct for storing state information during traversal.
	struct StackEntry
	{
		U32		node;
		AABBf	aabb;
	};
	StackEntry	stack[64];
	int			stackPtr = 0;

	// "Push" the root node.
	stack[0].node = 0;
	stack[0].aabb = m_bounds;

	while( stackPtr >= 0 )
	{
		// Pop off the next node to work on.
		const AABBf aabb = stack[stackPtr].aabb;
		const U32 iNode = stack[stackPtr].node;
		const Node& node = m_nodes[iNode];		
		stackPtr--;

		if( !node.IsLeaf() )
		{
			const int axis = node.Axis();

			AABBf	leftBox( aabb );
			AABBf	rightBox( aabb );

			leftBox.max_corner[axis] = node.inner.clip[0];
			rightBox.min_corner[axis] = node.inner.clip[1];

			renderer.DrawAABB( leftBox, RGBAf::RED );
			renderer.DrawAABB( rightBox, RGBAf::GREEN );

			// Push the children onto the stack.
			++stackPtr;
			stack[ stackPtr ].node = node.inner.children;
			stack[ stackPtr ].aabb = leftBox;
			
			++stackPtr;
			stack[ stackPtr ].node = node.inner.children + 1;
			stack[ stackPtr ].aabb = rightBox;
		}
	}

	renderer.DrawAABB( m_bounds, RGBAf::WHITE );
}

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
