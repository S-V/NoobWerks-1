/*
%%
pass DDGI_TraceProbeRays {
	compute = main_CS
}
%%
*/
#include <Shared/nw_shared_globals.h>

#include "ddgi_interop.h"
#include "ddgi_common.hlsli"

#include "vxgi_interop.h"
#include "vxgi_common.hlsli"
#include "voxel_raymarching.hlsli"


// voxel grid
Texture3D< float4 >		t_radiance_opacity_volume;

RWTexture2D< float4 >	t_output_color;

//-----------------------------------------------------------------------------

[numthreads( 8, 8, 1 )]
void main_CS( uint3 dispatch_thread_id: SV_DispatchThreadID )
{
	uint dst_width, dst_height;
    t_output_color.GetDimensions(dst_width, dst_height);
	
	//
	const float2 pixel_coords01_in_screen_space
		= (dispatch_thread_id.xy + float2(0.5f, 0.5f))
		/ float2(dst_width, dst_height)
		;
	
    const float2 pixel_coords_NDC = pixel_coords01_in_screen_space * 2.f - 1.f;
	
	//
	const float3 ray_origin_WS = g_camera_origin_WS.xyz;

	float3 ray_direction
		= g_camera_right_WS.xyz * pixel_coords_NDC.x
		- g_camera_up_WS.xyz * pixel_coords_NDC.y	// flip Y
		+ g_camera_fwd_WS.xyz // near plane == 1
		;
	ray_direction = normalize(ray_direction);

	
	//
	GPU_VXGI_Cascade	vxgi_cascade0 = g_vxgi.cascades[0];
	
	const float3 ray_origin_in_voxel_uvw_space = vxgi_cascade0.TransformFromWorldToUvwTextureSpace(ray_origin_WS);
	
	//
	VoxelGridConfig	voxel_grid_config;
	voxel_grid_config.grid_resolution			= g_vxgi.cascadeResolutionF();
	voxel_grid_config.inverse_grid_resolution	= g_vxgi.inverseCascadeResolutionF();
	voxel_grid_config.radiance_opacity_volume	= t_radiance_opacity_volume;
	
	//
	int3	hit_voxel_coords;
	bool	did_hit_sky;
	const float4 traced_radiance_and_opacity = RaymarchThroughVoxelGrid(
		ray_origin_in_voxel_uvw_space
		, ray_direction

		, voxel_grid_config

		, hit_voxel_coords
		, did_hit_sky
	);
	
	const float3 ray_hit_pos = vxgi_cascade0.GetWorldPosFromGridCoords(hit_voxel_coords);
	const float ray_hit_distance = length(ray_hit_pos - ray_origin_WS);
	
	
	//t_output_color[ dispatch_thread_id.xy ] = float4(ray_hit_pos, 1);//+
	
#if 0	// debug: display depth only
	// multiply by 0.01f to avoid all white
	t_output_color[ dispatch_thread_id.xy ] = (float4) ray_hit_distance*0.01f;
	return;
#endif
	
	t_output_color[ dispatch_thread_id.xy ] = float4(
		traced_radiance_and_opacity.rgb,
		ray_hit_distance
	);
}
