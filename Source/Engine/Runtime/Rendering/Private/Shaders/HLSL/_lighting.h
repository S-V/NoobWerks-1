#ifndef __LIGHTING_H__
#define __LIGHTING_H__

#include "base.hlsli"

static const float3 DEFAULT_AMBIENT = float3(0.4945, 0.465, 0.5);

/*
	N - Surface normal.
	L - Light vector - normalized vector opposite the direction of the light.
	R - Reflection vector.
	E - Eye vector - normalized vector from surface point to eye position.


	//In the preceding formula, R is the reflection vector being computed,
	//E is the normalized position-to-eye vector, and N is the camera-space vertex normal.
*/

//#define WRAP_DIFFUSE_LIGHTING


// Energy-Conserving Wrapped Diffuse
// W must be between 0 and 1
float WrapDiffuse( float NdotL, float W )
{
	return saturate( NdotL * W + (1 - W) );
}

// Intensity of the diffuse light. Saturate to keep within the 0-1 range.
// L - normalized surface to light vector
// N - surface normal
float CalculateDiffuseLightIntensity( float3 L, float3 N )
{
	float NdotL = dot( N, L );

	float intensity;
	
#ifdef WRAP_DIFFUSE_LIGHTING
	intensity = WrapDiffuse( NdotL );
#else //! WRAP_DIFFUSE_LIGHTING
	intensity = saturate( NdotL );
#endif //! WRAP_DIFFUSE_LIGHTING

	return intensity;
}

// L - normalized surface to light vector
// N - surface normal
//
float3 EvaluateLighting( float3 N, float3 L, float shadowAmount, float3 lightColor, float attenuation )
{
	float diffuseLightIntensity = CalculateDiffuseLightIntensity( L, N );
	float3 lightResult = lightColor * diffuseLightIntensity;
	return lightResult * shadowAmount * attenuation;
}

// N - surface normal (normalized)
// L - light vector (surface-to-light)
float3 Phong_DiffuseLighting( in float3 N, in float3 L, in float3 color )
{
	float NdotL = dot( N, L );
	float angleAttenuation = max( 0, NdotL );
	return color * angleAttenuation;
}






float3 Phong_PointLight( in float3 N, in float3 L, in float d, in float3 color, in float lightRadius )
{
	float kc = 1.0; //constant attenuation factor
	float kl = 2.0 / lightRadius; //linear attenuation factor
	float kq = 1.0 / (lightRadius * lightRadius); //quadratic attenuation factor
	float attenuation = 1.0 / (kc + kl * d + kq * d * d);
	return color * (attenuation * CalculateDiffuseLightIntensity( L, N ))
	;
}


/*
	accepts dot product of light vector and surface normal
*/
float3 HemisphereLighting( float3 albedo, float NdotL )
{
	return albedo * squaref( 0.5 + 0.5 * NdotL );
}
float3 HemisphereLight( float3 skyColor, float3 groundColor, float NdotL )
{
	// determine the sky/ground factor
	float factor = NdotL;
	return lerp( skyColor, groundColor, factor );
}
mxSTOLEN("CryEngine")
// - Blinn has good properties for plastic and some metallic surfaces. 
// - Good for general use. Very cheap.
// N - surface normal
// V - view vector (from surface towards the eye)
// L - light vector
// Exp - specular exponent
float BlinnSpecularIntensity( float3 N, float3 V, float3 L, float Exp )
{
	// compute half-vector
	float3 H = normalize( V + L );			// 4 alu
	return pow( saturate( dot(N, H) ), Exp );	// 4 alu
	// 8 ALU
}

/*
// Compute fresnel term, assumes NdotI comes clamped
half Fresnel( in half NdotI, in half bias, in half power)
{
  half facing = (1.0 - NdotI);
  return bias + (1.0-bias)*pow(facing, power);  
}
*/


// See: "Physically Based Shading Models in Film and Game Production: Practical Implementation at tri-Ace"
// f0 is the normal specular reflectance.
// This can be computed with: f0 = ((1-n)/(1+n))^2, where n is the complex refractive index.
// For typical dielectric materials, n is between 1.3 and 1.7
//
float SchlickFresnel_f0( in float3 half, in float3 view, in float f0 )
{
	float base = 1.0 - dot(view, half);
	float exponential = pow(base, 5.0);
	return exponential + f0 * (1.0 - exponential);
}

// See: http://renderwonk.com/publications/s2010-shading-course/hoffman/s2010_physically_based_shading_hoffman_b_notes.pdf

float SchlickFresnel( float energy, float cosine )
{
	return energy + (1-energy)*(1 - pow( cosine, 5 ));
}

float3 ComputeSpecular( float3 L, float3 V, float3 N, float3 LightColor, float power )
{
	float3 H = normalize( L - V );

	float dotLH = saturate( dot( L, H ));
	float dotNH = saturate( dot( N, H ));
	float dotNL = saturate( dot( N, L ));

	float3 strength = SchlickFresnel( 0.04, dotLH );

	float3 specular = pow( dotNH, power )*strength;
	specular *= power*0.125 + 0.25;
	specular *= dotNL*LightColor;

	return specular;
}

/*
//
//	SParallelLight
//
struct SParallelLight
{
	float4	lightVector;	// normalized lightVector ( light vector = -1 * direction )
	float4	albedo;
	float4	specularColor;
};

float3 DirectionalLight( in SurfaceData surface, in SParallelLight light )
{
	float3 outputColor = 0.0f;

	// diffuse
	float3 lightVec = light.lightVector.xyz;
	float NdotL = saturate(dot( surface.normal, lightVec ));

	outputColor += surface.albedo.rgb * light.albedo.rgb * NdotL;

	// compute surface specular term

	// Blinn-Phong
	const float3 halfVector = normalize(normalize( -surface.position ) + lightVec);

	float specularFactor = pow( abs(dot( surface.normal, halfVector )), surface.specularPower );
	float3 specularColor = (surface.specularIntensity * light.specularColor.rgb) * specularFactor;

	outputColor += specularColor;

	return outputColor;
}
*/



/*
// from Serious Engine (2011):
// calculates omni-light intensity
//
float CalcPointLightIntensity( in float3 pixelPos, in float3 lightPos, in float lightRange )
{
  // distance attenuation
  float3 vDist   = pixelPos -  lightPos;
  float  fDistSqr  = dot( vDist, vDist );
  float  fDist = rsqrt( fDistSqr );
  float  fFactor = 1- saturate( 1 / (lightRange * fDist));
  fFactor /= fDistSqr;
 return fFactor;
}
*/

// linear falloff instead of quadratic gives a softer look
//
float CalcPointLightAttenuation(
	in float3 pixelPos, in float3 N,
	in float3 lightPos, in float lightRange, in float invLightRange )
{
	float3 L = lightPos.xyz - pixelPos.xyz;
	float distance = length(L);
	L /= distance;

	float NdotL = saturate(dot(N,L));
	float attenuation = saturate( 1.0f - distance * invLightRange ) * NdotL;

	return attenuation;
}

#if 0
//Diffuse lighting, Normalized lambert
float3 Diffuse(float3 albedo)
{
	return albedo / PI;
}
//Geometric visibility term, Slick
float GeometricVisibility(float roughness, float NoV, float NoL)
{
	float k = roughness * roughness * 0.5;

	float GtermV = NoV * (1.0 - k) + k;
	float GtermL = NoL * (1.0 - k) + k;
	return 0.25 / (GtermV * GtermL);
}
//Distribution, Normalized Blinn-Phong
float Distribution_BlinnPhong(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	return rcp(PI * a2) * pow(NoH, 2.0/a2 - 2.0);
}
//Distribution, Trowbridge-Reitz GGX
float Distribution(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float t1 = NoH * NoH * (a2 - 1.0) + 1.0;
	float t2 = PI * t1 * t1;
	return a2 / t2;
}

//Fresnel term, Slick's approximation
float3 Fresnel(float3 specColor, float VoH)
{
	//Raise to power of 5
	float VoH1 = 1.0 - VoH;
	float VoH5 = VoH1 * VoH1 * VoH1 * VoH1 * VoH1;
	return specColor + (1.0 - specColor) * VoH5;
}

float3 Specular(float3 specularColor, float roughness, float NoV, float NoL, float NoH, float VoH)
{
	float G = GeometricVisibility(roughness, NoV, NoL);
	float3 F = Fresnel(specularColor, VoH);
	float D = Distribution(roughness, NoH);
	return G * D * F;
}
#endif


// Computes the visibility term by performing the shadow test.
// Returns [0..1) if the pixel is shadowed, 1 if the pixel is not in shadow.
float getCascadeShadowOcclusion( in float3 view_space_position
								,in float4x4 view_to_shadow_texture_space
								,Texture2D shadowDepthMap
								,SamplerComparisonState bilinearPcfSampler
								)
{
	//NOTE: no division by transformed .w, because it's an orthographic projection
	const float3 computed_position_in_shadow_map = mul( view_to_shadow_texture_space, float4( view_space_position, 1 ) ).xyz;

	// the (biased) pixel's depth from the light's view computed in the shader - it will be compared to the depth stored in the shadowmap
	const float computed_depth_in_shadow_map = computed_position_in_shadow_map.z;

	// how much is in shadow? (cheap hardware bilinear PCF)
	const float shadow_occlusion = shadowDepthMap.SampleCmpLevelZero(
		bilinearPcfSampler,
		computed_position_in_shadow_map.xy,	// texture coords within the shadow atlas
		computed_depth_in_shadow_map
	).x;	// 1 == not in shadow
	
	return shadow_occlusion;
}


#ifdef NW_SHARED_GLOBALS_H

	int selectTightestShadowCascade( in GlobalDirectionalLight directionalLight
									,in float4 pixelPosInLightViewSpace
									,in int cascadeCount
									)
	{
		int iTightestCascade = cascadeCount - 1;

		[unroll]
		for( int iCascade = cascadeCount - 1; iCascade >= 0; iCascade-- )
		{
			/*
			// this always works, but somewhat costly (4 dp's)
			float3 projectedPosInShadowMapTexture = mul( directionalLight.camera_view_space_to_shadow_cascade_projection[iCascade], float4(pixelPosInLightViewSpace) ).xyz;	// works
			*/

			/*
			// NOTE: the following line is wrong! We use a right-handed Z-up coordinate system
			// while DirectX uses left-handed Y-up.
			// We transform from the light's view space (our coords) into the cascade's projection space (Direct3D's coords).
			// So, we must swap Y and Z axes.
			//projectedPosInShadowMapTexture = pixelPosInLightViewSpace.xyz * directionalLight.cascade_scale[iCascade].xyz;
			
			float3 projectedPosInShadowMapTexture = float3(
				pixelPosInLightViewSpace.x * directionalLight.cascade_scale[iCascade].x,
				pixelPosInLightViewSpace.z * directionalLight.cascade_scale[iCascade].y,
				pixelPosInLightViewSpace.y * directionalLight.cascade_scale[iCascade].z
			);
			projectedPosInShadowMapTexture += directionalLight.cascade_offset[iCascade].xyz;
			*/
			
			const float2 projectedPosInShadowMapTexture = float2(
				pixelPosInLightViewSpace.x * directionalLight.cascade_scale[iCascade].x,
				pixelPosInLightViewSpace.z * directionalLight.cascade_scale[iCascade].y
			)
			+ directionalLight.cascade_offset[iCascade].xy;



			/* works, but we use a cheaper version below:
			if( min( projectedPosInShadowMapTexture.x, projectedPosInShadowMapTexture.y ) >= 0
			&& max( projectedPosInShadowMapTexture.x, projectedPosInShadowMapTexture.y ) <= 1 )
			{
				iTightestCascade = iCascade;
			}
			*/
			if( all( abs( projectedPosInShadowMapTexture.xy - 0.5 ) < 0.5 ) )
			{
				iTightestCascade = iCascade;
			}
		}

		return iTightestCascade;
	}

#endif // __SHADER_COMMON_H__



// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/mp/src/materialsystem/stdshaders/common_vertexlitgeneric_dx9.h

// Better suited to Pixel shader models, 11 instructions in pixel shader
// ... actually, now only 9: mul, cmp, cmp, mul, mad, mad, mad, mad, mad
float3 PixelShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 linearColor, nSquared = worldNormal * worldNormal;
	float3 isNegative = ( worldNormal >= 0.0 ) ? 0 : nSquared;
	float3 isPositive = ( worldNormal >= 0.0 ) ? nSquared : 0;
	linearColor = isPositive.x * cAmbientCube[0] + isNegative.x * cAmbientCube[1] +
				  isPositive.y * cAmbientCube[2] + isNegative.y * cAmbientCube[3] +
				  isPositive.z * cAmbientCube[4] + isNegative.z * cAmbientCube[5];
	return linearColor;
}

// Better suited to Vertex shader models
// Six VS instructions due to use of constant indexing (slt, mova, mul, mul, mad, mad)
float3 VertexShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCube[isNegative.x] +
	              nSquared.y * cAmbientCube[isNegative.y+2] +
	              nSquared.z * cAmbientCube[isNegative.z+4];
	return linearColor;
}


#endif // __LIGHTING_H__
