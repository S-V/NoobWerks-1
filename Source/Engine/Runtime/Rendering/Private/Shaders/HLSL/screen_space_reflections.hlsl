#include <Shared/nw_shared_globals.h>
#include "_gbuffer.h"
#include "_lighting.h"
#include "_screen_shader.h"

cbuffer DATA
{
	PointLight g_light;
};

struct VS_OUT
{
    float4 clipPosition : SV_Position;
	float2 viewPosition : Position0;	// x and y are linear in screen space, we can use them to restore view-space position
	float2 texCoord : TexCoord0;
	//float3 viewRay : TexCoord1;
};

VS_OUT VS_SSR( in uint vertexID: SV_VertexID )
{
    VS_ScreenOutput screenVertex;
	GetFullScreenTrianglePosTexCoord( vertexID, screenVertex.position, screenVertex.texCoord );

	VS_OUT	output;
	output.clipPosition = screenVertex.position;

	output.viewPosition.x = output.clipPosition.x * g_ProjParams2.x;
	output.viewPosition.y = output.clipPosition.y * g_ProjParams2.y;
	
	output.texCoord = screenVertex.texCoord;

	//float4 view_space_position = mul(g_inverse_projection_matrix, screenVertex.position);
	//view_space_position.xyz /= view_space_position.w;

	//output.viewRay = view_space_position.xyz;

    return output;
}

float3 PS_SSR( in VS_OUT _inputs ) : SV_Target
{
	GBufferData surface = GBuffer_Sample_Data( _inputs.texCoord.xy );

	// read hardware depth (Z/W)
	float hardware_z_depth = DepthTexture.Load( int3(_inputs.clipPosition.x, _inputs.clipPosition.y, 0) );

	float4 clipSpacePosition = float4( _inputs.clipPosition.xy, hardware_z_depth, 1 );

	// recover view-space position
	float3 view_space_position = float3( _inputs.viewPosition.x, 1, _inputs.viewPosition.y ) * HardwareDepthToInverseW( hardware_z_depth );


	//float3 decodedNormal = (tex2D( _CameraGBufferTexture2, _inputs.texCoord)).rgb * 2.0 - 1.0;
	//decodedNormal = mul( (float3x3)_NormalMatrix, decodedNormal);

	float3 vsRayOrigin = view_space_position;
	
	float3 vsRayDirection = normalize( reflect( normalize( rayOrigin), normalize(normal)));

	float2 hitPixel; 
	float3 hitPoint;
	float iterationCount;

	float2 uv2 = _inputs.texCoord * _RenderBufferSize;
	float c = (uv2.x + uv2.y) * 0.25;
	float jitter = fmod( c, 1.0);

	bool intersect = traceScreenSpaceRay( rayOrigin, vsRayDirection, jitter, hitPixel, hitPoint, iterationCount, _inputs.texCoord.x > 0.5);
	float alpha = calculateAlphaForIntersection( intersect, iterationCount, specularStrength, hitPixel, hitPoint, rayOrigin, vsRayDirection);
	hitPixel = lerp( _inputs.texCoord, hitPixel, intersect);

	// Comment out the line below to get faked specular,
	// in no way physically correct but will tint based
	// on spec. Physically correct handling of spec is coming...
	specRoughPixel = half4( 1.0, 1.0, 1.0, 1.0);

	return half4( (tex2D( _MainTex, hitPixel)).rgb * specRoughPixel.rgb, alpha);

	return result;
}
