#include "stdafx.h"
#pragma hdrstop

#include <inttypes.h>	// PRIu64
#include <Base/Memory/ScratchAllocator.h>
#include <Base/Memory/Allocators/FixedBufferAllocator.h>

#include <Base/Template/Algorithm/Sorting/InsertionSort.h>

#include <Core/ObjectModel/ClumpInfo.h>
#include <Graphics/graphics_system.h>

#include <Engine/WindowsDriver.h>	// Windows stuff

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>	// Debug UI
#include <Utility/GUI/Nuklear/Nuklear_Support.h>	// In-Game UI
#include <Core/Serialization/Text/TxTSerializers.h>

//
#include <Engine/Model.h>

//
#include "game_app.h"
#include "app_states/app_states.h"
#include "experimental/rccpp.h"


namespace
{
	enum UpperDesignLimits
	{
		MAX_SPACESHIPS = 16384
		//MAX_SPACESHIPS = 4096
		//MAX_SPACESHIPS = 1024
		//MAX_SPACESHIPS = 64
	};

	const char* PATH_TO_SAVED_CAMERA_STATE = PP_JOIN(MY_GAME_NAME, ".saved_camera.son");
	const char* PATH_TO_SAVED_GAME_SETTINGS = PP_JOIN(MY_GAME_NAME,".engine_settings.son");
	const char* PATH_TO_SAVED_INGAME_SETTINGS = PP_JOIN(MY_GAME_NAME, ".cfg");
}



SgGameApp::SgGameApp( AllocatorI & allocator )
	: _allocator( allocator )
	, input_mgr( allocator )
	, world( allocator )
#if !GAME_CFG_WITH_SOUND
	, sound(*(NwSoundSystemI*)nil)
#endif // !GAME_CFG_WITH_SOUND
{
	is_paused = false;

	player_wants_to_start_again = false;

	//dbg_target_pos = CV3f(0);

	states.ConstructInPlace();

	current_loading_progress = 0;
}

SgGameApp::~SgGameApp()
{
	states.Destruct();
}

ERet SgGameApp::initialize( const NwEngineSettings& engine_settings )
{
	AllocatorI& scratchpad = MemoryHeaps::temporary();


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	mxDO(RunTimeCompiledCpp::initialize( &g_SystemTable ));

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP


	//
	mxDO(DemoApp::initialize(engine_settings));

	//
	mxDO(_PreallocateObjectListsInClump( runtime_clump ));


#if GAME_CFG_WITH_SOUND
	mxDO(sound.initialize(&runtime_clump));
#endif // GAME_CFG_WITH_SOUND

	//
	mxDO(state_mgr.Initialize());

	is_paused = false;


	mxDO(input_mgr.initialize());

	//
	mxDO(renderer.initialize(
		engine_settings
		, settings.renderer
		, &runtime_clump
		));

	//
	mxDO(states->state_debug_hud.Initialize(
		renderer._render_system
		));

	//
	mxDO(gui.Initialize(
		_allocator
		, mxKiB(512)// engine_settings.game_UI_mem
		));

	//
	mxDO(world.initialize(
		settings
		, &runtime_clump
		, renderer._render_system
		, scratchpad
		));


	//
	PhysConfig	phys_config;
	phys_config.max_bodies = MAX_SPACESHIPS;

	mxDO(physics_mgr.Initialize(
		phys_config
		, settings
		, &runtime_clump
		, MemoryHeaps::physics()
		));


	//
	SgGameplayConfig	gameplay_config;
	gameplay_config.max_spaceships = MAX_SPACESHIPS;

	mxDO(gameplay.Initialize(
		gameplay_config
		, settings
		, physics_mgr
		, runtime_clump
		, _allocator
		));



//	world._fast_noise_scale = settings.fast_noise_scale;

	//
	world.spatial_database->setGlobalLight( settings.sun_light );

	// Disable world origin shifting! (not supported by physics/audio)
	mxREFACTOR(":");
	settings.renderer.floating_origin_threshold = BIG_NUMBER;

	world.spatial_database->modifySettings( settings.renderer );

	//
	state_mgr.pushState( &states->state_main_game );



	//
#if 1
	mxDO(gameplay.StartNewGame(
		world
		, physics_mgr
		));
#else
	
//#if 0
	mxDO(gameplay.SetupTestScene(
		world
		, physics_mgr
		));
//#else
//	mxDO(gameplay.SetupLevel0(
//		world
//		, physics_mgr
//		));
//#endif

#endif

	CompleteLevelLoading();


	return ALL_OK;
}

void SgGameApp::shutdown()
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


	gameplay.Shutdown();

	physics_mgr.Shutdown();

	world.shutdown();

	gui.Shutdown();

	renderer.shutdown();

	input_mgr.shutdown();

	//
	states->state_debug_hud.Shutdown();
	state_mgr.Shutdown();

#if GAME_CFG_WITH_SOUND
	sound.shutdown();
#endif // GAME_CFG_WITH_SOUND

	//
	DemoApp::shutdown();

	//
	runtime_clump.clear();


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::shutdown();

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

}

void SgGameApp::_handleInput(
	const NwTime& game_time
	)
{
	input_mgr.UpdateOncePerFrame();

	if(input_mgr._input_system->getState().window.has_focus)
	{
		state_mgr.handleInput( this, game_time );

		Nuklear_processInput( game_time.real.delta_seconds );
	}
}

void SgGameApp::RunFrame(
	const NwTime& game_time
)
{
	if(player_wants_to_start_again)
	{
		player_wants_to_start_again = false;

		// yeah, it's bad, but it works
		states->u_died_game_over.player_chose_to_start_again = false;
		states->u_died_game_over.player_wants_to_continue_running_the_game_despite_mission_failure = false;

		states->game_completed.player_chose_to_start_again = false;
		states->game_completed.player_wants_to_continue_running_the_game = false;

		DBGOUT("Restarting the game...");

		gameplay.player.StopCameraMovement();

		gameplay.StartNewGame(
			world
			, physics_mgr
			);

		CompleteLevelLoading();

		return;
	}


	_UpdateFrame(game_time);
	_RenderFrame(game_time);

	gameplay.RemoveDeadShips(
		game_time
		, world
		, physics_mgr
		, sound
		);




	//
	{
		if(this->gameplay.IsGameMissionCompleted()
			&& this->state_mgr.getTopState() != &this->states->game_completed
			&& !this->states->game_completed.player_wants_to_continue_running_the_game
			)
		{
			this->gameplay.player.StopCameraMovement();

			this->state_mgr.pushState(
				&this->states->game_completed
				);
		}
	}
}

void SgGameApp::_UpdateFrame(
	const NwTime& game_time
)
{
	//rmt_ScopedCPUSample(updateGame, 0);
	//PROFILE;

	NwJobSchedulerI& job_sched
		= user_settings.developer.use_multithreaded_update
		? *(NwJobSchedulerI*) &NwJobScheduler_Parallel::s_instance
		: *(NwJobSchedulerI*) &NwJobScheduler_Serial::s_instance
		;

#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::update( game_time.real.delta_seconds );

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP


	//DBGOUT("UPDATE GAME: frame=%" PRIu64 "", game_time.frame_number);

	_handleInput( game_time );



	//
	_update_FPS_counter.add( game_time.real.delta_seconds );




	mxFIXME("must pause objects, but UpdateOncePerFrame render world!")

	const JobID h_job_update_physics = physics_mgr.UpdateOncePerFrame(
		game_time
		, this->is_paused
		, job_sched
		, gameplay.GetShipHandleMgr()
		);

	//
	job_sched.waitForAll();

	//
	world.UpdateOncePerFrame(
		game_time
		, gameplay.player
		, settings
		, *this
		);

	//
	gameplay.player.UpdateOncePerFrame(
		game_time
		, settings.cheats
		);


	//
	{
		gameplay.UpdateOncePerFrame(
			game_time
			, user_settings
			, Vector4_Load(renderer.scene_view.eye_pos.a)
			, world
			, physics_mgr
			, sound
			);

		// allow the player to rotate the camera even when the game is paused
		if(gameplay.spaceship_controller.IsControllingSpaceship())
		{
			gameplay.spaceship_controller.Update(
				game_time
				, gameplay
				, physics_mgr
				);
		}
	}



	//
	NwGameStateI* current_game_state = state_mgr.getTopState();


	//
	current_game_state->Update(
		game_time
		, this
		);


	//
#if GAME_CFG_WITH_SOUND

	{
		const NwFlyingCamera& camera = gameplay.player.camera;

		const V3f listener_pos_ws = V3f::fromXYZ(camera._eye_position);

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



	// Hot-Reload assets.

#if MX_DEVELOPER

	//
	this->dev_ReloadChangedAssetsIfNeeded();

#endif

}

void SgGameApp::_RenderFrame(
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
		, gameplay
		);

	//
	state_mgr.DrawScene(
		this
		, game_time
		);

	gui.Draw(
		state_mgr
		, *renderer._render_system
		, game_time
		, this
		);
}

void SgGameApp::setDefaultSettingsAfterEngineInit()
{
	NwFlyingCamera & camera = gameplay.player.camera;
	camera._eye_position = V3d::set(0, -100, 20);
	camera.m_movementSpeed = 10.0f;
	//camera.m_strafingSpeed = camera.m_movementSpeed;
	camera.m_rotationSpeed = 0.5;
	camera.m_invertYaw = true;
	camera.m_invertPitch = true;

	settings.renderer.sceneAmbientColor = V3_Set(0.4945, 0.465, 0.5) * 0.3f;

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





	//
	const F32 terrainRegionSize = 8;

	//
	{
		RrShadowSettings &shadowSettings = settings.renderer.shadows;
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
		RrSettings_VXGI &vxgiSettings = settings.renderer.gi;
		vxgiSettings.num_cascades = 1;

		float scale = 2;
		for( int i = 0; i < RrOffsettedCascadeSettings::MAX_CASCADES; i++ )
		{
			float lodScale = ( 1 << i ) * 0.5f;
			vxgiSettings.cascade_radius[i] = terrainRegionSize * lodScale;
			vxgiSettings.cascade_max_delta_distance[i] = terrainRegionSize * lodScale * scale;
		}
	}
}

void SgGameApp::loadSettingsFromFiles()
{
	DBGOUT("SgGameApp::loadSettingsFromFiles()");

	mxBUG("memory corruption when accessing mem block from TempAllocator8192");
#if 0
	TempAllocator8192	temp_allocator( &MemoryHeaps::process() );
#else
	AllocatorI &	temp_allocator = MemoryHeaps::temporary();
#endif

	SON::LoadFromFile( PATH_TO_SAVED_GAME_SETTINGS, settings, temp_allocator );
	SON::LoadFromFile( PATH_TO_SAVED_CAMERA_STATE, gameplay.player.camera, temp_allocator );

	settings.FixBadValues();
}

void SgGameApp::saveSettingsToFiles()
{
	DBGOUT("SgGameApp::saveSettingsToFiles()");

#if !GAME_CFG_RELEASE_BUILD
	//
	SON::SaveToFile( settings, PATH_TO_SAVED_GAME_SETTINGS );

	//
	SON::SaveToFile( gameplay.player.camera, PATH_TO_SAVED_CAMERA_STATE );
#endif

	// always remember developer settings
	SaveUserDefinedInGameSettingsToFile();
}

void SgGameApp::overrideSomeGameSettingsWithDebugValues()
{
}

ERet SgGameApp::_PreallocateObjectListsInClump( NwClump & clump )
{
#if 0
	clump.CreateObjectList(NwGeometryBuffer::metaClass(), 256);

	clump.CreateObjectList(NwMesh::metaClass(), 256);

	clump.CreateObjectList(RrMeshInstance::metaClass(), 256);
	clump.CreateObjectList(RrTransform::metaClass(), 256);
	
	clump.CreateObjectList(NwMaterial::metaClass(), 32);

	clump.CreateObjectList(NwRenderState::metaClass(), 8);

	clump.CreateObjectList(NwGeometryShader::metaClass(), 2);
	clump.CreateObjectList(RrMaterialInstance::metaClass(), 2);

	clump.CreateObjectList(NwCollisionObject::metaClass(), 256);
	clump.CreateObjectList(NwRigidBody::metaClass(), 128);

	clump.CreateObjectList(idMaterial::metaClass(), 64);

#if GAME_CFG_WITH_SOUND
	clump.CreateObjectList(NwAudioClip::metaClass(), 4);
	clump.CreateObjectList(NwSoundSource::metaClass(), 32);
#endif

	clump.CreateObjectList(NwTexture::metaClass(), 64);
	clump.CreateObjectList(idRenderModel::metaClass(), 64);
	clump.CreateObjectList(NwModelDef::metaClass(), 16);

	clump.CreateObjectList(NwShaderEffect::metaClass(), 8);
	clump.CreateObjectList(NwShaderProgram::metaClass(), 8);
	clump.CreateObjectList(NwPixelShader::metaClass(), 8);
	clump.CreateObjectList(NwVertexShader::metaClass(), 8);
	clump.CreateObjectList(NwModel::metaClass(), 64);
#endif

	//clump.CreateObjectList(RrLocalLight::metaClass(), 16);

	return ALL_OK;
}

ERet SgGameApp::_PrecacheResources()
{
	return ALL_OK;
}

ERet SgGameApp::LoadLevelNamed(
	const char* level_name
	)
{
	DBGOUT("Loading level '%s'...", level_name);
UNDONE;
	//Str::CopyS(name_of_level_being_loaded, level_name);

	////
	//FilePathStringT	path_to_level_file;
	//Str::ComposeFilePath(
	//	path_to_level_file
	//	, settings.path_to_levels.c_str()
	//	, name_of_level_being_loaded.c_str()
	//	, "son"
	//	);

	//// 0) Show loading screen.
	//// 1) Copy voxel data from Levels to userdata folder.
	//// 2) Load geometry.
	//// 3) Remove old monsters and items.
	//// 4) Spawn monsters and items.
	//// 5) Spawn player.
	//// -) Hide loading screen.

	////
	//FilePathStringT	path_to_voxel_file;
	//Str::ComposeFilePath(
	//	path_to_voxel_file
	//	, settings.path_to_levels.c_str()
	//	, level_name
	//	, "vxl"
	//	);

	//mxDO(world.voxels.voxel_world->LoadFromFile(path_to_voxel_file.c_str()));

	//// WAIT UNTIL EVERYTHING IS LOADED...

	//is_paused = true;

	//current_loading_progress = 0;
	//_PushLoadingScreen();

	return ALL_OK;
}

ERet SgGameApp::CompleteLevelLoading()
{
	gameplay.player.time_when_map_was_loaded = mxGetTimeInMilliseconds();

	//
	DBGOUT("Finished loading level '%s'...", name_of_level_being_loaded.c_str());
	name_of_level_being_loaded.empty();

	//
	_PopLoadingScreen();

	//
#if GAME_CFG_WITH_SOUND
	//
	gameplay.PlayAmbientMusic(sound, user_settings);

	// preload the audio clip to avoid hitches at runtime
	gameplay.PlayFightMusic(sound, user_settings);
#endif // GAME_CFG_WITH_SOUND


	//
	is_paused = false;

	return ALL_OK;
}

void SgGameApp::_PushLoadingScreen()
{
	if(state_mgr.getTopState() != &states->loading_progress)
	{
		state_mgr.pushState(
			&states->loading_progress
			);
	}
}

void SgGameApp::_PopLoadingScreen()
{
	if(state_mgr.getTopState() == &states->loading_progress)
	{
		state_mgr.popState();
	}
}

void SgGameApp::ApplyUserDefinedInGameSettings()
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

ERet SgGameApp::SaveUserDefinedInGameSettingsToFile() const
{
	DBGOUT("Saving user-defined in-game settings...");

	mxDO(SON::SaveToFile(
		user_settings
		, PATH_TO_SAVED_INGAME_SETTINGS
		));
	return ALL_OK;
}

static
ERet LoadUserGameSettingsFile(SgUserSettings &game_user_settings_)
{
	TempAllocator8192	temp_allocator( &MemoryHeaps::process() );

	mxTRY(SON::LoadFromFile(
		PATH_TO_SAVED_INGAME_SETTINGS
		, game_user_settings_
		, temp_allocator
		));

	game_user_settings_.window.FixBadSettings();

	return ALL_OK;
}





ERet gameEntryPoint()
{
	//
	NwEngineSettings engine_settings;
	/*mxDO*/(engine_settings.loadFromFile());

	//
	SgUserSettings	game_user_settings;
	// no error - use default settings
	/*mxDO*/(LoadUserGameSettingsFile(game_user_settings));

	//
	game_user_settings.window.name = MY_GAME_NAME;


	// override engine settings with game settings
	{
		engine_settings.window = game_user_settings.window;
	}

	//
	mxDO(g_engine.initialize(
		engine_settings
		));





#if 0

	const M44f	align_mesh_with_our_coord_system
		=
		M44_FromRotationMatrix(M33_RotationX(DEG2RAD(90)))
		//*
		//M44_FromRotationMatrix(M33_RotationZ(DEG2RAD(90)))
		//*
		//M44_FromRotationMatrix(M33_RotationY(DEG2RAD(180)))
		;

	SON::SaveToFile(align_mesh_with_our_coord_system, "R:/test.son");

	UNDONE;

#endif




	//
	{
		SgGameApp game( MemoryHeaps::global() );

		// remember the loaded settings
		game.user_settings = game_user_settings;

		game.setDefaultSettingsAfterEngineInit();
		game.loadSettingsFromFiles();
		game.overrideSomeGameSettingsWithDebugValues();

		mxDO(game.initialize(engine_settings));

		game.ApplyUserDefinedInGameSettings();

		//
#if 0//GAME_CFG_RELEASE_BUILD
		game.LoadLevelNamed("test_level");
#endif
		//MyLevelData_::CreateMissionExitIfNeeded(CV3f(0,0,18));


		//
		NwSimpleGameLoop	game_loop;
		WindowsDriver::run( &game, &game_loop );

		//
#if 0//MX_DEVELOPER
		//
		{
			ClumpInfo	clump_info( MemoryHeaps::global() );
			clump_info.buildFromClump( game.runtime_clump );
			clump_info.saveToFile("R:/runtime_clump.son");
		}
#endif

		// save settings before shutting down, because shutdown() may fail in render doc (D3D Debug RunTime)
		game.saveSettingsToFiles();

		//
		game.shutdown();
	}

	//
	g_engine.shutdown();

	return ALL_OK;
}
