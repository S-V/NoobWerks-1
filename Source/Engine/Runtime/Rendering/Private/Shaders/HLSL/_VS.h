// Vertex shader utilities.
#ifndef __VERTEX_SHADERS_COMMON_H__
#define __VERTEX_SHADERS_COMMON_H__

#include <base.hlsli>	// UnpackVertexNormal()

/// Compress vertex positions for CLOD/geomorphing?
#define CLOD_USE_PACKED_VERTICES	(0)

/*
// generic vertex type, can be used for rendering both static and skinned meshes
struct DrawVertex
{
	V3f		xyz;	//12 POSITION
	Half2	st;		//4  TEXCOORD		DXGI_FORMAT_R16G16_FLOAT

	UByte4	N;		//4  NORMAL			DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	T;		//4  TANGENT		DXGI_FORMAT_R8G8B8A8_UINT

	UByte4	indices;//4  BLENDINDICES	DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	weights;//4  BLENDWEIGHT	DXGI_FORMAT_R8G8B8A8_UNORM
	//32 bytes
};
*/
struct DrawVertex
{
	float3 position : Position;
	float2 texCoord : TexCoord;
	uint4 normal : Normal;
	uint4 tangent : Tangent;	// .w stores the handedness of the tangent space
	uint4 indices : BoneIndices;
	float4 weights : BoneWeights;
};

/// for auxiliary/debug rendering
struct AuxVertex
{
	float3 position : Position;
	float2 texCoord : TexCoord0;
	float2 texCoord2 : TexCoord1;	// for lightmap
	float4 color : Color;	//autonormalized into [0..1] range
	uint4 normal : Normal;
	uint4 tangent : Tangent;
};


// Compute the triangle face normal from 3 points
float3 faceNormal( in float3 posA, in float3 posB, in float3 posC )
{
    return normalize( cross(normalize(posB - posA), normalize(posC - posA)) );
}

float2 unpackTexCoordsFromHalf2( in float2 packedUVs )
{
	return float2( f16tof32(packedUVs.x), f16tof32(packedUVs.y) );
}


/// 32 bytes
struct Vertex_Static
{
	float3 position : Position;		// 3 * float
	float2 tex_coord : TexCoord;	// 2 * unorm short
	uint4 normal : Normal;			// 4 * unorm byte
	uint4 tangent : Tangent;		// 4 * unorm byte
	float4 color : Color0;			// 4 * unorm byte

	// higher-precision normal for planet UV calculation
	float4 normal_for_UV_calc : Normal1;		// R10G10B10A2_UNORM unorm
};

//=====================================================================
//   VOXEL TERRAIN
//=====================================================================

/// Vertex for rendering voxel terrains with Discrete Level-Of-Detail ("stitching" and alpha blending).
struct Vertex_DLOD
{
	uint	xy        : Position0;	//!< (16;16) - quantized X and Y of normalized position, relative to the chunk's minimal corner
	uint	z_and_mat : Position1;	//!< lower 16 bits - Z pos, remaining bits - material IDs
	uint4	N         : Normal0;	//!< XYZ - object-space normal, W - unused
	float4	color     : Color0;		//!< RGB - vertex color, A - ambient occlusion
};
/*
struct Vertex_CLOD
{
	U32		xyz0;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
	U32		xyz1;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
	UByte4	N0;		//4 XYZ - fine normal, W - unused
	UByte4	N1;		//4 XYZ - coarse normal, W - unused

	// RGB - vertex color, A - ambient occlusion
	U32		color0;	//4
	U32		color1;	//4

	// material IDs
	UByte4	materialIDs;
	UByte4	pad1;
	//32 bytes
};
*/

#if CLOD_USE_PACKED_VERTICES
	/// Vertex for rendering voxel terrains with Continuous Level-Of-Detail (with geomorphing).
	struct Vertex_CLOD
	{
		uint	xyz0 : Position0;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		uint	xyz1 : Position1;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		uint4	N0 : Normal0;		//4 XYZ - fine normal, W - unused
		uint4	N1 : Normal1;		//4 XYZ - coarse normal, W - unused

		// RGB - vertex color, A - ambient occlusion
		float4	color0 : Color0;	//4
		float4	color1 : Color1;	//4

		uint4	materials : TexCoord0;	//!< material IDs
		uint	miscFlags : TexCoord1;	//!< vertex type (internal or boundary)
	};//32 bytes

	#if 1
		/// De-quantizes normalized coords from 32-bit integer (11,11,10).
		/// '_scale' contains {x,y = (1.0f/2047.0f), z=(1.0f/1023.0f)} multiplied by CHUNK_SIZE.
		float3 CLOD_UnpackPosition0( in uint u, in float3 _scale ) {
			return float3(
				float( (u       ) & 2047u ) * _scale.x,	// x
				float( (u >> 11u) & 2047u ) * _scale.y,	// y
				float( (u >> 22u) & 1023u ) * _scale.z	// z
			);
		}
	#else
		/// De-quantizes normalized coords from 32-bit integer (10,10,10).
		/// '_scale' contains {x,y,z=(1.0f/1023.0f)} multiplied by CHUNK_SIZE.
		float3 CLOD_UnpackPosition0( in uint u, in float3 _scale ) {
			return float3(
				float( (u       ) & 1023u ) * _scale.x,	// x
				float( (u >> 10u) & 1023u ) * _scale.y,	// y
				float( (u >> 20u) & 1023u ) * _scale.z	// z
			);
		}
	#endif

	#define CLOD_UnpackPosition1( u, scale )	CLOD_UnpackPosition0( u, scale )

	//float3 CLOD_UnpackPosition1( in uint u, in float3 _scale ) {
	//	// mul by 2.0f to compensate for 1.0/1023.0f built into _scale
	//	return float3(
	//		float( (u       ) & 511u ) * 2.0f * _scale.x,	// x
	//		float( (u >> 9u ) & 511u ) * 2.0f * _scale.y,	// y
	//		float( (u >> 18u) & 511u ) * 2.0f * _scale.z	// z
	//	);
	//}

#else // if !CLOD_USE_PACKED_VERTICES

	struct Vertex_CLOD
	{
		float3	xyz0 : Position0;	//12 fine position
		uint4	N0 : Normal0;		//4 XYZ - fine normal, W - unused
		float3	xyz1 : Position1;	//12 coarse position
		uint4	N1 : Normal1;		//4 XYZ - coarse normal, W - unused

		// RGB - vertex color, A - ambient occlusion
		float4	color0 : Color0;	//4
		float4	color1 : Color1;	//4

		uint4	materials : TexCoord0;	//!< material IDs
		uint	miscFlags : TexCoord1;	//!< vertex type (internal or boundary)
	};//48 bytes

	#define CLOD_UnpackPosition0( _pos, scale )	(_pos)
	#define CLOD_UnpackPosition1( _pos, scale )	(_pos)

#endif // !CLOD_USE_PACKED_VERTICES


// DLOD vertex positions are quantized in the same way as in CLOD
float3 UnpackPosition_DLOD( in uint u, in float3 _scale )
{
	return float3(
		float( (u       ) & 2047u ) * _scale.x,	// x
		float( (u >> 11u) & 2047u ) * _scale.y,	// y
		float( (u >> 22u) & 1023u ) * _scale.z	// z
	);
}

// Decodes quantized (11,11,10) vertex positions and returns normalized [0..1] floats.
float3 unpackNormalizedPosition_DLoD( in uint u )
{
	return float3(
		float( (u       ) & 2047u ) * (1.0f / 2047.0f),	// x
		float( (u >> 11u) & 2047u ) * (1.0f / 2047.0f),	// y
		float( (u >> 22u) & 1023u ) * (1.0f / 1023.0f)	// z
	);
}

float3 unpack_NormalizedPosition_and_MaterialIDs(
	in Vertex_DLOD vertex
	, out uint material_id_
	)
{
	float3	normalized_position;
	
	normalized_position.x = float( (vertex.xy)        & 65535 ) * (1.0f / 65535.0f);
	normalized_position.y = float( (vertex.xy >> 16)  & 65535 ) * (1.0f / 65535.0f);
	normalized_position.z = float( (vertex.z_and_mat) & 65535 ) * (1.0f / 65535.0f);

	//
	material_id_ = (vertex.z_and_mat >> 16)  & 255;
	
	return normalized_position;
}

/*
// generic vertex type, can be used for rendering both static and skinned meshes (based on idTech4's idDrawVert)
struct DrawVertex
{
	V3f	xyz;	//12 POSITION
	Half2	st;		//4  TEXCOORD		DXGI_FORMAT_R16G16_FLOAT

	UByte4	N;		//4  NORMAL			DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	T;		//4  TANGENT		DXGI_FORMAT_R8G8B8A8_UINT

	UByte4	indices;//4  BLENDINDICES	DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	weights;//4  BLENDWEIGHT	DXGI_FORMAT_R8G8B8A8_UNORM
	//32 bytes
};
*/
struct SkinnedVertex
{
	float3 position : Position;
	float2 texCoord : TexCoord;
	uint4 normal : Normal;
	uint4 tangent : Tangent;	// .w stores the handedness of the tangent space
	uint4 indices : BoneIndices;
	float4 weights : BoneWeights;
};

#define NUM_BONE_WEIGHTS	(4)


#ifdef __SHADER_COMMON_H__

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

#endif // #ifndef __SHADER_COMMON_H__

float3 transformByBoneMatrix( in float4 v, in row_major float4x4 bone_matrix )
{
	return (float3)mul( bone_matrix, v );
}


#ifdef __SHADER_COMMON_H__

	// transforms position and tangent by weighted sum of skinning matrices
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

#endif // #ifndef __SHADER_COMMON_H__

#endif // __VERTEX_SHADERS_COMMON_H__
