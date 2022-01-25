#pragma once

#include <Base/Template/MovingAverage.h>
#include <Core/Tasking/TinyCoroutines.h>
//
#include <Utility/Camera/NwFlyingCamera.h>

//
#include "game_compile_config.h"

//
#if GAME_CFG_WITH_PHYSICS
//#include <Physics/Physics_CharacterController.h>
#include <Physics/Bullet/DynamicCharacterController.h>
#endif // GAME_CFG_WITH_PHYSICS

//
#include "game_forward_declarations.h"

// CoTaskI
#include "experimental/game_experimental.h"
#include "npc/game_nps.h"	// HealthComponent

#include "weapons/game_player_weapons.h"

class MyGamePlayer;



/*
-------------------------------------------------------------------------------
	NgPlayerInventory
-------------------------------------------------------------------------------
*/
struct NgPlayerInventory: NonCopyable
{
	// Gameplay-specific:

	bool	has_BFG_briefcase;


	// Weapons:

	/// 
	U32		weapons_mask;

	TPtr< NwPlayerWeapon >	weapon_models[WeaponType::Count];

public:
	NgPlayerInventory();

	ERet LoadModels(
		NwClump& clump
		);

	void UnloadModels(
		NwClump& clump
		);

private:
	ERet _LoadWeaponModel(
		const WeaponType::Enum weapon_type
		, const TResPtr<idEntityDef>& entity_def
		, const AssetID& weapon_def_id	// NwWeaponDef
		, const AssetID& weapon_mesh_id	// NwSkinnedMesh
		, NwClump& clump
		);
};





struct PlayerMovementFlags
{
	enum Enum {
		FORWARD		= BIT(0),
		BACKWARD	= BIT(1),
		LEFT		= BIT(2),
		RIGHT		= BIT(3),
		JUMP		= BIT(4),
		//DOWN		= BIT(5),
		//ACCELERATE	= BIT(6),	//!< useful for debugging planetary LoD
		//RESET		= BIT(7),	//!< reset position and orientation
		////STOP_NOW	= BIT(7),	//!< 		
	};
};
mxDECLARE_ENUM( PlayerMovementFlags::Enum, U32, PlayerMovementFlagsT );





//
mxSTOLEN("Based on Doom3's code, loggedAccel_t struct in Player.h")
struct LoggedAccelerations
{
	enum { NUM = 16 };

	struct LoggedAccel
	{
		MillisecondsU32	time_when_added;	// milliseconds
		V3f				dir;	// scaled larger for running
	};
	LoggedAccel	items[NUM];
	U32			num;

public:
	LoggedAccelerations()
	{
		num = 0;
	}

	void Add(
		const V3f& weapon_accel_dir,
		const MillisecondsU32 curr_time
		)
	{
		LoggedAccel &new_item = items[ num++ & (mxCOUNT_OF(items) - 1) ];
		new_item.time_when_added = curr_time;
		new_item.dir = weapon_accel_dir;
	}
};




/// for momentum-based weapon swaying, weapon/hand bobbing and turning inertia, landing bounce:
/// - Horizontal and vertical sway for any weapon
/// - Sway based on player velocity
struct WeaponSway
{
	//int		bobCycle;		// for view bobbing and footstep generation
	//int		foot;

	float	time_accumulator;

	V3f		prev_pos;
	V3f		prev_look_dir;

	//
	LoggedAccelerations	accel;

public:
	WeaponSway();

	/// returns interpolated view angles
	M44f UpdateOncePerFrame(
		const NwTime& game_time
		, const NwFlyingCamera& camera
		, const MyGamePlayer& player
		, WeaponType::Enum weapon_type
		);

	// for momentum-based weapon sway when jumping/landing
	V3f GunAcceleratingOffset(const MillisecondsU32 curr_time);

private:
	//
};



/*
-------------------------------------------------------------------------------
	MyGamePlayer
-------------------------------------------------------------------------------
*/
class MyGamePlayer: CStruct, NonCopyable
{
public:
	NwFlyingCamera	camera;


#if GAME_CFG_WITH_PHYSICS

	DynamicCharacterController	phys_char_controller;

	// used only when noclip cheat is disabled
	PlayerMovementFlagsT		phys_movement_flags;

	//
	bool		was_in_air;
	SecondsF	time_in_air;

	// for weapon swaying and footstep generation
	SecondsF	foot_cycle;
	bool		left_foot;

	void _ResetFootCycle();

#endif // GAME_CFG_WITH_PHYSICS

	//
	WeaponSway		weapon_sway;

	//
	HealthComponent		comp_health;

	//
	TPtr< NwPlayerWeapon >	_current_weapon;

	//
	NgPlayerInventory	inventory;

	//
	float	num_seconds_shift_is_held;

	CoTaskQueue	task_scheduler;

	// for player damage indicator
	MillisecondsU32	time_when_received_damage;

	// for fading in after loading the level
	MillisecondsU32	time_when_map_was_loaded;

public:
	mxDECLARE_CLASS(MyGamePlayer, CStruct);
	mxDECLARE_REFLECTION;
	MyGamePlayer();

	ERet Init(
		NwClump& storage
		, MyGameWorld& world
		, const MyGameSettings& game_settings
		);
	void unload(NwClump& storage_clump);

	void UpdateOncePerFrame(
		const NwTime& args
		, const GameCheatsSettings& game_cheat_settings
		);

	/// corrected for eye height
	V3f GetEyePosition() const;

	void SetCameraPosition(
		const V3d new_position_in_world_space
		);

	const V3f GetPhysicsBodyCenter() const;

public:
	void SetGravityEnabled(bool affected_by_gravity);

	void RespawnAtPos(
		const V3f& spawn_pos
		);

public:
	void RenderFirstPersonView(
		const NwCameraView& camera_view
		, const RrRuntimeSettings& renderer_settings
		, GL::NwRenderContext & render_context
		, const NwTime& args
		, ARenderWorld* render_world
		) const;

	void Dbg_RenderFirstPersonView(
		const NwCameraView& camera_view
		, MyGameRenderer& renderer
		) const;

private:
	ERet _RenderFullScreenDamageIndicatorIfNeeded(
		GL::NwRenderContext & render_context
		, const NwTime& args
		) const;
	
	ERet _FadeInUponLevelLoadingIfNeeded(
		GL::NwRenderContext & render_context
		, const NwTime& args
		) const;

public:
	void PlaySound3D(
		const AssetID& sound_id
		);

public:
	ERet SelectWeapon( WeaponType::Enum type_of_weapon_to_select );

	/// Cycles to the previous weapon in the player inventory.
	void SelectPreviousWeapon();

	/// Cycles to the next weapon in the player inventory.
	void SelectNextWeapon();

	void ReloadWeapon();

public:
	void DealDamage(
		int damage
		);

	bool HasFullHealth() const;
	void ReplenishHealth();
	void ReplenishAmmo();

public:	// Item Pickups

	bool PickUp_Item_RocketAmmo();
	bool PickUp_Item_BFG_Case();

	bool PickUp_Weapon_Plasmagun();

private:
	bool _PickUp_Ammo_for_Weapon(
		WeaponType::Enum weapon_type
		, int num_ammo_in_item
		);

public:
	float calc_debug_camera_speed(
		) const;
};
