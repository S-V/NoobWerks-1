/*
Heavily based on
https://github.com/KhronosGroup/glTF-Sample-Viewer
https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag

https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr_khr.frag
https://github.com/SaschaWillems/Vulkan-glTF-PBR

https://github.com/microsoft/DirectXTK/blob/master/Src/Shaders/PBRCommon.fxh
https://github.com/microsoft/DirectXTK/blob/master/Src/Shaders/PBREffect.fx

https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl

Reference:
https://github.khronos.org/glTF-Sample-Viewer/

%%
pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS

	state = default
}

pass Opaque_Forward
{
	vertex = main_VS
	pixel = main_PS

	state = default
}

//

samplers
{
	s_base_color = DiffuseMapSampler
	s_roughness_metallic = DiffuseMapSampler
	s_normals = NormalMapSampler
}

%%
*/

#define	USE_GLTF_IMPL				(0)

/// The base color texture is already created with sRGB format,
/// so it's automatically converted from sRGB to linear space after sampling.
#define GAMMA_DECOMPRESS_BASE_COLOR	(0)

#define ENABLE_NORMAL_MAP			(1)
#define ENABLE_AMBIENT_OCCLUSION	(1)

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include <Common/Colors.hlsl>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>
#include "PBR/PBR_Lighting.h"
#include "PBR/PBR_Lighting_glTF.h"




// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
// https://medium.com/game-dev-daily/the-srgb-learning-curve-773b7f68cf7a
static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / 2.2;

// linear to sRGB approximation
float3 LINEARtoSRGB(float3 color)
{
    return pow( abs(color), (float3)INV_GAMMA );
}

// sRGB to linear approximation
float3 SRGBtoLINEAR( float3 srgbIn )
{
    return pow( abs(srgbIn), (float3)GAMMA );
}




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

/*
cbuffer Uniforms
{
	row_major float2x4	u_tex_coord_transform = {
		// row 0, row 1
		1,        0,
		0,        1,
		0,        0,
		0,        0,
	};
};
*/


struct VS_Out
{
	float4 position : SV_Position;

	float3 position_VS : Position0;

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
	in SkinnedVertex vertex_in,
	out VS_Out vertex_out_
	)
{
	const float3 local_position = vertex_in.position;
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = -UnpackVertexNormal( vertex_in.tangent );
	
	vertex_out_.position = Pos_Local_To_Clip( float4( local_position, 1 ) );
	
	vertex_out_.position_VS = Pos_Local_To_View( float4( local_position, 1 ) ).xyz;

	//
	vertex_out_.texCoord = vertex_in.texCoord;

	//
	vertex_out_.normal_in_view_space = Dir_Local_To_View( local_normal );
	vertex_out_.tangent_in_view_space = Dir_Local_To_View( local_tangent );
	vertex_out_.bitangent_in_view_space = cross( vertex_out_.tangent_in_view_space, vertex_out_.normal_in_view_space );

	vertex_out_.normal_in_view_space = normalize( vertex_out_.normal_in_view_space );
	vertex_out_.tangent_in_view_space = normalize( vertex_out_.tangent_in_view_space );
	vertex_out_.bitangent_in_view_space = normalize( vertex_out_.bitangent_in_view_space );
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
	// Get base color/diffuse/albedo.
	const float4 sampled_base_color = t_base_color.Sample(
		s_base_color,
		pixel_in.texCoord
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
		pixel_in.texCoord
	).rgb;


	const float sampled_occlusion = sampled_roughness_metallic.r;
	const float sampled_roughness = sampled_roughness_metallic.g;
	const float sampled_metalness = sampled_roughness_metallic.b;


	//

#if ENABLE_NORMAL_MAP

	const float3 sampled_normal_in_tangent_space = ExpandNormal(
		t_normals.Sample(
			s_normals,
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






#if USE_GLTF_IMPL

	// Shade surface

	const float3 u_LightColor =
		g_directional_light.color.rgb
		//float3(1,1,1)
		//float3(1,0,0)
		;

	const float3 light_vector_VS =
		g_directional_light.light_vector_VS.xyz
		//mul( g_view_matrix, float4(float3(0,0,1), 0) ).xyz
		//-mul( g_view_matrix, float4(float3(1,0,0), 0) ).xyz
		;

	//const float4 u_BaseColorFactor	= 1;
	//const float u_MetallicFactor	= 1;
	//const float u_RoughnessFactor	= 1;

	PBR_Material_glTF	material_info;
	material_info.setDefaults();


	//
	const float4 baseColor			= linear_base_color;
	const float ambientOcclusion	= sampled_roughness_metallic.r;
	/*const*/ float perceptualRoughness = sampled_roughness_metallic.g * material_info.roughnessFactor;
	/*const*/ float metallic			= sampled_roughness_metallic.b * material_info.metallicFactor;
	//
//perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
//metallic = clamp(metallic, 0.0, 1.0);





//perceptualRoughness = 0.1;



	//
	const float3 f0 = (float3) c_MinRoughness;
	const float3 diffuseColor0 = baseColor.rgb * ((float3)1.0 - f0);
	const float3 diffuseColor = diffuseColor0 * ( 1.0 - metallic );

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	const float alphaRoughness = perceptualRoughness * perceptualRoughness;

	//
	const float3 specularColor = lerp(f0, baseColor.rgb, metallic);


	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	float3 specularEnvironmentR0 = specularColor.rgb;
	float3 specularEnvironmentR90 = float3(1.0, 1.0, 1.0) * reflectance90;

	float3 n = pixel_normal_in_view_space;
	
	float3 v = normalize(-pixel_in.position_VS);    // Vector from surface point to camera
	float3 l = normalize(light_vector_VS);			// Vector from surface point to light
	float3 h = normalize(l+v);                        // Half vector between both l and v


	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);


	PBRInfo pbrInputs;

	pbrInputs.NdotL = NdotL;                  // cos angle between normal and light direction
	pbrInputs.NdotV = NdotV;                  // cos angle between normal and view direction
	pbrInputs.NdotH = NdotH;                  // cos angle between normal and half vector
	pbrInputs.LdotH = LdotH;                  // cos angle between light direction and half vector
	pbrInputs.VdotH = VdotH;                  // cos angle between view direction and half vector
	pbrInputs.perceptualRoughness = perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	pbrInputs.metalness = metallic;              // metallic value at the surface
	pbrInputs.reflectance0 = specularEnvironmentR0;            // full reflectance color (normal incidence angle)
	pbrInputs.reflectance90 = specularEnvironmentR90;           // reflectance color at grazing angle
	pbrInputs.alphaRoughness = alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	pbrInputs.diffuseColor = diffuseColor;            // color contribution from diffuse lighting
	pbrInputs.specularColor = specularColor;           // color contribution from specular lighting


	// Calculate the shading terms for the microfacet specular shading model
	float3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	// Calculation of analytical lighting contribution
	float3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	float3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	float3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);



#if 0
	float3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0f;
	// Calculate lighting contribution from image based lighting source (IBL)
	color += getIBLContribution(pbrInputs, n, reflection);
#endif

	

	// Apply optional PBR terms for additional (optional) shading
#if ENABLE_AMBIENT_OCCLUSION
	const float u_OcclusionStrength = 1.0f;
	float ao = ambientOcclusion;
	color = lerp(color, color * ao, u_OcclusionStrength);
#endif // ENABLE_AMBIENT_OCCLUSION

	//const float u_EmissiveFactor = 1.0f;
	//if (material.emissiveTextureSet > -1) {
	//	float3 emissive = SRGBtoLINEAR(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb * u_EmissiveFactor;
	//	color += emissive;
	//}


#endif
	


	//

#if __PASS == FillGBuffer

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
	GBuffer_Write( surface, pixel_out_ );

#else



	#if !USE_GLTF_IMPL

		DirectLighting	direct_lighting;
		{
			direct_lighting.surface_position		= pixel_in.position_VS;
			direct_lighting.surface_normal			= pixel_normal_in_view_space;
			direct_lighting.surface_to_eye_direction	= -normalize(pixel_in.position_VS);

			direct_lighting.surface_diffuse_albedo	= linear_base_color.rgb;
			direct_lighting.surface_specular_albedo	= WHITE.rgb * 0.1f;
			direct_lighting.surface_roughness		= sampled_roughness;
			direct_lighting.surface_metalness		= sampled_metalness;

			direct_lighting.surface_occlusion		= sampled_occlusion;
		}

		//
		pixel_out_ = direct_lighting.computeDirectionalLighting(
			g_directional_light.color.rgb,
			g_directional_light.light_vector_VS.xyz
		);

		//pixel_out_ += (sampled_base_color.rgb + sampled_roughness_metallic.rgb) * 1e-6;

		//pixel_out_ = LINEARtoSRGB( pixel_out_ );

		//pixel_out_ = (float3) direct_lighting.surface_occlusion + pixel_out_*1e-5;

	#else

		//pixel_out_ = float4(color, baseColor.a);
		
		//pixel_out_ = LINEARtoSRGB( color );//wrong!
		pixel_out_ = color;

	#endif

#endif

}
