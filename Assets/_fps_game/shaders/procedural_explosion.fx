/*

A simple-sprite based explosion.

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

/// use the method from "Soft Particles" sample [Direct3D 10] in NVIDIA Direct3D SDK 10 [2007]
#define	SOFT_PARTICLES_QUALITY_MEDIUM	(1)

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
	Vertex shader for drawing particles
==========================================================
*/

struct ParticleVertex
{
	float4	position_and_size: Position;

	// w: life in [0..1], if life > 1 => particle is dead
	float4	color_and_life: TexCoord;
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
    
    output.Pos = input.position_and_size.xyz;
    output.life = input.color_and_life.w;
    output.size = input.position_and_size.w;
	output.color = input.color_and_life.rgb;
    
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

	float2 sprite_uv : TexCoord0;
	
	float  size			  : TexCoord3;
	float3 worldPos		  : Position1;	// world position

	float3 particleOrig	  : Position2;	// particle_center_WS
	float3 particleColor  : TexCoord6;

	float3 eye_to_particle_dir : TexCoord7;

	// view-space depth of this particle; used by soft particles
	float1 view_space_depth : TexCoord8;

	float1 life : LIFE;
	float3 color: Color;
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

	const float3 eye_to_particle_dir = normalize( particle_origin_WS - g_camera_origin_WS.xyz );

	output.particleOrig = particle_origin_WS.xyz;
	output.eye_to_particle_dir = eye_to_particle_dir;

	//
	output.size = input[0].size;

	output.life = input[0].life;
	
	output.color = input[0].color;

	//

    // calculate color from a 1d gradient texture
    float3 particleColor = RED.rgb;// g_txColorGradient.SampleLevel( g_samLinearClamp, float2(input[0].life,0), 0 );
    output.particleColor = particleColor;

	//
	const float radius = input[0].size;

#if 0
	const float3 local_up		= g_camera_up_WS.xyz * radius;
	const float3 local_right	= g_camera_right_WS.xyz * radius;
#else
	const float3x3 basis = buildOrthonormalBasisFromNormal( g_camera_fwd_WS.xyz );
	const float3 local_up		= basis[0] * radius;
	const float3 local_right	= basis[1] * radius;
#endif

	//
	const float3 top_left_WS = particle_origin_WS - local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_left_WS,1) );
	output.sprite_uv = float2(0,0);
	output.worldPos = top_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_left_WS,1) ).y;
	output.eye_to_particle_dir = top_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 top_right_WS = particle_origin_WS + local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_right_WS,1) );
	output.sprite_uv = float2(1,0);
	output.worldPos = top_right_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_right_WS,1) ).y;
	output.eye_to_particle_dir = top_right_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_left_WS = particle_origin_WS - local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_left_WS,1) );
	output.sprite_uv = float2(0,1);
	output.worldPos = bottom_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(bottom_left_WS,1) ).y;
	output.eye_to_particle_dir = bottom_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_right_WS = particle_origin_WS + local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_right_WS,1) );
	output.sprite_uv = float2(1,1);
	output.worldPos = bottom_right_WS;
	output.view_space_depth = mul( g_view_matrix, float4(bottom_right_WS,1) ).y;
	output.eye_to_particle_dir = bottom_right_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	SpriteStream.RestartStrip();
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/

float calcSoftParticleDepthContrast(float Input, float ContrastPower)
{
#if 1
     //piecewise contrast function
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? 1-Input : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? 1-Output : Output;
     return Output;
#else
    // another solution to create a kind of contrast function
    return 1.0 - exp2(-2*pow(2.0*saturate(Input), ContrastPower));
#endif
}

///
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


	float depthFade = 1;


/// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]
#elif SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_CHEAP


	float depthDiff = view_space_depth_from_zbuffer - view_space_depth_from_particle;
	if( depthDiff < 0 ) {
		discard;
	}

	float g_fFadeDistance = sphere_radius * 0.5f;	// 0 = hard edges
	float depthFade = saturate( depthDiff / g_fFadeDistance );


/// use the method from "Soft Particles" sample [Direct3D 10] in NVIDIA Direct3D SDK 10 [2007]
#elif SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_MEDIUM


	float SoftParticleContrast = 2.0;
	float SoftParticleScale = 1.0;
	float zEpsilon = 0;

	const float zdiff = ( view_space_depth_from_zbuffer - pixel_in.Z );
	const float depthFade = calcSoftParticleDepthContrast(
		zdiff * SoftParticleScale,
		SoftParticleContrast
		);
	if( depthFade * zdiff <= zEpsilon )
	{
		discard;
	}


#endif

	//
	const float4 sampled_color = t_sprite.Sample(
		s_sprite,
		pixel_in.sprite_uv
	);

	const float life = pixel_in.life;	// dead if life > 1

	return float4(
		sampled_color.rgb,
		sampled_color.a * ( 1 - life ) * depthFade
		);
}
