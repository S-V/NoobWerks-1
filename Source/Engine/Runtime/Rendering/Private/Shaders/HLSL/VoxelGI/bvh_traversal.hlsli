// Ray Traversal of BVH with Voxel 'Bricks' in leaf nodes.
#ifndef VOXEL_BVH_TRAVERSE_HLSLI
#define VOXEL_BVH_TRAVERSE_HLSLI

#include <base.hlsli>	// nwMAX_CB_SIZE


/// BVH_USER_NODE must have 'aabb_min' and 'aabb_max' of type 'float3'
#ifndef BVH_USER_NODE
#define BVH_USER_NODE struct SdfBvhNode
#endif

#ifndef BVH_USER_DECLARE_STATE
#define BVH_USER_DECLARE_STATE
#endif

#ifndef BVH_USER_INTERSECT_LEAF
#define BVH_USER_INTERSECT_LEAF(node, node_index, trace_state)
#endif




/// The depth of the traversal stack (stored in thread-local memory).
/// For ray-tracing voxel bricks we can use a short stack.
#define BVH_SHORT_STACK_SIZE (16)//(32)

//#define BVH_NIL_NODE_INDEX	(-1)
#define BVH_NIL_NODE_INDEX	(0xFFFFFFFF)

#define BVH_NODE_GET_CHILD0_INDEX(node, node_index)	(node_index + 1)
#define BVH_NODE_GET_CHILD1_INDEX(node, node_index)	(node.GetRightChildIndex())


bool VoxelBVH_TraceRay(
	in PrecomputedRay precomp_ray
	, in float ray_tmax //= 1e34
	, in VoxelBvh bvh
	, in VoxelBvhResources resources
)
{
	// Allocate traversal stack from thread-local memory.
	// (the stack is just a fixed size array of indices to BVH nodes)
	uint traversal_stack[BVH_SHORT_STACK_SIZE];
	
	uint stack_top = 0;	// == the number of nodes pushed onto the stack
	
	// Push NULL to indicate that there are no postponed nodes.
	traversal_stack[0] = BVH_NIL_NODE_INDEX;

	// The index of the current node.
	uint	curr_node_idx = 0;	// start with the root node
	
	BVH_USER_DECLARE_STATE;

	do
	{
		const BVH_USER_NODE node = bvh.nodes[ curr_node_idx ];

		// If this is an inner node:
		if(node.IsInternal())
		{
			// Intersect the ray against the child nodes.
			const uint child0_idx = BVH_NODE_GET_CHILD0_INDEX(node, curr_node_idx);
			const uint child1_idx = BVH_NODE_GET_CHILD1_INDEX(node, curr_node_idx);
			//
			const BVH_USER_NODE child0 = bvh.nodes[ child0_idx ];
			const BVH_USER_NODE child1 = bvh.nodes[ child1_idx ];
			//
			const float2 child0_tnear_tfar = IntersectRayAgainstAABB(
				precomp_ray
				, ray_tmax
				, child0.aabb_min
				, child0.aabb_max
			);
			const float2 child1_tnear_tfar = IntersectRayAgainstAABB(
				precomp_ray
				, ray_tmax
				, child1.aabb_min
				, child1.aabb_max
			);

			const bool hit_child0 = RAY_DID_HIT_AABB(child0_tnear_tfar);
			const bool hit_child1 = RAY_DID_HIT_AABB(child1_tnear_tfar);

			// If no children were hit, pop the node off the stack.
			if( !hit_child0 && !hit_child1 )
			{
				curr_node_idx = traversal_stack[ stack_top-- ];
			}
			else
			{
				// One or both child nodes were hit.
				
				// Take the first child that was hit.
				curr_node_idx = hit_child0 ? child0_idx : child1_idx;
				
				// If both children were hit, traverse the closest one first.
				if( hit_child0 && hit_child1 )
				{
					// If the right child was actually closer:
					if( child1_tnear_tfar.x < child0_tnear_tfar.x )
					{
						// First visit the right child:
						curr_node_idx = child1_idx;
						// And push the left child for visiting it later:
						traversal_stack[ ++stack_top ] = child0_idx;
					}
					else
					{
						traversal_stack[ ++stack_top ] = child1_idx;
					}
				}//If both children were hit.
			}//If children were hit.
		}
		else
		{
			// This is a leaf node.

			//TODO: optimize, we already know it intersects
			const float2 node_aabb_tnear_tfar = IntersectRayAgainstAABB(
				precomp_ray
				, ray_tmax
				, node.aabb_min
				, node.aabb_max
			);
			
			BVH_USER_INTERSECT_LEAF(
				precomp_ray
				, node
				, node_index
				, trace_state
				, node_aabb_tnear_tfar
			);
// if(RayIntersectsVoxelBrick()) {
	// return true;
// }
			
			curr_node_idx = traversal_stack[ stack_top-- ];
		}
	}	
	while( curr_node_idx != BVH_NIL_NODE_INDEX );
	
	// The ray didn't hit anything.
	return false;
}

#endif // VOXEL_BVH_TRAVERSE_HLSLI
