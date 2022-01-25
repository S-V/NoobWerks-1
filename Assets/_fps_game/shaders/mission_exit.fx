/*

%%

pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS

	// using default render states
}

//

feature ENABLE_NORMAL_MAP
{
	default = 0
}

//

samplers
{
	s_diffuseMap = DiffuseMapSampler
	s_normalMap = NormalMapSampler
}

%%
*/

#define ENABLE_SPECULAR_MAP 0

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>

#include "_VS.h"	// DrawVertex
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>


Texture2D		t_diffuseMap: register(t0);
SamplerState	s_diffuseMap: register(s0);

Texture2D		t_normalMap: register(t1);
SamplerState	s_normalMap: register(s1);

// Texture2D		t_specularMap: register(t2);	// rgb - specular color
// SamplerState	s_specularMap: register(s2);

struct VS_Out
{
	float4 position : SV_Position;

	float2 texCoord : TexCoord0;

	float3 normal_in_view_space : Normal;
	float3 tangent_in_view_space : Tangent;
	float3 bitangent_in_view_space : Bitangent;
};

/*
==========================================================
	VERTEX SHADER
==========================================================
*/
void main_VS(
			 in DrawVertex vertex_in,	// idDrawVert
			 out VS_Out vertex_out_
			 )
{
	vertex_out_.position = Pos_Local_To_Clip( float4( vertex_in.position, 1 ) );
	
	//
	vertex_out_.texCoord = vertex_in.texCoord;

	//
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = -UnpackVertexNormal( vertex_in.tangent );
	
	vertex_out_.normal_in_view_space = Dir_Local_To_View( local_normal ).xyz;
	vertex_out_.tangent_in_view_space = Dir_Local_To_View( local_tangent ).xyz;
	vertex_out_.bitangent_in_view_space = cross(
		vertex_out_.tangent_in_view_space,
		vertex_out_.normal_in_view_space
		);
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

#else
			out float4 pixel_out_: SV_Target
#endif

			 )
{
	//
	const float4 sampled_diffuse_color = t_diffuseMap.Sample(
		s_diffuseMap,
		pixel_in.texCoord
	);


	//

#if ENABLE_NORMAL_MAP

	const float3 sampled_normal_in_tangent_space = ExpandNormal(
		t_normalMap.Sample(
			s_normalMap,
			pixel_in.texCoord
			).rgb
		);

	const float3 pixel_normal_in_view_space = normalize
		( pixel_in.tangent_in_view_space * sampled_normal_in_tangent_space.x
		+ pixel_in.bitangent_in_view_space * sampled_normal_in_tangent_space.y
		+ pixel_in.normal_in_view_space * sampled_normal_in_tangent_space.z
		);

#else

	const float3 pixel_normal_in_view_space = normalize(
		pixel_in.normal_in_view_space
		);

#endif

	//

#if ENABLE_SPECULAR_MAP

    const float3 sampled_specular_color = t_specularMap.Sample(
		s_specularMap,
		pixel_in.texCoord
	).rgb;

#else

	const float3 sampled_specular_color = float3(1,1,1);

#endif

	//

#if __PASS == FillGBuffer

	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = pixel_normal_in_view_space;
		surface.albedo = sampled_diffuse_color.rgb;

		surface.specular_color = sampled_specular_color;
		surface.specular_power = 10.0f;

		surface.metalness = PBR_metalness();
		surface.roughness = PBR_roughness();
	}
	GBuffer_Write( surface, pixel_out_ );

#else

	pixel_out_ = sampled_diffuse_color;

#endif

}
