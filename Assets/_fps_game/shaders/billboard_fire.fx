/*

A simple sprite-based fire.

%%

// NOTE: draw before smoke trails
//pass VolumetricParticles
pass Deferred_Translucent
{
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	state = volumetric_particles
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
Texture2D		t_sprite;	//atlas
SamplerState	s_sprite;

static const int TEX_ATLAS_RES_Y = 4;
static const int TEX_ATLAS_RES_X = 4;
static const int TEX_ATLAS_RES = TEX_ATLAS_RES_Y * TEX_ATLAS_RES_X;

static const float2 TEX_ATLAS_TILE_SIZE = float2(
	(float)1 / (float)TEX_ATLAS_RES_X,
	(float)1 / (float)TEX_ATLAS_RES_Y
);

float2 CalcTexTile(in float life01)
{
	const float offset = floor( TEX_ATLAS_RES * life01 );

	// split into horizontal and vertical index
	const float row = floor( offset / TEX_ATLAS_RES_X );
	const float col = floor( offset - row * TEX_ATLAS_RES_X );

	return float2(col, row);
}

float2 CalcTexTile2(in float offset_with_frac)
{
	const float flat_index = floor( offset_with_frac );

	// split into horizontal and vertical index
	const float row = floor( flat_index / TEX_ATLAS_RES_X );
	const float col = floor( flat_index - row * TEX_ATLAS_RES_X );

	return float2(col, row);
}

struct SpriteBlend
{
	float2 uv0;
	float2 uv1;
	float lerp_alpha;
};

SpriteBlend CalcSpriteBlend(in float life01)
{
	float curr_offset_with_frac = (float)TEX_ATLAS_RES * life01;

	float next_offset_with_frac = curr_offset_with_frac + 1.0f;
	if( next_offset_with_frac >= TEX_ATLAS_RES ) {
		next_offset_with_frac -= TEX_ATLAS_RES;
	}

	SpriteBlend	sprite_blend;
	sprite_blend.uv0 = CalcTexTile2( curr_offset_with_frac );
	sprite_blend.uv1 = CalcTexTile2( next_offset_with_frac );
	sprite_blend.lerp_alpha = frac(next_offset_with_frac);
	return sprite_blend;
}


/*
==========================================================
	VERTEX SHADER
==========================================================
*/

struct ParticleVertex
{
	float4	center_and_radius: Position;

	// w: life in [0..1], if life > 1 => particle is dead
	float4	color_and_life01: TexCoord;
};

struct VS_to_GS
{
    float3 Pos : POSITION;
    float life : LIFE;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float size : SIZE;
	float3 color: Color;
};

VS_to_GS main_VS(ParticleVertex input)
{
    VS_to_GS output;
    
    output.Pos = input.center_and_radius.xyz;
    output.life = input.color_and_life01.w;
    output.size = input.center_and_radius.w;
	output.color = input.color_and_life01.rgb;
    
    return output;
}

/*
==========================================================
	GEOMETRY SHADER
	Geometry shader for creating point sprites
==========================================================
*/

struct GS_to_PS
{
	float4 posH : SV_Position;
	float  size	: Radius;

	float1 life : LIFE;
	float3 color: Color;
	
	// for sprite sheet animation blending
	SpriteBlend	sprite_blend: SpriteBlend;
	
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
	
	output.color = input[0].color;

	
	//
	const SpriteBlend sprite_blend = CalcSpriteBlend( input[0].life );
	output.sprite_blend = sprite_blend;
	
	//
	const float radius = input[0].size;

	const float3 local_up		= g_camera_up_WS.xyz * radius;
	const float3 local_right	= g_camera_right_WS.xyz * radius;

	const float2 top_left_UV = float2(0,0);
	const float2 top_right_UV = float2(1,0);
	const float2 bottom_left_UV = float2(0,1);
	const float2 bottom_right_UV = float2(1,1);
	
	//
	const float3 top_left_WS = particle_origin_WS - local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_left_WS,1) );

	output.sprite_blend.uv0 = (top_left_UV + sprite_blend.uv0) * TEX_ATLAS_TILE_SIZE;
	output.sprite_blend.uv1 = (top_left_UV + sprite_blend.uv1) * TEX_ATLAS_TILE_SIZE;

	output.view_space_depth = mul( g_view_matrix, float4(top_left_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 top_right_WS = particle_origin_WS + local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_right_WS,1) );
	
	output.sprite_blend.uv0 = (top_right_UV + sprite_blend.uv0) * TEX_ATLAS_TILE_SIZE;
	output.sprite_blend.uv1 = (top_right_UV + sprite_blend.uv1) * TEX_ATLAS_TILE_SIZE;
	
	output.view_space_depth = mul( g_view_matrix, float4(top_right_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 bottom_left_WS = particle_origin_WS - local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_left_WS,1) );

	output.sprite_blend.uv0 = (bottom_left_UV + sprite_blend.uv0) * TEX_ATLAS_TILE_SIZE;
	output.sprite_blend.uv1 = (bottom_left_UV + sprite_blend.uv1) * TEX_ATLAS_TILE_SIZE;
	
	output.view_space_depth = mul( g_view_matrix, float4(bottom_left_WS,1) ).y;
	SpriteStream.Append(output);

	
	//
	const float3 bottom_right_WS = particle_origin_WS + local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_right_WS,1) );

	output.sprite_blend.uv0 = (bottom_right_UV + sprite_blend.uv0) * TEX_ATLAS_TILE_SIZE;
	output.sprite_blend.uv1 = (bottom_right_UV + sprite_blend.uv1) * TEX_ATLAS_TILE_SIZE;

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
		pixel_in.sprite_blend.uv0
	);
	const float4 sampled_color2 = t_sprite.Sample(
		s_sprite,
		pixel_in.sprite_blend.uv1
	);

	//
	return float4(
		lerp(
			sampled_color.rgb,
			sampled_color2.rgb,
			pixel_in.sprite_blend.lerp_alpha
		) + RED.rgb
		, sampled_color.a * depth_fade
		);
}
