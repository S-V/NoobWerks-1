/*

Updates irradiance and visibility probes with ray-traced samples
and blends with the previous irradiance and visibility probe data.

%%
pass DDGI_UpdateIrradianceProbes {
	compute = main_CS
}

%%

*/

#include "ddgi_interop.h"
#include "ddgi_common.hlsli"

// set to 1 to blend with previous irradiance/depths
#define BLEND_RESULTS_WITH_PREVIOUS	(1)


#define PROBE_DIM_TEXELS	(IRRADIANCE_PROBE_DIM)


// width == num rays per probe,
// height == num probes
// RGBA16F: xyz - raytraced radiance, w - ray hit distance
Texture2D< float4 >	t_src_raytraced_probe_radiance_and_distance;



/// we have to two textures for ping-ponging, because we cannot read from RWTexture3D< float3 > in Direct3D 11:
/// error X3676: typed UAV loads are only allowed
/// for single-component 32-bit element types
Texture2D< float3 >	t_prev_probe_irradiance;


// irradiance: R11G11B10F
// width = probe_counts.X * probe_counts.Y,
// height = probe_counts.Z.
RWTexture2D< float3 >	t_probe_irradiance;


[numthreads( PROBE_DIM_TEXELS, PROBE_DIM_TEXELS, 1 )]
void main_CS(
	in uint3 dispatch_thread_id: SV_DispatchThreadID
	, in uint3 thread_group_id: SV_GroupID
	)
{
	// Texel relative to the corner of the probe in the final texture.
	const uint2 probe_relative_texel_coord = uint2(
		dispatch_thread_id.x % PROBE_DIM_TEXELS,
		dispatch_thread_id.y % PROBE_DIM_TEXELS
	);

	// Get the probe ray direction associated with this thread.
	const float3 probe_ray_direction = OctDecodeNormal(
		DDGI_GetNormalizedOctahedralCoordinates(
			probe_relative_texel_coord,
			PROBE_DIM_TEXELS
		)
	);

	//
	const uint3 probe_coords = DDGI_GetProbeCoordsByAtlasDispatchThreadCoords(
		thread_group_id.xy
		, g_ddgi
	);

	const uint probe_index = FlattenIndex3D(
		probe_coords,
		g_ddgi.probe_grid_dimensions()
	);


	//
	float3 resulting_radiance = (float3)0;

	// used for normalizing the resulting_radiance
	float combined_weight = 0;

	// Blend radiance or distance values from each ray to compute irradiance or fitered distance.
	for(uint traced_ray_idx = 0; traced_ray_idx < RAYS_PER_PROBE; traced_ray_idx++)
	{
		const float3 traced_ray_direction = g_ddgi.ray_directions[ traced_ray_idx ].xyz;
		
        // Find the weight of the contribution for this ray
        // Weight is based on the cosine of the angle between the ray direction and the direction of the probe octant's texel
        const float weight = max(0.f, dot(probe_ray_direction, traced_ray_direction));
		
		// Load the ray traced radiance and hit distance.
		const int2 src_texel_coord = int2(traced_ray_idx, probe_index);
		const float4 raytraced_probe_radiance_and_distance = t_src_raytraced_probe_radiance_and_distance.mips[0][ src_texel_coord ];
		
		const float3 raytraced_probe_ray_radiance = raytraced_probe_radiance_and_distance.rgb;
		
		// Blend the ray's radiance
        resulting_radiance += raytraced_probe_ray_radiance * weight;

		combined_weight += weight;
	}//for each ray




	// Transform the thread dispatch index into probe texel coordinates.
	const uint2 probe_texel_coord
		= probe_relative_texel_coord.xy
		+ uint2(1, 1)	//  account for the 1 texel probe border
		+ (thread_group_id.xy * (1 + PROBE_DIM_TEXELS + 1))	// add offset within atlas
		;
	
	
	// Normalize the blended irradiance (or filtered distance), if the combined weight is not close to zero.
	// To match the Monte Carlo Estimator for Irradiance, we should divide by N. Instead, we are dividing by
	// N * sum(cos(theta)) (the sum of the weights) to reduce variance.
	// To account for this, we must mulitply in a factor of 1/2.
	const float epsilon = 1e-9f * RAYS_PER_PROBE;
	const float normalization_factor = 1.f / max(2.f * combined_weight, epsilon);
	

	resulting_radiance *= normalization_factor;
	



	// Tone-mapping gamma adjustment
	
#if DDGI_CONFIG_APPLY_IRRADIANCE_GAMMA
	// add abs(): error X3571: pow(f, e) will not work for negative f,
	// use abs(f) or conditionally handle negative values if you expect them
	resulting_radiance.rgb = pow(
		abs(resulting_radiance.rgb),
		g_ddgi.probe_irradiance_encoding_gamma
	);
#endif // DDGI_CONFIG_APPLY_IRRADIANCE_GAMMA



	//
#if BLEND_RESULTS_WITH_PREVIOUS

	const float3 previous = t_prev_probe_irradiance[ probe_texel_coord ];
	float  hysteresis = g_ddgi.probe_hysteresis;

	if(max3(previous.rgb - resulting_radiance.rgb) > g_ddgi.probe_change_threshold)
	{
		// Lower the hysteresis when a large lighting change is detected
		hysteresis = max(0.f, hysteresis - 0.75f);
	}
	
	float3 delta = (resulting_radiance.rgb - previous.rgb);
	if (length(delta) > g_ddgi.probe_brightness_threshold)
	{
		// Clamp the maximum change in irradiance when a large brightness change is detected
		resulting_radiance.rgb = previous.rgb + (delta * 0.25f);
	}

	// Interpolate the new blended irradiance with the existing irradiance in the probe.
	// A high hysteresis value emphasizes the existing probe irradiance.
	//
	// When using lower bit depth formats for irradiance, the difference between lerped values
	// may be smaller than what the texture format can represent. This can stop progress towards
	// the target value when going from high to low values. When darkening, step at least the minimum
	// value the texture format can represent to ensure the target value is reached. The threshold value
	// for 10-bit/channel formats is always used (even for 32-bit/channel formats) to speed up light to
	// dark convergence.
	static const float c_threshold = 1.f / 1024.f;
	float3 lerp_delta = (1.f - hysteresis) * delta;
	if (max3(resulting_radiance.rgb) < max3(previous.rgb))
	{
		lerp_delta = min(max(c_threshold, abs(lerp_delta)), abs(delta)) * sign(lerp_delta);
	}
	resulting_radiance = previous.rgb + lerp_delta;
	
	t_probe_irradiance[ probe_texel_coord ] = resulting_radiance;
		

#else // !BLEND_RESULTS_WITH_PREVIOUS


	t_probe_irradiance[ probe_texel_coord ] = resulting_radiance;


#endif // !BLEND_RESULTS_WITH_PREVIOUS

}
