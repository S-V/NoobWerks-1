/*

Modifies albedo in FillGBuffer pass
and light accumulation buffer in Emissive pass.

Based on:

Why Geometry Shaders Are Slow (Unless you’re Intel)
March 18, 2015 by Joshua Barczak	
http://www.joshbarczak.com/blog/?p=667
https://github.com/jbarczak/gs_cubes/blob/master/GS.hlsl

%%

pass DeferredDecals
{
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'Back'
			flags = (
				'Enable_DepthClip'
			)
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
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
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


pass EmissiveAdditive
{
	vertex = main_VS
	geometry = main_GS
	pixel = main_PS

	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'Back'
			flags = (
				'Enable_DepthClip'
			)
		}
		depth_stencil = 	{
			flags = (
				'Enable_DepthTest'
				; no depth writes
			)
			; Reverse Z
			comparison_function = 'Greater_Equal'
		}
		blend = {
			flags = (
				'Enable_Blending'
			)
			color_channel = {
				operation = 'ADD'
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
			}
			write_mask	= ('ENABLE_ALL')
		}
	}
}


//samplers
//{
//	s_decal = DiffuseMapSampler
//}
//
//textures
//{
//	t_decal = 'uv-test'
//}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/Colors.hlsl>
#include <Common/transform.hlsli>
#include "_basis.h"	// buildOrthonormalBasisFromNormal()
#include "_PS.h"
#include "_noise.h"
#include <GBuffer/nw_gbuffer_read.h>	// DepthTexture
#include <GBuffer/nw_gbuffer_write.h>
#include <Noise/SimplexNoise3D.hlsli>
#include <Noise/fBM.hlsli>
#include <Utility/CubeTriangleStrip.hlsli>
#include <Utility/DistanceFunctions.hlsli>

/*
==========================================================
	DEFINES
==========================================================
*/

/*
==========================================================
	SHADER PARAMETERS
==========================================================
*/
//Texture2D		t_decal;
//SamplerState	s_decal;

/*
==========================================================
	STRUCTS
==========================================================
*/
struct VS_In
{
	float4	position_and_size: Position;

	// w: life in [0..1], if life > 1 => particle is dead
	float4	color_and_life: TexCoord0;
	
	// w - unused
	float4	decal_axis_x: TexCoord1;
	// w - unused
	float4	decal_normal: TexCoord2;
	// axis_y = cross(axis_x, normal)
};

struct GS_In
{
	nointerpolation float3	decal_center_ws : Position;
	nointerpolation float3	decal_axis_x: TexCoord0;
	nointerpolation float3	decal_axis_y: TexCoord1;
	nointerpolation float3	decal_axis_z: TexCoord2;	// decal UP == surface normal

    nointerpolation float	life : Life;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    nointerpolation float	radius : Radius;
	nointerpolation float3	color: Color;
};

struct PS_In
{
	float4 position : SV_Position;
	float4 position_cs : Position0;

	nointerpolation float3	decal_center_ws: Position1;
	nointerpolation float3	decal_axis_x: TexCoord0;
	nointerpolation float3	decal_axis_y: TexCoord1;
	nointerpolation float3	decal_axis_z: TexCoord2;	// decal UP == surface normal

	nointerpolation float1 radius : Radius;
	nointerpolation float1 alpha : Transparency;

	float3 color: Color;
};

/*
==========================================================
	VERTEX SHADER
	Vertex shader for drawing particles
==========================================================
*/
GS_In main_VS( in VS_In vs_in )
{
    GS_In vs_out;

	vs_out.decal_center_ws	= vs_in.position_and_size.xyz;
	vs_out.decal_axis_x	= vs_in.decal_axis_x.xyz;
	vs_out.decal_axis_z	= vs_in.decal_normal.xyz;
	vs_out.decal_axis_y = normalize(cross(
		vs_in.decal_axis_x.xyz,
		vs_in.decal_normal.xyz
		));

	vs_out.life			= vs_in.color_and_life.w;
    vs_out.radius		= vs_in.position_and_size.w;
	vs_out.color		= vs_in.color_and_life.rgb;

	return vs_out;
}

/*
==========================================================
	GEOMETRY SHADER
	Geometry shader for creating point sprites
==========================================================
*/
	


[maxvertexcount(14)]
void main_GS(
	in point GS_In box[1]
	, inout TriangleStream< PS_In > triangle_stream
	)
{
	const GS_In	gs_in = box[0];

#if __PASS == EmissiveAdditive
	if( gs_in.life >= 1 ) {
		// discard - this decal is not glowing
		return;
	}
#endif
	
    [unroll]
    for( int i=0; i<14; i++ )
	{
		PS_In	gs_out;

		const float3 unit_cube_vertex_position = STRIPIFIED_UNIT_CUBE_VERTS[ CUBE_TRISTRIP_INDICES[i] ];

		const float3 scaled_cube_vertex_position = unit_cube_vertex_position * gs_in.radius;

		const float3 rotated_cube_vertex_position = float3(
			dot(scaled_cube_vertex_position, gs_in.decal_axis_x),
			dot(scaled_cube_vertex_position, gs_in.decal_axis_y),
			dot(scaled_cube_vertex_position, gs_in.decal_axis_z)
			);

		const float3 cube_vertex_position_ws
			= rotated_cube_vertex_position + gs_in.decal_center_ws
			;

		const float4 cube_vertex_position_cs
			= Transform_Position_World_To_Clip( float4(cube_vertex_position_ws,1) )
			;
		
		{
			gs_out.position = cube_vertex_position_cs;
			gs_out.position_cs = cube_vertex_position_cs;

			gs_out.decal_center_ws = gs_in.decal_center_ws;
			gs_out.decal_axis_x = gs_in.decal_axis_x;
			gs_out.decal_axis_y = gs_in.decal_axis_y;
			gs_out.decal_axis_z = gs_in.decal_axis_z;

			gs_out.radius = gs_in.radius;
			gs_out.alpha = 1 - gs_in.life;
			gs_out.color = gs_in.color;
		}

		triangle_stream.Append(gs_out);
	}//For each strip index.
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
shdSphere shdSphere_Make(
	in float3 sphere_center,
	in float sphere_radius
){
	shdSphere	result;
	result.center = sphere_center;
	result.radius = sphere_radius;
	return result;
}

///
struct RadialFalloffUtil
{
	shdSphere	sphere;

	float core_radius_ratio;
	float atmo_radius_ratio;

	///
	float CalcOpacity(
		in float3 sample_position
		)
	{
		const float core_radius = sphere.radius * core_radius_ratio;
		const float atmo_radius = sphere.radius * atmo_radius_ratio;
		
		const float dist_to_core = SDF_Sphere(
			sample_position,
			shdSphere_Make(
				sphere.center, core_radius
			)
		);
		
		const float dist_to_core01 = saturate( dist_to_core / ( atmo_radius - core_radius ) );	// make round
		return 1 - dist_to_core01;
	}
};

///
struct SphericNoiseUtil
{
	shdSphere	sphere;

	float core_radius_ratio;

	int num_octaves;

	bool turbulence;

	// > 1 - more detail
	float frequency;

	///
	float CalcOpacity(
		in float3 sample_position
		)
	{
		const float core_radius = sphere.radius * core_radius_ratio;
		const float atmo_radius = sphere.radius;

		const shdSphere	core_sphere = shdSphere_Make(
			sphere.center, core_radius
		);
		const float dist_to_core = SDF_Sphere( sample_position, core_sphere );

		// Perturb the sphere using noise.
		fBM_SimplexNoise3D	fBM;
		fBM.SetDefaults();
		fBM._num_octaves = num_octaves;

		// [0..1]
		const float noise_value = fBM.Evaluate(
			sample_position * frequency - sphere.center
			, turbulence
			);

		//
		const float perturbed_distance = dist_to_core + noise_value;
		const float perturbed_distance01 = saturate( perturbed_distance / ( atmo_radius - core_radius ) );	// make round
		return 1 - perturbed_distance01;
	}
};


void main_PS(
			 in PS_In ps_in,
			 out float4 color_out_: SV_Target
			 )
{
	const float2 pixel_screen_space_position = ps_in.position_cs.xy / ps_in.position_cs.w;
	//const float2 pixel_tex_coords = ClipPosToTexCoords( pixel_screen_space_position );
	
	//
	const float hardware_z_depth = DepthTexture.Load( int3( ps_in.position.xy, 0 ) );
	const float3 surface_position_VS = restoreViewSpacePosition( pixel_screen_space_position, hardware_z_depth );

	//
	const float3 surface_position_WS = Pos_View_To_World( float4(surface_position_VS,1) ).xyz;

	//OPTIMIZE! in view space!

	// Burn/ScorchMark
	SphericNoiseUtil	high_detail_scorchmark;
	high_detail_scorchmark.sphere.center = ps_in.decal_center_ws;
	high_detail_scorchmark.sphere.radius = ps_in.radius;
	high_detail_scorchmark.core_radius_ratio = 0.6f;
	high_detail_scorchmark.num_octaves = 2;
	high_detail_scorchmark.turbulence = true;
	high_detail_scorchmark.frequency = 16;	// scorchmark must have more high-frequency detail

	SphericNoiseUtil	low_detail_scorchmark;
	low_detail_scorchmark.sphere.center = ps_in.decal_center_ws;
	low_detail_scorchmark.sphere.radius = ps_in.radius * 0.8f;
	low_detail_scorchmark.core_radius_ratio = 0.1f;
	low_detail_scorchmark.num_octaves = 1;
	low_detail_scorchmark.turbulence = false;
	low_detail_scorchmark.frequency = 1;

	//
	SphericNoiseUtil	glowing_scorchmark;
	glowing_scorchmark.sphere.center = ps_in.decal_center_ws;
	glowing_scorchmark.sphere.radius = ps_in.radius * 0.6f;
	glowing_scorchmark.core_radius_ratio = 0.1f;
	glowing_scorchmark.num_octaves = 1;
	glowing_scorchmark.turbulence = false;
	glowing_scorchmark.frequency = 8;	// scorchmark must have more high-frequency detail

	//
#if __PASS == DeferredDecals

	// Burn/Scorch
	const float high_detail_opacity = high_detail_scorchmark.CalcOpacity(
		surface_position_WS
	);
	
	RadialFalloffUtil	low_detail_radial_scorchmark_falloff;
	low_detail_radial_scorchmark_falloff.sphere = low_detail_scorchmark.sphere;
	low_detail_radial_scorchmark_falloff.core_radius_ratio = 0.5f;
	low_detail_radial_scorchmark_falloff.atmo_radius_ratio = 1;

	const float low_detail_opacity = low_detail_radial_scorchmark_falloff.CalcOpacity(
		surface_position_WS
	);

	color_out_ = DARK_GRAY*0.1f;
	color_out_.a = max(high_detail_opacity, low_detail_opacity*0.7f);

//dbg boxes
//color_out_ += BLUE * 0.8f;

#elif __PASS == EmissiveAdditive

	// Glow
	RadialFalloffUtil	glow_radial_falloff;
	glow_radial_falloff.sphere = glowing_scorchmark.sphere;
	glow_radial_falloff.core_radius_ratio = 0.7f;
	glow_radial_falloff.atmo_radius_ratio = 1;

	const float opacity = glow_radial_falloff.CalcOpacity(
		surface_position_WS
	);
	
	const float glowing_opacity_low_detail = glowing_scorchmark.CalcOpacity(
		surface_position_WS
	);

	color_out_ = float4(0.95, 0.3, 0.2, opacity * glowing_opacity_low_detail) * ps_in.alpha;

	
	//if( length(surface_position_WS - ps_in.decal_center_ws) < ps_in.radius * 0.8 )
	//{
	//	color_out_ = GREEN;
	//}

#endif

}
