#ifndef NW_GBUFFER_WRITE_H
#define NW_GBUFFER_WRITE_H

// HLSL utilities for writing into the geometry buffer.

// Contains common materials properties that are output by surface shaders.
// This structure is packed into geometry buffer.
struct Surface_to_GBuffer
{
	// [0..1]
	float3	albedo;	// aka diffuse color

	// normalized to [-1..+1]
	float3	normal;		// view-space normal



	float	metalness;		// 0=non-metal, 1=metal
	float	roughness;		// 0=smooth, 1=rough




	// NOT USED:


	/// light accessibility, obscurance, or 'openness' to ambient light
	float	accessibility;	// = ( 1 - ambient_occlusion ), 1 == unoccluded (default=1)

	/// [0..1], 1 = unshadowed, 0 - shadowed
	float	sun_visibility;



	

	// [0..1]
	float3	specular_color;
	float	specular_power;


	float3	emissive_color;
	float	opacity;		// 0..1






	///
	void setDefaults()
	{
		albedo = (float3) 1;

		normal = (float3) 0;

		metalness = 0;
		roughness = 1;

		accessibility = 1;
		sun_visibility = 1;	// not shadowed by default

		specular_color = (float3) 1;
		specular_power = 0;

		emissive_color = (float3) 0;
		opacity = 0;
	}
};

// This is what gets written into the geometry buffer.
struct PS_to_GBuffer
{
	float4	RT0	: SV_Target0;	// view-space normal + roughness
	float4	RT1	: SV_Target1;	// diffuse + metalness
	//float4	RT2 : SV_Target2;	// specular color and specular power
};

void GBuffer_Write(
				   in Surface_to_GBuffer surface
				   , out PS_to_GBuffer outputs_
				   )
{
	outputs_.RT0.xyz = surface.normal * 0.5f + 0.5f;	// [-1..1] -> [0..1]
	outputs_.RT0.w = surface.roughness;

	outputs_.RT1.rgb = surface.albedo;
	outputs_.RT1.a = surface.metalness;

	//outputs_.RT2.rgb = surface.specular_color;
	//surface.specular_power;
	//outputs_.RT2.a = surface.accessibility;

	//outputs_.RT2.r = surface.accessibility;
	//outputs_.RT2.g = surface.sun_visibility;
	//outputs_.RT2.ba = 0;
}

#endif // NW_GBUFFER_WRITE_H
