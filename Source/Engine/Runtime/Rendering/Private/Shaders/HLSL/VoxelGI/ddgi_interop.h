// Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields
#ifdef __cplusplus
#pragma once
#endif

#ifndef DDGI_INTEROP_H
#define DDGI_INTEROP_H

#include <Shared/__shader_interop_pre.h>

/// We encode the spherical diffuse irradiance
/// (in R11G11B10F format at 6x6 octahedral resolution),
/// spherical distance and squared distances to the nearest
/// geometry (both in RG16F format at 16x16 octahedral resolution)



#if 0	//larger values for debugging
	// cannot make greater than 32, because of CS limitations: error X3655:
	// the product of the arguments of numthreads(,,) must be less than or equal to 1024
	#define IRRADIANCE_PROBE_DIM	(32)
	#define DEPTH_PROBE_DIM			(32)
#else
	// The number of texels used in one dimension of the irradiance texture,
	// not including the 1-pixel border on each side.
	#define IRRADIANCE_PROBE_DIM	(6)

// The number of texels used in one dimension of the distance texture,
	// not including the 1-pixel border on each side.
	#define DEPTH_PROBE_DIM			(16)
#endif


///
/// for avoiding error X3676: typed UAV loads are only allowed
/// for single-component 32-bit element types
///
#define DDGI_CONFIG_PACK_R11G11B10_INTO_UINT	(0)


/// The number of rays emitted each frame for each probe in the scene.
#define RAYS_PER_PROBE	(128)

/// The maximum number of Levels of Detail (nested) boxes
#define GPU_MAX_DDGI_CASCADES		(4)





struct GPU_DDGI
{
	/// The quaternion for rotating probe rays.
	//float4	random_orientation;

	/// xyz - the world-space offset (min. corner) of the probe grid;
	/// w - the distance between neighboring probes, in world-space
	float4	probe_grid_origin_and_spacing;
#ifndef __cplusplus
	float3	probe_grid_min_corner_WS()	{return probe_grid_origin_and_spacing.xyz;}
	float	probe_spacing()	{return probe_grid_origin_and_spacing.w;}
#endif

	/// x - the inverse of the distance between neighboring probes, in world-space
	/// yzw - unused
	float4	probe_grid_params0;
#ifndef __cplusplus
	float	inverse_probe_spacing()	{return probe_grid_params0.x;}
#endif


	/// x - resolution of the probe grid; yzw- unused
	uint4	probe_grid_res_params;
#ifndef __cplusplus
	uint probe_grid_resolution_uint()	{return probe_grid_res_params.x;}
	uint3 probe_grid_dimensions()	{return (uint3) probe_grid_res_params.x;}

	/// Snaps positions outside of grid to the grid edge
	int3 ClampCoordsToGridBounds(in int3 probe_coords)
	{
		return clamp(probe_coords
			, (int3) 0
			, (int3) (probe_grid_resolution_uint() - 1)
			);
	}
#endif


	/// x - the inverse of the width of the irradiance probe texture
	/// y - the inverse of the height of the irradiance probe texture
	/// z - the inverse of the width of the distance probe texture
	/// w - the inverse of the height of the distance probe texture
	float4	probe_texture_params;
#ifndef __cplusplus
	float inverse_irradiance_probe_texture_width()	{return probe_texture_params.x;}
	float inverse_irradiance_probe_texture_height()	{return probe_texture_params.y;}
	float inverse_distance_probe_texture_width()	{return probe_texture_params.z;}
	float inverse_distance_probe_texture_height()	{return probe_texture_params.w;}
#endif


	//


	// Maximum distance a probe ray may travel.
	float           probe_max_ray_distance;

    // Controls the influence of new rays when updating each probe. A value close to 1 will
    // very slowly change the probe textures, improving stability but reducing accuracy when objects
    // move in the scene. Values closer to 0.9 or lower will rapidly react to scene changes,
    // but will exhibit flickering.
    float           probe_hysteresis;

	// Exponent for depth testing. A high value will rapidly react to depth discontinuities, 
	// but risks causing banding.
	float           probe_distance_exponent;

    // Irradiance blending happens in post-tonemap space
    float           probe_irradiance_encoding_gamma;


	//
	

	// A threshold ratio used during probe radiance blending that determines if a large lighting change has happened.
	// If the max color component difference is larger than this threshold, the hysteresis will be reduced.
	float           probe_change_threshold;

	// A threshold value used during probe radiance blending that determines the maximum allowed difference in brightness
	// between the previous and current irradiance values. This prevents impulses from drastically changing a
	// texel's irradiance in a single update cycle.
	float           probe_brightness_threshold;

	// Bias values for Indirect Lighting
	float           view_bias;
	float           normal_bias;

	
	//


	//
	float4	debug_params;
#ifndef __cplusplus
	float	dbg_viz_light_probe_scale()	{return debug_params.x;}
#endif


	//
#ifdef __cplusplus
	mxOPTIMIZE("use max trace distance for better perf");
	//float	max_trace_distance;
#endif

	// we use the same ray directions for each probe
	float4	ray_directions[RAYS_PER_PROBE];	// 2048 B for 128 rays

#ifdef __cplusplus
	mxOPTIMIZE("precompute inverse ray dirs");
#endif

};


DECLARE_CBUFFER( G_DDGI, 6 )
{
	GPU_DDGI	g_ddgi;
};

//struct GPU_DDGI_Ray
//{
//	float4	org;
//	float4	dir;
//};

#include <Shared/__shader_interop_post.h>

#endif // DDGI_INTEROP_H
