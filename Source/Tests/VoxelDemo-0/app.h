
#pragma once

#pragma comment( lib, "bx.lib" )
#pragma comment( lib, "psapi.lib" )	// GetProcessMemoryInfo()


#include "compile_config.h"


#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#include <Sound/FMOD/SoundSystem_FMOD.h>
#endif // GAME_CFG_WITH_SOUND

//
#include <Utility/DemoFramework/DemoFramework.h>
#include <Utility/GameFramework/NwGameGUI.h>

#include <Engine/Engine.h>

#include "app_settings.h"
#include "common.h"
#include "game_input_service.h"
#include "world/game_world.h"
#include "rendering/renderer.h"
#include "voxels/voxel_terrain_mgr.h"

// don't include due to a lot of dependencies
//#include "app_states/game_states.h"

#include "game_player.h"




mxOPTIMIZE("singleton without indirection - cast array of bytes to Game ref");
//
class MyApp
	: public Demos::DemoApp
	, public NwWindowedGameI
	, public TGlobal< MyApp >
{
public:
	AllocatorI &		_allocator;


	/// internal settings
	MyAppSettings		settings;

	/// public settings
	MyGameUserSettings	user_settings;





#if GAME_CFG_WITH_SOUND
	NwSoundSystem_FMOD	sound;
#else
	NwSoundSystemI &	sound;
#endif // GAME_CFG_WITH_SOUND

	//
	GameInputService	input;

	//
	MyGamePlayer		player;

	//
	MyWorld			world;

	MyVoxelTerrainMgr	voxel_mgr;

	MyGameRenderer		renderer;

	//
	NwGameStateManager		state_mgr;
	TPtr< MyGameStates >	states;

	NwGameGUI			gui;

	//
	bool				is_paused;

	// Scene management
	NwClump			runtime_clump;

	//
	String32		name_of_level_being_loaded;
	float01_t		current_loading_progress;

	//
	TPtr< NwModel >	_mission_exit_mdl;
	bool			_player_reached_mission_exit;

	//
	//V3f		dbg_target_pos;

	//
	TMovingAverageN< 128 >	_update_FPS_counter;
	TMovingAverageN< 128 >	_render_FPS_counter;

public:
	MyApp( AllocatorI & allocator );
	~MyApp();

	ERet Initialize( const NEngine::LaunchConfig & settings );
	void Shutdown();

public:	// NwAppI

	virtual void RunFrame(
		const NwTime& game_time
	) override;

private:
	void update(
		const NwTime& game_time
	);

	void render(
		const NwTime& game_time
	);

public:	// INI/Config/Settings

	/// should be called after initializing the engine
	void SetDefaultSettings();

	void LoadSettingsFromFiles();

	/// should be called after changing the settings
	void SaveSettingsToFiles();

public:
	ERet SaveUserDefinedInGameSettingsToFile() const;
	void ApplyUserDefinedInGameSettings();

public: // Input handlers

	void _handleInput(
		const NwTime& game_time
	);

#if 0
	void dev_toggle_GUI( GameActionID action, EInputState status, float value )
	{
		settings.showGUI ^= 1;
	}
	void dev_toggle_wireframe( GameActionID action, EInputState status, float value )
	{
		settings.renderer.drawModelWireframes ^= 1;
	}
#endif
};
