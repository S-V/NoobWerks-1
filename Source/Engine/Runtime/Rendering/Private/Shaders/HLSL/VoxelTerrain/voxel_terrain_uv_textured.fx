// For rendering voxel terrains with Discrete Level-Of-Detail ("stitching").

/*
%%
pass FillGBuffer {
	vertex = VS_Deferred
	pixel = PS_Deferred
	state = default
}

pass DebugWireframe {
	vertex = VS_Wireframe
	geometry = GSSolidWire
	pixel = PSSolidWireFade
	state = solid_wireframe
}

samplers {
	s_diffuseMap = DiffuseMapSampler
}

%%

//TODO:
resources = [
	{
		name = 't_diffuseMaps'
		asset = 'voxel_terrain_dmaps'
	}
	{
		name = 't_normalMaps'
		asset = 'voxel_terrain_nmaps'
	}
]

*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>
#include "Debug/solid_wireframe.h"


Texture2D		t_diffuseMap: register(t5);
SamplerState	s_diffuseMap: register(s5);


struct VS_Out
{
	float4	position : SV_Position;
	float3	normalVS : Normal0;	// view-space normal
	float2	texCoord : TexCoord0;
//	float4	color	 : Color;
};

VS_Out VS_Deferred( in DrawVertex vs_in )
{
	VS_Out	vs_out;
	vs_out.position = Pos_Local_To_Clip( float4( vs_in.position, 1 ) );
	vs_out.normalVS = Dir_Local_To_View( UnpackVertexNormal( vs_in.normal ) );
	vs_out.texCoord	= vs_in.texCoord;
//	vs_out.color = vs_in.color;

	return vs_out;
}

PS_to_GBuffer PS_Deferred( in VS_Out ps_in )
{
	const float3 sampled_diffuse_color = t_diffuseMap.Sample(
		s_diffuseMap,
		ps_in.texCoord
	).rgb;

	//
	float3 viewSpaceNormal = normalize( ps_in.normalVS );

	//
	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = normalize(viewSpaceNormal);
		surface.albedo = sampled_diffuse_color.rgb;
		surface.metalness = 0.1;
		surface.roughness = 0.9;
	}

	//
	PS_to_GBuffer ps_out;
	GBuffer_Write( surface, ps_out );
	return ps_out;
}

/*
==============================================================================
	WIREFRAME RENDERING
==============================================================================
*/
GS_INPUT VS_Wireframe( in DrawVertex vs_in )
{
	const float4 local_position = float4( vs_in.position, 1 );

	//
	GS_INPUT	vs_out;
	vs_out.Pos = Pos_Local_To_Clip( local_position );
	vs_out.PosV = Pos_Local_To_View( local_position );
	return vs_out;
}
