/*
This fragment shader defines a reference implementation for Physically Based Shading of
a microfacet surface material defined by a glTF model.

Based on
https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr_khr.frag

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

#ifndef __PBR_LIGHTING_GLTF_SHADER_H__
#define __PBR_LIGHTING_GLTF_SHADER_H__

#include <base.hlsli>



#if 0

mxSTOLEN("https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/functions.glsl")

const float c_MinReflectance = 0.04;

struct AngularInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector

    float VdotH;                  // cos angle between view direction and half vector

    float3 padding;
};


AngularInfo getAngularInfo(float3 pointToLight, float3 normal, float3 view)
{
    // Standard one-letter names
    float3 n = normalize(normal);           // Outward direction of surface point
    float3 v = normalize(view);             // Direction from surface point to view
    float3 l = normalize(pointToLight);     // Direction from surface point to light
    float3 h = normalize(l + v);            // Direction of the vector between l and v

    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    return AngularInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        float3(0, 0, 0)
    );
}

//////////////////////////////////////////////////////////////////////////

struct MaterialInfo
{
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float3 reflectance0;            // full reflectance color (normal incidence angle)

    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    float3 diffuseColor;            // color contribution from diffuse lighting

    float3 reflectance90;           // reflectance color at grazing angle
    float3 specularColor;           // color contribution from specular lighting
};


// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
float3 getIBLContribution(MaterialInfo materialInfo, float3 n, float3 v)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
    float3 reflection = normalize(reflect(-v, n));

    vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = texture2D(u_brdfLUT, brdfSamplePoint).rg;

    float4 diffuseSample = textureCube(u_DiffuseEnvSampler, n);

#ifdef USE_TEX_LOD
    float4 specularSample = textureCubeLodEXT(u_SpecularEnvSampler, reflection, lod);
#else
    float4 specularSample = textureCube(u_SpecularEnvSampler, reflection);
#endif

#ifdef USE_HDR
    // Already linear.
    float3 diffuseLight = diffuseSample.rgb;
    float3 specularLight = specularSample.rgb;
#else
    float3 diffuseLight = SRGBtoLINEAR(diffuseSample).rgb;
    float3 specularLight = SRGBtoLINEAR(specularSample).rgb;
#endif

    float3 diffuse = diffuseLight * materialInfo.diffuseColor;
    float3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

    return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
float3 diffuse(MaterialInfo materialInfo)
{
    return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float NdotL = angularInfo.NdotL;
    float NdotV = angularInfo.NdotV;
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
    float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

float3 getPointShade(float3 pointToLight, MaterialInfo materialInfo, float3 normal, float3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        float3 F = specularReflection(materialInfo, angularInfo);
        float Vis = visibilityOcclusion(materialInfo, angularInfo);
        float D = microfacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        float3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
        float3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return float3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            return smoothstep(outerConeCos, innerConeCos, actualCos);
        }
        return 1.0;
    }
    return 0.0;
}

float3 applyDirectionalLight(
						   float3 light_vector,	// surface to light
						   float3 light_color,
						   MaterialInfo materialInfo,
						   float3 normal,
						   float3 view
						   )
{
    float3 shade = getPointShade(light_vector, materialInfo, normal, view);
    return /*light.intensity **/ light_color * shade;
}

/*
float3 applyPointLight(Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.position - v_Position;
    float distance = length(pointToLight);
    float attenuation = getRangeAttenuation(light.range, distance);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

float3 applySpotLight(Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.position - v_Position;
    float distance = length(pointToLight);
    float rangeAttenuation = getRangeAttenuation(light.range, distance);
    float spotAttenuation = getSpotAttenuation(pointToLight, light.direction, light.outerConeCos, light.innerConeCos);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}
*/


///
struct DirectLighting_glTF
{
	float3	surface_position;
	float3	surface_normal;
	float3	surface_to_eye_direction;	// aka 'View Vector', 'V' = normalize( eye_pos - pixel_pos )

	//
	float3	surface_diffuse_albedo;
	float3	surface_specular_albedo;
	float	surface_roughness;
	float	surface_metalness;

	//
	float	surface_occlusion;

	//

	// Apply Disney-style physically based rendering to a surface with: 
	// 
	// numLights:        Number of directional lights. 
	// 
	// lightColor[]:     Color and intensity of directional light. 
	// 
	// lightDirection[]: Light direction. 
	float3 computeDirectionalLighting(
		in float3 light_color, in float3 light_vector
		)
	{
		const float4 u_BaseColorFactor	= 1;
		const float u_MetallicFactor	= 1;
		const float u_RoughnessFactor	= 1;

		const float3 baseColor				= surface_diffuse_albedo * u_BaseColorFactor;
		/*const*/ float perceptualRoughness	= surface_roughness * u_RoughnessFactor;
		const float metallic			= surface_metalness;
		const float ambientOcclusion	= surface_occlusion;




		// f0 = specular
		float3 f0 = float3(0.04);
		const float3 specularColor = f0;


		perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);

		float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
		float3 diffuseColor = baseColor.rgb * oneMinusSpecularStrength;



		// Roughness is authored as perceptual roughness; as is convention,
		// convert to material roughness by squaring the perceptual roughness [2].
		float alphaRoughness = perceptualRoughness * perceptualRoughness;

		// Compute reflectance.
		float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

		float3 specularEnvironmentR0 = specularColor.rgb;
		// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
		float3 specularEnvironmentR90 = float3(clamp(reflectance * 50.0, 0.0, 1.0));

		//
		MaterialInfo materialInfo = MaterialInfo(
			perceptualRoughness,
			specularEnvironmentR0,
			alphaRoughness,
			diffuseColor,
			specularEnvironmentR90,
			specularColor
			);


		//
		float3 color = (float3) 0;

		color += applyDirectionalLight(
			light_vector,
			light_color,
			materialInfo,
			surface_normal,
			surface_to_eye_direction
			);

    // Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
    color += getIBLContribution(materialInfo, normal, view);
#endif

/*
		// Add diffuse irradiance
		float3 diffuse_env = Diffuse_IBL(N);
		acc_color += c_diff * diffuse_env;

		// Add specular radiance 
		float3 specular_env = Specular_IBL(N, V, roughness);
		acc_color += c_spec * specular_env;
*/
		return color;
	}
};



#endif














// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	float3 reflectance0;            // full reflectance color (normal incidence angle)
	float3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	float3 diffuseColor;            // color contribution from diffuse lighting
	float3 specularColor;           // color contribution from specular lighting
};

static const float c_MinRoughness = 0.04;


// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
float3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
}

// Gets metallic factor from specular glossiness workflow inputs 
float convertMetallic(float3 diffuse, float3 specular, float maxSpecular) {
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < c_MinRoughness) {
		return 0.0;
	}
	float a = c_MinRoughness;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;
	float c = c_MinRoughness - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

















struct PBR_Material_glTF
{
	float4 baseColorFactor;
	//float4 emissiveFactor;
	float4 diffuseFactor;
	float4 specularFactor;
	//float workflow;

	float metallicFactor;	
	float roughnessFactor;	
	//float alphaMask;	
	//float alphaMaskCutoff;

	void setDefaults()
	{
		baseColorFactor	= 1;
		diffuseFactor	= 1;
		specularFactor	= 1;

		metallicFactor	= 1;
		roughnessFactor	= 1;
	}
};


#endif // __PBR_LIGHTING_GLTF_SHADER_H__
