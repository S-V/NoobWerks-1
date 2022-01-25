/*
Deferred full-screen additive directional light.

%%
pass DeferredGlobalLights
{
	vertex = FullScreenTriangle_VS
	pixel = man_PS

	state = deferred_fullscreen_light
}

samplers
{
	s_point = PointSampler
	s_bilinear = BilinearSampler
	s_trilinear = TrilinearSampler
}

%%
*/

/*
//feature ENABLE_SHADOWS {}
//feature ENABLE_AMBIENT_OCCLUSION {}
// DDGI
//feature ENABLE_INDIRECT_LIGHTING {}


feature DBG_VISUALIZE_CASCADES {}
feature ENABLE_VXGI_AMBIENT_OCCLUSION {}

// off, low (HW bilinear PCF), medium (edge tap smoothing), high
feature ShadowQuality
{
	bits = 2
	default = 1	// low
}
*/

#define ENABLE_EARTH_SKYLIGHT (0)
#define ENABLE_INDIRECT_LIGHTING (1)
#define ENABLE_SHADOWS (1)



#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include "_PS.h"
#include <Common/transform.hlsli>
#include "_screen_shader.h"
#include <GBuffer/nw_gbuffer_read.h>
#include "_lighting.h"


#include "_basis.h"
#include <Lighting/PBS.hlsli>

#include <Sky/clearsky.hlsl>
#include <Sky/atmosphere_analytic.hlsl>


#if ENABLE_INDIRECT_LIGHTING
	#include <VoxelGI/ddgi_interop.h>
	#include <VoxelGI/ddgi_common.hlsli>
#endif

#if ENABLE_INDIRECT_LIGHTING || ENABLE_SHADOWS
	#include <VoxelGI/vxgi_interop.h>
	#include <VoxelGI/vxgi_common.hlsli>
	#include <VoxelGI/voxel_raymarching.hlsli>
	#include <VoxelGI/voxel_cone_tracing.hlsli>
	#include <VoxelGI/voxel_shadowing.hlsli>

	#include <VoxelGI/bvh_interop.h>
	#include <VoxelGI/sdf_bvh_traversal.hlsli>
	
	#include <VoxelGI/scene_shadows.hlsl>
#endif


// voxel grid
Texture3D< float4 >		t_radiance_opacity_volume;

//
Texture3D< float >		t_sdf_atlas3d;

// irradiance probes
Texture2D< float3 >		t_probe_irradiance;
Texture2D< float2 >		t_probe_distance;


//
SamplerState	s_point;
SamplerState	s_bilinear;
SamplerState	s_trilinear;


float3 man_PS(in VS_ScreenOutput ps_inputs) : SV_Target
{
	const float hardware_z_depth = DepthTexture.SampleLevel( s_point, ps_inputs.texCoord, 0 );
	const float2 screen_space_position = TexCoordsToClipPos( ps_inputs.texCoord );
	const float3 surface_position_VS = restoreViewSpacePosition( screen_space_position, hardware_z_depth );

	const GSurfaceParameters surface = Sample_Surface_Data_From_GBuffer(
		s_point,
		ps_inputs.texCoord
		);

	//
	const float3 surface_position_WS = Pos_View_To_World(
		float4(surface_position_VS, 1)
	).xyz;

	mxOPTIMIZE("store world-space normals in G-Buffer to avoid matrix-vec mul?")
	const float3 surface_normal_WS = normalize(
		mul( g_inverse_view_matrix, float4( surface.normal, 0 ) ).xyz
	);

	//
	float3 resulting_color = (float3) 0;

	// Compute direct lighting
	{
		PBR_Surface	pbr_surface;
		//
		pbr_surface.position = surface_position_WS;
		pbr_surface.normal = surface_normal_WS;
		//
		pbr_surface.diffuse_albedo = surface.albedo;
		pbr_surface.specular_albedo = 0.04f;//float3(1,1,1);// surface.specular_color;
		//
		pbr_surface.roughness = 1;// surface.roughness;
		pbr_surface.metalness = 0;//surface.metalness;
	
	#if ENABLE_SHADOWS
		#if 0
		if(0)
		{
			// soft shadows - working with artifacts at grid boundaries
			pbr_surface.accessibility = SdfBvh_GetSoftSunLightVisibilityFromPoint(
				surface_position_WS
				, surface_normal_WS
				
				, g_directional_light.light_vector_WS.xyz // direction_to_light_source
				, t_sdf_atlas3d
				, s_trilinear

				, 8	// shadow hardness; 4 - very soft, 128 - hard-edged
			);
			/*				
			pbr_surface.accessibility = SDF_GetSoftSunLightVisibilityFromPoint_UVW(
				surface_position_WS
				, surface_normal_WS
				
				, g_directional_light.light_vector_WS.xyz // direction_to_light_source
				, t_sdf_atlas3d
				, s_trilinear
	
				, 32	// shadow hardness; 4 - very soft, 128 - hard-edged
			);
			*/
		}
		else
		#endif
		{
			// hard shadows
			if(1)
			{
				const bool hit_anything = IsInShadow(
					surface_position_WS
					, surface_normal_WS
					
					, g_directional_light.light_vector_WS.xyz

					, t_sdf_atlas3d
					, s_trilinear
				);
				
				pbr_surface.accessibility = hit_anything ? 0 : 1;
			}
		#if 0
			else
			{
				pbr_surface.accessibility = SdfBvh_GetSunLightVisibilityFromPoint(
					surface_position_WS
					, surface_normal_WS
					
					, g_directional_light.light_vector_WS.xyz // direction_to_light_source
					, t_sdf_atlas3d
					, s_trilinear
		
					//, 4
				);
			}
		#endif
		}

	#else
		pbr_surface.surface_occlusion = 1;	// unoccluded
	#endif

		const float3 to_eye_direction = normalize(
			g_camera_origin_WS.xyz - surface_position_WS
		);

		//
#if ENABLE_EARTH_SKYLIGHT
	#if 1
			const float3 sky_color = GetSkyColorOnEarthGround(
				g_directional_light.light_vector_WS.xyz
				, surface_normal_WS	// eye_to_pixel_ray_direction_WS
				);
			resulting_color = PBR_ComputeDirectionalLighting(
				sky_color
				, g_directional_light.light_vector_WS.xyz
				, to_eye_direction
				, pbr_surface
				);
	#else
			const float3 sky_color = CIEClearSky(
				surface_normal_WS,// eye_to_pixel_ray_direction_WS
				g_directional_light.light_vector_WS.xyz
			) * 1e-1;
			resulting_color = PBR_ComputeDirectionalLighting(
				sky_color
				, g_directional_light.light_vector_WS.xyz
				, to_eye_direction
				, pbr_surface
				);
	#endif
#else
		resulting_color = PBR_ComputeDirectionalLighting(
			g_directional_light.color.rgb
			, g_directional_light.light_vector_WS.xyz
			, to_eye_direction
			, pbr_surface
		);
#endif
	}

	//
	const float view_space_depth = surface_position_VS.y;
	const float	surface_accessibility = surface.accessibility;

//AO
//resulting_color *= surface_accessibility;
//resulting_color += surface_accessibility*1e-4f;

#if ENABLE_INDIRECT_LIGHTING

	const float3 surface_bias = DDGI_GetSurfaceBias(
		surface_normal_WS
		, g_camera_fwd_WS.xyz
		, g_ddgi
	);
	
	//
	DDGI_VolumeResources	ddgi_resources;
	ddgi_resources.probe_irradianceSRV = t_probe_irradiance;
	ddgi_resources.probeDistanceSRV = t_probe_distance;
    ddgi_resources.bilinearSampler = s_bilinear;

	//
	const float3 irradiance = DDGI_SampleVolumeIrradiance(
		surface_position_WS // in float3 world_position
		, surface_normal_WS // in float3 direction
		, surface_bias // in float3 surface_bias
		, g_ddgi
		, ddgi_resources
	);
	resulting_color += irradiance;

#endif // ENABLE_INDIRECT_LIGHTING

	// Ambient
	resulting_color += (float3)0.001;

	return resulting_color;
}
