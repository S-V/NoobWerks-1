/*
Traces rays and writes results into ray-probe G-Buffer.
%%
pass DDGI_TraceProbeRays {
	compute = main_CS
}
samplers {
	s_trilinear = TrilinearSampler
}
%%
*/
#include "ddgi_interop.h"
#include "ddgi_common.hlsli"

/// 0 - to enable voxel grid tracing (a lot worse precision, light leaks)
#define RAYTRACE_SDF_BVH	(1)


#include "vxgi_interop.h"
#include "vxgi_common.hlsli"

#if RAYTRACE_SDF_BVH
	#include <VoxelGI/bvh_interop.h>
	#include <VoxelGI/sdf_bvh_traversal.hlsli>
#else
	#include "voxel_raymarching.hlsli"
	#include <VoxelGI/voxel_shadowing.hlsli>
#endif



#if RAYTRACE_SDF_BVH
	// for precise occlusion/distances
	Texture3D< float >		t_sdf_atlas3d;
#endif

// voxel grid
Texture3D< float4 >		t_radiance_opacity_volume;


SamplerState	s_trilinear;


// width == num rays per probe,
// height == num probes
// RGBA16F: xyz - raytraced radiance, w - hit distance
RWTexture2D< float4 >	t_dst_hit_radiance_and_distance;
//t_dst_raytraced_probe_radiance_and_distance

//-----------------------------------------------------------------------------

[numthreads( 32, 1, 1 )]
void main_CS( uint3 dispatch_thread_id: SV_DispatchThreadID )
{
	const uint ray_index   = dispatch_thread_id.x;
	const uint probe_index = dispatch_thread_id.y;

	const float3 ray_direction_WS = g_ddgi.ray_directions[ ray_index ].xyz;
	
	const int3 probe_coords = DDGI_GetProbeCoordsByIndex(
		probe_index,
		g_ddgi
	);

	const float3 probe_center_WS = DDGI_GetProbePosition(
		probe_coords,
		g_ddgi
	);
	
	
	//
	GPU_VXGI_Cascade	vxgi_cascade0 = g_vxgi.cascades[0];
	
	//
	mxOPTIMIZE("don't trace probes inside solid");
/*
	const bool is_probe_in_solid = ?;
	if(is_probe_in_solid) {
		?;
	}
*/

#if RAYTRACE_SDF_BVH

	//
	SdfBvhTraceConfig	sdf_trace_config;
	sdf_trace_config.SetDefaultsForTracingHardShadows();
	sdf_trace_config.sdf_atlas_texture = t_sdf_atlas3d;
	sdf_trace_config.inverse_sdf_atlas_resolution = g_sdf_bvh.inverse_sdf_atlas_resolution;

	//				
	PrecomputedRay precomp_ray;
	precomp_ray.SetFromRay(
		probe_center_WS
		, ray_direction_WS
	);

	const SdfBvhClosestHitTraceResult closest_hit = SdfBVH_TraceRay_ClosestHit(
		precomp_ray

		, g_sdf_bvh

		, sdf_trace_config
		, s_trilinear
	);
	
	const float ray_hit_distance = closest_hit.dist_until_hit_WS;
	
	const float3 ray_hit_pos_WS
		= probe_center_WS + ray_direction_WS * closest_hit.dist_until_hit_WS
		;	
	const float3 ray_hit_pos_voxel_UVW
		= vxgi_cascade0.TransformFromWorldToUvwTextureSpace(ray_hit_pos_WS)
		;
	const float4 traced_radiance_and_opacity = t_radiance_opacity_volume.SampleLevel(
		s_trilinear,
		ray_hit_pos_voxel_UVW,
		0
	);
	
#else

	const float3 probe_center_in_voxel_uvw_space
		= vxgi_cascade0.TransformFromWorldToUvwTextureSpace(probe_center_WS)
		;

	//
	VoxelGridConfig	voxel_grid_config;
	voxel_grid_config.grid_resolution			= g_vxgi.cascadeResolutionF();
	voxel_grid_config.inverse_grid_resolution	= g_vxgi.inverseCascadeResolutionF();
	voxel_grid_config.radiance_opacity_volume	= t_radiance_opacity_volume;
	
	//
	int3	hit_voxel_coords;
	bool	did_hit_sky;
	const float4 traced_radiance_and_opacity = RaymarchThroughVoxelGrid(
		probe_center_in_voxel_uvw_space
		, ray_direction_WS

		, voxel_grid_config

		, hit_voxel_coords
		, did_hit_sky
	);
	
	const float3 ray_hit_pos = vxgi_cascade0.GetWorldPosFromGridCoords(hit_voxel_coords);
	const float ray_hit_distance = length(ray_hit_pos - probe_center_WS);
#endif	// !RAYTRACE_SDF_BVH
	
	//
	t_dst_hit_radiance_and_distance[ dispatch_thread_id.xy ] = float4(
		traced_radiance_and_opacity.rgb,
		ray_hit_distance
	);
}
