
//
#include "stdafx.h"
#pragma hdrstop

#include <gainput/gainput.h>

#include <Base/Memory/Allocators/FixedBufferAllocator.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Engine/WindowsDriver.h>
#include <Engine/Private/EngineGlobals.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include <Core/Serialization/Text/TxTSerializers.h>
//
#include <Voxels/public/vx5.h>
#include <Voxels/private/Scene/vx5_octree_world.h>
#include <voxels/private/vxp_voxel_engine.h>
//
#include "app_states/game_states.h"
#include "app.h"


/*
-----------------------------------------------------------------------------
	RTSGameState_MainGame
-----------------------------------------------------------------------------
*/
RTSGameState_MainGame::RTSGameState_MainGame()
{
	this->DbgSetName("MainMode");

	_game_camera_mode = GameCam_DebugFreeFlight;
}

EStateRunResult RTSGameState_MainGame::handleInput(
	NwAppI* app
	, const NwTime& game_time
)
{
	MyApp* game = checked_cast< MyApp* >( app );

	const gainput::InputMap& input_map = *game->input._input_map;

	//
	const gainput::InputState& keyboard_input_state =
		*game->input._keyboard->GetInputState()
		;

	//
	if( game->input.wasButtonDown( eGA_Exit ) )
	{
		game->state_mgr.pushState(
			&game->states->escape_menu
			);

		//game->state_mgr.pushState(
		//	&game->states->state_do_really_wanna_exit
		//	);
		return StopFurtherProcessing;
	}

	if( game->input.wasButtonDown( eGA_Show_Debug_Console ) )
	{
		game->state_mgr.pushState(
			&game->states->state_debug_console
			);
		return StopFurtherProcessing;
	}

	////
	//if(
	//	game->player.comp_health.IsDead()
	//	&& game->state_mgr.getTopState() != &game->states->u_died_game_over
	//	)
	//{
	//	game->state_mgr.pushState(
	//		&game->states->u_died_game_over
	//		);
	//	return StopFurtherProcessing;
	//}


#if !GAME_CFG_RELEASE_BUILD
	//
	if(game->input.wasButtonDown( eGA_PauseGame ))
	{
		
#if MX_DEVELOPER

		game->is_paused ^= 1;
#else

		game->is_paused = true;
		game->state_mgr.pushState(
			&game->states->game_paused
			);

#endif

	}


if(game->input.wasButtonDown( eGA_DBG_Toggle_SDF_Visualization ))
{
	game->settings.dbg_show_bvh ^= 1;
}
#endif


	//
#if !GAME_CFG_RELEASE_BUILD
	if( game->input.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game->state_mgr.pushState(
			&game->states->state_debug_hud
			);
		return StopFurtherProcessing;
	}
#endif // !GAME_CFG_RELEASE_BUILD

	////
	//if( game->input.wasButtonDown( eGA_Show_Construction_Center ) )
	//{
	//	game->state_mgr.pushState(
	//		&game->states->state_construction_center
	//		);
	//	return StopFurtherProcessing;
	//}

	const NwGameStateI* topmost_state = game->state_mgr.getTopState();
	const bool this_is_topmost_state = this == topmost_state;

	if( topmost_state->allowsFirstPersonCameraToReceiveInput() )
	{
		switch( _game_camera_mode )
		{
		case GameCam_PlayerSpaceship:
			UNDONE;
			break;

		case GameCam_DebugFreeFlight:
			_handleInput_for_FirstPersonPlayerCamera(
				game
				, game_time
				, input_map
				, keyboard_input_state
				);
			break;
		}
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


	const bool LCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlL );
	const bool RCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlR );
	const bool Ctrl_held = LCtrl_held || RCtrl_held;


#if !GAME_CFG_RELEASE_BUILD
	//
	if( game->input.wasButtonDown( eGA_DBG_Toggle_Wireframe ) )
	{
		NEngine::g_settings.rendering.debug.flags.raw_value ^= Rendering::DebugRenderFlags::DrawWireframe;
		DBGOUT("Wireframe -> %d",
			NEngine::g_settings.rendering.debug.flags.raw_value
			);
	}
	//
	if( game->input.wasButtonDown( eGA_DBG_Toggle_Torchlight ) )
	{
		game->settings.player_torchlight_params.enabled ^= 1;
	}


	//
	if( /*Ctrl_held && */game->input.wasButtonDown(eGA_DBG_Clear_Debug_Lines) )
	{
		game->voxel_mgr.ClearDebugLines();
	}
	//
	if( game->input.wasButtonDown(eGA_DBG_Toggle_LOD_Octree_Display) )
	{
		game->settings.voxel_world_debug_settings.world_debug_draw_settings.draw_lod_octree ^= 1;
	}
	


	//
	if( game->input.wasButtonDown( eGA_DBG_Save_Game_State ) )
	{
		game->SaveSettingsToFiles();
	}
	//
	if( game->input.wasButtonDown( eGA_DBG_Load_Game_State ) )
	{
		game->LoadSettingsFromFiles();

		// reset debug acceleration
		game->player.num_seconds_shift_is_held = 0;
	}

#endif // !GAME_CFG_RELEASE_BUILD







	//
	if(!this_is_topmost_state) {
		return ContinueFurtherProcessing;
	}






	//
	const InputState& input_state = NwInputSystemI::Get().getState();

	//
#if MX_DEVELOPER && !GAME_CFG_RELEASE_BUILD



	////
	//if(
	//	Ctrl_held
	//	&&
	//	keyboard_input_state.GetBool( gainput::KeyE )
	//	)
	//{
	//	game->state_mgr.pushState(
	//		&game->states->world_editor
	//		);
	//	return StopFurtherProcessing;
	//}


#if 0
	//
	if( !Ctrl_held
		&& keyboard_input_state.GetBool( gainput::KeyE ) )// input_state.keyboard.held[KEY_E] )
	{
		game->settings.voxels.engine.debug.flags.XorWith( VX5::DbgFlag_Draw_LoD_Octree );
	}
#endif

	//
	if( game->input.wasButtonDown( eGA_Voxel_CSG_Subtract ) )
	{
		game->voxel_mgr.Test_CSG_Subtract(game->player.camera._eye_position);
	}

	if( game->input.wasButtonDown( eGA_Voxel_CSG_Add ) )
	{
		game->voxel_mgr.Test_CSG_Add(game->player.camera._eye_position);
	}

	if( game->input.wasButtonDown( eGA_Voxel_PlaceBlock ) )
	{
		game->voxel_mgr.Test_CSG_Add(game->player.camera._eye_position);
	}

	//
	_processInput_DebugTerrainCamera(
		input_map, game
		);
#endif // MX_DEVELOPER && !GAME_CFG_RELEASE_BUILD


	//
	return ContinueFurtherProcessing;
}

void RTSGameState_MainGame::_handleInput_for_FirstPersonPlayerCamera(
	MyApp* game
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	)
{
	const NwGameStateI* topmost_state = game->state_mgr.getTopState();
	const bool this_is_topmost_state = this == topmost_state;

	//
	NwFlyingCamera &camera = game->player.camera;

	FlyingCamera_handleInput_for_FirstPerson(
		camera
		, game_time
		, input_map
		, keyboard_input_state
		//, game->settings.cheats
		, game->user_settings.ingame.controls
		);

#if !GAME_CFG_RELEASE_BUILD
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
		game->player.num_seconds_shift_is_held += game_time.real.delta_seconds;

		game->player.num_seconds_shift_is_held = smallest(
			game->player.num_seconds_shift_is_held
			, 2
			);

		// increase the speed twice for every second the SHIFT key is held
		game->player.camera._dbg_speed_multiplier = powf(
			4.0f,
			1.0f + game->player.num_seconds_shift_is_held
			);
	}
	else
	{
		// decrease the speed faster if the acceleration key is not held
		game->player.num_seconds_shift_is_held -= game_time.real.delta_seconds * 2.0f;

		game->player.num_seconds_shift_is_held = maxf(
			game->player.num_seconds_shift_is_held,
			0
			);

		game->player.camera._dbg_speed_multiplier = 1;
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
		game->settings.dbg_3rd_person_camera_position = game->player.camera._eye_position;
	}

#endif // MX_DEVELOPER

}

void RTSGameState_MainGame::_processInput_DebugTerrainCamera(
	//const gainput::InputState& keyboard_input_state
	const gainput::InputMap& input_map
	, MyApp* game
	)
{
	MyAppSettings & app_settings = game->settings;

	//
	const double camera_speed = app_settings.dbg_3rd_person_camera_speed;

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

	app_settings.dbg_3rd_person_camera_position += delta_movement;
}

EStateRunResult RTSGameState_MainGame::DrawScene(
	NwAppI* app
	, const NwTime& game_time
	)
{

	return ContinueFurtherProcessing;
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

void RTSGameState_MainGame::DrawUI(
								   const NwTime& game_time
								   , NwAppI* app
								   )
{
	MyApp* game = checked_cast< MyApp* >( app );

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


#if 0//GAME_CFG_SHOW_RTS_GAME_UI_CONTROLS

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


	_DrawInGameGUI(
		*game
		, window_size
		, ctx
		);

	//
#if !GAME_CFG_RELEASE_BUILD

	if( game->settings.ui_show_debug_text )
	{
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

void RTSGameState_MainGame::_DrawDebugHUD(
	const MyApp& game
	, const UInt2 window_size
	, nk_context* ctx
	)
{
	if( game.settings.ui_show_debug_text )
	{
		//
		const NwFlyingCamera& player_camera = game.player.camera;



		const UInt2 window_size = WindowsDriver::getWindowSize();
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

		////
		//const Rendering::NwFloatingOrigin floating_origin = game.world.spatial_database->getFloatingOrigin();

		//Str::Format(text_buf,
		//	"Flt.Org World: (%f, %f, %f)",
		//	floating_origin.center_in_world_space.x,
		//	floating_origin.center_in_world_space.y,
		//	floating_origin.center_in_world_space.z
		//	);
		//NkTextUtil::drawTextLine(
		//	ctx
		//	, rect
		//	, text_buf
		//	, row++
		//	);

		//
		const Rendering::NwCameraView& scene_view = game.renderer.GetCameraView();

		Str::Format(text_buf,
			"Cam.Pos Rel (renderer.cam_view): (%f, %f, %f)",
			scene_view.eye_pos.x, scene_view.eye_pos.y, scene_view.eye_pos.z
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

#if 0
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
#endif

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
			const Rendering::NwGeometryBufferPoolStats& geom_buffer_pool_stats = Rendering::NwGeometryBufferPool::Get().getStats();

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
			NGpu::FrontEndStats	gfx_front_end_stats;
			NGpu::getFrontEndStats( &gfx_front_end_stats );

			NGpu::BackEndCounters	graphics_perf_counters;
			NGpu::getLastFrameCounters( &graphics_perf_counters );


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

			VX5::OctreeWorld & octree_world = game.voxel_mgr.Test_GetOctreeWorld();

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
				"[VX] Cache: writes: %u, hash table overflows: %u, ring buffer overflows: %u, requests: %u, misses: %u, hits: %u"
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
			game.voxel_mgr.Test_GetVoxelEngine().GetStats( &voxel_engine_stats );

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
	}//
}

void RTSGameState_MainGame::_DrawInGameGUI(
	const MyApp& game
	, const UInt2 window_size
	, nk_context* ctx
	)
{
}
