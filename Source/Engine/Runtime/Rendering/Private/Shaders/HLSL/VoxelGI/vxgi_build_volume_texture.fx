/*

Builds a volume texture from the buffer.

%%
pass VXGI_BuildVolumeTexture {
	compute = main_CS
}
%%

Heavily based on Wicked engine (voxelSceneCopyClearCS.hlsl).
*/

#include "base.hlsli"
#include <Common/transform.hlsli>

#include "vxgi_interop.h"
#include "vxgi_common.hlsli"

#define USE_CPU_DENSITY_TEXTURE	(0)

#if USE_CPU_DENSITY_TEXTURE
// density (R8_UNORM) - updated on CPU, has the same dimension as voxel cascades
Texture3D< float >					t_voxelDensityGrid;
#endif

RWStructuredBuffer< VXGI_Voxel >	srcVoxelGridBuffer;
RWTexture3D< float4 >				dstVolumeTexture;

//-----------------------------------------------------------------------------

[numthreads( 8, 8, 8 )]
void main_CS( uint3 threadId: SV_DispatchThreadID )
{
	const uint flatIndexInBuffer = FlattenIndex3D( threadId, (uint3)g_vxgi.cascadeResolutionI() );
	const VXGI_Voxel srcVoxel = srcVoxelGridBuffer[ flatIndexInBuffer ];

	const uint3 writecoord = threadId;

	//
#if USE_CPU_DENSITY_TEXTURE
	const float voxel_density_from_cpu = t_voxelDensityGrid[ writecoord ];
#else
	const float voxel_density_from_cpu = 0;
#endif

	float4 albedo_opacity = decodeAlbedoOpacity( srcVoxel.albedo_opacity );
	albedo_opacity.a = max( albedo_opacity.a, voxel_density_from_cpu );

	dstVolumeTexture[writecoord] = albedo_opacity;

	// clear the source buffer
	srcVoxelGridBuffer[ flatIndexInBuffer ].albedo_opacity = 0;
}
