#pragma once


#include <Core/Tasking/TinyCoroutines.h>
#include <Renderer/Modules/Animation/AnimatedModel.h>
#include <Developer/RunTimeCompiledCpp.h>
#include <Utility/GameFramework/TbPlayerWeapon.h>

#include "game_forward_declarations.h"

struct CoTimestamp;
struct NwWeaponBehaviorI;


/// the order is used by weapon slots in inventory
/// TODO: shouldn't be hardcoded!
///
struct WeaponType
{
	enum Enum
	{
		// always available
		//Fists = 0,

		//Shotgun,
		Chaingun,
		Plasmagun,
		//Railgun,
		RocketLauncher,

		Count,

		Default = WeaponType::Plasmagun
	};

	static Enum IntToEnumSafe( unsigned u )
	{
		return (Enum) (u % WeaponType::Count);
	}

	static bool HasBarrel(Enum weapon_type)
	{
		//return weapon_type != Fists;
		return true;
	}
};

mxSTATIC_ASSERT2(WeaponType::Count <= 32,
				 Weapon_statuses_are_stored_in_a_32_bit_integer);

mxDECLARE_ENUM( WeaponType::Enum, U8, WeaponType8 );


///*
//----------------------------------------------------------
//	EWeaponState
//----------------------------------------------------------
//*/

enum EWeaponControlButton
{
	WEAPON_BUTTON_PRIMARY_FIRE		= BIT(0),
	WEAPON_BUTTON_SECONDARY_FIRE	= BIT(1),

	WEAPON_BUTTON_RELOAD			= BIT(2),
};

struct WeaponState
{
	enum Enum
	{
		OutOfAmmo = 0,
		IdleReady,
		Raising,
		Firing,
		Reloading,
		Lowering,
		Holstered,	// in this state the player can switch weapons right away
	};
};

/*
----------------------------------------------------------
	NwPlayerWeapon
----------------------------------------------------------
*/
struct NwPlayerWeapon
	: CStruct, NonCopyable
	, RunTimeCompiledCpp::ListenerI
{
	//
	WeaponType::Enum		type;

	TPtr< NwWeaponBehaviorI >	behavior;

	TResPtr< NwWeaponDef >	def;

	TResPtr< idEntityDef >	entity_def;

	//
	TPtr< NwAnimatedModel >	anim_model;

	// 0 - normal height;
	// negative => the weapon is being lowered or is hidden.
	float					curr_height_for_hiding;

	//
	// requested  state - a mask of EWeaponControlButton
	U32		pressed_buttons;



	//EWeaponState	current_state;
	//EWeaponState	requested_state;

	//int		animBlendFrames;	// time to blend in for the next animation, in MD5 frames
	//int		animDoneTimeMsec;	// 

	//float	next_attack;	// seconds

	// how much ammo is left in the clip
	U32		ammo_in_clip;

	// how much ammo is left in inventory
	U32		recharge_ammo;

	WeaponState::Enum	current_state;

	//
	int		barrel_joint_index;

public:
	mxDECLARE_CLASS(NwPlayerWeapon, CStruct);
	mxDECLARE_REFLECTION;
	NwPlayerWeapon();

	//

	virtual void RTCPP_OnConstructorsAdded() override;


	void ResetAmmoOnRespawn();

public:	// Behaviour

	bool CanBeReloaded() const
	{
		const NwWeaponDef& weapon_def = *this->def;
		return (this->ammo_in_clip < weapon_def.clip_size)
			&& this->recharge_ammo
			;
	}

	/// Auto-Reload
	bool ShouldBeAutoReloaded() const
	{
		return (this->ammo_in_clip == 0)
			&& this->CanBeReloaded()
			;
	}

	void primaryAttack();
	void reload();

	CoTaskI* CreateTask_LowerAnimated(
		CoTaskQueue & task_scheduler
		, const CoTimestamp& now
		);
	CoTaskI* CreateTask_RaiseAnimated(
		CoTaskQueue & task_scheduler
		, const CoTimestamp& now
		);

	void UpdateOncePerFrame(
		float delta_time_seconds
		 , const M44f& camera_world_matrix
		);

public:
	bool getMuzzlePosAndDirInWorldSpace(
		V3f &pos_
		, V3f &dir_
		//, const NwCameraView& camera_view
		) const;

public:	// Rendering

	void AddMuzzleFlashToWorld(
		ARenderWorld& render_world
		, const V3f& position
		, const CoTimestamp& now
		);

	void DrawCrosshair(
		ADebugDraw& dbg_draw
		, const NwCameraView& camera_view
		);

	void Dbg_DrawRayFromMuzzle(
		MyGameRenderer& renderer
		);

public:
	void dbg_SetFullyLoaded();

private:
	CoTaskI* _CreateTask_ChangeWeaponHeight(
		CoTaskQueue & task_scheduler
		, const CoTimestamp& now
		, bool for_raising_weapon
		);
};

namespace NwPlayerWeapon_
{
	ERet Create(
		NwPlayerWeapon *& new_weapon_obj_
		, NwClump & clump
		, const TResPtr<idEntityDef>& entity_def
		, const TResPtr<NwSkinnedMesh>& skinned_mesh
		, const TResPtr<NwWeaponDef>& weapon_def
		, const WeaponType::Enum weapon_type
		);

	void Destroy(
		NwPlayerWeapon *& weapon_obj
		, NwClump & clump
		);

	const NameHash32 LOWER_THE_WEAPON_ANIM_ID = mxHASH_STR("putaway");
	const NameHash32 RAISE_THE_WEAPON_ANIM_ID = mxHASH_STR("raise");

}//namespace





/*
----------------------------------------------------------
	NwWeaponBehaviorI

	weapon behaviours can be specified in scripts
	or in C++ code (preferrably, in run-time compiled C++ files)
----------------------------------------------------------
*/
struct NwWeaponBehaviorI
{
	virtual void UpdateWeapon(
		NwPlayerWeapon & weapon
		)
	{};

	///// e.g. is playing an animation that cannot be interrupted
	//virtual bool IsBusy()
	//{
	//	return false;
	//}

	mxREMOVE_THIS
	/*
	virtual int primaryAttack(
		NwPlayerWeapon & weapon
		)
	{return 0;};

	virtual void secondaryAttack(
		NwPlayerWeapon & weapon
		)
	{};

	//

	virtual void Lower(
		NwPlayerWeapon & weapon
		)
	{};

	virtual void Raise(
		NwPlayerWeapon & weapon
		)
	{};

	virtual void Idle()
	{};

	virtual void Reload(
		NwPlayerWeapon & weapon
		)
	{};
	*/
};
