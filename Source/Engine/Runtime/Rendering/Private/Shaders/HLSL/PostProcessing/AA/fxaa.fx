; 
/*

Fast Approximate Anti-Aliasing (FXAA)

%%
pass FXAA
{
	vertex = FullScreenTriangle_VS
	pixel = FXAA_PSMain
	state = nocull
}

samplers
{
	s_anisotropic = AnisotropicSampler
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include "_screen_shader.h"

#if 0

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12
#include "FXAA3_11.h"

/** 
 * parameters description:
 * QUALITY_SUBPIX : 
 * Choose the amount of sub-pixel aliasing removal. 
 * 		1.00 - upper limit (softer)
 * 		0.75 - default amount of filtering
 * 		0.50 - lower limit (sharper, less sub-pixel aliasing removal)
 * 		0.25 - almost off
 * 		0.00 - completely off
 * QUALITY_EDGE_THRESHOLD :
 * The minimum amount of local contrast required to apply algorithm.
 * 		0.333 - too little (faster)
 * 		0.250 - low quality
 * 		0.166 - default
 * 		0.125 - high quality 
 * 		0.063 - overkill (slower)
 * QUALITY_EDGE_THRESHOLD_MIN
 * Trims the algorithm from processing darks.
 * 		0.0833 - upper limit (default, the start of visible unfiltered edges)
 * 		0.0625 - high quality (faster)
 * 		0.0312 - visible limit (slower)
 */
#define QUALITY_SUBPIX 0.75f
#define QUALITY_EDGE_THRESHOLD 0.166f
#define QUALITY_EDGE_THRESHOLD_MIN 0.0833f

	
// source texture
Texture2D		t_screen;

// FXAA presets 0 through 2 require an anisotropic sampler with max anisotropy set to 4
SamplerState	s_anisotropic;	// for FXAA

float4 FXAA_PSMain( in VS_ScreenOutput _inputs ) : SV_Target
{
	// {x_} = 1.0/screenWidthInPixels
	// {_y} = 1.0/screenHeightInPixels
	const float2 texelSize = { g_framebuffer_dimensions.z, g_framebuffer_dimensions.w };

	FxaaTex	t;
	t.smpl = s_anisotropic;
	t.tex = t_screen; /* should be in gamma-space */

	return FxaaPixelShader(
		_inputs.texCoord.xy, // {xy} = center of pixel
		0,  // Used only for FXAA Console
		t,	// Input color texture.
		t,	// Only used on the optimized 360 version of FXAA Console.
		t,	// Only used on the optimized 360 version of FXAA Console.
		texelSize,
		0,	// Only used on FXAA Console.
		0,	// Only used on FXAA Console.
		0,	// Only used on FXAA Console.
		QUALITY_SUBPIX,
		QUALITY_EDGE_THRESHOLD,
		QUALITY_EDGE_THRESHOLD_MIN,
		0, 0, 0, 0);
}

#else

#define FXAA_PRESET 3 
#define FXAA_HLSL_4 1
#include <PostProcessing/AA/FXAA.h>


// source texture
Texture2D		t_screen;

// FXAA presets 0 through 2 require an anisotropic sampler with max anisotropy set to 4
SamplerState	s_anisotropic;	// for FXAA

float3 FXAA_PSMain( in VS_ScreenOutput _inputs ) : SV_Target
{
	// {x_} = 1.0/screenWidthInPixels
	// {_y} = 1.0/screenHeightInPixels
	const float2 texelSize = { g_framebuffer_dimensions.z, g_framebuffer_dimensions.w };

	FxaaTex	t;
	t.smpl = s_anisotropic;
	t.tex = t_screen; /* should be in gamma-space */

	return FxaaPixelShader(
		_inputs.texCoord.xy, // {xy} = center of pixel
		t,	// Input color texture.
		texelSize
	);
}

#endif
