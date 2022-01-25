/*
Slowly adjusts the scene luminance based on the previous scene luminance.

%%
pass HDR_AdaptLuminance
{
	vertex = FullScreenTriangle_VS
	pixel = AdaptLuminance
	state = nocull
}

samplers
{
	s_pointSampler = PointSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_HDR.h"

// source textures
Texture2D		t_initialLuminance : register(t4);
Texture2D		t_adaptedLuminance : register(t5);
// point sampler
SamplerState	s_pointSampler : register(s4);

// Slowly adjusts the scene luminance based on the previous scene luminance.
float AdaptLuminance( in VS_ScreenOutput _inputs ) : SV_Target
{
	const float currentLuminance = t_initialLuminance.Sample(s_pointSampler, _inputs.texCoord).r;
    const float lastLuminance = exp(t_adaptedLuminance.Sample(s_pointSampler, _inputs.texCoord).r);	

    // Adapt the luminance using Pattanaik's technique

    // The user's adapted luminance level is simulated by closing the gap
	// between adapted luminance and current luminance.
	// This is not an accurate model of human adaptation,
	// which can take longer than half an hour.
	const float deltaSeconds = g_DeltaTime.x;
    float adaptedLum = lastLuminance + (currentLuminance - lastLuminance) * (1 - exp(-deltaSeconds * g_fHdrAdaptationRate));

    return log(adaptedLum);
}
