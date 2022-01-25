// This is a shared header file included by both host application (C++) code and shader code (HLSL).

#ifdef __cplusplus
#pragma once
#endif

#ifndef NW_SHADER_INTEROP_SHARED_DEFINITIONS_H
#define NW_SHADER_INTEROP_SHARED_DEFINITIONS_H


#include "__shader_interop_pre.h"


// Must be kept in sync with C++ code!


// It contains common definitions used by tile-based renderers (e.g. Tiled Deferred Shading, Forward+).

/// should be the default
#define USE_REVERSED_DEPTH		(1)

/// maximum number of view volume splits for CSM/PSSM
#define GPU_MAX_SHADOW_CASCADES		(4)



//#define MAX_LOCAL_LIGHTS	(4)

/// the resolution of each cubemap face for generating light probes using voxel cone tracing
/// 8x8 is good enough for diffuse GI
#define GPU_LIGHTPROBE_CUBEMAP_RES	(8)

#define GPU_NUM_CUBEMAP_FACES		(6)



/*
===============================================================================

common definitions used by shadow renderers

===============================================================================
*/


//// size of (square) shadow map for each cascade
////#define SHADOW_MAP_SIZE			256
////#define SHADOW_MAP_SIZE			512
//#define SHADOW_MAP_SIZE			(1024)
//
//#define INV_SHADOW_MAP_SIZE		(1.0f/SHADOW_MAP_SIZE)


/*
===============================================================================

	LIGHTING

===============================================================================
*/




///
struct SkyAmbientCube
{
	float4	irradiance_posX;
	float4	irradiance_negX;

	float4	irradiance_posY;
	float4	irradiance_negY;

	float4	irradiance_posZ;
	float4	irradiance_negZ;
};

///
struct GlobalDirectionalLight
{
	float4		light_vector_VS;	//!< normalized light vector in view space ( L = -1 * direction )
	float4		light_vector_WS;	//!< normalized light vector in world space ( L = -1 * direction )

	//float4	halfVectorVS;	//!< normalized half vector in view space ( H = (L + E)/2 or H = normalize(L+ E) )
	float4		color;

	float4		padding[1];

	// view-space center and half-size of each cubic cascade (shadow cascades are nested, like ClipBoxes in voxel terrain rendering);
	// shadow maps for each cascade are cached
//	float4		cascades[4];

//cascade selection stuff:
	// transforms from the camera's view space into the light's view space
	// (all cascades are centered around the viewer and have the same view matrix)
	row_major float4x4	camera_to_light_view_space;

	float4				cascade_scale[GPU_MAX_SHADOW_CASCADES];	// only xyz are used
    float4				cascade_offset[GPU_MAX_SHADOW_CASCADES];// only xyz are used

	// view space -> shadow map texture space
	row_major float4x4	view_to_shadow_texture_space[GPU_MAX_SHADOW_CASCADES];
};

struct LocalLight
{
	float4		origin;	//!< the light's position in view space

	// x - light radius,
	// y - inverse light radius/range,
	float4		radius_invRadius;

	// common light data (both point and spotlights)
	float4		diffuseColor;
	float4		specularColor;

	// spot light data
	float4		spotDirection;		// normalized axis direction in view space
	// x = cosTheta (cosine of half inner cone angle),
	// y = cosPhi (cosine of half outer cone angle),
	// z = projectorIntensity,
	// w = shadowDepthBias to reduce self-shadowing (we'll get shadow acne on all surfaces without this bias)
	float4		spotLightAngles;

	/// for transforming light volume meshes in the vertex shader during deferred lighting
//	row_major float4x4	lightShapeTransform;	// for spotlights

	/// The combined 'inverseView * lightViewProjection * clipToTextureSpace' matrix
	/// used for shadow mapping/projective texture mapping
	row_major float4x4	eyeToLightProjection;	// for spotlights
};

/*
/// The aerial fog settings.
struct SHeightFogData
{
	float3	color;
	float	density;

	float	startDistance;	//!< start distance
	float	extinctionDistance;	//!< end distance
	float	minHeight;
	float	maxHeight;
};

//DECLARE_CBUFFER( G_PostProcessing, 5 )
//{
//	ShHeightFogData		fog;
//};

/// Parameters for analytic sky models.
struct SkyParameters
{
	// Vertex shader uniforms:
	row_major float4x4	worldViewProjection;

	// Pixel shader uniforms:
	float4	sunDirection;
	//NB: For Preetham, only A, B, C, D, E and Z are used.
	float4	A, B, C, D, E, F, G, H, I, Z;
};


//
//cbuffer SkyIrradiance
//{
//	SkyAmbientCube	u_sky_ambient_cube;
//};

*/


/*
===============================================================================

Definitions used by tile-based renderers (e.g. Tiled Deferred Shading, Forward+)

===============================================================================
*/

//-----------------------------------------------------------------------------
// Compile-time configuration
//-----------------------------------------------------------------------------

// build view-space frustum planes
#define RR_BUILD_TILE_FRUSTA_ON_CPU	(0)

//-----------------------------------------------------------------------------
// Light culling constants: Parameters for the light culling shader.
//-----------------------------------------------------------------------------

#define MAX_LIGHTS_POWER 10
#define MAX_LIGHTS (1<<MAX_LIGHTS_POWER)


#define MAX_LIGHTS_PER_TILE (1024)


// Tile dimensions determine the tile size for light binning and associated trade-offs.
#define TILE_SIZE_X (32)
#define TILE_SIZE_Y (32)

// number of threads in a thread group (each thread corresponds to one pixel)
#define THREADS_PER_TILE	(TILE_SIZE_X*TILE_SIZE_Y)


//--------------------------------------------------------------------------------------
//	Constant Buffers
//--------------------------------------------------------------------------------------

CPP_PREALIGN_CBUFFER struct DeferredLight
{
	float4 position_and_radius;	// view-space light position and radius
	float4 color_and_inv_radius;	// light color and inverse radius
};

cbuffer TiledShadingConstants
{
	// x - number of horizontal tiles, y - vertical tiles (for tiled deferred/forward shading)
	uint4	g_tileCount;

	// x - number of dynamic point lights
	uint4	g_deferredLightCount;

#if RR_BUILD_TILE_FRUSTA_ON_CPU
	// frustum planes in view space (built on CPU)
	float4	g_horizontalFrustumPlanes[1024];
	float4	g_verticalFrustumPlanes[1024];
#endif
};

//--------------------------------------------------------------------------------------
//	Debugging
//--------------------------------------------------------------------------------------


// emit tile frustum corners (via AppendStructuredBuffer.append())
#define DEBUG_EMIT_POINTS	(0)


#if DEBUG_EMIT_POINTS

	struct DebugPoint
	{
		//float4	position;
		uint4	groupId;
		//uint4	additional;	
		//float4	color;
		//float4	depthRange;
	};
	HLSL_CODE(
		AppendStructuredBuffer< DebugPoint >	u_debugPoints;
	)

#endif



#define DEBUG_EMIT_LINES	(0)

#if DEBUG_EMIT_LINES

	struct DebugLine
	{
		float4	start;
		float4	end;
	};
	AppendStructuredBuffer< DebugLine >	u_debugLines;

#endif


#if 0
	struct DebugInt
	{
		uint2	position;
		uint	value;
		uint	color;
	};
	AppendStructuredBuffer< DebugInt >	u_debugText : register(u2);
#endif




#define DEPTH_REDUCTION_TG_SIZE	(16)




/*
===============================================================================

	ATMOSPHERE / LIGHT SCATTERING

===============================================================================
*/


struct AtmosphereParameters
{
	/// radius of the planet in meters (excluding the atmosphere) default = 6371e3
	float	inner_radius;

	/// radius of the atmosphere in meters, default = 6471e3
	float	outer_radius;


	/// intensity of the sun, default = 22
	/// affects the brightness of the atmosphere
	float	sun_intensity;

	/// how much extra the atmosphere blocks light
	float	density_multiplier;


	//=== [16]

	/// Rayleigh scattering coefficient at sea level ("Beta R") in m^-1, default = (3.8e-6f, 13.5e-6f, 33.1e-6f)//( 5.5e-6, 13.0e-6, 22.4e-6 )
	/// affects the color of the sky
	/// the amount Rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
	float3	beta_Rayleigh;

	/// Rayleigh scale height (atmosphere thickness if density were uniform) in meters, default = 7994
	/// How high do you have to go before there is no Rayleigh scattering?
	float	height_Rayleigh;

	//=== [16]

	/// Mie scattering coefficient at sea level ("Beta M") in m^-1, default = 21e-6
	/// affects the color of the blob around the sun
	/// the amount mie scattering scatters colors
	float3	beta_Mie;

	/// Mie scale height (atmosphere thickness if density were uniform) in meters, default = 1.2e3
	/// How high do you have to go before there is no Mie scattering?
	float	height_Mie;


	// Ozone layer:

//#define HEIGHT_ABSORPTION 30e3 /* at what height the absorption is at it's maximum */
//#define ABSORPTION_FALLOFF 3e3 /* how much the absorption decreases the further away it gets from the maximum height */


	//=== [16]

	/// Mie preferred scattering direction (aka "mean cosine"), default = 0.758
	/// The Mie phase function includes a term g (the Rayleigh phase function doesn't)
	/// which controls the anisotropy of the medium. Aerosols exhibit a strong forward directivity.
	/// the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
	float	g;

	/// the amount of scattering that always occurs, can help make the back side of the atmosphere a bit brighter
	float3	beta_ambient;



#ifndef __cplusplus
	
	//void setDefaults()

	void preset_Earth()
	{
		sun_intensity = 22;

		inner_radius = 6371e3;
		outer_radius = 6471e3;

		//
		beta_Rayleigh = float3( 3.8e-6f, 13.5e-6f, 33.1e-6f );
		beta_Mie = (float3) ( 21e-6 );

		height_Rayleigh = 7994;
		height_Mie = 1.2e3;

		g = 0.758;

		//max_scene_depth = 1e12;

		density_multiplier = 1;

		beta_ambient = float3(0,0,0);
	}

	//void preset_Mars()

#endif // #ifndef __cplusplus

};

/// all is relative to the planet center (at (0,0,0))
struct NwrAtmosphereViewerParameters
{
	float3	relative_eye_position;
	float	pad16;
};

/*
===============================================================================

	POST PROCESSING

===============================================================================
*/

struct UeberPostProcessingShaderData
{
	NwrAtmosphereViewerParameters	viewer;

	// atmosphere is rendered as a full-screen effect only for the closest planet
	AtmosphereParameters			atmosphere;
};

/*
===============================================================================

	VOXEL TERRAIN

===============================================================================
*/

/// Voxel terrain.
struct VoxelMaterial
{
	uint4	textureIDs;	// xyz: indices into the texture array, for each axis; w - unused
	float4	pbr_params;	// x - metalness, y - roughness; zw - unused

	float metalness() { return pbr_params.x; }
	float roughness() { return pbr_params.y; }
};


#endif // NW_SHADER_INTEROP_SHARED_DEFINITIONS_H
