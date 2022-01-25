// Tiled Deferred Renderer
#include <Base/Base.h>
#pragma hdrstop

// for std::sort()
#include <algorithm>

//#include <XNAMath/xnamath.h>

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Math/BoundingVolumes/ViewFrustum.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Util/Tweakable.h>

#include <Engine/WindowsDriver.h>

#include <Graphics/Public/graphics_utilities.h>

#include <Rendering/Private/Shaders/Shared/nw_shared_globals.h>

#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Scene/Light.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Private/ShaderInterop.h>
//#include <Renderer/Shadows.h>
//#include <Renderer/scene/VisibilityCulling.h>
#include <Rendering/Private/Modules/PostProcessing/PostProcessing.h>
#include <Rendering/Private/Pipelines/TiledDeferred/TiledDeferred.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Public/Settings.h>
#include <Rendering/Private/SceneRendering.h>

#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Rendering/Private/Shaders/HLSL/VoxelGI/bvh_interop.h>	// G_SDF_BVH_CBuffer_Index

#include <Core/Serialization/Text/TxTSerializers.h>

namespace Rendering {

namespace
{
	const int _max_deferred_lights = 2048;
}


DeferredRenderer::DeferredRenderer()
	//: _depth_reducer_CS(MemoryHeaps::global())
{
}

DeferredRenderer::~DeferredRenderer()
{

}

ERet DeferredRenderer::Initialize( U32 _screenWidth, U32 _screenHeight )
{
	// Load the rendering pipeline as a compiled blob.
	mxDO(Resources::Load(m_rendererData._ptr, MakeAssetID("tiled_deferred")));

	// Modify renderer_settings before creating resources.

	// Initialize G-Buffer targets.
	{
		mxDO(GetByHash(m_colorRT0._ptr, mxHASH_STR("GBufferTexture0"), ClumpList::g_loadedClumps));
		mxDO(GetByHash(m_colorRT1._ptr, mxHASH_STR("GBufferTexture1"), ClumpList::g_loadedClumps));
		//mxDO(GetByHash(m_colorRT2._ptr, mxHASH_STR("GBufferTexture2"), ClumpList::g_loadedClumps));
		mxDO(GetByHash(m_depthRT._ptr, mxHASH_STR("MainDepthStencil"), ClumpList::g_loadedClumps));
	}

	// Initialize average luminance calculation targets.
	{
		mxDO(GetByHash(m_litTexture._ptr, mxHASH_STR("LightingTarget"), ClumpList::g_loadedClumps));

		mxDO(GetByHash(m_adaptedLuminance[0]._ptr, mxHASH_STR("AdaptedLuminance0"), ClumpList::g_loadedClumps));
		mxDO(GetByHash(m_adaptedLuminance[1]._ptr, mxHASH_STR("AdaptedLuminance1"), ClumpList::g_loadedClumps));
		m_iCurrentLuminanceRT = 0;

		{
			TbColorTarget* adaptedLuminance0 = m_adaptedLuminance[0];
			adaptedLuminance0->sizeX.SetAbsoluteSize( HDR_Renderer::LUMINANCE_RT_SIZE );
			adaptedLuminance0->sizeY.SetAbsoluteSize( HDR_Renderer::LUMINANCE_RT_SIZE );
			adaptedLuminance0->numMips = TLog2< HDR_Renderer::LUMINANCE_RT_SIZE >::value + 1;
			adaptedLuminance0->format = HDR_Renderer::LUMINANCE_RT_FORMAT;
		}
		{
			TbColorTarget* adaptedLuminance1 = m_adaptedLuminance[1];
			adaptedLuminance1->sizeX.SetAbsoluteSize( HDR_Renderer::LUMINANCE_RT_SIZE );
			adaptedLuminance1->sizeY.SetAbsoluteSize( HDR_Renderer::LUMINANCE_RT_SIZE );
			adaptedLuminance1->numMips = TLog2< HDR_Renderer::LUMINANCE_RT_SIZE >::value + 1;
			adaptedLuminance1->format = HDR_Renderer::LUMINANCE_RT_FORMAT;
		}
	}


	m_renderPath = m_rendererData->FindSingleInstance< RenderPath >();

	{
		const ScenePassHandle hBuildShadowMapPass = m_renderPath->findScenePass( ScenePasses::RenderDepthOnly );
		m_renderPath->setNumSubpasses( hBuildShadowMapPass, MAX_SHADOW_CASCADES );

		const ScenePassHandle hVXGI_VoxelizePass = m_renderPath->findScenePass( ScenePasses::VXGI_Voxelize );
		m_renderPath->setNumSubpasses( hVXGI_VoxelizePass, MAX_GI_CASCADES );

		m_renderPath->recalculatePassDrawingOrder();
	}

	//mxDO(m_shadowRenderer.preInit_ModifySettings( *m_renderPath, *m_rendererData ));

	//mxDO(m_shadowRenderer.Initialize( *m_renderPath, m_rendererData ));
	//mxDO(_shadow_map_renderer.Initialize());

	{
		NwBufferDescription	buffer_description(_InitDefault);
		buffer_description.type = Buffer_Data;
		buffer_description.size = _max_deferred_lights * sizeof(DeferredLight);
		buffer_description.stride = sizeof(DeferredLight);
		// light buffer is read in compute technique
		buffer_description.flags.raw_value = NwBufferFlags::Sample | NwBufferFlags::Dynamic;
		_deferred_lights_buffer_handle = NGpu::CreateBuffer(
			buffer_description
			, nil
			IF_DEVELOPER , "LightBuffer"
			);
	}

#if DEBUG_EMIT_POINTS
	{
		NwBufferDescription	desc(_InitDefault);
		//desc.type = Buffer_Data;
		//desc.size = sizeof(DebugLine)*MAX_DEBUG_LINES;
		//desc.stride = sizeof(DebugLine);
		//desc.randomWrites = true;
		//desc.appendBuffer = true;
		//m_debug_lines = NGpu::CreateBuffer(desc);

		desc.type = Buffer_Data;
		desc.size = sizeof(DebugPoint)*MAX_DEBUG_POINTS;
		desc.stride = sizeof(DebugPoint);
		desc.randomWrites = true;
		desc.appendBuffer = true;
		u_debugPoints = NGpu::CreateBuffer(desc);
	}
#endif


	//_depth_reducer_CS.createRenderTargets( _screenWidth, _screenHeight );
	//_depth_reducer_CS.createReductionStagingTextures();

	///m_cullingSystem = ACullingSystem::StaticCreate(MemoryHeaps::global());

	return ALL_OK;
}

void DeferredRenderer::Shutdown()
{
	//ACullingSystem::StaticDestroy(m_cullingSystem);
	//m_cullingSystem = nil;

	//_depth_reducer_CS.Shutdown();

	NGpu::DeleteBuffer(_deferred_lights_buffer_handle);
}

ERet SubmitTransientPointLight(
							   const V3f position
							   , const float radius
							   , const V3f color
							   )
{
	TransientPointLight	tpl;
	tpl.position = position;
	tpl.radius = radius;
	tpl.color = color;
	return Globals::g_data->_deferred_renderer.transient_point_lights.add(tpl);
}


//
// Causes a breakpoint exception in debug builds, causes fatal error in release builds.
//
#if MX_DEBUG || MX_DEVELOPER
	#define mxCHECK( X )\
		mxMACRO_BEGIN\
			const ERet __result = (X);\
			static int s_assertBehavior = AB_Break;\
			if( mxFAILED(__result) ) {\
				if( s_assertBehavior != AB_Ignore ) {\
					s_assertBehavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #X, "%d", __result );\
					if( s_assertBehavior == AB_Break ) ptBREAK;\
					if( s_assertBehavior != AB_Ignore ) {\
						return __result;\
					}\
				}\
			} else {\
				s_assertBehavior = AB_Break;\
			}\
		mxMACRO_END
#else
	#define mxCHECK( X )	mxDO( X )
#endif





/// Applies deferred directional global_light.
ERet SubmitDirectionalLight(
							const RrDirectionalLight& global_light
							, const NwCameraView& scene_view
							, const RrGlobalSettings& renderer_settings
							, NGpu::NwRenderContext & render_context

							, const VXGI::VoxelGrids* voxel_grids

							, const VXGI::IrradianceField* irradiance_field
							)
{
	mxASSERT(V3_IsNormalized(global_light.direction));

	RenderSystemData& data = *Globals::g_data;
	NwClump& clump = *data._deferred_renderer.m_rendererData;

	//
	const U32 viewId = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::DeferredGlobalLights )
		;

	NGpu::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		NGpu::buildSortKey(viewId, 0)
			nwDBG_CMD_SRCFILE_STRING
		);


	//
	{
		G_Lighting *	lighting_constants;
		mxDO(NGpu::updateGlobalBuffer(
			data._global_uniforms.lighting.handle
			, &lighting_constants
			));

		ShaderGlobals::SetConstantsForSceneLighting(
			lighting_constants

			, scene_view

			, global_light

#if nwRENDERER_ENABLE_SHADOWS
			, staticShadowMapState

			, cameraToLightViewSpace
			, cascadeScales
			, cascadeOffsets
			, cascadeViewProjectionZBiasMatrices
			, cameraToCascadeTextureSpace
#endif

			//, vxgiState

			, renderer_settings
			);
	}


	{
		IF_DEVELOPER GFX_SCOPED_MARKER( render_context._command_buffer, RGBAi::LIGHTYELLOW );

		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("deferred_directional_light"), &clump));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		const bool enableShadows = 0
			| (renderer_settings.shadows.flags & ShadowEnableFlags::enable_static_shadows)
			| (renderer_settings.shadows.flags & ShadowEnableFlags::enable_dynamic_shadows)
			;

		NwShaderFeatureBitMask program_index = 0
			//|pass0.ComposeFeatureMask( mxHASH_STR("bCastShadows"), 0/*renderer_settings.enableShadows*/ )
			//|pass0.ComposeFeatureMask( mxHASH_STR("bUseSoftShadows"), renderer_settings.shadows.enable_soft_shadows )

			//|technique->composeFeatureMask( mxHASH_STR("ShadowQuality"), enableShadows ? 1 : 0 )	// low quality or none
			//			|technique->composeFeatureMask( mxHASH_STR("ENABLE_SHADOWS"), enableShadows )	// low quality or none

			//			|technique->composeFeatureMask( mxHASH_STR("FILTER_ACROSS_CASCADES"), renderer_settings.shadows.blend_between_cascades )
			//			|technique->composeFeatureMask( mxHASH_STR("DBG_VISUALIZE_CASCADES"), renderer_settings.shadows.dbg_visualize_cascades )
			//			|technique->composeFeatureMask( mxHASH_STR("ENABLE_VXGI_AMBIENT_OCCLUSION"), renderer_settings.gi.enable_GI && renderer_settings.gi.enable_AO )
			;

		//		mxCHECK(technique->SetInput( mxHASH_STR("t_shadowDepthMap"), NGpu::AsInput(render_system._shadow_map_renderer._shadow_depth_map_handle) ));

		if( renderer_settings.shadows.enable_soft_shadows ) {
			//			FxSlow_SetSampler(technique, "s_shadowMapSampler", BuiltIns::g_samplers[BuiltIns::ShadowMapSampler]);
		} else {
			//			FxSlow_SetSampler(technique, "s_shadowMapPCFBilinearSampler", BuiltIns::g_samplers[BuiltIns::ShadowMapSamplerPCF]);
		}

		mxCHECK(technique->SetInput(
			nwNAMEHASH("DepthTexture"),
			NGpu::AsInput(data._deferred_renderer.m_depthRT->handle)
			));
		mxCHECK(technique->SetInput(
			nwNAMEHASH("GBufferTexture0"),
			NGpu::AsInput(data._deferred_renderer.m_colorRT0->handle)
			));
		mxCHECK(technique->SetInput(
			nwNAMEHASH("GBufferTexture1"),
			NGpu::AsInput(data._deferred_renderer.m_colorRT1->handle)
			));
		//		mxCHECK(technique->SetInput( mxHASH_STR("GBufferTexture2"),	NGpu::AsInput(m_colorRT2->handle) ));

		//QQQ
		//		render_system._gpu_voxels.bindVoxelCascadeTexturesToShader( technique );

		// lightprobes disabled
		//		render_system._lightProbeGrid.bindIrradianceVolumeTexturesToShader( technique );


		//
		
		if(voxel_grids && irradiance_field)
		{
			//program_index |= technique->composeFeatureMask(
			//	mxHASH_STR("ENABLE_INDIRECT_LIGHTING")
			//	);

			const VXGI::VoxelBVH& voxel_bvh = *voxel_grids->voxel_BVH;
			//
			NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );
			cmd_writer.SetCBuffer(
				G_SDF_BVH_CBuffer_Index
				, voxel_bvh._bvh_cb_handle
				);


			//
			const VXGI::BrickMap& voxel_brickmap = *voxel_grids->brickmap;

#if 0
			//
			(technique->SetInput(
				mxHASH_STR("t_radiance_opacity_volume"),
				GL::AsInput(voxel_grids->_radiance_opacity_texture[0])
				));
#endif

			(technique->SetInput(
				nwNAMEHASH("t_sdf_atlas3d"),
				NGpu::AsInput(voxel_brickmap.h_sdf_brick_atlas_tex3d)
				));

			//
			const VXGI::DDGIResources& ddgi_resources = irradiance_field->GetDstIrradianceProbes();
			(technique->SetInput(
				nwNAMEHASH("t_probe_irradiance"),
				NGpu::AsInput(ddgi_resources.irradiance_probes_tex2d)
				));
			(technique->SetInput(
				nwNAMEHASH("t_probe_distance"),
				NGpu::AsInput(ddgi_resources.visibility_probes_tex2d)
				));
		}


		//
		const HProgram selected_program_handle = pass0.program_handles[ program_index ]; 

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, selected_program_handle
			));
	}

	return ALL_OK;
}

static ERet Apply_Deferred_Spot_Light(
	NGpu::NwRenderContext & context, const NGpu::LayerID viewId,
	const NwCameraView& sceneView, const RrLocalLight& light,
	const DeferredRenderer& renderer
	)
{
	NwRenderState * renderState = nil;
	mxDO(Resources::Load(renderState, MakeAssetID("deferred_fullscreen_light"), renderer.m_rendererData));
UNDONE;
	//NwShaderEffect* shader;
	//mxDO(Resources::Load(shader, MakeAssetID("deferred_spot_light"), renderer.m_rendererData));

#if 0
	const V3f lightPositionWS = light.GetOrigin();
	const V3f lightPositionVS = M44_TransformPoint( sceneView.view_matrix, lightPositionWS );
	const V3f lightDirectionWS = { 0, 0, -1 }; // light.GetDirection();
	const V3f lightDirectionVS = M44_TransformNormal( sceneView.view_matrix, lightDirectionWS );
	//const V3f light_vector_WS = V3_Negate( light.direction );
	//const V3f light_vector_VS = V3_Normalized( M44_TransformNormal( sceneView.view_matrix, light_vector_WS ) );
#else
	const V3f lightPositionWS = sceneView.eye_pos;
	const V3f lightPositionVS = V3_Zero();
	const V3f lightDirectionWS = sceneView.look_dir;
	const V3f lightDirectionVS = V3_FORWARD;
		//M44_TransformNormal( sceneView.view_matrix, lightDirectionWS );//V3_FORWARD;
	//const V3f lightDirectionVS2 = V3_Normalized( M44_TransformNormal( sceneView.view_matrix, lightDirectionWS ) );//V3_FORWARD;
#endif


#if 1
	// world-to-light view transform
	//const M44f	lightViewMatrix = M44_LookTo(
	//	light.position,
	//	lightDirectionWS,
	//	V3_UP
	//);
    V3f right = V3_Cross( lightDirectionWS, V3_UP );
	float len = V3_Length( right );
	if( len < 1e-3f ) {
		right = V3_RIGHT;	// light direction is too close to the UP vector
	} else {
		right /= len;
	}

	const V3f up = V3_Cross( right, lightDirectionWS );

	//const M44f	lightViewMatrix = sceneView.view_matrix;
	//const M44f	lightViewMatrix = M44_LookTo(
	//	lightPositionWS,
	//	lightDirectionWS,
	//	up
	//);
	const M44f	lightViewMatrix = M44_BuildView(
		right,
		lightDirectionWS,
		up,
		lightPositionWS
	);

	const M44f	lightProjectionMatrix = light.BuildSpotLightProjectionMatrix();
	const M44f	lightViewProjectionMatrix = lightViewMatrix * lightProjectionMatrix;

	const M44f	biasedLightViewProjectionMatrix = lightViewProjectionMatrix * M44_NDC_to_TexCoords_with_Bias();

	LocalLight	appToShaderData;

	appToShaderData.origin = V4f::set(lightPositionVS, 1);
	appToShaderData.radius_invRadius = V4f::set(light.radius, 1.0f/light.radius, 0, 0);

	appToShaderData.diffuseColor = V4f::set(light.color * light.brightness, 1);
	appToShaderData.specularColor = V4f::Zero();

	// spot light data
	appToShaderData.spotDirection = V4f::set( lightDirectionVS, 0 );
	appToShaderData.spotLightAngles = V4f::set( light.m_spotAngles.x, light.m_spotAngles.y, light.GetProjectorIntensity(), light.m_shadowDepthBias );

	appToShaderData.eyeToLightProjection = M44_Transpose( sceneView.inverse_view_matrix * biasedLightViewProjectionMatrix );

#else

	// world-to-light view transform
	const XMMATRIX	lightViewMatrix = XMMatrixLookToLH(
		XMLOAD(light.position, 1.0f),
		XMLOAD(lightDirectionWS, 0),
		//XMLOAD(V3_UP, 0)
		XMLOAD(V3_Set(0,1,0), 0)
	);

	const XMMATRIX	lightProjectionMatrix = CalcSpotLightProjectionMatrix2(&light);
	const XMMATRIX	lightViewProjectionMatrix = lightViewMatrix * lightProjectionMatrix;

	LocalLight	appToShaderData;

	appToShaderData.origin = Vector4_Set(lightPositionVS, 0);
	appToShaderData.radius_invRadius = Vector4_Set(light.radius, 1.0f/light.radius, 0, 0);

	appToShaderData.diffuseColor = Vector4_Set(light.color * light.brightness, 1);
	appToShaderData.specularColor = V4_Zero;

	// spot light data
	appToShaderData.spotDirection = Vector4_Set( lightDirectionVS, 0 );
	appToShaderData.spotLightAngles = Vector4_Set( light.m_spotAngles.x, light.m_spotAngles.y, 0, 0 );

	//appToShaderData.eyeToLightProjection
	appToShaderData.eyeToLightProjection = (M44f&) XMMatrixTranspose( (XMMATRIX&)sceneView.inverse_view_matrix * lightViewProjectionMatrix );
#endif

	//NwTexture *	texture;
	//mxDO(Resources::Load(texture, MakeAssetID("missing_texture"), renderer.m_rendererData));
	//mxDO(FxSlow_SetInput(shader, "t_projector", texture->m_resource));

	//mxDO(FxSlow_SetInput(shader, "DepthTexture", NGpu::AsInput(renderer.m_depthRT->handle)));
	//mxDO(FxSlow_SetInput(shader, "GBufferTexture0", NGpu::AsInput(renderer.m_colorRT0->handle)));
	//mxDO(FxSlow_SetInput(shader, "GBufferTexture1", NGpu::AsInput(renderer.m_colorRT1->handle)));

	//if( global_light.DoesCastShadows() ) {
	//	mxDO(FxSlow_SetInput(shader, "shadowDepthMap", NGpu::AsInput(renderer.m_shadowRenderer.m_shadowMap->handle)));
	//}
UNDONE;
	//NGpu::SortKey sortKey = 0;
	//NGpu::UpdateBuffer( render_context, viewId, sortKey++,
	//	shader->localCBs.getFirst().handle,
	//	&appToShaderData, sizeof(appToShaderData) );

	//mxDO(DrawFullScreenTriangle( render_context, viewId, sortKey, *renderState, shader ));

	return ALL_OK;
}



namespace
{

ERet _AddCommandsToUpdateDeferredLightsBuffer(
	const NwCameraView& scene_view
	, const TSpan< TransientPointLight >& transient_point_lights
	, const HBuffer _deferred_lights_buffer_handle
	, NGpu::NwRenderContext & render_context
	)
{
	enum { SSE_ALIGNMENT = 16 };
	const U32 size_of_uniforms = _max_deferred_lights * sizeof(DeferredLight);

	const NGpu::Memory* uniforms_data = NGpu::Allocate( size_of_uniforms, SSE_ALIGNMENT );
	mxENSURE(uniforms_data, ERR_OUT_OF_MEMORY, "");
	DeferredLight* deferred_lights_dst = (DeferredLight*) uniforms_data->data;

	NGpu::updateBuffer( _deferred_lights_buffer_handle, uniforms_data );

#if 0
	//
	const UINT num_local_lights = visible_from_main_camera.count[RE_Light];

	//
	int num_deferred_lights_written;

	for( num_deferred_lights_written = 0; num_deferred_lights_written < num_local_lights; num_deferred_lights_written++ )
	{
		if( num_deferred_lights_written > _max_deferred_lights ) {
			ptDBG_BREAK;
			break;
		}

		const RrLocalLight& light = *(const RrLocalLight*) visible_from_main_camera.getObjectAt(RE_Light, num_deferred_lights_written);

		if( light.m_lightType == Light_Point )
		{
			const V3f viewSpaceLightPosition = M44_TransformPoint(scene_view.view_matrix, light.position);

			DeferredLight &dst = deferred_lights_dst[ num_deferred_lights_written ];

			dst.position_and_radius = V4f::set( viewSpaceLightPosition, light.radius );
			dst.color_and_inv_radius = V4f::set( light.color * light.brightness, 1.0f / light.radius );
		}
	}
#endif

	//

#if 0

	const int num_dynamic_point_lights = visible_from_main_camera.count[RE_DynamicPointLight];
	const int num_deferred_lights_that_can_be_written = _max_deferred_lights - num_deferred_lights_written;

	if( num_dynamic_point_lights && num_deferred_lights_that_can_be_written )
	{
		//
		const int num_dynamic_point_lights_that_can_be_written
			= num_deferred_lights_that_can_be_written >= num_dynamic_point_lights
			? num_dynamic_point_lights
			: largest( num_deferred_lights_that_can_be_written - num_dynamic_point_lights, 0 )
			;

		const Range< const TbDynamicPointLight* > dynamic_point_lights
			= visible_from_main_camera.getObjectsOfType< TbDynamicPointLight >( RE_DynamicPointLight );

		for( UINT i = 0; i < dynamic_point_lights.num(); i++ )
		{
			const TbDynamicPointLight& dynamic_point_light = *dynamic_point_lights[i];

			const V3f viewSpaceLightPosition = M44_TransformPoint( scene_view.view_matrix, dynamic_point_light.position );

			//
			DeferredLight &deferred_light_dst = deferred_lights_dst[ num_deferred_lights_written + i ];

			deferred_light_dst.position_and_radius = V4f::set( viewSpaceLightPosition, dynamic_point_light.radius );
			deferred_light_dst.color_and_inv_radius = V4f::set( dynamic_point_light.color, 1.0f / dynamic_point_light.radius );
		}

		num_deferred_lights_written += num_dynamic_point_lights_that_can_be_written;
	}

#else


	const int num_dynamic_point_lights = transient_point_lights._count;

	if( num_dynamic_point_lights )
	{
		const int safe_num_dynamic_point_lights = smallest(num_dynamic_point_lights, _max_deferred_lights);

		for( UINT i = 0; i < safe_num_dynamic_point_lights; i++ )
		{
			const TransientPointLight& dynamic_point_light = transient_point_lights._data[i];

			const V3f viewSpaceLightPosition = M44_TransformPoint( scene_view.view_matrix, dynamic_point_light.position );

			//
			DeferredLight &deferred_light_dst = deferred_lights_dst[ i ];

			deferred_light_dst.position_and_radius = V4f::set( viewSpaceLightPosition, dynamic_point_light.radius );
			deferred_light_dst.color_and_inv_radius = V4f::set( dynamic_point_light.color, 1.0f / dynamic_point_light.radius );
		}
	}

#endif

	//
	return ALL_OK;
}

	ERet ApplyToneMapping(
		const RenderPath& render_path
		, RenderSystemData& render_system
		, const RrGlobalSettings& renderer_settings
		, const TbColorTarget& hdr_scene_color_texture
		, const TbColorTarget& dstLumTarget
		, HColorTarget renderTarget_LDR_Scene
		, const HColorTarget* bloomRT_downscaled_and_blurred
		, NwClump& storage
		, NGpu::NwRenderContext & render_context
		)
{
		const U32 viewId_HDR_Tonemap = render_path.getPassDrawingOrder( ScenePasses::HDR_Tonemap );
		{
			NGpu::ViewState	viewState;
			viewState.clear();
			viewState.color_targets[0] = renderTarget_LDR_Scene;
			viewState.target_count = 1;
			//				viewState.renderState = *state_NoCull;
			viewState.viewport.width = render_system._screen_width;
			viewState.viewport.height = render_system._screen_height;
			NGpu::SetViewParameters( viewId_HDR_Tonemap, viewState );
		}
		{
			NGpu::NwPushCommandsOnExitingScope	submitBatch(
				render_context,
				NGpu::buildSortKey(viewId_HDR_Tonemap, 0)
			nwDBG_CMD_SRCFILE_STRING
				);

			NwShaderEffect* technique;
			mxDO(Resources::Load(technique, MakeAssetID("hdr_tonemap"), &storage));
			mxDO(technique->SetInput(
				nwNAMEHASH("t_HDRSceneColor"),
				NGpu::AsInput(hdr_scene_color_texture.handle))
				);
			mxDO(technique->SetInput(
				nwNAMEHASH("t_adaptedLuminance"),
				NGpu::AsInput(dstLumTarget.handle))
				);
			if( bloomRT_downscaled_and_blurred ) {
				mxDO(technique->SetInput(
					nwNAMEHASH("t_bloom"),
					NGpu::AsInput(*bloomRT_downscaled_and_blurred))
					);
			}
			const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
			const NwShaderFeatureBitMask program_index =
				technique->composeFeatureMask( mxHASH_STR("bWithBloom"), renderer_settings.enableBloom )|

				// FXAA (Fast Approximate Anti-Aliasing) requires colors in non-linear, gamma space
				technique->composeFeatureMask( mxHASH_STR("bApplyGammaCorrection"), renderer_settings.enableFXAA )
				;

			mxDO(push_FullScreenTriangle(
				render_context
				, pass0.render_state
				, technique
				, pass0.program_handles[ program_index ]
			));
		}

		return ALL_OK;
	}
}


ERet DeferredRenderer::_DrawCommon(
	const NwCameraView& scene_view
	, const RrGlobalSettings& renderer_settings
	//, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
)
{
	tbPROFILE_FUNCTION;

	RenderSystemData& render_system = *Globals::g_data;

	//
	NwViewport	viewport;
	viewport.width = render_system._screen_width;
	viewport.height = render_system._screen_height;
	viewport.x = 0;
	viewport.y = 0;

	//
	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();


	// always clear the Geometry Buffer before rendering.
	{
		NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

		const U32 scene_pass_index = m_renderPath->getPassDrawingOrder( ScenePasses::FillGBuffer );

		NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			NGpu::buildSortKey( scene_pass_index, 0 )	// first
			nwDBG_CMD_SRCFILE_STRING
			);
		//NGpu::CommandBufferWriter(render_context._command_buffer).dbgPrintf(0,GFX_SRCFILE_STRING);
		//NGpu::CommandBufferWriter(render_context._command_buffer).dbgPrintf(0,"Clear GBuffer / RTs");
		//
		NGpu::Cmd_ClearRenderTargets	clearRenderTargetsCommand;
		{
			clearRenderTargetsCommand.color_targets[0] = m_litTexture->handle;
			clearRenderTargetsCommand.color_targets[1] = m_colorRT0->handle;
			clearRenderTargetsCommand.color_targets[2] = m_colorRT1->handle;
			//clearRenderTargetsCommand.color_targets[3] = m_colorRT2->handle;
			mxZERO_OUT(clearRenderTargetsCommand.clear_colors);
			clearRenderTargetsCommand.target_count = 3;
		}
		render_context._command_buffer.put( clearRenderTargetsCommand );
		//NGpu::CommandBufferWriter(render_context._command_buffer).dbgPrintf(0,"Clear GBuffer / DSV");
		//
		NGpu::Cmd_ClearDepthStencilView	clear_depth_stencil_command;
		{
			clear_depth_stencil_command.depth_target = m_depthRT->handle;

#if USE_REVERSED_DEPTH
			clear_depth_stencil_command.depth_clear_value = 0.0f;
#else
			clear_depth_stencil_command.depth_clear_value = 1.0f;
#endif

			clear_depth_stencil_command.stencil_clear_value = 0;
		}
		render_context._command_buffer.put( clear_depth_stencil_command );
	}




	//


	HColorTarget	renderTarget_LDR_Scene( LLGL_NIL_HANDLE );	//!< the final gamma-corrected LDR render target (before FXAA)
	if( renderer_settings.enableFXAA ) {
		TbColorTarget *	pRT_FXAA_Input;
		mxDO(GetByHash( pRT_FXAA_Input, mxHASH_STR("FXAA_Input"), ClumpList::g_loadedClumps ));
		renderTarget_LDR_Scene = pRT_FXAA_Input->handle;
	} else {
		renderTarget_LDR_Scene.SetDefault();	// back buffer
	}


	// Apply Post-Processing effects and blit the global_light accumulation buffer to the back buffer.
	mxDO(this->applyPostProcessingAndBlit(
		scene_view
		, render_context
		, renderer_settings
		, render_system
		//, atmosphere_rendering_parameters
		));








	if( renderer_settings.enableHDR )
	{
		// Calculate the average luminance first.

		const UINT iSrcLumTarget = m_iCurrentLuminanceRT ^ 1;	// take adapted luminance from previous frame
		const UINT iDstLumTarget = m_iCurrentLuminanceRT;

		const TbColorTarget* pSrcLumTarget = m_adaptedLuminance[ iSrcLumTarget ];
		const TbColorTarget* pDstLumTarget = m_adaptedLuminance[ iDstLumTarget ];

		// Luminance mapping.
		{
			const U32 viewId_CreateLuminance = m_renderPath->getPassDrawingOrder( ScenePasses::HDR_CreateLuminance );

			// Create average luminance calculation targets
			NwPooledRT	initialLuminanceRT(
				const_cast<NwRenderTargetPool&>(render_system._renderTargetPool),
				HDR_Renderer::LUMINANCE_RT_SIZE, HDR_Renderer::LUMINANCE_RT_SIZE,
				HDR_Renderer::LUMINANCE_RT_FORMAT
				);

			{
				NGpu::ViewState	viewState;
				viewState.clear();
				viewState.color_targets[0] = initialLuminanceRT.handle;
				viewState.target_count = 1;
				viewState.viewport.width = initialLuminanceRT.width;
				viewState.viewport.height = initialLuminanceRT.height;
				NGpu::SetViewParameters( viewId_CreateLuminance, viewState );
			}

			// Convert HDR image to log luminance floating-point values
			{
				NGpu::NwPushCommandsOnExitingScope	submitBatch(
					render_context,
					NGpu::buildSortKey(viewId_CreateLuminance, 0)
			nwDBG_CMD_SRCFILE_STRING
					);

				NwShaderEffect* technique;
				mxDO(Resources::Load(technique, MakeAssetID("hdr_calculate_luminance"), m_rendererData));

				mxDO(technique->SetInput(nwNAMEHASH("t_screen"), NGpu::AsInput(m_litTexture->handle)));

				const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

				mxDO(push_FullScreenTriangle( render_context, pass0.render_state, technique, pass0.default_program_handle ));
			}

			// Eye adaptation
			{
				const U32 viewId_AdaptLuminance = m_renderPath->getPassDrawingOrder( ScenePasses::HDR_AdaptLuminance );
				{
					NGpu::ViewState	viewState;
					viewState.clear();
					viewState.color_targets[0] = pDstLumTarget->handle;
					viewState.target_count = 1;
					viewState.viewport.width = HDR_Renderer::LUMINANCE_RT_SIZE;
					viewState.viewport.height = HDR_Renderer::LUMINANCE_RT_SIZE;
					viewState.flags = 0;	// Don't clear! we need luminance from previous frames!
					NGpu::SetViewParameters( viewId_AdaptLuminance, viewState );
				}
				{
					NGpu::NwPushCommandsOnExitingScope	submitBatch(
						render_context,
						NGpu::buildSortKey(viewId_AdaptLuminance, 0)
			nwDBG_CMD_SRCFILE_STRING
						);

					NwShaderEffect* technique;
					mxDO(Resources::Load(technique, MakeAssetID("hdr_adapt_luminance"), m_rendererData));

					mxDO(technique->SetInput(nwNAMEHASH("t_initialLuminance"), NGpu::AsInput(initialLuminanceRT.handle)));
					mxDO(technique->SetInput(nwNAMEHASH("t_adaptedLuminance"), NGpu::AsInput(pSrcLumTarget->handle)));

					const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

					mxDO(push_FullScreenTriangle( render_context, pass0.render_state, technique, pass0.default_program_handle ));

					// Downscale luminance

					// Generate the mip chain
					{
						NGpu::Cmd_GenerateMips	cmd_GenerateMips;
						cmd_GenerateMips.encode( NGpu::CMD_GENERATE_MIPS, 0, NGpu::AsInput(pDstLumTarget->handle).id );

						mxDO(render_context._command_buffer.put( cmd_GenerateMips ));
					}
				}

				m_iCurrentLuminanceRT ^= 1;	// ping-pong
			}
		}





		// Now do the bloom.
		if( renderer_settings.enableBloom )
		{
			// //#define FLOAT_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT
			//const NwDataFormat::Enum BLOOM_RT_FORMAT = NwDataFormat::RGBA16F;
			const NwDataFormat::Enum BLOOM_RT_FORMAT = NwDataFormat::R11G11B10_FLOAT;

			// Downscale by 2, extract and blur bright parts of the scene.

			NwPooledRT	bloomRT_extract_bright_parts(
				render_system._renderTargetPool,
				scene_view.screen_width / 2, scene_view.screen_height / 2,
				BLOOM_RT_FORMAT
				);
			{
				// Extract the bright parts of the scene
				const U32 viewId_Bloom_Threshold = m_renderPath->getPassDrawingOrder( ScenePasses::Bloom_Threshold );

				NGpu::NwPushCommandsOnExitingScope	submitBatch(
					render_context,
					NGpu::buildSortKey(viewId_Bloom_Threshold, 0)
			nwDBG_CMD_SRCFILE_STRING
					);

				{
					NGpu::ViewState	viewState;
					viewState.clear();
					viewState.color_targets[0] = bloomRT_extract_bright_parts.handle;
					viewState.target_count = 1;
					viewState.viewport.width = bloomRT_extract_bright_parts.width;
					viewState.viewport.height = bloomRT_extract_bright_parts.height;
					NGpu::SetViewParameters( viewId_Bloom_Threshold, viewState );
				}
				{
					NwShaderEffect* technique;
					mxDO(Resources::Load(technique, MakeAssetID("bloom_threshold"), m_rendererData));
					mxDO(technique->SetInput(nwNAMEHASH("t_hdr_scene_color"), NGpu::AsInput(m_litTexture->handle)));
					mxDO(technique->SetInput(nwNAMEHASH("t_adaptedLuminance"), NGpu::AsInput(pDstLumTarget->handle)));

					const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
					mxDO(push_FullScreenTriangle(
						render_context, pass0.render_state, technique, pass0.default_program_handle
						));
				}
			}//Bloom_Threshold


			// Downscale

			//
			NwPooledRT	bloomRT_downscaled2(
				render_system._renderTargetPool,
				scene_view.screen_width / 2, scene_view.screen_height / 2,
				BLOOM_RT_FORMAT
				);

			NwPooledRT	bloomRT_downscaled4(
				render_system._renderTargetPool,
				scene_view.screen_width / 4, scene_view.screen_height / 4,
				BLOOM_RT_FORMAT
				);

			NwPooledRT	bloomRT_downscaled8(
				render_system._renderTargetPool,
				scene_view.screen_width / 8, scene_view.screen_height / 8,
				BLOOM_RT_FORMAT
				);

			NwPooledRT	bloomRT_downscaled_blurred(
				render_system._renderTargetPool,
				bloomRT_downscaled8.width, bloomRT_downscaled8.height,
				BLOOM_RT_FORMAT
				);


			{
				const NGpu::LayerID view_id_Bloom_Downscale = m_renderPath->getPassDrawingOrder( ScenePasses::Bloom_Downscale );

				NGpu::NwPushCommandsOnExitingScope	submitBatch(
					render_context,
					NGpu::buildSortKey(view_id_Bloom_Downscale, 0)
			nwDBG_CMD_SRCFILE_STRING
					);

				//
				struct BloomDownscaleHelper
				{
					static ERet Downscale(
						const NwPooledRT& src_RT
						, const NwPooledRT& dst_RT
						, NwClump& storage
						, NGpu::NwRenderContext & render_context
						)
					{
						NGpu::CommandBufferWriter	cmd_buf_writer(render_context._command_buffer);

						{
							NGpu::RenderTargets	render_targets;
							render_targets.clear();
							render_targets.color_targets[0] = dst_RT.handle;
							render_targets.target_count = 1;

							cmd_buf_writer.SetRenderTargets(render_targets);
						}

						cmd_buf_writer.SetViewport(dst_RT.width, dst_RT.height);
						

						{
							NwShaderEffect* technique;
							mxDO(Resources::Load(technique, MakeAssetID("bloom_downscale"), &storage));

							mxDO(technique->SetInput(nwNAMEHASH("t_src"), NGpu::AsInput(src_RT.handle)));

							const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
							mxDO(push_FullScreenTriangle(
								render_context, pass0.render_state, technique, pass0.default_program_handle
								));
						}

						return ALL_OK;
					}
				};

				mxDO(BloomDownscaleHelper::Downscale(
					bloomRT_extract_bright_parts
					, bloomRT_downscaled2
					, *m_rendererData
					//, view_id_Bloom_Downscale
					, render_context
					));
				mxDO(BloomDownscaleHelper::Downscale(
					bloomRT_downscaled2
					, bloomRT_downscaled4
					, *m_rendererData
					//, view_id_Bloom_Downscale
					, render_context
					));
				mxDO(BloomDownscaleHelper::Downscale(
					bloomRT_downscaled4
					, bloomRT_downscaled8
					, *m_rendererData
					//, view_id_Bloom_Downscale
					, render_context
					));

				//

				// Blur it

				struct BloomBlurHelper
				{
					static ERet BlurSingleDirection(
						const NwPooledRT& src_RT
						, const NwPooledRT& dst_RT
						, const bool blur_horizontally
						, NwClump& storage
						//, const NGpu::LayerID view_id_Bloom_Downscale
						, NGpu::NwRenderContext & render_context
						//, TbColorTarget& hdr_scene_color_texture
						//, TbColorTarget& adapted_luminance_texture
						)
					{
						NGpu::CommandBufferWriter	cmd_buf_writer(render_context._command_buffer);

						float afSampleOffsets[15];
						float afSampleWeights[15];

						GetSampleOffsets_Bloom(
							blur_horizontally ? src_RT.width : src_RT.height
							, afSampleOffsets
							, afSampleWeights
							, 3.0f
							, 1.25f
							);

						//
						V4f horiz_sample_offsets[15];
						V4f horiz_sample_weights[15];

						for( int i = 0; i < 15; i++ )
						{
							if(blur_horizontally) {
								horiz_sample_offsets[i] = V4f::set( afSampleOffsets[i], 0.0f, 0.0f, 0.0f );
							} else {
								horiz_sample_offsets[i] = V4f::set( 0.0f, afSampleOffsets[i], 0.0f, 0.0f );
							}

							horiz_sample_weights[i] = V4f::replicate( afSampleWeights[i] );
						}

						//
						NwShaderEffect* technique_bloom_blur;
						mxDO(Resources::Load(technique_bloom_blur, MakeAssetID("bloom_blur"), &storage));

						mxDO(technique_bloom_blur->SetInput(
							nwNAMEHASH("t_input_texture"),
							NGpu::AsInput(src_RT.handle)
							));
						mxDO(technique_bloom_blur->setUniform(
							mxHASH_STR("g_avSampleOffsets"),
							horiz_sample_offsets
							));
						mxDO(technique_bloom_blur->setUniform(
							mxHASH_STR("g_avSampleWeights"),
							horiz_sample_weights
							));

						technique_bloom_blur->pushAllCommandsInto(render_context._command_buffer);

						{
							NGpu::RenderTargets	render_targets;
							render_targets.clear();
							render_targets.color_targets[0] = dst_RT.handle;
							render_targets.target_count = 1;

							cmd_buf_writer.SetRenderTargets(render_targets);
						}

						cmd_buf_writer.SetViewport(dst_RT.width, dst_RT.height);
						

						const NwShaderEffect::Pass& pass0 = technique_bloom_blur->getPasses()[0];
						mxDO(push_FullScreenTriangle(
							render_context,
							pass0.render_state,
							technique_bloom_blur,
							pass0.default_program_handle
							));

						return ALL_OK;
					}

				private:

					static float GaussianDistribution( float x, float y, float rho )
					{
						float g = 1.0f / sqrtf( mxTWO_PI * rho * rho );
						g *= expf( -( x * x + y * y ) / ( 2 * rho * rho ) );

						return g;
					}

					static void GetSampleOffsets_Bloom(
						DWORD dwD3DTexSize,
						float afTexCoordOffset[15],
						float avColorWeights[15],
						float fDeviation, float fMultiplier )
					{
						int i = 0;
						float tu = 1.0f / ( float )dwD3DTexSize;

						// Fill the center texel
						float weight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
						avColorWeights[0] = weight;

						afTexCoordOffset[0] = 0.0f;

						// Fill the right side
						for( i = 1; i < 8; i++ )
						{
							weight = fMultiplier * GaussianDistribution( ( float )i, 0, fDeviation );
							afTexCoordOffset[i] = i * tu;

							avColorWeights[i] = weight;
						}

						// Copy to the left side
						for( i = 8; i < 15; i++ )
						{
							avColorWeights[i] = avColorWeights[i - 7];
							afTexCoordOffset[i] = -afTexCoordOffset[i - 7];
						}
					}
				};


				//

				// Blur 'bright pass' horizontally.

				mxDO(BloomBlurHelper::BlurSingleDirection(
					bloomRT_downscaled8
					, bloomRT_downscaled_blurred
					, true
					, *m_rendererData
					, render_context
					));

				// Blur 'bright pass' vertically.

				mxDO(BloomBlurHelper::BlurSingleDirection(
					bloomRT_downscaled_blurred
					, bloomRT_downscaled8
					, false
					, *m_rendererData
					, render_context
					));
			}//Downscale and Blur

			mxDO(ApplyToneMapping(
				*m_renderPath
				, render_system
				, renderer_settings
				, *m_litTexture
				, *pDstLumTarget
				, renderTarget_LDR_Scene
				, &bloomRT_downscaled8.handle
				, *m_rendererData
				, render_context
				));
		}//If Bloom is enabled.

		else
		{
			// Apply tone mapping without bloom.

			mxDO(ApplyToneMapping(
				*m_renderPath
				, render_system
				, renderer_settings
				, *m_litTexture
				, *pDstLumTarget
				, renderTarget_LDR_Scene
				, nil//HTexture bloomRT_downscaled_and_blurred
				, *m_rendererData
				, render_context
				));
}




	}//If HDR is enabled.



	// Blit to main RT (or FXAA target).

	// Fast Approximate Anti-Aliasing (FXAA)

	if( renderer_settings.enableFXAA )
	{
		const U32 viewId = m_renderPath->getPassDrawingOrder( ScenePasses::FXAA );
		mxASSERT(viewId != ~0);

		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(viewId, 0)
			nwDBG_CMD_SRCFILE_STRING
			);

		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("fxaa"), m_rendererData));

		TPtr< TbColorTarget >	t_FXAA_Input;
		mxDO(GetByHash(t_FXAA_Input._ptr, mxHASH_STR("FXAA_Input"), ClumpList::g_loadedClumps));
		mxDO(technique->SetInput(nwNAMEHASH("t_screen"), NGpu::AsInput(t_FXAA_Input->handle)));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, pass0.default_program_handle
			));
	}

	return ALL_OK;
}

ERet DeferredRenderer::DrawEntities(
								 const NwCameraView& scene_view
								 , const RenderEntityList& visible_entities
								 , const RenderSystemData& render_system
								 //, const SpatialDatabaseI* world
								 , const RrGlobalSettings& renderer_settings
								 )
{
	tbPROFILE_FUNCTION;

	mxDO(_DrawCommon(
		scene_view
		, renderer_settings
		//, atmosphere_rendering_parameters
		));

	//
	NwViewport	viewport;
	viewport.width = render_system._screen_width;
	viewport.height = render_system._screen_height;
	viewport.x = 0;
	viewport.y = 0;

	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();


	// Fill Geometry Buffer.
	// NOTE: We actually only need to clear the depth buffer here since we replace unwritten (i.e. far plane) samples
	// with the skybox. We use the depth buffer to reconstruct position and only in-frustum positions are shaded.


	// Generate an occlusion buffer for Screen space ambient [occlusion|obscurance].
	{

		//UNDONE;
		//		m_AO.Render( render_context, *m_renderPath, scene_view, m_rendererData );
	}

	// Apply deferred lighting: additively blend into 'LightingTarget'.
	mxBUG("Run-Time Check Failure #2 - Stack around the variable 'lightIt' was corrupted.");
#if 0
	{
		const U32 viewId = m_renderPath->getPassDrawingOrder( ScenePasses::DeferredLocalLights );
		mxASSERT(viewId != ~0);

		Clump::Iterator< RrLocalLight >	lightIt( sceneData );
		while( lightIt.IsValid() )
		{
			const RrLocalLight& light = lightIt.Value();
			mxTEMP
				if( light.m_lightType == Light_Spot )
				{
					//				const V3f viewSpaceLightPosition = M44_TransformPoint(scene_view.view_matrix, light.position);
					if( light.DoesCastShadows() ) {
						m_shadowRenderer.Prepare_ShadowMap_for_SpotLight(sceneData, sceneView, light, *m_renderPath);
					}

					Apply_Deferred_Spot_Light(context, viewId, sceneView, light, *this);

					stats->deferred_lights++;
				}

				lightIt.MoveToNext();
		}
	}
#endif






	// Cull lights and do lighting on the GPU, using a single Compute Shader.
	// NOTE: this also clears the light accumulation buffer.
	{
		const U32 viewId_TiledShadingCS = m_renderPath->getPassDrawingOrder( ScenePasses::TiledDeferred_CS );

		// Compute shader setup (always does all the lights at once)
		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("deferred_compute_shader_tile"), m_rendererData));

		//
		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(viewId_TiledShadingCS, 0)
			nwDBG_CMD_SRCFILE_STRING
			);
//NGpu::buildSortKey(viewId_TiledShadingCS,
//	NGpu::buildSortKey(cmd_dispatch_compute_shader.program, NGpu::SortKey(1))

		//UNDONE;
		U32 lightCount = transient_point_lights.num();
		if(lightCount)
		{
			_AddCommandsToUpdateDeferredLightsBuffer(
				scene_view
				, Arrays::GetSpan(transient_point_lights)
				, _deferred_lights_buffer_handle
				, render_context
				);
			transient_point_lights.RemoveAll();
		}

		{
			const U32 tilesX = CalculateNumTilesX(viewport.width);	// numHorizontalTiles
			const U32 tilesY = CalculateNumTilesY(viewport.height);// numVerticalTiles

			//
			TiledShadingConstants *	constants;
			mxDO(technique->BindCBufferData(
				&constants
				, render_context._command_buffer
				));
			constants->g_tileCount.x = tilesX;
			constants->g_tileCount.y = tilesY;
			constants->g_deferredLightCount.x = lightCount;
		}

		mxDO(technique->SetInput( nwNAMEHASH("b_pointLights"), NGpu::AsInput(_deferred_lights_buffer_handle) ));
		mxDO(technique->SetInput( nwNAMEHASH("DepthTexture"), NGpu::AsInput(m_depthRT->handle) ));
		mxDO(technique->SetInput( nwNAMEHASH("GBufferTexture0"), NGpu::AsInput(m_colorRT0->handle) ));
		mxDO(technique->SetInput( nwNAMEHASH("GBufferTexture1"), NGpu::AsInput(m_colorRT1->handle) ));
		mxDO(technique->setUav( mxHASH_STR("t_litTarget"), NGpu::AsOutput(m_litTexture->handle) ));
#if DEBUG_EMIT_POINTS
		mxDO(FxSlow_SetOutput(technique, "u_debugPoints", NGpu::AsOutput(u_debugPoints)));
#endif

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::Cmd_DispatchCS cmd_dispatch_compute_shader;
		{
			cmd_dispatch_compute_shader.clear();
			cmd_dispatch_compute_shader.encode( NGpu::CMD_DISPATCH_CS, 0, 0 );

			cmd_dispatch_compute_shader.program = pass0.default_program_handle;
			// Construct a screen space grid, covering the frame buffer, with some fixed tile size.
			cmd_dispatch_compute_shader.groupsX = CalculateNumTilesX(render_system._screen_width);
			cmd_dispatch_compute_shader.groupsY = CalculateNumTilesY(render_system._screen_height);
			cmd_dispatch_compute_shader.groupsZ = 1;
		}
		render_context._command_buffer.put( cmd_dispatch_compute_shader );
	}




	return ALL_OK;
}

ERet DeferredRenderer::applyPostProcessingAndBlit(
	const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const RrGlobalSettings& renderer_settings
	, const RenderSystemData& render_system
	//, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
	)
{
	IF_DEVELOPER GFX_SCOPED_MARKER( render_context._command_buffer, RGBAi::LIGHTYELLOW );

	mxTEMP
	NwViewport	viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = render_system._screen_width;
	viewport.height = render_system._screen_height;

	const ScenePassInfo scene_pass_info = m_renderPath->getPassInfo( ScenePasses::DeferredCompositeLit );

	ScenePassData & viewData = m_renderPath->passData[ scene_pass_info.passDataIndex ];
	{
		// Blit to main RT (or FXAA target).
		if( renderer_settings.enableFXAA ) {
			viewData.render_targets[0] = mxHASH_STR("FXAA_Input");
		} else {
			viewData.render_targets[0] = ScenePassData::BACKBUFFER_ID;
		}
		NGpu::ViewState	viewState;
		viewState.clear();
		mxDO(ViewState_From_ScenePass( viewState, viewData, viewport ));
		NGpu::SetViewParameters( scene_pass_info.draw_order, viewState );
	}

	//
	NwShaderEffect* technique;
	mxCHECK(Resources::Load(technique, MakeAssetID("post_processing_ueber"), m_rendererData));

	mxCHECK(technique->SetInput( nwNAMEHASH("t_screen"), NGpu::AsInput(m_litTexture->handle)));
	//technique->SetInput( mxHASH_STR("DepthTexture"), NGpu::AsInput(m_depthRT->handle));
	//mxDO(FxSlow_SetInput(technique, "GBufferTexture0", NGpu::AsInput(m_colorRT0->handle)));
	//mxDO(FxSlow_SetInput(technique, "GBufferTexture1", NGpu::AsInput(m_colorRT1->handle)));

	//
	//UeberPostProcessingShaderData	shader_data;
	//ShaderGlobals::packAtmosphereRenderingParameters( shader_data, atmosphere_rendering_parameters );
	//technique->setUniform( mxHASH_STR("u_data"), &shader_data );

	//
	const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

	//
	NGpu::NwPushCommandsOnExitingScope	submitCommands(
		render_context,
		NGpu::buildSortKey( scene_pass_info.draw_order, pass0.default_program_handle, NGpu::SortKey(0) )
			nwDBG_CMD_SRCFILE_STRING
		);

	//if( renderer_settings.enableBlendedOIT )
	//{
	//	TPtr< TbColorTarget >	t_blendedOIT_accumulated;
	//	TPtr< TbColorTarget >	t_blendedOIT_revealage;
	//	mxDO(GetByHash(t_blendedOIT_accumulated._ptr, mxHASH_STR("BlendedOIT_Accumulation"), ClumpList::g_loadedClumps));
	//	mxDO(GetByHash(t_blendedOIT_revealage._ptr, mxHASH_STR("BlendedOIT_Revealage"), ClumpList::g_loadedClumps));
	//	mxDO(FxSlow_SetInput(technique, "t_blendedOIT_accumulated", NGpu::AsInput(t_blendedOIT_accumulated->handle)));
	//	mxDO(FxSlow_SetInput(technique, "t_blendedOIT_revealage", NGpu::AsInput(t_blendedOIT_revealage->handle)));
	//}


	//const bool bApplyGammaCorrection = renderer_settings.enableFXAA ?
	//	(renderer_settings.enableHDR ? false : true)
	//	: false	// Hardware Linear -> sRGB conversion
	//	;

//	const NwShaderFeatureBitMask program_index =
////		technique->ComposeFeatureMask( mxHASH_STR("bEnableFog"), renderer_settings.enableGlobalFog )|
////		technique->ComposeFeatureMask( mxHASH_STR("bAddBlendedOIT"), renderer_settings.enableBlendedOIT )|
//		// FXAA requires colors in non-linear, gamma space
////		technique->ComposeFeatureMask( mxHASH_STR("bApplyGammaCorrection"), bApplyGammaCorrection )
//		technique->composeFeatureMask( mxHASH_STR("FeatureBool_UseVxgiAmbientOcclusion"), renderer_settings.gi.enable_AO )|		
//		;

	//HBuffer	hCBFog = technique->localCBs.getFirst().handle;
	////mxDO(shader->FindCBuffer(mxHASH_STR("u_fog"), &hCBFog));

	//SHeightFogData *		fogData;
	//mxDO(NGpu::Recording::insertUpdateBufferCommand( render_context._commands, hCBFog, &fogData ));
	//{
	//	fogData->color = V3_Set(0.5, 0.6, 0.9);
	//	fogData->density = 1;
	//	fogData->startDistance = 10;
	//	fogData->extinctionDistance = 400;
	//	fogData->minHeight = -10;
	//	fogData->maxHeight = 150;
	//}

	mxDO(technique->pushAllCommandsInto( render_context._command_buffer ));

	mxDO(push_FullScreenTriangle( render_context, pass0.render_state, technique, pass0.default_program_handle ));

	return ALL_OK;
}

///
ERet DeferredRenderer::addDebugRenderCommandsIfNeeded(
	const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const RrGlobalSettings& renderer_settings
	) const
{
	const TbRendererDebugSettings& renderer_debug_settings = renderer_settings.debug;

	if( renderer_debug_settings.scene_render_mode != DebugSceneRenderMode::None )
	{
		const U32 first_command_offset = render_context._command_buffer.currentOffset();

		const U32 view_id_debug_textures = m_renderPath->getPassDrawingOrder( ScenePasses::DebugTextures );

		//
		NwRenderState * state_NoCull = nil;
		mxDO(Resources::Load(state_NoCull, MakeAssetID("nocull"), m_rendererData));
UNDONE;
		////
		//NwShaderEffect* shader;

		//switch( renderer_debug_settings.scene_render_mode )
		//{
		//case DebugSceneRenderMode::ShowDepthBuffer:
		//	Unimplemented;
		//	return ERR_NOT_IMPLEMENTED;

		//case DebugSceneRenderMode::ShowAlbedo:
		//	Unimplemented;
		//	return ERR_NOT_IMPLEMENTED;

		//case DebugSceneRenderMode::ShowAmbientOcclusion:
		//	{
		//		TbColorTarget *	albedo_and_occlusion_RT;
		//		mxDO(GetByHash( albedo_and_occlusion_RT, mxHASH_STR("GBufferTexture1"), ClumpList::g_loadedClumps ));

		//		mxDO(Resources::Load( shader, MakeAssetID("debug_show_ambient_occlusion"), m_rendererData ));
		//		mxDO(FxSlow_SetInput(shader, "source_texture", NGpu::AsInput(albedo_and_occlusion_RT->handle)));
		//	} break;

		//case DebugSceneRenderMode::ShowNormals:
		//	Unimplemented;
		//	return ERR_NOT_IMPLEMENTED;

		//case DebugSceneRenderMode::ShowSpecularColor:
		//	Unimplemented;
		//	return ERR_NOT_IMPLEMENTED;
		//}

		//mxDO(DrawFullScreenTriangle( render_context, *state_NoCull, shader ));

		//render_context.submit(
		//	first_command_offset,
		//	NGpu::buildSortKey( view_id_debug_textures, NGpu::SortKey(0) )
		//);
	}

#if 0

	if( g_settings.gi.dbg_show_ao_texture )
	{
		const U32 first_command_offset = render_context._commands.current_offset();

		const U32 view_id_debug_textures = m_renderPath->getPassDrawingOrder( ScenePasses::DebugTextures );
		mxASSERT(view_id_debug_textures != ~0);

		//
		NwRenderState * state_NoCull = nil;
		mxDO(Resources::Load(state_NoCull, MakeAssetID("nocull"), m_rendererData));

		//
		NwShaderEffect* shader;
		mxDO(Resources::Load( shader, MakeAssetID("debug_voxel_gi_show_voxel_ambient_occlusion"), m_rendererData ));

		//
		/*mxDO*/(FxSlow_SetInput(shader, "DepthTexture", GL::AsInput(m_depthRT->handle)));
		/*mxDO*/(FxSlow_SetInput(shader, "GBufferTexture0", GL::AsInput(m_colorRT0->handle)));
		/*mxDO*/(FxSlow_SetInput(shader, "GBufferTexture1", GL::AsInput(m_colorRT1->handle)));
		///*mxDO*/(FxSlow_SetInput(shader, "GBufferTexture2", GL::AsInput(m_colorRT2->handle)));

		mxDO(FxSlow_SetInput(shader, "t_voxel_albedo_density", GL::AsInput(_voxel_cone_tracing._radiance_opacity_texture)));
		mxDO(FxSlow_SetSampler( shader, "s_trilinear_sampler", BuiltIns::g_samplers[BuiltIns::TrilinearSampler] ));

		RenderSystemData& render_system = (RenderSystemData&)Rendering::Globals::;
		mxDO(FxSlow_SetInput(shader, "t_shadow_depth_map", GL::AsInput(render_system._shadow_map_renderer._shadow_depth_map_handle)));
		mxDO(FxSlow_SetSampler( shader, "s_shadow_depth_map_pcf_sampler", BuiltIns::g_samplers[BuiltIns::ShadowMapSamplerPCF] ));

		const M44f	u_view_to_shadow_texture_space = M44_Transpose(render_system._view_to_shadow_texture_space[0]);
		mxDO(FxSlow_SetUniform( shader, "u_view_to_shadow_texture_space", &u_view_to_shadow_texture_space ));

		//
		V3f		u_voxel_grid_min_corner;	// in view space - for converting pixel positions from view space into volume texture space
		float	u_position_bias;
		float	u_inv_voxel_grid_size;		// world-space dimensions of the voxel grid
		UINT	u_voxel_grid_dimension;		//
		M44f	u_inverse_view_matrix;		// for converting normals from view space into world space
		M44f	u_view_to_grid_space;
		//
		u_voxel_grid_min_corner = _voxel_cone_tracing._current_settings.volume_origin_in_view_space;
		u_position_bias = _voxel_cone_tracing._current_settings.position_bias;
		u_inv_voxel_grid_size = 1.0f / _voxel_cone_tracing._current_settings.vol_tex_dim_in_world_space;
		u_voxel_grid_dimension = 1 << _voxel_cone_tracing._current_settings.vol_tex_res_log2;
		u_inverse_view_matrix = M44_Transpose( scene_view.inverse_view_matrix );
		{
			u_view_to_grid_space = scene_view.inverse_view_matrix;	// transform from view into world space
			u_view_to_grid_space = M44_Multiply( u_view_to_grid_space, M44_buildTranslationMatrix( -_voxel_cone_tracing._current_settings.volume_min_corner_in_world_space ) );
			u_view_to_grid_space = M44_Multiply( u_view_to_grid_space, M44_uniformScaling( 1.0f / _voxel_cone_tracing._current_settings.vol_tex_dim_in_world_space ) );

			u_view_to_grid_space = M44_Transpose( u_view_to_grid_space );
		}
		//
		mxDO(FxSlow_SetUniform( shader, "u_voxel_grid_min_corner",	&u_voxel_grid_min_corner ));
		mxDO(FxSlow_SetUniform( shader, "u_position_bias",			&u_position_bias ));
		mxDO(FxSlow_SetUniform( shader, "u_inv_voxel_grid_size",	&u_inv_voxel_grid_size ));
		mxDO(FxSlow_SetUniform( shader, "u_voxel_grid_dimension",	&u_voxel_grid_dimension ));
		mxDO(FxSlow_SetUniform( shader, "u_inverse_view_matrix",	&u_inverse_view_matrix ));
		mxDO(FxSlow_SetUniform( shader, "u_view_to_grid_space",		&u_view_to_grid_space ));

		mxDO(FxSlow_Commit( render_context, view_id_debug_textures, shader, 0 ));

		SetShaderParameters( render_context, *shader );

		//
		const NwShaderFeatureBitMask program_index = 0
			//|shader->passes[0].ComposeFeatureMask( mxHASH_STR("ENABLE_CASCADED_SHADOW_MAPPING"), 1/*renderer_settings.enableShadows*/ )
			;

		mxDO(DrawFullScreenTriangle( render_context, *state_NoCull, shader, program_index ));

		//
		render_context.submit(
			first_command_offset,
			GL::buildSortKey( view_id_debug_textures, GL::SortKey(0) )
		);
	}
#endif
	return ALL_OK;
}

}//namespace Rendering

/*
Physically Based Deferred Rendering in Costume Quest 2 [2014]:
http://nosferalatu.com/CQ2Rendering.html
*/
