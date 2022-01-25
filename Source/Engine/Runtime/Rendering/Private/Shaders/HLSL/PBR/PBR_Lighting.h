/*
This fragment shader defines an implementation for Physically Based Shading of
a microfacet surface material defined by a glTF model.

Heavily based on
https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag

https://github.com/microsoft/DirectXTK/blob/master/Src/Shaders/PBRCommon.fxh
https://github.com/microsoft/DirectXTK/blob/master/Src/Shaders/PBREffect.fx

(many links on github page)
https://github.com/Nadrin/PBR

References:
[1] Real Shading in Unreal Engine 4
    http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
[2] Physically Based Shading at Disney
    http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
[3] README.md - Environment Maps
    https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
[4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
    https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
*/

#ifndef __PBR_LIGHTING_SHADER_H__
#define __PBR_LIGHTING_SHADER_H__

#include <base.hlsli>

mxSTOLEN("https://github.com/steaklive/EveryRay-Rendering-Engine/blob/master/content/effects/PBR.fx")




static const float PI = 3.14159265f;
static const float EPSILON = 1e-6f;

// Shlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
float3 Fresnel_Shlick(in float3 f0, in float3 f90, in float x)
{
    return f0 + (f90 - f0) * pow(1.f - x, 5.f);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float Diffuse_Burley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2.f * roughness * LdotH * LdotH;
    return Fresnel_Shlick(1, fd90, NdotL).x * Fresnel_Shlick(1, fd90, NdotV).x;
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(in float alpha, in float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1)) + 1;
    return alpha2 / max(EPSILON, PI * lower * lower);
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
// http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html
float G_Shlick_Smith_Hable(float alpha, float LdotH)
{
    return rcp(lerp(LdotH * LdotH, 1, alpha * alpha * 0.25f));
}

// A microfacet based BRDF.
//
// alpha:           This is roughness * roughness as in the "Disney" PBR model by Burley et al.
//
// specularColor:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals. This follows model 
//                  used by Unreal Engine 4.
//
// NdotV, NdotL, LdotH, NdotH: vector relationships between,
//      N - surface normal
//      V - eye normal
//      L - light normal
//      H - half vector between L & V.
float3 Specular_BRDF(in float alpha, in float3 specularColor, in float NdotV, in float NdotL, in float LdotH, in float NdotH)
{
    // Specular D (microfacet normal distribution) component
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // Specular Fresnel
    float3 specular_F = Fresnel_Shlick(specularColor, 1, LdotH);

    // Specular G (visibility) component
    float specular_G = G_Shlick_Smith_Hable(alpha, LdotH);

    return specular_D * specular_F * specular_G;
}

/*
// Diffuse irradiance
float3 Diffuse_IBL(in float3 N)
{
    return IrradianceTexture.Sample(IBLSampler, N);
}

// Approximate specular image based lighting by sampling radiance map at lower mips 
// according to roughness, then modulating by Fresnel term. 
float3 Specular_IBL(in float3 N, in float3 V, in float lodBias)
{
    float mip = lodBias * NumRadianceMipLevels;
    float3 dir = reflect(-V, N);
    return RadianceTexture.SampleLevel(IBLSampler, dir, mip);
}
*/

///
struct DirectLighting
{
	float3	surface_position;
	float3	surface_normal;
	float3	surface_to_eye_direction;	// aka 'View Vector', 'V' = normalize( eye_pos - pixel_pos )

	//
	float3	surface_diffuse_albedo;
	float3	surface_specular_albedo;

	float	surface_roughness;
	float	surface_metalness;

	// 1 = occluded
	float	surface_occlusion;

	//

	// Apply Disney-style physically-based rendering to the surface. 
	// 
	// numLights:        Number of directional lights. 
	// 
	// lightColor[]:     Color and intensity of directional light. 
	// 
	// lightDirection[]: (opposite to Light direction. 
	float3 computeDirectionalLighting(
		in float3 lightColor,

		// normalized vector from the surface point to the light source: normalize(-lightDirection)
		in float3 light_vector
		)
	{
		// these values are constant wrt each light
		const float3 albedo = surface_diffuse_albedo;
		const float roughness = surface_roughness;
		const float metallic = surface_metalness;
		const float ambientOcclusion = surface_occlusion;

		const float3 N = surface_normal;
		const float3 V = surface_to_eye_direction;


		// Specular coefficient - fixed reflectance value for non-metals
		static const float kSpecularCoefficient = 0.04;

		const float NdotV = saturate(dot(N, V));

		// Burley roughness bias
		const float alpha = roughness * roughness;

		// Blend base colors
		const float3 c_diff = lerp(albedo, float3(0, 0, 0), metallic)       * ambientOcclusion;
		const float3 c_spec = lerp(kSpecularCoefficient, albedo, metallic)  * ambientOcclusion;


		//


		// Output color
		float3 acc_color = 0;                         

		// light vector (to light)
		const float3 L = light_vector;	// normalize(-lightDirection);

		// Half vector
		const float3 H = normalize(L + V);

		// products
		const float NdotL = saturate(dot(N, L));
		const float LdotH = saturate(dot(L, H));
		const float NdotH = saturate(dot(N, H));

		// Diffuse & specular factors
		float diffuse_factor = Diffuse_Burley(NdotL, NdotV, LdotH, roughness);
		float3 specular      = Specular_BRDF(alpha, c_spec, NdotV, NdotL, LdotH, NdotH);

		// Directional light
		acc_color += NdotL * lightColor * (((c_diff * diffuse_factor) + specular));


		return acc_color;
	}

/* 
		// Add diffuse irradiance
		float3 diffuse_env = Diffuse_IBL(N);
		acc_color += c_diff * diffuse_env;

		// Add specular radiance 
		float3 specular_env = Specular_IBL(N, V, roughness);
		acc_color += c_spec * specular_env;
*/



	/*
	///
	float3 computeDirectionalLight(
		in float3 light_vector	// normalized vector from the surface point to the light source
		)
	{
		float3 lighting = 0.0f;

		float nDotL = saturate(dot(surface_normal, light_vector));

		if (nDotL > 0.0f)
		{
			lighting
				= DirectDiffuseBRDF(surface_diffuse_albedo, nDotL)
				+ DirectSpecularBRDF(surface_specular_albedo, surface_position, surface_normal, light_vector, roughness)
				;
		}

		return max(lighting, 0.0f) * lightColor;
	}




	// ================================================================================================
	// Lambertian BRDF
	// http://en.wikipedia.org/wiki/Lambertian_reflectance
	// ================================================================================================
	float3 DirectDiffuseBRDF(float3 surface_diffuse_albedo, float nDotL)
	{
		return (surface_diffuse_albedo * nDotL) / M_PI;
	}


	// ================================================================================================
	// Cook-Torrance BRDF
	float3 DirectSpecularBRDF(
		float3 surface_specular_albedo
		, float3 surface_position
		, float3 surface_normal
		, float3 light_vector
		, float roughness
		)
	{
		float3 half_vector = normalize(surface_to_eye_direction + light_vector);

		float nDotH = saturate(dot(surface_normal, half_vector));
		float nDotL = saturate(dot(surface_normal, light_vector));
		float nDotV = max(dot(surface_normal, surface_to_eye_direction), 0.0001f);

		float alpha2 = roughness * roughness;

		// Normal distribution term with Trowbridge-Reitz GGX.
		float D = alpha2 / (M_PI * pow(nDotH * nDotH * (alpha2 - 1) + 1, 2.0f));

		// Fresnel term with Schlick's approximation.
		float3 F = Schlick_Fresnel(surface_specular_albedo, half_vector, light_vector);

		// Geometry term with Smith's approximation.
		float G = G_Smith(roughness, nDotV, nDotL);

		return D * F * G;
	}


	// ===============================================================================================
	// GGX microfacet distribution
	// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
	// ===============================================================================================
	float GGX(float NdotV, float a)
	{
		float k = a / 2;
		return NdotV / (NdotV * (1.0f - k) + k);
	}

	// ===============================================================================================
	// Geometry with Smith approximation:
	// http://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf
	// http://graphicrants.blogspot.fr/2013/08/specular-brdf-reference.html
	// ===============================================================================================
	float G_Smith(float a, float nDotV, float nDotL)
	{
		return GGX(nDotL, a * a) * GGX(nDotV, a * a);
	}

	// ================================================================================================
	// Fresnel with Schlick's approximation:
	// http://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf
	// http://graphicrants.blogspot.fr/2013/08/specular-brdf-reference.html
	// ================================================================================================
	float3 Schlick_Fresnel(float3 f0, float3 h, float3 l)
	{
		return f0 + (1.0f - f0) * pow((1.0f - dot(l, h)), 5.0f);
	}
	*/
};

#endif // __PBR_LIGHTING_SHADER_H__
