// 
#ifndef SCENE_SHADOWS_HLSL
#define SCENE_SHADOWS_HLSL

#include <VoxelGI/bvh_interop.h>
#include <VoxelGI/sdf_bvh_traversal.hlsli>

/// Returns true if the ray from the surface point towards the light
/// intersects something. This is used for hard ray-traced shadows.
bool IsInShadow(
	in float3 surface_position_WS
	, in float3 surface_normal_WS
	
	, in float3 direction_to_light_WS
	
	, in Texture3D< float >	t_sdf_atlas3d
	, in SamplerState		s_trilinear
)
{
	//
	SdfBvhTraceConfig	sdf_trace_config;
	sdf_trace_config.SetDefaultsForTracingHardShadows();
	sdf_trace_config.sdf_atlas_texture = t_sdf_atlas3d;
	sdf_trace_config.inverse_sdf_atlas_resolution = g_sdf_bvh.inverse_sdf_atlas_resolution;

	// add bias to avoid self-shadowing
	const float3 biased_pos_WS
		= surface_position_WS
		+ surface_normal_WS * SDF_BVH_SURFACE_BIAS
		;
	//
	PrecomputedRay precomp_ray;
	precomp_ray.SetFromRay(
		biased_pos_WS
		, direction_to_light_WS
	);

	const bool hit_anything = SdfBVH_TraceRay_AnyHit(
		precomp_ray

		, g_sdf_bvh

		, sdf_trace_config
		, s_trilinear
	);
	return hit_anything;
}

#endif // SCENE_SHADOWS_HLSL
