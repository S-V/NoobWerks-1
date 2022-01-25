#include "stdafx.h"
#pragma hdrstop

#if MX_DEVELOPER
#include <Core/Util/Tweakable.h>
#endif // MX_EDITOR

#include <Engine/WindowsDriver.h>	// InputState
#include <Engine/Engine.h>	// NEngine::LaunchConfig 
#include <Engine/Private/EngineGlobals.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Public/Scene/SceneRenderer.h>
mxREFACTOR(":")
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/Modules/Sky/SkyModel.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>

#include "app.h"
#include "app_settings.h"
#include "world/game_world.h"
#include "game_player.h"
#include "rendering/renderer.h"
#include "rendering/renderer_data.h"


namespace
{
	static ProxyAllocator& rendererHeap() { return MemoryHeaps::renderer(); }
}

/*
-----------------------------------------------------------------------------
	MyGameRenderer
-----------------------------------------------------------------------------
*/
MyGameRenderer::MyGameRenderer()
{
}

ERet MyGameRenderer::Initialize(
								 const NEngine::LaunchConfig & engine_settings
								 , const Rendering::RrGlobalSettings& renderer_settings
								 , NwClump* scene_clump
								 )
{
	AllocatorI& allocator = rendererHeap();
	//
	mxDO(nwCREATE_OBJECT(
		_data
		, allocator
		, MyGameRendererData
		));


	_data->_scene_clump = scene_clump;

	//
	mxDO(Rendering::Globals::Initialize(
		engine_settings.window.width, engine_settings.window.height
		, renderer_settings
		, scene_clump
		));


	//
#if GAME_CFG_WITH_VOXEL_GI
	mxDO(_data->voxel_grids.Initialize( renderer_settings.vxgi ));
	mxDO(_data->irradiance_field.Initialize( renderer_settings.ddgi ));
#endif

	//
	_data->_gizmo_renderer.BuildGeometry();


	return ALL_OK;
}

void MyGameRenderer::Shutdown()
{
	AllocatorI& allocator = rendererHeap();

	//
#if GAME_CFG_WITH_VOXEL_GI
	_data->irradiance_field.Shutdown();
	_data->voxel_grids.Shutdown();
#endif

	//
	Rendering::Globals::Shutdown();

	//
	_data->_scene_clump = nil;

	//
	nwDESTROY_OBJECT(_data._ptr, allocator);
	_data = nil;
}

void MyGameRenderer::ApplySettings( const Rendering::RrGlobalSettings& new_settings )
{
	Rendering::Globals::ApplySettings( new_settings );

#if GAME_CFG_WITH_VOXEL_GI
	_data->voxel_grids.ApplySettings( new_settings.vxgi );
#endif
}

const Rendering::NwCameraView& MyGameRenderer::GetCameraView() const
{
	return _data->scene_view;
}

Rendering::NwCameraView& MyGameRenderer::GetCameraViewNonConst()
{
	return _data->scene_view;
}

void MyGameRenderer::BeginFrame(
	const NwTime& game_time
	, const MyWorld& world
	, const MyAppSettings& app_settings
	, const MyGameUserSettings& user_settings
	, const MyGamePlayer& player
	)
{
	//
	//const Rendering::NwFloatingOrigin floating_origin = world.spatial_database->getFloatingOrigin();

	//
	Rendering::NwCameraView & scene_view = _data->scene_view;
	{
#if 0
		scene_view.eye_pos = floating_origin.relative_eye_position;
		scene_view.eye_pos_in_world_space = player.camera._eye_position;
#else
		//scene_view.eye_pos_in_world_space = player.camera._eye_position;
		scene_view.eye_pos_in_world_space = V3d::fromXYZ(player.GetEyePosition());

		scene_view.eye_pos = player.GetEyePosition();
#endif

		const NwFlyingCamera& camera = player.camera;
		scene_view.right_dir = camera.m_rightDirection;
		scene_view.look_dir = camera.m_look_direction;

		// repair near plane
//		scene_view.near_clip = maxf( NEngine::g_settings.rendering.camera_near_clip, MIN_NEAR_CLIP_PLANE_DIST );

		//
		scene_view.near_clip = NEngine::g_settings.rendering.camera_near_clip;
		scene_view.far_clip = NEngine::g_settings.rendering.camera_far_clip;

		//
		const UInt2 window_size = WindowsDriver::getWindowSize();
		scene_view.screen_width = window_size.x;
		scene_view.screen_height = window_size.y;
	}
	scene_view.recomputeDerivedMatrices();

	//
	Rendering::Globals::BeginFrame(
		scene_view
		, NEngine::g_settings.rendering
		, game_time
		);

	//
	_PrepareDebugLineRenderer();
}

ERet MyGameRenderer::_PrepareDebugLineRenderer()
{
	const Rendering::NwCameraView& scene_view = _data->scene_view;

	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

	//
	const U32 passIndex = Rendering::Globals::GetRenderPath()
		.getPassDrawingOrder(mxHASH_STR("DebugLines"));

	NwShaderEffect* shaderTech;
	mxDO(Resources::Load(shaderTech, MakeAssetID("debug_lines"), _data->_scene_clump));
	Rendering::Globals::GetImmediateDebugRenderer().setTechnique(shaderTech);

	{
		NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			NGpu::buildSortKey( passIndex, 0 )	// first
			nwDBG_CMD_SRCFILE_STRING
			);


		M44f *	view_projection_matrixT;
		mxDO(shaderTech->BindCBufferData(
			&view_projection_matrixT
			, render_context._command_buffer
			));
		*view_projection_matrixT = M44_Transpose(scene_view.view_projection_matrix);


		mxDO(shaderTech->pushShaderParameters( render_context._command_buffer ));
	}

	Rendering::Globals::GetImmediateDebugRenderer().SetViewID( passIndex );

	return ALL_OK;
}

ERet MyGameRenderer::RenderFrame(
	const MyWorld& world
	, const NwTime& game_time
	, const MyAppSettings& app_settings
	, const MyGameUserSettings& user_settings
	, const MyGamePlayer& player
	)
{
	const Rendering::NwCameraView& scene_view = _data->scene_view;
	//
	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

	//
	_DrawWorld(
		world
		, scene_view
		, game_time
		, app_settings
		, render_context
		);

	//player.RenderFirstPersonView(
	//	_data->scene_view
	//	, render_context
	//	, game_time
	//	, world.spatial_database
	//	);

	////
	//if( app_settings.player_torchlight_params.enabled )
	//{
	//	TbDynamicPointLight	test_light;
	//	test_light.position = V3f::fromXYZ(player.camera._eye_position); //region_of_interest_position;
	//	test_light.radius = app_settings.player_torchlight_params.radius;
	//	test_light.color = app_settings.player_torchlight_params.color;
	//	world.spatial_database->AddDynamicPointLight(test_light);
	//}

	////

	_BeginDebugLineDrawing(scene_view, render_context);


#if !GAME_CFG_RELEASE_BUILD

	DebugDrawWorld(
		world
		, world.spatial_database
		, scene_view
		, player
		, app_settings
		, NEngine::g_settings.rendering
		, game_time
		, render_context
		);

#endif // !GAME_CFG_RELEASE_BUILD

	//
	_FinishDebugLineDrawing();

	return ALL_OK;
}

void MyGameRenderer::_DrawWorld(
	const MyWorld& world
	, const Rendering::NwCameraView& scene_view
	, const NwTime& game_time
	, const MyAppSettings& app_settings
	, NGpu::NwRenderContext & render_context
	)
{
// Start visibility culling jobs. While they are running, we can do other tasks.

//Task<> find_visible_objects_task = Tasking::findVisibleObjects( world, scene_view );

//Tasking::waitFor( find_visible_objects_task );


	// Find potentially visible objects.

	Rendering::RenderEntityList	visible_entities(MemoryHeaps::temporary());

	{
		const ViewFrustum	view_frustum = scene_view.BuildViewFrustum();
		mxOPTIMIZE("jobify");
		(*world.spatial_database).GetEntitiesIntersectingViewFrustum(
			visible_entities
			, view_frustum
			);
	}

	//
	Rendering::SubmitEntities(
		scene_view
		, visible_entities
		, NEngine::g_settings.rendering
		);

	Rendering::SubmitDirectionalLight(
		app_settings.sun_light
		, scene_view
		, NEngine::g_settings.rendering
		, render_context

		, &_data->voxel_grids
		, &_data->irradiance_field
		);

	
	if(app_settings.draw_skybox) {
		_Submit_Skybox();
	}

	//
#if GAME_CFG_WITH_VOXEL_GI

	//
	_data->voxel_grids.voxel_BVH->UpdateGpuBvhIfNeeded(
		scene_view
		, *_data->voxel_grids.brickmap
		, render_context
		);

	_data->voxel_grids.UpdateAndReVoxelizeDirtyCascades(
		*world.spatial_database
		, scene_view
		, render_context
		, NEngine::g_settings.rendering
		);

	_data->voxel_grids.DebugDrawIfNeeded(
		scene_view
		, render_context
		);

	//
	_data->irradiance_field.UpdateProbes(
		scene_view
		, _data->voxel_grids
		, game_time
		, render_context
		);

	_data->irradiance_field.DebugDrawIfNeeded(
		scene_view
		, render_context
		);

	if(app_settings.dbg_show_bvh)
	{
		if(0)
		{
			_data->voxel_grids.voxel_BVH->DebugDrawLeafNodes(
				scene_view
				, render_context
				);
		}
		else
		{
			_data->voxel_grids.voxel_BVH->DebugDrawBVH(
				scene_view
				, render_context
				, app_settings.dbg_max_bvh_depth_to_show
				);
		}

		_data->voxel_grids.voxel_BVH->DebugRayTraceBVH(
			scene_view
			, NGpu::AsOutput(Rendering::Globals::g_data->_deferred_renderer.m_litTexture->handle)
			, UInt2(
				Rendering::Globals::g_data->_screen_width,
				Rendering::Globals::g_data->_screen_height
			)
			, *_data->voxel_grids.brickmap
			, render_context
			);
	}
#endif

}

ERet MyGameRenderer::_BeginDebugLineDrawing(
	const Rendering::NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	)
{
	_data->_is_drawing_debug_lines = true;
	
	return ALL_OK;
}

void MyGameRenderer::_FinishDebugLineDrawing()
{
	Rendering::Globals::GetRetainedDebugView().flush(
		Rendering::Globals::GetImmediateDebugRenderer()
		);
	Rendering::Globals::GetImmediateDebugRenderer().Flush();
	_data->_is_drawing_debug_lines = false;
}

ERet MyGameRenderer::DebugDrawWorld(
									  const MyWorld& game_world
									  , const Rendering::SpatialDatabaseI* spatial_database
									  , const Rendering::NwCameraView& scene_view
									  , const MyGamePlayer& game_player
									  , const MyAppSettings& app_settings
									  , const Rendering::RrGlobalSettings& renderer_settings
									  , const NwTime& game_time
									  , NGpu::NwRenderContext & render_context
									  )
{
	mxASSERT(_data->_is_drawing_debug_lines);


	//
#if 0
	game_player.Dbg_RenderFirstPersonView(
		scene_view
		, *this
		);
#endif

#if !GAME_CFG_RELEASE_BUILD
	// Crosshair
	if( app_settings.drawCrosshair )
	{
		Rendering::Globals::GetImmediateDebugRenderer().DrawSolidCircle(
			scene_view.eye_pos + scene_view.look_dir,
			scene_view.right_dir,
			scene_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
	}

	if( app_settings.player_torchlight_params.enabled )
	{
		Rendering::SubmitTransientPointLight(
			scene_view.eye_pos //region_of_interest_position;
			, app_settings.player_torchlight_params.radius
			, app_settings.player_torchlight_params.color
			);
	}

#endif

	//
	if( app_settings.drawGizmo )
	{
		const Rendering::NwFloatingOrigin floating_origin =
			Rendering::NwFloatingOrigin::GetDummy_TEMP_TODO_REMOVE()
			//spatial_database->getFloatingOrigin()
			;


		// Draws a red point at the center of the world.
#if 0
		Rendering::Globals::GetImmediateDebugRenderer().DrawSolidCircle(
			-V3f::fromXYZ(floating_origin.position_in_world_space),
			scene_view.right_dir,
			scene_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
#endif


		// draw axes at the world origin

		DrawGizmo(
			_data->_gizmo_renderer
			, M44_BuildTS(
				-V3f::fromXYZ(floating_origin.center_in_world_space)// - scene_view.eye_pos
				, 1
			)
			, scene_view.eye_pos
			, Rendering::Globals::GetImmediateDebugRenderer()
			);
	}

	//
	if( app_settings.dbg_use_3rd_person_camera_for_terrain_RoI )
	{
		//
		const Rendering::NwFloatingOrigin floating_origin =
			Rendering::NwFloatingOrigin::GetDummy_TEMP_TODO_REMOVE()
			//spatial_database->getFloatingOrigin()
			;

		double radius = sqrt(( game_player.camera._eye_position - app_settings.dbg_3rd_person_camera_position ).lengthSquared()) * 0.03;

		Rendering::Globals::GetImmediateDebugRenderer().DrawSolidSphere(
			floating_origin.getRelativePosition( app_settings.dbg_3rd_person_camera_position )
			, radius
			, RGBAf::RED
			);
	}


#if GAME_CFG_WITH_PHYSICS
	if( app_settings.physics.debug_draw )
	{
		// VERY SLOW!!!
		btDynamicsWorld& bt_world = (btDynamicsWorld&) game_world.physics_world.m_dynamicsWorld;
		bt_world.debugDrawWorld();
		//dbg_drawRigidBodyFrames(_physics_world, Rendering::Globals::m_debugRenderer);
	}
#endif // GAME_CFG_WITH_PHYSICS

	return ALL_OK;
}

ERet MyGameRenderer::_Submit_Skybox()
{
	NGpu::NwRenderContext& render_context = NGpu::getMainRenderContext();

	const NGpu::LayerID viewId = Rendering::Globals::GetRenderPath()
		.getPassDrawingOrder(Rendering::ScenePasses::Sky)
		;

	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, MakeAssetID("earth_skybox") ));

	const NwShaderEffect::Pass& pass0 = shader_effect->getPasses()[0];

	//
	NGpu::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		NGpu::buildSortKey( viewId, pass0.default_program_handle, 0 )
			nwDBG_CMD_SRCFILE_STRING
		);

	mxDO(shader_effect->pushShaderParameters( render_context._command_buffer ));

	mxDO(push_FullScreenTriangle(
		render_context
		, pass0.render_state
		, shader_effect
		, pass0.default_program_handle
		));
	//mxDO(Rendering::DrawSkybox_TriStrip_NoVBIB(
	//	render_context
	//	, pass0.render_state
	//	, pass0.default_program_handle
	//));

	return ALL_OK;
}
