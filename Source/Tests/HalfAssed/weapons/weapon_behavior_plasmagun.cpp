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

/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "bx.lib");
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "ozz-animation.lib");


//
#include <Base/Base.h>
#include <Base/Math/Random.h>
//
#include <Core/Tasking/TinyCoroutines.h>
#include <Core/Util/Tweakable.h>
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


//#define PLASMAGUN_FIRERATE_MSEC			125 //changed by Tim
#define PLASMAGUN_FIRERATE_MSEC			110

#define PLASMAGUN_LOWAMMO			10
#define PLASMAGUN_NUMPROJECTILES	1

#define PLASMAGUN_MAX_AMMO_IN_CLIP		50

// blend times
#define PLASMAGUN_IDLE_TO_LOWER		4
#define PLASMAGUN_IDLE_TO_FIRE		1
#define	PLASMAGUN_IDLE_TO_RELOAD	4
#define PLASMAGUN_RAISE_TO_IDLE		4
#define PLASMAGUN_FIRE_TO_IDLE		4
#define PLASMAGUN_RELOAD_TO_IDLE	4

/*
----------------------------------------------------------
	Task_Idle_Plasmagun
----------------------------------------------------------
*/
struct Task_Idle_Plasmagun: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Idle_Plasmagun, CoTaskI);
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
mxDEFINE_CLASS(Task_Idle_Plasmagun);
mxBEGIN_REFLECTION(Task_Idle_Plasmagun)
mxEND_REFLECTION


static const NameHash32 FIRE_ANIM_IDS[] = {
	mxHASH_STR("fire1"),
	mxHASH_STR("fire2"),
};


/// from Doom3's source code: for converting from 24 frames per second to milliseconds
static mxFORCEINLINE
MillisecondsT FRAME2MS( int framenum )
{
	return ( framenum * 1000 ) / 24;
}

static mxFORCEINLINE
SecondsF FRAME2S( int framenum )
{
	return float(framenum) * (1.0f / 24.0f);
}

/// The animDone function takes a blend count as the second argument.
/// E.g. if the blend count is 4, it will return true when there are only 4 frames left in the animation to play.
// https://web.archive.org/web/20200613234937/https://www.iddevnet.com/doom3/script_weapon.html
///
bool AnimDone(
			  const NwAnimatedModel& anim_model,
			  const NameHash32 anim_name_hash, const int num_blend_frames
			  )
{
	return anim_model.IsAnimDone(anim_name_hash, FRAME2S(num_blend_frames));
}

/*
----------------------------------------------------------
	Task_Fire_Plasmagun
----------------------------------------------------------
*/
struct Task_Fire_Plasmagun: CoTaskI
{
	CoTimer	next_attack_timer;
	UINT	curr_fire_anim_idx;

	GlobalTimeMillisecondsT	kick_endtime;

public:
	mxDECLARE_CLASS(Task_Fire_Plasmagun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Fire_Plasmagun()
	{
		curr_fire_anim_idx = 0;
		kick_endtime = 0;
	}

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
	{
		nwTWEAKABLE_CONST(int, PLASMAGUN_MUZZLE_KICK_MAX_TIME_MSEC, 500);
		nwTWEAKABLE_CONST(int, PLASMAGUN_MUZZLE_KICK_TIME, 200);
		nwTWEAKABLE_CONST(float, PLASMAGUN_MAX_HORIZ_LEFT_SPREAD, 0.02f);
		nwTWEAKABLE_CONST(float, PLASMAGUN_MAX_HORIZ_RIGHT_SPREAD, 0.1f);
		nwTWEAKABLE_CONST(float, PLASMAGUN_MAX_VERT_SPREAD, 0.02f);

		//
		idRandom	firerate_rnd(now.time_msec);

		//
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;
		const NwWeaponDef& weapon_def = *weapon.def;

		const NameHash32 fire_anim_id = FIRE_ANIM_IDS[ curr_fire_anim_idx ];

		//
		FPSGame& game = FPSGame::Get();

		$CoBEGIN;

		mxASSERT(weapon.ammo_in_clip > 0);

		// cannot fire faster than the rate of fire
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );


		//
		V3f		muzzle_position_in_world, muzzle_direction_in_world;
		weapon.getMuzzlePosAndDirInWorldSpace(muzzle_position_in_world, muzzle_direction_in_world);

		// muzzle flash
		weapon.AddMuzzleFlashToWorld(
			*game.world.render_world,
			muzzle_position_in_world,
			now
			);


		// kick up based on repeat firing
		{
			I64 kick_time_remaining = kick_endtime - now.time_msec;
			if ( kick_time_remaining > 0 )
			{
				kick_time_remaining = smallest(kick_time_remaining, PLASMAGUN_MUZZLE_KICK_MAX_TIME_MSEC);

				const float01_t kick_amount01 = float(kick_time_remaining) / float(PLASMAGUN_MUZZLE_KICK_MAX_TIME_MSEC);

				//
				nwTWEAKABLE_CONST(float, PLASMAGUN_VERTICAL_MUZZLE_KICK, 0.1f);
				const float vertical_kick = kick_amount01 * PLASMAGUN_VERTICAL_MUZZLE_KICK;

				muzzle_direction_in_world += game.player.camera.m_upDirection * vertical_kick;


				// add horizontal spread

				idRandom	spread_rnd(now.time_msec);
				muzzle_direction_in_world
					+= game.player.camera.m_rightDirection
					* spread_rnd.RandomFloat(-PLASMAGUN_MAX_HORIZ_LEFT_SPREAD, +PLASMAGUN_MAX_HORIZ_RIGHT_SPREAD) * kick_amount01
					;

				// add some vertical spread

				spread_rnd.SetSeed(now.time_msec ^ kick_time_remaining);
				muzzle_direction_in_world
					+= game.player.camera.m_upDirection
					* spread_rnd.RandomFloat(-PLASMAGUN_MAX_VERT_SPREAD, +PLASMAGUN_MAX_VERT_SPREAD) * kick_amount01
					;
			}

			//
			muzzle_direction_in_world = muzzle_direction_in_world.normalized();
		}



		// launch projectile
		game.world.projectiles.CreatePlasmaBolt(
			muzzle_position_in_world
			, muzzle_direction_in_world
			, weapon_def.projectile.initial_velocity
			);



		//
		{
			// add some to the kick time, incrementally moving repeat firing weapons back
			if ( kick_endtime < now.time_msec ) {
				kick_endtime = now.time_msec;
			}

			kick_endtime += PLASMAGUN_MUZZLE_KICK_TIME;

			if ( kick_endtime > now.time_msec + PLASMAGUN_MUZZLE_KICK_MAX_TIME_MSEC ) {
				kick_endtime = now.time_msec + PLASMAGUN_MUZZLE_KICK_MAX_TIME_MSEC;
			}
		}




		--weapon.ammo_in_clip;

		//
		next_attack_timer.StartWithDuration(
			PLASMAGUN_FIRERATE_MSEC + firerate_rnd.RandomInt(40),
			now
			);

		//if(weapon.anim_model->IsPlayingAnim(fire_anim_id))
		//{
		//	weapon.anim_model->PlayAnim(
		//		fire_anim_id
		//		, NwPlayAnimParams().SetPriority(NwAnimPriority::Required).SetMaxTimeRatio(0.9)
		//		, PlayAnimFlags::Defaults
		//		, NwStartAnimParams().SetStartTimeRatio(0.5)
		//		);
		//}
		//else
		//{
			weapon.anim_model->PlayAnim(
				fire_anim_id
				, NwPlayAnimParams()
			//	, PlayAnimFlags::DoNotRestart
				);
		//}

		//
		//$CoWAIT_UNTIL(AnimDone(*weapon.anim_model, fire_anim_id, PLASMAGUN_FIRE_TO_IDLE));


		// NOTE: do NOT wait until the "fire" anim is finished! the fire rate is too slow!
		//$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(fire_anim_id));

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$CoWAIT_UNTIL( next_attack_timer.IsExpired(now) );

		////
		//if( !weapon.anim_model->IsPlayingAnim(fire_anim_id) ) {
		//	curr_fire_anim_idx = ++curr_fire_anim_idx % mxCOUNT_OF(FIRE_ANIM_IDS);
		//}

		//timestamp_of_last_shot_msec = now.time_msec;

		$CoEND;
	}
};
mxDEFINE_CLASS(Task_Fire_Plasmagun);
mxBEGIN_REFLECTION(Task_Fire_Plasmagun)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Reload_Plasmagun
----------------------------------------------------------
*/
struct Task_Reload_Plasmagun: CoTaskI
{
public:
	mxDECLARE_CLASS(Task_Reload_Plasmagun, CoTaskI);
	mxDECLARE_REFLECTION;
	Task_Reload_Plasmagun()
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
mxDEFINE_CLASS(Task_Reload_Plasmagun);
mxBEGIN_REFLECTION(Task_Reload_Plasmagun)
mxEND_REFLECTION


///
struct TbWeaponBehavior_Plasmagun
	: NwWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	//Task_Idle_Plasmagun	task_idle;
	Task_Fire_Plasmagun	task_fire;
	Task_Reload_Plasmagun	task_reload;
	CoTaskI *	curr_task;

public:
    TbWeaponBehavior_Plasmagun()
    {
		//curr_task = &task_idle;
		curr_task = nil;

		PerModuleInterface::g_pSystemTable->weapon_behaviors[
			WeaponType::Plasmagun
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
				&& !curr_task->Is<Task_Idle_Plasmagun>()
				)
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


		//
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

REGISTERSINGLETON(TbWeaponBehavior_Plasmagun,true);
