/*
Debug primitive rendering.

%%
pass DebugLines {
	vertex = VertexShaderMain
	pixel = PixelShaderMain

	state = default
}
%%
*/

#include "_VS.h"

struct VSOut
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord;
	float4 color : Color;
	float3 normal : Normal;
};

row_major float4x4	u_worldViewProjection;

void VertexShaderMain( in AuxVertex inputs, out VSOut outputs )
{
	float4 localPosition = float4( inputs.position, 1.0f );
	float3 localNormal = UnpackVertexNormal( inputs.normal );

	outputs.position = mul( u_worldViewProjection, localPosition );
	outputs.texCoord = inputs.texCoord;
	outputs.color = inputs.color;
	outputs.normal = localNormal;
}

void PixelShaderMain( in VSOut inputs, out float4 pixelColor_ : SV_Target )
{
	pixelColor_ = inputs.color;
}
