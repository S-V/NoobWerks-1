#ifndef __SKY_SHADER_H__
#define __SKY_SHADER_H__


//float3 getAmbientCubeDirectionForCubeFace( int cubeFaceIndex )
//{
//	//cubeFaceIndex
//}
static const float3 g_AmbientCubeDirections[6] =
{
	float3( +1.0f,  0.0f,  0.0f ),
	float3( -1.0f,  0.0f,  0.0f ),

	float3(  0.0f, +1.0f,  0.0f ),
	float3(  0.0f, -1.0f,  0.0f ),

	float3(  0.0f,  0.0f, +1.0f ),
	float3(  0.0f,  0.0f, -1.0f ),
};

float3 dbg_computeSkyColor( in float3 direction )
{
	float3 color = (float3) 0;

	//NOTE: must put [unroll] here - otherwise, the resulting color will be wrong for Z-up direction!!!
	[unroll]
	for( int cubemapFace = 0; cubemapFace < 6; cubemapFace++ )
	{
		const float3 cubeFaceDir = g_AmbientCubeDirections[ cubemapFace ];

		const float NdotL = saturate( dot( direction, cubeFaceDir ) );

		const float3 skyRadiance = max( cubeFaceDir, 0 ) * NdotL;

		// +X - red, +Y - green, +Z - blue
		if(
			//cubemapFace == 0 ||
			//cubemapFace == 2 ||
			cubemapFace == 4
			)
		{
			color += skyRadiance;
		}
	}

	return color;
}

//-------------------------------------------------------------------------------------------------
// Uses the CIE Clear Sky model to compute a color for a pixel, given a direction + sun direction
//-------------------------------------------------------------------------------------------------
float3 CIEClearSky( in float3 dir, in float3 sunDir )
{
	float3 skyDir = float3(dir.x, dir.y, abs(dir.z));
	float gamma = AngleBetween(skyDir, sunDir);
	float S = AngleBetween(sunDir, float3(0, 0, 1));
	float theta = AngleBetween(skyDir, float3(0, 0, 1));

	float cosTheta = cos(theta);
	float cosS = cos(S);
	float cosGamma = cos(gamma);

	float num = (0.91f + 10 * exp(-3 * gamma) + 0.45 * cosGamma * cosGamma) * (1 - exp(-0.32f / cosTheta));
	float denom = (0.91f + 10 * exp(-3 * S) + 0.45 * cosS * cosS) * (1 - exp(-0.32f));

	float lum = num / max(denom, 0.0001f);

	// Clear Sky model only calculates luminance, so we'll pick a strong blue color for the sky
	const float3 SkyColor = float3(0.2f, 0.5f, 1.0f) * 1;
	const float3 SunColor = float3(1.0f, 0.8f, 0.3f) * 1500;
	const float SunWidth = 0.015f;

	float3 color = SkyColor;

	// Draw a circle for the sun
	static const bool bDrawSunDisk = true;
	if (bDrawSunDisk)
	{
		float sunGamma = AngleBetween(dir, sunDir);
		color = lerp(SunColor, SkyColor, saturate(abs(sunGamma) / SunWidth));
	}

	return max(color * lum, 0);
}

#endif // __SKY_SHADER_H__
