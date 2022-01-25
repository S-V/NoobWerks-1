
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
//#include <Voxels/private/Scene/vx5_octree_world.h>
//
#include "game_states/game_states.h"
#include "Localization/game_localization.h"
#include "FPSGame.h"


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
	NwGameI* game_app
	, const NwTime& game_time
)
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

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

	//
	if(
		game->player.comp_health.IsDead()
		&& game->state_mgr.getTopState() != &game->states->u_died_game_over
		)
	{
		game->state_mgr.pushState(
			&game->states->u_died_game_over
			);
		return StopFurtherProcessing;
	}


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

	//
	if( topmost_state->AllowPlayerToReceiveInput() )
	{
		_handleInput_for_Controlling_Player_Weapons(
			game
			, game_time
			, input_map
			, keyboard_input_state
			);
	}

#if !GAME_CFG_RELEASE_BUILD
	//
	if( game->input.wasButtonDown( eGA_DBG_Toggle_Wireframe ) )
	{
		game->settings.renderer.drawModelWireframes ^= 1;
	}
	//
	if( game->input.wasButtonDown( eGA_DBG_Toggle_Torchlight ) )
	{
		game->settings.player_torchlight_params.enabled ^= 1;
	}



	//
	if( game->input.wasButtonDown( eGA_DBG_Save_Game_State ) )
	{
		game->saveSettingsToFiles();
	}
	//
	if( game->input.wasButtonDown( eGA_DBG_Load_Game_State ) )
	{
		game->loadSettingsFromFiles();

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

	const bool LCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlL );
	const bool RCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlR );
	const bool Ctrl_held = LCtrl_held || RCtrl_held;

	//
	if(
		Ctrl_held
		&&
		keyboard_input_state.GetBool( gainput::KeyE )
		)
	{
		game->state_mgr.pushState(
			&game->states->world_editor
			);
		return StopFurtherProcessing;
	}


	//
	if( !Ctrl_held
		&& keyboard_input_state.GetBool( gainput::KeyE ) )// input_state.keyboard.held[KEY_E] )
	{
		game->settings.voxels.engine.debug.flags.XorWith( VX5::DbgFlag_Draw_LoD_Octree );
	}

	if( game->input.wasButtonDown( eGA_Voxel_PlaceBlock ) )
	{
		VX5::BrushEditOp	edit_op;
		edit_op.op.type = VX5::BrushEditOp::CSG_Add;
		edit_op.op.csg_add.volume_material = VoxMat_Floor;
		edit_op.brush.shape.type = VX5::EditingBrush::Cube;
		edit_op.brush.shape.cube.center = game->player.camera._eye_position;
		edit_op.brush.shape.cube.half_size = 4;

		game->world.voxels.voxel_world->Edit_CSG_Brush(
			edit_op
			);
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
	FPSGame* game
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
		, game->settings.cheats
		, game->user_settings.ingame.controls
		);


#if GAME_CFG_WITH_PHYSICS
	{
		MyGamePlayer& game_player = game->player;
		//
		setbit_cond(
			game_player.phys_movement_flags.raw_value,
			input_map.GetBool( eGA_Move_Forward ),
			PlayerMovementFlags::FORWARD
			);

		setbit_cond(
			game_player.phys_movement_flags.raw_value,
			input_map.GetBool( eGA_Move_Left ),
			PlayerMovementFlags::LEFT
			);

		setbit_cond(
			game_player.phys_movement_flags.raw_value,
			input_map.GetBool( eGA_Move_Right ),
			PlayerMovementFlags::RIGHT
			);

		setbit_cond(
			game_player.phys_movement_flags.raw_value,
			input_map.GetBool( eGA_Move_Backwards ),
			PlayerMovementFlags::BACKWARD
			);

		setbit_cond(
			game_player.phys_movement_flags.raw_value,
			input_map.GetBool( eGA_Jump ),
			PlayerMovementFlags::JUMP
			);
	}
#endif // GAME_CFG_WITH_PHYSICS



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
	, FPSGame* game
	)
{
	MyGameSettings & game_settings = game->settings;

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


void RTSGameState_MainGame::_handleInput_for_Controlling_Player_Weapons(
	FPSGame* game
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	)
{
	MyGamePlayer& game_player = game->player;

	if(NwPlayerWeapon* active_weapon = game_player._current_weapon._ptr)
	{
		const InputState& input_state = game->input._input_system->getState();
		if(input_state.mouse.held_buttons_mask & BIT(MouseButton_Left))
		{
			active_weapon->pressed_buttons |= WEAPON_BUTTON_PRIMARY_FIRE;
		}

		if(input_state.mouse.held_buttons_mask & BIT(MouseButton_Right))
		{
			active_weapon->pressed_buttons |= WEAPON_BUTTON_SECONDARY_FIRE;
		}

		if( keyboard_input_state.GetBool( gainput::KeyR ) )
		{
			active_weapon->pressed_buttons |= WEAPON_BUTTON_RELOAD;
		}

#if 0
		if( keyboard_input_state.GetBool( gainput::KeyV ) )
		{
#if MX_DEVELOPER && GAME_CFG_WITH_PHYSICS
			//
			const btVector3	bt_ray_start = toBulletVec(V3f::fromXYZ(game_player.camera._eye_position));
			const btVector3	bt_ray_end = toBulletVec(
				V3f::fromXYZ(game_player.camera._eye_position)
				+ game_player.camera.m_look_direction * 1000.0f
				);

			btCollisionWorld::ClosestRayResultCallback	ray_result_callback(bt_ray_start, bt_ray_end);

			game->world.physics_world.m_dynamicsWorld.rayTest(
				bt_ray_start, bt_ray_end, ray_result_callback
				);

			if( ray_result_callback.hasHit() )
			{
				game->dbg_target_pos = fromBulletVec(ray_result_callback.m_hitPointWorld);
			}
#endif // GAME_CFG_WITH_PHYSICS
		}
#endif
	}

	//
	if( input_map.GetBool( eGA_SelectPreviousWeapon ) )
	{
		game_player.SelectPreviousWeapon();
	}
	if( input_map.GetBool( eGA_SelectNextWeapon ) )
	{
		game_player.SelectNextWeapon();
	}
	if( input_map.GetBool( eGA_ReloadWeapon ) )
	{
		game_player.ReloadWeapon();
	}

	if( input_map.GetBool( eGA_SelectWeapon1 ) )
	{
		game_player.SelectWeapon(WeaponType::IntToEnumSafe(0));
	}
	if( input_map.GetBool( eGA_SelectWeapon2 ) )
	{
		game_player.SelectWeapon(WeaponType::IntToEnumSafe(1));
	}
	if( input_map.GetBool( eGA_SelectWeapon3 ) )
	{
		game_player.SelectWeapon(WeaponType::IntToEnumSafe(2));
	}
	if( input_map.GetBool( eGA_SelectWeapon4 ) )
	{
		game_player.SelectWeapon(WeaponType::IntToEnumSafe(3));
	}
	if( input_map.GetBool( eGA_SelectWeapon5 ) )
	{
		game_player.SelectWeapon(WeaponType::IntToEnumSafe(4));
	}
}

EStateRunResult RTSGameState_MainGame::drawScene(
	NwGameI* game_app
	, const NwTime& game_time
	)
{
	FPSGame& the_game = FPSGame::Get();

	MyGameRenderer& game_renderer = the_game.renderer;
	MyGameWorld& world = the_game.world;
	const MyGameSettings& game_settings = the_game.settings;
	const MyGamePlayer& player = the_game.player;

	//
	GL::NwRenderContext & render_context = GL::getMainRenderContext();

	//
	const NwFloatingOrigin floating_origin = world.render_world->getFloatingOrigin();

	//
	NwCameraView & camera_view = game_renderer._camera_matrices;
	{
#if 0
		camera_view.eye_pos = floating_origin.relative_eye_position;
		camera_view.eye_pos_in_world_space = player.camera._eye_position;
#else

		//camera_view.eye_pos_in_world_space = player.camera._eye_position;
		camera_view.eye_pos_in_world_space = V3d::fromXYZ(player.GetEyePosition());

		camera_view.eye_pos = player.GetEyePosition();
		
#endif

		switch( _game_camera_mode )
		{
		case GameCam_PlayerSpaceship:
			{
				UNDONE;
				//const GameSpaceship& player_spaceship = world._player_spaceship;
				//camera_view.right_dir = player_spaceship.right_dir;
				//camera_view.look_dir = player_spaceship.forward_dir;
			}
			break;

		case GameCam_DebugFreeFlight:
			{
				const NwFlyingCamera& camera = player.camera;
				camera_view.right_dir = camera.m_rightDirection;
				camera_view.look_dir = camera.m_look_direction;
			}
			break;
		}


		camera_view.near_clip = maxf( game_settings.renderer.camera_near_clip, 0.1f );
		camera_view.far_clip = game_settings.renderer.camera_far_clip;

		const UInt2 window_size = WindowsDriver::getWindowSize();
		camera_view.screen_width = window_size.x;
		camera_view.screen_height = window_size.y;
	}
	camera_view.recomputeDerivedMatrices();

	//
	game_renderer.beginFrame(
		camera_view
		, game_settings.renderer
		, game_time
		);


	//
	const NwGameStateI* topmost_state = the_game.state_mgr.getTopState();
	// don't draw the player's weapon in editor mode
	if( topmost_state->ShouldRenderPlayerWeapon() )
	{
		player.RenderFirstPersonView(
			camera_view
			, game_settings.renderer
			, render_context
			, game_time
			, world.render_world
			);
	}


	//
	world.Render(
		camera_view
		, game_settings.renderer
		, game_time
		, game_renderer
		);

	////
	//game_renderer.submit_Star(
	//	world._main_star
	//	, player
	//	, camera_view
	//	, render_context
	//	);


	if( game_settings.player_torchlight_params.enabled )
	{
		TbDynamicPointLight	test_light;
		test_light.position = V3f::fromXYZ(player.camera._eye_position); //region_of_interest_position;
		test_light.radius = game_settings.player_torchlight_params.radius;
		test_light.color = game_settings.player_torchlight_params.color;
		world.render_world->pushDynamicPointLight(test_light);
	}

	//if( game_settings.terrain_roi_torchlight_params.enabled )
	//{
	//	TbDynamicPointLight	test_light;
	//	test_light.position = player.camera._eye_position; //region_of_interest_position;
	//	test_light.radius = game_settings.terrain_roi_torchlight_params.radius;
	//	test_light.color = game_settings.terrain_roi_torchlight_params.color;
	//	world.render_world->pushDynamicPointLight(test_light);
	//}

	//


	game_renderer.beginDebugLineDrawing(camera_view, render_context);


	if( topmost_state->ShouldRenderPlayerWeapon() && player._current_weapon != nil )
	{
		player._current_weapon->DrawCrosshair(
			game_renderer._render_system->_debug_renderer
			, camera_view
			);
	}


#if !GAME_CFG_RELEASE_BUILD

	game_renderer.drawWorld_Debug(
		world
		, world.render_world
		, camera_view
		, player
		, game_settings
		, game_settings.renderer
		, game_time
		, render_context
		);


	//
	//if( game_settings.voxels.engine.debug.flags & VX5::DbgFlag_Draw_LoD_Octree )
	{
		const NwFloatingOrigin floating_origin = world.render_world->getFloatingOrigin();

		world.debugDrawVoxels(
			floating_origin
			, camera_view
			);
	}

#endif // !GAME_CFG_RELEASE_BUILD




	game_renderer.endDebugLineDrawing();


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

void RTSGameState_MainGame::drawUI(
								   float delta_time_in_seconds
								   , NwGameI* game_app
								   )
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

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

void RTSGameState_MainGame::_DrawDebugHUD_ImGui(
	const FPSGame& game
	)
{

#if MX_DEVELOPER

	const EnemyNPC* npc = game.runtime_clump.FindFirstObjectOfType<EnemyNPC>();
	if(npc)
	{
		ImGui::SetNextWindowSize(ImVec2(600,300));
		if(ImGui::Begin("Test NPC"))
		{
			const NwAnimatedModel& anim_model = *npc->anim_model;
			const NwSkinnedMesh& skinned_mesh = *anim_model._skinned_mesh;

			//
			String256	text_buf;
			anim_model.anim_player.DbgDump(text_buf);

			ImGui::TextWrapped(
				text_buf.c_str()
				);

			//const NwAnimClip* anim_clip = skinned_mesh.getAnimByNameHash(mxHASH_STR("walk1"));
			//const V3f& org_speed = anim_clip->root_joint_average_speed;

#if 0
			const TSpan<const M44f> joints = Arrays::getSpan(anim_model.render_model->joint_matrices);
			//const M44f& root_joint_mat = joints[0];
			const V3f& root_offs = anim_model.job_result.root_joint_offset;

			//Str::Format(text_buf,
			//	"NPC[0] root mat: x = %f, y = %f, z = %f, org speed: x = %f, y = %f, z = %f"
			//	, root_joint_mat.translation().x
			//	, root_joint_mat.translation().y
			//	, root_joint_mat.translation().z
			//	, org_speed.x, org_speed.y, org_speed.z
			//	);

			ImGui::TextWrapped(
				"\n\nNPC[0] time ratio = %f, _did_just_wrap_around = %d, root offset: x = %f, y = %f, z = %f"
				, anim_model.anim_player._active_anims[0].playback_controller._time_ratio
				, anim_model.anim_player._active_anims[0].playback_controller.didWrapAroundDuringLastUpdate()
				, root_offs.x, root_offs.y, root_offs.z
				);
#endif
		}
		ImGui::End();
	}

#endif // #if MX_DEVELOPER

}

void RTSGameState_MainGame::_DrawDebugHUD(
	const FPSGame& game
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

		//
		const NwFloatingOrigin floating_origin = game.world.render_world->getFloatingOrigin();

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
		const NwCameraView& camera_view = game.renderer._camera_matrices;

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
			const NwGeometryBufferPoolStats& geom_buffer_pool_stats = NwGeometryBufferPool::Get().getStats();

			Str::Format(text_buf,
				"Smoke Puffs: %u"
				, game.renderer.particle_renderer.smoke_puffs.num()
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

			VX5::VoxelEngineStats	voxel_engine_stats;
			game.world.voxels.engine->getStats( &voxel_engine_stats );

			//
			Str::Format(text_buf,
				"Skipped: %u Empty, %u Solid; Processed: %u"
				, AtomicLoad( voxel_engine_stats.generated_chunks_skipped_as_empty )
				, AtomicLoad( voxel_engine_stats.generated_chunks_skipped_as_filled )
				, AtomicLoad( voxel_engine_stats.generated_chunks_not_skipped )
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);
		}




		//
#if GAME_CFG_WITH_PHYSICS
		{
			Str::Format(text_buf, "=== Physics:");
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);

			//
			Str::Format(text_buf,
				"IsOnGround: %d, IsOnSteepSlope: %d"
				, game.player.phys_char_controller.onGround()
				, game.player.phys_char_controller.mOnSteepSlope
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);

			//
			const V3f& player_char_ctrl_pos = game.player.phys_char_controller.getPos();
			Str::Format(text_buf,
				"PhysCharCtrl.Pos: x = %f, y = %f, z = %f"
				, player_char_ctrl_pos.x, player_char_ctrl_pos.y, player_char_ctrl_pos.z
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);

			//
			const V3f& player_char_ctrl_vel = fromBulletVec(game.player.phys_char_controller.getLinearVelocity());
			Str::Format(text_buf,
				"PhysCharCtrl.Vel: x = %f, y = %f, z = %f"
				, player_char_ctrl_vel.x, player_char_ctrl_vel.y, player_char_ctrl_vel.z
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);



			////
			//const V3f& sway_pos = game.player.weapon_sway.prev_pos;
			//Str::Format(text_buf,
			//	"WeaponSway.Pos: x = %f, y = %f, z = %f"
			//	, sway_pos.x, sway_pos.y, sway_pos.z
			//	);
			//NkTextUtil::drawTextLine(
			//	ctx
			//	, rect
			//	, text_buf
			//	, row++
			//	);

			////
			//String256		tmp_str;
			//StringStream	str_stream(tmp_str);

			//str_stream
			//	,"Weapon.RenderModel.pos: ",game.player._current_weapon->anim_model->render_model->local_to_world.translation()
			//	;

			//Str::Format(text_buf,
			//	tmp_str.c_str()
			//	);
			//NkTextUtil::drawTextLine(
			//	ctx
			//	, rect
			//	, text_buf
			//	, row++
			//	);
		}
#endif // GAME_CFG_WITH_PHYSICS




		//nk_command_buffer * window_cmd_buffer = nk_window_get_canvas( ctx );

		//nk_draw_text(
		//	window_cmd_buffer
		//	, nk_rect(
		//		10, screen_height*0.3f,
		//		screen_width, screen_height
		//	)
		//	, text_buf.c_str()
		//	, text_buf.length()
		//	, ctx->style.font
		//	, white
		//	, white
		//	);
		//
		//
		//nk_draw_list_add_text( ctx->draw_list, ctx->style.font, )
		//nk_label( ctx, "", );

	}//
}

void RTSGameState_MainGame::_DrawInGameGUI(
	const FPSGame& game
	, const UInt2 window_size
	, nk_context* ctx
	)
{
	const float screen_width = window_size.x;
	const float screen_height = window_size.y;

	//
	String256	text_buf;

	{
		const struct nk_rect rect = nk_rect(
			screen_width*0.05f, screen_height*0.04f,
			screen_width, screen_height
			);

		int row = 0;

		// Health/Armor.
		{
			Str::Format(text_buf,
				"Health: %d",
				game.player.comp_health._health
				);
			NkTextUtil::drawTextLine(
				ctx
				, rect
				, text_buf
				, row++
				);
		}
	}

	//
	{
		const struct nk_rect rect = nk_rect(
			screen_width*0.91f, screen_height*0.04f,
			screen_width, screen_height
			);

		int row = 0;

		// Ammo.
		{
			if(NwPlayerWeapon* curr_weapon = game.player._current_weapon._ptr)
			{
				Str::Format(text_buf,
					"%u / %u",
					curr_weapon->ammo_in_clip, curr_weapon->recharge_ammo
					);
				NkTextUtil::drawTextLine(
					ctx
					, rect
					, text_buf
					, row++
					);
			}
		}
	}
}
