// 
#ifndef VOXEL_CONE_TRACING_HLSLI
#define VOXEL_CONE_TRACING_HLSLI

#include <Shared/nw_shared_definitions.h>	// GPU_MAX_VXGI_CASCADES
#include <base.hlsli>	// M_PI
#include <_basis.h>


// based on Godot's GI probe code (OpenGL ES 3 / "scene.glsl", gi_probes_compute())
// and Matthias Moulin's MAGE: https://github.com/matt77hias/MAGE/blob/master/MAGE/Shaders/shaders/vct.hlsli
// https://matt77hias.github.io/blog/2018/08/19/voxel-cone-tracing.html

struct VCT_ConeConfig
{
	/// The apex of the cone.
	float3	apex;

	/// The (normalized) direction of the cone.
	float3	direction;

	/// The tangent of the half aperture angle of the cone, multiplied by two.
	float	two_by_tan_half_aperture;

	/**
	 Computes the position at the given distance along the direction of this
	 cone.
	 @param[in]		distance
					The distance along the direction of this cone.
	 @return		The position at the given distance along the direction of
					this cone.
	 */
	float3 GetPosition( in float distance_along_direction )
	{ return apex + distance_along_direction * direction; }
};

//=====================================================================



/// Voxel Cone Tracing Config
struct VCT_TraceConfig
{
	/// The resolution of the voxel grid for all dimensions
	float	grid_resolution;

	/// The inverse resolution of the voxel grid for all dimensions
	float	inverse_grid_resolution;

	/// The maximum mip level of the voxel texture.
	float	num_mip_levels;
	
	/// The cone step expressed in voxel UVW space.
	float	cone_step;

	/// The maximal cone distance expressed in voxel UVW space.
	float	max_cone_distance;

	//
	Texture3D< float4 >	radiance_opacity_texture;
	SamplerState		trilinear_sampler;

	//-------------------------------------------------------------------------
	// Member Methods
	//-------------------------------------------------------------------------

	/**
	 Computes the offset distance expressed in voxel
	 UVW space.
	 @return		The offset distance expressed in
					voxel UVW space.
	 */
	float GetStartOffsetDistance()
	{
		// The initial distance is equal to the voxel diagonal expressed in
		// voxel UVW space (= sqrt(2)/grid_resolution) to avoid sampling the
		// voxel containing a cone's apex or ray's origin.
		return nwSQRT2 * inverse_grid_resolution;
	}

	/**
	 Computes the diameter associated with the given
	 cone and distance along the direction of the cone.
	 @param[in]		cone
					The cone.
	 @param[in]		distance
					The distance along the direction of this cone expressed in
					voxel UVW space.
	 @return		The diameter associated with the
					given cone and distance along the direction of the cone.
	 */
	float GetDiameter( in VCT_ConeConfig cone, in float distance_travelled )
	{
		// Obtain the diameter (expressed in voxel UVW space).
		//
		//                            diameter/2
		// two_by_tan_half_aperture = ---------- <=> diameter = 2 * two_by_tan_half_aperture distance
		//                             distance
		//
		// which is in [inverse_grid_resolution,inf]
		return max( inverse_grid_resolution,
				    cone.two_by_tan_half_aperture * distance_travelled );
	}

	/**
	 Computes the MIP level associated with the given cone diameter.
	 @param[in]		diameter
					The cone diameter expressed in voxel UVW space.
	 @return		The MIP level associated with the
					given cone diameter.
	 */
	float GetMipLevel( in float diameter )
	{
		// Obtain the MIP level in [1,inf]
		return log2( diameter * grid_resolution );	// == log2( diameter / voxel_size_at_mip_0 )
	}
};

#endif // VOXEL_CONE_TRACING_HLSLI
