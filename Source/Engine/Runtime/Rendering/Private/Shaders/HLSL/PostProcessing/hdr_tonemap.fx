/*
Applies exposure and tone mapping to the input, and combines it with the results of the bloom pass.

%%
pass HDR_Tonemap
{
	vertex = FullScreenTriangle_VS
	pixel = PS_ToneMap
	state = nocull
}

samplers
{
	s_pointSampler = PointSampler
	s_linearSampler = BilinearSampler
}

feature bWithBloom {}

// FXAA (Fast Approximate Anti-Aliasing) requires colors in non-linear, gamma space
feature bApplyGammaCorrection {}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_HDR.h"
#include "_PS.h"

Texture2D		t_HDRSceneColor : register(t4);
Texture2D		t_bloom : register(t5);
Texture2D		t_adaptedLuminance : register(t6);

SamplerState	s_pointSampler : register(s4);
SamplerState	s_linearSampler : register(s5);

/// Applies exposure and tone mapping to the input, and combines it with the results of the bloom pass.
float3 PS_ToneMap( in VS_ScreenOutput _inputs ) : SV_Target
{
	// Tone map the primary input
    float3 color = t_HDRSceneColor.Sample( s_pointSampler, _inputs.texCoord ).rgb;

	//float avgLuminance = 0.7f;
	const float avgLuminance = GetAvgLuminance( t_adaptedLuminance );

	const float max_luminance_to_avoid_over_brightness = 0.03f;

	float exposure = 0;

	color = ToneMap(
		color,
		max(avgLuminance, max_luminance_to_avoid_over_brightness),
		0,
		g_fExposureKey,
		exposure
		);

#if bWithBloom
    // Sample the bloom
    const float3 bloom = t_bloom.Sample( s_linearSampler, _inputs.texCoord ).rgb;
    // Add in the bloom
	color += bloom * g_fBloomScale;
#endif

#if bApplyGammaCorrection
	// FXAA requires colors in non-linear, gamma space
	return LinearToGamma( color );
#else
	return color;
#endif
}
