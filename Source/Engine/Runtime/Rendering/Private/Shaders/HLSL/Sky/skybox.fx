/*

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

feature SKY_RENDERING_MODE
{
	bits = 1	// 0 = analytic model, 1 - skybox cube map
	default = 0	// analytic model
}

%%
*/

#include "base.hlsli"
#include <Common/transform.hlsli>
//#include "vxgi_interop.h"
//#include "vxgi_common.hlsli"
// 
#include <Shared/nw_shared_globals.h>
#include "base.hlsli"
#include <Sky/clearsky.hlsl>


TextureCube		t_cubeMap;
SamplerState	s_cubeMap;	// trilinear sampler

//=====================================================================

struct Skybox_VS_Out
{
    float4 position : SV_Position;
	// Because we have a axis aligned cube that will never rotate,
	// we can just use the interpolated position of those vertices
	// (without any other transformations) as our look-up vector.
    float3 texCoord : TexCoord;
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
	
	#if SKY_RENDERING_MODE == 0	// analytic model
		output.texCoord = cube_vertex_relative_to_center.xyz;
	#endif
	
	#if SKY_RENDERING_MODE == 1	// skybox cube map
		// Use local vertex position as cubemap lookup vector.
		//NOTE: in our coordinate system Z axis points UP, and Y axis points forward
		output.texCoord = cube_vertex_relative_to_center.xzy;
	#endif

    return output;
}

//=====================================================================

float3 computeSkyColor_CIEClearSky( in Skybox_VS_Out input, in float3 light_vector_WS )
{
	float3 pixelToLight = normalize( input.texCoord.xyz );
	const float3 color = CIEClearSky( pixelToLight, light_vector_WS.xyz );
	return color;
}

float3 computeSkyColor_SkyboxEnvironmentMap( in Skybox_VS_Out input )
{
	//NOTE: we don't have to normalize input.TexCoord:
	const float3 color = t_cubeMap.Sample( s_cubeMap, input.texCoord ).rgb;
	return color;
}

//=====================================================================

float3 SkyboxPS( in Skybox_VS_Out pixelIn ) : SV_Target
{

#if SKY_RENDERING_MODE == 0	// analytic model
	return computeSkyColor_CIEClearSky( pixelIn, g_directional_light.light_vector_WS.xyz );
#endif

#if SKY_RENDERING_MODE == 1	// skybox cube map
	return computeSkyColor_SkyboxEnvironmentMap( pixelIn );
#endif

}
