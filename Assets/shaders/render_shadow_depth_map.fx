// This shader only transforms position (e.g. for depth pre-pass).
// It is used for rendering shadow casters into a shadow depth map.

/*
%%
pass DepthOnly {
	vertex = main_VS
	// null pixel shader

	state = default
	
	//feature bPackedPositions {
	//	; Are input vertex positions quantized?
	//	name = 'bPackedPositions'
	//	defaultValue = 0
	//}
}
%%
*/
#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"

float4 main_VS( in Vertex_DLOD _inputs ) : SV_Position
{
	// const float3 normalizedPosition = unpackNormalizedPosition_DLoD( _inputs.xyz );	// [0..1]
	// const float4 localPosition = float4( normalizedPosition, 1.0f );
	// return Pos_Local_To_Clip(localPosition);
	return float4(0,0,0,0);
}

// Null pixel shader.
