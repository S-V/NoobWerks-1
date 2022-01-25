
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

#include "game_settings.h"
#include "common.h"
#include "input_mgr.h"
#include "world/world.h"
#include "rendering/game_renderer.h"
#include "gameplay/gameplay_mgr.h"

// don't include due to a lot of dependencies
//#include "app_states/app_states.h"

#include "human_player.h"




mxOPTIMIZE("singleton without indirection - cast array of bytes to Game ref");
//
class SgGameApp
	: public Demos::DemoApp
	, public NwWindowedGameI
	, public TGlobal< SgGameApp >
{
public:
	AllocatorI &		_allocator;


	/// internal settings
	SgAppSettings		settings;

	/// public settings
	SgUserSettings	user_settings;


#if GAME_CFG_WITH_SOUND
	NwSoundSystem_FMOD	sound;
#else
	NwSoundSystemI &	sound;
#endif // GAME_CFG_WITH_SOUND

	//
	SgInputMgr	input_mgr;

	//
	SgGameplayMgr	gameplay;

	//
	SgWorld			world;

	//
	SgPhysicsMgr	physics_mgr;

	//
	SgRenderer		renderer;

	//
	NwGameStateManager	state_mgr;
	TPtr< SgAppStates >	states;

	NwGameGUI			gui;

	//
	bool				is_paused;

	bool	player_wants_to_start_again;

	//
	NwClump			runtime_clump;

	//
	String32		name_of_level_being_loaded;
	float01_t		current_loading_progress;

	//
	TMovingAverageN< 128 >	_update_FPS_counter;
	TMovingAverageN< 128 >	_render_FPS_counter;

public:
	SgGameApp( AllocatorI & allocator );
	~SgGameApp();

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

private:
	void _PushLoadingScreen();
	void _PopLoadingScreen();

public:	// NwAppI

	virtual void RunFrame(
		const NwTime& game_time
	) override;

private:
	virtual void _UpdateFrame(
		const NwTime& game_time
	);

	virtual void _RenderFrame(
		const NwTime& game_time
	);

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
