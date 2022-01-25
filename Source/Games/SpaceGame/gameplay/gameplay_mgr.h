//
#pragma once

#include "common.h"
#include "experimental/game_experimental.h"
#include "gameplay/spaceship_controller.h"
#include "gameplay/game_entities.h"
#include "physics/physics_mgr.h"
#include "human_player.h"
#include "rendering/sdf_volume.h"


struct SgGameplayConfig
{
	int	max_spaceships;
	//int	max_projectiles;

public:
	SgGameplayConfig()
		: max_spaceships(1024)
		//, max_projectiles(1024)
	{
	}
};



struct SgGameplayRenderData
{
	TSpan< const SgShip >		spaceships;

	TSpan< const SgShipDef >	ship_defs_by_ship_type;

	TSpan< const SgProjectile >		npc_projectiles;
	TSpan< const SgProjectile >		player_projectiles;

#if SG_USE_SDF_VOLUMES
	HTexture	ship_sdf_volume_textures_by_ship_type[obj_type_count];
#endif
};



struct SgGameplayDisplayedStats
{
	int	num_friendly_ships;
	int	mothership_health;

	int	num_enemy_ships;
};




class SgGameplayMgr: TSingleInstance<SgGameplayMgr>
{
	TPtr< struct SgGameplayMgrData >	_data;

public:
	//
	SgHumanPlayer	player;

	//
	SgSpaceshipController	spaceship_controller;

	//
	SgShipHandle	h_spaceship_under_crosshair;

public:
	SgGameplayMgr();


	ERet Initialize(
		const SgGameplayConfig& gameplay_config
		, const SgAppSettings& game_settings
		, SgPhysicsMgr& physics_mgr
		, NwClump& storage_clump
		, AllocatorI& allocator
		);

	void Shutdown();



	ERet StartNewGame(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);



	ERet SetupTestScene(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);

	ERet SetupTestLevel(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);

	ERet SetupLevel0(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);


	//
	void UpdateOncePerFrame(
		const NwTime& game_time
		, const SgUserSettings& user_settings
		, const Vector4& eye_position
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		);

	void RemoveDeadShips(
		const NwTime& game_time
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		);

	ERet HandleSpaceshipUnderCrosshair();


	bool IsGameMissionCompleted() const;
	bool IsGameOverMissionFailed() const;

	const SgGameplayDisplayedStats GetDisplayedStats() const;

#if GAME_CFG_WITH_SOUND
	ERet PlayAmbientMusic(
		NwSoundSystemI& snd_sys
		, const SgUserSettings& user_settings
		, const float01_t rel_volume = 1.0f
		);
	ERet PlayFightMusic(
		NwSoundSystemI& snd_sys
		, const SgUserSettings& user_settings
		, const float01_t rel_volume = 1.0f
		, const bool reset_to_beginning = false
		);
#endif

private:
	void _ClearGameState(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);


	ERet _SetupPlayerBase(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);
	ERet _SetupEnemyBase(
		SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);


	void _FindShipToControl();

	SgShipHandle GetShipToDefend() const;
	void SetShipToDefend(const SgShipHandle h_ship_to_defend) const;


	void _ChooseWhichMusicToPlayInBackground(
		const NwTime& game_time
		, NwSoundSystemI& snd_sys
		, const SgUserSettings& user_settings
		);

	void _StartNextEnemyWaveIfNeeded(
		const NwTime& game_time
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		, const SgUserSettings& user_settings
		);

	ERet _StartEnemyWave(
		const int enemy_wave
		, const NwTime& game_time
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		, const SgUserSettings& user_settings
		);

	ERet _TickAI(
		const NwTime& game_time
		, SgWorld& world
		, const Vector4& eye_position
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		);

	ERet _AdvanceShips(
		const NwTime& game_time
		, const SgWorld& world
		, const Vector4& eye_position
		, SgPhysicsMgr& physics_mgr
		);

	ERet _AdvanceProjectilesFromNPCs(
		const NwTime& game_time
		, const Vector4& eye_position
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		);

	ERet _AdvanceAndCollideProjectilesFromPlayer(
		const NwTime& game_time
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		);

public:
	const SgGameplayRenderData GetRenderData() const;

	const NwHandleTable& GetShipHandleMgr() const;

public:

	const SgShipDef& GetShipDef(
		const EGameObjType obj_type
		) const;

	SgShip* FindSpaceship(
		const SgShipHandle handle
		) const;
	SgShip& GetSpaceship(
		const SgShipHandle handle
		) const;

	ERet CreateSpaceship(
		SgShipHandle &new_spaceship_handle_
		, const EGameObjType spaceship_type
		, SgWorld& world
		, SgPhysicsMgr& physics_mgr
		, const V3f& pos
		, const int flags = 0	// SpaceshipFlags
		);

	//TODO: bulk creation

	ERet AttackEnemySpaceship(
		const SgShipHandle h_ship_to_attack
		);
	ERet TakeControlOfSpaceship(
		const SgShipHandle spaceship_to_control
		);

	ERet ShootCannonOfPlayerControlledShip(
		const SgShipHandle ship_handle
		, const V3f& ship_origin
		, const V3f& ship_forward_vec
		, const V3f& spaceship_right_vec
		, const V3f& spaceship_up_vec
		, const NwTime& game_time
		, const V3f& ship_velocity
		, SgPhysicsMgr& physics_mgr
		, NwSoundSystemI& snd_sys
		);

public:
	ERet DbgSpawnFriendlyShip(
		const V3f& pos
		, const EGameObjType spaceship_type = obj_small_fighter_ship
		);
	ERet DbgSpawnEnemyShip(
		const V3f& pos
		);
};
