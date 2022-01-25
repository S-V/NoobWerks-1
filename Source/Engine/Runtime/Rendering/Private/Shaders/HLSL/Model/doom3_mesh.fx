/*
GPU-skinned mesh shader for rendering animated md5 models
from Doom 3 (idTech 4).

%%

pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS

	// using default render states
}

pass Translucent
{
	vertex = main_VS
	pixel = main_PS

	// additive color blending (e.g. for weapon muzzle flashes, monster eyes)
	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'None'
			flags = (
				'Enable_DepthClip'
			)
		}

		depth_stencil = {
			flags = (
				'Enable_DepthTest'
				; no depth writes
			)
		}

		; using SRC_COLOR and INV_SRC_COLOR for src and dst factors instead of ONE:
		; srccolor * one + destcolor * invsrccolor.
		; This scales down the dest by 1-src, then adds in the src. This keeps things from oversaturating, and can give a foggy feel.
		;
		blend = {
			flags = (
				'Enable_Blending'
			)
			color_channel = {
				operation = 'ADD'
				source_factor = 'SRC_COLOR'
				destination_factor = 'INV_SRC_COLOR'
			}
			alpha_channel = {
				operation = 'ADD'
				source_factor = 'ZERO'
				destination_factor = 'ZERO'
			}

			write_mask	= ('ENABLE_ALL')
		}
	}
}

//

feature ENABLE_NORMAL_MAP
{
	default = 0
}

feature ENABLE_SPECULAR_MAP
{
	default = 0
}

feature HAS_TEXTURE_TRANSFORM
{
	default = 0
}

feature IS_ALPHA_TESTED
{
	default = 0
}

//

samplers
{
	s_diffuseMap = DiffuseMapSampler
	s_normalMap = NormalMapSampler
	s_specularMap = SpecularMapSampler
}

%%
*/

// #ifndef DBG_VISUALIZE_TANGENTS
// #define DBG_VISUALIZE_TANGENTS (0)
// #endif

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include <Common/skinning.hlsli>

#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>


cbuffer Uniforms : register(b5)
{
	//0:32
	row_major float2x4	u_tex_coord_transform: packoffset(c0) = {
		// row 0, row 1
		1,        0,
		0,        1,
		0,        0,
		// the. w components are unused
		0,        0,
	};

	//32:12
	float3	u_color_rgb: packoffset(c2.x);

	// discard the pixel if sampled alpha is less than this value (0 = don't "clip" at all)
	//44:4
	float	u_alpha_threshold: packoffset(c2.w);

	//48
};

Texture2D		t_diffuseMap: register(t0);
SamplerState	s_diffuseMap: register(s0);

Texture2D		t_normalMap: register(t1);
SamplerState	s_normalMap: register(s1);

Texture2D		t_specularMap: register(t2);	// rgb - specular color
SamplerState	s_specularMap: register(s2);

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
			 in SkinnedVertex vertex_in,	// idDrawVert
			 out VS_Out vertex_out_
			 )
{
	// transform position by weighted sum of skinning matrices
	const row_major float4x4 bone_matrix = calculateBoneMatrix( vertex_in );
	
	// animated vertex position in object space:
	float3 animated_local_position;
	float3 animated_local_normal;
	float3 animated_local_tangent;

	transform_Skinned_Vertex(
		vertex_in
		, animated_local_position
		, animated_local_normal
		, animated_local_tangent
		);
	
	vertex_out_.position = Pos_Local_To_Clip( float4( animated_local_position, 1 ) );
	
	//
#if HAS_TEXTURE_TRANSFORM

	vertex_out_.texCoord = mul(
		(float2x3) u_tex_coord_transform
		, float3( vertex_in.texCoord, 1 )
	);

#else

	vertex_out_.texCoord = vertex_in.texCoord;

#endif

	//
	vertex_out_.normal_in_view_space = Dir_Local_To_View( animated_local_normal );
	vertex_out_.tangent_in_view_space = Dir_Local_To_View( animated_local_tangent );
	vertex_out_.bitangent_in_view_space = cross( vertex_out_.tangent_in_view_space, vertex_out_.normal_in_view_space );

	//vertex_out_.normal_in_view_space = normalize( vertex_out_.normal_in_view_space );
	//vertex_out_.tangent_in_view_space = normalize( vertex_out_.tangent_in_view_space );
	//vertex_out_.bitangent_in_view_space = normalize( vertex_out_.bitangent_in_view_space );
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
#if IS_ALPHA_TESTED

	clip( sampled_diffuse_color.a - u_alpha_threshold );

#endif


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
