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
	s_base_color = DiffuseMapSampler
	s_roughness_metallic = DiffuseMapSampler
	s_normals = NormalMapSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>
#include "Debug/solid_wireframe.h"
#include <PBR/PBR_Lighting.h>


/// The base color texture is already created with sRGB format,
/// so it's automatically converted from sRGB to linear space after sampling.
#define GAMMA_DECOMPRESS_BASE_COLOR	(0)

#define ENABLE_NORMAL_MAP			(1)
#define ENABLE_AMBIENT_OCCLUSION	(1)


/*
==============================================================================
	SAMPLERS
==============================================================================
*/

/// The base color has two different interpretations depending on the value of metalness.
/// When the material is a metal, the base color is the specific measured reflectance value at normal incidence (F0).
/// For a non-metal the base color represents the reflected diffuse color of the material.
Texture2D		t_base_color: register(t5);	// base color aka albedo / diffuse reflectance
SamplerState	s_base_color: register(s5);

/// glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel
/// The R channel: ambient occlusion
/// The G channel: roughness value
/// The B channel: metallic value
Texture2D		t_roughness_metallic: register(t7);	// the roughness (or glossiness) map
SamplerState	s_roughness_metallic: register(s7);

///
Texture2D		t_normals: register(t6);
SamplerState	s_normals: register(s6);


/// De-quantizes normalized coords from 32-bit integer (10,10,10).
float3 R10G10B10_to_float01( in uint u )
{
	return float3(
		float( ( u        ) & 1023u ),
		float( ( u >> 10u ) & 1023u ),
		float( ( u >> 20u ) & 1023u )
	);
}

// must be synced with Vertex_VoxelTerrainTextured
struct VS_In
{
	uint	q_position : Position;
	float2	tex_coord  : TexCoord;
	uint4	normal	   : Normal;
	uint4	tangent    : Tangent;	// .w stores the handedness of the tangent space
};

/*
==============================================================================
	DEFERRED RENDERING
==============================================================================
*/

struct VS_Out
{
	float4	position  : SV_Position;

	float2	tex_coord : TexCoord;

	float3 normal_in_view_space : Normal;
	float3 tangent_in_view_space : Tangent;
	float3 bitangent_in_view_space : Bitangent;
};

VS_Out VS_Deferred( in VS_In vertex_in )
{
	const float3 local_position = R10G10B10_to_float01( vertex_in.q_position );
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = -UnpackVertexNormal( vertex_in.tangent );

	//
	VS_Out	vertex_out;

	vertex_out.position		= Pos_Local_To_Clip( float4( local_position, 1 ) );
	vertex_out.tex_coord	= vertex_in.tex_coord;
	//
	vertex_out.normal_in_view_space = Dir_Local_To_View( local_normal );
	vertex_out.tangent_in_view_space = Dir_Local_To_View( local_tangent );
	vertex_out.bitangent_in_view_space = cross(
		vertex_out.tangent_in_view_space,
		vertex_out.normal_in_view_space
		);

	return vertex_out;
}

PS_to_GBuffer PS_Deferred( in VS_Out pixel_in )
{
	// Get base color/diffuse/albedo.
	const float4 sampled_base_color = t_base_color.Sample(
		s_base_color,
		pixel_in.tex_coord
	);


#if GAMMA_DECOMPRESS_BASE_COLOR
	const float4 linear_base_color = float4(
		SRGBtoLINEAR(sampled_base_color.rgb),
		sampled_base_color.a
	);
#else
	const float4 linear_base_color = sampled_base_color;
#endif


	// Get roughness, metalness, and ambient occlusion.
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data.
	const float3 sampled_roughness_metallic = t_roughness_metallic.Sample(
		s_roughness_metallic,
		pixel_in.tex_coord
	).rgb;


	const float sampled_occlusion = sampled_roughness_metallic.r;
	const float sampled_roughness = sampled_roughness_metallic.g;
	const float sampled_metalness = sampled_roughness_metallic.b;


	//

#if ENABLE_NORMAL_MAP

	const float3 sampled_normal_in_tangent_space = ExpandNormal(
		t_normals.Sample(
			s_normals,
			pixel_in.tex_coord
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
	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = pixel_normal_in_view_space;
		surface.albedo = linear_base_color.rgb;

		surface.specular_color = sampled_base_color.rgb;
		surface.specular_power = 10.0f;

		surface.metalness = sampled_metalness;
		surface.roughness = sampled_roughness;
	}

	//
	PS_to_GBuffer	pixel_out;
	GBuffer_Write( surface, pixel_out );

	return pixel_out;
}

/*
==============================================================================
	WIREFRAME RENDERING
==============================================================================
*/
GS_INPUT VS_Wireframe( in VS_In vertex_in )
{
	const float3 local_position = R10G10B10_to_float01( vertex_in.q_position );

	//
	GS_INPUT	vertex_out;
	vertex_out.Pos = Pos_Local_To_Clip( float4( local_position, 1 ) );
	vertex_out.PosV = Pos_Local_To_View( float4( local_position, 1 ) );
	return vertex_out;
}
