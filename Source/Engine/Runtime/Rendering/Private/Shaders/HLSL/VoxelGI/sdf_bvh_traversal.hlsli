// Ray Traversal of BVH with SDF 'Bricks' in leaf nodes.
#ifndef SDF_BVH_TRAVERSAL_HLSLI
#define SDF_BVH_TRAVERSAL_HLSLI

#include <base.hlsli>	// nwMAX_CB_SIZE
#include <VoxelGI/sdf_common.hlsl>


#define SDF_BVH_USE_RCP	(1)


/// to prevent the GPU from hanging and the PC from crashing
#define SDF_BVH_LIMIT_NUM_ITERATIONS	(1)

static const uint BVH_MAX_LOOP_ITERATIONS = 512;


static const uint BVH_ROOT_NODE_INDEX = 0;

static const float BVH_MAX_FLOAT = 3.402823466e+38f;


// a bias to avoid self-intersection when ray-tracing
// TODO: don't hardcode, should be proportional to g_sdf_bvh.sdf_voxel_size_at_LoD0
static const float SDF_BVH_SURFACE_BIAS = 1.5;




/*
====================================================
	SDF BRICK RAYMARCHING
====================================================
*/

/// Config for sphere tracing through SDF bricks in a BVH.
struct SdfBvhTraceConfig
{
	/// can be zero; usually, a small positive number
	float min_solid_hit_distance_UVW;

	/// must be >= 0; usually, a fraction of voxel's size
	float min_step_distance_UVW;


	/// The volume texture with normalized distance values
	/// in [0..1] range (decoded to [-1..1] in the shader).
	Texture3D< float >	sdf_atlas_texture;
	
	///
	float inverse_sdf_atlas_resolution;
	
	

	void SetDefaults()
	{
		min_step_distance_UVW = 1e-3f;
		min_solid_hit_distance_UVW = 0;
	}
	
	void SetDefaultsForTracingHardShadows()
	{
		min_step_distance_UVW = 1e-3f;

		// NOTE: must be > 0 to avoid unshadowed 'contours' (like in variance shadows)
		min_solid_hit_distance_UVW = 1e-3;
	}
};


///
struct SdfTraceResult
{
	float	ray_hit_time;
	uint	num_steps_taken;
	
	void Reset()
	{
		ray_hit_time = -1;
		num_steps_taken = 0;
	}
	
	bool DidHitAnything()
	{ return ray_hit_time >= 0; }
};


/// Traces through SDF brick in volume texture space.
SdfTraceResult TraceThroughSdfBrick2(
	in float3 ray_origin_WS
	, in float3 ray_direction_WS

	, in SdfBvhTraceConfig config
	, in SamplerState trilinear_sampler
	, in uint3 padded_brick_offset_within_sdf_atlas

	, in float3 brick_aabb_min_WS
	, in float3 brick_aabb_max_WS
	, in float2 ray_aabb_intersection_times
	, in uint3 brick_resolution_in_voxels_no_margins
)
{
	const float3 aabb_entry_pos_WS
		= ray_origin_WS
		+ ray_direction_WS * ray_aabb_intersection_times.x
		;
	const float3 ray_origin_relative_to_AABB_corner
		= aabb_entry_pos_WS - brick_aabb_min_WS
		;
	//
	const float3 brick_size_WS_no_margins
		= brick_aabb_max_WS - brick_aabb_min_WS
		;

	const float3 inverse_sdf_brick_size_WS_no_margins =
	#if SDF_BVH_USE_RCP
		rcp(brick_size_WS_no_margins)
	#else
		1.0f / brick_size_WS_no_margins
	#endif
		;

	// Add a small offset along ray direction to start sampling inside the volume texture.
	const float ray_origin_bias_WS = 1e-3f;

	// Get the UVW coords of the ray origin within brick-local texture space (excluding margins)
	const float3 ray_origin_in_brick_local_UVW
		= (ray_origin_relative_to_AABB_corner + ray_direction_WS * ray_origin_bias_WS)
		* inverse_sdf_brick_size_WS_no_margins
		;
	//
	const float3 tex_coord_offset_in_atlas_UVW
		= (padded_brick_offset_within_sdf_atlas
		+ SDF_BRICK_MARGIN_VOXELS
		+ 0.5
		) * config.inverse_sdf_atlas_resolution
		;

	// scale from the brick's texture space [0..1]^3 into the atlas' texture space [0..1]^3
	// ; always <= 1
	const float3 tex_coord_scale_in_atlas_UVW
		= brick_resolution_in_voxels_no_margins * config.inverse_sdf_atlas_resolution
		;

	//
	SdfTraceResult	trace_result;
	trace_result.Reset();
	//
	float curr_ray_dist_UVW = 0;
	//
	int step_counter = 0;
	for( ; step_counter < MAX_SDF_TRACE_STEPS; step_counter++ )
	{
		const float3 curr_pos_on_ray__brick_local_UVW
			= ray_origin_in_brick_local_UVW
			+ ray_direction_WS * curr_ray_dist_UVW
			;
		if(Is3DTextureCoordOutsideVolume(curr_pos_on_ray__brick_local_UVW)) {
			// exited the grid
			break;
		}

		//
		const float3 texture_coords_in_sdf_atlas
			= curr_pos_on_ray__brick_local_UVW * tex_coord_scale_in_atlas_UVW
			+ tex_coord_offset_in_atlas_UVW
			;

		const float sampled_sdf_value01 = config.sdf_atlas_texture.SampleLevel(
			trilinear_sampler
			, texture_coords_in_sdf_atlas
			, 0 // mip_level
		);
		const float sdf_value = Decode01_to_minus1plus1(sampled_sdf_value01);

		if(sdf_value <= config.min_solid_hit_distance_UVW)
		{
			// hit solid
			trace_result.ray_hit_time = curr_ray_dist_UVW;
			break;
		}

		const float step_distance = max( sdf_value, config.min_step_distance_UVW );
		curr_ray_dist_UVW += step_distance;
	}//for

	trace_result.num_steps_taken = step_counter;
	return trace_result;
}




/*
====================================================
	BVH RAY-TRACING
====================================================
*/

struct SdfBvhClosestHitTraceResult
{
	float	dist_until_hit_WS;
	uint	max_traversal_depth;
	bool	did_hit_solid_leaf;
	
	void Reset()
	{
		dist_until_hit_WS = BVH_MAX_FLOAT;
		max_traversal_depth = 0;
		did_hit_solid_leaf = false;
	}
	
	bool DidHitAnything()
	{ return dist_until_hit_WS < BVH_MAX_FLOAT; }
};



/// The depth of the BVH traversal stack (stored in thread-local memory).
/// A shallow stack can be used for ray-tracing voxel bricks (compared to triangles).
#define BVH_STACK_SIZE (16)//(32)

//#define BVH_NIL_NODE_INDEX	(-1)
#define BVH_NIL_NODE_INDEX	(0xFFFFFFFF)

#define BVH_NODE_GET_CHILD0_INDEX(node, node_index)	(node_index + 1)
#define BVH_NODE_GET_CHILD1_INDEX(node, node_index)	(node.GetRightChildIndex())





/// for finding any hit (occlusion)
bool SdfBVH_TraceRay_AnyHit(
	in PrecomputedRay precomp_ray

	, in SdfBvhData bvh

	, in SdfBvhTraceConfig sdf_trace_config
	, in SamplerState sdf_volume_trilinear_sampler
)
{
	// Allocate traversal stack from thread-local memory.
	// (the stack is just a fixed size array of indices to BVH nodes)
	uint traversal_stack[BVH_STACK_SIZE];
	
	uint stack_top = 0;	// == the number of nodes pushed onto the stack
	
	// Push NULL to indicate that there are no postponed nodes.
	traversal_stack[0] = BVH_NIL_NODE_INDEX;

	// The index of the current node.
	uint	curr_node_idx = 0;	// start with the root node
	
	//
	uint loop_counter = 0;

	while(curr_node_idx != BVH_NIL_NODE_INDEX
#if SDF_BVH_LIMIT_NUM_ITERATIONS
	&& (loop_counter++ < BVH_MAX_LOOP_ITERATIONS)
#endif // SDF_BVH_LIMIT_NUM_ITERATIONS
	)
	{
		const SdfBvhNode bvh_node = bvh.nodes[ curr_node_idx ];

		const float2 ray_node_aabb_hit_times = IntersectRayAgainstAABB(
			precomp_ray
			, bvh_node.aabb_min
			, bvh_node.aabb_max
		);
		if(!RAY_DID_HIT_AABB(ray_node_aabb_hit_times)) {
			curr_node_idx = traversal_stack[ stack_top-- ];
			continue;
		}

		// If this is an inner node:
		if(bvh_node.IsInternal())
		{
			const uint child0_idx = BVH_NODE_GET_CHILD0_INDEX(bvh_node, curr_node_idx);
			const uint child1_idx = BVH_NODE_GET_CHILD1_INDEX(bvh_node, curr_node_idx);

			curr_node_idx = child0_idx;
			traversal_stack[ ++stack_top ] = child1_idx;
		}
		else
		{
			// This is a leaf node.
			
			if(bvh_node.IsSolidLeaf()) {
				return true;
			}

			// Intersect heterogeneous leaf.
			{
				const uint3 padded_brick_offset_within_sdf_atlas
					= bvh_node.GetBrickOffsetWithinBrickAtlas()
					;
				//
				const SdfTraceResult sdf_brick_trace_result = TraceThroughSdfBrick2(
					precomp_ray.origin
					, precomp_ray.direction

					, sdf_trace_config
					, sdf_volume_trilinear_sampler
					, padded_brick_offset_within_sdf_atlas

					, bvh_node.aabb_min
					, bvh_node.aabb_max
					, ray_node_aabb_hit_times
					, bvh_node.GetBrickInnerResolution()
				);
				if(sdf_brick_trace_result.DidHitAnything()) {
					return true;
				}
			}
			//
			curr_node_idx = traversal_stack[ stack_top-- ];
		}//If hit a leaf.
	}//while curr node != nil

	// The ray didn't hit anything.
	return false;
}



bool SdfBVH_TraceRay_AnyHit_TestClosestFirst(
	in PrecomputedRay precomp_ray

	, in SdfBvhData bvh

	, in SdfBvhTraceConfig sdf_trace_config
	, in SamplerState sdf_volume_trilinear_sampler
)
{
	// Allocate traversal stack from thread-local memory.
	// (the stack is just a fixed size array of indices to BVH nodes)
	uint traversal_stack[BVH_STACK_SIZE];
	
	uint stack_top = 0;	// == the number of nodes pushed onto the stack
	
	// Push NULL to indicate that there are no postponed nodes.
	traversal_stack[0] = BVH_NIL_NODE_INDEX;

	// The index of the current node.
	uint	curr_node_idx = 0;	// start with the root node
	
	//BVH_USER_DECLARE_STATE;

	//
	uint loop_counter = 0;

	while(curr_node_idx != BVH_NIL_NODE_INDEX
#if SDF_BVH_LIMIT_NUM_ITERATIONS
	&& (loop_counter++ < BVH_MAX_LOOP_ITERATIONS)
#endif // SDF_BVH_LIMIT_NUM_ITERATIONS
	)
	{
		const SdfBvhNode bvh_node = bvh.nodes[ curr_node_idx ];

		// If this is an inner node:
		if(bvh_node.IsInternal())
		{
			// Intersect the ray against the child nodes.
			const uint child0_idx = BVH_NODE_GET_CHILD0_INDEX(bvh_node, curr_node_idx);
			const uint child1_idx = BVH_NODE_GET_CHILD1_INDEX(bvh_node, curr_node_idx);
			//
			const SdfBvhNode child0 = bvh.nodes[ child0_idx ];
			const SdfBvhNode child1 = bvh.nodes[ child1_idx ];
			//
			const float2 child0_tnear_tfar = IntersectRayAgainstAABB(
				precomp_ray
				, child0.aabb_min
				, child0.aabb_max
			);
			const float2 child1_tnear_tfar = IntersectRayAgainstAABB(
				precomp_ray
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
			
			if(bvh_node.IsSolidLeaf()) {
				return true;
			}
			
			const float2 ray_node_aabb_hit_times = IntersectRayAgainstAABB(
				precomp_ray
				, bvh_node.aabb_min
				, bvh_node.aabb_max
			);
			
			{
				const uint3 padded_brick_offset_within_sdf_atlas
					= bvh_node.GetBrickOffsetWithinBrickAtlas()
					;
				//
				const SdfTraceResult sdf_brick_trace_result = TraceThroughSdfBrick2(
					precomp_ray.origin
					, precomp_ray.direction

					, sdf_trace_config
					, sdf_volume_trilinear_sampler
					, padded_brick_offset_within_sdf_atlas

					, bvh_node.aabb_min
					, bvh_node.aabb_max
					, ray_node_aabb_hit_times
					, bvh_node.GetBrickInnerResolution()
				);
				if(sdf_brick_trace_result.DidHitAnything()) {
					return true;
				}
			}
			//
			curr_node_idx = traversal_stack[ stack_top-- ];
		}//If hit a leaf.
	}//while curr node != nil

	// The ray didn't hit anything.
	return false;
}//while curr node != nil




SdfBvhClosestHitTraceResult SdfBVH_TraceRay_ClosestHit(
	in PrecomputedRay precomp_ray

	, in SdfBvhData bvh

	, in SdfBvhTraceConfig sdf_trace_config
	, in SamplerState sdf_volume_trilinear_sampler
)
{
	//
	SdfBvhClosestHitTraceResult	closest_hit_result;
	closest_hit_result.Reset();

	// Allocate traversal stack from thread-local memory.
	// (the stack is just a fixed size array of indices to BVH nodes)
	uint traversal_stack[BVH_STACK_SIZE];
	
	int stack_top = 0;	// == the number of nodes pushed onto the stack

	traversal_stack[ 0 ] = BVH_NIL_NODE_INDEX;
	
	// The index of the current node.
	uint	curr_node_idx = 0;	// start with the root node

	//
	uint loop_counter = 0;

	while(curr_node_idx != BVH_NIL_NODE_INDEX
#if SDF_BVH_LIMIT_NUM_ITERATIONS
	&& (loop_counter++ < BVH_MAX_LOOP_ITERATIONS)
#endif // SDF_BVH_LIMIT_NUM_ITERATIONS
	)
	{
		const SdfBvhNode bvh_node = bvh.nodes[ curr_node_idx ];
		
		const float2 ray_node_aabb_hit_times = IntersectRayAgainstAABB(
			precomp_ray
			, bvh_node.aabb_min
			, bvh_node.aabb_max
		);

		// skip testing this node if:
		if(
			// the ray doesn't intersect the AABB
			!RAY_DID_HIT_AABB(ray_node_aabb_hit_times)
			||
			// or this node is further than the closest found intersection
			ray_node_aabb_hit_times.x > closest_hit_result.dist_until_hit_WS
		)
		{
			curr_node_idx = traversal_stack[ stack_top-- ];
			continue;
		}

		const float ray_hit_time_WS = ray_node_aabb_hit_times.x;
	
		// If this is an inner node:
		if(bvh_node.IsInternal())
		{
			const uint child0_idx = BVH_NODE_GET_CHILD0_INDEX(bvh_node, curr_node_idx);
			const uint child1_idx = BVH_NODE_GET_CHILD1_INDEX(bvh_node, curr_node_idx);

			// Traverse the closest child first.
			
			const uint split_axis = bvh_node.GetSplitAxis();
			
			const uint near_child_idx
				= (precomp_ray.direction[ split_axis ] < 0)
				? child0_idx
				: child1_idx
				;
				
			const uint far_child_idx
				= (precomp_ray.direction[ split_axis ] < 0)
				? child1_idx
				: child0_idx
				;
			
			curr_node_idx = near_child_idx;
			traversal_stack[ ++stack_top ] = far_child_idx;
		}
		else
		{
			// This is a leaf node.

			/* Solid leafs are disabled for now to minimize branches.
			if(bvh_node.IsSolidLeaf())
			{
				[flatten]
				if(ray_hit_time_WS < closest_hit_result.dist_until_hit_WS) {
					closest_hit_result.dist_until_hit_WS = ray_hit_time_WS;
				}
				closest_hit_result.did_hit_solid_leaf = true;
			}
			else
			*/
			{
				const SdfTraceResult sdf_brick_trace_result = TraceThroughSdfBrick2(
					precomp_ray.origin
					, precomp_ray.direction

					, sdf_trace_config
					, sdf_volume_trilinear_sampler
					, bvh_node.GetBrickOffsetWithinBrickAtlas()

					, bvh_node.aabb_min
					, bvh_node.aabb_max
					, ray_node_aabb_hit_times
					, bvh_node.GetBrickInnerResolution()
				);
				const float ray_hit_time_WS = lerp(
					ray_node_aabb_hit_times.x,
					ray_node_aabb_hit_times.y,
					sdf_brick_trace_result.ray_hit_time
				);

				[flatten]
				if(sdf_brick_trace_result.DidHitAnything()
				&& ray_hit_time_WS < closest_hit_result.dist_until_hit_WS
				)
				{
					closest_hit_result.dist_until_hit_WS = ray_hit_time_WS;

					closest_hit_result.max_traversal_depth = max(
						stack_top,
						closest_hit_result.max_traversal_depth
					);
				}
			}//If hit heterogeneous leaf.
			
			curr_node_idx = traversal_stack[ stack_top-- ];
			
		}//If hit a leaf.
	}//While curr node != nil

	return closest_hit_result;
}



bool SdfBVH_TraceRay_DebugNoAccelSlow(
	in PrecomputedRay precomp_ray

	, in SdfBvhData bvh

	, in SdfBvhTraceConfig sdf_trace_config
	, in SamplerState sdf_volume_trilinear_sampler
)
{
	for(uint iNode = 0; iNode < g_sdf_bvh.num_nodes; iNode++)
	{
		const SdfBvhNode bvh_node = g_sdf_bvh.nodes[ iNode ];
		
		if(bvh_node.IsInternal()) {
			continue;
		}
		
		//
		const float2 ray_aabb_hit_times = IntersectRayAgainstAABB(
			precomp_ray
			, bvh_node.aabb_min
			, bvh_node.aabb_max
		);
		if(RAY_DID_HIT_AABB(ray_aabb_hit_times))
		{
			if(bvh_node.IsSolidLeaf())
			{
				return true;
			}
			
			//
			const SdfTraceResult sdf_brick_trace_result = TraceThroughSdfBrick2(
				precomp_ray.origin
				, precomp_ray.direction

				, sdf_trace_config
				, sdf_volume_trilinear_sampler
				, bvh_node.GetBrickOffsetWithinBrickAtlas()

				, bvh_node.aabb_min
				, bvh_node.aabb_max
				, ray_aabb_hit_times
				, bvh_node.GetBrickInnerResolution()
			);
			if(sdf_brick_trace_result.DidHitAnything()) {
				return true;
			}
		}//if ray hit node AABB
	}//for each node
	
	return false;
}

#endif // SDF_BVH_TRAVERSAL_HLSLI
