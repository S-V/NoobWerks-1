// 
#ifndef VOXEL_RAYMARCHING_HLSLI
#define VOXEL_RAYMARCHING_HLSLI

struct VoxelGridConfig
{
	float grid_resolution;
	float inverse_grid_resolution;
	
	// rgb - radiance; w - opacity/density
	Texture3D< float4 >	radiance_opacity_volume;
};
 
float4 RaymarchThroughVoxelGrid(
	// ray origin in voxel texture space
	in float3 ray_origin_uvw
	, in float3 ray_direction

	, in VoxelGridConfig config

	, out int3 hit_voxel_coords_
	, out bool did_hit_sky_
)
{
	// 1. Initialization phase.

	const float3 dir_abs = abs(ray_direction);

	// for determining the fraction of the cell size the ray travels in each step:
	// const float3 inv_dir_abs = float3(
		// (dir_abs.x != 0) ? (1.0f / dir_abs.x) : 1.0f,
		// (dir_abs.y != 0) ? (1.0f / dir_abs.y) : 1.0f,
		// (dir_abs.z != 0) ? (1.0f / dir_abs.z) : 1.0f
		// );
	const float3 inv_dir_abs = float3(
		(dir_abs.x > 1e-5f) ? (1.0f / (dir_abs.x + 1e-9f)) : 1.0f,
		(dir_abs.y > 1e-5f) ? (1.0f / (dir_abs.y + 1e-9f)) : 1.0f,
		(dir_abs.z > 1e-5f) ? (1.0f / (dir_abs.z + 1e-9f)) : 1.0f
		);
		
	// 1.1. Identify the voxel containing the ray origin.
	// these are integer coordinates:
	const int3 start_voxel_icoords = (int3) floor( ray_origin_uvw * config.grid_resolution );

	// 1.2. Identify which coordinates are incremented/decremented as the ray crosses voxel boundaries.
	const int3 ray_step_i = sign( ray_direction );

	// 1.3. Determine the values of t at which the ray crosses the first voxel boundary along each dimension.

	const float3 mins_uvw = (float3) start_voxel_icoords * config.inverse_grid_resolution;
	const float3 maxs_uvw = mins_uvw + config.inverse_grid_resolution;
	const float3 start_next_t_uvw = ( ( ray_step_i >= 0 )
								? ( maxs_uvw - ray_origin_uvw )
								: ( ray_origin_uvw - maxs_uvw ) ) * inv_dir_abs;

	// 1.4. Compute how far we have to travel in the given direction (in units of t)
	// to reach the next voxel boundary (with each movement equal to the size of a cell) along each dimension.

	/// The t value which moves us from one voxel to the next
	const float3 delta_t_uvw = config.inverse_grid_resolution * inv_dir_abs;

	// 2. Traversal phase is minimal.
	// ...

	int3 curr_voxel_icoords = start_voxel_icoords;
	float3 next_t_uvw = start_next_t_uvw;


	//
	float3 color = 0;
	float  alpha = 0;	// 'opacity'/transmittance/density accumulator

	// Maximum number of steps to take when raymarching.
	const int MAX_RAY_STEPS = 256;

	int step_counter = 0;

	while( step_counter < MAX_RAY_STEPS && alpha < 1.0f )
	{
		// break if the ray exits the voxel grid
		[branch]
		if(
			//any( pos - saturate( pos ) )
			any( curr_voxel_icoords < 0 ) || any( curr_voxel_icoords >= config.grid_resolution )
			)
		{
			did_hit_sky_ = true;
//color += float3(0,1,0);//ZZZ
			//color += computeAmbientLight( ray_direction );
			break;
		}

		// Sample the radiance and occlusion.
		const float4 sample = config.radiance_opacity_volume.mips[0][ curr_voxel_icoords ];

		// front-to-back blending
		const float weight = (1.0f - alpha) * sample.a;
		color += weight * sample.rgb;
		alpha += weight;//ZZZ

		// Find the minimum of tMaxX, tMaxY and tMaxZ during each iteration.
		// This minimum will indicate how much we can travel along the ray and still remain in the current voxel.
//OPTIMIZE: on AMD you could go for https://www.khronos.org/registry/OpenGL/extensions/AMD/AMD_shader_trinary_minmax.txt 
//const float3 mask = next_t_uvw == min3( next_t_uvw.x, next_t_uvw.y, next_t_uvw.z ); // it's a single op on AMD
		const float3 mask = ( next_t_uvw <= min( next_t_uvw.yzx, next_t_uvw.zxy ) );

		// Update the current voxel coords.
		next_t_uvw += mask * delta_t_uvw;
		curr_voxel_icoords += mask * ray_step_i;

		++step_counter;
	}

	hit_voxel_coords_ = curr_voxel_icoords;
	did_hit_sky_ = false;
//color += float3(1,0,0);
	return float4( color, alpha );
}

#endif // VOXEL_RAYMARCHING_HLSLI
