// App -> Shader communication, setting shader parameters (uniforms)
#pragma once

#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>

#include <Rendering/ForwardDecls.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/Shaders/Shared/nw_shared_globals.h>


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
	);

	void SetPerViewConstants(
		G_PerCamera *per_view_
		, const NwCameraView& scene_view
		, const float viewport_width, const float viewport_height
		, const RrGlobalSettings& renderer_settings
	);

	void copyPerObjectConstants(
		G_PerObject *cb_per_object_,
		const M44f& _worldMatrix,
		const M44f& _view_matrix,
		const M44f& _view_projection_matrix,
		const UINT vxgiCascadeIndex
	);

	void SetConstantsForSceneLighting(
		G_Lighting *lightingConstants_

		//
		, const NwCameraView& scene_view

		, const RrDirectionalLight& global_light

#if nwRENDERER_ENABLE_SHADOWS
		, const RrWorldState_StaticShadowMap& shadowMapState

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
	);

	void setSkinningMatrices(
		G_SkinningData *skinning_data_
		, const M44f* joint_matrices
		, const UINT num_joint_matrices
	);

	void packAtmosphereRenderingParameters(
		UeberPostProcessingShaderData &ueber_post_processing_shader_data_
		, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
		);


	/// allocates constant buffers inside the command buffer
	ERet SetupPerObjectMatrices(
		// out:
		G_PerObject	**cb_per_object_
		// in:
		, const M44f& local_to_world_matrix
		, const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		);

	ERet SetupSkinningMatrices(
		// out:
		G_SkinningData **cb_skinning_data_
		// in:
		, const TSpan< const M44f >& joint_matrices
		, NGpu::NwRenderContext & render_context
		);

}//namespace ShaderGlobals
}//namespace Rendering
