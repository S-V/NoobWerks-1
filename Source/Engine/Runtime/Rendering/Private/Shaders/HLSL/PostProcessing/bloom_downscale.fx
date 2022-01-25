/*
// Uses hw bilinear filtering for upscaling or downscaling

%%
pass Bloom_Downscale
{
	vertex = FullScreenTriangle_VS
	pixel = Downscale
	state = nocull
}

samplers
{
	s_src = BilinearSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"


Texture2D		t_src : register(t4);
SamplerState	s_src : register(s4);

float3 Downscale( in VS_ScreenOutput ps_in ) : SV_Target
{
    return t_src.Sample(
		s_src
		, ps_in.texCoord
	).rgb;
}
