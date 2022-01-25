//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Globals.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <Voxels/private/debug/vxp_debug.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include "game_app.h"
#include "app_states/app_states.h"
#include "app_states/game_state_debug_hud.h"


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
									  const NwRenderSystemI* render_system
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
	NwAppI* game_app
	, const NwTime& game_time
)
{
	SgGameApp* game = checked_cast< SgGameApp* >( game_app );

	if( game->input_mgr.wasButtonDown( eGA_Exit ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	if( game->input_mgr.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	// debug HUD blocks main app UI
	return StopFurtherProcessing;
}

void NwGameState_ImGui_DebugHUD::Update(
								const NwTime& game_time
								, NwAppI* game_app
								)
{
	ImGui_Renderer::UpdateInputs();
}

void NwGameState_ImGui_DebugHUD::DrawUI(
								  const NwTime& game_time
								  , NwAppI* game_app
								  )
{
	//
	_draw_actual_ImGui( game_time, game_app );
}

void NwGameState_ImGui_DebugHUD::_draw_actual_ImGui(
	const NwTime& game_time
	, NwAppI* game_app
	)
{
	SgGameApp* app = checked_cast< SgGameApp* >( game_app );

	SgAppSettings & game_settings = app->settings;


#if GAME_CFG_WITH_VOXELS
	if(ImGui::Button("Clear Debug Lines"))
	{
		VX5::MyDebugDraw& dbg_view = app->world.voxels.voxel_engine_dbg_drawer;
		dbg_view.VX_Lock();
		dbg_view.VX_Clear();
		dbg_view.VX_Unlock();
	}
#endif


	ImGui::Checkbox("Show Developer HUD", &app->user_settings.developer.show_hud);

#if GAME_CFG_RELEASE_BUILD
	return;
#endif // !GAME_CFG_RELEASE_BUILD

	
	ImGui::Checkbox("Show Debug Text", &game_settings.ui_show_debug_text);

	ImGui::Checkbox("Show Game Settings", &game_settings.ui_show_game_settings);
	ImGui::Checkbox("Show Cheats", &game_settings.ui_show_cheats);

	//ImGui::Checkbox("Show this HUD (F1)", &game_settings.ui_show_debug_text);

	ImGui::Checkbox("Show Scene Settings", &game_settings.dev_showSceneSettings);
	ImGui::Checkbox("Show Graphics Debugger", &game_settings.dev_showGraphicsDebugger);
	ImGui::Checkbox("Show Renderer Settings", &game_settings.dev_showRendererSettings);

	ImGui::Checkbox("Show Voxel Engine Settings", &game_settings.dev_show_voxel_engine_settings);
	ImGui::Checkbox("Show Voxel Engine Debug Menu", &game_settings.dev_show_voxel_engine_debug_menu);

	ImGui::Checkbox("Show Tweakables", &game_settings.dev_showTweakables);
	ImGui::Checkbox("Show Perf Stats", &game_settings.dev_showPerfStats);
	ImGui::Checkbox("Show Memory Usage", &game_settings.dev_showMemoryUsage);


#if 0
	if(ImGui::Button("Load Test Level"))
	{
		SgGameApp::Get().LoadLevelNamed("test_level");
	}
#endif

	//
	if( ImGui::Begin("Camera") )
	{
		NwFlyingCamera * camera = & app->gameplay.player.camera;
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




#if 0
	//
	if( ImGui::Begin("NPC Control") )
	{
		NwFlyingCamera * camera = & app->gameplay.player.camera;

		const V3f	npc_spawn_pos
			= V3f::fromXYZ(camera->_eye_position)
			+ camera->m_look_direction * 2.0f
			;

		//ImGui::Spacing();
		ImGui::Separator();
	}
	ImGui::End();
#endif


#if 0
	if( game_settings.ui_show_game_settings )
	{
		if( ImGui::Begin("Game") )
		{
			SgState_MainGame& game_state_main_game = game->states->state_main_game;
			bool use_spaceship_camera = game_state_main_game._game_camera_mode == GameCam_PlayerSpaceship;
			
			ImGui::Checkbox("Use Spaceship Camera?", &use_spaceship_camera);

			game_state_main_game._game_camera_mode = use_spaceship_camera ? GameCam_PlayerSpaceship : GameCam_DebugFreeFlight;
		}
		ImGui::End();
	}
#endif

	if(game_settings.ui_show_cheats)
	{
		if( ImGui::Begin("Cheats") )
		{
			ImGui::Checkbox("Godmode", &app->settings.cheats.godmode_on);
			ImGui::Checkbox("Noclip", &app->settings.cheats.enable_noclip);
		}
		ImGui::End();
	}


#if MX_DEVELOPER && !GAME_CFG_RELEASE_BUILD
	//
	if( game_settings.dev_showTweakables )
	{
		ImGui_DrawTweakablesWindow();
	}
#endif

	if(app->user_settings.developer.show_hud)
	{
		_ImGUI_DrawDeveloperHUD();
	}


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
			//	((Rendering::NwRenderSystem&)Rendering::Globals::)._test_need_to_relight_light_probes = true;
			//}

			//
			const U64 rendererSettingsHash0 = game_settings.renderer.computeHash();

			
			const U64 rendererSettingsHash1 = game_settings.renderer.computeHash();
			if( rendererSettingsHash0 != rendererSettingsHash1 )
			{
				DEVOUT("Renderer Settings changed");
				app->world.spatial_database->modifySettings( game_settings.renderer );
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
		app->renderer._render_system->ApplySettings( game_settings.renderer );
	}//Renderer


#if GAME_CFG_WITH_VOXELS
	//
	if( game_settings.dev_show_voxel_engine_settings )
	{
		if( ImGui::Begin("Voxel Engine Settings") )
		{
			//
			if( ImGui::Button("Save To File") )
			{
				app->saveSettingsToFiles();
			}

			//
			//ImGui::InputDouble( "Noise Scale", &app->world._fast_noise_scale );

			//
			if( ImGui::Button("Regenerate World") )
			{
				app->world.regenerateVoxelTerrains();
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
					app->world.voxels.voxel_engine->ApplySettings(game_settings.voxels.engine);

					VX5::RunTimeWorldSettings	world_settings;
					world_settings.lod = game_settings.voxels.engine.lod;
					world_settings.mesher = game_settings.voxels.engine.mesher;
					world_settings.debug = game_settings.voxels.engine.debug;
					app->world.voxels.voxel_world->ApplySettings(world_settings);
				}
			}
		}
		ImGui::End();



		//if( game_settings.ui_show_planet_settings )
		//{
		//	Planets::PlanetProcGenData & planet_proc_gen_data = app->world._test_voxel_planet_procgen_data;

		//	//
		//	ImGui::InputDouble("Radius", &planet_proc_gen_data.sphere_zero_height.radius);
		//	ImGui::InputDouble("Max Height", &planet_proc_gen_data.max_height);
		//}
	}//Voxel Engine
#endif // #if GAME_CFG_WITH_VOXELS


#if GAME_CFG_WITH_VOXELS
	//
	if( game_settings.dev_show_voxel_engine_debug_menu )
	{
		VXExt::ImGui_DrawVoxelEngineDebugHUD(
			app->world.voxels.voxel_world
			, app->gameplay.player.camera._eye_position
			, app->gameplay.player.camera.m_look_direction
			, app->world.voxels.voxel_engine_dbg_drawer
			);
	}
#endif

}

void NwGameState_ImGui_DebugHUD::_ImGUI_DrawDeveloperHUD()
{
	if( ImGui::Begin("Developer HUD") )
	{
		SgGameApp& app = SgGameApp::Get();

		SgDeveloperSettings& dev_settings = app.user_settings.developer;

		ImGui::Checkbox("Show BVH?", &dev_settings.visualize_BVH);

		ImGui::Checkbox("Multithreaded Update?", &dev_settings.use_multithreaded_update);
		
		//
		if( ImGui::Button("Spawn Ally Fighter Ship at Current Pos" ) )
		{
			app.gameplay.DbgSpawnFriendlyShip(
				app.renderer.scene_view.eye_pos
				);
		}

		//
		if( ImGui::Button("Spawn Enemy Fighter Ship at Current Pos" ) )
		{
			app.gameplay.DbgSpawnEnemyShip(
				app.renderer.scene_view.eye_pos
				);
		}

		//
		if( ImGui::Button("Spawn Ally Heavy Battleship" ) )
		{
			app.gameplay.DbgSpawnFriendlyShip(
				app.renderer.scene_view.eye_pos
				, obj_heavy_battleship_ship
				);
		}
	}
	ImGui::End();
}

void DevConsoleState::Update(
	const NwTime& game_time
	, NwAppI* game_app
	)
{
}

void DevConsoleState::DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
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
