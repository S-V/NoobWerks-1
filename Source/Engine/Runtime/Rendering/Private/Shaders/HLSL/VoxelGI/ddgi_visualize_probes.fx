/*
Visualizes the irradiance probe grid.

%%
pass DDGI_VisualizeProbes {
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	// using default render states
	//state = default
}

samplers {
	s_bilinear = BilinearSampler
	s_point = PointSampler
}
%%
*/

#include "ddgi_interop.h"
#include "ddgi_common.hlsli"
#include <Common/transform.hlsli>


Texture2D< float3 >	t_probe_irradiance;
Texture2D< float2 >	t_probe_distance;
SamplerState	s_bilinear;
SamplerState	s_point;


struct VS_to_GS
{
	nointerpolation uint3  probe_coords : ProbeCoords;
	nointerpolation float3 probe_center_WS : Position;
};

VS_to_GS main_VS( in uint vertexID: SV_VertexID )
{
	const uint3 probe_coords = DDGI_GetProbeCoordsByIndex(
		vertexID,
		g_ddgi
	);

	//
	VS_to_GS	vert_out;

	vert_out.probe_coords = probe_coords;
	vert_out.probe_center_WS = DDGI_GetProbePosition(probe_coords, g_ddgi);

	return vert_out;
}

//=====================================================================

struct GS_to_PS
{
	float4 position_CS : SV_Position;
	float3 position_WS : Position0;

	nointerpolation uint3  probe_coords : ProbeCoords;
	nointerpolation float3 probe_center_WS : TexCoord;
};

[maxvertexcount(14)]
void main_GS( in point VS_to_GS geom_in[1], inout TriangleStream<GS_to_PS> geom_out_stream )
{
	const VS_to_GS geom_in_vert = geom_in[0];

	const float cube_size_scale = g_ddgi.dbg_viz_light_probe_scale();

	for( uint cube_vertex_idx = 0; cube_vertex_idx < 14; cube_vertex_idx++ )
	{
		const float3 cube_vertex_pos_01 = getCubeCoordForVertexId( cube_vertex_idx );
		const float3 cube_vertex_pos_min1plus1 = cube_vertex_pos_01 * 2 - 1;

		const float3 cube_vertex_pos_WS
			= geom_in_vert.probe_center_WS + cube_vertex_pos_min1plus1 * cube_size_scale
			;
		//
		GS_to_PS	gs_to_ps;
		
		gs_to_ps.position_CS = Transform_Position_World_To_Clip(
			float4(cube_vertex_pos_WS, 1)
		);
		gs_to_ps.position_WS = cube_vertex_pos_WS;
		gs_to_ps.probe_coords = geom_in_vert.probe_coords;
		gs_to_ps.probe_center_WS = geom_in_vert.probe_center_WS;

		geom_out_stream.Append(gs_to_ps);
	}
	geom_out_stream.RestartStrip();
}

//=====================================================================

float3 main_PS( in GS_to_PS pixel_in ) : SV_Target
{
	// face normal in eye-space:
	// '-' because we use right-handed coordinate system
	const float3 normal_WS = normalize(
		-cross(
			ddx(pixel_in.position_WS),
			ddy(pixel_in.position_WS)
		)
	);


	const float3 surface_bias = DDGI_GetSurfaceBias(
		normal_WS
		, g_camera_fwd_WS.xyz
		, g_ddgi
	);


	//
	DDGI_VolumeResources	ddgi_resources;
	ddgi_resources.probe_irradianceSRV = t_probe_irradiance;
	ddgi_resources.probeDistanceSRV = t_probe_distance;
    ddgi_resources.bilinearSampler = s_bilinear;
	

#if 1


	const float2 irradiance_octa_coords = OctEncodeNormal(
		normal_WS
	);

	const float2 irradiance_probe_texture_coords = DDGI_GetIrradianceProbeUV(
		pixel_in.probe_coords
		, irradiance_octa_coords
		, g_ddgi
		);
	const float3 probe_irradiance = ddgi_resources.probe_irradianceSRV.SampleLevel(
		ddgi_resources.bilinearSampler
		, irradiance_probe_texture_coords
		, 0
	).rgb;
	
	return probe_irradiance;


#else


	const float3 irradiance = DDGI_SampleVolumeIrradiance(
		pixel_in.position_WS // in float3 world_position
		, normal_WS // in float3 direction
		, surface_bias // in float3 surface_bias
		, g_ddgi
		, ddgi_resources
	);
//return normal_WS;

	return irradiance;


#endif

}
