#include <Base/Base.h>
#pragma hdrstop

// for std::sort()
#include <algorithm>

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Math/BoundingVolumes/ViewFrustum.h>

#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Util/Tweakable.h>

#include <Engine/WindowsDriver.h>

#include <Graphics/Public/graphics_utilities.h>

#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Settings.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Private/ShaderInterop.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/SceneRendering.h>
//#include <Renderer/private/shadows/shadow_map_renderer.h>
#include <Rendering/Private/Modules/Particles/Particle_Rendering.h>
#include <Rendering/Private/Modules/idTech4/idRenderModel.h>

mxREFACTOR("shouldn't be here:")//for DeferredLight
#include <Rendering/Private/Pipelines/TiledDeferred/TiledCommon.h>


namespace Rendering
{
namespace Globals
{

TPtr< RenderSystemData >	g_data;


/// mxTRYnWARN(X) - evaluates the given expression; if it failes,
/// shows a warning.
#if MX_DEBUG || MX_DEVELOPER
	#define mxTRYnWARN( X )\
		mxMACRO_BEGIN\
			const ERet __result = (X);\
			if( mxFAILED(__result) )\
			{\
				const char* errorMessage = EReturnCode_To_Chars( __result );\
				ptWARN( "%s(%d): '%s' failed with '%s'\n", __FILE__, __LINE__, #X, errorMessage );\
			}\
		mxMACRO_END
#else
	#define mxTRYnWARN( X )\
		mxMACRO_BEGIN\
			const ERet __result = (X);\
			if( mxFAILED(__result) )\
			{\
				ptWARN( "Error: %d\n", (int)__result );\
			}\
		mxMACRO_END
#endif




namespace
{

ERet _updatePerFrameShaderConstants(
	RenderSystemData& data
	, const RrGlobalSettings& renderer_settings
	, const NwTime& game_time
	)
{
	const InputState& inputState = NwInputSystemI::Get().getState();

	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

	NGpu::NwPushCommandsOnExitingScope	submitCommands(
		render_context,
		NGpu::buildSortKey( NGpu::LayerID(0), 0 )	// first
			nwDBG_CMD_SRCFILE_STRING
		);

	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

//cmd_writer.dbgPrintf(0,GFX_SRCFILE_STRING);

	G_PerFrame *		per_frame_constants;
	mxDO(cmd_writer.allocateUpdateBufferCommandWithData(
		data._global_uniforms.per_frame.handle
		, &per_frame_constants
		));

	ShaderGlobals::SetPerFrameConstants(
		game_time.real.total_elapsed
		, game_time.real.delta_seconds
		, inputState.mouse.x
		, inputState.mouse.y
		, (inputState.mouse.held_buttons_mask & BIT(MouseButton_Left))
		, (inputState.mouse.held_buttons_mask & BIT(MouseButton_Right))
		, renderer_settings
		, per_frame_constants
		);

	// thick wireframe lines should be antialiased, if possible
	{
		const ScenePassInfo debugWireframePassInfo = GetRenderPath().getPassInfo( ScenePasses::DebugWireframe );

		RenderPath &render_path = *data._deferred_renderer.m_renderPath;
		ScenePassData & scenePassData = render_path.passData[ debugWireframePassInfo.passDataIndex ];

		if( renderer_settings.enableFXAA ) {
			scenePassData.render_targets[0] = mxHASH_STR("FXAA_Input");
		} else {
			scenePassData.render_targets[0] = ScenePassData::BACKBUFFER_ID;
		}

		NGpu::ViewState	viewState;
		viewState.clear();

		//
		NwViewport	viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = data._screen_width;
		viewport.height = data._screen_height;
		mxDO(ViewState_From_ScenePass( viewState, scenePassData, viewport ));

		NGpu::SetViewParameters( debugWireframePassInfo.draw_order, viewState );
	}

	return ALL_OK;
}

ERet _UpdatePerViewShaderConstants(
	RenderSystemData& data
	, const NwCameraView& scene_view
	, const RrGlobalSettings& renderer_settings
	)
{
	G_PerCamera *		per_view_constants;
	mxDO(NGpu::updateGlobalBuffer(
		data._global_uniforms.per_camera.handle
		, &per_view_constants
		));


	//
	ShaderGlobals::SetPerViewConstants(
		per_view_constants
		, scene_view
		, (float)data._screen_width, (float)data._screen_height
		, renderer_settings
		);

#if nwRENDERER_CFG_WITH_VOXEL_GI
	//
	UNDONE;
	//
	const RrWorldState_VXGI& vxgiState = world->getState_VXGI();

#endif

	return ALL_OK;
}

}//namespace







void ApplySettings( const RrGlobalSettings& new_settings )
{

#if nwRENDERER_ENABLE_SHADOWS
	_shadow_map_renderer.modify( new_settings.shadows );
#endif

#if nwRENDERER_CFG_WITH_VOXEL_GI
	_gpu_voxels.ApplySettings( new_settings.vxgi );
#endif

}





void _resizeViewportIfNeeded(
	int screen_width, int screen_height
	, const RrGlobalSettings& renderer_settings
	)
{
	mxTEMP
	UNDONE;
#if 0
	const float viewportWidth = (float) screen_width;
	const float viewportHeight = (float) screen_height;

	if( viewportWidth != data._screen_width || viewportHeight != data._screen_height )
	{
		ptWARN("Resizing from %ux%u to %ux%u",
			(int)data._screen_width, (int)data._screen_height, (int)viewportWidth, (int)viewportHeight);

		renderer_ResizeBuffers( viewportWidth, viewportHeight, false );

		data._screen_width = viewportWidth;
		data._screen_height = viewportHeight;
	}

	{
		U32 videoModeFlags = 0;
		if( renderer_settings.enable_Fullscreen ) {
			videoModeFlags |= GL::FullScreen;
		}
		if( renderer_settings.enable_VSync ) {
			videoModeFlags |= GL::EnableVSync;
		}

		GL::RunTimeSettings	runtime_settings;
		runtime_settings.width = screen_width;
		runtime_settings.height = screen_height;
		runtime_settings.flags = videoModeFlags;

		GL::modifySettings( runtime_settings );
	}
#endif
}

ERet BeginFrame(
	const NwCameraView& scene_view
	, const RrGlobalSettings& renderer_settings
	, const NwTime& game_time
)
{
	tbPROFILE_FUNCTION;

	RenderSystemData& data = *Globals::g_data;

	mxTODO("resizing!");
	//_resizeViewportIfNeeded(
	//	scene_view.viewportWidth, scene_view.viewportHeight
	//	, renderer_settings
	//	);


	// update per-frame and per-camera constant buffers

	_updatePerFrameShaderConstants(
		data
		, renderer_settings
		, game_time
		);

#if nwRENDERER_CFG_ENABLE_idTech4
	//
	_id_material_system.Update(
		game_time
		);
#endif




	Globals::_UpdatePerViewShaderConstants(
		data
		, scene_view
		, renderer_settings
		);

	return ALL_OK;
}










const RenderPath& GetRenderPath()
{
	return *g_data->_deferred_renderer.m_renderPath;
}

TbPrimitiveBatcher& GetImmediateDebugRenderer()
{
	return g_data->_debug_renderer;
}
TbDebugLineRenderer& GetRetainedDebugView()
{
	return g_data->_debug_line_renderer;
}

}//namespace Globals





ERet RegisterCallbackForRenderingEntitiesOfType(
							const ERenderEntity entity_type
							, RenderCallback* render_callback
							, void* user_data
							)
{
	RenderSystemData& data = *Globals::g_data;

	mxENSURE(
		nil == data._render_callbacks.code[ entity_type ],
		ERR_DUPLICATE_OBJECT,
		""
		);

	data._render_callbacks.code[ entity_type ] = render_callback;
	data._render_callbacks.data[ entity_type ] = user_data;

	return ALL_OK;
}

FrontEndStats	g_FrontEndStats;

}//namespace Rendering
