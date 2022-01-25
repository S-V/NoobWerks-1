// Common definitions for deferred lighting.

#ifndef __DEFERRED_LIGHTING_H__
#define __DEFERRED_LIGHTING_H__

#include "_common.h"
#include "_gbuffer.h"
#include "_lighting.h"
#include "_screen_shader.h"

float3 ComputeDeferredLighting(
							   )
{
	//
}

#if 0
float3 DeferredLightRadialPS(in float4 in_screen_position : TEXCOORD0) : SV_Target0
{
	// Fetch GBuffer data
	float2 uv = in_screen_position.xy / in_screen_position.w * float2(0.5, -0.5) + 0.5;
	GBufferData data = GetGBufferData(uv);
	if(data.depth == 1.0) discard;

	// Calculate world position of the fragment we are about to shade
	float4 screen_position = float4(in_screen_position.xy / in_screen_position.w * data.scene_depth, data.scene_depth, 1.0);
	float4 homogenous_world_position = mul(View.screen_to_world, screen_position);
	float3 world_position = homogenous_world_position.xyz / homogenous_world_position.w;

	// Calculate camera to fragment vector
	float3 view = normalize(View.view_position - world_position);

	return GetDynamicLighting(data, world_position, view);
}

float3 GetDynamicLighting(in GBufferData data, in float3 world_position, in float3 V)
{
	// Lighting parameters
	float3 N = data.world_normal;

	#if defined(LIGHT_POINT) || defined(LIGHT_SPOT)
		float3 L = Light.position - world_position;
		float light_distance = length(L);
		L /= light_distance;

		// Make sure fragments outside light radius are not accidentally
		// shaded.
		if(light_distance > Light.radius) discard;
		#if defined(LIGHT_SPOT)
			if(dot(Light.direction, -L) < cos(Light.spot_angle)) discard;
		#endif

		float distance_attenuation = LightingGetPointIntensity(Light.radius, light_distance);
	#elif defined(LIGHT_DIRECTIONAL)
		float distance_attenuation = 1.0;
		float3 L = -Light.direction;
	#endif


	float3 H = normalize(V + L);
	float NoL = saturate(dot(N, L));
	float NoH = saturate(dot(N, H));
	float NoV = saturate(dot(N, V));
	float VoH = saturate(dot(V, H));

	// Diffuse lighting
	float3 diffuse = Diffuse(data.albedo_color);

	// Specular lighting
	float3 specular = Specular(data.specular_color, data.roughness, NoV, NoL, NoH, VoH);

	// add atenuation
	return (diffuse + specular) * Light.color * (NoL * distance_attenuation);
}
#endif

#endif // __DEFERRED_LIGHTING_H__
