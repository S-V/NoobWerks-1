/*
Extracts the bright parts of the scene.

%%
pass HDR_CreateLuminance
{
	vertex = FullScreenTriangle_VS
	pixel = LuminanceMap
	state = nocull
}

samplers
{
	s_hdr_scene_color = BilinearSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_HDR.h"

Texture2D		t_hdr_scene_color : register(t4);
Texture2D		t_adaptedLuminance : register(t6);

SamplerState	s_hdr_scene_color : register(s4);

// Uses a lower exposure to produce a value suitable for a bloom pass
float3 LuminanceMap( in VS_ScreenOutput ps_in ) : SV_Target
{
	// Sample the input
    const float3 sampled_scene_color = t_hdr_scene_color.Sample(
		s_hdr_scene_color
		, ps_in.texCoord
	).rgb;

	// Tone map it to threshold
	const float avgLuminance = GetAvgLuminance(t_adaptedLuminance);

	float exposure = 0;

	return ToneMap(
		sampled_scene_color,
		avgLuminance,
		g_fBloomThreshold,
		g_fExposureKey,
		exposure
		);
}
