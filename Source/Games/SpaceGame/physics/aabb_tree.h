//
#pragma once

#include "common.h"
#include <Base/Math/BoundingVolumes/VectorAABB.h>

// all culled objects are spaceships for now
enum {
	CULLED_OBJ_INDEX_BITS = 26,
	CULLED_OBJ_LOD_BITS = 2,
	CULLED_OBJ_TYPE_BITS = 4,

	CULLED_OBJ_LOD_SHIFT = CULLED_OBJ_INDEX_BITS,
	CULLED_OBJ_TYPE_SHIFT = CULLED_OBJ_LOD_SHIFT + CULLED_OBJ_LOD_BITS,
};
const U32 CULLED_OBJ_INDEX_MASK = ((1<<CULLED_OBJ_INDEX_BITS)-1);
const U32 CULLED_OBJ_TYPE_MASK = ((1<<CULLED_OBJ_TYPE_BITS)-1);


typedef U32 CulledObjectID;
typedef DynamicArray<CulledObjectID>	CulledObjectsIDs;

const CulledObjectID INVALID_OBJ_ID = ~0;


typedef void FillCulledObjectIDsArray(
									  CulledObjectID *	culled_obj_ids_
									  , const U32 num_objs
									  , void* user_data0
									  , void* user_data1
									  );


/// cannot be stored!
mxDECLARE_32BIT_HANDLE(TempNodeID);



struct SgAABBTreeConfig
{
	U32	max_item_count;
	U32	max_items_per_leaf;

public:
	SgAABBTreeConfig()
	{
		max_item_count = 1024;
		max_items_per_leaf = 4;
	}
};

class SgAABBTree: NwNonCopyable
{
public:
	static ERet Create(
		SgAABBTree *&aabb_tree_
		, const SgAABBTreeConfig& cfg
		, AllocatorI& allocator
		);

	static void Destroy(
		SgAABBTree* p
		);

public:
	ERet Update(
		const TSpan< const SimdAabb >& aabbs
		, FillCulledObjectIDsArray* fill_obj_ids_fun
		, void* fill_obj_ids_fun_data0
		, void* fill_obj_ids_fun_data1
		/// must be able to allocate 'num_items' instances of vec4
		, AllocatorI & scratch_allocator
		);

	/// returns indices of culled primitives
	ERet GatherObjectsInFrustum(
		const ViewFrustum& frustum
		, const Vector4& eye_position
		, CulledObjectsIDs &culled_objects_ids_
		) const;

	ERet GatherObjectsInAABB(
		const SimdAabb& simd_aabb
		, CulledObjectsIDs &culled_objects_ids_
		) const;

	bool CheckIfPointInsideSomeAABB(
		const V3f& point
		, CulledObjectID &hit_obj_
		) const;

	bool CastRay(
		const V3f& ray_origin
		, const V3f& ray_direction
		, CulledObjectID &closest_hit_obj_
		, TempNodeID *hit_node_id_ = nil
		) const;

	ERet GetNodeAABB(
		const TempNodeID node_id
		, SimdAabb &node_aabb_
		) const;

	void DebugDraw(
		TbPrimitiveBatcher& prim_batcher
	);
};
