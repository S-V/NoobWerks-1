// This shader only transforms position (e.g. for depth pre-pass).
// It's used for rendering shadow casters into a shadow depth map.

#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"

#if bPackedPositions

	#if bCLOD
		float4 VS_Main( in Vertex_CLOD _inputs ) : SV_Position
		{
			const float3 localPosition0 = CLOD_UnpackPosition0( _inputs.xyz0, g_objectScale.xyz );
			const float3 localPosition1 = CLOD_UnpackPosition1( _inputs.xyz1, g_objectScale.xyz );
const float4 localPosition = float4( localPosition0, 1.0f );
			return Pos_Local_To_Clip(localPosition);
		}
	#else
		float4 VS_Main( in Vertex_DLOD _inputs ) : SV_Position
		{
			return float4(0,0,0,0);
			//const float3 normalizedPosition = unpackNormalizedPosition_DLoD( _inputs.xyz );	// [0..1]
			//const float4 localPosition = float4( normalizedPosition, 1.0f );
			//return Pos_Local_To_Clip(localPosition);
		}
	#endif

#else

	float4 VS_Main( in float3 objectSpacePosition : Position ) : SV_Position
	{
		float4 localPosition = float4( objectSpacePosition, 1.0f );
		return Pos_Local_To_Clip( localPosition );
	}

#endif

// Null pixel shader.
