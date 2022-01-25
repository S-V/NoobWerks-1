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

#ifndef PBS_HLSLI
#define PBS_HLSLI

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
struct PBR_Surface
{
	float3	position;
	float3	normal;

	//
	float3	diffuse_albedo;
	float3	specular_albedo;

	float	roughness;
	float	metalness;

	// light accessibility, or 'openness' to ambient light (1 == unoccluded, 0 = occluded)
	float	accessibility;	// opposite of ambient occlusion
};


// Apply Disney-style physically-based rendering to the surface. 
// 
// numLights:        Number of directional lights. 
// 
// lightColor[]:     Color and intensity of directional light. 
// 
// lightDirection[]: (opposite to Light direction. 
float3 PBR_ComputeDirectionalLighting(
	in float3 lightColor

	// normalized vector from the surface point to the light source: normalize(-lightDirection)
	, in float3 light_vector
	
	// aka 'View Vector', 'V' = normalize( eye_pos - pixel_pos )
	, in float3 to_eye_direction
	
	, in PBR_Surface surface
	)
{
	// these values are constant wrt each light
	const float3 albedo = surface.diffuse_albedo;
	const float roughness = surface.roughness;
	const float metallic = surface.metalness;

	const float3 N = surface.normal;
	const float3 V = to_eye_direction;

	// Specular coefficient - fixed reflectance value for non-metals
	static const float kSpecularCoefficient = 0.04;

	const float NdotV = saturate(dot(N, V));

	// Burley roughness bias
	const float alpha = roughness * roughness;

	// Blend base colors
	const float3 c_diff = lerp(albedo, float3(0, 0, 0), metallic)       * surface.accessibility;
	const float3 c_spec = lerp(kSpecularCoefficient, albedo, metallic)  * surface.accessibility;

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


#endif // PBS_HLSLI
