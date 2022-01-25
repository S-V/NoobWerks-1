/*

Updates irradiance and visibility probes with ray-traced samples
and blends with the previous irradiance and visibility probe data.

%%
pass DDGI_UpdateDistanceProbes {
	compute = main_CS
}

%%

*/

#include "ddgi_interop.h"
#include "ddgi_common.hlsli"



// set to 1 to blend with previous irradiance/depths
#define BLEND_RESULTS_WITH_PREVIOUS	(1)


#define PROBE_DIM_TEXELS	(DEPTH_PROBE_DIM)


// width == num rays per probe,
// height == num probes
// RGBA16F: xyz - raytraced radiance, w - ray hit distance
Texture2D< float4 >	t_src_raytraced_probe_radiance_and_distance;


Texture2D< float2 >	t_prev_probe_distance;

// width = probe_counts.X * probe_counts.Y,
// height = probe_counts.Z.
// distance and squared distance: RG16F
RWTexture2D< float2 >	t_probe_distance;


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
	float2 resulting_depths = (float2)0;

	// used for normalizing the result
	float combined_weight = 0;

	// Blend radiance or distance values from each ray to compute irradiance or fitered distance.
	for(uint traced_ray_idx = 0; traced_ray_idx < RAYS_PER_PROBE; traced_ray_idx++)
	{
		const float3 traced_ray_direction = g_ddgi.ray_directions[ traced_ray_idx ].xyz;
		
        // Find the weight of the contribution for this ray
        // Weight is based on the cosine of the angle between the ray direction and the direction of the probe octant's texel
        float weight = max(0.f, dot(probe_ray_direction, traced_ray_direction));
		
		// Load the ray traced radiance and hit distance.
		const int2 src_texel_coord = int2(traced_ray_idx, probe_index);
		const float4 raytraced_probe_radiance_and_distance = t_src_raytraced_probe_radiance_and_distance.mips[0][ src_texel_coord ];
		
		const float raytraced_probe_ray_distance = raytraced_probe_radiance_and_distance.w;
		
        // Increase or decrease the filtered distance value's "sharpness"
        weight = pow(weight, g_ddgi.probe_distance_exponent);
	
#if 0
		// Initialize the probe hit distance to three quarters of the distance of the grid cell diagonal
        float probe_max_ray_distance = length(g_ddgi.probe_spacing()) * 0.75f;

		// HitT is negative on backface hits for the probe relocation, so take the absolute value
		const float probe_ray_distance = min(
			abs(raytraced_probe_ray_distance),
			g_ddgi.probe_max_ray_distance
		);
#else
		const float probe_ray_distance = raytraced_probe_ray_distance;
#endif
		
		// Filter the ray distance
		resulting_depths += float2(
			probe_ray_distance * weight,
			(probe_ray_distance * probe_ray_distance) * weight
		);

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
	
	resulting_depths *= normalization_factor;
	

	//
#if BLEND_RESULTS_WITH_PREVIOUS

	const float2 previous = t_prev_probe_distance[ probe_texel_coord ].rg;
	const float  hysteresis = g_ddgi.probe_hysteresis;

	// Interpolate the new filtered distance with the existing filtered distance in the probe.
	// A high hysteresis value emphasizes the existing probe filtered distance.
	resulting_depths = lerp(resulting_depths.rg, previous.rg, hysteresis);
	
	t_probe_distance[ probe_texel_coord ] = resulting_depths;

#else // !BLEND_RESULTS_WITH_PREVIOUS


	#if 0
		t_probe_distance[ probe_texel_coord ] = resulting_depths.xx
					* 0.01f	// debug viz
					;
		return;
	#endif


	t_probe_distance[ probe_texel_coord ] = resulting_depths;

#endif // !BLEND_RESULTS_WITH_PREVIOUS

}
