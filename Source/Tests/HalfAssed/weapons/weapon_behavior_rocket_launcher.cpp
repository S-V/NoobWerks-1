#include "stdafx.h"
#pragma hdrstop

//
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/RuntimeInclude.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/IObject.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/ObjectInterfacePerModule.h>
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes

RUNTIME_COMPILER_LINKLIBRARY("user32.lib");
RUNTIME_COMPILER_LINKLIBRARY("advapi32.lib");
RUNTIME_COMPILER_LINKLIBRARY("kernel32.lib");
RUNTIME_COMPILER_LINKLIBRARY("gdi32.lib");
RUNTIME_COMPILER_LINKLIBRARY("winmm.lib");
RUNTIME_COMPILER_LINKLIBRARY("Dbghelp.lib");
RUNTIME_COMPILER_LINKLIBRARY("Imm32.lib");
RUNTIME_COMPILER_LINKLIBRARY("comctl32.lib");
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Base.lib");
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Core.lib");
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Graphics.lib");
//LINK : fatal error LNK1104: cannot open file 'DxErr.lib'
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Renderer.lib");



//
#include <Base/Base.h>
#include <Base/Math/Random.h>
//
#include <Core/Util/Tweakable.h>
#include <Core/Tasking/TinyCoroutines.h>
//
#include <Renderer/Renderer.h>
#include <Renderer/Modules/Animation/SkinnedMesh.h>
#include <Renderer/Modules/idTech4/idMaterial.h>
//
#include <Utility/GameFramework/TbPlayerWeapon.h>

//
//#include "RCCpp_WeaponBehavior.h"
#include "experimental/rccpp.h"
#include "experimental/game_experimental.h"
#include "weapons/game_player_weapons.h"
#include "FPSGame.h"



mxSTOLEN("Doom3 BFG source code, file: weapon_rocketlauncher.script");

enum {
	ROCKETLAUNCHER_AMMO_CAPACITY = 5
};


#define ROCKETLAUNCHER_LOWAMMO			1
#define ROCKETLAUNCHER_NUMPROJECTILES	1
#define ROCKETLAUNCHER_FIREDELAY_SECONDS	0.75

// blend times
#define ROCKETLAUNCHER_IDLE_TO_LOWER	4
#define ROCKETLAUNCHER_IDLE_TO_FIRE		0
#define	ROCKETLAUNCHER_IDLE_TO_RELOAD	4
#define ROCKETLAUNCHER_RAISE_TO_IDLE	4
#define ROCKETLAUNCHER_FIRE_TO_IDLE		0
#define ROCKETLAUNCHER_RELOAD_TO_IDLE	4
#define ROCKETLAUNCHER_RELOAD_FRAME		34		// how many frames from the end of "reload" to fill the clip




/*
----------------------------------------------------------
	Task_Fire_RocketLauncher
----------------------------------------------------------
*/
struct Task_Fire_RocketLauncher: CoTaskI
{
	CoTimer	next_attack_timer;

public:
	mxDECLARE_CLASS(Task_Fire_RocketLauncher, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Fire_RocketLauncher()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		// so that the rockets come out of the muzzle
		nwNON_TWEAKABLE_CONST(float, ROCKET_PROJECTLE_HORIZ_OFFSET, 0.1f);
		nwNON_TWEAKABLE_CONST(float, ROCKET_PROJECTLE_VERT_OFFSET, 0.2f);

		//
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;
		const NwWeaponDef& weapon_def = *weapon.def;

		const NameHash32 fire_anim_id = mxHASH_STR("fire");

		//
		FPSGame& game = FPSGame::Get();

		$CoBEGIN;

		mxASSERT(weapon.ammo_in_clip > 0);

		// cannot fire faster than the rate of fire
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );





		V3f		muzzle_position_in_world, muzzle_direction_in_world;
		weapon.getMuzzlePosAndDirInWorldSpace(muzzle_position_in_world, muzzle_direction_in_world);


		muzzle_position_in_world += game.renderer._camera_matrices.right_dir * ROCKET_PROJECTLE_HORIZ_OFFSET;
		muzzle_position_in_world -= game.renderer._camera_matrices.getUpDir() * ROCKET_PROJECTLE_VERT_OFFSET;

		//game.world.voxels.voxel_engine_dbg_drawer.VX_AddPoint_AnyThread(
		//	V3d::fromXYZ(muzzle_position_in_world)
		//	, RGBAi::WHITE
		//	, 1e-2f
		//	);
		//game.world.voxels.voxel_engine_dbg_drawer.VX_DrawLine_MainThread(
		//	V3d::fromXYZ(muzzle_position_in_world)
		//	, RGBAi::WHITE
		//	, V3d::fromXYZ(muzzle_position_in_world + muzzle_direction_in_world)
		//	, RGBAi::RED
		//	);


		// muzzle flash
		weapon.AddMuzzleFlashToWorld(
			*game.world.render_world,
			muzzle_position_in_world,
			now
			);

		// muzzle smoke
		game.renderer.particle_renderer.CreateMuzzleSmokePuff_RocketLauncher(
			muzzle_position_in_world
			);

		// launch projectile
		game.world.projectiles.CreateRocket(
			muzzle_position_in_world
			, muzzle_direction_in_world
			, weapon_def.projectile.initial_velocity
			);





		//
		--weapon.ammo_in_clip;

		//
		next_attack_timer.StartWithDuration( SEC2MS(ROCKETLAUNCHER_FIREDELAY_SECONDS), now );

		//
		weapon.anim_model->PlayAnim(
			fire_anim_id
			, NwPlayAnimParams()
			.SetPriority(NwAnimPriority::Required)
			);
		// wait until the "fire" anim is finished
		$CoWAIT_WHILE( weapon.anim_model->IsPlayingAnim(fire_anim_id) );

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Fire_RocketLauncher);
mxBEGIN_REFLECTION(Task_Fire_RocketLauncher)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Reload_RocketLauncher
----------------------------------------------------------
*/
struct Task_Reload_RocketLauncher: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Reload_RocketLauncher, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Reload_RocketLauncher()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		//
		const NwWeaponDef& weapon_def = *weapon.def;

		const U32 ammo_count_to_load_into_clip = weapon_def.clip_size - weapon.ammo_in_clip;
		mxASSERT(ammo_count_to_load_into_clip);

		const U32 available_recharge_ammo = smallest(weapon.recharge_ammo, ammo_count_to_load_into_clip);
		mxASSERT(available_recharge_ammo);

		//
		const NameHash32 reload_anim_name_hash = mxHASH_STR("reload");

		$CoBEGIN;

		weapon.anim_model->PlayAnim(
			reload_anim_name_hash
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Required)
			);

		// wait until the "reload" anim is finished
		$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(reload_anim_name_hash));

		// the weapon is now loaded
		weapon.recharge_ammo -= available_recharge_ammo;
		weapon.ammo_in_clip += available_recharge_ammo;

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Reload_RocketLauncher);
mxBEGIN_REFLECTION(Task_Reload_RocketLauncher)
mxEND_REFLECTION


///
struct NgWeaponBehavior_RocketLauncher
	: NwWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	Task_Fire_RocketLauncher	task_fire;
	Task_Reload_RocketLauncher	task_reload;
	CoTaskI *	curr_task;

public:
    NgWeaponBehavior_RocketLauncher()
    {
		//curr_task = &task_idle;
		curr_task = nil;

		PerModuleInterface::g_pSystemTable->weapon_behaviors[
			WeaponType::RocketLauncher
		] = this;
    }

	//virtual bool IsBusy() override
	//{
	//	if( curr_task && curr_task->zz__state != Completed )
	//	{
	//		return true;
	//	}
	//	return false;
	//}

    virtual void UpdateWeapon(
		NwPlayerWeapon & weapon
		) override
	{
		CoTimestamp	clock;
		clock.time_msec = tbGetTimeInMilliseconds();//

		if( curr_task )
		{
			CoTaskStatus status = curr_task->Run( &weapon, clock );

			if( status != Completed )
			{
				return;
			}
		}

		// Auto-Reload
		if(weapon.ShouldBeAutoReloaded())
		{
			SwitchToTaskIfNeeded(&task_reload);
			return;
		}

		if( weapon.pressed_buttons & WEAPON_BUTTON_PRIMARY_FIRE )
		{
			if(weapon.ammo_in_clip)
			{
				SwitchToTask(&task_fire);
			}
			else if(weapon.CanBeReloaded())
			{
				SwitchToTaskIfNeeded(&task_reload);
			}
			else
			{
				// dry fire
			}
		}
		else if( weapon.pressed_buttons & WEAPON_BUTTON_RELOAD )
		{
			if(weapon.CanBeReloaded())
			{
				SwitchToTaskIfNeeded(&task_reload);
			}
		}
		else
		{
			curr_task = nil;
		}
	};

	void SwitchToTask( CoTaskI * new_task )
	{
		//if( curr_task != new_task )
		{
			curr_task = new_task;
			new_task->Restart();
		}
	}

	void SwitchToTaskIfNeeded( CoTaskI * new_task )
	{
		if( curr_task != new_task )
		{
			curr_task = new_task;
			new_task->Restart();
		}
	}
};

REGISTERSINGLETON(NgWeaponBehavior_RocketLauncher,true);
