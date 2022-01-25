/*
Visualizes the radiance volume texture.

%%
pass Unlit {
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	state = alpha_blended
}
%%
*/

#include "base.hlsli"
#include <Common/transform.hlsli>
#include "vxgi_interop.h"
#include "vxgi_common.hlsli"
// 
#include <Shared/nw_shared_globals.h>
#include "base.hlsli"

cbuffer Uniforms
{
	// depends on the mip level
	uint3				u_voxel_radiance_grid_size   : packoffset(c0);

	// 0 - the most detailed LoD (the smallest voxels)
	uint				u_mip_level_to_visualize     : packoffset(c0.w);

	//
	float3				u_color    					 : packoffset(c1);

	// projects from [0..1] grid space into screen space
	row_major float4x4	u_voxel_grid_to_screen_space : packoffset(c2);
};

// rgb - color, a - opacity
Texture3D< float4 >	t_voxel_radiance;

//=====================================================================

struct VS_to_GS
{
	uint3  voxel_coords : Position;
	float4 color		: Color;
};

VS_to_GS main_VS( in uint vertexID: SV_VertexID )
{
	const uint3 voxel_coords = UnravelIndex3D( vertexID, u_voxel_radiance_grid_size );

	VS_to_GS	vert_out;
	vert_out.voxel_coords = voxel_coords;
	vert_out.color = t_voxel_radiance.Load( uint4( voxel_coords, u_mip_level_to_visualize ) );
	return vert_out;
}

//=====================================================================

struct GS_to_PS
{
	float4 position : SV_Position;
	float4 color    : Color;
};

[maxvertexcount(14)]
void main_GS( in point VS_to_GS geom_in[1], inout TriangleStream<GS_to_PS> geom_out_stream )
{
	const float voxel_radiance_grid_inverse_resolution = float(1 << u_mip_level_to_visualize) * g_vxgi.inverseCascadeResolutionF();
	
	const VS_to_GS geom_in_vert = geom_in[0];
	const float density = geom_in_vert.color.a;
	
	[branch]
	if( density > 0 )	// if the voxel is non-empty:
	{
		// this position is in [0..1)
		const float3 cube_min_corner_in_grid_space = (float3)geom_in_vert.voxel_coords * voxel_radiance_grid_inverse_resolution;

		for( uint cube_vertex_idx = 0; cube_vertex_idx < 14; cube_vertex_idx++ )
		{
			const float3 cube_vertex_pos_01 = getCubeCoordForVertexId( cube_vertex_idx );
			const float3 cube_vertex_pos_in_grid_space = cube_min_corner_in_grid_space + cube_vertex_pos_01 * voxel_radiance_grid_inverse_resolution;

			GS_to_PS	element;
			element.position = mul( u_voxel_grid_to_screen_space, float4( cube_vertex_pos_in_grid_space, 1 ) );
			element.color.rgb = geom_in_vert.color.rgb * u_color;
			element.color.a = 0.4;

			geom_out_stream.Append(element);
		}
		geom_out_stream.RestartStrip();
	}
}

//=====================================================================

float4 main_PS( in GS_to_PS pixel_in ) : SV_Target
{
	return pixel_in.color;
}
