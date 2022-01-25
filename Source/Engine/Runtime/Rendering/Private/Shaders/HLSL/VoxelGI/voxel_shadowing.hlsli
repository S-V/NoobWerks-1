// 
#ifndef VOXEL_SHADOWING_HLSLI
#define VOXEL_SHADOWING_HLSLI


#include <VoxelGI/vxgi_interop.h>
#include <VoxelGI/vxgi_common.hlsli>
#include <VoxelGI/voxel_raymarching.hlsli>
#include <VoxelGI/voxel_cone_tracing.hlsli>

/**
 Computes the ambient occlusion at the given surface position using voxel cone
 tracing.
 @pre			@a n_uvw is normalized.
 @param[in]		p_uvw
				The surface position expressed in voxel UVW space.
 @param[in]		n_uvw
				The surface normal expressed in voxel UVW space.
 @param[in]		max_cone_distance
				The maximum cone distance expressed in voxel UVW space.
 @param[in]		config
				The VCT configuration.
 @return		The ambient occlusion at the given surface position using voxel
				cone tracing.
 */
float VCT_GetSunLightOcclusionFromPoint(
	in float3 pos_WS
	, in float3 direction_to_light_source
	
	, in Texture3D< float4 > t_radiance_opacity_volume
	)
{
	//
	const GPU_VXGI_Cascade	vxgi_cascade0 = g_vxgi.cascades[0];
	
	const float3 ray_origin_in_voxel_uvw_space = vxgi_cascade0.TransformFromWorldToUvwTextureSpace(pos_WS);
	

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
		, direction_to_light_source

		, voxel_grid_config

		, hit_voxel_coords
		, did_hit_sky
	);

	return did_hit_sky ? 0 : 1;
}




/**
 Computes the (incoming) radiance and ambient occlusion of the given cone
 for this VCT configuration.
 @param[in]		cone
				The cone.
 @param[in]		max_cone_distances
				The maximum cone distance along the direction of the given
				cone expressed in voxel UVW space for the (incoming)
				radiance and ambient occlusion.
 @return		The (incoming) radiance and ambient occlusion of the given
				cone for this VCT configuration.
 */
float4 VCT_ComputeRadianceAndAO(
	in VCT_ConeConfig cone
	, in VCT_TraceConfig vct_cfg
	
	, in bool apply_start_offset

	, out float distance_travelled_
	, out bool hit_the_sky_
	)
{
	float3 color = 0.0f;
	float  alpha = 0.0f;	// 'opacity'/transmittance/density accumulator

	// Compute the initial distance to avoid sampling the voxel containing
	// the cone's apex.
	float distance_travelled = apply_start_offset
		? vct_cfg.GetStartOffsetDistance()
		: 0
		;

	while( distance_travelled < vct_cfg.max_cone_distance && alpha < 1.0f )
	{
		// Compute the position (expressed in voxel UVW space).
		const float3 pos_UVW = cone.GetPosition( distance_travelled );
		// Compute the cone diameter.
		const float diameter = vct_cfg.GetDiameter( cone, distance_travelled );
		// Compute the MIP level.
		const float mip_level = vct_cfg.GetMipLevel( diameter );

		distance_travelled_ = distance_travelled;

		// break if the ray exits the voxel grid, or we sample from the last mip:
		[branch]
		if(
			Is3DTextureCoordOutsideVolume( pos_UVW )
			||
			mip_level > vct_cfg.num_mip_levels
			)
		{
			hit_the_sky_ = true;
			break;
		}

		// Sample the radiance and occlusion.
		const float4 sample = vct_cfg.radiance_opacity_texture.SampleLevel(
			vct_cfg.trilinear_sampler,
			pos_UVW,
			mip_level
		);

		const float weight = (1.0f - alpha) * sample.a * vct_cfg.cone_step;
		color += weight * sample.rgb;
		alpha += weight;

		// Update the marching distance.
		distance_travelled += diameter * vct_cfg.cone_step;
	}

	hit_the_sky_ = false;
	return float4( color, alpha );
}




float VCT_GetSoftSunLightOcclusionFromPoint(
	in float3 pos_WS
	, in float3 direction_to_light_source
	, in Texture3D< float4 > t_radiance_opacity_volume
	, in SamplerState s_trilinear
	)
{
	const GPU_VXGI_Cascade	vxgi_cascade0 = g_vxgi.cascades[0];
	
	const float3 ray_origin_in_voxel_uvw_space = vxgi_cascade0.TransformFromWorldToUvwTextureSpace(pos_WS);


	// Construct a cone.
	VCT_ConeConfig	vct_cone_config;
	vct_cone_config.apex = ray_origin_in_voxel_uvw_space;
	vct_cone_config.direction = direction_to_light_source;
	vct_cone_config.two_by_tan_half_aperture = tan(M_PI / 6);
	
	
	//
	VCT_TraceConfig	vct_trace_config;

	vct_trace_config.grid_resolution			= g_vxgi.cascadeResolutionF();
	vct_trace_config.inverse_grid_resolution	= g_vxgi.inverseCascadeResolutionF();
	vct_trace_config.num_mip_levels				= g_vxgi.cascadeMipLevelsCountF();
	vct_trace_config.cone_step					= 1;
	vct_trace_config.max_cone_distance			= 1e27;
	//
	vct_trace_config.radiance_opacity_texture = t_radiance_opacity_volume;
	vct_trace_config.trilinear_sampler = s_trilinear;

	//
	float distance_travelled;
	bool	hit_the_sky;
	const float4 color_ao = VCT_ComputeRadianceAndAO(
		vct_cone_config
		, vct_trace_config
		, true

		, distance_travelled
		, hit_the_sky
	);

	return color_ao.a;
}

#endif // VOXEL_SHADOWING_HLSLI
