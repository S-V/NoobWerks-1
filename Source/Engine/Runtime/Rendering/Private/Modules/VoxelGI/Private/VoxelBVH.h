// BVH with voxel bricks at leaf nodes.
// Used for raytracing on GPU (for updating DDGI probes).
#pragma once

#include <Base/Template/NwHandleTable.h>

#include <Rendering/ForwardDecls.h>
#include <Rendering/Private/Modules/VoxelGI/vxgi_brickmap.h>


/// Should the BVH included fully solid leaves?
/// For now disabled to minimize branching in shaders
#define VOXEL_BVH_CFG__CREATE_SOLID_LEAFS	(0)


struct SdfBvhNode;


namespace Rendering
{
namespace VXGI
{

class BrickMap;

mxDECLARE_32BIT_HANDLE(BvhLeafNodeHandle);

//struct BvhNode
//{
//	Tuple3< U16 >	offset_in_brick_atlas_texture;
//	U16				unused;
//};
mxPREALIGN(16)
struct BvhNode
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
mxSTATIC_ASSERT(sizeof(BvhNode) == 32);



///
class VoxelBVH: NonCopyable
{
public:
	HBuffer		_bvh_cb_handle;
	bool		_bvh_on_gpu_is_dirty;

	///
	struct BvhLeafItem
	{
		// {center; radius} representation is more convenient for building the BVH
		AabbCEf		bbox;	// 24
		BrickHandle	brick_handle;	// NIL if the leaf is solid
		BvhLeafNodeHandle	my_handle;

	public:
		bool IsFullySolid() const { return brick_handle.IsNil(); }
	};
	ASSERT_SIZEOF(BvhLeafItem, 32);

	BvhLeafItem  *	_leaf_items;
	NwHandleTable	_leaf_item_handle_tbl;

	SdfBvhNode  *	_gpu_bvh_nodes;
	U32				_num_gpu_bvh_nodes;

public:
	VoxelBVH(AllocatorI& allocator);

	struct Config {
		U16	max_leaf_node_count;
	public:
		Config() {
			max_leaf_node_count = 1024;
		}
	};
	ERet Initialize(const Config& cfg);
	void Shutdown();


	ERet UpdateGpuBvhIfNeeded(
		const NwCameraView& scene_view
		, const BrickMap& brickmap
		, NGpu::NwRenderContext & render_context
		);


	BvhLeafItem& GetLeafNodeByHandle(
		const BvhLeafNodeHandle node_handle
		);


	ERet AllocSolidLeaf(
		BvhLeafNodeHandle &new_node_handle_
		, const AABBd& aabb_WS
		);
	ERet AllocHeterogeneousLeaf(
		BvhLeafNodeHandle &new_node_handle_
		, const AABBd& aabb_WS
		, const void* sdf_data
		, const U32 sdf_grid_dim_1D
		, BrickMap& brickmap
		);
	ERet DeleteLeaf(
		const BvhLeafNodeHandle old_node_handle
		, BrickMap& brickmap
		);

	//ERet UpdateBrick(
	//	const BvhLeafNodeHandle brick_handle
	//	, const SDFValue* sdf_values
	//	, const U32 sdf_grid_dim_1D
	//	);

	void DebugDrawLeafNodes(
		const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		);

	bool DebugIntersectGpuBvhAabbsOnCpu(
		V3f &hit_pos_
		, const NwCameraView& scene_view
		);

	void DebugDrawBVH(
		const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		, const U32 max_depth = ~0
		);

	ERet DebugRayTraceBVH(
		const NwCameraView& scene_view
		, const HShaderOutput h_dst_tex2d	// screen buffer
		, const UInt2& dst_tex2d_size
		, const BrickMap& brickmap
		, NGpu::NwRenderContext & render_context
		);

private:
	ERet _BuildBvhForGpu(
		const BrickMap& brickmap
		, AllocatorI & scratch_allocator
		);

	ERet _UpdateGpuBvh(
		const NwCameraView& scene_view
		, const BrickMap& brickmap
		, NGpu::NwRenderContext & render_context
		);
};

}//namespace VXGI
}//namespace Rendering
