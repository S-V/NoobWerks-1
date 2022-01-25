#include <Base/Base.h>
#pragma hdrstop

#include <Core/Tasking/TaskSchedulerInterface.h>

#include <GPU/Public/graphics_types.h>
#include <GPU/Public/graphics_system.h>

#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Rendering/Private/Modules/VoxelGI/vxgi_brickmap.h>
#include <Rendering/Public/Globals.h>	// RenderPath
#include <Rendering/Public/Scene/CameraView.h>	// NwCameraView
#include <Rendering/Private/Shaders/HLSL/VoxelGI/bvh_interop.h>


namespace Rendering
{
namespace VXGI
{

namespace
{
/// Each brick is surrounded by 1 voxel thick border
/// so that we can use hardware trilinear filtering
/// for sampling the brick's interior voxels.
static const int SDF_BRICK_MARGIN_VOXELS = 1;	// aka 'apron' in GVDB

}//namespace


VoxelBVH::VoxelBVH(AllocatorI& allocator)
	: _leaf_items(nil)
	, _leaf_item_handle_tbl(allocator)
{
}

ERet VoxelBVH::Initialize(const Config& cfg)
{
	//
	_bvh_cb_handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_SDF_BVH_CBuffer);

	mxDO(nwAllocArray(
		_leaf_items
		, cfg.max_leaf_node_count
		, _leaf_item_handle_tbl._allocator
		));

	mxDO(_leaf_item_handle_tbl.InitWithMaxCount(
		cfg.max_leaf_node_count
		));

	//
	const U32 max_num_bvh_nodes = cfg.max_leaf_node_count*2;
	mxDO(nwAllocArray(
		_gpu_bvh_nodes
		, max_num_bvh_nodes
		, _leaf_item_handle_tbl._allocator
		));

	_bvh_on_gpu_is_dirty = false;

	_num_gpu_bvh_nodes = 0;

	return ALL_OK;
}

void VoxelBVH::Shutdown()
{
	mxDELETE_AND_NIL(
		_gpu_bvh_nodes
		, _leaf_item_handle_tbl._allocator
		);

	_leaf_item_handle_tbl.Shutdown();

	mxDELETE_AND_NIL(
		_leaf_items
		, _leaf_item_handle_tbl._allocator
		);

	NGraphics::DestroyGlobalConstantBuffer(_bvh_cb_handle);
}

ERet VoxelBVH::UpdateGpuBvhIfNeeded(
	const NwCameraView& scene_view
	, const BrickMap& brickmap
	, NGpu::NwRenderContext & render_context
	)
{
	if(_bvh_on_gpu_is_dirty)
	{
		mxDO(_UpdateGpuBvh(
			scene_view
			, brickmap
			, render_context
			));
		_bvh_on_gpu_is_dirty = false;
	}//if dirty



	// For directional lights.
	{
		const NGpu::LayerID global_light_pass_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DeferredGlobalLights )
			;

		NGpu::SetLayerConstantBuffer(
			global_light_pass_index,
			_bvh_cb_handle,
			G_SDF_BVH_CBuffer_Index
			);
	}


	return ALL_OK;
}

VoxelBVH::BvhLeafItem& VoxelBVH::GetLeafNodeByHandle(
	const BvhLeafNodeHandle node_handle
	)
{
	const U32 node_index = _leaf_item_handle_tbl.GetItemIndexByHandle(node_handle.id);
	return _leaf_items[ node_index ];
}

namespace
{
ERet AllocBvhLeafNode(
	BvhLeafNodeHandle &new_node_handle_
	, VoxelBVH& bvh
	, const AABBd& aabb_WS
	, const BrickHandle brick_handle
	)
{
	const UINT new_leaf_node_index = bvh._leaf_item_handle_tbl._num_alive_items;

	mxASSERT(new_leaf_node_index < BVH_MAX_NODES);

	BvhLeafNodeHandle	new_node_handle;
	mxDO(bvh._leaf_item_handle_tbl.AllocHandle(
		new_node_handle.id
		));
	new_node_handle_ = new_node_handle;

	//
	VoxelBVH::BvhLeafItem& new_leaf_node = bvh._leaf_items[ new_leaf_node_index ];

	new_leaf_node.bbox.center = V3f::fromXYZ(aabb_WS.center());
	new_leaf_node.bbox.extent = V3f::fromXYZ(aabb_WS.halfSize());

	new_leaf_node.my_handle = new_node_handle;
	new_leaf_node.brick_handle = brick_handle;

	return ALL_OK;
}

}//namespace

ERet VoxelBVH::AllocSolidLeaf(
	BvhLeafNodeHandle &new_node_handle_
	, const AABBd& aabb_WS
	)
{
	AllocBvhLeafNode(
		new_node_handle_
		, *this
		, aabb_WS
		, BrickHandle::MakeNilHandle()
		);

	_bvh_on_gpu_is_dirty = true;

	return ALL_OK;
}

ERet VoxelBVH::AllocHeterogeneousLeaf(
	BvhLeafNodeHandle &new_node_handle_
	, const AABBd& aabb_WS
	, const void* sdf_data
	, const U32 sdf_grid_dim_1D
	, BrickMap& brickmap
	)
{
	const SDFValue* src_sdf_values_f = (SDFValue*) sdf_data;

	BrickHandle	new_brick_handle;
	mxDO(brickmap.AllocBrick(
		new_brick_handle
		, src_sdf_values_f
		, sdf_grid_dim_1D
		));
	//
	mxDO(AllocBvhLeafNode(
		new_node_handle_
		, *this
		, aabb_WS
		, new_brick_handle
		));

	_bvh_on_gpu_is_dirty = true;

	return ALL_OK;
}

ERet VoxelBVH::DeleteLeaf(
	const BvhLeafNodeHandle old_node_handle
	, BrickMap& brickmap
	)
{
	BvhLeafItem& old_node = GetLeafNodeByHandle(old_node_handle);
	if(old_node.brick_handle.IsValid()) {
		brickmap.FreeBrick(old_node.brick_handle);
	}

	//
	U32	indices_of_swapped_items[2];
	mxDO(_leaf_item_handle_tbl.DestroyHandle(
		old_node_handle.id
		, _leaf_items
		, indices_of_swapped_items
		));

	_bvh_on_gpu_is_dirty = true;

	return ALL_OK;
}

namespace
{
void SetupGpuBvhLeafNode(
						 SdfBvhNode &gpu_bvh_node_
						 , const VoxelBVH::BvhLeafItem& bvh_leaf_item
						 , const BrickMap& brickmap
						 )
{
	gpu_bvh_node_.aabb_min = bvh_leaf_item.bbox.minCorner();
	gpu_bvh_node_.aabb_max = bvh_leaf_item.bbox.maxCorner();

	if(bvh_leaf_item.IsFullySolid())
	{
		gpu_bvh_node_.leaf_flags__or__brick_offset = 0x80000000;
		gpu_bvh_node_.right_child_idx__or__brick_inner_dim = ~0;
	}
	else
	{
		mxASSERT(bvh_leaf_item.brick_handle.IsValid());

		const BrickMap::VoxelBrick& brick = brickmap.GetBrickByHandle(bvh_leaf_item.brick_handle);

		gpu_bvh_node_.leaf_flags__or__brick_offset
			= 0xC0000000
			| (brick.offset_in_atlas_tex.z << 20)
			| (brick.offset_in_atlas_tex.y << 10)
			| (brick.offset_in_atlas_tex.x)
			;

		//
		const UINT brick_inner_dim = brick.dim_1D - SDF_BRICK_MARGIN_VOXELS * 2;

		gpu_bvh_node_.right_child_idx__or__brick_inner_dim
			= (brick_inner_dim)
			| (brick_inner_dim << 10)
			| (brick_inner_dim << 20)
			;
#if MX_DEBUG
		//
		const UInt3 brick_offset_uint3 = gpu_bvh_node_.GetBrickOffsetWithinBrickAtlas();
		const UInt3 brick_inner_dim_uint3 = gpu_bvh_node_.GetBrickInnerResolution();
		mxASSERT(brick_offset_uint3 == UInt3::fromXYZ(brick.offset_in_atlas_tex));
		mxASSERT(brick_inner_dim_uint3 == UInt3(brick_inner_dim));
#endif
	}
}

//
enum {
	ROOT_NODE_INDEX = 0,
	NODE_STACK_DEPTH = 64
};

struct BVHBuildEntry
{
	/// If non-zero then this is the index of the parent (used in offsets).
	U32 parent;
	/// The range of objects in the object list covered by this node.
	U32 start, end;
};

}//namespace

mxSTOLEN("Heavily based on https://github.com/brandonpelfrey/Fast-BVH");
/*! Build the BVH, given an input data set
 *  - Handling our own stack is quite a bit faster than the recursive style.
 *  - Each build stack entry's parent field eventually stores the offset
 *    to the parent of that node. Before that is finally computed, it will
 *    equal exactly three other values. (These are the magic values Untouched,
 *    Untouched-1, and TouchedTwice).
 *  - The partition here was also slightly faster than std::partition.
 */

ERet VoxelBVH::_BuildBvhForGpu(
							   const BrickMap& brickmap
							   , AllocatorI & scratch_allocator
	)
{
	const U32 num_items = _leaf_item_handle_tbl._num_alive_items;
	if(!num_items) {
		_num_gpu_bvh_nodes = 0;
		return ALL_OK;
	}

	const U32 max_num_bvh_nodes = num_items * 2;

	//
	U32 *	item_indices;
	mxTRY_ALLOC_SCOPED( item_indices, num_items, scratch_allocator );

	for(U32 item_index = 0; item_index < num_items; item_index++)
	{
		item_indices[ item_index ] = item_index;
		////
		//const BvhLeafItem aabb = _leaf_items[ item_index ];
		//src_aabbs_centers[ item_index ] = V4_MUL(
		//	V4_ADD(aabb.mins, aabb.maxs)
		//	, v_half
		//	);
	}

	//
	U32	num_written_bvh_nodes = 0;

	//
	BVHBuildEntry todo[ NODE_STACK_DEPTH ];
	//memset(todo, 0xCCCCCCCC, sizeof(todo));

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

	// In-place quick sort-like construction algorithm with global heuristic
	while( stack_top > 0 )
	{
		// Pop the next item off of the stack (copy, because the stack may change).
		const BVHBuildEntry build_entry = todo[ --stack_top ];
		const U32 start = build_entry.start;
		const U32 end = build_entry.end;

		// Build a bounding box from the primitives' centroids
		const BvhLeafItem&	start_item = _leaf_items[ item_indices[ start ] ];
		AABBf centers_aabb = { start_item.bbox.center, start_item.bbox.center };

		//
		SdfBvhNode new_node;
		
		const U32 new_node_index = num_written_bvh_nodes;
		mxASSERT(new_node_index < max_num_bvh_nodes);

		const U32 primitive_count = end - start;

		//
		if( primitive_count == 1 )
		{
			// This is a leaf (terminal node).
			SetupGpuBvhLeafNode(
				new_node
				, start_item
				, brickmap
				);
		}
		else
		{
			// This is an internal node.
			mxASSERT(primitive_count > 1);

			// Calculate the bounding box for this node
			{
				// bounding box from the primitives' boxes
				AABBf node_aabb = start_item.bbox.ToAabbMinMax();
				for( U32 j = start + 1; j < end; j++ )
				{
					const U32 item_index = item_indices[ j ];
					const BvhLeafItem& item = _leaf_items[ item_index ];
					centers_aabb.addPoint( item.bbox.center );
					node_aabb.ExpandToIncludeAABB( item.bbox.ToAabbMinMax() );
				}
				new_node.aabb_min = node_aabb.min_corner;
				new_node.aabb_max = node_aabb.max_corner;
			}

			// Partition the list of objects into two halves.
			U32 mid = start;
			const EAxis split_axis = centers_aabb.LongestAxis();
			{
				// Split on the center of the longest axis
				const float split_coord = centers_aabb.CenterAlongAxis( split_axis );
				// Partition the list of objects on this split
				for( U32 i = start; i < end; i++ )
				{
					const U32 item_index = item_indices[ i ];
					const V3f& item_aabb_center = _leaf_items[ item_index ].bbox.center;
					if( item_aabb_center.a[ split_axis ] < split_coord )
					{
						TSwap( item_indices[i], item_indices[mid] );
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
			todo[stack_top].parent = new_node_index;
			stack_top++;

			// Then push the left child.
			todo[stack_top].start = start;
			todo[stack_top].end = mid;
			todo[stack_top].parent = new_node_index;
			stack_top++;

			// temporarily store split axis here (it will be overwritten with zero)
			new_node.leaf_flags__or__brick_offset = split_axis;
			new_node.right_child_idx__or__brick_inner_dim = Untouched;	// children will decrement this field later during building
		}//if this is an internal node.

		// Child touches parent...
		// Special case: Don't do this for the root.
		const U32 parent_index = build_entry.parent;
		if( parent_index != RootNodeID )
		{
			SdfBvhNode & parent_node = _gpu_bvh_nodes[ parent_index ];

			parent_node.right_child_idx__or__brick_inner_dim--;

			// When this is the second touch, this is the right child.
			// The right child sets up the offset for the flat tree.
			if( parent_node.right_child_idx__or__brick_inner_dim == TouchedTwice )
			{
				const EAxis split_axis = (EAxis) parent_node.leaf_flags__or__brick_offset;
				parent_node.right_child_idx__or__brick_inner_dim = (split_axis << 30) | new_node_index;
				parent_node.leaf_flags__or__brick_offset = 0;
			}
		}//If parent is not root.

		_gpu_bvh_nodes[ num_written_bvh_nodes++ ] = new_node;

	}//While build stack is not empty.

	//
	mxASSERT(num_written_bvh_nodes <= max_num_bvh_nodes);

	_num_gpu_bvh_nodes = num_written_bvh_nodes;

	return ALL_OK;
}

ERet VoxelBVH::_UpdateGpuBvh(
							 const NwCameraView& scene_view
							 , const BrickMap& brickmap
							 , NGpu::NwRenderContext & render_context
							 )
{
	mxDO(_BuildBvhForGpu(
		brickmap
		, threadLocalHeap()
		));

	//
	SdfBvhData	*	gpu_bvh;
	mxDO(NGpu::updateGlobalBuffer(
		_bvh_cb_handle
		, &gpu_bvh
		));

	//
	const U32 num_gpu_bvh_nodes = _num_gpu_bvh_nodes;

	//
	gpu_bvh->num_nodes = num_gpu_bvh_nodes;
	//gpu_bvh->sdf_voxel_size_at_LoD0_WS = 1.0f;
	//gpu_bvh->max_ray_distance = 100.0f;
	gpu_bvh->inverse_sdf_atlas_resolution = brickmap.inverse_sdf_brick_atlas_resolution;

	//
	for(UINT i = 0; i < num_gpu_bvh_nodes; i++)
	{
		gpu_bvh->nodes[i] = _gpu_bvh_nodes[i];
	}//for each node

	return ALL_OK;
}

void VoxelBVH::DebugDrawLeafNodes(
	const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	)
{
	TbPrimitiveBatcher& dbg_drawer = Globals::GetImmediateDebugRenderer();

	if(0)
	{
		for(UINT i = 0; i < _leaf_item_handle_tbl._num_alive_items; i++)
		{
			const BvhLeafItem& bvh_leaf_item = _leaf_items[i];

			dbg_drawer.DrawAABB(
				bvh_leaf_item.bbox.ToAabbMinMax(),
				bvh_leaf_item.IsFullySolid() ? RGBAf::GREEN : RGBAf::RED
				);
		}
	}
	else
	{
		for(UINT i = 0; i < _num_gpu_bvh_nodes; i++)
		{
			const SdfBvhNode& bvh_node = _gpu_bvh_nodes[i];

			if(bvh_node.IsInternal()) {
				continue;
			}

			dbg_drawer.DrawAABB(
				bvh_node.aabb_min, bvh_node.aabb_max,
				bvh_node.IsSolidLeaf() ? RGBAf::GREEN : RGBAf::RED
				);
		}
	}
}

namespace
{
	inline V3f GetInverseRayDirectionSafe(const V3f& raydir)
	{
		// Fix ray directions that are too close to 0.
		const float ooeps = SMALL_NUMBER; // to avoid div by zero
		const V3f dir_abs = V3f::abs(raydir);

		V3f invdir;
		invdir.x = 1.0f / ((dir_abs.x > ooeps) ? raydir.x : mmCopySign(ooeps, raydir.x));
		invdir.y = 1.0f / ((dir_abs.y > ooeps) ? raydir.y : mmCopySign(ooeps, raydir.y));
		invdir.z = 1.0f / ((dir_abs.z > ooeps) ? raydir.z : mmCopySign(ooeps, raydir.z));
		return invdir;
	}

	struct PrecomputedRay
	{
		V3f	origin;

		V3f	direction;
		float	max_t;	// max_ray_distance

		// precomputed values for faster intersection testing:

		V3f	invdir;	// 1 / dir

		/// for faster ray-aabb intersection testing
		V3f	neg_org_x_invdir;	// -(orig / dir)


		void SetFromRay(
			const V3f& ray_origin
			, const V3f& ray_direction
			)
		{
			origin = ray_origin;
			direction = ray_direction;
			max_t = 1e10;
			//
			invdir = GetInverseRayDirectionSafe(ray_direction);
			neg_org_x_invdir = -V3f::mul( ray_origin, invdir );
		}
	};

	inline V2f IntersectRayAgainstAABB(
		const PrecomputedRay& precomp_ray
		, const V3f& aabb_min
		, const V3f& aabb_max
		)
	{
		// V3f n = (aabb_min - ray_origin) / ray_direction;
		// V3f f = (aabb_max - ray_origin) / ray_direction;
		const V3f n = V3f::mul(aabb_min, precomp_ray.invdir) + precomp_ray.neg_org_x_invdir;
		const V3f f = V3f::mul(aabb_max, precomp_ray.invdir) + precomp_ray.neg_org_x_invdir;

		const V3f tmin = V3f::min(f, n);
		const V3f tmax = V3f::max(f, n);

		const float t0 = maxf(tmin.maxComponent(), 0.f);	// don't allow intersections 'behind' the ray
		const float t1 = minf(tmax.minComponent(), precomp_ray.max_t);

		return V2f(t0, t1);
	}

	inline bool RayIntersectsAABB(
		const PrecomputedRay& precomp_ray
		, const V3f& aabb_min
		, const V3f& aabb_max
		, float &_tnear, float &tfar_
		)
	{
		// V3f n = (aabb_min - ray_origin) / ray_direction;
		// V3f f = (aabb_max - ray_origin) / ray_direction;
		const V3f n = V3f::mul(aabb_min, precomp_ray.invdir) + precomp_ray.neg_org_x_invdir;
		const V3f f = V3f::mul(aabb_max, precomp_ray.invdir) + precomp_ray.neg_org_x_invdir;

		const V3f tmin = V3f::min(f, n);
		const V3f tmax = V3f::max(f, n);

		const float t0 = maxf(tmin.maxComponent(), 0.f);	// don't allow intersections 'behind' the ray
		const float t1 = minf(tmax.minComponent(), precomp_ray.max_t);

		_tnear = t0;
		tfar_ = t1;

		return t0 <= t1;
	}
}//namespace

bool VoxelBVH::DebugIntersectGpuBvhAabbsOnCpu(
	V3f &hit_pos_
	, const NwCameraView& scene_view
	)
{
	PrecomputedRay	precomp_ray;
	precomp_ray.SetFromRay(
		V3f::fromXYZ(scene_view.eye_pos_in_world_space)
		, scene_view.look_dir
		);

	//
	float closest_dist_until_hit_WS = BIG_NUMBER;

	if(0)
	{
		for(UINT i = 0; i < _num_gpu_bvh_nodes; i++)
		{
			const SdfBvhNode& bvh_node = _gpu_bvh_nodes[i];

			if(bvh_node.IsInternal()) {
				continue;
			}

			const V2f ray_node_aabb_hit_times =
				IntersectRayAgainstAABB
				//IntersectRayAgainstAABB_Simple
				(// (//
				precomp_ray
				, bvh_node.aabb_min
				, bvh_node.aabb_max
				);

			if(ray_node_aabb_hit_times.x <= ray_node_aabb_hit_times.y)
			{
				if(ray_node_aabb_hit_times.x < closest_dist_until_hit_WS)
				{
					closest_dist_until_hit_WS = ray_node_aabb_hit_times.x;
				}
			}
		}
	}
	else if(1)
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
			if (tnear > closest_dist_until_hit_WS) {
				continue;
			}

			const SdfBvhNode& node = _gpu_bvh_nodes[ node_index ];

			if( node.IsInternal() )
			{
				const U32 iChild0 = node_index + 1;	// the first (left) child is stored right after the node
				const U32 iChild1 = node.right_child_idx__or__brick_inner_dim;
				const SdfBvhNode& child0 = _gpu_bvh_nodes[iChild0];
				const SdfBvhNode& child1 = _gpu_bvh_nodes[iChild1];

				float tnear0, tfar0;
				const bool hitc0 = RayIntersectsAABB(
					precomp_ray
					, child0.aabb_min, child0.aabb_max
					, tnear0, tfar0
					);

				float tnear1, tfar1;
				const bool hitc1 = RayIntersectsAABB(
					precomp_ray
					, child1.aabb_min, child1.aabb_max
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
			else
			{
				// Leaf:
				float tnear, tfar;
				const bool hitc0 = RayIntersectsAABB(
					precomp_ray
					, node.aabb_min, node.aabb_max
					, tnear, tfar
					);
				if(tnear < closest_dist_until_hit_WS)
				{
					closest_dist_until_hit_WS = tnear;
				}
			}
		}//while
	}
	else if(0)
	{
		// Allocate traversal stack from thread-local memory.
		// (the stack is just a fixed size array of indices to BVH nodes)
		UINT traversal_stack[32];

		int stack_top = 0;	// == the number of nodes pushed onto the stack
		traversal_stack[ stack_top++ ] = 0;

		while(stack_top > 0)
		{
			const UINT curr_node_idx = traversal_stack[ --stack_top ];
			const SdfBvhNode& bvh_node = _gpu_bvh_nodes[ curr_node_idx ];

			const V2f ray_node_aabb_hit_times =
				IntersectRayAgainstAABB
				//IntersectRayAgainstAABB_Simple
				(// (//
				precomp_ray
				, bvh_node.aabb_min
				, bvh_node.aabb_max
				);

			if(ray_node_aabb_hit_times.x <= ray_node_aabb_hit_times.y)
			{
				// If this is an inner node:
				if(bvh_node.IsInternal())
				{
					const UINT child0_idx = curr_node_idx + 1;
					const UINT child1_idx = bvh_node.GetRightChildIndex();

					traversal_stack[ stack_top++ ] = child0_idx;
					traversal_stack[ stack_top++ ] = child1_idx;
				}
				else
				{
					// This is a leaf node.
					/*
					if(bvh_node.IsSolidLeaf())
					{
					closest_hit_result.did_hit_solid_leaf = true;
					break;
					}
					*/
					if(ray_node_aabb_hit_times.x < closest_dist_until_hit_WS)
					{
						closest_dist_until_hit_WS = ray_node_aabb_hit_times.x;
					}
				}//If hit a leaf.
			}//If the ray intersects the node's bounding box.
		}//While not stack is not empty.
	}

	hit_pos_ = precomp_ray.origin + precomp_ray.direction * closest_dist_until_hit_WS;

	return closest_dist_until_hit_WS < BIG_NUMBER;
}

void VoxelBVH::DebugDrawBVH(
	const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const U32 max_depth
	)
{
	if(!_num_gpu_bvh_nodes) {
		return;
	}

	TbPrimitiveBatcher& dbg_drawer = Globals::GetImmediateDebugRenderer();

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

	U32	stack[ NODE_STACK_DEPTH ];
	// "Push" the root node.
	stack[0] = ROOT_NODE_INDEX;
	UINT stack_top = 1;

	float	aabb_scale = 1.0f;

	while( stack_top > 0 )
	{
		// Pop off the next node to work on.
		const U32 node_index = stack[ --stack_top ];

		SdfBvhNode& node = _gpu_bvh_nodes[ node_index ];

		AABBf aabb = { node.aabb_min, node.aabb_max };
		aabb = aabb.expanded(CV3f(aabb_scale));
		aabb_scale *= 0.98f;

		if( node.IsInternal() )
		{
			const UINT treeDepth = stack_top;
			const int colorIndex = smallest( treeDepth, mxCOUNT_OF(s_NodeColors)-1 );
			dbg_drawer.DrawAABB( aabb, RGBAf::fromRgba32(s_NodeColors[colorIndex]) );

			if(stack_top < max_depth)
			{
				stack[ stack_top++ ] = node_index + 1;	// left
				stack[ stack_top++ ] = node.right_child_idx__or__brick_inner_dim;	// right
			}
		}
		else
		{
			if(node.IsSolidLeaf()) {
				dbg_drawer.DrawAABB( aabb, RGBAf::GREEN );
			} else {
				dbg_drawer.DrawAABB( aabb, RGBAf::RED );
			}
		}
	}
}

ERet VoxelBVH::DebugRayTraceBVH(
	const NwCameraView& scene_view
	, const HShaderOutput h_dst_tex2d
	, const UInt2& dst_tex2d_size
	, const BrickMap& brickmap
	, NGpu::NwRenderContext & render_context
	)
{
	//
	const U32 dst_view_id = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::DeferredCompositeLit )
		;
	//
	NwShaderEffect* dbg_tech_viz_bvh;
	mxDO(Resources::Load(dbg_tech_viz_bvh, MakeAssetID("vxgi_dbg_viz_bvh")));

	//
	NGpu::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		NGpu::buildSortKey(dst_view_id, 2)
			nwDBG_CMD_SRCFILE_STRING
		);

	/*mxDO*/(dbg_tech_viz_bvh->SetInput(
		nwNAMEHASH("t_sdf_atlas3d"),
		NGpu::AsInput(brickmap.h_sdf_brick_atlas_tex3d)
		));
	mxDO(dbg_tech_viz_bvh->setUav(
		mxHASH_STR("t_output_rt"),
		h_dst_tex2d
		));

	//
	mxDO(dbg_tech_viz_bvh->pushShaderParameters( render_context._command_buffer ));

	//
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	cmd_writer.SetCBuffer(G_SDF_BVH_CBuffer_Index, _bvh_cb_handle);

	//
	enum { THREAD_GROUP_SIZE = 8 };

	cmd_writer.DispatchCS(
		dbg_tech_viz_bvh->getDefaultVariant().program_handle
		, AlignUp(dst_tex2d_size.x, THREAD_GROUP_SIZE) / THREAD_GROUP_SIZE
		, AlignUp(dst_tex2d_size.y, THREAD_GROUP_SIZE) / THREAD_GROUP_SIZE
		, 1
		);
	return ALL_OK;
}

}//namespace VXGI
}//namespace Rendering
