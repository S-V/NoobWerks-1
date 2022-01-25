#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <Common/transform.hlsli>

float4 u_diffuse_color = float4(1,1,1,1);

// position and normal must be in the same coordinate space for this to work
struct VSOut
{
	float4 position : SV_Position;
	float3 normal : Normal;
};

void VS_Main( in DrawVertex inputs, out VSOut outputs )
{
	float4 localPosition = float4(inputs.position, 1.0f);
	float3 localNormal = UnpackVertexNormal(inputs.normal);
	outputs.position =Pos_Local_To_Clip(localPosition);
	outputs.normal = Dir_Local_To_View(localNormal);
}

void PS_Main( in VSOut input, out float3 output : SV_Target )
{
	output = u_diffuse_color.rgb;
}
