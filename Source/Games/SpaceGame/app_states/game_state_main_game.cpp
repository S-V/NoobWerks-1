
//
#include "stdafx.h"
#pragma hdrstop

#include <gainput/gainput.h>

#include <Base/Memory/Allocators/FixedBufferAllocator.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Engine/WindowsDriver.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include <Core/Serialization/Text/TxTSerializers.h>
//
#include <Voxels/public/vx5.h>
#include <Voxels/private/Scene/vx5_octree_world.h>
#include <voxels/private/vxp_voxel_engine.h>
//
#include "app_states/app_states.h"
#include "game_app.h"


/*
-----------------------------------------------------------------------------
	SgState_MainGame
-----------------------------------------------------------------------------
*/
SgState_MainGame::SgState_MainGame()
{
	this->DbgSetName("MainMode");
}

EStateRunResult SgState_MainGame::handleInput(
	NwAppI* game_app
	, const NwTime& game_time
)
{
	SgGameApp& game = *checked_cast< SgGameApp* >( game_app );

	const gainput::InputMap& input_map = *game.input_mgr._input_map;

	//
	const gainput::InputState& keyboard_input_state =
		*game.input_mgr._keyboard->GetInputState()
		;

	//
	if( game.input_mgr.wasButtonDown( eGA_Exit ) )
	{
		SgGameplayMgr& gameplay = game.gameplay;

		if(gameplay.spaceship_controller.IsControllingSpaceship())
		{
			//
			{
				const SgFollowingCamera& orbit_camera = gameplay.spaceship_controller._following_camera;
				NwFlyingCamera &free_camera = gameplay.player.camera;

				free_camera._eye_position = V3d::fromXYZ(orbit_camera.cam_eye_pos);
				free_camera.m_upDirection = orbit_camera.cam_up_dir;
				free_camera.m_look_direction = orbit_camera.cam_look_dir;
				free_camera.m_rightDirection = orbit_camera.cam_right_dir;

				free_camera.m_movementFlags = 0;
				free_camera.m_linearVelocity = CV3f(0);
				free_camera.m_linAcceleration = CV3f(0);
			}

			gameplay.spaceship_controller.RelinquishControlOfSpaceship();
		}
		else
		{
			const NwGameStateI* top_state = game.state_mgr.getTopState();

			//game.state_mgr.pushState(
			//	&game.states->in_game_ui
			//	);

			if(top_state != &game.states->u_died_game_over)
			{
				game.state_mgr.pushState(
					&game.states->escape_menu
					);
			}
		}

		return StopFurtherProcessing;
	}

	//
	if(game.gameplay.IsGameOverMissionFailed()
	&& game.state_mgr.getTopState() != &game.states->u_died_game_over
	&& !game.states->u_died_game_over.player_wants_to_continue_running_the_game_despite_mission_failure
	)
	{
		game.gameplay.player.StopCameraMovement();

		game.state_mgr.pushState(
			&game.states->u_died_game_over
			);
		return StopFurtherProcessing;
	}




	//
	if(game.input_mgr.wasButtonDown( eGA_HighlightVisibleShips ))
	{
		game.renderer.should_highlight_visible_ships ^= 1;
	}

	//
	if(game.input_mgr.wasButtonDown( eGA_PauseGame ))
	{
		game.is_paused ^= 1;
	}




	//
//#if !GAME_CFG_RELEASE_BUILD
	if( game.input_mgr.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game.state_mgr.pushState(
			&game.states->state_debug_hud
			);
		return StopFurtherProcessing;
	}
//#endif // !GAME_CFG_RELEASE_BUILD


	//
	const NwGameStateI* topmost_state = game.state_mgr.getTopState();
	const bool this_is_topmost_state = this == topmost_state;

	if( this_is_topmost_state )
	{
		_HandlePlayerInputForControllingCamera(
			game
			, game_time
			, input_map
			, keyboard_input_state
			);
	}

	////
	//if( topmost_state->AllowPlayerToReceiveInput() )
	//{
	//	_handleInput_for_Controlling_Player_Weapons(
	//		game
	//		, game_time
	//		, input_map
	//		, keyboard_input_state
	//		);
	//}

#if !GAME_CFG_RELEASE_BUILD
	//
	if( game.input_mgr.wasButtonDown( eGA_DBG_Toggle_Wireframe ) )
	{
		game.settings.renderer.drawModelWireframes ^= 1;
	}
	//
	if( game.input_mgr.wasButtonDown( eGA_DBG_Toggle_Torchlight ) )
	{
		game.settings.player_torchlight_params.enabled ^= 1;
	}



	//
	if( game.input_mgr.wasButtonDown( eGA_DBG_Save_Game_State ) )
	{
		game.saveSettingsToFiles();
	}
	//
	if( game.input_mgr.wasButtonDown( eGA_DBG_Load_Game_State ) )
	{
		game.loadSettingsFromFiles();

		// reset debug acceleration
		//game.gameplay.player.num_seconds_shift_is_held = 0;
	}

#endif // !GAME_CFG_RELEASE_BUILD





	//
	if(!this_is_topmost_state) {
		return ContinueFurtherProcessing;
	}





	//
	const InputState& input_state = NwInputSystemI::Get().getState();


	if(
		!game.gameplay.spaceship_controller.IsControllingSpaceship()
		&&
		input_state.mouse.held_buttons_mask
		)
	{
		if(game.gameplay.h_spaceship_under_crosshair.isValid())
		{
			game.gameplay.HandleSpaceshipUnderCrosshair();
		}
	}

	//
	return ContinueFurtherProcessing;
}

void SgState_MainGame::_handleInput_for_FirstPersonPlayerCamera(
	SgGameApp* app
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	)
{
	const NwGameStateI* topmost_state = app->state_mgr.getTopState();
	const bool this_is_topmost_state = this == topmost_state;

	//
	NwFlyingCamera &camera = app->gameplay.player.camera;

	if(app->gameplay.spaceship_controller.IsControllingSpaceship())
	{
		const InputState& input_state = NwInputSystemI::Get().getState();
		float yaw_delta = input_state.mouse.delta_x * app->user_settings.ingame.controls.mouse_sensitivity;
		float pitch_delta = input_state.mouse.delta_y * app->user_settings.ingame.controls.mouse_sensitivity;

		if(app->user_settings.ingame.controls.invert_mouse_x) {
			yaw_delta *= -1;
		}
		if(app->user_settings.ingame.controls.invert_mouse_y) {
			pitch_delta *= -1;
		}

		//
		app->gameplay.spaceship_controller.ProcessPlayerInput(
			game_time
			, input_map
			, yaw_delta
			, pitch_delta
			);
	}
	else
	{
		FlyingCamera_handleInput_for_FirstPerson(
			camera
			, game_time
			, input_map
			, keyboard_input_state
			, app->settings.cheats
			, app->user_settings.ingame.controls
			);
	}




	//
	const bool is_CTRL_key_held = (
		keyboard_input_state.GetBool( gainput::KeyCtrlL )
		||
		keyboard_input_state.GetBool( gainput::KeyCtrlR )
		);

	const bool is_shift_key_held = (
		keyboard_input_state.GetBool( gainput::KeyShiftL )
		||
		keyboard_input_state.GetBool( gainput::KeyShiftR )
		);

	// If the SHIFT key is held and the camera is moving
	if( is_shift_key_held &&& camera.m_movementFlags )
	{
		//app->gameplay.player.num_seconds_shift_is_held += game_time.real.delta_seconds;

		// increase the speed twice for every second the SHIFT key is held
		app->gameplay.player.camera._dbg_speed_multiplier = powf(
			4.0f,
			1.0f// + app->gameplay.player.num_seconds_shift_is_held
			);
	}
	else
	{
		// decrease the speed faster if the acceleration key is not held
		//app->gameplay.player.num_seconds_shift_is_held -= game_time.real.delta_seconds * 2.0f;

		//app->gameplay.player.num_seconds_shift_is_held = maxf(
		//	app->gameplay.player.num_seconds_shift_is_held,
		//	0
		//	);

		app->gameplay.player.camera._dbg_speed_multiplier = 1;
	}


#if !GAME_CFG_RELEASE_BUILD && MX_DEVELOPER
	//
	if(keyboard_input_state.GetBool( gainput::KeyT ))
	{
		app->gameplay.DbgSpawnFriendlyShip(
			app->renderer.scene_view.eye_pos
			);
	}
	if(keyboard_input_state.GetBool( gainput::KeyY ))
	{
		app->gameplay.DbgSpawnEnemyShip(
			app->renderer.scene_view.eye_pos
			);
	}
#endif // #if !GAME_CFG_RELEASE_BUILD


#if !GAME_CFG_RELEASE_BUILD && MX_DEVELOPER

	//
	if( this_is_topmost_state
		&&
		is_CTRL_key_held
		&&
		keyboard_input_state.GetBool( gainput::KeyC )
		)
	{
		DBGOUT("Copying the Render Camera's position to the Debug 3rd-Person Camera position");
		app->settings.dbg_3rd_person_camera_position = app->gameplay.player.camera._eye_position;
	}

#endif // MX_DEVELOPER

}

void SgState_MainGame::_processInput_DebugTerrainCamera(
	//const gainput::InputState& keyboard_input_state
	const gainput::InputMap& input_map
	, SgGameApp& game
	)
{
	SgAppSettings & game_settings = game.settings;

	const double camera_speed = game_settings.dbg_3rd_person_camera_speed;

	V3d	delta_movement = CV3d(0);

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Left ) )
	{
		delta_movement.x -= camera_speed;
	}

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Right ) )
	{
		delta_movement.x += camera_speed;
	}

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Forward ) )
	{
		delta_movement.y += camera_speed;
	}

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Backwards ) )
	{
		delta_movement.y -= camera_speed;
	}

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Up ) )
	{
		delta_movement.z += camera_speed;
	}

	//
	if( input_map.GetBool( eGA_DebugTerrainCamera_Move_Down ) )
	{
		delta_movement.z -= camera_speed;
	}

	game_settings.dbg_3rd_person_camera_position += delta_movement;
}

void SgState_MainGame::Update(
							  const NwTime& game_time
							  , NwAppI* game
							  )
{
	//SgGameApp& game_app = SgGameApp::Get();
	//SgGameplayMgr& gameplay = game_app.gameplay;
}

EStateRunResult SgState_MainGame::DrawScene(
	NwAppI* p_game_app
	, const NwTime& game_time
	)
{
	SgGameApp& game_app = SgGameApp::Get();

	SgWorld& world = game_app.world;
	const SgAppSettings& game_settings = game_app.settings;
	const SgHumanPlayer& player = game_app.gameplay.player;

	SgRenderer& game_renderer = game_app.renderer;

	game_renderer.RenderFrame(
		world
		, game_time
		, game_app.gameplay
		, game_app.physics_mgr
		, game_settings
		, game_app.user_settings
		, player
		);

	return ContinueFurtherProcessing;
}

void SgState_MainGame::_HandlePlayerInputForControllingCamera(
	SgGameApp& game
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	)
{
	_handleInput_for_FirstPersonPlayerCamera(
		&game
		, game_time
		, input_map
		, keyboard_input_state
		);
}


struct NkTextUtil
{
	static void drawTextLine(
		//nk_command_buffer * cmd_buffer
		nk_context* ctx
		, const struct nk_rect& rect
		, const String& text
		, const int line
		)
	{
		const nk_color white = {~0,~0,~0,~0};

		struct nk_rect	text_rect = rect;
		text_rect.y += line * ctx->style.font->height;

		nk_draw_text(
			nk_window_get_canvas( ctx )
			, text_rect
			, text.c_str()
			, text.length()
			, ctx->style.font
			, white
			, white
		);
	}
};

void SgState_MainGame::DrawUI(
								   const NwTime& game_time
								   , NwAppI* game_app
								   )
{
	SgGameApp* game = checked_cast< SgGameApp* >( game_app );

	const UInt2 window_size = WindowsDriver::getWindowSize();
	const float screen_width = window_size.x;
	const float screen_height = window_size.y;

	//
	nk_context* ctx = Nuklear_getContext();

	const U32 num_resources = 9997;
	String128	resources_string;
	Str::Format( resources_string, "Resources: %u", num_resources );

	// BEGIN transparent window
	const nk_style_item prev_style_item = ctx->style.window.fixed_background;
	ctx->style.window.fixed_background = nk_style_item_hide();


	//
	if( nk_begin_titled(
		ctx
		, "GAME_WINDOW_TITLE__MAIN"
		, ""
		, nk_rect( 0, 0, screen_width, screen_height )
		, NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
		) )
	{


#if 0//GAME_CFG_SHOW_Sg_GAME_UI_CONTROLS

		// Menu Bar.
		{
			float spacing_x = ctx->style.window.spacing.x;

			/* output path directory selector in the menubar */
			ctx->style.window.spacing.x = 0;
			nk_menubar_begin(ctx);
			{

				//
				nk_layout_row_dynamic(ctx, 25, 6);

				if( nk_button_label( ctx, nwLOCALIZE("Construction Center") ) )
				{
					game->state_mgr.pushState(
						&game->states->state_construction_center
						);
				}

				if( nk_button_label( ctx, resources_string.c_str() ) ) {
					DBGOUT("Resources Tapped!");
				}



				//
				// TODO: Tabs for Construction Center, Research, Units, etc.?

			}
			nk_menubar_end(ctx);
			ctx->style.window.spacing.x = spacing_x;
		}
#endif


	}//

	//
	_DrawInGameGUI(
		*game
		, window_size
		, ctx
		);

	//
#if !GAME_CFG_RELEASE_BUILD

	if( game->settings.ui_show_debug_text )
	{
		_DrawDebugHUD_ImGui(*game);

		_DrawDebugHUD(
			*game
			, window_size
			, ctx
			);
	}

#endif


	// END transparent window
	nk_end(ctx);

	//
	ctx->style.window.fixed_background = prev_style_item;
}

void SgState_MainGame::_DrawDebugHUD_ImGui(
	const SgGameApp& game
	)
{

#if MX_DEVELOPER


#endif // #if MX_DEVELOPER

}

void SgState_MainGame::_DrawDebugHUD(
	const SgGameApp& game
	, const UInt2 window_size
	, nk_context* ctx
	)
{
	if( !game.settings.ui_show_debug_text ) {
		return;
	}

	//
	const NwFlyingCamera& player_camera = game.gameplay.player.camera;


	const float screen_width = window_size.x;
	const float screen_height = window_size.y;



	//
	String256	text_buf;

	const struct nk_rect rect = nk_rect(
		10, screen_height*0.1f,
		screen_width, screen_height
		);

	int row = 0;

	//
	Str::Format(text_buf,
		"Cam.Pos World: (%f, %f, %f)",
		player_camera._eye_position.x, player_camera._eye_position.y, player_camera._eye_position.z
		);
	NkTextUtil::drawTextLine(
		ctx
		, rect
		, text_buf
		, row++
		);

	//
	const NwFloatingOrigin floating_origin = game.world.spatial_database->getFloatingOrigin();

	Str::Format(text_buf,
		"Flt.Org World: (%f, %f, %f)",
		floating_origin.center_in_world_space.x,
		floating_origin.center_in_world_space.y,
		floating_origin.center_in_world_space.z
		);
	NkTextUtil::drawTextLine(
		ctx
		, rect
		, text_buf
		, row++
		);

	//
	const NwCameraView& camera_view = game.renderer.scene_view;

	Str::Format(text_buf,
		"Cam.Pos Rel (renderer.cam_view): (%f, %f, %f)",
		camera_view.eye_pos.x, camera_view.eye_pos.y, camera_view.eye_pos.z
		);
	NkTextUtil::drawTextLine(
		ctx
		, rect
		, text_buf
		, row++
		);

	//
	Str::Format(text_buf,
		"Cam.LookDir: (%f, %f, %f)",
		player_camera.m_look_direction.x, player_camera.m_look_direction.y, player_camera.m_look_direction.z
		);
	NkTextUtil::drawTextLine(
		ctx
		, rect
		, text_buf
		, row++
		);

	//
	Str::Format(text_buf,
		"Dist. from Dbg.Cam: %f",
		(player_camera._eye_position - game.settings.dbg_3rd_person_camera_position).length()
	);
	NkTextUtil::drawTextLine(
		ctx
		, rect
		, text_buf
		, row++
		);

#if 0
	//
	{
		Str::Format(text_buf, "=== World:");
		Util::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);

		Str::Format(text_buf,
			"Num LoDs: %u, Chunk Size at LoD 0: %f",
			game.world.voxel_engine->,
			?
			);
		Util::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}
#endif

	//
	const SgRendererStats& renderer_stats = game.renderer.stats;
	{
		Str::Format(text_buf, "=== Renderer:");
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}
	{
		//
		Str::Format(text_buf,
			"Avg Update frame time: %f msec (Avg FPS %f)",
			game._update_FPS_counter.average() * 1000.0f,
			1.0f / ( game._update_FPS_counter.average() + 1e-5f )
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);

		//
		Str::Format(text_buf,
			"Avg Render frame time: %f msec (Avg FPS %f)",
			game._render_FPS_counter.average() * 1000.0f,
			1.0f / ( game._render_FPS_counter.average() + 1e-5f )
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}
	//
	{
		const NwGeometryBufferPoolStats& geom_buffer_pool_stats = NwGeometryBufferPool::Get().getStats();

		Str::Format(text_buf,
			"Geom Buffers: Created: %u, Used: %u, Mem Total: %u, Mem Wasted: %u"
			, geom_buffer_pool_stats.num_created_buffers_in_total
			, geom_buffer_pool_stats.num_buffers_used_right_now
			, geom_buffer_pool_stats.total_buffer_bytes_allocated
			, geom_buffer_pool_stats.num_bytes_wasted_due_to_padding
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}

	//
	{
		Str::Format(text_buf,
			"Visible Objects: %u / %u"
			, renderer_stats.num_objects_visible
			, renderer_stats.num_objects_total
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}

	////
	//{
	//	const NwGeometryBufferPoolStats& geom_buffer_pool_stats = NwGeometryBufferPool::Get().GetStats();

	//	Str::Format(text_buf,
	//		"Smoke Puffs: %u"
	//		, game.renderer.particle_renderer.smoke_puffs.num()
	//		);
	//	NkTextUtil::drawTextLine(
	//		ctx
	//		, rect
	//		, text_buf
	//		, row++
	//		);
	//}


	//
	{
		Str::Format(text_buf, "=== GPU:");
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}
	//
	{
		GL::FrontEndStats	gfx_front_end_stats;
		GL::getFrontEndStats( &gfx_front_end_stats );

		GL::BackEndCounters	graphics_perf_counters;
		GL::getLastFrameCounters( &graphics_perf_counters );


		//
		Str::Format(text_buf,
			"Draw Calls: %u, Tris: %u, Verts: %u, Time: %f msec"
			, graphics_perf_counters.c_draw_calls
			, graphics_perf_counters.c_triangles
			, graphics_perf_counters.c_vertices
			, graphics_perf_counters.gpu.GPU_Time_Milliseconds
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);

		//
		Str::Format(text_buf,
			"Created: Buffers: %u, Textures: %u, Shaders: %u, Programs: %u"
			, gfx_front_end_stats.num_created_buffers
			, gfx_front_end_stats.num_created_textures
			, gfx_front_end_stats.num_created_shaders
			, gfx_front_end_stats.num_created_programs
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}


#if GAME_CFG_WITH_VOXELS
	// don't show voxels debug menu
	if(!GAME_CFG_RELEASE_BUILD)
	{
		Str::Format(text_buf, "=== Voxels:");
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);

		VX5::OctreeWorld & octree_world = *((VX5::OctreeWorld*) game.world.voxels.voxel_world._ptr);

		//
		Str::Format(text_buf,
			"num_scheduled_jobs_high_watermark: %u"
			, octree_world._engine._stats.num_scheduled_jobs_high_watermark
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);


		//
		Str::Format(text_buf,
			"[VX] Cache: mem used: %u KiB, writes: %u, hash table overflows: %u, ring buffer overflows: %u, requests: %u, misses: %u, hits: %u"
			, octree_world._engine._stats.proc_gen_cache.memory_used_KiB
			, octree_world._engine._stats.proc_gen_cache.num_writes

			, octree_world._engine._stats.proc_gen_cache.evicted_due_to_hashtable_overflow
			, octree_world._engine._stats.proc_gen_cache.evicted_due_to_ringbuffer_overflow

			, octree_world._engine._stats.proc_gen_cache.num_requests
			, octree_world._engine._stats.proc_gen_cache.num_misses
			, octree_world._engine._stats.proc_gen_cache.num_hits
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);


		//
		VX5::VoxelEngineStats	voxel_engine_stats;
		game.world.voxels.voxel_engine->GetStats( &voxel_engine_stats );

		//
		Str::Format(text_buf,
			"[VX] Stats: num_scheduled_jobs_high_watermark=%d"
			, voxel_engine_stats.num_scheduled_jobs_high_watermark
			//"Skipped: %u Empty, %u Solid; Processed: %u"
			//, AtomicLoad( voxel_engine_stats.generated_chunks_skipped_as_empty )
			//, AtomicLoad( voxel_engine_stats.generated_chunks_skipped_as_filled )
			//, AtomicLoad( voxel_engine_stats.generated_chunks_not_skipped )
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}

#endif // #if GAME_CFG_WITH_VOXELS

}

void SgState_MainGame::_DrawInGameGUI(
	const SgGameApp& game
	, const UInt2 window_size
	, nk_context* ctx
	)
{
	const float screen_width = window_size.x;
	const float screen_height = window_size.y;


	//
	String256	text_buf;


	{
		int row = 0;

		// Draw "Paused" text in upper right corner, if needed.
		if(game.is_paused)
		{
			Str::Format(text_buf,
				"PAUSED"
				);

			const struct nk_rect rect = nk_rect(
				screen_width*0.92f, screen_height*0.01f,
				screen_width, screen_height
				);

			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row
				);
		}

		row++;




		//
		const SgGameplayDisplayedStats displayedGameplayStats = game.gameplay.GetDisplayedStats();

		const struct nk_rect rect = nk_rect(
			screen_width*0.8f, screen_height*0.02f,
			screen_width, screen_height
			);

		//
		{
			Str::Format(text_buf,
				"Enemy ships: %d",
				displayedGameplayStats.num_enemy_ships
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);
		}

		//
		{
			Str::Format(text_buf,
				"Friendly ships: %d",
				displayedGameplayStats.num_friendly_ships
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);
		}

		if(displayedGameplayStats.mothership_health > 0)
		{
			Str::Format(text_buf,
				"Mothership health: %d",
				displayedGameplayStats.mothership_health
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);
		}
	}




	{
		const struct nk_rect rect = nk_rect(
			10, screen_height*0.01f,
			screen_width, screen_height
			);

		int row = 0;

		//
		const float average_frame_time_seconds = game._render_FPS_counter.average();

		//
		Str::Format(text_buf,
			"FPS %d (%.2f msec)",
			int(1.0f / ( average_frame_time_seconds + 1e-5f )),
			average_frame_time_seconds * 1000.0f
			);
		NkTextUtil::drawTextLine(
			ctx
			, rect
			, text_buf
			, row++
			);
	}
}
