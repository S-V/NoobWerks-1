/*

NOTE: UNUSED!

When using deferred rendering, applying decals is quite simple. All that is required is to draw a cube that represents the decal volume onto the gbuffer. The decal application is independent of the complexity of the scene geometry. 

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
				source_factor = 'ONE'
				destination_factor = 'ONE'
			}
			write_mask	= ('ENABLE_ALL')
		}
	}
}


samplers
{
	s_decal = DiffuseMapSampler
}

textures
{
	t_decal = 'uv-test'
}

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
#include "Noise/SimplexNoise3D.hlsli"

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
Texture2D		t_decal;
SamplerState	s_decal;

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

	nointerpolation float1 inv_radius : Radius;
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

// Single tri-strip cube, from:
//  Interesting CUBE triangle strip
//   http://www.asmcommunity.net/board/index.php?action=printpage;topic=6284.0
// ; 1-------3-------4-------2   Cube = 8 vertices
// ; |  E __/|\__ A  |  H __/|   =================
// ; | __/   |   \__ | __/   |   Single Strip: 4 3 7 8 5 3 1 4 2 7 6 5 2 1
// ; |/   D  |  B   \|/   I  |   12 triangles:     A B C D E F G H I J K L
// ; 5-------8-------7-------6
// ;         |  C __/|
// ;         | __/   |
// ;         |/   J  |
// ;         5-------6
// ;         |\__ K  |
// ;         |   \__ |
// ;         |  L   \|         Left  D+E
// ;         1-------2        Right  H+I
// ;         |\__ G  |         Back  K+L
// ;         |   \__ |        Front  A+B
// ;         |  F   \|          Top  F+G
// ;         3-------4       Bottom  C+J
// ;
static const int INDICES[14] =
{
   4, 3, 7, 8, 5, 3, 1, 4, 2, 7, 6, 5, 2, 1,
};

//     3   4
//   1   2
//     8   7
//   5   6
void GenerateTransformedBox(
	out float4 v[8],
	in float3 center,
	float4 R0, float4 R1, float4 R2
	)
{
    // All of the canonical box verts are +- 1, transformed by the 3x4 matrix { R0,R1,R2}
    //    This boils down to just offsetting from the box center with different signs for each vert
    //
    // Instead of transforming each of the 8 verts, we can exploit the distributive law
    //   for matrix multiplication and do the extrusion in clip space
    //
    //float4 center = float4( R0.w,R1.w,R2.w,1);
    float4 X = float4( R0.x,R1.x,R2.x,0);
    float4 Y = float4( R0.y,R1.y,R2.y,0);
    float4 Z = float4( R0.z,R1.z,R2.z,0);
    float4 centerH = Transform_Position_World_To_Clip( float4(center,1) );
    X = Transform_Position_World_To_Clip( X );
    Y = Transform_Position_World_To_Clip( Y );
    Z = Transform_Position_World_To_Clip( Z );

    float4 t1 = centerH - X - Z;
    float4 t2 = centerH + X - Z;
    float4 t3 = centerH - X + Z;
    float4 t4 = centerH + X + Z;

    v[0] = t1 + Y;
    v[1] = t2 + Y;
    v[2] = t3 + Y;
    v[3] = t4 + Y;
    v[4] = t1 - Y;
    v[5] = t2 - Y;
    v[6] = t4 - Y;
    v[7] = t3 - Y;
}

[maxvertexcount(14)]
void main_GS(
	in point GS_In box[1]
	, inout TriangleStream< PS_In > triangle_stream
	)
{
	const GS_In	gs_in = box[0];

#if 1

	const float3	X = gs_in.decal_axis_x.xyz;
	const float3	Z = gs_in.decal_axis_z.xyz;
	const float3	Y = gs_in.decal_axis_y.xyz;

    const float3	t1 =  - X - Y;
    const float3	t2 =  + X - Y;
    const float3	t3 =  - X + Y;
    const float3	t4 =  + X + Y;

	float3 v[8];
    v[0] = t1 + Z;
    v[1] = t2 + Z;
    v[2] = t3 + Z;
    v[3] = t4 + Z;
    v[4] = t1 - Z;
    v[5] = t2 - Z;
    v[6] = t4 - Z;
    v[7] = t3 - Z;

    // Indices are off by one, so we just let the optimizer fix it
    [unroll]
    for( int i=0; i<14; i++ )
	{
		PS_In	gs_out;

		const float3 unit_cube_vertex_position = v[ INDICES[i]-1 ];

		const float3 cube_vertex_position_ws
			= unit_cube_vertex_position + gs_in.decal_center_ws
			;

		const float4 cube_vertex_position_cs
			= Transform_Position_World_To_Clip( float4(cube_vertex_position_ws,1) )
			;
		
		{
			gs_out.position = cube_vertex_position_cs;
			gs_out.position_cs = cube_vertex_position_cs;

			gs_out.decal_center_ws = gs_in.decal_center_ws;
			gs_out.decal_axis_x = X.xyz;
			gs_out.decal_axis_y = Y.xyz;
			gs_out.decal_axis_z = Z.xyz;

			gs_out.inv_radius = 1.0 / gs_in.radius;
			gs_out.alpha = 1 - gs_in.life;
			gs_out.color = gs_in.color;
		}

		triangle_stream.Append(gs_out);
	}//For each strip index.

#else

	const float4	R0 = box[0].decal_axis_x;
	const float4	R2 = box[0].decal_normal;
	const float4	R1 = float4(
		normalize(cross(R0.xyz, R2.xyz)),
		0
	);


	// Indices are off by one, so we just let the optimizer fix it
    [unroll]
    for( int i=0; i<14; i++ )
	{
		PS_In	gs_out;

		float4 position_cs = v[INDICES[i]-1];
		{
			gs_out.position = position_cs;
			gs_out.position_cs = position_cs;

			gs_out.decal_axis_x = R0.xyz;
			gs_out.decal_axis_y = R1.xyz;
			gs_out.decal_axis_z = R2.xyz;

			gs_out.life = box[0].life;
			gs_out.color = box[0].color;
		}
		triangle_stream.Append(gs_out);
	}//For each strip index.

#endif

}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
void main_PS(
			 in PS_In ps_in,
			 //out PS_to_GBuffer pixel_out_
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


	//
	//const GBufferPixel source_gbuffer_pixel = GBuffer_LoadPixelData(
	//	ps_in.position.xy
	//	);

	// Transform the view-space position into object space.
	const float3 surface_position_relative_to_decal_center
		= (surface_position_WS - ps_in.decal_center_ws)
		* ps_in.inv_radius
		;

	const float3 surface_position_in_decal_space = float3(
		dot(surface_position_relative_to_decal_center, ps_in.decal_axis_x),
		dot(surface_position_relative_to_decal_center, ps_in.decal_axis_y),
		dot(surface_position_relative_to_decal_center, ps_in.decal_axis_z)
		);

	const float2 surface_position_decal_proj = surface_position_in_decal_space.xz;	// [-1..+1]
	if(
		surface_position_decal_proj.x < -1 || surface_position_decal_proj.x > +1
		||
		surface_position_decal_proj.y < -1 || surface_position_decal_proj.y > +1
		)
	{
		discard;
	}

	//
	//const float2 decal_tex_coord = surface_position_decal_proj.xy * 0.5 + 0.5;	// [0..1]
	const float2 decal_tex_coord = ClipPosToTexCoords(surface_position_decal_proj.xy);	// [0..1]

	//
	const float4 sampled_decal_color = t_decal.Sample(
		s_decal,
		decal_tex_coord
	);




	color_out_ = sampled_decal_color;

	//color_out_.a *= ps_in.alpha;
	

	////
	//Surface_to_GBuffer surface;
	//surface.setDefaults();

	//surface.albedo = sampled_decal_color.rgb;

	//GBuffer_Write( surface, pixel_out_ );

/*
	//pixel_out_.RT1.rgb = GREEN.rgb;

	pixel_out_.RT0 = source_gbuffer_pixel.RT0;
	
	pixel_out_.RT1 = source_gbuffer_pixel.RT1;
//	pixel_out_.RT1.rgb = GREEN.rgb;

	pixel_out_.RT2 = source_gbuffer_pixel.RT2;
*/

}
