/*
Performs gaussian blur.

%%
pass HDR_CreateLuminance
{
	vertex = FullScreenTriangle_VS
	pixel = MainPS
	state = nocull
}

// Horizontal gaussian blur?
feature HORIZONTAL_BLUR {}

samplers
{
	s_point_sampler = PointSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_HDR.h"



static const float BloomBlurSigma = 1;
static const float InputSize0 = 1024 / 8;


Texture2D		t_input_texture : register(t4);
SamplerState	s_point_sampler : register(s4);


cbuffer Data
{
	float4 g_avSampleWeights[15];
	float4 g_avSampleOffsets[15];
};


//-----------------------------------------------------------------------------
// Name: Bloom
// Type: Pixel shader                                      
// Desc: Blur the source image along the horizontal using a gaussian
//       distribution
//-----------------------------------------------------------------------------
float3 BloomBlurPS(
				   in float2 vScreenPosition : TEXCOORD0,
				   in float4 g_avSampleOffsets[15],
				   in float4 g_avSampleWeights[15]
				   ) : COLOR
{
    float3 vSample = 0.0f;
    float3 vColor = 0.0f;
    float2 vSamplePosition;
    
    for( int iSample = 0; iSample < 15; iSample++ )
    {
        // Sample from adjacent points
        vSamplePosition = vScreenPosition + g_avSampleOffsets[iSample].xy;
        vColor = t_input_texture.Sample(s_point_sampler, vSamplePosition).xyz;
        
        vSample += g_avSampleWeights[iSample].x * vColor;
    }
    
    return vSample;
}

float3 MainPS( in VS_ScreenOutput ps_in ) : SV_Target
{

#if HORIZONTAL_BLUR

	// Horizontal gaussian blur

	return BloomBlurPS(
		ps_in.texCoord
		, g_avSampleOffsets
		, g_avSampleWeights
		);
#else
	return BloomBlurPS(
		ps_in.texCoord
		, g_avSampleOffsets
		, g_avSampleWeights
		);

#endif

}
