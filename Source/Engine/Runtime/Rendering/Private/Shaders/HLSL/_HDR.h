// Helper Functions for High Dynamic Range (HDR) rendering.

#ifndef __HDR_COMMON_H__
#define __HDR_COMMON_H__

#include "base.hlsli"

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return max(dot(color, LUM_WEIGHTS), 0.0001f);
}

/// Retrieves the log-average luminance from the texture.
float GetAvgLuminance( Texture2D luminanceTexture )
{
	uint textureWidth, textureHeight;
	uint numberOfMipLevels;
	luminanceTexture.GetDimensions(
	  0/*mipLevel*/,
	  textureWidth,
	  textureHeight,
	  numberOfMipLevels
	);
	// sample the last level (of size 1x1)
	const uint lastMipLevel = numberOfMipLevels - 1;
	return exp( luminanceTexture.Load( int3( 0, 0, lastMipLevel ) ).r );
}

/// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);

    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}


/// Determines the color based on exposure settings
float3 CalcExposedColor(
						in float3 color
						, in float avgLuminance
						, in float threshold
						, in float exposureKey
						, out float exposure
						)
{
	// Use geometric mean
	avgLuminance = max(avgLuminance, 0.001f);
	float linearExposure = (exposureKey / avgLuminance);
	exposure = log2(max(linearExposure, 0.0001f));
    exposure -= threshold;
    return exp2(exposure) * color;
}

/// Applies exposure and tone mapping to the specific color,
// and applies the threshold to the exposure value.
float3 ToneMap(
			   in float3 color
			   , in float avgLuminance
			   , in float threshold
			   , in float exposureKey
			   , out float exposure
			   )
{
    float pixelLuminance = CalcLuminance(color);
    color = CalcExposedColor(color, avgLuminance, threshold, exposureKey, exposure);
	color = ToneMapFilmicALU(color);
    return color;
}


#if 0

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight(int sampleDist, float sigma)
{
	float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
	return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}
#endif

#endif //__HDR_COMMON_H__
