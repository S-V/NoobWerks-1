// Voxel Brick BVH <-> Host App Interop.
#ifdef __cplusplus
#pragma once
#endif

#ifndef SDF_BVH_INTEROP_H
#define SDF_BVH_INTEROP_H

#include <Shared/__shader_interop_pre.h>


/*

From MSB to LSB:

----------------------------
Solid Leaf Flags Layout:
u0.1  : = 1 (leaf flag)
u0.1  : = 1 (homogeneous flag)
u0.30 : unused

u1.5  : lod level
u1.27 : resolution in voxels (voxel size is scaled by (1 << lod))

----------------------------
Heterogeneous Leaf Flags Layout:
u0.1  : = 1 (leaf flag)
u0.1  : = 0 (homogeneous flag)
u0.30 : brick offset within atlas, in texels

u1.5  : lod level
u1.27 : resolution in voxels (voxel size is scaled by (1 << lod))

----------------------------
Internal Node Flags Layout:
u0.1  : = 0 (leaf flag)
u0.31 : second child index (the first child is located right after the node)

u1    : unused (yet)
*/

struct SdfBvhNode
{
    float3	aabb_min;


	/// From MSB to LSB:
	///=== Internal node layout:
	///   32: always zero
	///
	///=== Solid leaf layout:
	///    1: 1 (leaf flag)
	///    1: 0 (heterogeneous flag)
	///   30: unused
	///
	///=== Heterogeneous leaf layout:
	///   1 : 1 (leaf flag)
	///   1 : 1 (heterogeneous flag)
	///   30: brick offset within 3D atlas (including brick apron/border padding)
	uint	leaf_flags__or__brick_offset;


	///
    float3	aabb_max;


	/// From MSB to LSB:
	///=== Internal node layout:
	///    2: split axis (0, 1 or 2).
	///   30: index of the second (right) child (the first (left) child follows the node).
	///
	///=== Solid leaf layout:
	///   32: unused (but can store material id / average water density / etc.)
	///
	///=== Heterogeneous leaf layout:
	///    2: unused, always zero
	///   30: the voxel brick's inner resolution (i.e. including apron/border padding)
	uint	right_child_idx__or__brick_inner_dim;



	// Internal node methods:

	bool IsInternal() CONST_METHOD
	{
		return leaf_flags__or__brick_offset == 0;
	}

	uint GetSplitAxis() CONST_METHOD
	{
		return right_child_idx__or__brick_inner_dim >> 30;
	}

	uint GetRightChildIndex() CONST_METHOD
	{
		return right_child_idx__or__brick_inner_dim & ( (1<<30) - 1 );
	}


	// Leaf methods:

	bool IsSolidLeaf() CONST_METHOD
	{
		return leaf_flags__or__brick_offset == 0x80000000;
	}

	uint3 GetBrickOffsetWithinBrickAtlas() CONST_METHOD
	{
		const uint brick_offset30
			= leaf_flags__or__brick_offset & ( (1<<30) - 1 )
			;
		return uint3(
			brick_offset30 & 1023,
			(brick_offset30 >> 10) & 1023,
			(brick_offset30 >> 20) & 1023
		);
	}

	uint3 GetBrickInnerResolution() CONST_METHOD
	{
		return uint3(
			right_child_idx__or__brick_inner_dim & 1023,
			(right_child_idx__or__brick_inner_dim >> 10) & 1023,
			(right_child_idx__or__brick_inner_dim >> 20) & 1023
		);
	}
};


#define BVH_MAX_NODES		(1024)

// if node size is 32 bytes, BVH_MAX_NODES = 64 KiB / 32 = 2048 nodes
//#define BVH_MAX_NODES		(nwMAX_CB_SIZE/SIZE_OF_BVH_NODE)

struct SdfBvhData
{
	uint		num_nodes;
	//float		sdf_voxel_size_at_LoD0_WS;

	/// for transforming from world into volume texture space
	//float		inverse_sdf_brick_size_at_LoD0_WS;
	
	float		inverse_sdf_atlas_resolution;	// = 1.0f / 3d_atlas.size_1d (in texels)
	//float		max_ray_distance;
	uint		pad1;
	uint		pad2;
	
	float4		pad32;

	//TODO: move these into another CBuffer or Buffer or smth
    SdfBvhNode	nodes[ BVH_MAX_NODES ];
};
DECLARE_CBUFFER( G_SDF_BVH_CBuffer, 4 )
{
	SdfBvhData	g_sdf_bvh;
};


#include <Shared/__shader_interop_post.h>

#endif // SDF_BVH_INTEROP_H
