//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <Voxels/private/debug/vxp_debug.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include <Engine/Private/EngineGlobals.h>

#include <Developer/DevTools/ImGuiHUD.h>

#include "app.h"
#include "rendering/renderer_data.h"
#include "app_states/game_states.h"
#include "app_states/game_state_debug_hud.h"


static V3f		gs_dbg_bvh_hit_pos;
static bool		gs_dbg_did_hit_bvh = false;

/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugHUD
-----------------------------------------------------------------------------
*/
NwGameState_ImGui_DebugHUD::NwGameState_ImGui_DebugHUD()
{
	this->DbgSetName("DebugHUD");
}


EStateRunResult NwGameState_ImGui_DebugHUD::handleInput(
	NwAppI* app
	, const NwTime& game_time
)
{
	MyApp* game = checked_cast< MyApp* >( app );

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

void NwGameState_ImGui_DebugHUD::Update(
								const NwTime& game_time
								, NwAppI* app
								)
{
	ImGui_Renderer::UpdateInputs();
}

void NwGameState_ImGui_DebugHUD::DrawUI(
								  const NwTime& game_time
								  , NwAppI* app
								  )
{
	MyApp* game = checked_cast< MyApp* >( app );

	MyAppSettings & app_settings = game->settings;


#if 0
	if(ImGui::Button("Clear Debug Lines"))
	{
		game->voxel_mgr.ClearDebugLines();
	}

	//
	ImGui::Checkbox("Show Debug Text", &app_settings.ui_show_debug_text);
#endif

	ImGui::Checkbox("Show Engine Settings", &app_settings.dev_show_engine_settings);
	if(app_settings.dev_show_engine_settings)
	{
		DevTools::Draw_ImGUI_HUD();
	}

	//
	ImGui::Checkbox("Show Voxel Engine Debug Menu", &app_settings.dev_show_voxel_engine_debug_menu);
	


	//
	ImGui::Checkbox("Draw Sky", &app_settings.draw_skybox);
	//
	{
		//ImGui::InputFloat3("Sun dir", app_settings.sun_light.direction.a);
		{
			const bool changed = ImGui::DragFloat3("Sun dir", app_settings.sun_light.direction.a, 0.1f, -1.0f, +1.0f, "%.3f");
			app_settings.sun_light.direction = app_settings.sun_light.direction.normalized();
			if(changed)
			{
				Rendering::VXGI::VoxelGrids& vxgi = game->renderer._data->voxel_grids;
				vxgi.cpu.clipmap.SetAllCascadesDirty();
			}
		}

		if(ImGui::Button("Copy Sun Direction from Camera"))
		{
			app_settings.sun_light.direction = game->player.camera.m_look_direction;

			Rendering::VXGI::VoxelGrids& vxgi = game->renderer._data->voxel_grids;
			vxgi.cpu.clipmap.SetAllCascadesDirty();
		}
	}





	//
#if GAME_CFG_WITH_VOXEL_GI
	if( ImGui::Begin("VXGI") )
	{
		Rendering::VXGI::VoxelGrids& vxgi = game->renderer._data->voxel_grids;
		Rendering::VXGI::IrradianceField& light_probe_grid = game->renderer._data->irradiance_field;

		//
		if(ImGui::Button("Re-Voxelize And Update Probes"))
		{
			vxgi.cpu.clipmap.SetAllCascadesDirty();

			//
			light_probe_grid.SetDirty();
		}

		//
		ImGui::Checkbox("Show BVH", &app_settings.dbg_show_bvh);
		ImGui::InputInt("Max BVH level to show", &app_settings.dbg_max_bvh_depth_to_show);

		static bool dbg_raycast_bvh = false;
		if(ImGui::Button("Cast Ray against BVH"))
		{
			gs_dbg_did_hit_bvh = vxgi.voxel_BVH->DebugIntersectGpuBvhAabbsOnCpu(
				gs_dbg_bvh_hit_pos
				, game->renderer.GetCameraView()
				);

			if(gs_dbg_did_hit_bvh) {
				DBGOUT("DID HIT!!!");
			} else {
				DBGOUT("NO HIT.");
			}
		}

		if(gs_dbg_did_hit_bvh) {
			Rendering::Globals::GetImmediateDebugRenderer().DrawSolidSphere(
				gs_dbg_bvh_hit_pos
				, 1.0f
				);
		}

		//
		ImGui::Separator();
		//

		{
			Rendering::VXGI::RuntimeSettings& vxgi_settings = NEngine::g_settings.rendering.vxgi;
			const EditPropertyGridResult result = ImGui_DrawPropertyGridT(
				&vxgi_settings,
				"VXGI Settings"
				);
			if(result != PropertiesDidNotChange)
			{
				vxgi_settings.FixBadValues();
				vxgi.ApplySettings(vxgi_settings);
			}
		}

		//
		ImGui::Separator();
		//

		{
			Rendering::VXGI::IrradianceFieldSettings& ddgi_settings = NEngine::g_settings.rendering.ddgi;
			const EditPropertyGridResult result = ImGui_DrawPropertyGridT(
				&ddgi_settings,
				"DDGI Settings"
				);
			if(result != PropertiesDidNotChange)
			{
				ddgi_settings.FixBadValues();
				light_probe_grid.ApplySettings(ddgi_settings);
			}
		}
	}
	ImGui::End();
#endif // GAME_CFG_WITH_VOXEL_GI



	//
	if( app_settings.dev_show_voxel_engine_debug_menu )
	{
		VXExt::ImGui_DrawVoxelEngineDebugHUD(
			(VX5::WorldI&)game->voxel_mgr.Test_GetOctreeWorld()
			, game->settings.voxel_world_debug_settings

			, game->player.camera._eye_position
			, game->player.camera.m_look_direction
			, game->voxel_mgr.GetDbgView()
			);
	}


#if 0
	ImGui::Checkbox("Show Game Settings", &app_settings.ui_show_game_settings);
	ImGui::Checkbox("Show Cheats", &app_settings.ui_show_cheats);

	//ImGui::Checkbox("Show this HUD (F1)", &app_settings.ui_show_debug_text);

	ImGui::Checkbox("Show Scene Settings", &app_settings.dev_showSceneSettings);
	ImGui::Checkbox("Show Graphics Debugger", &app_settings.dev_showGraphicsDebugger);
	ImGui::Checkbox("Show Renderer Settings", &app_settings.dev_showRendererSettings);


	ImGui::Checkbox("Show GI Settings", &app_settings.dev_show_gi_hud);

	ImGui::Checkbox("Show Voxel Engine Settings", &app_settings.dev_show_voxel_engine_settings);
	

	ImGui::Checkbox("Show Tweakables", &app_settings.dev_showTweakables);
	ImGui::Checkbox("Show Perf Stats", &app_settings.dev_showPerfStats);
	ImGui::Checkbox("Show Memory Usage", &app_settings.dev_showMemoryUsage);

	//
//	ImGui::InputDouble("Test PlaneOffsetZ", &game->world.voxels._test_plane.abcd.w);

	ImGui::InputInt3("Debug Int Coords", (int*)VX5::g_dbg_cell_coords.a);
	ImGui::InputInt("Debug Axis", (int*)&VX5::g_dbg_axis);
	//ImGui_drawEnumComboBoxT()

	//if(ImGui::Button("Load Test Level"))
	//{
	//	MyApp::Get().LoadLevelNamed("test_level");
	//}


	//
	if( ImGui::Begin("Camera") )
	{
		NwFlyingCamera * camera = & game->player.camera;
		V3d * camera_pos = & camera->_eye_position;
		//
		//ImGui::Text("Eye position: %f, %f, %f",
		//	scene_view.eye_pos.x, scene_view.eye_pos.y, scene_view.eye_pos.z
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

		//ImGui::Spacing();
		ImGui::Separator();
	}
	ImGui::End();


	if( app_settings.ui_show_game_settings )
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

	if(app_settings.ui_show_cheats)
	{
		if( ImGui::Begin("Cheats") )
		{
			ImGui::Checkbox("Godmode", &game->settings.cheats.godmode_on);
			ImGui::Checkbox("Noclip", &game->settings.cheats.enable_noclip);
		}
		ImGui::End();
	}


#if MX_DEVELOPER
	//
	if( app_settings.dev_showTweakables )
	{
		ImGui_DrawTweakablesWindow();
	}
#endif


	//
	if( app_settings.dev_showSceneSettings )
	{
		if( ImGui::Begin("Scene Settings") )
		{
#if 0
			bool sunLightChanged = false;

			if( ImGui::Button("Copy Camera Look Dir to Sun Direction" ) )
			{
				app_settings.sun_light.direction = camera.m_look_direction;
				sunLightChanged = true;
			}
			if( ImGui::Button("Reset Sun Direction" ) )
			{
				app_settings.sun_light.direction = -V3_UP;
				sunLightChanged = true;
			}

			{
				V3f newSunLightDir = app_settings.sun_light.direction;

				ImGui::SliderFloat("SunDirX", &newSunLightDir.x, -1, 1);
				ImGui::SliderFloat("SunDirY", &newSunLightDir.y, -1, 1);
				ImGui::SliderFloat("SunDirZ", &newSunLightDir.z, -1, 1);

				newSunLightDir = V3_Normalized( newSunLightDir );
				if( newSunLightDir != app_settings.sun_light.direction )
				{
					app_settings.sun_light.direction = newSunLightDir;
					sunLightChanged = true;
				}
			}

			//float luminance = RGBf(globalLight->color.x, globalLight->color.y, globalLight->color.z).Brightness();
			//ImGui::SliderFloat("SunLightBrightness", &globalLight->brightness, 0, 100);
			//globalLight->color *= luminance;

			if( sunLightChanged )
			{
				_render_world->setGlobalLight( app_settings.sun_light );
				_render_world->modifySettings( app_settings.renderer );
			}
#endif
			ImGui_DrawPropertyGrid(
				&app_settings,
				mxCLASS_OF(app_settings),
				"App Settings"
				);
		}
		ImGui::End();
	}

	//
	if( app_settings.dev_showRendererSettings )
	{
		if( ImGui::Begin("Renderer") )
		{
			//ImGui::SliderFloat(
			//	"Animation Speed"
			//	, &app_settings.renderer.animation.animation_speed_multiplier
			//	, RrAnimationSettings::minValue()
			//	, RrAnimationSettings::maxValue()
			//	);

			////
			//if( ImGui::Button("Regenerate Light Probes" ) )//zzz
			//{
			//	((Rendering::NwRenderSystem&)Rendering::Globals::)._test_need_to_relight_light_probes = true;
			//}

			//
			//const U64 rendererSettingsHash0 = app_settings.renderer.computeHash();

			//
			//const U64 rendererSettingsHash1 = app_settings.renderer.computeHash();
			//if( rendererSettingsHash0 != rendererSettingsHash1 )
			//{
			//	DEVOUT("Renderer Settings changed");
			//	game->world.spatial_database->modifySettings( app_settings.renderer );
			//}

//			ImGui::Text("Objects visible: %u / %u (%u culled)", g_FrontEndStats.objects_visible, g_FrontEndStats.objects_total, g_FrontEndStats.objects_culled);
			//ImGui::Text("Shadow casters rendered: %u", g_FrontEndStats.shadow_casters_rendered);
			//ImGui::Text("Meshes: %u", Rendering::NwMesh::s_totalCreated);
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
		game->renderer.ApplySettings( app_settings.renderer );
	}//Renderer


	//
	if( app_settings.dev_show_gi_hud )
	{
		game->renderer.Dbg_DrawImGui_VoxelGI();
	}

	//
	if( app_settings.dev_show_voxel_engine_settings )
	{
		if( ImGui::Begin("Voxel Engine Settings") )
		{
			//
			if( ImGui::Button("Save To File") )
			{
				game->SaveSettingsToFiles();
			}

			//
			//ImGui::InputDouble( "Noise Scale", &game->world._fast_noise_scale );

			//
			if( ImGui::Button("Regenerate World") )
			{
				game->voxel_mgr.regenerateVoxelTerrains();
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
				const U64 voxel_engine_settings_hash_before = CalcPodHash(app_settings.voxels.engine);
				ImGui_DrawPropertyGrid(
					&app_settings.voxels.engine,
					mxCLASS_OF(app_settings.voxels.engine),
					"Voxel Engine Settings"
					);
				const U64 voxel_engine_settings_hash_after = CalcPodHash(app_settings.voxels.engine);
				if(voxel_engine_settings_hash_before != voxel_engine_settings_hash_after)
				{
					DBGOUT("Voxel Engine Settings changed!!!");
					game->voxel_mgr.ApplySettings(app_settings.voxels.engine);
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



		//if( app_settings.ui_show_planet_settings )
		//{
		//	Planets::PlanetProcGenData & planet_proc_gen_data = game->world._test_voxel_planet_procgen_data;

		//	//
		//	ImGui::InputDouble("Radius", &planet_proc_gen_data.sphere_zero_height.radius);
		//	ImGui::InputDouble("Max Height", &planet_proc_gen_data.max_height);
		//}
	}//Voxel Engine



#endif
}

void DevConsoleState::Update(
	const NwTime& game_time
	, NwAppI* app
	)
{
}

void DevConsoleState::DrawUI(
		const NwTime& game_time
		, NwAppI* app
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
