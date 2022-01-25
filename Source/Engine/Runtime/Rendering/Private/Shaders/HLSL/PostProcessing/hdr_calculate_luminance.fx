/*
Creates the luminance map for the scene.

%%
pass HDR_CreateLuminance
{
	vertex = FullScreenTriangle_VS
	pixel = LuminanceMap
	state = nocull
}

samplers
{
	s_screen = BilinearSampler
}

%%

*/

#include <Shared/nw_shared_definitions.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_HDR.h"

// source texture
Texture2D		t_screen : register(t4);
// linear sampler
SamplerState	s_screen : register(s4);

float LuminanceMap( in VS_ScreenOutput _inputs ) : SV_Target
{
	// Sample the input
    float3 color = t_screen.Sample( s_screen, _inputs.texCoord ).rgb;

    // calculate the luminance using a weighted average
    float luminance = CalcLuminance( color );
    return luminance;
}
