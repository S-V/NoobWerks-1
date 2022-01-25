/*
%%
pass RenderDepthOnly {
	vertex = main_VS

	// null pixel shader
	// see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage
	// On hardware that implements hierarchical Z-buffer optimizations,
	// you may enable preloading the z-buffer by setting the pixel shader stage to NULL while enabling depth and stencil testing.

	state = render_depth_to_shadow_map
}
%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>

float4 main_VS( in Vertex_DLOD _inputs ) : SV_Position
{
	// get the normalized vertex position in the chunk's seam space
	uint	material_id;
	const float3 normalizedPosition = unpack_NormalizedPosition_and_MaterialIDs( _inputs, material_id );	// [0..1]

	const float4 f4LocalPosition = float4( normalizedPosition, 1.0f );

	return Pos_Local_To_Clip( f4LocalPosition );
}
