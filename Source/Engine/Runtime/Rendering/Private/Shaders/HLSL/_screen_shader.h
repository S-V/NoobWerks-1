/*
=============================================================================
	File:	_screen_shader.h
	Desc:	Shader code for rendering screen-space quads/triangles.
=============================================================================
*/
#ifndef H_SCREEN_SHADER_HLSL
#define H_SCREEN_SHADER_HLSL

//
//	This is used for drawing screen-aligned quads.
//
struct VS_ScreenInput
{
    float4 position : Position;
    float2 texCoord : TexCoord0;
};
struct VS_ScreenOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

VS_ScreenOutput ScreenVertexShader( in VS_ScreenInput input )
{
    VS_ScreenOutput output;
    output.position = input.position;
	output.texCoord = input.texCoord;
    return output;
}

/// Outputs a triangle that covers the entire viewport.
inline void GetFullScreenTrianglePosTexCoord(
	in uint vertexID: SV_VertexID,
	out float4 position,
    out float2 texCoord
	)
{
	// Produce a fullscreen triangle.

	const float farthest_depth = USE_REVERSED_DEPTH ? 0 : 1;

	// NOTE: when not using reverse depth, z = 1 (the farthest possible distance) -> optimization for deferred lighting/shading engines:
	// skybox outputs z = w = 1 -> if we set depth test to 'less' then the sky won't be shaded.
	// or you can just set your viewport so that the skybox is clamped to the far clipping plane, which is done by setting MinZ and MaxZ to 1.0f.

	//
	if( vertexID == 0 )// lower-left
	{
		position = float4( -1.0f, -3.0f, farthest_depth, 1.0f );
		texCoord = float2( 0.0f, 2.0f );
	}
	else if ( vertexID == 1 )// upper-left
	{
		position = float4( -1.0f,  1.0f, farthest_depth, 1.0f );
		texCoord = float2( 0.0f, 0.0f );
	}
	else //if ( vertexID == 2 )// upper-right
	{
		position = float4(  3.0f,  1.0f, farthest_depth, 1.0f );
		texCoord = float2( 2.0f, 0.0f );
	}
	/*
	another way to calc position ( from NVidia samples [2008] / Intel's Deferred Shading sample [2010] ):

	//--------------------------------------------------------------------------------------------------------
	struct FullScreenTriangleVSOut
	{
		float4 positionViewport : SV_Position;
	};

	// Parametrically work out vertex location for full screen triangle
	FullScreenTriangleVSOut output;
	float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
	output.positionViewport = float4(grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);

	or from Intel's sample [2010]:

	output.positionClip.x = (float)(vertexID / 2) * 4.0f - 1.0f;
	output.positionClip.y = (float)(vertexID % 2) * 4.0f - 1.0f;
	output.positionClip.z = 0.0f;
	output.positionClip.w = 1.0f;

	output.texCoord.x = (float)(vertexID / 2) * 2.0f;
	output.texCoord.y = 1.0f - (float)(vertexID % 2) * 2.0f;

	or yet another one:
	Out.position.x = (vertexID == 2)?  3.0 : -1.0;
	Out.position.y = (vertexID == 0)? -3.0 :  1.0;
	Out.position.zw = 1.0;




	//--------------------------------------------------------------------------------------------------------
	
	See: http://anteru.net/2012/03/03/1862/

	float4 VS_main (uint id : SV_VertexID) : SV_Position
	{
		switch (id) {
		case 0: return float4 (-1, 1, 0, 1);
		case 1: return float4 (-1, -1, 0, 1);
		case 2: return float4 (1, 1, 0, 1);
		case 3: return float4 (1, -1, 0, 1);
		default: return float4 (0, 0, 0, 0);
		}
	}

	All you need for rendering now is to issue a single draw call
	which renders a triangle strip containing four vertices, and you?re done.
	Note: This quad is for RH rendering, if your culling order is reversed,
	you?ll need to swap the second and third vertex.
	In the pixel shader, you can now use SV_Position, or alternatively,
	you can also generate UV coordinates in the vertex shader.

	//--------------------------------------------------------------------------------------------------------
	// Without the explicit casts, this does not compile correctly using the
	// D3D Compiler (June 2010 SDK)
	float x = float ((id & 2) << 1) - 1.0;
	float y = 1.0 - float ((id & 1) << 2);
	 
	return float4 (x, y, 0, 1);



	float4 result[3] = { float4(-1,-1,0.5,1), float4(-1,3,0.5,1), float4(3,-1,0.5,1) };
	return result[id];
	*/
}

//****************************************************************************************
/**
	FullScreenTriangle_VS

	rendered as a single triangle:

	ID3D11DeviceContext* context;

	(uses no vertex/index buffers)

	context->IASetInputLayout(nil);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3,0);
*/
VS_ScreenOutput FullScreenTriangle_VS( in uint vertexID: SV_VertexID )
{
    VS_ScreenOutput output;
	GetFullScreenTrianglePosTexCoord(
		vertexID
		, output.position
		, output.texCoord
		);
    return output;
}

//****************************************************************************************

inline void GetFullScreenQuadPosTexCoord(
	in uint vertexID: SV_VertexID,
	out float4 position,
    out float2 texCoord
	)
{
	// Produce a fullscreen triangle.
	// NOTE: z = 1 (the farthest possible distance) -> optimization for deferred lighting/shading engines:
	// skybox outputs z = w = 1 -> if we set depth test to 'less' then the sky won't be shaded.
	// or you can just set your viewport so that the skybox is clamped to the far clipping plane, which is done by setting MinZ and MaxZ to 1.0f.
	//
	if( vertexID == 0 )// upper-left
	{
		position = float4( -1.0f,  1.0f, 1.0f, 1.0f );
		texCoord = float2( 0.0f, 0.0f );
	}
	else if ( vertexID == 1 )// lower-left
	{
		position = float4( -1.0f, -1.0f, 1.0f, 1.0f );
		texCoord = float2( 0.0f, 1.0f );
	}
	else if ( vertexID == 2 )// upper-right
	{
		position = float4( 1.0f,  1.0f, 1.0f, 1.0f );
		texCoord = float2( 1.0f, 0.0f );
	}
	else //if ( vertexID == 3 )// lower-right
	{
		position = float4( 1.0f, -1.0f, 1.0f, 1.0f );
		texCoord = float2( 1.0f, 1.0f );
	}
}


#ifdef PER_VIEW_CONSTANTS_DEFINED

/**
	FullScreenQuad_VS
	rendered as a triangle strip:

	ID3D11DeviceContext* context;

	(uses no vertex/index buffers)

	context->IASetInputLayout(nil);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->Draw(4,0);
*/
struct VS_ScreenQuadOutput
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
	float3 eyeRayVS : TexCoord1;	// ray to far frustum corner in view space, used for restoring view-space pixel position from depth
	//float3 eyeRayWS : TexCoord2;	// ray to far frustum corner in world space, used for restoring world-space pixel position from depth
};

VS_ScreenQuadOutput FullScreenQuad_VS( in uint vertexID: SV_VertexID )
{
	VS_ScreenQuadOutput output;

	GetFullScreenQuadPosTexCoord( vertexID, output.position, output.texCoord );

	if( vertexID == 0 )// upper-left
	{
		output.eyeRayVS = frustumCornerVS_FarTopLeft.xyz;
	}
	else if ( vertexID == 1 )// lower-left
	{
		output.eyeRayVS = frustumCornerVS_FarBottomLeft.xyz;
	}
	else if ( vertexID == 2 )// upper-right
	{
		output.eyeRayVS = frustumCornerVS_FarTopRight.xyz;
	}
	else //if ( vertexID == 3 )// lower-right
	{
		output.eyeRayVS = frustumCornerVS_FarBottomRight.xyz;
	}

	//@fixme: this is wrong:
	//output.eyeRayWS = mul( float4( output.eyeRayVS, 0.0f ), inverse_view_matrix ).xyz;
	//output.eyeRayWS = normalize( output.eyeRayWS );

	return output;
}
#endif // PER_VIEW_CONSTANTS_DEFINED
//****************************************************************************************

#endif // H_SCREEN_SHADER_HLSL
