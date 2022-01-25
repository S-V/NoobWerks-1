// shared code for rendering voxel terrain with geomorphing/CLOD

#include <Shared/nw_shared_globals.h>
#include "_VS.h"

#ifndef ENABLE_GEOMORPHING
#define ENABLE_GEOMORPHING	(1)
#endif

/// clamp boundary vertices to prevent seams - not really needed when using 'magic' LoD blending function
#ifndef ENFORCE_BOUNDARY_CONSTRAINTS
#define ENFORCE_BOUNDARY_CONSTRAINTS	(0)
#endif

/// Calculates the LoD blend factor to smooth LoD transitions and to enforce boundary constraints.
float Get_CLOD_Blend( in Vertex_CLOD _inputs )
{
#if ENFORCE_BOUNDARY_CONSTRAINTS
	const int boundaryCellType = _inputs.miscFlags;	// ECellType (internal, edge, face vertex?)
	const bool vertex_is_on_face = ( boundaryCellType >= 1 && boundaryCellType <= 6 );
	const bool vertex_is_on_edge = ( boundaryCellType >= 7 && boundaryCellType <= 18  );
	const bool vertex_is_on_corner = ( boundaryCellType > 18  );
	// LOD stitching
	if( boundaryCellType )	// if the vertex is the chunk's boundary...
	{
		const bool vertex_is_shared_by_a_chunk_of_coarser_lod = ( g_adjacency.x & (1u << boundaryCellType) );
		if( vertex_is_shared_by_a_chunk_of_coarser_lod ) {
			return 1;
		} else {
			return 0;
		}
	}
#endif

#if ENABLE_GEOMORPHING

	// See Proland documentation, 'Core module: 3.2.2. Continuous level of details': http://proland.inrialpes.fr
	const float3 localCameraPosition = g_morphInfo.xyz;	// the camera position relative to the chunk's min corner and divided by the chunk's size
	const float k_plus_1 = g_distanceFactors.y;	// = (k+1)
	const float k_minus_1_inv = g_distanceFactors.z;	// = 1 / (k-1)

	const float3 localPosition0 = CLOD_UnpackPosition0( _inputs.xyz0, g_objectScale.xyz );	// in [0..1]
	const float dist = max3( abs( localCameraPosition - localPosition0 ) );

	// blend = clamp( (d/l-(k+1))/(k-1), 0, 1 ), where d = max(|x-cx|, |y-cy|)
	const float blend = saturate( ( dist - k_plus_1 ) * k_minus_1_inv );

	return blend;

#else

	//return 0;
	return g_distanceFactors.w;

#endif

}

/// Blends vertex positions and normals.
void CLOD_Blend(
	in Vertex_CLOD _inputs,
	in float blend,
	out float3 interpolatedLocalPosition,
	out float3 interpolatedLocalNormal
	)
{
	const float3 localPosition0 = CLOD_UnpackPosition0( _inputs.xyz0, g_objectScale.xyz );	// [0..1]
	const float3 localPosition1 = CLOD_UnpackPosition1( _inputs.xyz1, g_objectScale.xyz );	// [0..1]
	interpolatedLocalPosition = lerp( localPosition0, localPosition1, blend );

	const float3 localNormal0 = UnpackVertexNormal( _inputs.N0 );
	const float3 localNormal1 = UnpackVertexNormal( _inputs.N1 );
	// normalize() is needed in the pixel shader
	//interpolatedLocalNormal = normalize( lerp( localNormal0, localNormal1, blend ) );
	interpolatedLocalNormal = lerp( localNormal0, localNormal1, blend );
}
