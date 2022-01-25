//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Renderer/Pipeline/RenderPipeline.h>
#include <Renderer/Renderer.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <Voxels/private/Common/vx5_debug.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include "FPSGame.h"
#include "game_states/game_states.h"
#include "game_states/game_state_debug_hud.h"
#include "gameplay_constants.h"
#include "game_world/pickable_items.h"


/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugHUD
-----------------------------------------------------------------------------
*/
NwGameState_ImGui_DebugHUD::NwGameState_ImGui_DebugHUD()
{
	this->DbgSetName("DebugHUD");
}

ERet NwGameState_ImGui_DebugHUD::Initialize(
									  const TbRenderSystemI* render_system
									  )
{
	ImGui_Renderer::Config	cfg;
	cfg.save_to_ini = !GAME_CFG_RELEASE_BUILD;

	mxDO(ImGui_Renderer::Initialize(cfg));

	return ALL_OK;
}

void NwGameState_ImGui_DebugHUD::Shutdown()
{
	ImGui_Renderer::Shutdown();
}

EStateRunResult NwGameState_ImGui_DebugHUD::handleInput(
	NwGameI* game_app
	, const NwTime& game_time
)
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

	if( game->input.wasButtonDown( eGA_Exit ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	if( game->input.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	// debug HUD blocks main game UI
	return StopFurtherProcessing;
}

void NwGameState_ImGui_DebugHUD::tick(
								float delta_time_in_seconds
								, NwGameI* game_app
								)
{
	ImGui_Renderer::UpdateInputs();
}

void NwGameState_ImGui_DebugHUD::drawUI(
								  float delta_time_in_seconds
								  , NwGameI* game_app
								  )
{
	//
	_draw_actual_ImGui( delta_time_in_seconds, game_app );
}

void NwGameState_ImGui_DebugHUD::_draw_actual_ImGui(
	float delta_time_in_seconds
	, NwGameI* game_app
	)
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

	MyGameSettings & game_settings = game->settings;


	if(ImGui::Button("Clear Debug Lines"))
	{
		game->world.voxels.voxel_engine_dbg_drawer.VX_Clear_AnyThread();
	}

	//
	ImGui::Checkbox("Show Debug Text", &game_settings.ui_show_debug_text);
	
	ImGui::Checkbox("Show Game Settings", &game_settings.ui_show_game_settings);
	ImGui::Checkbox("Show Cheats", &game_settings.ui_show_cheats);

	ImGui::Checkbox("[Game] NwWeaponDef Editor", &game_settings.show_editor_for_NwWeaponDef);

	//ImGui::Checkbox("Show this HUD (F1)", &game_settings.ui_show_debug_text);

	ImGui::Checkbox("Show Scene Settings", &game_settings.dev_showSceneSettings);
	ImGui::Checkbox("Show Graphics Debugger", &game_settings.dev_showGraphicsDebugger);
	ImGui::Checkbox("Show Renderer Settings", &game_settings.dev_showRendererSettings);

	ImGui::Checkbox("Show Voxel Engine Settings", &game_settings.dev_showVoxelEngineSettings);
	ImGui::Checkbox("Show Voxel Engine Debug Menu", &game_settings.dev_showVoxelEngineDebugMenu);

	ImGui::Checkbox("Show Tweakables", &game_settings.dev_showTweakables);
	ImGui::Checkbox("Show Perf Stats", &game_settings.dev_showPerfStats);
	ImGui::Checkbox("Show Memory Usage", &game_settings.dev_showMemoryUsage);

	//
	ImGui::InputDouble("Test PlaneOffsetZ", &game->world.voxels._test_plane.abcd.w);

	ImGui::InputInt3("Debug Int Coords", (int*)VX5::g_dbg_cell_coords.a);
	ImGui::InputInt("Debug Axis", (int*)&VX5::g_dbg_axis);
	//ImGui_drawEnumComboBoxT()

	if(ImGui::Button("Load Test Level"))
	{
		FPSGame::Get().LoadLevelNamed("test_level");
	}


	//
	if( ImGui::Begin("Camera") )
	{
		NwFlyingCamera * camera = & game->player.camera;
		V3d * camera_pos = & camera->_eye_position;
		//
		//ImGui::Text("Eye position: %f, %f, %f",
		//	camera_view.eye_pos.x, camera_view.eye_pos.y, camera_view.eye_pos.z
		//	);
		ImGui::InputDouble("Eye position X", &camera_pos->x);
		ImGui::InputDouble("Eye position Y", &camera_pos->y);
		ImGui::InputDouble("Eye position Z", &camera_pos->z);

		ImGui::Text("Look Dir X: %f", camera->m_look_direction.x);
		ImGui::Text("Look Dir Y: %f", camera->m_look_direction.y);
		ImGui::Text("Look Dir Z: %f", camera->m_look_direction.z);

		ImGui::InputFloat("Movement speed", &camera->m_movementSpeed, 0.0f, 100.0f);
		ImGui::InputFloat("Acceleration", &camera->m_initialAccel, 0.0f, 10.0f);

		//
	}
	ImGui::End();




	//
	if( ImGui::Begin("NPC Control") )
	{
		NwFlyingCamera * camera = & game->player.camera;

		const V3f	npc_spawn_pos
			= V3f::fromXYZ(camera->_eye_position)
			+ camera->m_look_direction * 2.0f
			;

		if(ImGui::Button("Spawn Spider"))
		{
			EnemyNPC *	new_npc;
			game->world.NPCs.Spawn_Spider(
				new_npc
				, game->world
				, npc_spawn_pos
				);
		}

		if(ImGui::Button("Spawn Imp"))
		{
			EnemyNPC *	new_npc;
			game->world.NPCs.Spawn_Imp(
				new_npc
				, game->world
				, npc_spawn_pos
				);
		}
		if(ImGui::Button("Spawn Maggot"))
		{
			EnemyNPC *	new_npc;
			game->world.NPCs.Spawn_Maggot(
				new_npc
				, game->world
				, npc_spawn_pos
				);
		}
		if(ImGui::Button("Spawn Mancubus"))
		{
			EnemyNPC *	new_npc;
			game->world.NPCs.Spawn_Mancubus(
				new_npc
				, game->world
				, npc_spawn_pos
				);
		}

		//ImGui::Spacing();
		ImGui::Separator();

		if(ImGui::Button("Spawn Medkit"))
		{
			MyGameUtils::SpawnItem(
				Gameplay::EPickableItem::ITEM_MEDKIT
				, npc_spawn_pos
				);
		}
		if(ImGui::Button("Spawn Rocket Ammo"))
		{
			MyGameUtils::SpawnItem(
				Gameplay::EPickableItem::ITEM_ROCKET_AMMO
				, npc_spawn_pos
				);
		}
		//if(ImGui::Button("Spawn Plasma Ammo"))
		//{
		//	MyGameUtils::SpawnItem(
		//		Gameplay::EPickableItem::ITEM_PLASMA_AMMO
		//		, npc_spawn_pos
		//		);
		//}
		if(ImGui::Button("Spawn Plasmagun"))
		{
			MyGameUtils::SpawnItem(
				Gameplay::EPickableItem::ITEM_PLASMAGUN
				, npc_spawn_pos
				);
		}
		if(ImGui::Button("Spawn BFG case"))
		{
			MyGameUtils::SpawnItem(
				Gameplay::EPickableItem::ITEM_BFG_CASE
				, npc_spawn_pos
				);
		}
	}
	ImGui::End();


	if( game_settings.ui_show_game_settings )
	{
		if( ImGui::Begin("Game") )
		{
			RTSGameState_MainGame& game_state_main_game = game->states->state_main_game;
			bool use_spaceship_camera = game_state_main_game._game_camera_mode == GameCam_PlayerSpaceship;
			
			ImGui::Checkbox("Use Spaceship Camera?", &use_spaceship_camera);

			game_state_main_game._game_camera_mode = use_spaceship_camera ? GameCam_PlayerSpaceship : GameCam_DebugFreeFlight;
		}
		ImGui::End();
	}

	if(game_settings.ui_show_cheats)
	{
		if( ImGui::Begin("Cheats") )
		{
			if(ImGui::Button("Replenish Health")) {
				game->player.ReplenishHealth();
			}
			if(ImGui::Button("SuperHealth")) {
				game->player.comp_health._health = 999;
			}
			if(ImGui::Button("Replenish Ammo")) {
				game->player.ReplenishAmmo();
			}
			ImGui::Checkbox("Godmode", &game->settings.cheats.godmode_on);
			ImGui::Checkbox("Noclip", &game->settings.cheats.enable_noclip);
		}
		ImGui::End();
	}


#if MX_DEVELOPER
	//
	if( game_settings.dev_showTweakables )
	{
		ImGui_DrawTweakablesWindow();
	}
#endif


	//
	if( game_settings.dev_showSceneSettings )
	{
		if( ImGui::Begin("Scene Settings") )
		{
#if 0
			bool sunLightChanged = false;

			if( ImGui::Button("Copy Camera Look Dir to Sun Direction" ) )
			{
				game_settings.sun_light.direction = camera.m_look_direction;
				sunLightChanged = true;
			}
			if( ImGui::Button("Reset Sun Direction" ) )
			{
				game_settings.sun_light.direction = -V3_UP;
				sunLightChanged = true;
			}

			{
				V3f newSunLightDir = game_settings.sun_light.direction;

				ImGui::SliderFloat("SunDirX", &newSunLightDir.x, -1, 1);
				ImGui::SliderFloat("SunDirY", &newSunLightDir.y, -1, 1);
				ImGui::SliderFloat("SunDirZ", &newSunLightDir.z, -1, 1);

				newSunLightDir = V3_Normalized( newSunLightDir );
				if( newSunLightDir != game_settings.sun_light.direction )
				{
					game_settings.sun_light.direction = newSunLightDir;
					sunLightChanged = true;
				}
			}

			//float luminance = RGBf(globalLight->color.x, globalLight->color.y, globalLight->color.z).Brightness();
			//ImGui::SliderFloat("SunLightBrightness", &globalLight->brightness, 0, 100);
			//globalLight->color *= luminance;

			if( sunLightChanged )
			{
				_render_world->setGlobalLight( game_settings.sun_light );
				_render_world->modifySettings( game_settings.renderer );
			}
#endif
			ImGui_DrawPropertyGrid(
				&game_settings,
				mxCLASS_OF(game_settings),
				"App Settings"
				);
		}
		ImGui::End();
	}

	//
	if( game_settings.dev_showRendererSettings )
	{
		if( ImGui::Begin("Renderer") )
		{
			ImGui::SliderFloat(
				"Animation Speed"
				, &game_settings.renderer.animation.animation_speed_multiplier
				, RrAnimationSettings::minValue()
				, RrAnimationSettings::maxValue()
				);

			////
			//if( ImGui::Button("Regenerate Light Probes" ) )//zzz
			//{
			//	((Rendering::NwRenderSystem&)TbRenderSystemI::Get())._test_need_to_relight_light_probes = true;
			//}

			//
			const U64 rendererSettingsHash0 = game_settings.renderer.computeHash();

			
			const U64 rendererSettingsHash1 = game_settings.renderer.computeHash();
			if( rendererSettingsHash0 != rendererSettingsHash1 )
			{
				DEVOUT("Renderer Settings changed");
				game->world.render_world->modifySettings( game_settings.renderer );
			}

			ImGui::Text("Objects visible: %u / %u (%u culled)", g_FrontEndStats.objects_visible, g_FrontEndStats.objects_total, g_FrontEndStats.objects_culled);
			ImGui::Text("Shadow casters rendered: %u", g_FrontEndStats.shadow_casters_rendered);
			//ImGui::Text("Meshes: %u", NwMesh::s_totalCreated);
			IF_DEBUG ImGui::Checkbox("Debug Break?", &g_DebugBreakEnabled);

#if 0
			m_visibleObjects.Reset();
			ViewFrustum	viewFrustum;
			viewFrustum.ExtractFrustumPlanes( view_projection_matrix );
			U64 startTimeMSec = mxGetTimeInMilliseconds();
			m_cullingSystem->FindVisibleObjects(viewFrustum, m_visibleObjects);
			U64 elapsedTimeMSec = mxGetTimeInMilliseconds() - startTimeMSec;
			ImGui::Text("Test Culling: %u visible (%u msec)", m_visibleObjects.count[RE_Model], elapsedTimeMSec);
#endif
		}
		ImGui::End();

		//
		game->renderer._render_system->applySettings( game_settings.renderer );
	}//Renderer



	//
	if( game_settings.dev_showVoxelEngineSettings )
	{
		if( ImGui::Begin("Voxel Engine Settings") )
		{
			//
			if( ImGui::Button("Save To File") )
			{
				game->saveSettingsToFiles();
			}

			//
			//ImGui::InputDouble( "Noise Scale", &game->world._fast_noise_scale );

			//
			if( ImGui::Button("Regenerate World") )
			{
				game->world.regenerateVoxelTerrains();
			}


			////
			//const bool isInGameState = ( state_mgr.CurrentState() == GameStates::main );
			//if( ImGui::Button("Regenerate world") || (isInGameState && input_state.keyboard.held[KEY_R]) ) {
			//	_voxel_world.dbg_waitForAllTasks();
			//	if(mxSUCCEDED( this->rebuildBlobTreeFromLuaScript() )) {
			//		_voxel_world.dbg_regenerateWorld();
			//	}
			//}

			//
			{
				const U64 voxel_engine_settings_hash_before = CalcPodHash(game_settings.voxels.engine);
				ImGui_DrawPropertyGrid(
					&game_settings.voxels.engine,
					mxCLASS_OF(game_settings.voxels.engine),
					"Voxel Engine Settings"
					);
				const U64 voxel_engine_settings_hash_after = CalcPodHash(game_settings.voxels.engine);
				if(voxel_engine_settings_hash_before != voxel_engine_settings_hash_after) {
					DBGOUT("Voxel Engine Settings changed!!!");
					game->world.voxels.engine->applySettings(game_settings.voxels.engine);

					VX5::RunTimeWorldSettings	world_settings;
					world_settings.lod = game_settings.voxels.engine.lod;
					world_settings.mesher = game_settings.voxels.engine.mesher;
					world_settings.debug = game_settings.voxels.engine.debug;
					game->world.voxels.voxel_world->applySettings(world_settings);
				}
			}

			////
			//if( ImGui::Button("Reset Tool Settings" ) )
			//{
			//	m_settings.dev_voxel_tools.resetAllToDefaultSettings(
			//		VX5::getChunkSizeAtLoD(0)
			//		);
			//}

			//ImGui_DrawPropertyGrid(
			//	&m_settings.dev_voxel_tools,
			//	mxCLASS_OF(m_settings.dev_voxel_tools),
			//	"Voxel Tools Settings"
			//	);

			////
			//if( ImGui::BeginChild("Chunk Debugging") )
			//{
			//	if( ImGui::Button("Remesh current chunk" ) )
			//	{
			//		_voxel_world.dbg_RemeshChunk( getCameraPosition() );
			//	}
			//}

			//ImGui::EndChild();
		}
		ImGui::End();


		//
		if( game_settings.dev_showVoxelEngineDebugMenu )
		{
			VXExt::ImGui_DrawVoxelEngineDebugHUD(
				game->world.voxels.voxel_world
				, game->player.camera._eye_position
				, game->world.voxels.voxel_engine_dbg_drawer
				);
		}

		//if( game_settings.ui_show_planet_settings )
		//{
		//	Planets::PlanetProcGenData & planet_proc_gen_data = game->world._test_voxel_planet_procgen_data;

		//	//
		//	ImGui::InputDouble("Radius", &planet_proc_gen_data.sphere_zero_height.radius);
		//	ImGui::InputDouble("Max Height", &planet_proc_gen_data.max_height);
		//}
	}//Voxel Engine


	//
	if(game_settings.show_editor_for_NwWeaponDef)
	{
		_Draw_Editor_UI_for_NwWeaponDef();
	}
}

void NwGameState_ImGui_DebugHUD::_Draw_Editor_UI_for_NwWeaponDef()
{
	if( ImGui::Begin("[Game] NwWeaponDef Editor") )
	{
		MyGamePlayer& player = FPSGame::Get().player;

		static WeaponType8	edited_weapon_type;

		ImGui_drawEnumComboBoxT(&edited_weapon_type, "Edited Weapon Type");

		NwPlayerWeapon& weapon = *player.inventory.weapon_models[ edited_weapon_type ];

		NwWeaponDef& weapon_def = *weapon.def;

		ImGui_DrawPropertyGrid(
			&weapon_def,
			mxCLASS_OF(weapon_def),
			WeaponType8_Type().GetString(edited_weapon_type)
			);

		//
		ImGui::DragFloat(
			"gun_scale",
			&weapon_def.view_offset.gun_scale,
			0.0005f,
			0.0005f,
			+1,
			"%f"
			);

		//
		ImGui::DragFloat3(
			"gun_offset",
			weapon_def.view_offset.gun_offset.a,
			0.001f,
			-1,
			+1,
			"%f"
			);

		//
		ImGui::DragFloat3(
			"gun_angles",
			weapon_def.view_offset.gun_angles_deg.a,
			0.01f,
			-180,
			+180,
			"%f"
			);

		//
		//
		if( ImGui::Button("Save to File") )
		{
			_Save_NwWeaponDef_to_File(weapon_def);
		}
		//
		if( ImGui::Button("Load from File") )
		{
			UNDONE;
			//_load_RawMesh_from_File();
		}
	}
	ImGui::End();
}

ERet NwGameState_ImGui_DebugHUD::_Save_NwWeaponDef_to_File(const NwWeaponDef& weapon_def)
{
	DBGOUT("Saving NwWeaponDef...");

	nfdchar_t *selected_folder = NULL;

	nfdresult_t result = NFD_PickFolder(
		NULL	// nfdchar_t *defaultPath
		, &selected_folder
		);

	if ( result == NFD_OKAY )
	{
		const char* filename = "test.son";	// Str::GetFileName(_model_editor._loaded_mesh_filepath.c_str());

		FilePathStringT	savefilepath;
		Str::ComposeFilePath(savefilepath, selected_folder, filename);
		//Str::setFileExtension(savefilepath, TbRawMeshData::metaClass().m_name );

		//
		SON::SaveToFile(
			weapon_def
			, savefilepath.c_str()
			);

		free(selected_folder);
	}
	else if ( result == NFD_CANCEL )
	{
		puts("User pressed cancel.");
	}
	else
	{
		printf("Error: %s\n", NFD_GetError() );
	}

	return ALL_OK;
}

ERet NwGameState_ImGui_DebugHUD::_Load_NwWeaponDef_from_File(NwWeaponDef &weapon_def_)
{
	UNDONE;
#if 0
	nfdchar_t *selected_filepath = NULL;

	nfdresult_t result = NFD_OpenDialog(
		NULL	// nfdchar_t *filterList
		, NULL	// nfdchar_t *defaultPath
		, &selected_filepath
		);

	if ( result == NFD_OKAY )
	{
		_model_editor.loadRawMeshFromFile(selected_filepath, *_storage_clump);

		free(selected_filepath);
	}
	else if ( result == NFD_CANCEL )
	{
		puts("User pressed cancel.");
	}
	else
	{
		printf("Error: %s\n", NFD_GetError() );
	}
#endif
	return ALL_OK;
}







void DevConsoleState::tick(
	float delta_time_in_seconds
	, NwGameI* game_app
	)
{
}

void DevConsoleState::drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
							 )
{
	static bool show_test_window = true;
	static bool show_another_window = false;

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		static float f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		show_test_window ^= ImGui::Button("Test Window");
		show_another_window ^= ImGui::Button("Another Window");

		// Calculate and show frame rate
		static float ms_per_frame[120] = { 0 };
		static int ms_per_frame_idx = 0;
		static float ms_per_frame_accum = 0.0f;
		ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
		ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
		ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
		ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
		const float ms_per_frame_avg = ms_per_frame_accum / 120;
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);
	}

	//// 2. Show another simple window, this time using an explicit Begin/End pair
	//if (show_another_window)
	//{
	//	ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
	//	ImGui::Text("Hello");
	//	ImGui::End();
	//}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20));
		ImGui::ShowDemoWindow(&show_test_window);
	}
}
