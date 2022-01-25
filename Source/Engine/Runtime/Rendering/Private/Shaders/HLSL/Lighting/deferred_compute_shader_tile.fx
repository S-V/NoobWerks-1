// Implementation of tiled deferred shading.
// Input: G-Buffers, depth buffer and list of lights.
// Output: fully composited and lit non-MSAA HDR texture.

/*
%%
pass TiledDeferred_CS
{
	compute = ComputeShaderTileCS
}
%%

entry = {
	file = 'HLSL/deferred_compute_shader_tile.hlsl'
	compute = 'ComputeShaderTileCS'
}
scene_passes = [
	{
		name = 'TiledDeferred_CS'
	}
]

not_reflected_uniform_buffers = [
	'TiledShadingConstants'
]

defines = [
	; valid values: 1, 2, 4, 8
	{
		name = 'MSAA_SAMPLES'
		defaultValue = '1'
	}
	{
		name = 'USE_DEPTH_BOUNDS_TESTING'
		defaultValue = '1'
	}
]
*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include <GBuffer/nw_gbuffer_read.h>
#include "_lighting.h"
#include "_screen_shader.h"
#include "_PS.h"
#include "PBR/PBR_Lighting.h"
#include <Common/Colors.hlsl>


#define	USE_PBS	(0)

/// these values are constant wrt each light
struct PBS_PrecomputedSurfaceData
{
	float3 N;
	float3 V;
	float NdotV;	// surface-to-eye direction, aka 'View Vector', 'V' = normalize( eye_pos - pixel_pos )

	// these values are constant wrt each light
	float3 base_color;

	float roughness;
	float metallic;

	/// light accessibility, obscurance, or 'openness' to ambient light
	float	accessibility;	// = ( 1 - ambient_occlusion ), 1 == unoccluded (default=1) 

	// Burley roughness bias
	float	alpha;

	// Blend base colors
	float3	c_diff;
	float3	c_spec;

	///
	void precomputeValuesUsingGBufferSurface(
		in GSurfaceParameters surface,
		in float3 surface_position	// in view space!
		)
	{
		N = surface.normal;
		V = -normalize(surface_position);
		NdotV = saturate(dot(N, V));

		// these values are constant wrt each light
		base_color = surface.albedo;

		roughness = surface.roughness;
		metallic = surface.metalness;

		accessibility = surface.accessibility;

		// Burley roughness bias
		alpha = roughness * roughness;

		// Specular coefficient - fixed reflectance value for non-metals
		static const float kSpecularCoefficient = 0.04;

		// Blend base colors
		c_diff = lerp( base_color,           BLACK.rgb,  metallic ) * accessibility;
		c_spec = lerp( kSpecularCoefficient, base_color, metallic ) * accessibility;
	}
};

//
float3 addDeferredPointLight_PBS(
	in float3 surface_position,
	in PBS_PrecomputedSurfaceData surface,
	in DeferredLight light
	)
{
	const float3 light_position		= light.position_and_radius.xyz;
	const float  light_radius		= light.position_and_radius.w;

	const float3 light_color		= light.color_and_inv_radius.xyz;
	const float  light_inv_radius	= light.color_and_inv_radius.w;

	//

	const float3 N = surface.N;
	const float3 V = surface.V;
	const float  NdotV = surface.NdotV;

	// light vector (from surface towards the light)
	float3 L = light_position - surface_position;
	const float distance = length(L);
	L /= distance;

	//

	// Half vector
	const float3 H = normalize(L + V);

	// products
	const float NdotL = saturate(dot(N, L));
	const float LdotH = saturate(dot(L, H));
	const float NdotH = saturate(dot(N, H));

	//

	// linear falloff instead of quadratic gives a softer look
	const float light_falloff = saturate( 1 - distance * light_inv_radius ) * NdotL;

	float diffuse_factor = Diffuse_Burley( NdotL, NdotV, LdotH, surface.roughness );
	float3 specular      = Specular_BRDF( surface.alpha, surface.c_spec, NdotV, NdotL, LdotH, NdotH );

	return NdotL * light_color * (((surface.c_diff * diffuse_factor) + specular)) * light_falloff;
}

//
float3 AddDeferredPointLight(
	in float3 surface_position,
	in GSurfaceParameters surface,
	in DeferredLight light
	)
{
	const float3 light_position		= light.position_and_radius.xyz;
	const float  light_radius		= light.position_and_radius.w;

	const float3 light_color		= light.color_and_inv_radius.xyz;
	const float  light_inv_radius	= light.color_and_inv_radius.w;

	// light vector (from surface towards the light)
	float3 L = light_position - surface_position;
	const float distance = length(L);
	L /= distance;

	//
	const float NdotL = saturate(dot(surface.normal, L));

	// linear falloff instead of quadratic gives a softer look
	const float light_falloff = saturate( 1 - distance * light_inv_radius ) * NdotL;


	// Diffuse

	//
	const float3 diffuse_color = light_color * surface.albedo;


	// Specular

	// view vector (from surface towards the eye) in view space
	const float3 V = -normalize( surface_position );

	// compute half-vector
	const float3 H = normalize( V + L );

	const float NdotH = saturate(dot(surface.normal, H));

	// ? max(SpecularPower,0.0001)
	const float specular_contribution = pow( NdotH, surface.specular_power );

	const float3 specular_color = surface.specular_color * specular_contribution;

	//
//	return ( diffuse_color + specular_color ) * light_falloff;
	return ( diffuse_color ) * light_falloff;
}

//-----------------------------------------------------------------------------------------
// Parameters for the light culling shader
//-----------------------------------------------------------------------------------------
// size of a thread group in threads
#define NUM_THREADS_X TILE_SIZE_X
#define NUM_THREADS_Y TILE_SIZE_Y

//-----------------------------------------------------------------------------------------
// Textures and Buffers
//-----------------------------------------------------------------------------------------

// all potentially visible point lights in the scene
StructuredBuffer< DeferredLight >	b_pointLights;

// output HDR target
RWTexture2D< float4 >	t_litTarget;

//-----------------------------------------------------------------------------------------
//	Group Shared Memory (aka local data share, or LDS)
//-----------------------------------------------------------------------------------------

// Min and Max depth for this tile for depth bounds testing:
groupshared uint gs_TileMinDepthInt;
groupshared uint gs_TileMaxDepthInt;

// Light list for the tile:
groupshared uint gs_TileLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint gs_TileNumLights;	// number of lights in this tile

//-----------------------------------------------------------------------------------------
//	Helper Functions
//-----------------------------------------------------------------------------------------

// this creates the standard Hessian-normal-form plane equation from three points, 
// except it is simplified for the case where the first point is the origin
inline float4 CreatePlaneEquation( in float4 b, in float4 c )
{
	float4 n;

	// normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
	//NOTE: my view space is RIGHT handed, hence the change in multiplication order
	n.xyz = normalize(cross( c.xyz, b.xyz ));

	// -(n dot a), except we know "a" is the origin
	n.w = 0;

	return n;
}

// point-plane distance, simplified for the case where 
// the plane passes through the origin (frustum side plane in view space)
inline float GetSignedDistanceFromPlane( float3 p, float4 eqn )
{
	// dot( eqn.xyz, p.xyz ) + eqn.w, except we know eqn.w is zero (see CreatePlaneEquation above).
	return dot( eqn.xyz, p.xyz );
}

void BuildFrustumPlanes(
	in uint2 groupId,	// 'Coordinates' of the thread group (x,y in range [0..TilesX-1, 0..TilesY-1]).
	in float nearClip, in float farClip,
	out float4 frustumPlanes[6]	// plane equations for the four sides of the frustum, with the positive half-space outside the frustum
)
{
	const uint numHorizontalTiles = g_tileCount.x;
	const uint numVerticalTiles = g_tileCount.y;

	// window width evenly divisible by tile width
	const float WW = (float) (TILE_SIZE_X * numHorizontalTiles);//<= NOTE: cast is important!
	// window height evenly divisible by tile height
	const float HH = (float) (TILE_SIZE_Y * numVerticalTiles);	//<= NOTE: must be float!

	// tile rectangle in viewport space
	const uint px_min = TILE_SIZE_X * groupId.x;	// [0 .. ScreenWidth - TILE_SIZE_X]
	const uint py_min = TILE_SIZE_Y * groupId.y;	// [0 .. ScreenHeight - TILE_SIZE_Y]
	const uint px_max = TILE_SIZE_X * (groupId.x + 1);
	const uint py_max = TILE_SIZE_Y * (groupId.y + 1);

	// scale to clip space
	const float px_left		= px_min / WW;
	const float px_right	= px_max / WW;
	const float py_top		= (HH - py_min) / HH;
	const float py_bottom	= (HH - py_max) / HH;

	// four corners of the tile in clip space, clockwise from top-left
	float4 frustumCornersCS[4];
	frustumCornersCS[0] = float4( px_left*2.f-1.f,	py_top*2.f-1.f,		1.f,	1.f );
	frustumCornersCS[1] = float4( px_right*2.f-1.f,	py_top*2.f-1.f,		1.f,	1.f );
	frustumCornersCS[2] = float4( px_right*2.f-1.f,	py_bottom*2.f-1.f,	1.f,	1.f );
	frustumCornersCS[3] = float4( px_left*2.f-1.f,	py_bottom*2.f-1.f,	1.f,	1.f );

	// four corners of the tile in view space
	float4 frustumCornersVS[4];
	frustumCornersVS[0] = ProjectPoint( g_inverse_projection_matrix, frustumCornersCS[0] );
	frustumCornersVS[1] = ProjectPoint( g_inverse_projection_matrix, frustumCornersCS[1] );
	frustumCornersVS[2] = ProjectPoint( g_inverse_projection_matrix, frustumCornersCS[2] );
	frustumCornersVS[3] = ProjectPoint( g_inverse_projection_matrix, frustumCornersCS[3] );

	// create plane equations for the four sides of the frustum,
	// with the positive half-space outside the frustum (and remember,
	// view space is left handed, so use the left-hand rule to determine
	// cross product direction)
	[unroll]
	for( uint i = 0; i < 4; i++ ) {
		frustumPlanes[i] = CreatePlaneEquation( frustumCornersVS[i], frustumCornersVS[(i+1)&3] );
	}

	// Near/far clipping planes in view space
	frustumPlanes[4] = float4( 0.0f, -1.0f, 0.0f, nearClip );
	frustumPlanes[5] = float4( 0.0f, +1.0f, 0.0f, -farClip );
}

// NOTE: expects plane equations for the sides of the frustum, with the positive half-space outside the frustum
inline bool FrustumIntersectsSphere( in float4 frustumPlanes[6], in float3 sphereCenter, in float sphereRadius )
{
	bool intersectsFrustum = true;
	[unroll]// commented out early exit
	for( uint i = 0; i < 6 /*&& intersectsFrustum*/; ++i )
	{
		// plane normals point outside the frustum
		float dist = dot( frustumPlanes[i], float4( sphereCenter, 1.0f ) );
		// If it is on totally the positive half space of one plane we can reject it.
		intersectsFrustum = intersectsFrustum && ( dist <= sphereRadius );
	}
	return intersectsFrustum;
}

//-----------------------------------------------------------------------------------------
// Light culling shader
//-----------------------------------------------------------------------------------------

// called with Dispatch( groupsX, groupsY, 1 ) where groupsX = ceil( screen_width/TILE_SIZE_X ), etc.

[numthreads( NUM_THREADS_X, NUM_THREADS_Y, 1 )]
void ComputeShaderTileCS(
	uint3 groupId			: SV_GroupID,		// 'Coordinates' of the thread group.
	uint groupIndex			: SV_GroupIndex,	// The "flattened" index of the compute shader thread within a thread group.
	uint3 groupThreadId		: SV_GroupThreadID,	// 'Relative coordinates' of the thread inside a thread group.
	uint3 dispatchThreadId	: SV_DispatchThreadID	// 'Absolute coordinates' of the thread as derived from the Dispatch() call.
	)
{
	// NOTE: This is currently necessary rather than just using SV_GroupIndex to work around a compiler bug on Fermi.

	// flattened index of this thread within thread group
	const uint localThreadIndex =  groupThreadId.y * TILE_SIZE_X + groupThreadId.x;

	// texel coordinates of this pixel in the [0, textureWidth - 1] x [0, textureHeight - 1] range
	const uint2 globalCoords = dispatchThreadId.xy;

	// Compute screen/clip-space position.
	float2 screenSize;
	DepthTexture.GetDimensions( screenSize.x, screenSize.y );

	// NOTE: Mind Direct3D 11 viewport transform and pixel center!
    // NOTE: This offset can be precomputed on the CPU, but it's actually slower
	// to read it from a constant buffer than to just recompute it.
	const float2 texCoords = (float2(globalCoords) + 0.5f) / screenSize;
	// here goes ya clip-space position:
	const float2 screen_space_position = TexCoordsToClipPos( texCoords );

	const float hardware_z_depth = DepthTexture.Load( int3( globalCoords.x, globalCoords.y, 0 ) );
	const float3 view_space_position = restoreViewSpacePosition( screen_space_position, hardware_z_depth );
	const float view_space_depth = view_space_position.y;

//float3 worldSpacePosition = mul( g_inverse_view_matrix, float4(view_space_position,1) ).xyz;

	// Work out depth bounds for our samples
	float minZSample = g_DepthClipPlanes.y;	// far plane initially
	float maxZSample = g_DepthClipPlanes.x;	// near plane initially

	// Avoid shading skybox/background or otherwise invalid pixels
	bool validPixel = view_space_depth >= g_DepthClipPlanes.x && view_space_depth <  g_DepthClipPlanes.y;
	//[flatten] if (validPixel)
	{
		minZSample = min( minZSample, view_space_depth );
		maxZSample = max( maxZSample, view_space_depth );
	}

	// Initialize per-tile variables - shared memory light list and depth bounds.
	if( localThreadIndex == 0 )
	{
		gs_TileNumLights = 0;
		gs_TileMinDepthInt = 0x7F7FFFFF;	// largest normal single precision float (3.4028235e+38)
		gs_TileMaxDepthInt = 0;
	}

	// Block execution of all threads in a group
	// until all group shared accesses have been completed
	// and all threads in the group have reached this call.
	GroupMemoryBarrierWithGroupSync();
	{
		// Calculate min and max depth in the threadgroup/tile.
		// Atomics still aren't that great though, since they effectively serialize the memory access among all threads.
		// To avoid using them, you can use a parallel reduction instead. However a parallel reduction requires more shared memory, so it may not always be a win.

		// Only scatter pixels with actual valid samples in them
		//if (maxZSample >= minZSample)
		{
			// atomics only work on integers, but we can always cast float to int (depth is always positive)
			InterlockedMin( gs_TileMinDepthInt, asuint(minZSample) );
			InterlockedMax( gs_TileMaxDepthInt, asuint(maxZSample) );
		}
	}
	GroupMemoryBarrierWithGroupSync();


    // Calculate the min and max depth for this tile, 
    // to form the front and back of the frustum.

	float minTileZ = asfloat( gs_TileMinDepthInt );
	float maxTileZ = asfloat( gs_TileMaxDepthInt );



	// Construct frustum for this tile.

	// NOTE: This is all uniform per-tile (i.e. no need to do it per-thread) but fairly inexpensive
    // We could just precompute the frusta planes for each tile and dump them into a constant buffer...
    // They don't change unless the projection matrix changes since we're doing it in view space.
    // Then we only need to compute the near/far ones here tightened to our actual geometry.
    // The overhead of group synchronization/LDS or global memory lookup is probably as much as this
    // little bit of math anyways, but worth testing.

	// plane equations for the four sides of the frustum, with the positive half-space outside the frustum
    float4 frustumPlanes[6];
	BuildFrustumPlanes( groupId.xy, minTileZ, maxTileZ, frustumPlanes );


	// How many total lights?
	const uint numActiveLights = g_deferredLightCount.x;


	// Cull lights for this tile.

	// Loop over the lights and do a sphere vs. frustum intersection test.
	// Each thread now operates on a sample instead of a pixel.
	// (Detailed explanation here: http://www.gamedev.net/topic/625569-light-culling-in-a-compute-shader/?view=findpost&p=4944777)

	// process THREADS_PER_TILE lights in parallel
#if 1
	for( uint lightIndex = localThreadIndex; lightIndex < numActiveLights; lightIndex += THREADS_PER_TILE )
	{
		float3	lightCenter = b_pointLights[ lightIndex ].position_and_radius.xyz;
		float	lightRadius = b_pointLights[ lightIndex ].position_and_radius.w;

		// test if sphere is intersecting or inside frustum

		if( FrustumIntersectsSphere( frustumPlanes, lightCenter, lightRadius ) )
		{
			//if( lightCenter.z - lightRadius > minTileZ && lightCenter.z + lightRadius < maxTileZ )
			//if( lightCenter.z > minTileZ && lightCenter.z < maxTileZ )
			{
				// Append light to list:
				// do a thread-safe increment of the list counter 
				// and put the index of this light into the list.
				// Compaction might be better if we expect a lot of lights.
				uint writeOffset;
				InterlockedAdd( gs_TileNumLights, 1, writeOffset );
				gs_TileLightIndices[ writeOffset ] = lightIndex;
			}
		}
	}
#else
	const uint passCount = (numActiveLights + THREADS_PER_TILE - 1) / THREADS_PER_TILE;

	for( uint passIndex = 0; passIndex < passCount; passIndex++ )
	{
		uint lightIndex = passIndex * THREADS_PER_TILE + localThreadIndex;

		// prevent overrun by clamping to a last ”null” light
		lightIndex = min( lightIndex, numActiveLights );

		float3	lightCenter = b_pointLights[ lightIndex ].position_and_radius.xyz;
		float	lightRadius = b_pointLights[ lightIndex ].position_and_radius.w;

		// test if sphere is intersecting or inside frustum

		if( FrustumIntersectsSphere( frustumPlanes, lightCenter, lightRadius ) )
		{
			//if( lightCenter.z - lightRadius > minTileZ && lightCenter.z + lightRadius < maxTileZ )
			//if( lightCenter.z > minTileZ && lightCenter.z < maxTileZ )
			{
				// Append light to list:
				// do a thread-safe increment of the list counter 
				// and put the index of this light into the list.
				// Compaction might be better if we expect a lot of lights.
				uint writeOffset;
				InterlockedAdd( gs_TileNumLights, 1, writeOffset );
				gs_TileLightIndices[ writeOffset ] = lightIndex;
			}
		}
	}
#endif

	// Sync threads
	GroupMemoryBarrierWithGroupSync();


	const uint numLightsAffectingTile = gs_TileNumLights;


#if DEBUG_EMIT_POINTS
	if( localThreadIndex == 0 )
	{
		DebugPoint	debugPoint;

		debugPoint.groupId.x = groupId.x;
		debugPoint.groupId.y = groupId.y;
		debugPoint.groupId.z = numLightsAffectingTile;
		debugPoint.groupId.w = 0;

		u_debugPoints.Append( debugPoint );
	}
#endif


	// Only process onscreen pixels (tiles can span screen edges)
    //if( all( globalCoords.xy < g_framebuffer_dimensions.xy ) )
	{
		const GSurfaceParameters gbuffer_surface = Load_Surface_Data_From_GBuffer( globalCoords );

		//
		PBS_PrecomputedSurfaceData	pbr_surface;
		pbr_surface.precomputeValuesUsingGBufferSurface( gbuffer_surface, view_space_position );


		// RGB accumulated RGB HDR color, A: luminance for screenspace subsurface scattering
		float4 compositedLighting = 0;


		//t_litTarget[globalCoords.xy] = float4(float(numLightsAffectingTile).xxx/5.0f, salt.x);

		for( uint tileLightIndex = 0; tileLightIndex < numLightsAffectingTile; tileLightIndex++ )
		{
			const uint lightIndex = gs_TileLightIndices[ tileLightIndex ];
			const DeferredLight light = b_pointLights[ lightIndex ];

#if USE_PBS

			compositedLighting.rgb += addDeferredPointLight_PBS(
				view_space_position, pbr_surface, light
			);

#else
			compositedLighting.rgb += AddDeferredPointLight(
				view_space_position, gbuffer_surface, light
			);

#endif

		}//for each light in this tile

		t_litTarget[globalCoords.xy] = float4( compositedLighting.rgb, 1 );

		// debugging
		//t_litTarget[globalCoords.xy] = float4(float(numLightsAffectingTile).xxx/5.0f, salt.x);
	}
}

/* References:
https://github.com/vonture/DeferredRenderer/blob/master/doc/GDC11_DX11inBF3_Public.pdf
*/
