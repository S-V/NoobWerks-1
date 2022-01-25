// Global shader parameters.
// This is a shared header file included by both host application (C++) code and shader code (HLSL).
// 

#ifdef __cplusplus
#pragma once
#endif

#ifndef NW_SHARED_GLOBALS_H
#define NW_SHARED_GLOBALS_H

#include "__shader_interop_pre.h"
#include "nw_shared_definitions.h"




/*===============================================
		Global shader constants.
===============================================*/

// Shader constant registers that are reserved by the engine.

/// Per-frame constants.
DECLARE_CBUFFER( G_PerFrame, 0 )
{
	// viewport- and time-dependent variables
	//float4	screenSize_invSize;	// xy - backbuffer dimensions, zw - inverse viewport size
	// x - globalTimeInSeconds;	// Time parameter, in seconds. This keeps increasing.

	// Time (t = time since current level load) values from Unity
	float4 g_Time; // (t, t*2, t*3, t/20)
	float4 g_SinTime; // sin(t/8), sin(t/4), sin(t/2), sin(t)
	float4 g_CosTime; // cos(t/8), cos(t/4), cos(t/2), cos(t)
	float4 g_DeltaTime; // dt, 1/dt, smoothdt, 1/smoothdt (in seconds)

	// x,y - mouse cursor position; z - is LMB pressed? w - is RMB held?
	uint4 g_Mouse;


	//=== Scene-Wide Settings:

	float4	g_PBR_params;	// metalness, roughness, 0, 0

	float PBR_metalness()			{ return g_PBR_params.x; }
	float PBR_roughness()			{ return g_PBR_params.y; }


	float4	g_ambientColor;



	// High Dynamic Range and Bloom parameters:
	// x - bloom_threshold, y - bloom_scale, z - exposure_key, w - adaptation rate
	float4	g_HDR_params;
#ifndef __cplusplus
	#define g_fBloomThreshold (g_HDR_params.x)
	#define g_fBloomScale (g_HDR_params.y)
	#define g_fExposureKey (g_HDR_params.z)
	#define g_fHdrAdaptationRate (g_HDR_params.w)
#endif

	//w - middle_gray
/*
	// Depth Of Field

	// x - Focal plane distance, y - Near blur plane distance,
	// z - Far blur plane distance, w - Far blur limit [0,1]
	// g_fFocalPlaneDistance, g_fNearBlurPlaneDistance, g_fFarBlurPlaneDistance, g_fFarBlurLimit
	float4	DoF_params;

	// Subsurface scattering
	// x - SSS width,
	float4	SSS_params;
*/


	// Environment:

	/// x - AO scale (intensity), y - AO power (exponent)
	float4	g_AO_params;
#define g_AO_scale	g_AO_params.x
#define g_AO_power	g_AO_params.y
};



/// Per-camera constants.
DECLARE_CBUFFER( G_PerCamera, 1 )
{
	// row-major so that post-multiply with float4 turns into 4 dp4s (the app transposes these matrices before passing to shader)
	row_major float4x4	g_view_matrix;
	row_major float4x4	g_view_projection_matrix;
	row_major float4x4	g_inverse_view_matrix;	//!< the camera's world matrix
	row_major float4x4	g_projection_matrix;		//!< needed for infinite, skybox rendering
	row_major float4x4	g_inverse_projection_matrix;	//!< used for restoring view space depth from hardware depth
	row_major float4x4	g_inverse_view_projection_matrix;
	//row_major float4x4	g_inverseTransposeViewMatrix;

	// x - framebuffer width, y - framebuffer height, zw - inverse width and height (texel size)
	float4	g_framebuffer_dimensions;

	///@TODO: get rid of far clipping plane!
	/// x = nearZ, y = farZ, z = invNearZ, w = invFarZ
	float4	g_DepthClipPlanes;

	/// x = tan( 0.5 * horizFOV ), y = tan( 0.5 * vertFOV ), z = aspect ratio (width/height), w = 1
	float4		g_viewport_params;	// used for restoring view-space position
#ifndef __cplusplus
	float g_viewport_tanHalfFoVy() { return g_viewport_params.y; }
	float g_viewport_aspectRatio() { return g_viewport_params.z; }
#endif


	// for debugging only!
	float4	g_camera_origin_WS;	// eye position in world space
	float4	g_camera_right_WS;	// Z axis
	float4	g_camera_fwd_WS;	// Y axis
	float4	g_camera_up_WS;		// Z axis

	/// Hole info for visual CSG: xyz - the sphere's center in view space, w - squared radius
	//float4	g_sphere_hole;

	///// Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
	///// x = 1-far/near
	///// y = far/near
	///// z = x/far
	///// w = y/far
	//float4	g_ZBufferParams;

	///// x = H, y = V, z = A, w = B
	//float4 g_ProjParams;

	///// x = 1/H
	///// y = 1/V
	///// z = 1/B
	///// w = -A/B
	//float4 g_ProjParams2;


	//float4	g_terrainCamera;	//!< camera for selecting LoD - for debugging geomorphing/CLOD

	/// Multipliers for computing LoD blending coefficient (see Proland documentation, '3.2.1. Distance-based subdivision': http://proland.inrialpes.fr)
	/// x = k, y = k+1, z = 1/(k-1), w - morph factor for debugging
	//float4	g_distanceFactors;
};



/// Per-object constants.
//TODO: allocate from CB pool!
DECLARE_CBUFFER( G_PerObject, 2 )
{
	row_major float4x4	g_world_view_projection_matrix;
	row_major float4x4	g_world_view_matrix;
	row_major float4x4	g_world_matrix;	// object-to-world transformation matrix

//	uint4				g_object_uint4_params;
//

	//PACK_MATRIX float4x4	g_worldMatrixIT;		// transpose of the inverse of the world matrix
	//PACK_MATRIX float4x4	g_worldViewMatrixIT;	// transpose of the inverse of the world-view matrix

	//// parameters for geomorphing/CLOD
	//float4		g_morphInfo;	// x = start morph distance (x>=0), y = end morph distance (y<=1), z - morph override (if >= 0), w - LoD level.
	//uint4		g_adjacency;	// x = bitmask: which adjacent nodes (ECellType) are 2x bigger.

	//// each vertex component is decompressed with the following equation:
	//// out = float3( in + offset ) * scale + bias
	//float4		g_objectScale;
	//float4		g_objectBias;

	//// for de-quantizing coarse positions (at the next LoD):
	//// de-quantize to parent node's space then add bias
	//float4		g_parentScale;
	//float4		g_parentBias;
};

//===========================================================


/// Global, scene-wide lighting parameters.
DECLARE_CBUFFER( G_Lighting, 3 )
{
	GlobalDirectionalLight	g_directional_light;
};

//===========================================================

// must be synced with nwRENDERER_MAX_BONES in app code!
#define MAX_BONES_LIMIT	(128)

DECLARE_CBUFFER( G_SkinningData, 4 )
{
	/** skinning matrices stored in 3x4 order */
	row_major float4x4	g_bone_matrices[MAX_BONES_LIMIT];
};

//===========================================================

/// this constant buffer should be updated very rarely
DECLARE_CBUFFER( G_VoxelTerrain, 7 )
{
	VoxelMaterial	g_voxel_materials[256];
};

#define INSTANCE_CBUFFER_SLOT	7

#endif // NW_SHARED_GLOBALS_H
