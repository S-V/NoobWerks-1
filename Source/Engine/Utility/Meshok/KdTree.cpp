// Adaptively Sampled Distance Field.
#include "stdafx.h"
#pragma hdrstop
#include <Core/Memory.h>
#include <Meshok/Meshok.h>
#include <Meshok/KdTree.h>

namespace Meshok
{

namespace
{

/// temporary struct used for building a Kd-tree
struct Primitive : TLinkedList< Primitive >
{
	AABBf	bounds;
	V3f		center;	//!< precached center of the bounding box
	U32		triangleIndex;	//! the original index of the traingle
};

struct SplittingPlane
{
	float	dist;	//!< split plane coordinate
	EAxis	axis;	//!< split axis
};


#if 0
const SplittingPlane FindSplittingPlane(
									  const DynamicArray< Primitive >& _primitives
									  )
{
	// Surface Area Heuristic (SAH) for finding optimal split:
	// The probability of intersection of the ray with subtree
	// is proportional to the surface area of the bounding box of that subtree.

	// The main idea of SAH is to minimize both:
	// surface areas of subtrees and number of polygons inside of them.


	// the splitting plane must touch the border of at least one object


	// determine split axis

}
#endif

const SplittingPlane FindSplittingPlane_NaiveMidpoint(
	const Primitive::Head _primitives
	//UINT & _checkedPrimitives	//!< number of primitives tested
	)
{
	// Compute the bounding box of the whole scene.
	AABBf totalBounds;
	AABBf_Clear(&totalBounds);

	//UINT primitiveCount = 0;	//!< number of primitives tested
	for(
		const Primitive* primitive = _primitives;
		primitive != nil;
		primitive = primitive->_next
		)
	{
		totalBounds = AABBf_Merge( totalBounds, primitive->bounds );
		//primitiveCount++;
	}
	//_checkedPrimitives = primitiveCount;

	// split along the longest axis
	const V3f aabbSize = AABBf_FullSize(totalBounds);
	const EAxis axis = (EAxis) Max3Index( aabbSize.x, aabbSize.y, aabbSize.z );

	SplittingPlane result;
	result.dist = (totalBounds.min_corner[axis] + totalBounds.max_corner[axis]) * 0.5f;
	result.axis = axis;
	return result;
}

void InitializeInternalNode(
							KdTree & _tree,
							const UINT nodeIndex,
							Primitive::Head _primitives,
							const UINT primitiveCount,
							const KdTree::BuildOptions& _options,
							const SplittingPlane& plane,
							UINT iFirstChildIndex
							)
{
	KdTree::Node & node = _tree.m_nodes[ nodeIndex ];
	UNDONE;
}

void InitializeLeafNode(
						KdTree & _tree,
						const UINT nodeIndex,
						Primitive::Head _primitives,
						const UINT primitiveCount,
						const KdTree::BuildOptions& _options
						)
{
UNDONE;
}

ERet BuildTreeRecursive(
						KdTree & _tree,
						const UINT nodeIndex,
						const AABBf& nodeAabb,
						const UINT nodeDepth,
						Primitive::Head _primitives,
						const UINT primitiveCount,
						const KdTree::BuildOptions& _options
						)
{
	if( primitiveCount <= _options.maxTrisPerLeaf || nodeDepth >= _options.maxTreeDepth )
	{
		InitializeLeafNode( _tree, nodeIndex, _primitives, primitiveCount, _options );
		return ALL_OK;
	}

	const SplittingPlane plane = FindSplittingPlane_NaiveMidpoint( _primitives );

	// Create new states for this node and the left/right children.
	Primitive::Head	primitivesOn = nil;
	Primitive::Head	primitivesBack = nil;
	Primitive::Head	primitivesFront = nil;
	UINT	counts[3] = { 0, 0, 0 };

	// assign primitives to correct sides
	for( Primitive* primitive = _primitives; primitive != nil; )
	{
		Primitive* nextObject = primitive->_next;

		// Assigned the triangle to the correct side, based on the splitting value.
		if( primitive->center[ plane.axis ] > plane.dist )
		{
			primitive->PrependSelfToList( &primitivesFront );
			counts[PLANESIDE_FRONT]++;
		}
		else if( primitive->center[ plane.axis ] < plane.dist )
		{
			primitive->PrependSelfToList( &primitivesBack );
			counts[PLANESIDE_BACK]++;
		}
		else
		{
			primitive->PrependSelfToList( &primitivesOn );
			counts[PLANESIDE_ON]++;
		}

		primitive = nextObject;
	}

	// allocate child nodes
	const UINT iFirstChildIndex = _tree.m_nodes.num();
	KdTree::Node emptyNode;
	mxDO(_tree.m_nodes.add(emptyNode));
	// The right child is stored right after the left child in the nodes array.
	mxDO(_tree.m_nodes.add(emptyNode));

	InitializeInternalNode( _tree, nodeIndex, _primitives, primitiveCount, _options, plane, iFirstChildIndex );
UNDONE;
	//BuildTreeRecursive(
	//	_tree,
	//	iFirstChildIndex,
	//	nodeDepth+1,
	//	primitivesBack, counts[PLANESIDE_BACK],
	//	_options
	//	);

	//BuildTreeRecursive(
	//	_tree,
	//	iFirstChildIndex + 1,
	//	nodeDepth+1,
	//	primitivesFront, counts[PLANESIDE_FRONT],
	//	_options
	//	);

	return ALL_OK;
}

}//namespace

ERet KdTree::Build(
				   const TriMesh& _mesh,
				   const BuildOptions& _options
				   )
{
	// Precompute bounding boxes for all objects to store.
	DynamicArray< Primitive >	primitives( MemoryHeaps::temporary() );
	primitives.setNum(_mesh.triangles.num());

	// Compute the bounding box of the whole scene.
	AABBf sceneBounds;
	AABBf_Clear(&sceneBounds);

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

		primitive.triangleIndex = iTriangle;

		// setup a linked list of primitives
		primitive._next = &(primitives[ iTriangle + 1 ]);
	}
	primitives.getLast()._next = nil;
UNDONE;
	// create the root node
	KdTree::Node rootNode;
	mxDO(m_nodes.add(rootNode));

	BuildTreeRecursive(
		*this,
		0/*root index*/,
		sceneBounds,
		0/*root depth*/,
		&primitives.getFirst(), primitives.num(),
		_options
	);

	return ALL_OK;
}

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
