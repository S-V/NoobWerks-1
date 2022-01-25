#include "stdafx.h"
#pragma hdrstop

#if 0
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
#include "weapons/weapon_behavior_rocket_launcher.h"
#include "FPSGame.h"



mxSTOLEN("Doom3 BFG source code, file: weapon_rocketlauncher.script");

enum {
	SHOTGUN_AMMO_CAPACITY = 5
};


#define SHOTGUN_LOWAMMO			1
#define SHOTGUN_NUMPROJECTILES	1
#define SHOTGUN_FIREDELAY_SECONDS	0.75

// blend times
#define SHOTGUN_IDLE_TO_LOWER	4
#define SHOTGUN_IDLE_TO_FIRE		0
#define	SHOTGUN_IDLE_TO_RELOAD	4
#define SHOTGUN_RAISE_TO_IDLE	4
#define SHOTGUN_FIRE_TO_IDLE		0
#define SHOTGUN_RELOAD_TO_IDLE	4
#define SHOTGUN_RELOAD_FRAME		34		// how many frames from the end of "reload" to fill the clip


/*
----------------------------------------------------------
	Task_Fire_Shotgun
----------------------------------------------------------
*/
struct Task_Fire_Shotgun: CoTaskI
{
	CoTimer	next_attack_timer;

public:
	mxDECLARE_CLASS(Task_Fire_Shotgun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Fire_Shotgun()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		const NameHash32 fire_anim_id = mxHASH_STR("fire1");


		//
		FPSGame& game = FPSGame::Get();



		$CoBEGIN;

//		mxASSERT(weapon.ammo_in_clip > 0);

		// cannot fire faster than the rate of fire
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );






		V3f		muzzle_position_in_world, muzzle_direction_in_world;
		weapon.getMuzzlePosAndDirInWorldSpace(muzzle_position_in_world, muzzle_direction_in_world);

		// muzzle flash
		weapon.AddMuzzleFlashToWorld(
			*game._game_world.render_world,
			muzzle_position_in_world,
			now
			);

		// launch projectile
		//
		//test_PlaySound();

//		--weapon.ammo_in_clip;

		//
		next_attack_timer.StartWithDuration( SEC2MS(SHOTGUN_FIREDELAY_SECONDS), now );

		//
		weapon.anim_model->PlayAnim(
			fire_anim_id
			, NwPlayAnimParams()
			.SetPriority(NwAnimPriority::Required)
			);
		// wait until the "fire" anim is finished
		$CoWAIT_UNTIL( weapon.anim_model->IsPlayingAnim(fire_anim_id) );

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Fire_Shotgun);
mxBEGIN_REFLECTION(Task_Fire_Shotgun)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Reload_Shotgun
----------------------------------------------------------
*/
struct Task_Reload_Shotgun: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Reload_Shotgun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Reload_Shotgun()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		const NameHash32 reload_anim_name_hash = mxHASH_STR("reload_loop");

		$CoBEGIN;

		weapon.anim_model->PlayAnim(
			reload_anim_name_hash
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Required)
			);

		// wait until the "reload" anim is finished
		$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(reload_anim_name_hash));

		// the weapon is now loaded
		weapon.ammo_in_clip = SHOTGUN_AMMO_CAPACITY;

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Reload_Shotgun);
mxBEGIN_REFLECTION(Task_Reload_Shotgun)
mxEND_REFLECTION


///
struct NgWeaponBehavior_Shotgun
	: NwWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	Task_Fire_Shotgun	task_fire;
	Task_Reload_Shotgun	task_reload;
	CoTaskI *	curr_task;

public:
    NgWeaponBehavior_Shotgun()
    {
		//curr_task = &task_idle;
		curr_task = nil;

		PerModuleInterface::g_pSystemTable->weapon_behaviors[
			WeaponType::Shotgun
		] = this;
    }

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

		//
		//SwitchToTask(&task_idle);

		//if(mxGetTimeInMilliseconds() % 1000 == 0)
		//{
		//	weapon.anim_model->PlayAnim(mxHASH_STR("fire2"));
		//}
		//
		if( weapon.pressed_buttons & WEAPON_BUTTON_PRIMARY_FIRE )
		{
			SwitchToTask(&task_fire);
		}
		else if( weapon.pressed_buttons & WEAPON_BUTTON_RELOAD )
		{
			SwitchToTaskIfNeeded(&task_reload);
		}
		else
		{
			curr_task = nil;
			//SwitchToTaskIfNeeded(&task_idle);
		}
	};

	void SwitchToTask( CoTaskI * new_task )
	{
		//if( curr_task != new_task )
		{
			curr_task = new_task;
			new_task->restart();
		}
	}

	void SwitchToTaskIfNeeded( CoTaskI * new_task )
	{
		if( curr_task != new_task )
		{
			curr_task = new_task;
			new_task->restart();
		}
	}
};

REGISTERSINGLETON(NgWeaponBehavior_Shotgun,true);
#endif