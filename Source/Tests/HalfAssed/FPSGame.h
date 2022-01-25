
#pragma once

#pragma comment( lib, "bx.lib" )
#pragma comment( lib, "psapi.lib" )	// GetProcessMemoryInfo()


#include "game_compile_config.h"


#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#include <Sound/FMOD/SoundSystem_FMOD.h>
#endif // GAME_CFG_WITH_SOUND

//
#include <Utility/DemoFramework/DemoFramework.h>

#include <Engine/Engine.h>

#include "game_settings.h"
#include "game_forward_declarations.h"
#include "game_input_service.h"
#include "game_world/game_world.h"
#include "game_world/game_level.h"
#include "game_rendering/game_renderer.h"

// don't include due to a lot of dependencies
//#include "game_states/game_states.h"

#include "game_player.h"
#include "game_editors/game_editors.h"
//#include "game_world/game_level.h"




mxOPTIMIZE("singleton without indirection - cast array of bytes to Game ref");
//
class FPSGame
	: public Demos::DemoApp
	, public NwWindowedGameI
	, public TGlobal< FPSGame >
{
public:
	AllocatorI &		_allocator;


	/// internal settings
	MyGameSettings		settings;

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
	MyGameWorld			world;

	MyGameRenderer		renderer;

	//
	NwGameStateManager		state_mgr;
	TPtr< MyGameStates >	states;

	bool				is_paused;

#if MX_DEVELOPER
	GameEditors			_game_editors;
#endif // MX_DEVELOPER

	// Scene management
	NwClump			runtime_clump;

	//
	String32		name_of_level_being_loaded;
	float01_t		current_loading_progress;

	MyLevelData		_loaded_level;
	UINT			_curr_attack_wave;	// EAttackWave

	//
	TPtr< NwModel >	_mission_exit_mdl;
	bool			_player_reached_mission_exit;

	//
	//V3f		dbg_target_pos;

	//
	TMovingAverageN< 128 >	_update_FPS_counter;
	TMovingAverageN< 128 >	_render_FPS_counter;

public:
	FPSGame( AllocatorI & allocator );
	~FPSGame();

	ERet initialize( const NwEngineSettings& settings );
	void shutdown();

public:	// Level/Map Loading.

	/// Loads the given map and spawns the entities.
	ERet LoadLevelNamed(
		const char* level_name
		);
	/// Executed after the level geometry has been loaded;
	/// spawns all the entities.
	ERet CompleteLevelLoading();

	void EvaluateLevelLogicOnNpcKilled();

	void OnPlayerReachedMissionExit();

private:
	void _SpawnItemsAndMonstersForCurrentWave();

	void _PushLoadingScreen();
	void _PopLoadingScreen();

public:	// NwGameI

	virtual void update(
		const NwTime& game_time
	) override;

	virtual void render(
		const NwTime& game_time
	) override;

public:	// INI/Config/Settings

	void setDefaultSettingsAfterEngineInit();

	void loadSettingsFromFiles();

	/// should be called after changing the settings
	void saveSettingsToFiles();

	void overrideSomeGameSettingsWithDebugValues();

public:
	ERet SaveUserDefinedInGameSettingsToFile() const;
	void ApplyUserDefinedInGameSettings();

private:	// Precaching.

	ERet _PreallocateObjectListsInClump( NwClump & clump );
	ERet _PrecacheResources();

public: // Input handlers

	void _handleInput(
		const NwTime& game_time
	);

	void dev_toggle_GUI( GameActionID action, EInputState status, float value )
	{
		settings.showGUI ^= 1;
	}
	void dev_toggle_wireframe( GameActionID action, EInputState status, float value )
	{
		settings.renderer.drawModelWireframes ^= 1;
	}
};
