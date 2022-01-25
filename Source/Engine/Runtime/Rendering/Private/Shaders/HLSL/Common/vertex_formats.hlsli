#ifndef NW_SKINNING_HLSLI
#define NW_SKINNING_HLSLI

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

#endif // NW_SKINNING_HLSLI
