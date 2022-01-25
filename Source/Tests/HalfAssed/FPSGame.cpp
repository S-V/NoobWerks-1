#include "stdafx.h"
#pragma hdrstop

#include <inttypes.h>	// PRIu64
#include <Base/Memory/ScratchAllocator.h>
#include <Base/Memory/Allocators/FixedBufferAllocator.h>
#include <Core/ObjectModel/ClumpInfo.h>
#include <Graphics/GraphicsSystem.h>

#include <Engine/WindowsDriver.h>	// Windows stuff

#include <Renderer/Renderer.h>
#include <Renderer/Pipeline/RenderPipeline.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>	// Debug UI
#include <Utility/GUI/Nuklear/Nuklear_Support.h>	// In-Game UI
#include <Core/Serialization/Text/TxTSerializers.h>

//
#include <Scripting/Scripting.h>
#include <Scripting/ScriptResources.h>
#include <Scripting/Lua/LuaScriptingEngine.h>

#include <Engine/Model.h>

//
#include <Developer/RunTimeCompiledCpp.h>

#include "FPSGame.h"
#include "game_ui/game_ui.h"
#include "game_states/game_states.h"
#include "experimental/rccpp.h"
#include "npc/npc_behavior_common.h"
#include "game_world/pickable_items.h"	// SpawnItem()
#include "gameplay_constants.h"	// PickableItem8


namespace
{
	const char* PATH_TO_SAVED_CAMERA_STATE = "saved_camera_state.son";
	const char* PATH_TO_SAVED_GAME_SETTINGS = "FPSGame.saved.son";

	const char* PATH_TO_SAVED_INGAME_SETTINGS = "Half-Assed.cfg";
}



FPSGame::FPSGame( AllocatorI & allocator )
	: _allocator( allocator )
	, world( allocator )
#if !GAME_CFG_WITH_SOUND
	, sound(*(NwSoundSystemI*)nil)
#endif // !GAME_CFG_WITH_SOUND
	, _loaded_level(MemoryHeaps::process())
{
	is_paused = false;

	//dbg_target_pos = CV3f(0);

	states.ConstructInPlace();

	current_loading_progress = 0;
	_curr_attack_wave = 0;
	_player_reached_mission_exit = false;
}

FPSGame::~FPSGame()
{
	states.Destruct();
}

ERet FPSGame::initialize( const NwEngineSettings& engine_settings )
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


	mxDO(input.initialize(
		_allocator
		));

	//
	mxDO(renderer.initialize(
		engine_settings
		, settings.renderer
		, &runtime_clump
		));

	//
//	mxDO(Scripting::Initialize());

	//
	mxDO(states->state_debug_hud.Initialize(
		renderer._render_system
		));

	//
	mxDO(states->world_editor.Initialize(
		settings
		));

	//
	mxDO(initialize_game_GUI(
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

//	world._fast_noise_scale = settings.fast_noise_scale;
	states->world_editor.SetVoxelWorld(world.voxels.voxel_world);

	//
	world.render_world->setGlobalLight( settings.sun_light );

	// Disable world origin shifting! (not supported by physics/audio)
	settings.renderer.floating_origin_threshold = DBL_MAX;

	world.render_world->modifySettings( settings.renderer );

	//
	state_mgr.pushState( &states->state_main_game );

	//
	mxDO(player.Init(
		runtime_clump
		, world
		, settings
		));


	//

	//
#if 0//GAME_CFG_WITH_SOUND
	sound.PlayAudioClip(
		MakeAssetID("loop_wind_02")
		, PlayAudioClip_Looped
		, 0.5f
		);
#endif


#if 0 //!GAME_CFG_RELEASE_BUILD
	//
	const V3f burning_fire_pos = CV3f(40,17,59);
	renderer.burning_fires.AddBurningFire(
		burning_fire_pos
		, 2
		, 0 // infinite life
		);
#if GAME_CFG_WITH_SOUND
	sound.PlaySound3D(
		MakeAssetID("burning_fire")
		, burning_fire_pos
		);
#endif
#endif


#if 0//!GAME_CFG_RELEASE_BUILD && MX_DEBUG
	world.NPCs.Spawn_Spider(
		world,
		CV3f(0,-2,2)//CV3f(38,17,62)
		);

	//world.NPCs.Spawn_Imp(
	//	world,
	//	CV3f(42,17,62)
	//	);

	//world.NPCs.Spawn_Mancubus(
	//	world,
	//	CV3f(0,3,2)
	//	);

	//world.NPCs.Spawn_Maggot(
	//	world,
	//	CV3f(0,5,2)
	//	);
	//world.NPCs.Spawn_Maggot(
	//	world,
	//	CV3f(0,7,2)
	//	);
#endif

	return ALL_OK;
}

void FPSGame::shutdown()
{

#if MX_DEVELOPER && !GAME_CFG_RELEASE_BUILD

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

	states->world_editor.SetVoxelWorld(nil);
	world.shutdown();

	shutdown_game_GUI();

	renderer.shutdown();

	input.shutdown();

	//
	states->state_debug_hud.Shutdown();
	state_mgr.Shutdown();

#if GAME_CFG_WITH_SOUND
	sound.shutdown();
#endif // GAME_CFG_WITH_SOUND

	//
	DemoApp::shutdown();


//	Scripting::Shutdown();

	//
	runtime_clump.clear();


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::shutdown();

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

}

void FPSGame::_handleInput(
	const NwTime& game_time
	)
{
	input.updateOncePerFrame();

	if(input._input_system->getState().window.has_focus)
	{
		state_mgr.handleInput( this, game_time );

		Nuklear_processInput( game_time.real.delta_seconds );
	}
}

void FPSGame::update(
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

#if GAME_CFG_WITH_PHYSICS
		{
			AI_Blackboard &ai_blackboard = AI_Blackboard::Get();

			//
			btRigidBody* player_char_ctrl_rb = player.phys_char_controller.getRigidBody();

			ai_blackboard.player_pos = player.GetEyePosition();
			ai_blackboard.is_player_alive = player.comp_health.IsAlive();

			//
			ai_blackboard.player_collision_object = player.phys_char_controller.getRigidBody();

			//
			const btBroadphaseProxy* player_char_ctrl_bp_proxy_bt = player_char_ctrl_rb->getBroadphaseProxy();
			//
			btVector3	player_aabb_min_with_margin = player_char_ctrl_bp_proxy_bt->m_aabbMin;
			btVector3	player_aabb_max_with_margin = player_char_ctrl_bp_proxy_bt->m_aabbMax;

			float	margin = 0.1f;
			AabbExpand(
				player_aabb_min_with_margin, player_aabb_max_with_margin,
				btVector3(-margin, -margin, -margin), btVector3(margin, margin, margin)
				);
			//UNDONE;
			//ai_blackboard.player_aabb_min_with_margin = player_aabb_min_with_margin;
			//ai_blackboard.player_aabb_max_with_margin = player_aabb_max_with_margin;

			ai_blackboard.player = &player;
		}
#endif // GAME_CFG_WITH_PHYSICS

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
			, settings.cheats
			);
	}


	//
	current_game_state->tick(
		game_time.real.delta_seconds
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

void FPSGame::render(
					 const NwTime& game_time
					 )
{
	//DBGOUT("RENDER GAME: frame=%" PRIu64 "", game_time.frame_number);

	//
	_render_FPS_counter.add( game_time.real.delta_seconds );

	//
	state_mgr.drawScene(
		this
		, game_time
		);


	// BEGIN DRAWING GUI
	{
		const U32	imgui_scene_pass_index = renderer._render_system->getRenderPath()
			.getPassDrawingOrder(mxHASH_STR("GUI"))
			;
		//
		ImGui_Renderer::BeginFrame(
			imgui_scene_pass_index
			, game_time.real.delta_seconds
			);

		//
		Nuklear_precache_Fonts_if_needed();


		// DRAW GUI

		//
		state_mgr.drawUI(
			game_time.real.delta_seconds
			, this
			);

		//
		const U32 scene_pass_index = renderer._render_system->getRenderPath()
			.getPassDrawingOrder(mxHASH_STR("HUD"))
			;
		Nuklear_render( scene_pass_index );

		//
		ImGui_Renderer::EndFrame();
	}
	// FINISH DRAWING GUI
}

void FPSGame::setDefaultSettingsAfterEngineInit()
{
	NwFlyingCamera & camera = player.camera;
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

void FPSGame::loadSettingsFromFiles()
{
	DBGOUT("FPSGame::loadSettingsFromFiles()");

	mxBUG("memory corruption when accessing mem block from TempAllocator8192");
#if 0
	TempAllocator8192	temp_allocator( &MemoryHeaps::process() );
#else
	AllocatorI &	temp_allocator = MemoryHeaps::temporary();
#endif

	SON::LoadFromFile( PATH_TO_SAVED_GAME_SETTINGS, settings, temp_allocator );
	SON::LoadFromFile( PATH_TO_SAVED_CAMERA_STATE, player.camera, temp_allocator );

	settings.FixBadValues();
}

void FPSGame::saveSettingsToFiles()
{
	DBGOUT("FPSGame::saveSettingsToFiles()");

#if !GAME_CFG_RELEASE_BUILD
	//
	SON::SaveToFile( settings, PATH_TO_SAVED_GAME_SETTINGS );

	//
	SON::SaveToFile( player.camera, PATH_TO_SAVED_CAMERA_STATE );
#endif

}

void FPSGame::overrideSomeGameSettingsWithDebugValues()
{
	settings.dbg_disable_parallel_voxels = 0;
}

ERet FPSGame::_PreallocateObjectListsInClump( NwClump & clump )
{
	clump.CreateObjectList(NwGeometryBuffer::metaClass(), 256);

	clump.CreateObjectList(NwMesh::metaClass(), 256);

	clump.CreateObjectList(RrMeshInstance::metaClass(), 256);
	clump.CreateObjectList(RrTransform::metaClass(), 256);
	
	clump.CreateObjectList(NwMaterial::metaClass(), 32);

	clump.CreateObjectList(NwRenderState::metaClass(), 8);

	clump.CreateObjectList(NwGeometryShader::metaClass(), 2);
	clump.CreateObjectList(RrMaterialInstance::metaClass(), 2);

	clump.CreateObjectList(NwPlayerWeapon::metaClass(), WeaponType::Count);
	clump.CreateObjectList(NwWeaponDef::metaClass(), WeaponType::Count);

	clump.CreateObjectList(NwCollisionObject::metaClass(), 256);
	clump.CreateObjectList(NwRigidBody::metaClass(), 128);

	clump.CreateObjectList(EnemyNPC::metaClass(), 64);

	clump.CreateObjectList(idMaterial::metaClass(), 64);

#if GAME_CFG_WITH_SOUND
	clump.CreateObjectList(NwAudioClip::metaClass(), 4);
	clump.CreateObjectList(NwSoundSource::metaClass(), 32);
#endif

	clump.CreateObjectList(NwTexture::metaClass(), 64);
	clump.CreateObjectList(idRenderModel::metaClass(), 64);
	clump.CreateObjectList(NwAnimatedModel::metaClass(), 64);
	clump.CreateObjectList(AI_Brain_Generic_Melee_NPC::metaClass(), 32);
	clump.CreateObjectList(NwModelDef::metaClass(), 16);

	clump.CreateObjectList(NwShaderEffect::metaClass(), 8);
	clump.CreateObjectList(NwShaderProgram::metaClass(), 8);
	clump.CreateObjectList(NwPixelShader::metaClass(), 8);
	clump.CreateObjectList(NwVertexShader::metaClass(), 8);
	clump.CreateObjectList(NwModel::metaClass(), 64);

	//clump.CreateObjectList(RrLocalLight::metaClass(), 16);

	return ALL_OK;
}

ERet FPSGame::_PrecacheResources()
{
	return ALL_OK;
}

ERet FPSGame::LoadLevelNamed(
	const char* level_name
	)
{
	DBGOUT("Loading level '%s'...", level_name);

	Str::CopyS(name_of_level_being_loaded, level_name);

	//
	FilePathStringT	path_to_level_file;
	Str::ComposeFilePath(
		path_to_level_file
		, settings.path_to_levels.c_str()
		, name_of_level_being_loaded.c_str()
		, "son"
		);

	mxDO(SON::LoadFromFile(
		path_to_level_file.c_str()
		, _loaded_level
		, MemoryHeaps::temporary())
		);

	// 0) Show loading screen.
	// 1) Copy voxel data from Levels to userdata folder.
	// 2) Load geometry.
	// 3) Remove old monsters and items.
	// 4) Spawn monsters and items.
	// 5) Spawn player.
	// -) Hide loading screen.

	//
	world.NPCs.DeleteAllNPCs();

	//
	FilePathStringT	path_to_voxel_file;
	Str::ComposeFilePath(
		path_to_voxel_file
		, settings.path_to_levels.c_str()
		, level_name
		, "vxl"
		);

	mxDO(world.voxels.voxel_world->LoadFromFile(path_to_voxel_file.c_str()));

	// WAIT UNTIL EVERYTHING IS LOADED...

	is_paused = true;
	player.SetGravityEnabled(false);	// don't fall thru floor while it's being loaded

	current_loading_progress = 0;
	_player_reached_mission_exit = false;
	_PushLoadingScreen();

	return ALL_OK;
}

ERet FPSGame::CompleteLevelLoading()
{
	// 2) Spawn monsters and items.
	_curr_attack_wave = 0;
	_player_reached_mission_exit = false;

	// 3)
	DEVOUT("Respawning player at pos: (%f, %f, %f)",
		_loaded_level.player_spawn_pos.x, _loaded_level.player_spawn_pos.y, _loaded_level.player_spawn_pos.z
		);
	player.RespawnAtPos(_loaded_level.player_spawn_pos);
	player.time_when_map_was_loaded = mxGetTimeInMilliseconds();

	// create NPC after placing the player so that they turn towards the player
	_SpawnItemsAndMonstersForCurrentWave();

	//
	DBGOUT("Finished loading level '%s'...", name_of_level_being_loaded.c_str());
	name_of_level_being_loaded.empty();

	//
	_PopLoadingScreen();


#if GAME_CFG_WITH_SOUND
	//
	// TODO: crossfade
	sound.PlayAudioClip(
		_loaded_level.background_music
		, PlayAudioClip_Looped
		, 1
		, SND_CHANNEL_MUSIC
		);
#endif // GAME_CFG_WITH_SOUND


	//
	is_paused = false;
	player.SetGravityEnabled(true);

	return ALL_OK;
}

void FPSGame::_PushLoadingScreen()
{
	if(state_mgr.getTopState() != &states->loading_progress)
	{
		state_mgr.pushState(
			&states->loading_progress
			);
	}
}

void FPSGame::_PopLoadingScreen()
{
	if(state_mgr.getTopState() == &states->loading_progress)
	{
		state_mgr.popState();
	}
}

void FPSGame::ApplyUserDefinedInGameSettings()
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

ERet FPSGame::SaveUserDefinedInGameSettingsToFile() const
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

	game_user_settings_.window.FixBadSettings();

	return ALL_OK;
}

ERet gameEntryPoint()
{
	//
	NwEngineSettings engine_settings;
	/*mxDO*/(engine_settings.loadFromFile());

	//
	MyGameUserSettings	game_user_settings;
	// no error - use default settings
	/*mxDO*/(LoadUserGameSettingsFile(game_user_settings));

	//
	game_user_settings.window.name = "Half-Assed";


	// override engine settings with game settings
	{
		engine_settings.window = game_user_settings.window;
	}

	//
	mxDO(g_engine.initialize(
		engine_settings
		));




	////
	//NwModelDef	def;
	//def.mesh._setId(MakeAssetID("test_mesh"));
	//def.base_map._setId(MakeAssetID("test_tex"));
	//mxDO(SON::SaveToFile(
	//	def
	//	, "R:/test_model.son"
	//	));

	////
	//U64 q = Encode_XYZ01_to_R21G21B22_64(0.2, 0.4, 0.9);
	//V3f f = Decode_XYZ01_from_R21G21B22_64(q);


	//
	{
		FPSGame game( MemoryHeaps::global() );

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

		//
#if MX_DEVELOPER
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
