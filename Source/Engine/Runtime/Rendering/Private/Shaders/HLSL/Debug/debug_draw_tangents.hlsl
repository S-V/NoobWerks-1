// shader for visualizing vertex normals (which can be useful for debugging)
#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <Common/transform.hlsli>

float g_lineLength = 1.0f;

// position and normal must be in the same coordinate space for this to work
struct VSOut
{
	float3 position : Position;
	float3 bitangent : Bitangent;
	float3 tangent : Tangent;
	float3 normal : Normal;
};
struct GSOut
{
	float4 position : SV_Position;
	float3 color : Color;
};

void VS_Main( in DrawVertex inputs, out VSOut outputs )
{
	float3 localNormal = UnpackVertexNormal(inputs.normal);
	float3 localTangent = UnpackVertexNormal(inputs.tangent);
	//NOTE: negate because Direct3D's cross is left-handed while our coord. system is right-handed
	float3 localBitangent = -cross(localTangent, localNormal);

	outputs.position = inputs.position;
	outputs.bitangent = localBitangent;
	outputs.tangent = localTangent;
	outputs.normal = localNormal;
}

/*
===============================================================================
	GEOMETRY SHADER
===============================================================================
*/
void DrawLine( float3 startPosition, float3 direction, float3 color, inout LineStream<GSOut> lineStream )
{
	GSOut	start;
	start.position = Pos_Local_To_Clip( float4(startPosition, 1.0f) );
	start.color = color;
	lineStream.Append( start );

	float3 endPosition = startPosition + direction * g_lineLength;

	GSOut	end;
	end.position = Pos_Local_To_Clip( float4(endPosition, 1.0f) );
	end.color = color;
	lineStream.Append( end );
	
	lineStream.RestartStrip();
}

[maxvertexcount(6)]
void GS_Main( point VSOut inputs[1], inout LineStream<GSOut> lineStream )
{
	VSOut input = inputs[0];

	float3 worldBitangent = Dir_Local_To_World( input.bitangent );
	float3 worldTangent = Dir_Local_To_World( input.tangent );
	float3 worldNormal = Dir_Local_To_World( input.normal );

	worldBitangent = normalize(worldBitangent);
	worldTangent = normalize(worldTangent);
	worldNormal = normalize(worldNormal);

	float3 startPosition = input.position;

	// Normal
	DrawLine( startPosition, worldNormal, float3(0,0,1), lineStream );

	// Tangent
	DrawLine( startPosition, worldTangent, float3(1,0,0), lineStream );
	
	// Bitangent
	DrawLine( startPosition, worldBitangent, float3(0,1,0), lineStream );    
}

void PS_Main( in GSOut input, out float3 output : SV_Target )
{
	output = input.color;
}
