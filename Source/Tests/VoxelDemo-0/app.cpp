#include "stdafx.h"
#pragma hdrstop

#include <inttypes.h>	// PRIu64
#include <Base/Memory/ScratchAllocator.h>
#include <Base/Memory/Allocators/FixedBufferAllocator.h>

#include <Base/Template/Algorithm/Sorting/InsertionSort.h>

#include <Core/ObjectModel/ClumpInfo.h>
#include <GPU/Public/graphics_system.h>

#include <Engine/WindowsDriver.h>	// Windows stuff
#include <Engine/Private/EngineGlobals.h>

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>	// Debug UI
#include <Utility/GUI/Nuklear/Nuklear_Support.h>	// In-Game UI
#include <Utility/GameFramework/NwGameGUI.h>
#include <Core/Serialization/Text/TxTSerializers.h>

//
#include <Scripting/Scripting.h>
#include <Scripting/ScriptResources.h>
#include <Scripting/Lua/LuaScriptingEngine.h>

#include <Engine/Model.h>

//
#include <Developer/RunTimeCompiledCpp.h>

#include <Voxels/private/base/math/vx_cube_geometry.h>

#include "app.h"
#include "app_states/game_states.h"
#include "experimental/rccpp.h"
#include "rendering/renderer_data.h"


namespace
{
	const char* PATH_TO_SAVED_ENGINE_SETTINGS = PP_JOIN(MY_GAME_NAME,".engine.cfg");
	const char* PATH_TO_SAVED_APP_SETTINGS = PP_JOIN(MY_GAME_NAME,".app.cfg");
	const char* PATH_TO_SAVED_INGAME_SETTINGS = PP_JOIN(MY_GAME_NAME, ".user.cfg");

	// Dbg
	const char* PATH_TO_SAVED_CAMERA_STATE = PP_JOIN(MY_GAME_NAME, ".saved_camera.son");
}



MyApp::MyApp( AllocatorI & allocator )
	: _allocator( allocator )
	, world( allocator )
#if !GAME_CFG_WITH_SOUND
	, sound(*(NwSoundSystemI*)nil)
#endif // !GAME_CFG_WITH_SOUND
{
	is_paused = false;

	//dbg_target_pos = CV3f(0);

	states.ConstructInPlace();

	current_loading_progress = 0;
	_player_reached_mission_exit = false;
}

MyApp::~MyApp()
{
	states.Destruct();
}

ERet MyApp::Initialize( const NEngine::LaunchConfig & engine_launch_config )
{
	AllocatorI& scratchpad = MemoryHeaps::temporary();


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	mxDO(RunTimeCompiledCpp::Initialize( &g_SystemTable ));

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP


	//
	mxDO(DemoApp::Initialize(engine_launch_config));

	//
#if GAME_CFG_WITH_SOUND
	mxDO(sound.Initialize(&runtime_clump));
#endif // GAME_CFG_WITH_SOUND

	//
	mxDO(states->Setup());
	mxDO(state_mgr.Initialize());

	is_paused = false;


	mxDO(input.Initialize(
		_allocator
		));

	//
	mxDO(renderer.Initialize(
		engine_launch_config
		, NEngine::g_settings.rendering
		, &runtime_clump
		));

	//
//	mxDO(Scripting::Initialize());

	//
	mxDO(gui.Initialize(
		_allocator
		, mxKiB(512)// engine_launch_config.game_UI_mem
		));

	//
	mxDO(world.Initialize(
		settings
		, &runtime_clump
		, scratchpad
		));

	mxDO(voxel_mgr.Initialize(
		engine_launch_config
		, settings
		, &runtime_clump
		, world.spatial_database
		, &renderer._data->voxel_grids
		));

	// Disable world origin shifting! (not supported by physics/audio)
	mxREFACTOR(":");
	NEngine::g_settings.rendering.floating_origin_threshold = BIG_NUMBER;

	//world.spatial_database->modifySettings( NEngine::g_settings.rendering );

	//
	state_mgr.pushState( &states->state_main_game );

	//
	mxDO(player.Init(
		runtime_clump
		, world
		, settings
		));

	return ALL_OK;
}

void MyApp::Shutdown()
{

#if 0//MX_DEVELOPER && !GAME_CFG_RELEASE_BUILD

	Resources::DumpUsedAssetsList("R:/");

	//
	{
		Resources::CopyUsedAssetsParams	params;
		params.used_assets_manifest_file = "R:/used_assets.son";
		params.src_assets_folder = "R:/NoobWerks/.Build/Assets/";
		params.dst_assets_folder = "R:/Half-Assed-alpha/Assets/";

		Resources::CopyOnlyUsedAssets(
			params
			, MemoryHeaps::temporary()
			);
	}

#endif // MX_DEVELOPER

	player.unload(runtime_clump);

	voxel_mgr.Shutdown();

	world.Shutdown();

	gui.Shutdown();

	renderer.Shutdown();

	input.Shutdown();

	//
	state_mgr.Shutdown();
	states->Teardown();

#if GAME_CFG_WITH_SOUND
	sound.Shutdown();
#endif // GAME_CFG_WITH_SOUND

	//
	DemoApp::Shutdown();


//	Scripting::Shutdown();

	//
	runtime_clump.clear();


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::Shutdown();

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

}

void MyApp::_handleInput(
	const NwTime& game_time
	)
{
	input.UpdateOncePerFrame();

	if(input._input_system->getState().window.has_focus)
	{
		state_mgr.handleInput( this, game_time );

		Nuklear_processInput( game_time.real.delta_seconds );
	}
}

void MyApp::RunFrame(
	const NwTime& game_time
)
{
	update(game_time);
	render(game_time);
}

void MyApp::update(
	const NwTime& game_time
)
{
	//rmt_ScopedCPUSample(updateGame, 0);
	//PROFILE;


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::update( game_time.real.delta_seconds );

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP


	//DBGOUT("UPDATE GAME: frame=%" PRIu64 "", game_time.frame_number);

	_handleInput( game_time );



	//
	_update_FPS_counter.add( game_time.real.delta_seconds );

	//
	NwGameStateI* current_game_state = state_mgr.getTopState();

	mxFIXME("must pause objects, but UpdateOncePerFrame render world!")
	//if( !current_game_state->pausesGame() )
	{
		voxel_mgr.UpdateOncePerFrame(
			game_time
			, player
			, settings
			, *this
			, player.camera._eye_position
			);

		//
		world.UpdateOncePerFrame(
			game_time
			, player
			, settings
			, *this
			);

		// NOTE: player must be updated after the world is updated,
		// because player camera uses character controller position.
		player.UpdateOncePerFrame(
			game_time
			//, settings.cheats
			);
	}


	//
	current_game_state->Update(
		game_time
		, this
		);



	//
#if GAME_CFG_WITH_SOUND

	{
		const NwFlyingCamera& camera = player.camera;

		V3f	listener_pos_ws = V3f::fromXYZ(camera._eye_position);

		//if(player._current_weapon!=nil)
		//{
		//	listener_pos_ws = player._current_weapon->
		//		anim_model->render_model->local_to_world.translation()
		//		;
		//}

		listener_pos_ws = player.GetEyePosition();


		//
		sound.UpdateListener(
			
			listener_pos_ws

			, camera.m_look_direction
			, camera.m_upDirection

#if GAME_CFG_WITH_PHYSICS
			, &fromBulletVec(player.phys_char_controller.getLinearVelocity())
#else
			mxTODO("velocity")
			, nil
#endif
			);
	}

	sound.UpdateOncePerFrame();

#endif // GAME_CFG_WITH_SOUND



	//
	if(_player_reached_mission_exit)
	{
		if(state_mgr.getTopState() != &states->game_completed)
		{
			state_mgr.pushState(
				&states->game_completed
				);
		}
	}


	// Hot-Reload assets.

#if MX_DEVELOPER

	//
	this->dev_ReloadChangedAssetsIfNeeded();

#endif



#if 0
	//
	_render_system->update( input_state.frameNumber, input_state.totalSeconds * 1000 );

	//
	//if( state_mgr.CurrentState() == GameStates::main ) {
	//	camera.processInput(input_state);
	//}

	this->_processInput( input_state );

	camera.Update(input_state.delta_time_seconds);

	//

	const M44f camera_world_matrix = M44_OrthoInverse( camera.BuildViewMatrix() );

	player.update(
		input_state
		, camera_world_matrix
		, m_settings.renderer
		, Rendering::NwRenderSystem::Get()._debug_line_renderer
		);

	//
	const V3d region_of_interest_position = m_settings.use_fake_camera
		? V3d::fromXYZ( m_settings.fake_camera.origin )
		: getCameraPosition()
		;
	//
	_render_world->update(
		V3d::fromXYZ( region_of_interest_position )
		, microsecondsToSeconds( Testbed::gameTimeElapsed() )
		);
	//_render_world->setGlobalShadowConfig( _render_system->_global_shadow );
	//_render_system->_global_shadow.dirty_cascades = 0;
#endif
}

void MyApp::render(
					 const NwTime& game_time
					 )
{
	//DBGOUT("RENDER GAME: frame=%" PRIu64 "", game_time.frame_number);

	//
	_render_FPS_counter.add( game_time.real.delta_seconds );


	//
	renderer.BeginFrame(
		game_time
		, world
		, settings
		, user_settings
		, player
		);

	renderer.RenderFrame(
		world
		, game_time
		, settings
		, user_settings
		, player
		);

	//
	state_mgr.DrawScene(
		this
		, game_time
		);

	gui.Draw(
		state_mgr
		, game_time
		, this
		);
}

void MyApp::SetDefaultSettings()
{
	NwFlyingCamera & camera = player.camera;
	camera._eye_position = V3d::set(0, -100, 20);
	camera.m_movementSpeed = 10.0f;
	//camera.m_strafingSpeed = camera.m_movementSpeed;
	camera.m_rotationSpeed = 0.5;
	camera.m_invertYaw = true;
	camera.m_invertPitch = true;

	NEngine::g_settings.rendering.sceneAmbientColor = V3_Set(0.4945, 0.465, 0.5) * 0.3f;

	//
	//settings.sun_direction = -V3_UP;
	//settings.sun_light.direction = V3_Normalized(V3_Set(1,1,-1));
	settings.sun_light.direction = V3_Normalized(V3_Set(-0.4,+0.4,-1));
//settings.sun_light.direction = V3_Set(1,0,0);
	if(0)
		settings.sun_light.color = V3_Set(0.5,0.7,0.6)* 0.1f;	// night
	else
		settings.sun_light.color = V3_Set(0.7,0.7,0.7);	// day
	//settings.sun_light.color = V3_Set(0.3,0.3,0.3);	// day

//settings.sun_light.color = V3_Set(0,1,0);
	settings.sun_light.ambientColor = V3_Set(0.4945, 0.465, 0.5);// * 0.1f;
	//settings.sun_light.ambientColor = V3f::set(0.017, 0.02, 0.019);
	//settings.sun_light.ambientColor = V3f::set(0.006, 0.009, 0.015);
	//settings.sun_light.ambientColor = V3f::zero();





#if 0
	//
	const F32 terrainRegionSize = 8;

	//
	{
		RrShadowSettings &shadowSettings = NEngine::g_settings.rendering.shadows;
		shadowSettings.num_cascades = MAX_SHADOW_CASCADES;

		mxSTATIC_ASSERT( MAX_SHADOW_CASCADES <= RrOffsettedCascadeSettings::MAX_CASCADES );

		float scale = 1;//0.3f;
		for( int i = 0; i < MAX_SHADOW_CASCADES; i++ )
		{
			float lodScale = ( 1 << i );
			shadowSettings.cascade_radius[i] = terrainRegionSize * lodScale * scale;
		}
	}

	//
	{
		RrSettings_VXGI &vxgiSettings = NEngine::g_settings.rendering.gi;
		vxgiSettings.num_cascades = 1;

		float scale = 2;
		for( int i = 0; i < RrOffsettedCascadeSettings::MAX_CASCADES; i++ )
		{
			float lodScale = ( 1 << i ) * 0.5f;
			vxgiSettings.cascade_radius[i] = terrainRegionSize * lodScale;
			vxgiSettings.cascade_max_delta_distance[i] = terrainRegionSize * lodScale * scale;
		}
	}
#endif
}

void MyApp::LoadSettingsFromFiles()
{
	AllocatorI &	temp_allocator = MemoryHeaps::temporary();

	//
	SON::LoadFromFile( PATH_TO_SAVED_ENGINE_SETTINGS, NEngine::g_settings, temp_allocator );
	SON::LoadFromFile( PATH_TO_SAVED_APP_SETTINGS, settings, temp_allocator );
	SON::LoadFromFile( PATH_TO_SAVED_CAMERA_STATE, player.camera, temp_allocator );

	//
	NEngine::g_settings.FixBadValues();
	settings.FixBadValues();
}

void MyApp::SaveSettingsToFiles()
{
	DBGOUT("MyApp::SaveSettingsToFiles()");

#if !GAME_CFG_RELEASE_BUILD
	SON::SaveToFile( NEngine::g_settings, PATH_TO_SAVED_ENGINE_SETTINGS );
	SON::SaveToFile( settings, PATH_TO_SAVED_APP_SETTINGS );
	SON::SaveToFile( player.camera, PATH_TO_SAVED_CAMERA_STATE );
#endif

}

void MyApp::ApplyUserDefinedInGameSettings()
{

#if GAME_CFG_WITH_SOUND

	sound.SetVolume(SND_CHANNEL_MAIN,
		user_settings.ingame.sound.effects_volume / 100.0f
		);
	sound.SetVolume(SND_CHANNEL_MUSIC,
		user_settings.ingame.sound.music_volume / 100.0f
		);

#endif // GAME_CFG_WITH_SOUND

}

ERet MyApp::SaveUserDefinedInGameSettingsToFile() const
{
	DBGOUT("Saving user-defined in-game settings...");

	mxDO(SON::SaveToFile(
		user_settings
		, PATH_TO_SAVED_INGAME_SETTINGS
		));
	return ALL_OK;
}

static
ERet LoadUserGameSettingsFile(MyGameUserSettings &game_user_settings_)
{
	TempAllocator8192	temp_allocator( &MemoryHeaps::process() );

	mxDO(SON::LoadFromFile(
		PATH_TO_SAVED_INGAME_SETTINGS
		, game_user_settings_
		, temp_allocator
		));

	game_user_settings_.window.FixBadValues();

	return ALL_OK;
}





ERet gameEntryPoint()
{
#if 0
	//signed char a = -2;
	//signed char b = -3;
	signed char a = 254;
	signed char b = 253;
	printf("%d + %d = %ld\n", a, b, a+b);
	printf("%d + %d = %u\n", a, b, a+b);

	unsigned char au = a;
	unsigned char bu = b;

	U32 max_u32 = ~0;
	U32 minus_2 = ~0 - 2;
	int minus_2_natural = -2;
#endif

	//Sorting::Test_InsertionSort();
	//UnitTests::Test_BinarySearch();

	//VX5::LUTs::Print__touched_external_faces_mask8__to__cell_type27();
	//VX5::LUTs::Print__cube_neighbor_type_to_touched_cube_faces();

	//
	NEngine::LaunchConfig  engine_launch_config;
	/*mxDO*/(engine_launch_config.LoadFromFile());

	//
	MyGameUserSettings	game_user_settings;
	// no error - use default settings
	/*mxDO*/(LoadUserGameSettingsFile(game_user_settings));

	// override engine settings with game settings
	game_user_settings.window.name = MY_GAME_NAME;
	engine_launch_config.window = game_user_settings.window;

	//
	mxDO(g_engine.Initialize(
		engine_launch_config
		));


	//
	{
		MyApp game( MemoryHeaps::global() );

		// remember the loaded settings
		game.user_settings = game_user_settings;

		game.SetDefaultSettings();
		game.LoadSettingsFromFiles();

		mxDO(game.Initialize(engine_launch_config));

		game.ApplyUserDefinedInGameSettings();

		//
#if 0//GAME_CFG_RELEASE_BUILD
		game.LoadLevelNamed("test_level");
#endif
		//MyLevelData_::CreateMissionExitIfNeeded(CV3f(0,0,18));



#if 0
		NwBlob	src_code_blob(MemoryHeaps::temporary());
		mxDO(NwBlob_::loadBlobFromFile(src_code_blob, "R:/NoobWerks/Assets/_fps_game/script/npc_trite.lua"));
		//
		NwScript	script;
		mxDO(Arrays::copyFrom(script.code, src_code_blob));
		script.sourcefilename = MakeAssetID("npc_trite");

		//
		LuaScriptingEngine* lse = (LuaScriptingEngine*) g_engine.scripting._ptr;
		lse->StartCoroutine(script, "Count");
#endif

		//
		NwSimpleGameLoop	game_loop;
		WindowsDriver::run( &game, &game_loop );

		// save settings before shutting down, because Shutdown() may fail in render doc (D3D Debug RunTime)
		game.SaveSettingsToFiles();

		//
		game.Shutdown();
	}

	//
	g_engine.Shutdown();

	return ALL_OK;
}
