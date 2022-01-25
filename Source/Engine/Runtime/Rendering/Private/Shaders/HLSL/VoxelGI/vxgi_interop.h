// Voxel-based Global Illumination parameters
#ifdef __cplusplus
#pragma once
#endif

#ifndef VXGI_INTEROP_H
#define VXGI_INTEROP_H

#include <Shared/__shader_interop_pre.h>


/// The maximum number of Levels of Detail ((nested) boxes) for Global Illumination based on Cascaded Voxel Cone Tracing
#define GPU_MAX_VXGI_CASCADES		(4)


/// info about a single Voxel Grid Cascade
struct GPU_VXGI_Cascade
{
#ifdef __cplusplus
	mxOPTIMIZE("all cascades have the same resolution - move to g_vxgi");
#endif

	float4	voxel_radiance_grid_params;				// voxel grid resolution
#ifndef __cplusplus
	float voxel_radiance_grid_resolution()			{return (voxel_radiance_grid_params.x);}	// voxel grid resolution
	float voxel_radiance_grid_inverse_resolution()	{return (voxel_radiance_grid_params.y);}	// 1.0 / voxel grid resolution
	float voxel_size_world()						{return (voxel_radiance_grid_params.z);}	// voxel_size_in_world_space
	float inverse_voxel_size_world()				{return (voxel_radiance_grid_params.w);}	// 1.0 / voxel_size_in_world_space
#endif

	uint4	voxel_radiance_grid_integer_params;				// voxel grid resolution
#ifndef __cplusplus
	uint voxel_radiance_grid_resolution_uint()		{return (voxel_radiance_grid_integer_params.x);}
	uint voxel_radiance_grid_mip_levels_count()		{return (voxel_radiance_grid_integer_params.y);}
#endif

	float4	voxel_radiance_grid_offset_and_inverse_size_world;
#ifndef __cplusplus
	float3 min_corner_pos()						{return (voxel_radiance_grid_offset_and_inverse_size_world.xyz);}
	float  inverse_volume_side_length_world()	{return (voxel_radiance_grid_offset_and_inverse_size_world.w)  ;}
#endif

	// These are called during voxelization:
#ifndef __cplusplus
	/// converts from world-space space into the normalized [0..1] (UVW) space within the voxel cascade volume
	float3 TransformFromWorldToUvwTextureSpace( in float3 pos_in_world_space )
	{
		return (pos_in_world_space - min_corner_pos()) * inverse_volume_side_length_world();
	}

	float3 GetGridCoordsFromWorldPos( in float3 pos_in_world_space )
	{
		return (pos_in_world_space - min_corner_pos()) * inverse_voxel_size_world();
	}

	float3 GetWorldPosFromGridCoords( in int3 voxel_coords )
	{
		return min_corner_pos() + (float3)voxel_coords * voxel_size_world();
	}
#endif




/*
	float4	world_to_uvw_space_muladd;	// transforms from world to texture space

#ifndef __cplusplus
	float3 TransformToUvwTextureSpace( in float3 pos_in_world_space )
	{
		return pos_in_world_space * world_to_uvw_space_muladd.xyz
			+ (float3)world_to_uvw_space_muladd.w
			;
	}
#endif
*/

/*
	// The transform from the UVW-space (volume texture) of the finer cascade into this cascade's space
	float4	transform_to_this_cascade;

	// These are called during Voxel Cone Tracing:
#ifndef __cplusplus
	// transform the position in the UVW-space (volume texture) of the finer cascade
	// into the UVW-space of this cascade
	float3 transformPositionFromFinerCascade( in float3 positionInUvwTextureSpace )
	{
		return positionInUvwTextureSpace * transform_to_this_cascade.w + transform_to_this_cascade.xyz;
	}

	float transformLengthFromFinerCascade( in float distanceTravelled )
	{
		return distanceTravelled * transform_to_this_cascade.w;
	}
#endif
*/
};



struct GPU_VXGI
{
	GPU_VXGI_Cascade	cascades[GPU_MAX_VXGI_CASCADES];

	uint4	params_uint_0;
	float4	params_float_1;
	float4	params_float_2;

	uint cascadeCount()				{ return params_uint_0.x; }
	uint cascadeResolutionI()		{ return params_uint_0.y; }
	uint cascadeMipLevelsCountI()	{ return params_uint_0.z; }

	float cascadeResolutionF()			{ return params_float_1.x; }
	float inverseCascadeResolutionF()	{ return params_float_1.y; }
	float cascadeMipLevelsCountF()		{ return params_float_1.z; }

	float GI_indirectLightMultiplier()		{ return params_float_2.x; }
	float GI_ambientOcclusionScale()		{ return params_float_2.y; }
};


DECLARE_CBUFFER( G_VXGI, 5 )
{
	GPU_VXGI	g_vxgi;
};

#include <Shared/__shader_interop_post.h>

#endif // VXGI_INTEROP_H
