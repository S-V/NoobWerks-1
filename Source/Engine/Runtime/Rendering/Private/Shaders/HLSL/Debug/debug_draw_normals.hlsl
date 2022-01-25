// shader for visualizing vertex normals (which can be useful for debugging)
#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <Common/transform.hlsli>

float g_lineLength = 1.0f;

// position and normal must be in the same coordinate space for this to work
struct VSOut
{
	float3 position : Position;
	float3 tangent : Tangent;
	float3 normal : Normal;
};
struct GSOut
{
	float4 position : SV_Position;
	float3 tangent : Tangent;
	float3 normal : Normal;
	float3 color : Color;
};

void VS_Main( in DrawVertex inputs, out VSOut outputs )
{
	float3 localTangent = UnpackVertexNormal(inputs.tangent);
	float3 localNormal = UnpackVertexNormal(inputs.normal);	

	outputs.position = inputs.position;
	outputs.tangent = localTangent;
	outputs.normal = localNormal;
}

/*
===============================================================================
	GEOMETRY SHADER
===============================================================================
*/
[maxvertexcount(2)]
void GS_Main( point VSOut inputs[1], inout LineStream<GSOut> lineStream )
{
	VSOut input = inputs[0];

	float3 worldTangent = Dir_Local_To_World( input.tangent );
	float3 worldNormal = Dir_Local_To_World( input.normal );
	float3 lineColor = worldNormal.rgb;
	float3 startPosition = input.position;

	GSOut	start;
	start.position = Pos_Local_To_Clip( float4(startPosition, 1.0f) );
	start.tangent = worldTangent;
	start.normal = worldNormal;
	start.color = float3(1,1,1);
	lineStream.Append( start );

	float3 endPosition = input.position + start.normal * g_lineLength;

	GSOut	end;
	end.position = Pos_Local_To_Clip( float4(endPosition, 1.0f) );
	end.tangent = worldTangent;
	end.normal = worldNormal;
	end.color = lineColor;
	lineStream.Append( end );

    lineStream.RestartStrip();
}

void PS_Main( in GSOut input, out float3 output : SV_Target )
{
	output = input.color;
}
