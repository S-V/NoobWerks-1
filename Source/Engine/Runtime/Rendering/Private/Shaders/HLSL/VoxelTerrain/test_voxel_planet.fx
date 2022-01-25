/*
Procedural Moon.

%%
pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS

	state = default
}

// Draws TBN basis; VS accepts points.
pass DebugLines
{
	vertex = dbg_show_tangents_VS
	geometry = dbg_show_tangents_GS
	pixel = dbg_show_tangents_PS

	state = default
}

//pass RenderDepthOnly
//{
//	vertex = main_VS
//	// null pixel shader
//
//	state = render_depth_to_shadow_map
//}

//

feature ENABLE_NORMAL_MAP
{
	default = 1
}

//
//
//SamplerState DetailMapSampler
//{
//    Filter = Min_Mag_Mip_Linear;
//    AddressU = Wrap;
//    AddressV = Wrap;
//	AddressW = Wrap;
//};

//

samplers
{
	s_diffuse_map = DiffuseMapSampler
	s_normal_map = NormalMapSampler
	s_detail_map = DetailMapSampler
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_noise.h"
#include <GBuffer/nw_gbuffer_write.h>
#include <Common/Colors.hlsl>


Texture2D		t_diffuse_map: register(t5);
Texture2D		t_normal_map: register(t6);

Texture2D		t_detail_base_map: register(t7);
Texture2D		t_detail_normal_map: register(t8);

//Texture2D<float>	t_height_map: register(t9);

//

SamplerState	s_diffuse_map: register(s5);
SamplerState	s_normal_map: register(s6);
SamplerState	s_detail_map: register(s7);
//SamplerState	s_height_map: register(s7);


///
struct VS_Out
{
	float4 position : SV_Position;
	float4 position_in_view_space : Position0;
	float4 position_in_world_space : Position1;

	float2 texCoord : TexCoord0;

	float3 normal_in_view_space : Normal;
	float3 tangent_in_view_space : Tangent;
	float3 bitangent_in_view_space : Bitangent;

	float3 normal_for_planet_UV_calc : Normal1;
};

/*
==========================================================
	VERTEX SHADER
==========================================================
*/
void main_VS(
			 in Vertex_Static vertex_in,
			 out VS_Out vertex_out_
			 )
{
	const float3 normalized_position = vertex_in.position;
	const float4 local_position_h = float4( normalized_position, 1 );

	//
	vertex_out_.position = Pos_Local_To_Clip( local_position_h );
	vertex_out_.position_in_view_space = Pos_Local_To_View( local_position_h );
	vertex_out_.position_in_world_space = Pos_Local_To_World( local_position_h );

	//
	vertex_out_.texCoord = vertex_in.tex_coord;

	//
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = UnpackVertexNormal( vertex_in.tangent );

	//
	vertex_out_.normal_in_view_space = Dir_Local_To_View( local_normal );

	vertex_out_.tangent_in_view_space = Dir_Local_To_View( local_tangent );

	vertex_out_.bitangent_in_view_space = cross(
		vertex_out_.tangent_in_view_space,
		vertex_out_.normal_in_view_space
	);

	//
	vertex_out_.normal_for_planet_UV_calc = ExpandNormal( vertex_in.normal_for_UV_calc.xyz );
}

inline float2 calc_UV_from_Sphere_Normal( const float3 N )
{
	return float2(
		atan2( N.y, N.x ) * tbINV_TWOPI + 0.5,
		N.z * 0.5 + 0.5
		);
}

float3 estimate_Normal_from_HeightMap(
									  Texture2D<float> heightmap_texture,
									  SamplerState texture_sampler,
									  in float2 uv
									  )
{
	const float delta_u = 1.0f / 1024;
	const float delta_v = 1.0f / 512;

	//
	float3 local_normal;

	local_normal.x
		= heightmap_texture.Sample( texture_sampler, uv + float2(  delta_u, 0 ) ).x
		- heightmap_texture.Sample( texture_sampler, uv + float2( -delta_u, 0 ) ).x
		;

	local_normal.y
		= heightmap_texture.Sample( texture_sampler, uv + float2( 0,  delta_v ) ).x
		- heightmap_texture.Sample( texture_sampler, uv + float2( 0, -delta_v ) ).x
		;

	local_normal.z = sqrt( 1.0 - local_normal.x * local_normal.x - local_normal.y * local_normal.y );

	return local_normal;
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
			out float3 pixel_out_: SV_Target
#endif

			 )
{
	const float3 normal_for_planet_UV_calc = normalize( pixel_in.normal_for_planet_UV_calc );

	const float2 uv = calc_UV_from_Sphere_Normal( normal_for_planet_UV_calc );

	//


	//
	const float3 sampled_diffuse_color = t_diffuse_map.Sample(
		s_diffuse_map,
		uv
	).rgb;

	float3 base_color = sampled_diffuse_color;



	//


	// Get noise at different frequencies:
	// 10 m - heightmap of the closeup terrain

	const float noise_10m = Noise2D(
		pixel_in.position_in_view_space.xy,
		10.0
		);	// [0..1]



	//
	const float distance_to_eye_pos = length( pixel_in.position_in_view_space );

	//
	const float3 sampled_detail_map_color = t_detail_base_map.Sample(
		s_detail_map,
		float2(uv.x*2, uv.y)*2//pixel_in.position_in_world_space.xy * 1e-4
	).rgb;


	//
	const float DETAIL_MAX_DISTANCE = 5;	// detail texels are not visible past this distance
	// the 'high detail' texture is shown up to 5 meters from the viewer
	const float detail_fade_factor = 1 - smoothstep(
		0, DETAIL_MAX_DISTANCE,
		distance_to_eye_pos
		);

	//
	// Eric Lengyel, 2010, P.49.
	// Calculate a vector that holds ones for values of s that should be negated. 
	// calculate the blend factor ('delta') for each direction
	const float blendSharpness = 0.5f;
	// Raise each component of normal vector to 4th power.
	const float power = 4.0f;

	float3 blendWeights = saturate(abs(normal_for_planet_UV_calc) - blendSharpness);
	// Normalize result by dividing by the sum of its components. 
	blendWeights /= dot(blendWeights, (float3)1.0f);	// force sum to 1.0

	//
	const float3 detail_tex_pos = pixel_in.position_in_world_space.xyz;

	//
	const float detail_texture_scale = 1.0f;	// the higher this value, the higher the tiling frequency/repetition
	const float2 detail_uv_axisX = float2( detail_tex_pos.yz * detail_texture_scale );
	const float2 detail_uv_axisY = float2( detail_tex_pos.zx * detail_texture_scale );
	const float2 detail_uv_axisZ = float2( detail_tex_pos.xy * detail_texture_scale );
	//
	const float3 detail_base_color_axisX = t_detail_base_map.Sample( s_detail_map, detail_uv_axisX ).rgb;
	const float3 detail_base_color_axisY = t_detail_base_map.Sample( s_detail_map, detail_uv_axisY ).rgb;
	const float3 detail_base_color_axisZ = t_detail_base_map.Sample( s_detail_map, detail_uv_axisZ ).rgb;
	// blend the results of the 3 planar projections
	const float3 detail_blended_base_color
		= detail_base_color_axisX * blendWeights.x
		+ detail_base_color_axisY * blendWeights.y
		+ detail_base_color_axisZ * blendWeights.z
		;


	base_color = lerp( base_color, detail_blended_base_color, detail_fade_factor );
	//base_color = base_color*1e-6 + detail_blended_base_color;


	//

#if ENABLE_NORMAL_MAP

	const float bumpiness = 0.2;// 0.2;	// 1 = max

	// NOTE: for some reason, the normal map generator stores 'inverted' normals, we should correct them:
	const float3 sampled_normal_in_tangent_space = ExpandNormal(
		t_normal_map.Sample(
			s_normal_map,
			uv
			).rgb
		);

	const float3 corrected_normal_in_tangent_space = -sampled_normal_in_tangent_space
		* float3( bumpiness, bumpiness, 1 )
		;


	#if 1
		//const float3 pixel_normal_in_view_space = normalize
		//	( pixel_in.tangent_in_view_space	* sampled_normal_in_tangent_space.x
		//	+ pixel_in.bitangent_in_view_space	* sampled_normal_in_tangent_space.y
		//	+ pixel_in.normal_in_view_space		* sampled_normal_in_tangent_space.z
		//	);

		//const float3x3 tangent_space_to_view_space = float3x3(
		//	normalize( pixel_in.tangent_in_view_space ),
		//	normalize( pixel_in.bitangent_in_view_space ),
		//	normalize( pixel_in.normal_in_view_space )
		//	);

		//const float3 pixel_normal_in_view_space =
		//	normalize( mul( tangent_space_to_view_space, sampled_normal_in_tangent_space ) )
		//	//normalize( mul( tangent_space_to_view_space, sampled_normal_in_tangent_space ) ) * 1e-6
		//	//+ normalize( pixel_in.normal_in_view_space ) * 1e-6
		//	//+ Dir_Local_To_View(normal_for_planet_UV_calc)
		//	;

		//float3 T = normalize( pixel_in.tangent_in_view_space );
		//float3 B = normalize( pixel_in.bitangent_in_view_space );
		//float3 N = normalize( pixel_in.normal_in_view_space );


		const float3 N = Dir_Local_To_View( normal_for_planet_UV_calc );
		const float3 T = normalize(
			pixel_in.tangent_in_view_space - dot(pixel_in.tangent_in_view_space, N) * N
			);
		float3 B = normalize( cross(N,T) );

		//
		const float3 pixel_normal_in_view_space =
			normalize(
				float3(
					dot( float3( T.x, B.x, N.x ), corrected_normal_in_tangent_space ),
					dot( float3( T.y, B.y, N.y ), corrected_normal_in_tangent_space ),
					dot( float3( T.z, B.z, N.z ), corrected_normal_in_tangent_space )
				)
			);

		//pixel_normal_in_view_space = normalize(pixel_normal_in_view_space);

	#else

		float3 N = normalize(pixel_in.normal_in_view_space);
		float3 T = normalize(pixel_in.tangent_in_view_space);//- dot(pixel_in.tangent_in_view_space, N)*N);
		float3 B = cross(N,T);
		float3x3 TBN = float3x3(T, B, N);
		// Transform from tangent space to world space.
		float3 bumpedNormalW = normalize(
			//mul( TBN, sampled_normal_in_tangent_space )
			mul( sampled_normal_in_tangent_space, TBN )
			);

		float3 pixel_normal_in_view_space = normalize(bumpedNormalW);

		//pixel_normal_in_view_space = normalize(sampled_normal_in_tangent_space);

		////
		//const float3 pixel_normal_WS = Dir_View_To_World( pixel_in.tangent_in_view_space );

		//base_color = pixel_normal_WS + base_color*1e-6;
	#endif

#else	// !ENABLE_NORMAL_MAP

	const float3 pixel_normal_in_view_space = normalize(
		pixel_in.normal_in_view_space
		);

	//const float3 local_normal = estimate_Normal_from_HeightMap(
	//	t_height_map,
	//	s_height_map,
	//	uv
	//	);

	//if(0)
	//{
	//	pixel_normal_in_view_space.x += local_normal.x;
	//	pixel_normal_in_view_space.y += local_normal.y;
	//	pixel_normal_in_view_space = normalize( pixel_normal_in_view_space );
	//}
	//else
	//{
	//	pixel_normal_in_view_space = normalize
	//		( pixel_in.tangent_in_view_space * local_normal.x
	//		+ pixel_in.bitangent_in_view_space * local_normal.y
	//		+ pixel_in.normal_in_view_space * local_normal.z
	//		);
	//}


#endif	// !ENABLE_NORMAL_MAP





	//const float3 pixel_normal_in_world_space = Dir_View_To_World( pixel_normal_in_view_space );

	//
	const float3 sampled_specular_color = float3(0,0,0);

	//

#if __PASS == FillGBuffer

	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = pixel_normal_in_view_space;
		surface.albedo = base_color.rgb;

		surface.specular_color = sampled_specular_color;
		surface.specular_power = 0.01f;

		surface.metalness = 0.04f;
		surface.roughness = 0.99f;
	}
	GBuffer_Write( surface, pixel_out_ );

#else

	pixel_out_ = sampled_diffuse_color.rgb;

#endif

}

/*
==========================================================
	DEBUG VERTEX SHADER
==========================================================
*/
///
struct Dbg_VS_Out
{
	float4 position : SV_Position;

	float3 position_in_view_space : Position;

	float3 normal_in_view_space : Normal;
	float3 tangent_in_view_space : Tangent;
	float3 bitangent_in_view_space : Bitangent;
};


void dbg_show_tangents_VS(
			 in Vertex_Static vertex_in,
			 out Dbg_VS_Out vertex_out_
			 )
{
	const float4 local_position = float4( vertex_in.position, 1 );

	vertex_out_.position = Pos_Local_To_Clip( local_position );

	vertex_out_.position_in_view_space = Pos_Local_To_View( local_position ).xyz;

	//
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = UnpackVertexNormal( vertex_in.tangent );

	//
	vertex_out_.normal_in_view_space = Dir_Local_To_View( local_normal );

	vertex_out_.tangent_in_view_space = Dir_Local_To_View( local_tangent );

	vertex_out_.bitangent_in_view_space = cross(
		vertex_out_.tangent_in_view_space,
		vertex_out_.normal_in_view_space
		);
}

/*
==========================================================
	DEBUG GEOMETRY SHADER
==========================================================
*/
struct Dbg_GS_Out
{
	float4 position : SV_Position;
	float3 color : Color;
};

void GS_AppendLine(
				in float3 start_position,
				in float3 direction,
				in float3 color,
				inout LineStream<Dbg_GS_Out> lineStream
				)
{
	{
		Dbg_GS_Out	startpoint;
		startpoint.position = Pos_View_To_Clip( start_position );
		startpoint.color = color;
		lineStream.Append( startpoint );
	}
	//
	{
		float line_length_VS = 1;
		float3 endPosition = start_position + direction * line_length_VS;

		Dbg_GS_Out	endpoint;
		endpoint.position = Pos_View_To_Clip( endPosition );
		endpoint.color = color;
		lineStream.Append( endpoint );
	}
	//
	lineStream.RestartStrip();
}

[maxvertexcount(6)]
void dbg_show_tangents_GS(
			 point Dbg_VS_Out input_vertices[1],
			 inout LineStream<Dbg_GS_Out> lineStream
			 )
{
	const Dbg_VS_Out vertex_in = input_vertices[0];
	const float3 start_position = vertex_in.position_in_view_space;

	// Tangent
	GS_AppendLine( start_position, vertex_in.tangent_in_view_space, RED.rgb, lineStream );

	// Bitangent
	GS_AppendLine( start_position, vertex_in.bitangent_in_view_space, GREEN.rgb, lineStream );

	// Normal
	GS_AppendLine( start_position, vertex_in.normal_in_view_space, BLUE.rgb, lineStream );
}

/*
==========================================================
	DEBUG PIXEL SHADER
==========================================================
*/
float3 dbg_show_tangents_PS(
			 in Dbg_GS_Out pixel_in
			 ) : SV_Target
{
	return pixel_in.color.rgb;
}

/*
References:

FlightGear Flight Simulator: free open-source multiplatform flight sim:
http://wiki.flightgear.org/Procedural_Texturing
https://sourceforge.net/p/flightgear/fgdata/ci/next/tree/Shaders/terrain-ALS-detailed.frag

*/
