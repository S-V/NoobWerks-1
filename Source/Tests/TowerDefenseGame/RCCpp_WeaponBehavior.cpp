//
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/RuntimeInclude.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/IObject.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/ObjectInterfacePerModule.h>
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes

RUNTIME_COMPILER_LINKLIBRARY("ProfilerCore32.lib");
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
//
#include <Utility/GameFramework/TbPlayerWeapon.h>

//
#include "RCCpp_WeaponBehavior.h"

// RCC++ uses interface Id's to distinguish between different classes
// here we have only one, so we don't need a header for this enum and put it in the same
// source code file as the rest of the code
enum InterfaceIDEnumConsoleExample
{
    IID_IRCCPP_MAIN_LOOP = IID_ENDInterfaceID, // IID_ENDInterfaceID from IObject.h InterfaceIDEnum

    IID_ENDInterfaceIDEnumConsoleExample
};





// for converting from 24 frames per second to milliseconds
inline int FRAME2MS( int framenum ) {
	return ( framenum * 1000 ) / 24;
}


extern void test_PlaySound();



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




//Optional
//TOptional<>
enum TaskStatus: int {
	Initial = 0,
	Running,
	Completed = ~0,
};

// 

struct AsyncTask
	: AObject
	, NonCopyable
{
	TaskStatus	zz__state;

public:
	mxDECLARE_CLASS(AsyncTask,AObject);
	mxDECLARE_REFLECTION;

	AsyncTask()
	{
		restart();
	}

	void restart() { zz__state = Initial; }

	/// Return true if the protothread is running or waiting, false if it has ended or exited.
	bool isRunning() const;

	bool isDone() const { return zz__state == Completed; }

	virtual TaskStatus run( void* user_data ) = 0;
};

mxDEFINE_ABSTRACT_CLASS(AsyncTask);
mxBEGIN_REFLECTION(AsyncTask)
mxMEMBER_FIELD_OF_TYPE(zz__state, T_DeduceTypeInfo<int>()),
mxEND_REFLECTION



#define $BEGIN	{ switch(zz__state) { case Initial:
#define $END	default: return (zz__state = Completed); } }

#define $WAIT_WHILE(cond)	zz__state = TaskStatus(__LINE__); case __LINE__: if (cond) return Running
#define $WAIT_UNTIL(cond)	$WAIT_WHILE(!(cond))


#define $YIELD		zz__state = TaskStatus(__LINE__); return TaskStatus(__LINE__); case __LINE__:

#define $EXIT		return (zz__state = Completed);




/// returns anim duration, in milliseconds
U32 playAnim2(
			   NwPlayerWeapon & weapon
			  , const NameHash32 anim_name_hash
			  , const char* dbgname
			  , F32 transition_duration = TB_DEFAULT_ANIMATION_BLEND_TIME
			  , bool looped = true
			  , bool remove_on_completion = true
			  )
{
//ptWARN("Play anim: '%s' (0x%X), blend duration: %.3f",
//	dbgname, anim_name_hash, transition_duration
//	);

	const NwAnimClip* anim = weapon
		._animated_model
		._skinned_model->findAnimByNameHash( anim_name_hash )
		;

	mxASSERT( anim );

	if( anim )
	{
		F32 anim_duration_seconds = anim->animation->ozz_animation.duration();
		U32 anim_duration_millseconds = SEC2MS( anim_duration_seconds );
//ptWARN("anim '%s' duration: %.3f seconds", dbgname, anim_duration_seconds);

//anim_duration_seconds = 5;

//strcpy_s(anim->animation->dbg_name, dbgname);


		//weapon.animDoneTimeMsec = SEC2MS( anim_duration_seconds );

		//weapon.curr_anim_timer.start( SEC2MS( 6 ) );
		weapon.curr_anim_timer.startWithDurationInMilliseconds( anim_duration_millseconds );

		weapon
			._animated_model
			.animator
			.animation_controller.PlayAnim_Additive(
				anim
				, transition_duration
				, 1.0f	// F32 target_blend_weight
				, looped	// bool looped
				, remove_on_completion
			);

		return anim_duration_millseconds;
	}
	else
	{
		mxASSERT(0&&"anim not found!");
		weapon.curr_anim_timer.reset();

		return 0;
	}
}

//bool animDone(
//			  NwPlayerWeapon & weapon
//			  , const int blendFrames = 0
//			  )
//{
//	return ( weapon.animDoneTimeMsec - FRAME2MS( blendFrames ) <= tbGetTimeInMilliseconds() );
//}
//


U32 ammoInClip( const NwPlayerWeapon& weapon )
{
	return weapon.ammo_in_clip;
}






struct Task_Idle_GrenadeLauncher: AsyncTask
{
public:
	mxDECLARE_CLASS(Task_Idle_GrenadeLauncher, AsyncTask);
	mxDECLARE_REFLECTION;

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

//ptWARN("ENTER IDLE!");

		playAnim2( weapon
			, mxHASH_STR("idle")
			, "idle"
			, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
			, true	// looped
			, false	// remove_on_completion
			);

		$WAIT_UNTIL( weapon.curr_anim_timer.isExpired() );

		$END;
	}
};
mxDEFINE_CLASS(Task_Idle_GrenadeLauncher);
mxBEGIN_REFLECTION(Task_Idle_GrenadeLauncher)
mxEND_REFLECTION


const V3f		M44_getForward( const M44f& m )
{
	return Vector4_As_V3( m.r1 );
}

void getMuzzlePosAndDirInWorldSpace(
									const NwPlayerWeapon& weapon
									, V3f &pos_
									, V3f &dir_
									)
{
	const RrAnimatedModel&	animated_model = weapon._animated_model;

	const M44f	local_to_world = animated_model.render_model.transform->getLocalToWorld4x4Matrix();

	const RrModelAnimator& animator = animated_model.animator;
	const ozz::animation::Skeleton& skeleton = animated_model._skinned_model->skeleton;

	const ozz::Range<const char* const> joint_names = skeleton.joint_names();

	int barrel_joint_index = getJointIndexByName(
		"barrel"
		, skeleton
		);

	mxASSERT( barrel_joint_index != INDEX_NONE );
	if( barrel_joint_index != INDEX_NONE )
	{
		const M44f& muzzle_joint_matrix_model_space = ozzMatricesToM44f( animator.models[ barrel_joint_index ] );

		const V3f muzzle_joint_pos_model_space
			= M44_getTranslation( muzzle_joint_matrix_model_space )
			//+ CV3f( -0.1f, 0, 0 )	// -20 cm
			;

		//
		const V3f muzzle_joint_pos_world_space
			= M44_TransformPoint( local_to_world, muzzle_joint_pos_model_space )
			;

		const V3f muzzle_joint_axis_model_space = V3_Normalized( Vector4_As_V3( muzzle_joint_matrix_model_space.r[2] ) );
		const V3f muzzle_joint_axis_world_space = M44_TransformNormal( local_to_world, muzzle_joint_axis_model_space );


		const V3f barrel_direction_world_space = V3_Normalized( muzzle_joint_axis_world_space );//V3_Normalized( muzzle_joint_pos_world_space - muzzle_parent_joint_pos_world_space );

		//
		TbRenderSystemI::Get().m_debug_line_renderer.DrawLine(
			muzzle_joint_pos_world_space, muzzle_joint_pos_world_space + barrel_direction_world_space * 10.0f
			, RGBAf::RED, RGBAf::WHITE
			);

		pos_ = muzzle_joint_pos_world_space;
		dir_ = barrel_direction_world_space;
	}
	else
	{
		pos_ = M44_getTranslation( local_to_world );
		dir_ = V3_Normalized( M44_getForward( local_to_world ) );
	}
}


struct Task_Fire_GrenadeLauncher: AsyncTask
{
	Async::Timer	next_attack_timer;
	Async::Timer	fire_anim_timer;

public:
	mxDECLARE_CLASS(Task_Fire_GrenadeLauncher, AsyncTask);
	mxDECLARE_REFLECTION;
	Task_Fire_GrenadeLauncher()
	{
	}

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

ptWARN("ENTER FIRE! : (time=%u)", tbGetTimeInMilliseconds());

mxASSERT(weapon.ammo_in_clip > 0);

		// cannot fire faster than the rate of fire
		$WAIT_UNTIL( next_attack_timer.isExpired() );

ptWARN("MUZZLE FLASH! : (time=%u)", tbGetTimeInMilliseconds());

		// muzzle flash
		{
			RrMeshInstance& render_model = weapon._animated_model.render_model;

			// these will be different each fire

			idRandom		rng;
			rng.SetSeed( tbGetTimeInMilliseconds() );

			render_model.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( tbGetTimeInMilliseconds() );
			render_model.shaderParms[ SHADERPARM_DIVERSITY ]	= rng.RandomFloat(0, 1);
		}

		// launch projectile
		{
			const NwWeaponTable& weapon_def = *weapon.def;

			//
			TbGameEvent	new_event;
			new_event.type = EV_PROJECTILE_FIRED;
			new_event.timestamp = Testbed::gameTime();

			getMuzzlePosAndDirInWorldSpace(
				weapon
				, new_event.shot_fired.position
				, new_event.shot_fired.direction
				);
			new_event.shot_fired.projectile_mass = weapon_def.projectile_mass;
			new_event.shot_fired.lin_velocity = weapon_def.initial_velocity;

			new_event.shot_fired.col_radius = weapon_def.projectile_collision_radius;

			//
			EventSystem::PostEvent( new_event );
		}

		//
		--weapon.ammo_in_clip;

		//
		next_attack_timer.startWithDurationInMilliseconds( SEC2MS(ROCKETLAUNCHER_FIREDELAY_SECONDS) );

		//
		{
ptWARN("START FIRE ANIM!");
			U32 anim_length_msec = playAnim2( weapon
				, mxHASH_STR("fire")
				, "fire"
				, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
				, false	// looped
				, true	// remove_on_completion
				);

			fire_anim_timer.startWithDurationInMilliseconds( anim_length_msec );
		}

		// wait until the "fire" anim is finished
		$WAIT_UNTIL( fire_anim_timer.isExpired() );

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$WAIT_UNTIL( next_attack_timer.isExpired() );


ptWARN("Timer ENDED!");

		$END;
	}
};
mxDEFINE_CLASS(Task_Fire_GrenadeLauncher);
mxBEGIN_REFLECTION(Task_Fire_GrenadeLauncher)
mxEND_REFLECTION






struct Task_Reload_GrenadeLauncher: AsyncTask
{
	Async::Timer	reload_anim_timer;

public:
	mxDECLARE_CLASS(Task_Reload_GrenadeLauncher, AsyncTask);
	mxDECLARE_REFLECTION;
	Task_Reload_GrenadeLauncher()
	{
	}

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

ptWARN("ENTER RELOAD! : (time=%u)", tbGetTimeInMilliseconds());

		//
		{
			U32 anim_length_msec = playAnim2( weapon
				, mxHASH_STR("reload")
				, "reload"
				, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
				, false	// looped
				, true	// remove_on_completion
				);

			reload_anim_timer.startWithDurationInMilliseconds( anim_length_msec );
		}

		// wait until the "reload" anim is finished
		$WAIT_UNTIL( reload_anim_timer.isExpired() );

		// the weapon is now loaded
		weapon.ammo_in_clip = ROCKETLAUNCHER_AMMO_CAPACITY;

		$END;
	}
};
mxDEFINE_CLASS(Task_Reload_GrenadeLauncher);
mxBEGIN_REFLECTION(Task_Reload_GrenadeLauncher)
mxEND_REFLECTION


///
struct TbWeaponBehavior_GrenadeLauncher
	: TbWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	Task_Idle_GrenadeLauncher	task_idle;
	Task_Fire_GrenadeLauncher	task_fire;
	Task_Reload_GrenadeLauncher	task_reload;
	AsyncTask *	curr_task;

public:
    TbWeaponBehavior_GrenadeLauncher()
    {
		curr_task = &task_idle;

		PerModuleInterface::g_pSystemTable->grenade_launcher = this;
    }

	void switchToTask( AsyncTask * new_task )
	{
		curr_task = new_task;
		new_task->restart();
	}

	virtual int primaryAttack(
		NwPlayerWeapon & weapon
		) override
	{
//DBGOUT("\n=== primaryAttack!!!!!!! (time=%u)", tbGetTimeInMilliseconds());
		if( !curr_task->Is<Task_Idle_GrenadeLauncher>() && !curr_task->isDone() )
		{
			// firing or reloading
		}
		else
		{
//DBGOUT("RESTART 'FIRE' TASK!");
			if( weapon.ammo_in_clip == 0 )
			{
				curr_task = &task_reload;
				curr_task->restart();
			}
			else
			{
				curr_task = &task_fire;
				curr_task->restart();
			}
		}

		return 0;
	};

	virtual void Reload(
		NwPlayerWeapon & weapon
		) override
	{
		if( !curr_task->Is<Task_Idle_GrenadeLauncher>() && !curr_task->isDone() )
		{
			// firing or reloading
		}
		else
		{
			switchToTask( &task_reload );
		}
	};

	int transitionToState(
		NwPlayerWeapon & weapon
		, const EWeaponState new_state
		)
	{
		if( weapon.current_state == new_state )
		{
			return 0;
		}

		weapon.requested_state = new_state;

#if 0
		weapon.state = new_state;

		//
		weapon.animBlendFrames = blendFrames;

		//
		NameHash32 anim_name_hash = mxHASH_STR("idle");

		switch( new_state )
		{
		case WEAPON_STATE_IDLE_READY:
ptWARN("Play anim: idle");
			anim_name_hash = mxHASH_STR("idle");
			break;

		case WEAPON_STATE_RAISING:
ptWARN("Play anim: raise");
			anim_name_hash = mxHASH_STR("raise");
			break;

		case WEAPON_STATE_LOWERING:
ptWARN("Play anim: lower");
			anim_name_hash = mxHASH_STR("lower");
			break;

		case WEAPON_STATE_FIRING:
ptWARN("Play anim: fire");
			anim_name_hash = mxHASH_STR("fire");
			break;

		case WEAPON_STATE_RELOADING:
ptWARN("Play anim: reload");
			anim_name_hash = mxHASH_STR("reload");
			break;

		case WEAPON_STATE_HOLSTERED:
			break;

		default:
			break;
		}

		F32 blend_time = MS2SEC( FRAME2MS( blendFrames ) );

		playAnim(
			anim_name_hash
			, weapon
			, blend_time
			);
#endif

		return 0;
	}

    virtual void updateWeapon(
		NwPlayerWeapon & weapon
		) override
	{

		//DBG!
V3f	pos, dir;
getMuzzlePosAndDirInWorldSpace( weapon, pos, dir );

		//
		const ozz::animation::Skeleton& skel = weapon._animated_model._skinned_model->skeleton;
		const ozz::math::SoaTransform& bindpose0 = skel.joint_bind_poses()[0];


		if( curr_task ) {
			TaskStatus status = curr_task->run( &weapon );
			if( status == Completed ) {
				curr_task = &task_idle;
				curr_task->restart();
			}
		}
	};

private:
	void goToState(
		NwPlayerWeapon & weapon
		, const EWeaponState new_state
		, const int blendFrames = 0
		)
	{
//		mxASSERT( weapon.state != new_state );

		weapon.current_state = new_state;

		//
//		weapon.animBlendFrames = blendFrames;

		//
		NameHash32 anim_name_hash = mxHASH_STR("idle");

		switch( new_state )
		{
		case WEAPON_STATE_IDLE_READY:
ptWARN("Play anim: idle");
			anim_name_hash = mxHASH_STR("idle");
			break;

		case WEAPON_STATE_RAISING:
ptWARN("Play anim: raise");
			anim_name_hash = mxHASH_STR("raise");
			break;

		case WEAPON_STATE_LOWERING:
ptWARN("Play anim: lower");
			anim_name_hash = mxHASH_STR("lower");
			break;

		case WEAPON_STATE_FIRING:
ptWARN("Play anim: fire");
			anim_name_hash = mxHASH_STR("fire");
			break;

		case WEAPON_STATE_RELOADING:
ptWARN("Play anim: reload");
			anim_name_hash = mxHASH_STR("reload");
			break;

		case WEAPON_STATE_HOLSTERED:
			break;

		default:
			break;
		}

		//F32 blend_time = MS2SEC( FRAME2MS( blendFrames ) );

		//playAnim(
		//	anim_name_hash
		//	, weapon
		//	, blend_time
		//	);
	}
};

REGISTERSINGLETON(TbWeaponBehavior_GrenadeLauncher,true);

















#define PLASMAGUN_FIRERATE			0.1//0.125 //changed by Tim
#define PLASMAGUN_LOWAMMO			10
#define PLASMAGUN_NUMPROJECTILES	1

#define PLASMAGUN_MAX_AMMO_IN_CLIP		30	//50

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
struct Task_Idle_Plasmagun: AsyncTask
{
	Async::Timer	idle_anim_timer;

public:
	mxDECLARE_CLASS(Task_Idle_Plasmagun, AsyncTask);
	mxDECLARE_REFLECTION;

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

		{
			U32 anim_length_msec = playAnim2( weapon
				, mxHASH_STR("idle")
				, "idle"
				, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
				, true	// looped
				, false	// remove_on_completion
				);

			idle_anim_timer.startWithDurationInMilliseconds( anim_length_msec );
		}

		$WAIT_UNTIL( idle_anim_timer.isExpired() );

		$END;
	}
};
mxDEFINE_CLASS(Task_Idle_Plasmagun);
mxBEGIN_REFLECTION(Task_Idle_Plasmagun)
mxEND_REFLECTION

/*
----------------------------------------------------------
	Task_Fire_Plasmagun
----------------------------------------------------------
*/
struct Task_Fire_Plasmagun: AsyncTask
{
	Async::Timer	next_attack_timer;
	Async::Timer	fire_anim_timer;

public:
	mxDECLARE_CLASS(Task_Fire_Plasmagun, AsyncTask);
	mxDECLARE_REFLECTION;
	Task_Fire_Plasmagun()
	{
	}

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

		mxASSERT(weapon.ammo_in_clip > 0);

		// cannot fire faster than the rate of fire
		$WAIT_UNTIL( next_attack_timer.isExpired() );

		// muzzle flash
		{
			RrMeshInstance& render_model = weapon._animated_model.render_model;

			// these will be different each fire

			idRandom		rng;
			rng.SetSeed( tbGetTimeInMilliseconds() );

			render_model.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( tbGetTimeInMilliseconds() );
			render_model.shaderParms[ SHADERPARM_DIVERSITY ]	= rng.RandomFloat(0, 1);
		}

		// launch projectile
		//
		//test_PlaySound();

		--weapon.ammo_in_clip;

		//
		next_attack_timer.startWithDurationInMilliseconds( SEC2MS(PLASMAGUN_FIRERATE) );

		//
		{
			U32 anim_length_msec = playAnim2( weapon
				, mxHASH_STR("fire1")
				, "fire"
				, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
				, false	// looped
				, true	// remove_on_completion
				);

			fire_anim_timer.startWithDurationInMilliseconds( anim_length_msec );
		}

		// wait until the "fire" anim is finished
		//$WAIT_UNTIL( fire_anim_timer.isExpired() );

		// NOTE: I also need to wait here to prevent firing after the "Fire" button was released.
		$WAIT_UNTIL( next_attack_timer.isExpired() );

		$END;
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
struct Task_Reload_Plasmagun: AsyncTask
{
	Async::Timer	reload_anim_timer;

public:
	mxDECLARE_CLASS(Task_Reload_Plasmagun, AsyncTask);
	mxDECLARE_REFLECTION;
	Task_Reload_Plasmagun()
	{
	}

	virtual TaskStatus run( void* user_data ) override
	{
		NwPlayerWeapon & weapon = *(NwPlayerWeapon*) user_data;

		$BEGIN;

		//
		{
			U32 anim_length_msec = playAnim2( weapon
				, mxHASH_STR("reload")
				, "reload"
				, TB_DEFAULT_ANIMATION_BLEND_TIME	// transition_duration
				, false	// looped
				, true	// remove_on_completion
				);

			reload_anim_timer.startWithDurationInMilliseconds( anim_length_msec );
		}

		// wait until the "reload" anim is finished
		$WAIT_UNTIL( reload_anim_timer.isExpired() );

		// the weapon is now loaded
		weapon.ammo_in_clip = PLASMAGUN_MAX_AMMO_IN_CLIP;

		$END;
	}
};
mxDEFINE_CLASS(Task_Reload_Plasmagun);
mxBEGIN_REFLECTION(Task_Reload_Plasmagun)
mxEND_REFLECTION


///
struct TbWeaponBehavior_Plasmagun
	: TbWeaponBehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
{
	Task_Idle_Plasmagun	task_idle;
	Task_Fire_Plasmagun	task_fire;
	Task_Reload_Plasmagun	task_reload;
	AsyncTask *	curr_task;

public:
    TbWeaponBehavior_Plasmagun()
    {
		curr_task = &task_idle;

		PerModuleInterface::g_pSystemTable->plasmagun = this;
    }

	void switchToTask( AsyncTask * new_task )
	{
		curr_task = new_task;
		new_task->restart();
	}

	virtual int primaryAttack(
		NwPlayerWeapon & weapon
		) override
	{
//DBGOUT("\n=== primaryAttack!!!!!!! (time=%u)", tbGetTimeInMilliseconds());
		if( !curr_task->Is<Task_Idle_Plasmagun>() && !curr_task->isDone() )
		{
			// firing or reloading
		}
		else
		{
//DBGOUT("RESTART 'FIRE' TASK!");
			if( weapon.ammo_in_clip == 0 )
			{
				curr_task = &task_reload;
				curr_task->restart();
			}
			else
			{
				curr_task = &task_fire;
				curr_task->restart();
			}
		}

		return 0;
	};

	virtual void Reload(
		NwPlayerWeapon & weapon
		) override
	{
		if( !curr_task->Is<Task_Idle_Plasmagun>() && !curr_task->isDone() )
		{
			// firing or reloading
		}
		else
		{
			switchToTask( &task_reload );
		}
	};

    virtual void updateWeapon(
		NwPlayerWeapon & weapon
		) override
	{
		const ozz::animation::Skeleton& skel = weapon._animated_model._skinned_model->skeleton;
		const ozz::math::SoaTransform& bindpose0 = skel.joint_bind_poses()[0];

		if( curr_task ) {
			TaskStatus status = curr_task->run( &weapon );
			if( status == Completed ) {
				curr_task = &task_idle;
				curr_task->restart();
			}
		}
	};
};

REGISTERSINGLETON(TbWeaponBehavior_Plasmagun,true);








//namespace WeaponBehaviors
//{
//	TPtr< TbWeaponBehaviorI >	grenade_launcher;
//}//namespace WeaponBehaviors
SystemTable              g_SystemTable;

//TbWeaponBehaviorI* TbWeaponBehaviorI::getWeaponBehaviorByWeaponId(
//	TbPlayerWeaponId weapon_id
//	)
//{
//	if( weapon_id == PLAYER_WEAPON_ID_GRENADE_LAUNCHER ) {
//		return g_SystemTable.grenade_launcher;
//	}
//	if( weapon_id == PLAYER_WEAPON_ID_PLASMAGUN ) {
//		return g_SystemTable.plasmagun;
//	}
//
//	UNDONE;
//	return nil;
//}
