/*
Full-Screen shader for visualizing the BVH with voxel bricks at leaf nodes.

%%
pass Unlit {
	compute = main_CS
}
samplers {
	s_trilinear = TrilinearSampler
	//s_trilinear = PointSampler
}
%%
*/

#include <Shared/nw_shared_globals.h>
#include <RayTracing/ray_intersection.hlsl>
#include <Common/constants.hlsli>
#include <Common/Colors.hlsl>

#include "bvh_interop.h"
#include <VoxelGI/sdf_bvh_traversal.hlsli>

/// Brick atlas texture, used with hardware trilinear filtering
Texture3D< float >		t_sdf_atlas3d;

SamplerState			s_trilinear;
//trilinear_wrap_sampler

///
RWTexture2D< float4 >	t_output_rt;

//--------------------------------------------------

// Transforms the normalized device coordinate 'ndc' into world-space.
float3 UnprojectPoint(float3 ndc)
{
	float4 projPos = mul(
		g_inverse_view_projection_matrix,
		//g_inverse_projection_matrix,//-
		float4(ndc, 1.0)
	);
	return projPos.xyz/projPos.w;
}

[numthreads( 8, 8, 1 )]
void main_CS( uint3 dispatch_thread_id: SV_DispatchThreadID )
{
	uint dst_width, dst_height;
#if 1
    t_output_rt.GetDimensions(dst_width, dst_height);
#else
	dst_width = 1024;
	dst_height = 768;
#endif	
	//
	const float2 pixel_coords01_in_screen_space
		= (dispatch_thread_id.xy + float2(0.5f, 0.5f))
		/ float2(dst_width, dst_height)
		;
	
    float2 pixel_coords_NDC = pixel_coords01_in_screen_space * 2.f - 1.f;
	//pixel_coords_NDC.y = 1 - pixel_coords_NDC.y;
	//
/*
	const float farPlaneDist = 1;	// distance from eye position to the image plane
	const float aspect_ratio = 1.3333;//g_camera_fwd_WS.w;	// screen width / screen height
	const float tanHalfFoVy = tan(0.78539819);// g_camera_right_WS.w;
	
	const float imagePlaneH = farPlaneDist * tanHalfFoVy;//g_viewport_tanHalfFoVy();	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * aspect_ratio;//g_viewport_aspectRatio();	// half-width of the image plane

	const float3 f3FarCenter = g_camera_fwd_WS.xyz * farPlaneDist;
	const float3 f3FarRight = g_camera_right_WS.xyz * imagePlaneW;	// scaled 'right' vector on the far plane, in world space
	const float3 f3FarUp = g_camera_up_WS.xyz * imagePlaneH;	// scaled 'up' vector on the far plane, in world space
*/


	const float3 ray_origin_WS = g_camera_origin_WS.xyz;
	
	const float3 point_on_image_plane_WS = UnprojectPoint(
		float3(pixel_coords_NDC, 0.1)
	);

#if 1
	float3 ray_direction_WS
		= g_camera_right_WS.xyz * pixel_coords_NDC.x
		- g_camera_up_WS.xyz * pixel_coords_NDC.y	// flip Y
		+ g_camera_fwd_WS.xyz * 1 // near plane == 1
		;
#elif 0
	float3 ray_direction_WS = g_camera_fwd_WS.xyz;
#else
	float3 ray_direction_WS = point_on_image_plane_WS - ray_origin_WS;
#endif

	ray_direction_WS = normalize(ray_direction_WS);

	//
	PrecomputedRay	precomp_ray;
	precomp_ray.SetFromRay(
		ray_origin_WS
		, ray_direction_WS
	);

	//
	SdfBvhTraceConfig	sdf_trace_config;
	sdf_trace_config.SetDefaults();
	sdf_trace_config.min_step_distance_UVW = 1e-4f;
	sdf_trace_config.min_solid_hit_distance_UVW = 0;// 1e-3;
	sdf_trace_config.sdf_atlas_texture = t_sdf_atlas3d;
	sdf_trace_config.inverse_sdf_atlas_resolution = g_sdf_bvh.inverse_sdf_atlas_resolution;
	//
	// uint	total_num_raymarching_steps_taken = 0;
	
	const SdfBvhClosestHitTraceResult closest_hit_result =
	//SdfBVH_TraceRay_AnyHit	//+
	//SdfBVH_TraceRay	//-
	SdfBVH_TraceRay_ClosestHit
	(
		precomp_ray

		, g_sdf_bvh

		, sdf_trace_config
		, s_trilinear
	);
	
	//
	float3	result_color = (float3) 0;
	//
	if(closest_hit_result.DidHitAnything())
	{
		if(closest_hit_result.did_hit_solid_leaf)
		{
			result_color = GREEN.rgb;
		}
		else
		{
			result_color = BLUE.rgb;
		}
		result_color *= 0.3f;

		//
		//const float3 ray_hit_pos = ray_origin_WS + ray_direction_WS * closest_hit_time;
//result_color = ray_hit_pos;
		
		const float ray_hit_distance = closest_hit_result.dist_until_hit_WS / 100;
		//result_color *= ray_hit_distance*0.001f + total_num_raymarching_steps_taken * 0.5f;
		
		//result_color += (float3)ray_hit_distance; // [0..1] - in texture volume
		result_color += (float3)ray_hit_distance * 0.6f;

//result_color += closest_hit_result.max_traversal_depth * 0.05f;

		
		//white-washed if multi-brick
		//result_color += total_num_raymarching_steps_taken * 0.05f;
	
/*
result_color = sdf_hit_dist * 0.3;
*/
//result_color = (total_num_raymarching_steps_taken / MAX_SDF_TRACE_STEPS) * 100;
//result_color = total_num_raymarching_steps_taken * 1e-1;
	}//if hit


//DBG
if(0)
{
	float tnear, tfar;
	if(IntersectRaySphere(
			ray_origin_WS, ray_direction_WS
			, (float3) 0, 16
			, tnear, tfar
			))
	{
		result_color += RED.rgb*0.1f;
	}
}


	//

	//t_output_rt[ dispatch_thread_id.xy ] = float4(ray_hit_pos, 1);//+
	
#if 0	// debug: display depth only
	// multiply by 0.01f to avoid all white
	t_output_rt[ dispatch_thread_id.xy ] = (float4) ray_hit_distance*0.01f;
	return;
#endif
	
	t_output_rt[ dispatch_thread_id.xy ] = float4(
		result_color.rgb,
		1
	);
}
