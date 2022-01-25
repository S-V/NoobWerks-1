// Bounding Interval Hierarchy
#include <Base/Base.h>
#pragma hdrstop

#include <Graphics/Public/graphics_utilities.h>

#include <Utility/MeshLib/TriMesh.h>

#include <Physics/Bullet_Wrapper.h>
#include <Physics/Collision/TbQuantizedBIH.h>

mxOPTIMIZE("SIMD for AABB calc");

#define DEBUG_BIH	(0)

/// ! slows down a lot, esp. debug builds
#define DBG_BIH_VISUALIZE_TESTED_TRIANGLES	(0)


static
const RGBAf getDebugColorForTreeLevel( UINT depth )
{
	static const RGBAf colors_for_first_levels[] =
	{
		RGBAf::WHITE,
		RGBAf::YELLOW,
		RGBAf::yellowgreen,
		RGBAf::GREEN,

		RGBAf::BLUE,
		RGBAf::MAGENTA,
		RGBAf::ORANGE,
		RGBAf::RED,
	};

	if( depth < mxCOUNT_OF(colors_for_first_levels) )
	{
		return colors_for_first_levels[ depth ];
	}

	return RGBAf::RED;
}

/*
-----------------------------------------------------------------------------
	NwBIH
-----------------------------------------------------------------------------
*/
//tbPRINT_SIZE_OF(NwBIH);

NwBIH::NwBIH()
{
}


namespace
{

	// internal node if the upper two bits are non-zero
	#define IS_INTERNAL_NODE( node )\
		((node).type_and_type_specific_data & NwBIH::Node::SPLIT_AXIS_MASK)

	#define GET_RIGHT_CHILD_INDEX( node )\
		((node).inner.axis_and_right_child_index & NwBIH::Node::RIGHT_CHILD_INDEX_MASK)

	#define SET_RIGHT_CHILD_INDEX( node, new_right_child_index )\
		(node).inner.axis_and_right_child_index =\
		((node).inner.axis_and_right_child_index & NwBIH::Node::SPLIT_AXIS_MASK)\
		|(new_right_child_index & NwBIH::Node::RIGHT_CHILD_INDEX_MASK)


	#define QUANT16_COEFF	(65535.0f)
	#define DEQUANT16_COEFF	(1.0f / 65535.0f)

	static mxFORCEINLINE
	U16 quantizeMinBound( float min_bound )
	{
		//remove annoying assert when the tested object is out of world bounds
		//mxASSERT( min_bound >= 0.0f && min_bound <= 1.0f );

		const int conservative_min_bound = (int) floorf( min_bound * QUANT16_COEFF );

		return (U16) conservative_min_bound;
	}

	static mxFORCEINLINE
	U16 quantizeMaxBound( float max_bound )
	{
		//remove annoying assert when the tested object is out of world bounds
		//mxASSERT( max_bound >= 0.0f && max_bound <= 1.0f );

		const int conservative_max_bound = (int) ceilf( max_bound * QUANT16_COEFF );

		return (U16) conservative_max_bound;
	}

	#define DEQUANTIZE16( quantized_uint16_value )	(((int)quantized_uint16_value) * DEQUANT16_COEFF)
	#define DEQUANTIZE_COORD( quantized_uint16_value )	DEQUANTIZE16( quantized_uint16_value )




	/// uses the same amount of memory as floating-point AABB, but comparing integers is faster
	mxOPTIMIZE("quantize to 16 bits -> reduces stack entry size to 16 bytes");
	struct AABBq
	{
		int	min_corner[3];
		int	max_corner[3];
	};

	//static bool quantizedAABBsIntersectOrTouch( const AABBq& a, const AABBq& b )
#define quantizedAABBsIntersectOrTouch( a, b )\
	(  a.min_corner[0] <= b.max_corner[0] && a.min_corner[1] <= b.max_corner[1] && a.min_corner[2] <= b.max_corner[2]\
	&& a.max_corner[0] >= b.min_corner[0] && a.max_corner[1] >= b.min_corner[1] && a.max_corner[2] >= b.min_corner[2])

	static mxFORCEINLINE const AABBf dequantizeLocalAABB( const AABBq& quantized_aabb )
	{
		AABBf	dequantized_aabb;

		dequantized_aabb.min_corner.x = DEQUANTIZE_COORD( quantized_aabb.min_corner[0] );
		dequantized_aabb.min_corner.y = DEQUANTIZE_COORD( quantized_aabb.min_corner[1] );
		dequantized_aabb.min_corner.z = DEQUANTIZE_COORD( quantized_aabb.min_corner[2] );

		dequantized_aabb.max_corner.x = DEQUANTIZE_COORD( quantized_aabb.max_corner[0] );
		dequantized_aabb.max_corner.y = DEQUANTIZE_COORD( quantized_aabb.max_corner[1] );
		dequantized_aabb.max_corner.z = DEQUANTIZE_COORD( quantized_aabb.max_corner[2] );

		return dequantized_aabb;
	}

	static mxFORCEINLINE const AABBq quantizeLocalAABB( const AABBf& aabb_in_bih_local_space )
	{
		AABBq	quantized_aabb;

		quantized_aabb.min_corner[0] = quantizeMinBound( maxf( aabb_in_bih_local_space.min_corner.x, 0 ) );
		quantized_aabb.min_corner[1] = quantizeMinBound( maxf( aabb_in_bih_local_space.min_corner.y, 0 ) );
		quantized_aabb.min_corner[2] = quantizeMinBound( maxf( aabb_in_bih_local_space.min_corner.z, 0 ) );

		quantized_aabb.max_corner[0] = quantizeMaxBound( minf( aabb_in_bih_local_space.max_corner.x, 1 ) );
		quantized_aabb.max_corner[1] = quantizeMaxBound( minf( aabb_in_bih_local_space.max_corner.y, 1 ) );
		quantized_aabb.max_corner[2] = quantizeMaxBound( minf( aabb_in_bih_local_space.max_corner.z, 1 ) );

		//const AABBf temp = dequantizeLocalAABB(quantized_aabb);
		//mxASSERT(aabb_in_bih_local_space.equals(temp, 1e-3f));

		return quantized_aabb;
	}



}//namespace

void NwBIH::ProcessTrianglesIntersectingBox( btTriangleCallback* callback
											, const btVector3& world_aabb_min
											, const btVector3& world_aabb_max
											, const btVector3& local_aabb_min
											, const btVector3& local_aabb_max
											, const AABBf& bih_bounds_local_space
											, float uniform_scale_local_to_world
											, const V3f& minimum_corner_in_world_space
											) const
{
	const AABBf tested_box_in_bih_local_space = AABBf::make(
		fromBulletVec(local_aabb_min), fromBulletVec(local_aabb_max)
		);
	const AABBq quantized_query_aabb = quantizeLocalAABB( tested_box_in_bih_local_space );

	//
	//UINT	num_triangles_tested = 0;

	//
	const Node *	nodes = _nodes.begin();
	const QTri *	tris = _triangles.begin();
	const QVert *	verts = _vertices.begin();


	/// Node for storing state information during traversal.
	mxPREALIGN(16) struct BIHTraversal
	{
		U32		node_index;	// 4
		AABBq	node_aabb;	// 24
		U32		padding;	// 4
	} mxPOSTALIGN(16);

	mxSTATIC_ASSERT(sizeof(BIHTraversal) == 32);

	BIHTraversal todo_stack[64];

	todo_stack[0].node_aabb = quantizeLocalAABB( bih_bounds_local_space );
	todo_stack[0].node_index = 0;

	int	iStackTop = 0;

	while( iStackTop >= 0 )
	{
		const BIHTraversal& item = todo_stack[ iStackTop-- ];
		const U32 node_index = item.node_index;

		const Node& node = nodes[ node_index ];

		if( IS_INTERNAL_NODE( node ) )
		{
			const U32 axis_and_right_child_index = node.inner.axis_and_right_child_index;

			//
			const int axis = (axis_and_right_child_index >> 30) - 1;

			AABBq	left_child_aabb_q = item.node_aabb;
			AABBq	right_child_aabb_q = item.node_aabb;

			left_child_aabb_q.max_corner[axis] = node.inner.split[0];
			right_child_aabb_q.min_corner[axis] = node.inner.split[1];

			if( quantizedAABBsIntersectOrTouch( left_child_aabb_q, quantized_query_aabb ) )
			{
				const U32 iChild0 = node_index + 1;	// the first (left) child is stored right after the node
				const Node& child0 = nodes[ iChild0 ];

				++iStackTop;
				todo_stack[ iStackTop ].node_aabb = left_child_aabb_q;
				todo_stack[ iStackTop ].node_index = iChild0;
			}

			if( quantizedAABBsIntersectOrTouch( right_child_aabb_q, quantized_query_aabb ) )
			{
				const U32 iChild1 = axis_and_right_child_index & Node::RIGHT_CHILD_INDEX_MASK;
				const Node& child1 = nodes[iChild1];

				++iStackTop;
				todo_stack[ iStackTop ].node_aabb = right_child_aabb_q;
				todo_stack[ iStackTop ].node_index = iChild1;
			}
		}
		else
		{
			const UINT tri_start = node.leaf.first_triangle_index;
			const UINT tri_end = tri_start + node.leaf.triangle_count;

			for( UINT iTriangle = tri_start; iTriangle < tri_end; iTriangle++ )
			{
				const QTri tri = tris[ iTriangle ];

				const UINT iV0 = (tri >> 43) & ((1<<21) - 1);
				const UINT iV1 = (tri >> 22) & ((1<<21) - 1);
				const UINT iV2 = (tri >>  0) & ((1<<22) - 1);

				const V3f a = BIH_DECODE_NORMALIZED_POS( verts[ iV0 ] );
				const V3f b = BIH_DECODE_NORMALIZED_POS( verts[ iV1 ] );
				const V3f c = BIH_DECODE_NORMALIZED_POS( verts[ iV2 ] );

				/*const*/ btVector3	faceverts[3] = {
					toBulletVec( a ),
					toBulletVec( b ),
					toBulletVec( c ),
				};

				//++num_triangles_tested;

				if( TestTriangleAgainstAabb2( faceverts, local_aabb_min, local_aabb_max ) )
				{
				/*const*/ btVector3	faceverts2[3] = {
					toBulletVec( a ) * uniform_scale_local_to_world,
					toBulletVec( b ) * uniform_scale_local_to_world,
					toBulletVec( c ) * uniform_scale_local_to_world,
				};

					callback->processTriangle(
						faceverts2
						, 0 // node.leaf.material_index /*partId*/
						, iTriangle /*triangleIndex*/
						);

#if DBG_BIH_VISUALIZE_TESTED_TRIANGLES

					const V3f nudged_corner = minimum_corner_in_world_space + CV3f(1e-2f);
					TbDebugLineRenderer::Get().addWireTriangle(
						a * uniform_scale_local_to_world + nudged_corner,
						b * uniform_scale_local_to_world + nudged_corner,
						c * uniform_scale_local_to_world + nudged_corner,
						RGBAf::RED
						);

#if 0
					const AABBf node_aabb = dequantizeLocalAABB( item.node_aabb );
					TbDebugLineRenderer::Get().addWireAABB(
						AABBf::make(
						node_aabb.min_corner * uniform_scale_local_to_world,
						node_aabb.max_corner * uniform_scale_local_to_world
						).shifted( minimum_corner_in_world_space ),
						getDebugColorForTreeLevel(iStackTop)
						);
#endif
#endif
				}
			}

		}//if leaf
	}//while node stack is not empty

//#if DEBUG_BIH
//	DBGOUT("num_triangles_tested: %d", num_triangles_tested);
//#endif
}

void NwBIH::WalkTreeAgainstRay(
	//NodeOverlapCallbackI * node_callback
	btTriangleCallback* triangle_callback
	, const btVector3& raySource, const btVector3& rayTarget
	, const btVector3& aabbMin, const btVector3& aabbMax
	, const AABBf& bih_bounds_local_space	//
	, const float uniform_scale_world_to_local
	) const
{
	btVector3 rayDirection = (rayTarget - raySource);
	rayDirection.safeNormalize();// stephengold changed normalize to safeNormalize 2020-02-17

	const btScalar lambda_max = rayDirection.dot(rayTarget - raySource);

	///what about division by zero? --> just set rayDirection[i] to 1.0
	rayDirection[0] = rayDirection[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[0];
	rayDirection[1] = rayDirection[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[1];
	rayDirection[2] = rayDirection[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[2];

	const unsigned int sign[3] = {rayDirection[0] < 0.0, rayDirection[1] < 0.0, rayDirection[2] < 0.0};


	/* Quick pruning by quantized box */
	btVector3 rayAabbMin = raySource;
	btVector3 rayAabbMax = raySource;
	rayAabbMin.setMin(rayTarget);
	rayAabbMax.setMax(rayTarget);

	/* Add box cast extents to bounding box */
	rayAabbMin += aabbMin;
	rayAabbMax += aabbMax;


	// the AABB of the whole ray
	const AABBf query_aabb_in_bih_local_space = AABBf::make(
		fromBulletVec(rayAabbMin) * uniform_scale_world_to_local,
		fromBulletVec(rayAabbMax) * uniform_scale_world_to_local
		);
	const AABBq quantized_query_aabb = quantizeLocalAABB( query_aabb_in_bih_local_space );


	//
	const float uniform_scale_local_to_world = 1 / uniform_scale_world_to_local;

	//
	const Node *	nodes = _nodes.begin();
	const QTri *	tris = _triangles.begin();
	const QVert *	verts = _vertices.begin();


	/// Node for storing state information during traversal.
	mxPREALIGN(16) struct BIHTraversal
	{
		U32		node_index;	// 4
		AABBq	node_aabb;	// 24
		U32		padding;	// 4
	} mxPOSTALIGN(16);

	mxSTATIC_ASSERT(sizeof(BIHTraversal) == 32);

	BIHTraversal todo_stack[64];

	todo_stack[0].node_aabb = quantizeLocalAABB( bih_bounds_local_space );
	todo_stack[0].node_index = 0;

	int	iStackTop = 0;

	while( iStackTop >= 0 )
	{
		const BIHTraversal& item = todo_stack[ iStackTop-- ];
		const U32 node_index = item.node_index;

		const Node& node = nodes[ node_index ];

		if( IS_INTERNAL_NODE( node ) )
		{
			const U32 axis_and_right_child_index = node.inner.axis_and_right_child_index;

			//
			const int axis = (axis_and_right_child_index >> 30) - 1;

			AABBq	left_child_aabb_q = item.node_aabb;
			AABBq	right_child_aabb_q = item.node_aabb;

			left_child_aabb_q.max_corner[axis] = node.inner.split[0];
			right_child_aabb_q.min_corner[axis] = node.inner.split[1];

			// First test node AABB against the whole ray/segment query AABB.
			// Then test the ray against the node AABB.

#define DEQUANTIZE_BULLET_VEC( quantized_coords )\
			btVector3( DEQUANTIZE_COORD(quantized_coords[0]), DEQUANTIZE_COORD(quantized_coords[1]), DEQUANTIZE_COORD(quantized_coords[2]) )

			//btScalar	ray_vs_node_tmin;

			if( quantizedAABBsIntersectOrTouch( left_child_aabb_q, quantized_query_aabb ) )
			{
				btVector3 bounds[2];
				bounds[0] = DEQUANTIZE_BULLET_VEC(left_child_aabb_q.min_corner);
				bounds[1] = DEQUANTIZE_BULLET_VEC(left_child_aabb_q.max_corner);

				/* Add box cast extents */
				bounds[0] -= aabbMax;
				bounds[1] -= aabbMin;
mxBUG("commented out, because some collsions are missed");
				// If the ray intersects the node's AABB:
				//if( btRayAabb2(raySource, rayDirection, sign, bounds, ray_vs_node_tmin, 0.0f, lambda_max) )
				{
					const U32 iChild0 = node_index + 1;	// the first (left) child is stored right after the node
					const Node& child0 = nodes[ iChild0 ];

					++iStackTop;
					todo_stack[ iStackTop ].node_aabb = left_child_aabb_q;
					todo_stack[ iStackTop ].node_index = iChild0;
				}
			}

			if( quantizedAABBsIntersectOrTouch( right_child_aabb_q, quantized_query_aabb ) )
			{
				btVector3 bounds[2];
				bounds[0] = DEQUANTIZE_BULLET_VEC(right_child_aabb_q.min_corner);
				bounds[1] = DEQUANTIZE_BULLET_VEC(right_child_aabb_q.max_corner);

				/* Add box cast extents */
				bounds[0] -= aabbMax;
				bounds[1] -= aabbMin;
mxBUG("commented out, because some collsions are missed");
				// If the ray intersects the node's AABB:
				//if( btRayAabb2(raySource, rayDirection, sign, bounds, ray_vs_node_tmin, 0.0f, lambda_max) )
				{
					const U32 iChild1 = axis_and_right_child_index & Node::RIGHT_CHILD_INDEX_MASK;
					const Node& child1 = nodes[iChild1];

					++iStackTop;
					todo_stack[ iStackTop ].node_aabb = right_child_aabb_q;
					todo_stack[ iStackTop ].node_index = iChild1;
				}
			}
		}
		else
		{
			const UINT tri_start = node.leaf.first_triangle_index;
			const UINT tri_end = tri_start + node.leaf.triangle_count;

			for( UINT iTriangle = tri_start; iTriangle < tri_end; iTriangle++ )
			{
				const QTri tri = tris[ iTriangle ];

				const UINT iV0 = (tri >> 43) & ((1<<21) - 1);
				const UINT iV1 = (tri >> 22) & ((1<<21) - 1);
				const UINT iV2 = (tri >>  0) & ((1<<22) - 1);

				const V3f a = BIH_DECODE_NORMALIZED_POS( verts[ iV0 ] );
				const V3f b = BIH_DECODE_NORMALIZED_POS( verts[ iV1 ] );
				const V3f c = BIH_DECODE_NORMALIZED_POS( verts[ iV2 ] );

				/*const*/ btVector3	faceverts[3] = {
					toBulletVec( a ),
					toBulletVec( b ),
					toBulletVec( c ),
				};

				//
				{
					/*const*/ btVector3	faceverts2[3] = {
						toBulletVec( a ) * uniform_scale_local_to_world,
						toBulletVec( b ) * uniform_scale_local_to_world,
						toBulletVec( c ) * uniform_scale_local_to_world,
					};

					triangle_callback->processTriangle(
						faceverts2
						, 0 // node.leaf.material_index /*partId*/
						, iTriangle /*triangleIndex*/
						);
//
//#if DBG_BIH_VISUALIZE_TESTED_TRIANGLES
//
//					const V3f nudged_corner = minimum_corner_in_world_space + CV3f(1e-2f);
//					TbDebugLineRenderer::Get().addWireTriangle(
//						a * uniform_scale_local_to_world + nudged_corner,
//						b * uniform_scale_local_to_world + nudged_corner,
//						c * uniform_scale_local_to_world + nudged_corner,
//						RGBAf::RED
//						);
//
//#if 0
//					const AABBf node_aabb = dequantizeLocalAABB( item.node_aabb );
//					TbDebugLineRenderer::Get().addWireAABB(
//						AABBf::make(
//						node_aabb.min_corner * uniform_scale_local_to_world,
//						node_aabb.max_corner * uniform_scale_local_to_world
//						).shifted( minimum_corner_in_world_space ),
//						getDebugColorForTreeLevel(iStackTop)
//						);
//#endif
//#endif
				}
			}

		}//if leaf
	}//while node stack is not empty
}

ERet NwBIH::ExtractVerticesAndTriangles(
	TArray<V3f>		&vertices_,
	TArray<UInt3>	&triangles_,
	const float uniform_scale_local_to_world
	) const
{
	mxDO(vertices_.setCountExactly(_vertices.count));
	mxDO(triangles_.setCountExactly(_triangles.count));

	const QVert* verts = _vertices.begin();
	for( UINT i = 0; i < _vertices.count; i++ )
	{
		const QVert vert = verts[i];
		const V3f vert_pos = BIH_DECODE_NORMALIZED_POS( vert ) * uniform_scale_local_to_world;
		vertices_._data[i] = vert_pos;
	}

	//
	const QTri* tris = _triangles.begin();
	for( UINT i = 0; i < _triangles.count; i++ )
	{
		const QTri tri = tris[i];
		const UINT iV0 = (tri >> 43) & ((1<<21) - 1);
		const UINT iV1 = (tri >> 22) & ((1<<21) - 1);
		const UINT iV2 = (tri >>  0) & ((1<<22) - 1);

		triangles_._data[i] = UInt3(iV0, iV1, iV2);
	}

	return ALL_OK;
}


/*
-----------------------------------------------------------------------------
	NwBIHBuilder
-----------------------------------------------------------------------------
*/

static
const RGBAf getDebugColorForTreeLevel( UINT depth );

NwBIHBuilder::NwBIHBuilder( AllocatorI & allocator )
	: _nodes( allocator )
	, _triangles( allocator )
	, _vertices( allocator )
{
	_bounds.clear();
}

#if 0
static
int calculateSplittingAxis( const Primitive* primitives
					   , const UINT startIndex
					   , const UINT indexCount
					   , const AABBf& node_aabb
					   , U32 *primitive_indices
					   , AABBf *leftBounds_
					   , AABBf *rightBounds_
					   , UINT *primitivesRight_	// the number of primitives assigned to the right child
					   )
{
	// determine the longest axis of the bounding box
	const V3f node_aabb_size = node_aabb.size();
	const int longest_axis = Max3Index( node_aabb_size.x, node_aabb_size.y, node_aabb_size.z );

	int	splitAxis;	// the split axis

	// Split based on the center of the longest axis

	AABBf leftBounds;
	AABBf rightBounds;
	UINT primitivesRight;	// the number of primitives assigned to the right child

	// iterate possible split axis choices, starting with the longest axis.
	// if all fail it means all children have the same bounds and we simply split
	// the list in half because each node can only have two children.
	for( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		// pick an axis
		splitAxis = (longest_axis + iAxis) % 3;
		// sort children into left and right lists
		const float splitValue = (node_aabb.mins[splitAxis] + node_aabb.maxs[splitAxis]) * 0.5f;

		AABBf_Clear( &leftBounds );
		AABBf_Clear( &rightBounds );
		primitivesRight = 0;

		// Because the bounding interval hierarchy is an object partitioning scheme,
		// all object sorting can be done in place and no temporary memory management is required.
		// The recursive construction procedure only needs two pointers
		// to the left and right objects in the index array similar to the quick-sort algorithm.
		UINT left = startIndex;	//!< index of the last left item
		UINT right = startIndex + indexCount - 1;	//!< index of the first right item

		while( left <= right )
		{
			const UINT iPrimitive = primitive_indices[ left ];
			const Primitive& primitive = primitives[ iPrimitive ];

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
				TSwap( primitive_indices[left], primitive_indices[right] );	// move this item to the rightmost position
				--right;
				AABBf_AddAABB( &rightBounds, primitive.bounds );
			}
		}//for each primitive

		// if both sides have some children, it's good enough for us.
		if( primitivesRight < indexCount ) {
			break;
		}
	}//for each candidate axis

	// If we get a bad split, just choose the center...
	if( !primitivesRight || primitivesRight == indexCount )
	{
		// somewhat common case: no good choice, divide children arbitrarily
		primitivesRight = indexCount / 2;

		// calculate extents
		leftBounds = ComputeBounds(
			primitive_indices + startIndex,
			indexCount - primitivesRight,
			primitives
			);
		rightBounds = ComputeBounds(
			primitive_indices + startIndex + (indexCount - primitivesRight),
			primitivesRight,
			primitives
			);

		splitAxis = longest_axis;
	}

	*leftBounds_ = leftBounds;
	*rightBounds_ = rightBounds;
	*primitivesRight_ = primitivesRight;

	return splitAxis;
}
#endif



namespace
{
	/// temporary struct used for building a BIH
	struct Primitive
	{
		AABBf	bounds;	//24
		V3f		center;	//12 precached center of the bounding box
	};

	static
	const AABBf computeBounds( const Primitive* primitives
							  , const U32* primitive_indices
							  , const U32 primitive_range_start
							  , const U32 primitive_range_end
							  )
	{
		AABBf	bounds;
		AABBf_Clear( &bounds );

		for( UINT ii = primitive_range_start; ii < primitive_range_end; ii++ )
		{
			const UINT primitive_index = primitive_indices[ ii ];
			const Primitive& primitive = primitives[ primitive_index ];
			AABBf_AddAABB( &bounds, primitive.bounds );
		}

		return bounds;
	}

	static
	void GatherPrimitives( const StridedTrianglesT& triangles
						  , const StridedPositionsT& vertices
						  , Primitive *primitives
						  , U32 *prim_indices
						  , AABBf *scene_bounds )
	{
		for( UINT iTriangle = 0; iTriangle < triangles.count; iTriangle++ )
		{
			const UInt3& T = triangles.GetAt( iTriangle );
			const V3f& V1 = vertices.GetAt( T.a[0] );
			const V3f& V2 = vertices.GetAt( T.a[1] );
			const V3f& V3 = vertices.GetAt( T.a[2] );

			Primitive& primitive = primitives[ iTriangle ];

			primitive.bounds.min_corner = V3_Mins( V3_Mins( V1, V2 ), V3 );
			primitive.bounds.max_corner = V3_Maxs( V3_Maxs( V1, V2 ), V3 );
			primitive.center = AABBf_Center( primitive.bounds );

			// Increase the size of the bounding box to avoid numerical issues.
			primitive.bounds = AABBf_GrowBy( primitive.bounds, 1e-4f );

			*scene_bounds = AABBf_Merge( *scene_bounds, primitive.bounds );

			prim_indices[iTriangle] = iTriangle;
		}
	}

	/// returns splitting axis index (0 = x, 1 = y, z = z).
	typedef int partitionTriangles_Fun( const Primitive* primitives
									   , const UINT first_prim_index
									   , const UINT prim_index_count
									   , const AABBf& containing_aabb
									   , U32 * primitive_indices
									   , AABBf *leftBounds_
									   , AABBf *rightBounds_
									   , UINT *primitivesRight_	// the number of primitives assigned to the right child
									   );

	///
	static
	int partitionTriangles_Fast( const Primitive* primitives
								, const UINT prim_range_start
								, const UINT prim_range_end
								, const AABBf& containing_aabb
								, U32 * primitive_indices
								, UINT *mid_	// the end of range of primitives assigned to the left child
								)
	{
		// Partition the list of objects into two halves.

		U32 mid = prim_range_start;

		// Split on the center of the longest axis
		const int splitting_axis = AABBf_LongestAxis( containing_aabb );
		const float split_coord = (containing_aabb.min_corner[splitting_axis] + containing_aabb.max_corner[splitting_axis]) * 0.5f;

		// Partition the list of objects on this split
		for( U32 i = prim_range_start; i < prim_range_end; ++i )
		{
			if( primitives[ primitive_indices[ i ] ].center[ splitting_axis ] < split_coord ) {
				TSwap( primitive_indices[ i ], primitive_indices[ mid ] );
				++mid;
			}
		}

		// If we get a bad split, just choose the center...
		if( mid == prim_range_start || mid == prim_range_end ) {
			mid = prim_range_start + (prim_range_end - prim_range_start) / 2;
		}

		*mid_ = mid;

		return splitting_axis;
	}

}//namespace



static inline
void setupInternalNode( NwBIH::Node &node_
					   , int splitting_axis
					   , float left_plane, float right_plane
					   , U32 right_child_index
					   )
{
	node_.type_and_type_specific_data = ( ( U32(splitting_axis) + 1 ) << 30 ) | right_child_index;
	node_.inner.split[0] = quantizeMaxBound( left_plane );
	node_.inner.split[1] = quantizeMinBound( right_plane );
}

static inline
void setupLeafNode( NwBIH::Node &node_
				   , U32 first_triangle_index
				   , U16 triangle_count
				   , U16 material_index
				   )
{
	node_.leaf.first_triangle_index = first_triangle_index;
	node_.leaf.triangle_count = triangle_count;
	node_.leaf.material_index = material_index;
}

ERet NwBIHBuilder::Build( const TriangleMeshI& trimesh
						 , const Options& build_options
						 , AllocatorI & scratch
						 )
{
	//
	const StridedTrianglesT	src_triangles = trimesh.GetTriangles();
	const StridedPositionsT	src_vertices = trimesh.GetVertexPositions();

	const UINT total_num_triangles = src_triangles.count;
	mxASSERT(total_num_triangles);
	mxDO(_triangles.setCountExactly( total_num_triangles ));

	const UINT total_num_vertices = src_vertices.count;
	mxDO(_vertices.setCountExactly( total_num_vertices ));

#if DEBUG_BIH
	DBGOUT("NwBIH::build(): %d tris, %d verts", total_num_triangles, total_num_vertices);
#endif

	// Precompute bounding boxes for all objects to store...

	Primitive * primitives;
	mxTRY_ALLOC_SCOPED( primitives, total_num_triangles, scratch );

	U32 *	prim_indices;	// sorted indices of stored primitives (triangles)
	mxTRY_ALLOC_SCOPED( prim_indices, total_num_triangles, scratch );

	// ...and compute the bounding box of the whole scene.
	AABBf scene_bounds;
	scene_bounds.clear();

	//
	GatherPrimitives(
		src_triangles
		, src_vertices
		, primitives
		, prim_indices
		, &scene_bounds
	);

	const AABBf UNIT_CUBE = { CV3f(0), CV3f(1) };
	mxASSERT2(UNIT_CUBE.contains( scene_bounds ),
		"The mesh must lie inside the unit cube [0..1] to improve quantization perf"
		);

	//
	for( UINT i = 0; i < total_num_vertices; i++ ) {
		_vertices[i] = BIH_ENCODE_NORMALIZED_POS( src_vertices.GetAt(i) );
	}

	_bounds = scene_bounds;

	//
	struct BuildItem
	{
		AABBf	bounds;	//24

		/// If non-zero then this is the index of the parent (used in offsets).
		U32 parent_index;

		/// The range of objects in the object list covered by this node.
		U32	triangle_range_start;	// index of the first triangle to consider
		U32	triangle_range_end;

		//36
	};

	enum { BUILD_STACK_SIZE = 128 };
	BuildItem	todo_stack[ BUILD_STACK_SIZE ];

	U32 stack_top = 0;
	const U32 Untouched    = 0xFFFFFFFF & NwBIH::Node::RIGHT_CHILD_INDEX_MASK;
	const U32 TouchedTwice = Untouched - 2;
	const U32 RootNodeID   = TouchedTwice - 1;

	// Push the root node.
	{
		BuildItem &	root_item = todo_stack[ stack_top ];
		root_item.bounds = scene_bounds;
		root_item.parent_index = RootNodeID;
		root_item.triangle_range_start = 0;
		root_item.triangle_range_end = total_num_triangles;
		++stack_top;
	}

	mxDO(_nodes.reserve( total_num_triangles * 2 ));

	// Stats:
	UINT leafCount = 0;	//!< number of (terminal) leaves
	UINT max_tree_depth = 1;

	// In-place quick sort-like construction algorithm with global heuristic
	while( stack_top > 0 )
	{
		max_tree_depth = largest( max_tree_depth, stack_top );

		// Pop the next item off of the stack
		const BuildItem& build_item = todo_stack[ --stack_top ];

		const U32 triangle_range_start	 = build_item.triangle_range_start;
		const U32 triangle_range_end	 = build_item.triangle_range_end;
		const U32 num_triangles_to_check = triangle_range_end - triangle_range_start;

		//
		const U32 new_node_index = _nodes.num();	// index of the new node

		// Child touches parent...

		// Special case: Don't do this for the root - the root doesn't have a parent node.
		const U32 parent_node_index = build_item.parent_index;
		if( parent_node_index != RootNodeID )
		{
			NwBIH::Node &	parent_node = _nodes[ parent_node_index ];

			U32 right_child_index = GET_RIGHT_CHILD_INDEX( parent_node );
			--right_child_index;
			SET_RIGHT_CHILD_INDEX( parent_node, right_child_index );

			// When this is the second touch, this is the right child.
			// The right child sets up the offset for the flat tree.
			if( right_child_index == TouchedTwice ) {
				SET_RIGHT_CHILD_INDEX( parent_node, new_node_index );
			}
		}

		// If the number of primitives at this point is less than the leaf
		// size, then this will become a leaf node.

		NwBIH::Node node;

		const bool is_internal_node = ( num_triangles_to_check > build_options.maxTrisPerLeaf )
									&& ( stack_top <= build_options.maxTreeDepth )
									;
		//
		if( is_internal_node )
		{
			U32		mid;	// [triangle_range_start .. triangle_range_end]

			const int split_axis = partitionTriangles_Fast(
				primitives
				, triangle_range_start
				, triangle_range_end
				, build_item.bounds
				, prim_indices
				, &mid
				);

#if DEBUG_BIH
			DBGOUT("Node[%d]: split axis: %d, start tri: %d, num tris: %d"
				, new_node_index, split_axis, triangle_range_start, triangle_range_end-triangle_range_start);
#endif

			const AABBf	left_bounds = computeBounds( primitives, prim_indices
				, triangle_range_start, mid
				);

			const AABBf	right_bounds = computeBounds( primitives, prim_indices
				, mid, triangle_range_end
				);

			const float clipL = left_bounds.max_corner[ split_axis ];	// the maximum bounding value of all objects on the left
			const float clipR = right_bounds.min_corner[ split_axis ];// the minimum bounding value of those on the right

			setupInternalNode( node
				, split_axis
				, clipL, clipR
				, Untouched	// children will decrement this field later during building
				);

			// we now have Left and Right primitives sorted in 'prim_indices' array...

			// Push the right child first: when the right child is popped off the stack (after the left child),
			// it will initialize the 'right child offset' in the parent.
			todo_stack[ stack_top ].bounds = right_bounds;
			todo_stack[ stack_top ].triangle_range_start = mid;
			todo_stack[ stack_top ].triangle_range_end = triangle_range_end;
			todo_stack[ stack_top ].parent_index = new_node_index;
			++stack_top;

			// Then push the left child.
			todo_stack[ stack_top ].bounds = left_bounds;
			todo_stack[ stack_top ].triangle_range_start = triangle_range_start;
			todo_stack[ stack_top ].triangle_range_end = mid;
			todo_stack[ stack_top ].parent_index = new_node_index;
			++stack_top;
		}
		else
		{
			// If this is a leaf, no need to subdivide.
			setupLeafNode( node, triangle_range_start, num_triangles_to_check, 0 );
			leafCount++;
		}

		_nodes.add( node );
	}

	_nodes.shrink();

	//
	NwBIH::QTri *tris = _triangles.raw();
	for( UINT i = 0; i < total_num_triangles; i++ )
	{
		const UInt3& T = src_triangles.GetAt( prim_indices[ i ] );
		const U64 i0 = T.a[0];
		const U64 i1 = T.a[1];
		const U64 i2 = T.a[2];
		tris[i] = (i0 << 43) | (i1 << 22) | i2;
	}

#if DEBUG_BIH
	DBGOUT("Built tree: %d nodes, %d leaves, tree depth: %d, node data size: %d bytes), total size: %d bytes"
			, _nodes.num() - leafCount, leafCount, max_tree_depth, _nodes.rawSize(), _nodes.rawSize() + _triangles.rawSize() + _vertices.rawSize());
#endif

	return ALL_OK;
}




namespace AWriter_
{
	ERet WriteBytes(AWriter& writer, const TSpan<char>& bytes)
	{
		return writer.Write(
			bytes._data,
			bytes._count * sizeof(bytes._data[0])
			);
	}

	template< class CONTAINER >
	ERet WriteRawBytes(AWriter& writer, const CONTAINER& container)
	{
		return writer.Write(
			(const char*) container.raw(),
			container.rawSize()
			);
	}

}//namespace AWriter_


//namespace TRelArray
template< class T >
static mxFORCEINLINE
void SetOffsetFromPtr( TRelArray<T> & rel_array, const T* obj_ptr )
{
	rel_array.offset = mxGetByteOffset32(&rel_array, obj_ptr);
}


ERet NwBIHBuilder::SaveToBlob( NwBlob &blob_ )
{
	NwBlobWriter	blob_writer(
		blob_
		, blob_.num()
		);

	//
	const size_t	start_offset = tbALIGN16(blob_.num());

	//
	size_t	curr_offset = start_offset;

	curr_offset += sizeof(NwBIH);
	curr_offset = tbALIGN16(curr_offset);

	const size_t	nodes_offset = curr_offset;
	curr_offset += _nodes.rawSize();
	curr_offset = tbALIGN16(curr_offset);

	const size_t	triangles_offset = curr_offset;
	curr_offset += _triangles.rawSize();
	curr_offset = tbALIGN16(curr_offset);

	const size_t	vertices_offset = curr_offset;
	curr_offset += _vertices.rawSize();
	curr_offset = tbALIGN16(curr_offset);

	//
	const size_t	total_bih_data_size = curr_offset;

	mxDO(blob_writer.reserveMoreBufferSpace(
		total_bih_data_size
		, Arrays::ResizeExactly
		));

	//
	blob_writer.alignBy(16);
	blob_writer.Put(NwBIH());

	blob_writer.alignBy(16);
	const NwBIH::Node* nodes_start = (NwBIH::Node*) blob_writer.end();
	AWriter_::WriteRawBytes(blob_writer, _nodes);

	blob_writer.alignBy(16);
	const NwBIH::QTri* triangles_start = (NwBIH::QTri*) blob_writer.end();
	AWriter_::WriteRawBytes(blob_writer, _triangles);

	blob_writer.alignBy(16);
	const NwBIH::QVert* vertices_start = (NwBIH::QVert*) blob_writer.end();
	AWriter_::WriteRawBytes(blob_writer, _vertices);

	blob_writer.alignBy(16);

	//
	NwBIH &new_bih = *(NwBIH*) mxAddByteOffset(
		blob_.raw(), start_offset
		);

	SetOffsetFromPtr(new_bih._nodes, nodes_start);
	new_bih._nodes.count = _nodes.num();
	
	SetOffsetFromPtr(new_bih._triangles, triangles_start);
	new_bih._triangles.count = _triangles.num();
	
	SetOffsetFromPtr(new_bih._vertices, vertices_start);
	new_bih._vertices.count = _vertices.num();

#if 0//DEBUG_BIH
	DBGOUT( "NwBIH::saveTo: %d nodes, %d tris, %d verts, hash: %ull"
			, header->num_nodes, header->num_tris, header->num_verts, header->hash );
#endif

	return ALL_OK;
}
