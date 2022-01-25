//
#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/BoundingVolumes/ViewFrustum.h>
// TbPrimitiveBatcher
#include <Graphics/graphics_utilities.h>

#include "game_app.h"
#include "rendering/game_renderer.h"

#include "physics/aabb_tree.h"


#define DBG_AABB_TREE	(0)//(MX_DEBUG)
#define USE_SSE_CULLING	(1)



mxSTOLEN("Heavily based on https://github.com/brandonpelfrey/Fast-BVH");

namespace
{
	/// NOTE: objects are stored only in leaf nodes!
	mxPREALIGN(16)
	struct BVHNode
	{
		V3f		aabb_min;	//12
		union {
			/// BVHNode data:
			U32	unused;	// must be unused! it's non-zero only for leaves!

			/// Leaf data:
			U32	count;	//!< number of primitives stored in this leaf
		};

		V3f		aabb_max;	//12
		union {
			/// BVHNode data:
			/// index of the second (right) child
			/// (the first (left) child goes right after the node)
			U32	right_child_idx;	// absolute, but could use relative offsets

			/// Leaf data:
			U32	start;	//!< index of the first primitive
		};

	public:
		mxFORCEINLINE BOOL IsLeaf() const
		{
			return count;	// if count != 0 then this is a leaf
		}
	};
	mxSTATIC_ASSERT(sizeof(BVHNode) == sizeof(SimdAabb));


	enum {
		ROOT_NODE_INDEX = 0,
		NODE_STACK_DEPTH = 128
	};


// slows down in debug!
#if 0//DBG_AABB_TREE
		typedef TStaticArray< U32, NODE_STACK_DEPTH >	NodeStackT;
#else
		typedef U32	NodeStackT[ NODE_STACK_DEPTH ];
#endif


	class SgAABBTreeInternal: public SgAABBTree
	{
	public:
		DynamicArray< BVHNode >		nodes;

		DynamicArray< CulledObjectID >	obj_ids;

		const SgAABBTreeConfig	cfg;

		AllocatorI& _allocator;

	public:
		SgAABBTreeInternal(
			const SgAABBTreeConfig& cfg
			, AllocatorI& allocator
			)
			: cfg(cfg)
			, nodes(allocator)
			, obj_ids(allocator)
			, _allocator(allocator)
		{
		}
	};

	/*! Build the BVH, given an input data set
	 *  - Handling our own stack is quite a bit faster than the recursive style.
	 *  - Each build stack entry's parent field eventually stores the offset
	 *    to the parent of that node. Before that is finally computed, it will
	 *    equal exactly three other values. (These are the magic values Untouched,
	 *    Untouched-1, and TouchedTwice).
	 *  - The partition here was also slightly faster than std::partition.
	 */
	ERet BuildBVH(
		SgAABBTreeInternal& bvh
		, const TSpan< const SimdAabb >& aabbs
		, FillCulledObjectIDsArray* fill_obj_ids_fun
		, void* fill_obj_ids_fun_data0
		, void* fill_obj_ids_fun_data1
		, AllocatorI & scratch_allocator
		)
	{
		const SimdAabb* src_aabbs = aabbs._data;
		const UINT num_items = aabbs._count;

		mxENSURE(num_items <= bvh.cfg.max_item_count, ERR_BUFFER_TOO_SMALL, "");

		//
		const U32 max_num_bvh_nodes = num_items*2;

		//
		mxDO(bvh.nodes.setCountExactly( max_num_bvh_nodes ));
		mxDO(bvh.obj_ids.setCountExactly( num_items ));

		if(!num_items) {
			return ALL_OK;
		}

		//
		(*fill_obj_ids_fun)(
			bvh.obj_ids._data
			, num_items
			, fill_obj_ids_fun_data0
			, fill_obj_ids_fun_data1
			);


		// to avoid recomputing AABB centers
		Vector4 *	src_aabbs_centers;
		mxTRY_ALLOC_SCOPED( src_aabbs_centers, num_items, scratch_allocator );


		U32 *	raw_item_indices;
		mxTRY_ALLOC_SCOPED( raw_item_indices, num_items, scratch_allocator );

		//
#if DBG_AABB_TREE
		TSpan< CulledObjectID >	item_indices( raw_item_indices, num_items );
#else
		CulledObjectID *	item_indices = raw_item_indices;
#endif


		const Vector4 v_half = V4_REPLICATE(0.5f);

		for(U32 item_index = 0; item_index < num_items; item_index++)
		{
			item_indices[ item_index ] = item_index;

			//
			const SimdAabb aabb = src_aabbs[ item_index ];
			src_aabbs_centers[ item_index ] = V4_MUL(
				V4_ADD(aabb.mins, aabb.maxs)
				, v_half
				);
		}

		//
#if DBG_AABB_TREE
		TSpan< BVHNode >	bvh_nodes( bvh.nodes._data, max_num_bvh_nodes );
#else
		BVHNode *	bvh_nodes = bvh.nodes._data;
#endif
		//
		U32	num_added_bvh_nodes = 0;


		//
		struct BVHBuildEntry {
			/// If non-zero then this is the index of the parent (used in offsets).
			U32 parent;
			/// The range of objects in the object list covered by this node.
			U32 start, end;
		};
		BVHBuildEntry todo[ NODE_STACK_DEPTH ];
		U32 stack_top = 0;

		//
		const U32 Untouched		= 0xFFFFFFFF;
		const U32 TouchedTwice	= 0xFFFFFFFD;
		const U32 RootNodeID	= 0xFFFFFFFC;

		// Push the root
		todo[stack_top].start = 0;
		todo[stack_top].end = num_items;
		todo[stack_top].parent = RootNodeID;
		stack_top++;

		UINT leafCount = 0;	//!< number of (terminal) leaves

		// In-place quick sort-like construction algorithm with global heuristic
		while( stack_top > 0 )
		{
			// Pop the next item off of the stack
			const BVHBuildEntry& bnode = todo[--stack_top];
			const U32 start = bnode.start;
			const U32 end = bnode.end;
			const U32 primitive_count = end - start;

			//
			BVHNode new_node;


			// Build a bounding box from the primitives' centroids

			const U32		start_item_index = item_indices[ start ];
			const SimdAabb	start_item_aabb = src_aabbs[ start_item_index ];
			const Vector4	start_item_aabb_center = src_aabbs_centers[ start_item_index ];
			//
			SimdAabb centers_aabb = {
				start_item_aabb_center,
				start_item_aabb_center
			};


			// Calculate the bounding box for this node
			{
				// bounding box from the primitives' boxes
				SimdAabb bb( start_item_aabb );
				for( U32 j = start + 1; j < end; j++ )
				{
					const U32 item_index = item_indices[ j ];
					const SimdAabb item_aabb = src_aabbs[ item_index ];
					SimdAabb_ExpandToIncludeAABB( &bb, item_aabb );
					SimdAabb_ExpandToIncludePoint( &centers_aabb, src_aabbs_centers[ item_index ] );
				}
				(SimdAabb&)new_node = bb;
			}

			new_node.count = 0;
			new_node.right_child_idx = Untouched;	// children will decrement this field later during building

			// If the number of primitives at this point is less than the leaf
			// size, then this will become a leaf (Signified by right_child_idx == 0)
			if( primitive_count <= bvh.cfg.max_items_per_leaf )
			{
				new_node.count = primitive_count;
				new_node.start = start;
				leafCount++;
			}


			mxASSERT(num_added_bvh_nodes < max_num_bvh_nodes);
			const U32 iThisNode = num_added_bvh_nodes;	// index of the new node
			bvh_nodes[ num_added_bvh_nodes++ ] = new_node;

			// Child touches parent...
			// Special case: Don't do this for the root.
			const U32 parentIndex = bnode.parent;
			if( parentIndex != RootNodeID )
			{
				bvh_nodes[ parentIndex ].right_child_idx--;

				// When this is the second touch, this is the right child.
				// The right child sets up the offset for the flat tree.
				if( bvh_nodes[ parentIndex ].right_child_idx == TouchedTwice ) {
					bvh_nodes[ parentIndex ].right_child_idx = iThisNode;
				}
			}

			// If this is a leaf, no need to subdivide.
			if( new_node.IsLeaf() ) {
				continue;
			}

			// Partition the list of objects into two halves.
			U32 mid = start;
			{
				// Split on the center of the longest axis
				const AABBf centers_aabb_f = {
					Vector4_As_V3(centers_aabb.mins),
					Vector4_As_V3(centers_aabb.maxs),
				};

				const U32 split_axis = AABBf_LongestAxis( centers_aabb_f );
				const float split_coord = (centers_aabb_f.min_corner[split_axis] + centers_aabb_f.max_corner[split_axis]) * 0.5f;
				// Partition the list of objects on this split
				for( U32 i = start; i < end; i++ )
				{
					const U32 item_index = item_indices[ i ];
					const V3f& item_aabb_center = Vector4_As_V3( src_aabbs_centers[ item_index ] );
					if( item_aabb_center.a[ split_axis ] < split_coord )
					{
						TSwap( item_indices[i], item_indices[mid] );
						TSwap( bvh.obj_ids._data[i], bvh.obj_ids._data[mid] );
						++mid;
					}
				}
			}

			// If we get a bad split, just choose the center...
			if( mid == start || mid == end ) {
				mid = start + (end - start) / 2;
			}

			// Push the right child first: when the right child is popped off the stack (after the left child),
			// it will initialize the 'right child offset' in the parent.
			todo[stack_top].start = mid;
			todo[stack_top].end = end;
			todo[stack_top].parent = iThisNode;
			stack_top++;

			// Then push the left child.
			todo[stack_top].start = start;
			todo[stack_top].end = mid;
			todo[stack_top].parent = iThisNode;
			stack_top++;
		}

		//
		mxASSERT(num_added_bvh_nodes <= max_num_bvh_nodes);

		//
		bvh.nodes.setNum( num_added_bvh_nodes );

		return ALL_OK;
	}

}//namespace


ERet SgAABBTree::Create(
			SgAABBTree *&aabb_tree_
			, const SgAABBTreeConfig& cfg
			, AllocatorI& allocator
			)
{
	SgAABBTreeInternal *	new_aabb_tree;
	mxDO(nwCREATE_OBJECT(
		new_aabb_tree
		, allocator
		, SgAABBTreeInternal
		, cfg
		, allocator
		));

	TAutoDestroy< SgAABBTreeInternal >	auto_destroy_aabb_tree(
		new_aabb_tree
		, allocator
		);

	mxDO(new_aabb_tree->nodes.reserveExactly(cfg.max_item_count*2));
	mxDO(new_aabb_tree->obj_ids.reserveExactly(cfg.max_item_count));

	auto_destroy_aabb_tree.Disown();
	aabb_tree_ = new_aabb_tree;

	return ALL_OK;
}

void SgAABBTree::Destroy(SgAABBTree* p)
{
	SgAABBTreeInternal* o = (SgAABBTreeInternal*)p;
	nwDESTROY_OBJECT(o, o->_allocator);
}

ERet SgAABBTree::Update(
	const TSpan< const SimdAabb >& aabbs
	, FillCulledObjectIDsArray* fill_obj_ids_fun
	, void* fill_obj_ids_fun_data0
	, void* fill_obj_ids_fun_data1
	, AllocatorI & scratch_allocator
	)
{
	return BuildBVH(
		*(SgAABBTreeInternal*)this
		, aabbs
		, fill_obj_ids_fun
		, fill_obj_ids_fun_data0
		, fill_obj_ids_fun_data1
		, scratch_allocator
		);
}

namespace
{
	struct SimdFrustum: NwNonCopyable
	{
		__m128	simd_planes[VF_CLIP_PLANES];
		__m128i	plane_masks[VF_CLIP_PLANES];

	public:
		SimdFrustum(const ViewFrustum& view_frustum)
		{
			for( UINT iPlane = 0; iPlane < VF_CLIP_PLANES; ++iPlane )
			{
				const int normal_signs = view_frustum.signs[ iPlane ];

				simd_planes[ iPlane ] = _mm_loadu_ps(
					view_frustum.planes[ iPlane ].a
					);

				plane_masks[ iPlane ] = _mm_set_epi32(
					0,
					( normal_signs & 4 ) ? 0xFFFFFFFF : 0,
					( normal_signs & 2 ) ? 0xFFFFFFFF : 0,
					( normal_signs & 1 ) ? 0xFFFFFFFF : 0
					);
			}
		}
	};


	//
	//	ViewFrustum::Classify
	//
	//	See: http://www.flipcode.com/articles/article_frustumculling.shtml
	//
	//	NOTE: This test assumes frustum simd_planes face inward.
	//
	SpatialRelation::Enum Test_AABB_against_ViewFrustum(
		const SimdAabb& simd_aabb
		, const SimdFrustum& simd_frustum
		)
	{
		// See "Geometric Approach - Testing Boxes II":
		// https://cgvr.cs.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html

		// will be 0xFFFFFFFF if the box is behind at least one frustum plane
		__m128	box_is_outside_frustum_mask = _mm_setzero_ps();

		// will be 0xFFFFFFFF if the box crosses at least one frustum plane
		__m128	box_intersects_frustum_mask = _mm_setzero_ps();

/* Reference:

		for( UINT iPlane = 0; iPlane < VF_CLIP_PLANES; ++iPlane )
		{
			// p_vertex is diagonally opposed to n_vertex

			const __m128i	mask = simd_frustum.plane_masks[ iPlane ];

			const __m128	p_vertex = Vector4_Select(
				simd_aabb.mins,
				simd_aabb.maxs,
				_mm_castsi128_ps(mask)
				);
			const __m128	n_vertex = Vector4_Select(
				simd_aabb.maxs,
				simd_aabb.mins,
				_mm_castsi128_ps(mask)
				);


			// Testing a single vertex is enough for the cases where the box is outside.

			// is the positive vertex outside the frustum?
			const __m128 p_vertex_is_behind_plane_mask = _mm_cmplt_ps(
				Plane_DotCoord( simd_frustum.simd_planes[ iPlane ], p_vertex )
				, g_MM_Zero
				);

			box_is_outside_frustum_mask = _mm_or_ps( box_is_outside_frustum_mask, p_vertex_is_behind_plane_mask );


			// The second vertex needs only to be tested if one requires distinguishing
			// between boxes totally inside and boxes partially inside the view view_frustum.

			// is the negative vertex outside the frustum?
			const __m128 n_vertex_is_behind_plane_mask = _mm_cmplt_ps(
				Plane_DotCoord( simd_frustum.simd_planes[ iPlane ], n_vertex )
				, g_MM_Zero
				);
			box_intersects_frustum_mask = _mm_or_ps( box_intersects_frustum_mask, n_vertex_is_behind_plane_mask );
		}

		// these are either 0 or 0xFFFFFFFF:
		const int	box_is_outside_frustum_flag = _mm_cvtsi128_si32( _mm_castps_si128(box_is_outside_frustum_mask) );
		const int	box_intersects_frustum_flag = _mm_cvtsi128_si32( _mm_castps_si128(box_intersects_frustum_mask) );
		
		return box_is_outside_frustum_flag
			? SpatialRelation::Outside
			: box_intersects_frustum_flag
			? SpatialRelation::Intersects
			: SpatialRelation::Inside
			;
*/


// unroll:

/// to preserve blank spaces after auto-formatting
#define NL__

#define TEST_AABB_P_N_VERTICES_AGAINST_PLANE( PLANE_INDEX )\
		{\
NL__		const __m128i	mask = simd_frustum.plane_masks[ PLANE_INDEX ];\
NL__		\
NL__		const __m128	p_vertex = Vector4_Select(\
NL__			simd_aabb.mins,\
NL__			simd_aabb.maxs,\
NL__			_mm_castsi128_ps(mask)\
NL__			);\
NL__		const __m128	n_vertex = Vector4_Select(\
NL__			simd_aabb.maxs,\
NL__			simd_aabb.mins,\
NL__			_mm_castsi128_ps(mask)\
NL__			);\
NL__			\
NL__		const __m128 plane_eq = simd_frustum.simd_planes[ PLANE_INDEX ];\
NL__		\
NL__		box_is_outside_frustum_mask = _mm_or_ps(\
NL__			_mm_cmplt_ps(\
NL__				Plane_DotCoord( plane_eq, p_vertex )\
NL__				, g_MM_Zero \
NL__			),\
NL__			box_is_outside_frustum_mask \
NL__		);\
NL__		\
NL__		box_intersects_frustum_mask = _mm_or_ps(\
NL__			_mm_cmplt_ps(\
NL__				Plane_DotCoord( plane_eq, n_vertex )\
NL__				, g_MM_Zero \
NL__				),\
NL__			box_intersects_frustum_mask \
NL__		);\
NL__	}



TEST_AABB_P_N_VERTICES_AGAINST_PLANE(VF_NEAR_PLANE);

TEST_AABB_P_N_VERTICES_AGAINST_PLANE(VF_LEFT_PLANE);
TEST_AABB_P_N_VERTICES_AGAINST_PLANE(VF_RIGHT_PLANE);

TEST_AABB_P_N_VERTICES_AGAINST_PLANE(VF_BOTTOM_PLANE);
TEST_AABB_P_N_VERTICES_AGAINST_PLANE(VF_TOP_PLANE);

// don't test against the far plane


#undef TEST_AABB_P_N_VERTICES_AGAINST_PLANE
#undef NL__


		// these are either 0 or 0xFFFFFFFF:
		const int	box_is_outside_frustum_flag = _mm_cvtsi128_si32( _mm_castps_si128(box_is_outside_frustum_mask) );
		const int	box_intersects_frustum_flag = _mm_cvtsi128_si32( _mm_castps_si128(box_intersects_frustum_mask) );
		

#define B_SELECT_MASKED( A, B, MASK )	((A & MASK) | (B & ~MASK))

		return (SpatialRelation::Enum) B_SELECT_MASKED(
			SpatialRelation::Outside,
				B_SELECT_MASKED(
					SpatialRelation::Intersects,
					SpatialRelation::Inside,
					box_intersects_frustum_flag
				),
				box_is_outside_frustum_flag
			);

#undef B_SELECT_MASKED

	}


	//

#define SQUARE(X)	(X*X)

	mmGLOBALCONST Vector4 g_LOD_distances =
	{
#if !MX_DEBUG
		SQUARE(100.0f),
		SQUARE(200.0f),
		SQUARE(400.0f),
		SQUARE(800.0f)
#else
		SQUARE(10.0f),
		SQUARE(20.0f),
		SQUARE(50.0f),
		SQUARE(90.0f)
#endif
	};

#undef SQUARE



	//
	U32 CalculateLOD(
		const SimdAabb& simd_aabb
		, const Vector4& eye_position
		)
	{
		const Vector4 v_aabb_center = SimdAabb_GetCenter(simd_aabb);
 
		const Vector4 v_diff = V4_SUB( v_aabb_center, eye_position );
		const Vector4 v_dist_sq = Vector4_Dot3( v_diff, v_diff );

		const Vector4 v_cmp_mask = V4_CMPGT(v_dist_sq, g_LOD_distances);

		// the number of set bits == LOD index
		const U32 cmp_mask = (U32)_mm_movemask_ps(v_cmp_mask);

		return smallest( vxPOPCNT32(cmp_mask), MAX_MESH_LODS-1 );
	}

}//namespace



namespace
{
	void AddChildLeavesToVisibleObjectsList(
		const U32 parent_node_index
		, const SgAABBTreeInternal& bvh
		, const UINT lod
		, CulledObjectsIDs &culled_objects_ids_
		)
	{
		NodeStackT	node_stack;

		node_stack[0] = parent_node_index;
		UINT stack_top = 1;

		while( stack_top > 0 )
		{
			// Pop off the next node to work on.
			const U32		node_index = node_stack[ --stack_top ];
			const BVHNode	node = bvh.nodes._data[ node_index ];

			if( !node.IsLeaf() )
			{
				node_stack[ stack_top++ ] = node_index + 1;	// left
				node_stack[ stack_top++ ] = node.right_child_idx;	// right
			}
			else
			{
				for( U32 ii = 0; ii < node.count; ii++ )
				{
					const CulledObjectID obj_id = bvh.obj_ids._data[ node.start + ii ];

					culled_objects_ids_.AddFastUnsafe(
						obj_id | (lod << CULLED_OBJ_LOD_SHIFT)
						);
				}
			}
		}//while
	}
}//namespace



//TODO: cannot be jobified; use a linear octree/K-d tree?
ERet SgAABBTree::GatherObjectsInFrustum(
	const ViewFrustum& view_frustum
	, const Vector4& v_eye_position
	, CulledObjectsIDs &culled_objects_ids_
	) const
{
	const SimdFrustum	simd_frustum( view_frustum );

	//
	const SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

	//
	culled_objects_ids_.RemoveAll();
	mxDO(culled_objects_ids_.reserve(bvh.obj_ids._count));

	//
	NodeStackT	node_stack;

	// "Push" the root node.
	node_stack[0] = ROOT_NODE_INDEX;
	UINT stack_top = 1;

	while( stack_top > 0 )
	{
		// Pop off the next node to work on.
		const U32		node_index = node_stack[ --stack_top ];
		const BVHNode	node = bvh.nodes._data[ node_index ];

		//
#if USE_SSE_CULLING
		const SpatialRelation::Enum result = Test_AABB_against_ViewFrustum(
			(SimdAabb&)node
			, simd_frustum
			);
#else
		const AABBf node_aabb_f = {
			node.aabb_min,
			node.aabb_max,
		};
		const SpatialRelation::Enum result = view_frustum.Classify(node_aabb_f);
#endif

#if 0	// DEBUG

		TbPrimitiveBatcher& prim_batcher = SgGameApp::Get().renderer._render_system->_debug_renderer;

		if(result == SpatialRelation::Outside) {
			prim_batcher.DrawAABB( node.aabb_min, node.aabb_max, RGBAf::WHITE );
		} else if(result == SpatialRelation::Inside) {
			prim_batcher.DrawAABB( node.aabb_min, node.aabb_max, RGBAf::GREEN );
		} else {
			prim_batcher.DrawAABB( node.aabb_min, node.aabb_max, RGBAf::RED );
		}
#endif

		if( !node.IsLeaf() )
		{
			// this is an internal node

			if(result == SpatialRelation::Inside)
			{
				// all children are inside
				const UINT lod = CalculateLOD(
					(SimdAabb&)node
					, v_eye_position
					);

				AddChildLeavesToVisibleObjectsList(
					node_index
					, bvh
					, lod
					, culled_objects_ids_
					);
			}
			else if(result != SpatialRelation::Outside)
			{
				// inspect both children
				node_stack[ stack_top++ ] = node_index + 1;	// left
				node_stack[ stack_top++ ] = node.right_child_idx;	// right
			}
		}
		else if(result != SpatialRelation::Outside)
		{
			const UINT lod = CalculateLOD(
				(SimdAabb&)node
				, v_eye_position
				);

			// this leaf intersects or is fully inside the frustum
			for( U32 ii = 0; ii < node.count; ii++ )
			{
				const CulledObjectID obj_id = bvh.obj_ids._data[ node.start + ii ];

				culled_objects_ids_.AddFastUnsafe(
					obj_id | (lod << CULLED_OBJ_LOD_SHIFT)
					);
			}
		}
	}//while

	return ALL_OK;
}

ERet SgAABBTree::GatherObjectsInAABB(
	const SimdAabb& simd_aabb
	, CulledObjectsIDs &culled_objects_ids_
	) const
{
	culled_objects_ids_.RemoveAll();
	//
	const SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

	//
	NodeStackT	node_stack;

	node_stack[0] = ROOT_NODE_INDEX;
	UINT stack_top = 1;

	while( stack_top > 0 )
	{
		const U32		node_index = node_stack[ --stack_top ];
		const BVHNode	node = bvh.nodes._data[ node_index ];

		//
		if(SimdAabb_IntersectAABBs(simd_aabb, (SimdAabb&)node))
		{
			if( !node.IsLeaf() )
			{
				// inspect both children
				node_stack[ stack_top++ ] = node_index + 1;	// left
				node_stack[ stack_top++ ] = node.right_child_idx;	// right
			}
			else
			{
				// this leaf intersects or is fully inside the frustum
				for( U32 ii = 0; ii < node.count; ii++ )
				{
					const CulledObjectID obj_id = bvh.obj_ids._data[ node.start + ii ];
					mxDO(culled_objects_ids_.add(obj_id));
				}
			}
		}
	}//while

	return ALL_OK;
}

bool SgAABBTree::CheckIfPointInsideSomeAABB(
	const V3f& point
	, CulledObjectID &hit_obj_
	) const
{
	const SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

	//
	const Vector4	v_point = Vector4_LoadFloat3_Unaligned(point.a);

	//
	NodeStackT	node_stack;

	node_stack[0] = ROOT_NODE_INDEX;
	UINT stack_top = 1;

	while( stack_top > 0 )
	{
		const U32		node_index = node_stack[ --stack_top ];
		const BVHNode	node = bvh.nodes._data[ node_index ];

		if(SimdAabb_ContainsPoint( (SimdAabb&)node, v_point ))
		{
			if( !node.IsLeaf() )
			{
				// inspect both children
				node_stack[ stack_top++ ] = node_index + 1;	// left
				node_stack[ stack_top++ ] = node.right_child_idx;	// right
			}
			else
			{
				// this leaf intersects or is fully inside the frustum
				for( U32 ii = 0; ii < node.count; ii++ )
				{
					const CulledObjectID obj_id = bvh.obj_ids._data[ node.start + ii ];
					//mxDO(culled_objects_ids_.add(obj_id));
					hit_obj_ = obj_id;
					return true;
				}
			}
		}
	}//while

	return false;
}



namespace
{
	/// Node for storing state information during traversal.
	struct BVHTraversal
	{
		/// The index of the node to be traversed.
		U32	node_index;

		/// Minimum hit time for this node.
		float	mint;

	public:
		mxFORCEINLINE BVHTraversal()
		{}
		mxFORCEINLINE BVHTraversal(U32 _i, float _mint)
			: node_index(_i), mint(_mint)
		{ }
	};

	///
	bool SimdAabb_intersects_Ray(
		const SimdAabb& simd_aabb
		, const Vector4& ray_origin, const Vector4& ray_dir

		, const Vector4& inv_ray_dir
		, const Vector4& plus_inf, const Vector4& minus_inf

		, float &tnear_, float &tfar_
		)
	{
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

		// use a div if inverted directions aren't available
		const __m128 l1 = mulps(subps(simd_aabb.mins, ray_origin), inv_ray_dir);
		const __m128 l2 = mulps(subps(simd_aabb.maxs, ray_origin), inv_ray_dir);

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

		storess(lmin, &tnear_);
		storess(lmax, &tfar_);

		return  ret;
	}

}//namespace


bool SgAABBTree::CastRay(
	const V3f& ray_origin
	, const V3f& ray_direction
	, CulledObjectID &closest_hit_obj_
	, TempNodeID *hit_node_id_ /*= nil*/
	) const
{
	SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

	//
	const Vector4	v_ray_origin = Vector4_LoadFloat3_Unaligned(ray_origin.a);
	const Vector4	v_ray_direction = Vector4_LoadFloat3_Unaligned(ray_direction.a);

	// 
	const CV3f inv_ray_dir(
		(mmAbs(ray_direction.x) > 1e-4f) ? mmRcp(ray_direction.x) : BIG_NUMBER,
		(mmAbs(ray_direction.y) > 1e-4f) ? mmRcp(ray_direction.y) : BIG_NUMBER,
		(mmAbs(ray_direction.z) > 1e-4f) ? mmRcp(ray_direction.z) : BIG_NUMBER
	);

	const Vector4 v_inv_ray_dir = Vector4_LoadFloat3_Unaligned(inv_ray_dir.a);

	//
	const float flt_plus_inf = -logf(0);

	// you may already have those values hanging around somewhere
	const __m128
		plus_inf	= V4_REPLICATE(+flt_plus_inf),
		minus_inf	= V4_REPLICATE(-flt_plus_inf);

	//
	U32		num_objects_hit = 0;
	float	closest_hit_time = BIG_NUMBER;

	// Working set
	BVHTraversal todo[NODE_STACK_DEPTH];

	// "Push" on the root node to the working set
	todo[0].node_index = ROOT_NODE_INDEX;
	todo[0].mint = -BIG_NUMBER;

	//
	int iStackTop = 0;

	while( iStackTop >= 0 )
	{
		// Pop off the next node to work on.
		const U32 node_index = todo[iStackTop].node_index;
		const float tnear = todo[iStackTop].mint;
		iStackTop--;

		// If this node is further than the closest found intersection, continue
		if (tnear > closest_hit_time) {
			continue;
		}

		const BVHNode& node = bvh.nodes._data[ node_index ];

		if( node.IsLeaf() )
		{
			for( U32 ii = 0; ii < node.count; ii++ )
			{
				const CulledObjectID obj_id = bvh.obj_ids._data[ node.start + ii ];
				//mxDO(culled_objects_ids_.add(obj_id));
				closest_hit_obj_ = obj_id;
				if(hit_node_id_) {
					hit_node_id_->id = node_index;
				}
				return true;
			}

			num_objects_hit += node.count;
		}
		else
		{
			const U32 iChild0 = node_index + 1;	// the first (left) child is stored right after the node
			const U32 iChild1 = node.right_child_idx;
			const BVHNode& child0 = bvh.nodes._data[iChild0];
			const BVHNode& child1 = bvh.nodes._data[iChild1];

			float tnear0, tfar0;
			const bool hitc0 = SimdAabb_intersects_Ray(
				(SimdAabb&)child0
				, v_ray_origin, v_ray_direction, v_inv_ray_dir
				, plus_inf, minus_inf
				, tnear0, tfar0
				);

			float tnear1, tfar1;
			const bool hitc1 = SimdAabb_intersects_Ray(
				(SimdAabb&)child1
				, v_ray_origin, v_ray_direction, v_inv_ray_dir
				, plus_inf, minus_inf
				, tnear1, tfar1
				);

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
					TSwap( tfar0, tfar1 );
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

	return num_objects_hit > 0;
}

ERet SgAABBTree::GetNodeAABB(
	const TempNodeID node_id
	, SimdAabb &node_aabb_
	) const
{
	SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

	node_aabb_ = (SimdAabb&) bvh.nodes[ node_id.id ];

	return ALL_OK;
}


void SgAABBTree::DebugDraw(
						   TbPrimitiveBatcher& prim_batcher
						   )
{
	SgAABBTreeInternal& bvh = *(SgAABBTreeInternal*)this;

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

	NodeStackT	stack;
	// "Push" the root node.
	stack[0] = ROOT_NODE_INDEX;
	UINT stack_top = 1;

	while( stack_top > 0 )
	{
		// Pop off the next node to work on.
		const U32 node_index = stack[ --stack_top ];
		const BVHNode& node = bvh.nodes[ node_index ];

		const UINT treeDepth = stack_top;
		const int colorIndex = smallest( treeDepth, mxCOUNT_OF(s_NodeColors)-1 );
		prim_batcher.DrawAABB( node.aabb_min, node.aabb_max, RGBAf::fromRgba32(s_NodeColors[colorIndex]) );

		if( !node.IsLeaf() )
		{
			stack[ stack_top++ ] = node_index + 1;	// left
			stack[ stack_top++ ] = node.right_child_idx;	// right
		}
	}
}
