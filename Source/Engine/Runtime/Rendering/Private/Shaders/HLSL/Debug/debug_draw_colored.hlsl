#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <Common/transform.hlsli>

float3 g_color = float3(0.4f, 0.7f, 0.5f);

// position and normal must be in the same coordinate space for this to work
struct VSOut
{
	float4 position : SV_Position;
	float3 tangent : Tangent;
	float3 normal : Normal;
	float3 color : Color;
};

void VS_Main( in DrawVertex inputs, out VSOut outputs )
{
	float4 localPosition = float4(inputs.position, 1.0f);
	float3 localTangent = UnpackVertexNormal(inputs.tangent);
	float3 localNormal = UnpackVertexNormal(inputs.normal);

	outputs.position = Pos_Local_To_Clip( localPosition );
	outputs.tangent = Dir_Local_To_World( localTangent );
	outputs.normal = Dir_Local_To_World( localNormal );
	outputs.color = g_color;
}

void PS_Main( in VSOut input, out float3 output : SV_Target )
{
	//output = input.color;
	output = float3(1,1,1);
}
