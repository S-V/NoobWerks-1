/*
Effect used for rendering Red Faction 1 levels (*.rfl).

%%
pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS

	state = default
}

pass Deferred_Translucent
{
	vertex = main_VS
	pixel = main_PS

	state = alpha_blended
}

//

samplers
{
	s_diffuse = DiffuseMapSampler
	s_lightmap = DiffuseMapSampler
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>
#include <Common/Colors.hlsl>

#define RF_RENDERER_SUPPORT_LIGHTMAPS	(0)

/*
==========================================================
	SHADER PARAMETERS
==========================================================
*/

cbuffer Uniforms
{
	row_major float4x4	world_view_projection_matrix;
	row_major float4x4	world_view_matrix;
};

Texture2D		t_diffuse;
SamplerState	s_diffuse;

Texture2D		t_lightmap;
SamplerState	s_lightmap;

/*
==========================================================
	VERTEX SHADER
==========================================================
*/

// the same as AuxVertex
struct VertexLightmapped
{
	float3 position : Position;
	float2 basemap_UV : TexCoord0;
	float2 lightmap_UV : TexCoord1;
	float4 color : Color;	//unused
	uint4 normal : Normal;
	uint4 tangent : Tangent;	//unused
};

struct VS_Out
{
	float4 position : SV_Position;
	float2 basemap_UV : TexCoord0;
	float2 lightmap_UV: TexCoord1;
	float3 normal_in_view_space : Normal;
};

void main_VS(
			 in VertexLightmapped vertex_in,	// idDrawVert
			 out VS_Out vertex_out_
			 )
{
	float3 local_normal = UnpackVertexNormal(vertex_in.normal);

	vertex_out_.position = mul( world_view_projection_matrix, float4(vertex_in.position, 1) );
	vertex_out_.basemap_UV = vertex_in.basemap_UV;
	vertex_out_.lightmap_UV = vertex_in.lightmap_UV;
	vertex_out_.normal_in_view_space = mul( (float3x3)world_view_matrix, local_normal );
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
void main_PS(
			in VS_Out pixel_in,


#if __PASS == FillGBuffer

			out PS_to_GBuffer pixel_out_

#elif __PASS == Deferred_Translucent

			out float4 pixel_out_: SV_Target

#else
			out float3 pixel_out_: SV_Target
#endif

			 )
{

	float4 diffuse = t_diffuse.Sample( s_diffuse, pixel_in.basemap_UV.xy );


//#if RF_RENDERER_SUPPORT_LIGHTMAPS
//
//	float3 lightmap_color	= t_lightmap.Sample( s_lightmap, pixel_in.lightmap_UV.xy ).rgb;
//	float3 lit_color		= diffuse.rgb * lightmap_color;
//
//#else
//
//	float3 lit_color		= diffuse.rgb;
//
//#endif



#if __PASS == FillGBuffer

	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = normalize( pixel_in.normal_in_view_space );
		surface.albedo = diffuse.rgb;

		surface.specular_color = BLACK.rgb;
		surface.specular_power = 0;

		surface.metalness = 0.4;
		surface.roughness = 1;
	}
	GBuffer_Write( surface, pixel_out_ );

#elif __PASS == Deferred_Translucent

	pixel_out_ = diffuse;

#else

	pixel_out_ = lit_color;

#endif

}
