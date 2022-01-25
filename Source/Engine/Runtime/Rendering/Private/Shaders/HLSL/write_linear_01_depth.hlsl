// write view space depth, normalized to [0..1] range

#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include "_gbuffer.h"
#include <Common/transform.hlsli>

struct VS_OUT
{
	float4 clipPosition : SV_Position;
	float linearDepth : TexCoord;
};

void VS_Main( in float3 position : SV_Position, out VS_OUT outputs )
{
	float4 localPosition = float4(position, 1.0f);
	outputs.clipPosition = Pos_Local_To_Clip( localPosition );
	float4 viewPosition = Pos_Local_To_View( localPosition );
	outputs.linearDepth = viewPosition.z * g_DepthClipPlanes.w;
}

void PS_Main( in VS_OUT inputs, out float output : SV_Target )
{
	output = inputs.linearDepth;
}
