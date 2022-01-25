// 
#ifndef __SHADER_PBR_MATERIAL_H__
#define __SHADER_PBR_MATERIAL_H__

///
struct PBR_Material
{
	/**
	 The (linear) base color of this material.
	 Must be in [0..1] range.
	 This color would be the same as a color in a photograph taken with a polarizing  filter.
	 */
	float3	albedo;

	/**
	 The (linear) metalness of this material.
	 Describes how close to metal the material behaves when interacting with light.
	 If the surface area is metal, it just reflects  all the  light  cast  on  it
	 and  gets its specular  highlight  color  from  the  albedo map.
	 */
	float	metalness;

	/**
	 The (linear) roughness of this material.
	roughness = 1- glossiness
	 */
	float	roughness;
};

#endif // __SHADER_PBR_MATERIAL_H__

// References:
// 
// Unreal Engine 4:
// Physically Based Materials
// An overview of the primary Material inputs and how best to use them.
// https://docs.unrealengine.com/en-US/Engine/Rendering/Materials/PhysicallyBased/index.html
// 
// 
// Unity:
// Material charts
// Use these charts as reference for realistic settings:
// https://docs.unity3d.com/Manual/StandardShaderMaterialCharts.html
//