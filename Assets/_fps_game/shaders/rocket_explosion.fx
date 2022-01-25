/*

A simple sprite-based explosion.

%%

// NOTE: draw before smoke trails
//pass VolumetricParticles
pass Deferred_Translucent
{
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'None'
			flags = ()
		}

		depth_stencil = 	{
			flags = (
				'Enable_DepthTest'
				; no depth writes
			)
			; Reverse Z
			comparison_function = 'Greater_Equal'
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
			|
			color_channel = {
				operation = 'ADD'
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
			}
			alpha_channel = {
				operation = 'ADD'
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
			}
			|
			write_mask	= ('ENABLE_ALL')
		}
	}
}

samplers
{
	s_sprite = DiffuseMapSampler
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/Colors.hlsl>
#include <Common/transform.hlsli>
#include <Utility/Rotate.hlsli>
#include "_basis.h"	// buildOrthonormalBasisFromNormal()
#include "_PS.h"
#include "_fog.h"
#include "_noise.h"
#include <GBuffer/nw_gbuffer_read.h>	// DepthTexture
#include "Noise/SimplexNoise3D.hlsli"

/*
==========================================================
	DEFINES
==========================================================
*/

/// Soft particles (aka Z-feather) blend the edges of particles so they reduce clipping against geo.

/// hard edges
#define	SOFT_PARTICLES_QUALITY_NONE		(0)

/// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]
#define	SOFT_PARTICLES_QUALITY_CHEAP	(1)

///
#define	SOFT_PARTICLES_QUALITY			(SOFT_PARTICLES_QUALITY_CHEAP)


/*
==========================================================
	SHADER PARAMETERS
==========================================================
*/
Texture2D		t_sprite;
SamplerState	s_sprite;


/*
==========================================================
	VERTEX SHADER
==========================================================
*/

struct ParticleVertex
{
	float4	center_and_radius: Position;

	// x - rotation in [0..1]
	// y - opacity
	// w: life in [0..1], if life > 1 => particle is dead
	float4	rotation_opacity_life01: TexCoord;
};

struct VS_to_GS
{
    float3 Pos : POSITION;
    float life : LIFE;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float size : SIZE;

	float rot_angle: RotationAngleInRadians;
	float opacity: Opacity;
};

VS_to_GS main_VS(ParticleVertex input)
{
    VS_to_GS output;
    
    output.Pos = input.center_and_radius.xyz;
    output.life = input.rotation_opacity_life01.w;
    output.size = input.center_and_radius.w;

	output.rot_angle	= input.rotation_opacity_life01.x;
	output.opacity		= input.rotation_opacity_life01.y;
    
    return output;
}

/*
==========================================================
	GEOMETRY SHADER
==========================================================
*/

struct GS_to_PS
{
	float4 posH : SV_Position;
	float  size	: Radius;

	float1 life : LIFE;
	float opacity: Opacity;
	
	float2 uv: TexCoord;
	
	// view-space depth of this particle; used by soft particles
	float1 view_space_depth : TexCoord8;
};

[maxvertexcount(4)]
void main_GS(
	point VS_to_GS input[1]
	, inout TriangleStream< GS_to_PS > SpriteStream
	)
{
    GS_to_PS output;

	//
    const float3 particle_origin_WS = input[0].Pos;

	//
	output.size = input[0].size;

	output.life = input[0].life;
	
	output.opacity = input[0].opacity;

	//
	const float radius = input[0].size;

	const float3 local_up		= g_camera_up_WS.xyz * radius;
	const float3 local_right	= g_camera_right_WS.xyz * radius;
	
	//
	const float3x3 rot_matrix = CreateRotationMatrix_AngleAxis(
		g_camera_fwd_WS.xyz,
		input[0].rot_angle
	);

	const float3 local_up_rotated		= normalize(mul(rot_matrix, local_up)) * radius;
	const float3 local_right_rotated	= normalize(mul(rot_matrix, local_right)) * radius;
	
	const float2 top_left_UV = float2(0,0);
	const float2 top_right_UV = float2(1,0);
	const float2 bottom_left_UV = float2(0,1);
	const float2 bottom_right_UV = float2(1,1);
	
	//
	const float3 top_left_WS = particle_origin_WS - local_right_rotated + local_up_rotated;
	output.posH = mul( g_view_projection_matrix, float4(top_left_WS,1) );

	output.uv = top_left_UV;

	output.view_space_depth = mul( g_view_matrix, float4(top_left_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 top_right_WS = particle_origin_WS + local_right_rotated + local_up_rotated;
	output.posH = mul( g_view_projection_matrix, float4(top_right_WS,1) );
	
	output.uv = top_right_UV;
	
	output.view_space_depth = mul( g_view_matrix, float4(top_right_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 bottom_left_WS = particle_origin_WS - local_right_rotated - local_up_rotated;
	output.posH = mul( g_view_projection_matrix, float4(bottom_left_WS,1) );

	output.uv = bottom_left_UV;
	
	output.view_space_depth = mul( g_view_matrix, float4(bottom_left_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 bottom_right_WS = particle_origin_WS + local_right_rotated - local_up_rotated;
	output.posH = mul( g_view_projection_matrix, float4(bottom_right_WS,1) );

	output.uv = bottom_right_UV;

	output.view_space_depth = mul( g_view_matrix, float4(bottom_right_WS,1) ).y;
	SpriteStream.Append(output);

	//
	SpriteStream.RestartStrip();
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/

float4 main_PS( GS_to_PS pixel_in ) : SV_Target
{
	const float sphere_radius = pixel_in.size;
 
	// We need to compare the distance stored in the depth buffer
	// and the value that came in from the GS.
	// Because the depth values aren't linear, we need to transform the depthsample back into view space
	// in order for the comparison to give us an accurate distance.

	const float hardware_depth = DepthTexture.Load( int3( pixel_in.posH.x, pixel_in.posH.y, 0 ) );

	const float view_space_depth_from_zbuffer = hardwareZToViewSpaceDepth( hardware_depth );
	const float view_space_depth_from_particle = pixel_in.view_space_depth;

	//

#if SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_NONE

	const float depth_fade = 1;

#else	// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]

	const float depth_diff = view_space_depth_from_zbuffer - view_space_depth_from_particle;
	if( depth_diff < 0 ) {
		discard;
	}

	const float depth_fade_distance = sphere_radius * 0.5f;	// 0 = hard edges
	const float depth_fade = saturate( depth_diff / depth_fade_distance );
	
#endif

	//
	const float4 sampled_color = t_sprite.Sample(
		s_sprite,
		pixel_in.uv
	);

	//
	const float alpha = pixel_in.opacity * depth_fade;

	//
	return float4(
		sampled_color.rgb * 2 * alpha	// increase brightness
		, sampled_color.a * alpha
		);
}
