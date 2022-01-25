#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Settings.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>
#include <Rendering/Private/ShaderInterop.h>


namespace Rendering
{
namespace ShaderGlobals
{
	void SetPerFrameConstants(
		double total_elapsed_time, float delta_time_seconds,
		int mouseX, int mouseY,
		bool LBMheld, bool RMBheld,
		const RrGlobalSettings& renderer_settings,
		G_PerFrame *_perFrame
	)
	{
		_perFrame->g_Time = V4f::set(
			total_elapsed_time,
			total_elapsed_time * 2.0,
			total_elapsed_time * 3.0,
			total_elapsed_time * (1.0/20.0)
		);
		_perFrame->g_SinTime = V4f::set(
			::sin(total_elapsed_time*0.125),
			::sin(total_elapsed_time*0.25),
			::sin(total_elapsed_time*0.5),
			::sin(total_elapsed_time)
		);
		_perFrame->g_CosTime = V4f::set(
			::cos(total_elapsed_time*0.125),
			::cos(total_elapsed_time*0.25),
			::cos(total_elapsed_time*0.5),
			::cos(total_elapsed_time)
		);
		_perFrame->g_DeltaTime = V4f::set(
			delta_time_seconds,
			1.0 / delta_time_seconds,
			0,
			0
		);
		_perFrame->g_Mouse.x = mouseX;
		_perFrame->g_Mouse.y = mouseY;
		_perFrame->g_Mouse.z = LBMheld;
		_perFrame->g_Mouse.w = RMBheld;

		_perFrame->g_PBR_params = V4f::set(
			renderer_settings.PBR_metalness,
			renderer_settings.PBR_roughness,
			0,
			0
			);

		_perFrame->g_ambientColor = V4f::set(
			renderer_settings.sceneAmbientColor,
			0
			);

		_perFrame->g_HDR_params = V4f::set(
			renderer_settings.bloom.bloom_threshold,
			renderer_settings.bloom.bloom_scale,
			renderer_settings.HDR.exposure_key,
			renderer_settings.HDR.adaptation_rate
		);

		_perFrame->g_AO_params = V4f::set(
			renderer_settings.terrain.ambient_occlusion_scale,
			renderer_settings.terrain.ambient_occlusion_power,
			0,
			0
		);
	}

	// '_distanceSplitFactor' - See Proland documentation, 'Core module: 3.2.2. Continuous level of details': http://proland.inrialpes.fr
	void SetPerViewConstants(
		G_PerCamera *per_view_
		, const NwCameraView& scene_view
		, const float viewport_width, const float viewport_height
		, const RrGlobalSettings& renderer_settings
		//, const RrWorldState_VXGI& vxgiState
	)
	{
		per_view_->g_view_matrix = M44_Transpose( scene_view.view_matrix );
		per_view_->g_view_projection_matrix = M44_Transpose( scene_view.view_projection_matrix );
		per_view_->g_inverse_view_matrix = M44_Transpose( scene_view.inverse_view_matrix );
		per_view_->g_projection_matrix = M44_Transpose( scene_view.projection_matrix );
		per_view_->g_inverse_projection_matrix = M44_Transpose( scene_view.inverse_projection_matrix );
		per_view_->g_inverse_view_projection_matrix = M44_Transpose( scene_view.inverse_view_projection_matrix );
//		per_view_->g_inverseTransposeViewMatrix = scene_view.inverse_view_matrix;

//		per_view_->g_WorldSpaceCameraPos = scene_view.origin;

		double n = scene_view.near_clip;
		double f = scene_view.far_clip;
		//double x = 1 - f / n;
		//double y = f / n;
		//double z = x / f;
		//double w = y / f;
		//per_view_->g_ZBufferParams = V4f::set( x, y, z, w );

		per_view_->g_DepthClipPlanes = V4f::set( n, f, 1.0f/n, 1.0f/f );

		per_view_->g_viewport_params = V4f::set(
			tan( 0.5 * scene_view.half_FoV_Y * scene_view.aspect_ratio )
			, tan( 0.5 * scene_view.half_FoV_Y )
			, scene_view.aspect_ratio
			, 1
			);

		per_view_->g_camera_origin_WS	= V4f::set( scene_view.eye_pos, 0 );
		per_view_->g_camera_right_WS	= V4f::set( scene_view.right_dir, 0 );
		per_view_->g_camera_fwd_WS		= V4f::set( scene_view.look_dir, 0 );
		per_view_->g_camera_up_WS		= V4f::set( scene_view.getUpDir(), 0 );

		////
		//// const RrTerrainSettings& terrain_settings
		//const GameplayConstants& gameplay = GameplayConstants::gs_shared;
		//const V3f sphere_hole_pos_in_view_space = M44_TransformPoint( scene_view.view_matrix, gameplay.sphere_hole_pos_in_world_space );

		//per_view_->g_sphere_hole = V4f::set(
		//	sphere_hole_pos_in_view_space.x,
		//	sphere_hole_pos_in_view_space.y,
		//	sphere_hole_pos_in_view_space.z,
		//	squaref(gameplay.sphere_hole_radius)
		//	);

		//double H = scene_view.projection_matrix[0][0];
		//double V = scene_view.projection_matrix[2][1];
		//double A = scene_view.projection_matrix[1][2];
		//double B = scene_view.projection_matrix[3][2];
		//per_view_->g_ProjParams = V4f::set( H, V, A, B );

		// x = 1/H
		// y = 1/V
		// z = 1/B
		// w = -A/B
//		per_view_->g_ProjParams2 = V4f::set( 1/H, 1/V, 1/B, -A/B );

		per_view_->g_framebuffer_dimensions = V4f::set(
			viewport_width,
			viewport_height,
			1.0 / viewport_width,	//CalculateNumTilesX( viewportWidth );
			1.0 / viewport_height	//CalculateNumTilesY( viewportHeight );
		);

		//per_view_->g_terrainCamera = V4f::set( scene_view.terrainCamera, terrain_settings.distanceSplitFactor );

		//// See Proland documentation, 'Core module: 3.2.2. Continuous level of details': http://proland.inrialpes.fr
		//const float k = terrain_settings.distanceSplitFactor;
		//const float k_plus_1 = k + 1.0f;
		//const float k_minus_1_inverse = 1.0f / (k - 1.0f);
		//per_view_->g_distanceFactors = V4f::set( k, k_plus_1, k_minus_1_inverse, terrain_settings.debug_morph_factor );
	}

	void copyPerObjectConstants(
		G_PerObject *cb_per_object_,
		const M44f& _worldMatrix,
		const M44f& _view_matrix,
		const M44f& _view_projection_matrix,
		const UINT vxgiCascadeIndex
	)
	{
		const M44f worldViewMatrix = M44_Multiply(_worldMatrix, _view_matrix);
		const M44f worldViewProjectionMatrix = M44_Multiply(_worldMatrix, _view_projection_matrix);

		cb_per_object_->g_world_matrix = M44_Transpose( _worldMatrix );
		cb_per_object_->g_world_view_matrix = M44_Transpose( worldViewMatrix );
		cb_per_object_->g_world_view_projection_matrix = M44_Transpose( worldViewProjectionMatrix );
		//cb_per_object_->g_object_uint4_params = UInt4::replicate( vxgiCascadeIndex );
	}

	void SetConstantsForSceneLighting(
		G_Lighting *lightingConstants_

		, const NwCameraView& scene_view

		, const RrDirectionalLight& global_light

#if nwRENDERER_ENABLE_SHADOWS
		, const RrWorldState_StaticShadowMap& shadowMapState

		//
		, const M44f&	cameraToLightViewSpace
		, const V3f		cascadeScales[MAX_SHADOW_CASCADES]
		, const V3f		cascadeOffsets[MAX_SHADOW_CASCADES]
		, const M44f	cascadeViewProjectionZBiasMatrices[MAX_SHADOW_CASCADES]
		// converts pixel positions from view space into shadow texture space; these are used when rendering the shadowed scene
		, const M44f	cameraToCascadeTextureSpace[MAX_SHADOW_CASCADES]
#endif

		//
		//, const RrWorldState_VXGI& vxgiState

		, const RrGlobalSettings& renderer_settings
	)
	{
		{
			mxASSERT(global_light.direction.isNormalized());

			const V3f lightVectorWorldSpace = -global_light.direction;
			const V3f lightVectorViewSpace = V3_Normalized( M44_TransformNormal( scene_view.view_matrix, lightVectorWorldSpace ) );

			lightingConstants_->g_directional_light.light_vector_VS = V4f::set( lightVectorViewSpace, 0.0f );
			lightingConstants_->g_directional_light.light_vector_WS = V4f::set( lightVectorWorldSpace, 0.0f );
			lightingConstants_->g_directional_light.color = V4f::set( global_light.color, 1.0f );

#if nwRENDERER_ENABLE_SHADOWS
			//
			lightingConstants_->g_directional_light.camera_to_light_view_space = M44_Transpose( cameraToLightViewSpace );

			for( int i = 0; i < MAX_SHADOW_CASCADES; i++ )
			{
				lightingConstants_->g_directional_light.cascade_scale[i] = V4f::set( cascadeScales[i], 0 );
				lightingConstants_->g_directional_light.cascade_offset[i] = V4f::set( cascadeOffsets[i], 0 );
				lightingConstants_->g_directional_light.view_to_shadow_texture_space[i] = M44_Transpose( cameraToCascadeTextureSpace[i] );
			}
#endif
		}

		//
#if 0
		{
			const VXGI_Cascade& vxgiCascade0 = vxgiState.cascades[0];

			M44f	u_view_to_grid_space = scene_view.inverse_view_matrix;	// transform from view into world space
			u_view_to_grid_space = M44_Multiply( u_view_to_grid_space, M44_buildTranslationMatrix( V3f::fromXYZ( -vxgiCascade0.minCorner() ) ) );
			u_view_to_grid_space = M44_Multiply( u_view_to_grid_space, M44_uniformScaling( 1.0f / vxgiCascade0.sideLength() ) );

			lightingConstants_->g_VXGI.view_to_volume_texture_space[0] = M44_Transpose( u_view_to_grid_space );
		}
#endif
	}

	void setSkinningMatrices(
		G_SkinningData *skinning_data_
		, const M44f* joint_matrices
		, const UINT num_joint_matrices
	)
	{
		memcpy( skinning_data_->g_bone_matrices
			, joint_matrices
			, sizeof(joint_matrices[0]) * num_joint_matrices );

#if 0//MX_DEVELOPER
		// zero-out the rest
		memset( skinning_data_->g_bone_matrices + num_joint_matrices
			, 0
			, sizeof(joint_matrices[0]) * ( mxCOUNT_OF(skinning_data_->g_bone_matrices) - num_joint_matrices ) );
#endif
	}

	void packAtmosphereRenderingParameters(
		UeberPostProcessingShaderData &ueber_post_processing_shader_data_
		, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
		)
	{
		mxUNDONE;
#if 0
		{
			const NwAtmosphereParameters& src_atmosphere_parameters = atmosphere_rendering_parameters.atmosphere;
			AtmosphereParameters &	dst_atmosphere_parameters = ueber_post_processing_shader_data_.atmosphere;

			dst_atmosphere_parameters.inner_radius = src_atmosphere_parameters.inner_radius;
			dst_atmosphere_parameters.outer_radius = src_atmosphere_parameters.outer_radius;

			dst_atmosphere_parameters.beta_Rayleigh = src_atmosphere_parameters.beta_Rayleigh;
			dst_atmosphere_parameters.beta_Mie = src_atmosphere_parameters.beta_Mie;

			dst_atmosphere_parameters.height_Rayleigh = src_atmosphere_parameters.height_Rayleigh;
			dst_atmosphere_parameters.height_Mie = src_atmosphere_parameters.height_Mie;

			dst_atmosphere_parameters.g = src_atmosphere_parameters.g;

			dst_atmosphere_parameters.sun_intensity = src_atmosphere_parameters.sun_intensity;
			dst_atmosphere_parameters.density_multiplier = src_atmosphere_parameters.density_multiplier;
			dst_atmosphere_parameters.beta_ambient = src_atmosphere_parameters.beta_ambient;
		}

		//
		ueber_post_processing_shader_data_.viewer.relative_eye_position = atmosphere_rendering_parameters.relative_eye_position;
#endif
	}

ERet SetupPerObjectMatrices(
	G_PerObject	**cb_per_object_
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	)
{
	G_PerObject	*	cb_per_object;
	mxDO(render_context._command_buffer.Allocate( &cb_per_object ));

	ShaderGlobals::copyPerObjectConstants(
		cb_per_object
		, local_to_world_matrix
		, scene_view.view_matrix
		, scene_view.view_projection_matrix
		, 0	// const UINT vxgiCascadeIndex
		);

	*cb_per_object_ = cb_per_object;

	return ALL_OK;
}


ERet SetupSkinningMatrices(
						   G_SkinningData **cb_skinning_data_
						   , const TSpan< const M44f >& joint_matrices
						   , NGpu::NwRenderContext & render_context
						   )
{
	mxASSERT(!joint_matrices.IsEmpty());

	G_SkinningData	*	cb_skinning_data;
	mxDO(render_context._command_buffer.Allocate( &cb_skinning_data ));

	ShaderGlobals::setSkinningMatrices(
		cb_skinning_data
		, joint_matrices.raw()
		, joint_matrices.num()
		);

	*cb_skinning_data_ = cb_skinning_data;

	return ALL_OK;
}

}//namespace ShaderGlobals
}//namespace Rendering
