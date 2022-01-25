/*

The last post-processing shader.

Combine/Blit : LightingTarget -> FrameBuffer

The sky is rendered as a fullscreen triangle.
The pixel shader traces rays through each pixel.

%%
pass DeferredCompositeLit
{
	vertex = main_VS
	pixel = main_PS

	state = nocull
}

samplers
{
	s_screen = PointSampler
}

//feature ENABLE_ATMOSPHERE {}

%%

// OLD CODE:
scene_passes = [
	{
		name = 'DeferredCompositeLit'
		features = [
			; Global Fog
			{
				name = 'bEnableFog'
				defaultValue = 1
			}
			; Blended Order-Independent Transparency (OIT)
			{
				name = 'bAddBlendedOIT'
				defaultValue = 0
			}
			; FXAA (Fast Approximate Anti-Aliasing) requires colors in non-linear, gamma space
			{
				name = 'bApplyGammaCorrection'
				defaultValue = 0
			}
		]
	}
]
*/
#include <Shared/nw_shared_globals.h>
#include <GBuffer/nw_gbuffer_read.h>
#include "_screen_shader.h"
#include "base.hlsli"
#include "_basis.h"
#include "_fog.h"
#include "_PS.h"
#include <PostProcessing/AA/FXAA.h>
#include <Common/transform.hlsli>

#include <Common/Colors.hlsl>


#define ENABLE_ATMOSPHERE	(0)


#if ENABLE_ATMOSPHERE
#include <Sky/atmosphere_analytic.hlsl>
#include <Atmosphere/atmosphere_functions.h>
#endif




// source texture
Texture2D		t_screen : register(t4);	// let's just hope nobody binds to this slot
// point sampler
SamplerState	s_screen : register(s4);

// FXAA presets 0 through 2 require an anisotropic sampler with max anisotropy set to 4
//SamplerState	s_anisotropic : register(s5);	// for FXAA

// Blended Order-Independent Transparency (OIT)
Texture2D		t_blendedOIT_accumulated : register(t5);	// weighted color (premultiplied with alpha)
Texture2D		t_blendedOIT_revealage : register(t6);	// opacity/weights




#if ENABLE_ATMOSPHERE

Texture2D transmittance_texture         ;
Texture3D scattering_texture            ;
Texture3D single_mie_scattering_texture ;
Texture2D irradiance_texture            ;

SamplerState atmosphere_sampler         ;

#endif





// Uniforms
UeberPostProcessingShaderData	u_data;




struct VS_OUT
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;

	float3 corner_WS : TexCoord1;	// the ray's end point on the far image plane

#if 0
	float3 ray_WS : TexCoord2;
	float3 ray_VS : TexCoord3;

	float3 ray_dir_WS : TexCoord4;
#endif
};

/*
==========================================================
	VERTEX SHADER
==========================================================
*/
VS_OUT main_VS( in uint vertexID : SV_VertexID )	// vertexID:=[0,1,2]
{
    VS_OUT output;
	GetFullScreenTrianglePosTexCoord( vertexID, output.position, output.texCoord );

	//
	const float farPlaneDist = 1;	// distance from eye position to the image plane
	const float aspect_ratio = 1.3333;//g_camera_fwd_WS.w;	// screen width / screen height
	const float tanHalfFoVy = tan(0.78539819);// g_camera_right_WS.w;
	
	const float imagePlaneH = farPlaneDist * tanHalfFoVy;//g_viewport_tanHalfFoVy();	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * aspect_ratio;//g_viewport_aspectRatio();	// half-width of the image plane

	const float3 f3FarCenter = g_camera_fwd_WS.xyz * farPlaneDist;
	const float3 f3FarRight = g_camera_right_WS.xyz * imagePlaneW;	// scaled 'right' vector on the far plane, in world space
	const float3 f3FarUp = g_camera_up_WS.xyz * imagePlaneH;	// scaled 'up' vector on the far plane, in world space


	// Compute the positions of the triangle's corners on the far image plane, in world space.
#if 0

	float3 cornersWS[3];
	// lower-left ( UV = ( 0, 2 ) )
	cornersWS[0] = u_data.viewer.relative_eye_position.xyz + f3FarCenter - (f3FarRight * 1) - (f3FarUp * 3);
	// upper left ( UV = ( 0, 0 ) )
	cornersWS[1] = u_data.viewer.relative_eye_position.xyz + f3FarCenter - (f3FarRight * 1) + (f3FarUp * 1);
	// upper-right ( UV = ( 2, 0 ) )
	cornersWS[2] = u_data.viewer.relative_eye_position.xyz + f3FarCenter + (f3FarRight * 3) + (f3FarUp * 1);

	// the positions on the far image plane will be linearly interpolated and passed to the pixel shader
	output.corner_WS = cornersWS[ vertexID ];

#else

	if( vertexID == 0 )
	{
		output.corner_WS = u_data.viewer.relative_eye_position.xyz + f3FarCenter - (f3FarRight * 1) - (f3FarUp * 3);
	}
	else if( vertexID == 1 )
	{
		output.corner_WS = u_data.viewer.relative_eye_position.xyz + f3FarCenter - (f3FarRight * 1) + (f3FarUp * 1);
	}
	else //if( vertexID == 2 )
	{
		output.corner_WS = u_data.viewer.relative_eye_position.xyz + f3FarCenter + (f3FarRight * 3) + (f3FarUp * 1);
	}

#endif


    return output;
}



/*
==========================================================
	PIXEL SHADER
==========================================================
*/
float3 main_PS( in VS_OUT ps_in ) : SV_Target
{
	const float3 source_color = t_screen.SampleLevel( s_screen, ps_in.texCoord, 0 ).rgb;

	const float hardware_z_depth = DepthTexture.SampleLevel( s_screen, ps_in.texCoord, 0 );
	const float2 screen_space_position = TexCoordsToClipPos( ps_in.texCoord );
	const float3 view_space_position = restoreViewSpacePosition( screen_space_position, hardware_z_depth );

	//
	float3 result = source_color;


	const float view_space_depth = view_space_position.y;
//	const float view_space_depth = length(view_space_position);	// for distance-based fog

	//const float3 ray_dir_in_WS = normalize( ps_in.ray_dir_WS );
	const float3 ray_dir_in_WS = normalize( ps_in.corner_WS - u_data.viewer.relative_eye_position.xyz );

	const float3 camera_position_WS = u_data.viewer.relative_eye_position;










	// calculate the scattering towards the camera
	//




#if ENABLE_ATMOSPHERE
	

	float3 view_direction = float3(-0.34738, 0.00081, -0.93772);
    float shadow_length = 0;
    float3 transmittance;

#if 0
	float3 radiance = GetSkyRadiance(
		atmosphere,
		transmittance_texture, scattering_texture, single_mie_scattering_texture, atmosphere_sampler,
		float3(9.00, 0.00, 6359.99805),
		float3(-0.99829, -0.03879, -0.04376),
		0.0,
		float3(0.00059, 0.0, -1.0),
		transmittance
		);
#else
	float3 radiance = GetSkyRadiance(
		atmosphere,
		transmittance_texture, scattering_texture, single_mie_scattering_texture,
		atmosphere_sampler,
		camera - earth_center,
		view_direction, shadow_length, sun_direction,
		transmittance	// out
		);
#endif
	result += radiance;


	#if 0
					//
					AtmosphereRenderContext	atmosphere_render_context;
					atmosphere_render_context.camera_position = camera_position_WS;
					atmosphere_render_context.eye_to_pixel_ray_direction = ray_dir_in_WS;
					atmosphere_render_context.light_vector = g_directional_light.light_vector_WS.xyz;
					atmosphere_render_context.max_scene_depth = view_space_depth;//length(view_space_position);//view_space_depth;

				#if 0
					//
					vec4 atm = calculateScattering(
						u_data.atmosphere
						, atmosphere_render_context
						);

					// godot doesn't have alpha compositing (yet), so improvise
					atm.w = clamp(atm.w, 0.0001, 1.0);
					//ALBEDO = atm.xyz / atm.w;

					result += (atm.rgb/ atm.w);// * (1 - atm.w);

				#endif

				//result += max(atm.rgb, 0);

				#if 1
					float	atmosphere_opacity;

					result += computeScatteredColor(
						u_data.atmosphere
						, atmosphere_render_context
						, atmosphere_opacity
						);
				#endif

					
					
					result += max(hardware_z_depth,0)*1e-3;


					//result = source_color + max(atm.rgb, 0);
				//	result += atm.rgb;

					

					//if( view_space_depth < 200 ) {
					//	result += RED.rgb;
					//}

					//result += view_space_position;

					//result += normalize( view_space_position )*0.5+0.5;

	#endif




/**/
#endif






	//+, but "back" sphere
#if 0
	//
	float2 intersections;
	if(RayIntersectsSphere(
		camera_position_WS, ray_dir_in_WS,
		u_data.atmosphere.outer_radius,
		intersections
		))
	{
		result += RED.rgb * 1e-1;
	}
#endif


	//+
#if 0
	Segment eye_segment;
	eye_segment.origin = atmosphere_render_context.camera_position;
	eye_segment.direction = atmosphere_render_context.eye_to_pixel_ray_direction;
	eye_segment.max_distance = atmosphere_render_context.max_scene_depth;

	float2 ray_atmosphere_hits;
	const bool ray_hits_atmosphere = intersect_Segment_vs_Sphere0(
		eye_segment,
		u_data.atmosphere.outer_radius,
		ray_atmosphere_hits
	);
	if( ray_hits_atmosphere )
	{
		result += RED.rgb * 1e-1;
	}
#endif

	//+
	//if( view_space_depth < (atmosphere.outer_radius - atmosphere.inner_radius) )
	//{
	//	result += GREEN.rgb * 0.1;
	//}
	

	//+
	//result += ray_dir_in_WS.rgb;







#if bApplyGammaCorrection
	// FXAA requires colors in non-linear, gamma space
	return LinearToGamma( result );
#else
	return result;
#endif

}
