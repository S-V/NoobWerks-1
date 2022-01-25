#ifndef NW_VERTEX_FORMATS_HLSLI
#define NW_VERTEX_FORMATS_HLSLI

#include <Shared/nw_shared_globals.h>
#include <base.hlsli>	// UnpackVertexNormal()
#include <Common/vertex_formats.hlsli>

/// computes weighted sum of skinning matrices
float4x4 calculateBoneMatrix( in SkinnedVertex v )
{
	row_major float4x4 bone_matrix = g_bone_matrices[v.indices[0]] * v.weights[0];

	[unroll]
	for( int i = 1; i < NUM_BONE_WEIGHTS; i++ )
	{
		bone_matrix += g_bone_matrices[v.indices[i]] * v.weights[i];
	}

	return bone_matrix;
}

///
float3 transformByBoneMatrix( in float4 v, in row_major float4x4 bone_matrix )
{
	return (float3)mul( bone_matrix, v );
}

/// transforms position and tangent by weighted sum of skinning matrices
void transform_Skinned_Vertex(
	in SkinnedVertex vertex_in
	, out float3 animated_local_position_
	, out float3 animated_local_normal_
	, out float3 animated_local_tangent_
	)
{
	// transform position by weighted sum of skinning matrices
	const row_major float4x4 bone_matrix = calculateBoneMatrix(
		vertex_in
		);

	// animated vertex position in object space:
	animated_local_position_ = transformByBoneMatrix(
		float4( vertex_in.position, 1 )
		, bone_matrix
		);

	//
	const float3 local_normal = UnpackVertexNormal( vertex_in.normal );
	const float3 local_tangent = -UnpackVertexNormal( vertex_in.tangent );

	animated_local_normal_	= transformByBoneMatrix( float4( local_normal, 0 ), bone_matrix );
	animated_local_tangent_	= transformByBoneMatrix( float4( local_tangent, 0 ), bone_matrix );
}

#endif // NW_VERTEX_FORMATS_HLSLI
