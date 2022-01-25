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


#define CHAINGUN_FIRERATE			0.125 //changed by Tim
//#define CHAINGUN_FIRERATE			0.1

#define CHAINGUN_LOWAMMO			10
#define CHAINGUN_NUMPROJECTILES	1

#define CHAINGUN_MAX_AMMO_IN_CLIP		50




#define CHAINGUN_FIRE_SKIPFRAMES	7		// 6 shots per second
#define CHAINGUN_LOWAMMO			10
#define CHAINGUN_NUMPROJECTILES		1
#define	CHAINGUN_BARREL_SPEED		( 60 * ( GAME_FPS / CHAINGUN_FIRE_SKIPFRAMES ) )
#define	CHAINGUN_BARREL_ACCEL_TIME	0.4
#define CHAINGUN_BARREL_DECCEL_TIME	1.0
#define	CHAINGUN_BARREL_ACCEL		( CHAINGUN_BARREL_SPEED / CHAINGUN_BARREL_ACCEL_TIME )
#define CHAINGUN_BARREL_DECCEL		( CHAINGUN_BARREL_SPEED / CHAINGUN_BARREL_DECCEL_TIME )

// blend times
#define CHAINGUN_IDLE_TO_LOWER		4
#define CHAINGUN_IDLE_TO_FIRE		0
#define	CHAINGUN_IDLE_TO_RELOAD		4
#define CHAINGUN_RAISE_TO_IDLE		0
#define CHAINGUN_WINDDOWN_TO_IDLE	0
#define CHAINGUN_RELOAD_TO_IDLE		0



/*
----------------------------------------------------------
	Task_Idle_Chaingun
----------------------------------------------------------
*/
struct Task_Idle_Chaingun: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Idle_Chaingun, CoTaskI);
	mxDECLARE_REFLECTION;

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		const NameHash32 idle_anim_name_hash = mxHASH_STR("idle");

		$CoBEGIN;
		{
			//NwPlayAnimParams	play_anim_params;
			//play_anim_params.transition_duration = 1;
			//play_anim_params.flags |=
			//	PlayAnimFlags::DoNotRestart |
			//	PlayAnimFlags::Looped;

			//weapon.anim_model->PlayAnim(
			//	idle_anim_name_hash, NwPlayAnimParams()
			//	.SetPriority(NwAnimPriority::Lowest)
			//	, ?
			//	);
		}
		$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(idle_anim_name_hash));

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Idle_Chaingun);
mxBEGIN_REFLECTION(Task_Idle_Chaingun)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Fire_Chaingun
----------------------------------------------------------
*/
struct Task_Fire_Chaingun: CoTaskI
{
	CoTimer	next_attack_timer;

public:
	mxDECLARE_CLASS(Task_Fire_Chaingun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Fire_Chaingun()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

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
			*game.world.render_world,
			muzzle_position_in_world,
			now
			);


		// launch projectile
		//
		//test_PlaySound();


//		--weapon.ammo_in_clip;



		//
		next_attack_timer.StartWithDuration( SEC2MS(CHAINGUN_FIRERATE), now );


		weapon.anim_model->PlayAnim(
			mxHASH_STR("windup")
			, NwPlayAnimParams()
			.SetPriority(NwAnimPriority::Required)
			//, PlayAnimFlags::DoNotRestart
			);
		// NOTE: do NOT wait until the "fire" anim is finished!
		//$CoWAIT_UNTIL( fire_anim_timer.IsExpired(clock) );

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Fire_Chaingun);
mxBEGIN_REFLECTION(Task_Fire_Chaingun)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Reload_Chaingun
----------------------------------------------------------
*/
struct Task_Reload_Chaingun: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Reload_Chaingun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Reload_Chaingun()
	{
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		const NameHash32 reload_anim_name_hash = mxHASH_STR("reload");

		$CoBEGIN;

		weapon.anim_model->PlayAnim(
			reload_anim_name_hash
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Required)
			);

		// wait until the "reload" anim is finished
		$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(reload_anim_name_hash));

		// the weapon is now loaded
		weapon.ammo_in_clip = CHAINGUN_MAX_AMMO_IN_CLIP;

		//weapon.anim_model->PlayAnim(
		//	mxHASH_STR("idle")
		//	, NwPlayAnimParams()
		//	);

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Reload_Chaingun);
mxBEGIN_REFLECTION(Task_Reload_Chaingun)
mxEND_REFLECTION


///
struct NgWeaponBehavior_Chaingun
	: NwWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	//Task_Idle_Chaingun	task_idle;
	Task_Fire_Chaingun	task_fire;
	Task_Reload_Chaingun	task_reload;
	CoTaskI *	curr_task;

public:
    NgWeaponBehavior_Chaingun()
    {
		//curr_task = &task_idle;
		curr_task = nil;

		PerModuleInterface::g_pSystemTable->weapon_behaviors[
			WeaponType::Chaingun
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

			if( status != Completed
				&& !curr_task->Is<Task_Idle_Chaingun>()
				)
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

REGISTERSINGLETON(NgWeaponBehavior_Chaingun,true);
