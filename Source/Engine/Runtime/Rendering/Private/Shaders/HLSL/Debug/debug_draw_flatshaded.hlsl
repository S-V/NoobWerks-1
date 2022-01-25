// shader for visualizing vertex normals (which can be useful for debugging)
#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <Common/transform.hlsli>

cbuffer Data
{
	float4	LightVector = float4(0, 0, -1, 0);	// in world space
	float4	LightColor = float4(0.1, 0.4, 0.2, 1);
};

//#define DrawVertex App_to_VS
struct App_to_VS
{
	float3 position : Position;
};
struct VS_to_GS
{
	float3 position : Position;	// in world space
};
struct GS_to_PS
{
	float4 position : SV_Position;
	float3 color : Color;
};

void VS_Main(
			 in App_to_VS input,
			 out VS_to_GS output
			 )
{
	float4 localPosition = float4(input.position, 1.0f);
	output.position = Pos_Local_To_World(localPosition).xyz;
}

[maxvertexcount(3)]
void GS_Main(
			 triangle VS_to_GS inputs[3],
			 inout TriangleStream< GS_to_PS > outputs
			 )
{
	const float3 a = inputs[0].position;
	const float3 b = inputs[1].position;
	const float3 c = inputs[2].position;

	const float3 worldNormal = normalize( cross( b - a, c - a ) );


	float3 lineColor = worldNormal.rgb;
	float3 startPosition = input.position;

	VS_to_GS	start;
	start.position = Pos_Local_To_Clip( float4(startPosition, 1.0f) );
	start.tangent = worldTangent;
	start.normal = worldNormal;
	start.color = float3(1,1,1);
	outputs.Append( start );

	float3 endPosition = input.position + start.normal * g_lineLength;

	VS_to_GS	end;
	end.position = Pos_Local_To_Clip( float4(endPosition, 1.0f) );
	end.tangent = worldTangent;
	end.normal = worldNormal;
	end.color = lineColor;
	outputs.Append( end );

    outputs.RestartStrip();
}

void PS_Main(
			 in GS_to_PS input,
			 out float3 output : SV_Target
			 )
{
	output = input.color;
}
