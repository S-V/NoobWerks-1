/*
Single-pass 'thin' (surface) voxelization using Geometry shader.

%%
pass VXGI_Voxelize
{
	vertex = main_VS_VoxelTerrain_DLoD
	geometry = main_GS
	pixel = main_PS_VoxelTerrain_DLoD

	state = voxelize
}

samplers
{
	s_pointSampler = PointSampler
	s_trilinear = TrilinearSampler
	// s_shadowMapSampler = ShadowMapSampler
	// s_shadowMapPCFBilinearSampler = ShadowMapSamplerPCF
	s_diffuseMap = DiffuseMapSampler
}
%%

*/

#include "base.hlsli"
#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "vxgi_interop.h"
#include "_lighting.h"	// selectTightestShadowCascade(), getCascadeShadowOcclusion()
#include "vxgi_interop.h"
#include "vxgi_common.hlsli"
#include "_VS.h"	// Vertex_DLOD
#include <VoxelGI/scene_shadows.hlsl>

cbuffer Uniforms
{
	row_major float4x4	u_local_to_world_space;
	
	// converts from normalized [0..1] local space
	// into NDC [-1..1] space within the voxel cascade volume
	//row_major float4x4	object_to_NDC_space;

	// converts from normalized [0..1] mesh-local space
	// into [0..1] space within the voxel cascade volume
	row_major float4x4	u_local_to_voxel_grid_space_01;
	
	uint4				u_cascade_index;
};

// // density (R8_UNORM) - updated on CPU, has the same dimension as voxel cascades
// Texture2D				t_voxelDensityGrid;

/// Output buffer (will be converted  to 3D texture in compute shader):
/// R8G8B8A8, where: rgb - radiance, a - density/opacity/occlusion
RWStructuredBuffer< uint >	rwsb_voxelGrid;

// for shadows
Texture3D< float >		t_sdf_atlas3d;
SamplerState			s_trilinear;

// // 
// Texture2D				t_shadowDepthMap;
// SamplerState			s_pointSampler;
// SamplerComparisonState	s_shadowMapSampler;
// SamplerComparisonState	s_shadowMapPCFBilinearSampler;

// Voxel Terrain Material Texture Arrays
Texture2DArray	t_diffuseMaps;
SamplerState	s_diffuseMap;

//-----------------------------------------------------------------------------

struct VSOutput
{
	float3 position_world : Position;	// 
	
	float3 position01_in_voxel_grid : Position1;	// 
	
	float3 normalWS : Normal0;	// world-space normal
	
	nointerpolation uint	materialId : Color0;	// voxel material id
};

VSOutput main_VS_VoxelTerrain_DLoD( in Vertex_DLOD vertexInput )
{
	// get the normalized vertex position in the chunk's seam space
	uint	materialId;
	const float3 normalizedPosition = unpack_NormalizedPosition_and_MaterialIDs( vertexInput, materialId );	// [0..1]

	VSOutput	vertexOutput;
	vertexOutput.position_world = mul( u_local_to_world_space, float4( normalizedPosition, 1.0f ) ).xyz;

	vertexOutput.position01_in_voxel_grid = mul( u_local_to_voxel_grid_space_01, float4( normalizedPosition, 1.0f ) ).xyz;
	
	// vertexOutput.positionNDC.xyz = (vertexOutput.position01_in_voxel_grid * 2 - 1);
	// vertexOutput.positionNDC.z = 1;
	// vertexOutput.positionNDC.w = 1;
	
	const float3 localNormal = UnpackVertexNormal( vertexInput.N );
	vertexOutput.normalWS = Dir_Local_To_World( localNormal );
	
	vertexOutput.materialId = materialId;

	return vertexOutput;
}

//-----------------------------------------------------------------------------

struct GSOutput
{
	float3					position_world	: Position;	// 
	float4					positionNDC		: SV_Position;	// 
	
	float3					normalWS		: Normal0;	// world-space normal
	nointerpolation uint	materialId		: Color0;	// voxel material id
//	float3 position01_in_voxel_grid : Position1;	// 
};

[maxvertexcount(3)]
void main_GS(
	triangle VSOutput input[3],
	inout TriangleStream< GSOutput > outputStream
)
{
	const GPU_VXGI_Cascade vxgi_cascade = g_vxgi.cascades[ u_cascade_index.x ];
	
	// Calculate the dominant direction of the surface normal.
	//
	// Face normal based on the triangle edges:
	// normalize(cross(input[1].p_world - input[0].p_world,
	//                 input[2].p_world - input[0].p_world))
	//
	// Normalization is not needed (i.e. uniform scaling):
	// cross(input[1].p_world - input[0].p_world,
	//       input[2].p_world - input[0].p_world))
	//
	// ~ abs(sum of shading normals)
	const float3 abs_n = abs(input[0].normalWS
						   + input[1].normalWS
						   + input[2].normalWS);
						   
	const int axis = maxIndex3( abs_n );

	
	// Project the triangle in the dominant direction for rasterization,
	// but not for lighting.

	GSOutput	outputs[3];

	[unroll]
	for( uint i = 0; i < 3; i++ )
	{
		GSOutput	output;

		output.position_world = input[i].position_world;
		
		const float3 position01_in_voxel_grid = vxgi_cascade.TransformFromWorldToUvwTextureSpace( input[i].position_world );
		output.positionNDC.xyz = position01_in_voxel_grid * 2.f - 1.f;

		[flatten]
		switch (axis) {
		case 0:
			output.positionNDC.xy = output.positionNDC.yz;
			break;
		case 1:
			output.positionNDC.xy = output.positionNDC.zx;
			break;
		default:
			break;
		}

		output.positionNDC.zw = 1;
		
		output.normalWS = input[i].normalWS;
		
		output.materialId = input[i].materialId;
		
		outputs[i] = output;
	}


	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(outputs[1].positionNDC.xy - outputs[0].positionNDC.xy);
	float2 side1N = normalize(outputs[2].positionNDC.xy - outputs[1].positionNDC.xy);
	float2 side2N = normalize(outputs[0].positionNDC.xy - outputs[2].positionNDC.xy);
	outputs[0].positionNDC.xy += normalize(side2N - side0N);
	outputs[1].positionNDC.xy += normalize(side0N - side1N);
	outputs[2].positionNDC.xy += normalize(side1N - side2N);


	[unroll]
	for( uint j = 0; j < 3; j++ )
	{
		outputStream.Append(outputs[j]);
	}
	
	outputStream.RestartStrip();
}

//-----------------------------------------------------------------------------

void main_PS_VoxelTerrain_DLoD( VSOutput pixelInput )
{
	const GPU_VXGI_Cascade vxgi_cascade = g_vxgi.cascades[ u_cascade_index.x ];

	const float3 posInVoxelGrid = vxgi_cascade.GetGridCoordsFromWorldPos( pixelInput.position_world );

	const int3 writecoord = (int3) trunc( posInVoxelGrid );

	
	
	

	


	// calculate the blend factor ('delta') for each direction
	const float blendSharpness = 0.5f;
	float3 blendWeights = saturate(abs(pixelInput.normalWS) - blendSharpness);
	// Normalize result by dividing by the sum of its components. 
	blendWeights /= dot(blendWeights, (float3)1.0f);	// force sum to 1.0

	static const float texture_scale = 0.04f;

	//
	const VoxelMaterial	voxelMaterial = g_voxel_materials[ pixelInput.materialId ];
	//const VoxelMaterial	voxelMaterial = g_voxel_materials[ 1 ];

	// the third component is an index into the texture array:
	const float3 texCoordA0 = float3( pixelInput.position_world.yz * texture_scale, voxelMaterial.textureIDs.x );
	const float3 texCoordA1 = float3( pixelInput.position_world.zx * texture_scale, voxelMaterial.textureIDs.y );
	const float3 texCoordA2 = float3( pixelInput.position_world.xy * texture_scale, voxelMaterial.textureIDs.z );

	// sample diffuse color
	const float3 sampledColorAX = t_diffuseMaps.Sample(s_diffuseMap, texCoordA0).rgb;
	const float3 sampledColorAY = t_diffuseMaps.Sample(s_diffuseMap, texCoordA1).rgb;
	const float3 sampledColorAZ = t_diffuseMaps.Sample(s_diffuseMap, texCoordA2).rgb;
	// blend the results of the 3 planar projections
	const float3 blendedColor =
		sampledColorAX * blendWeights.x +
		sampledColorAY * blendWeights.y +
		sampledColorAZ * blendWeights.z;


	//
	bool inBounds =
		writecoord.x >= 0 && writecoord.x < (int)vxgi_cascade.voxel_radiance_grid_resolution_uint() &&
		writecoord.y >= 0 && writecoord.y < (int)vxgi_cascade.voxel_radiance_grid_resolution_uint() &&
		writecoord.z >= 0 && writecoord.z < (int)vxgi_cascade.voxel_radiance_grid_resolution_uint()
		;

	[branch]
	if( inBounds )
	{
		const float3 view_space_position = mul(
			g_view_matrix,
			float4(pixelInput.position_world, 1)
		).xyz;

#if 1
		const bool hit_anything = IsInShadow(
			pixelInput.position_world
			, pixelInput.normalWS
			
			, g_directional_light.light_vector_WS.xyz

			, t_sdf_atlas3d
			, s_trilinear
		);

		float sun_visibility = hit_anything ? 0 : 1;
#else
		float sun_visibility = 1;	// not shadowed
#endif
		float backlightFactor = saturate(
			dot( pixelInput.normalWS, g_directional_light.light_vector_WS.xyz )
		);

		//const uint color_encoded = ~0;// 0xFF00FF00;//G,A
		//const uint color_encoded = packR8G8B8A8( float4( posInVoxelGrid, 1 ) );
		//const uint color_encoded = packR8G8B8A8( float4( pixelInput.normalWS, 1 ) );	// visualize world-space normals (flickering can be seen)
		//const uint color_encoded = packR8G8B8A8( float4( pixelInput.position01_in_voxel_grid, 1 ) );	// visualize world-space normals (flickering can be seen)

		//const uint color_encoded = packR8G8B8A8( float4( sun_visibility, sun_visibility, sun_visibility, 1 ) );
		//const uint color_encoded = packR8G8B8A8( float4( blendedColor * sun_visibility * backlightFactor * 0.1, 1 ) );

	
		
		//const float voxel_density_from_cpu = t_voxelDensityGrid[ writecoord ];

		
		const uint color_encoded = encodeAlbedoOpacity(
			float4(
				blendedColor * sun_visibility * backlightFactor,
				1
			)
		);

		const uint writeIndex1D = FlattenIndex3D(
			(uint3)writecoord,
			(uint3)vxgi_cascade.voxel_radiance_grid_resolution_uint()
		);

		InterlockedMax( rwsb_voxelGrid[writeIndex1D], color_encoded );
	}
	else
	{
		//rwsb_voxelGrid[writeIndex1D] = 0;
		//rwsb_voxelGrid[writeIndex1D] = 0xFF0000FF;//G,A
		return;
	}
}

/*
References:

The Basics of GPU Voxelization
By Masaya Takeshige, posted Mar 22 2015 at 11:44PM 
https://developer.nvidia.com/content/basics-gpu-voxelization

https://www.gamedev.net/forums/topic/695787-conservative-rasterization/

https://shlomisteinberg.com/2015/12/18/dense-voxelization-into-a-sparsely-allocated-3d-lattice/

https://github.com/ychding11/GraphicsCollection/wiki/GPU-Voxelization-and-Its-Related-Effects
*/
