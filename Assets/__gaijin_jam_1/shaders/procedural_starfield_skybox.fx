/*

Based on https://www.shadertoy.com/view/Md2SR3

%%
pass Sky
{
	vertex = SkyboxVS
	pixel = SkyboxPS

	state = skybox
}

samplers
{
	s_cubeMap = TrilinearSampler
}

%%
*/

#include "base.hlsli"
#include <Common/transform.hlsli>
#include <Common/Colors.hlsl>
#include <Shared/nw_shared_globals.h>
#include <Sky/clearsky.hlsl>


TextureCube<float>	t_cubeMap;	// star intensity (0..1)
SamplerState		s_cubeMap;	// trilinear sampler

//=====================================================================

struct Skybox_VS_Out
{
    float4 position : SV_Position;
	// Because we have a axis aligned cube that will never rotate,
	// we can just use the interpolated position of those vertices
	// (without any other transformations) as our look-up vector.
    float3 texCoord : TexCoord0;
};

Skybox_VS_Out SkyboxVS(
					   in uint cube_vertex_idx: SV_VertexID
					   )
{
	const float3 cube_vertex_pos_01 = getCubeCoordForVertexId( cube_vertex_idx );
	const float3 cube_vertex_relative_to_center = cube_vertex_pos_01 - (float3)0.5f;	// [-0.5 .. +0.5]

	//
    Skybox_VS_Out output;

	// Rotate into view-space, centered on the camera.
    float3 positionVS = mul(
		(float3x3)g_view_matrix,
		cube_vertex_relative_to_center
		);

	// Transform to clip-space.

#if USE_REVERSED_DEPTH

	//
    output.position = mul(
		g_projection_matrix,
		float4(positionVS, 1.0f)
	).xyww;

	// NOTE: the depth buffer is cleared to zero, so skybox Z must be greater OR EQUAL
	output.position.z = 0;

#else

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    output.position = mul(
		g_projection_matrix,
		float4(positionVS, 1.0f)
	).xyww;

#endif

	/*
	@from: http://antongerdelan.net/opengl/cubemaps.html
	Note: I've seen other tutorials that manipulate the Z and W components of gl_Position such that,
	after perspective division the Z distance will always be 1.0 - maximum distance in normalized device space,
	and therefore always in the background.
	This is a bad idea.
	It will clash, and possibly visibly flicker or draw all-black, when depth testing is enabled.
	*/
	
	output.texCoord = cube_vertex_relative_to_center.xyz;

    return output;
}

//=====================================================================


//-------------------------------------------------------------------------------------------------
// Uses the CIE Clear Sky model to compute a color for a pixel, given a direction + sun direction
//-------------------------------------------------------------------------------------------------
float3 Modified_CIEClearSky( in float3 dir, in float3 sunDir )
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
	const float SunWidth = 0.01f;

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

//=====================================================================

float3 SkyboxPS( in Skybox_VS_Out ps_in ) : SV_Target
{
	const float3 light_vector_WS = g_directional_light.light_vector_WS.xyz;
	
	float3 pixelToLight = normalize( ps_in.texCoord.xyz );
	float3 color = Modified_CIEClearSky( pixelToLight, light_vector_WS.xyz );
	
	color += float3(1, 0.4, 0.5);
	color *= 0.001f;

	const float star_intensity = t_cubeMap.Sample( s_cubeMap, ps_in.texCoord ).r;
	color += (float3) (star_intensity * 0.01);	// stars are very dim!

//
	//color *= 2.0f;

	return color;
}
