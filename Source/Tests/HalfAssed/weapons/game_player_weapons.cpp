#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/Random.h>	// idRandom
//
#include <Core/Tasking/TinyCoroutines.h>	// CoTimestamp
#include <Core/Util/Tweakable.h>
#include <Renderer/Modules/idTech4/idEntity.h>

//#include "experimental/game_experimental.h"
#include "experimental/rccpp.h"
#include "game_animation/game_animation.h"
#include "game_rendering/game_renderer.h"
#include "weapons/game_player_weapons.h"

mxTEMP
#include "FPSGame.h"

//#include <Renderer/Modules/idTech4/idEntity.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>

mxBEGIN_REFLECT_ENUM( WeaponType8 )
	//mxREFLECT_ENUM_ITEM( Fists, WeaponType::Fists ),
	//mxREFLECT_ENUM_ITEM( Shotgun, WeaponType::Shotgun ),
	mxREFLECT_ENUM_ITEM( Chaingun, WeaponType::Chaingun ),
	mxREFLECT_ENUM_ITEM( Plasmagun, WeaponType::Plasmagun ),
	//mxREFLECT_ENUM_ITEM( Railgun, WeaponType::Railgun ),
	mxREFLECT_ENUM_ITEM( RocketLauncher, WeaponType::RocketLauncher ),
mxEND_REFLECT_ENUM

/*
----------------------------------------------------------
	NwPlayerWeapon
----------------------------------------------------------
*/
mxDEFINE_CLASS(NwPlayerWeapon);
mxBEGIN_REFLECTION(NwPlayerWeapon)
mxEND_REFLECTION;
NwPlayerWeapon::NwPlayerWeapon()
{
	type = WeaponType::Default;

	curr_height_for_hiding = 0;

	pressed_buttons = 0;

	ammo_in_clip = 0;
	recharge_ammo = 0;

	current_state = WeaponState::OutOfAmmo;

	barrel_joint_index = INDEX_NONE;

	RTCPP_OnConstructorsAdded();
}

void NwPlayerWeapon::RTCPP_OnConstructorsAdded()
{
	behavior = g_SystemTable.weapon_behaviors[ this->type ];
}

void NwPlayerWeapon::ResetAmmoOnRespawn()
{
	const NwWeaponDef& weapon_def = *this->def;
	recharge_ammo = weapon_def.max_recharge_ammo;
}

void NwPlayerWeapon::primaryAttack()
{
	pressed_buttons |= WEAPON_BUTTON_PRIMARY_FIRE;
	//pressed_buttons |= WEAPON_BUTTON_SECONDARY_FIRE;

	//if( NwWeaponBehaviorI * behavior = NwWeaponBehaviorI::getWeaponBehaviorByWeaponId( weapon_id ) )
	//{
	//	behavior->primaryAttack( *this );
	//}
}

void NwPlayerWeapon::reload()
{
	pressed_buttons |= WEAPON_BUTTON_RELOAD;
	//if( NwWeaponBehaviorI * behavior = NwWeaponBehaviorI::getWeaponBehaviorByWeaponId( weapon_id ) )
	//{
	//	behavior->Reload( *this );
	//}
}

void NwPlayerWeapon::dbg_SetFullyLoaded()
{
	const NwWeaponDef& weapon_def = *this->def;
	ammo_in_clip = weapon_def.clip_size;
	recharge_ammo = weapon_def.max_recharge_ammo;
}

CoTaskI* NwPlayerWeapon::CreateTask_LowerAnimated(
	CoTaskQueue & task_scheduler
	, const CoTimestamp& now
	)
{
	return _CreateTask_ChangeWeaponHeight(task_scheduler, now, false);
}

CoTaskI* NwPlayerWeapon::CreateTask_RaiseAnimated(
	CoTaskQueue & task_scheduler
	, const CoTimestamp& now
	)
{
	return _CreateTask_ChangeWeaponHeight(task_scheduler, now, true);
}

CoTaskI* NwPlayerWeapon::_CreateTask_ChangeWeaponHeight(
	CoTaskQueue & task_scheduler
	, const CoTimestamp& now
	, bool for_raising_weapon
	)
{
	//
	struct CoTask_ChangeWeaponHeight: CoTaskI
	{
		NwPlayerWeapon &	weapon;

		const NwWeaponParamsForHidingDef	hide_params;
		const bool							raise_weapon;

		//
		CoTimestamp					start_time;

		float	curr_interp_weapon_offset;

		bool	is_first_run;

	public:
		CoTask_ChangeWeaponHeight(
			NwPlayerWeapon& weapon
			, const NwWeaponParamsForHidingDef& hide_params
			, const bool raise_weapon
			, const CoTimestamp& now
			)
			: weapon(weapon)
			, hide_params(hide_params)
			, raise_weapon(raise_weapon)
			, curr_interp_weapon_offset(0)
			, start_time(now)
			, is_first_run(true)
		{
		}
		virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
		{
			if(is_first_run)
			{
				is_first_run = false;

				start_time = now;

				if( raise_weapon ) {
					// set hidden
					weapon.curr_height_for_hiding = -hide_params.hide_distance;
				} else {
					weapon.curr_height_for_hiding = 0;
				}
			}

			//
			const U64 time_elapsed_msec = now.time_msec - start_time.time_msec;
			const float01_t hide_distance_lerp = minf(
				float(time_elapsed_msec) / float(hide_params.hide_time_msec),
				1
				);

//DBGOUT("LOWER_WEAPON: time_elapsed_msec: %d msec, hide_distance_lerp = %f",
//	(int)time_elapsed_msec, hide_distance_lerp
//	);

if( raise_weapon ) {
	weapon.curr_height_for_hiding = (hide_distance_lerp - 1) * hide_params.hide_distance;
} else {
	weapon.curr_height_for_hiding = -hide_distance_lerp * hide_params.hide_distance;
}

//DEVOUT("LOWER_WEAPON: WAITING: %d msec (curr_height_for_hiding = %f)",
//	hide_params.hide_time_msec, weapon.curr_height_for_hiding
//	);
			//
			$CoBEGIN;

			if( raise_weapon )
			{
				//
#if GAME_CFG_WITH_SOUND
				{
					const TResPtr<NwSoundSource>* sound_res = weapon.entity_def->sounds.find(
						mxHASH_STR("snd_acquire")
						);
					mxASSERT(sound_res);
					if(sound_res)
					{
						FPSGame::Get().sound.PlaySound3D(
							sound_res->_id
							, weapon.anim_model->render_model->local_to_world.translation()
							);
					}
				}
#endif // GAME_CFG_WITH_SOUND

				//
				weapon.anim_model->PlayAnim(
					NwPlayerWeapon_::RAISE_THE_WEAPON_ANIM_ID
					, NwPlayAnimParams()
					.SetFadeInTime(0)
					// In "Lower Weapon" anim, the weapon is hidden at time = 0.5.
					// At time = 1.0 it's visible again.
					//.SetMaxTimeRatio(0.5f)
					, PlayAnimFlags::Defaults
					, NwStartAnimParams().SetStartTimeRatio(0.1).SetSpeed(0.5)
					);

				$CoWAIT_MSEC( hide_params.hide_time_msec, now );

				$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(
					NwPlayerWeapon_::RAISE_THE_WEAPON_ANIM_ID
					));

			}
			else
			{
				weapon.anim_model->PlayAnim(
					NwPlayerWeapon_::LOWER_THE_WEAPON_ANIM_ID
					, NwPlayAnimParams()
					// In "Lower Weapon" anim, the weapon is hidden at time = 0.5.
					// At time = 1.0 it's visible again.
					//.SetMaxTimeRatio(0.5f)
					,
					PlayAnimFlags::DoNotRestart
					, NwStartAnimParams().SetSpeed(0.5)
					);

				$CoWAIT_MSEC( hide_params.hide_time_msec, now );

				$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(
					NwPlayerWeapon_::LOWER_THE_WEAPON_ANIM_ID
					));
			}

//DBGOUT("LOWER_WEAPON: FINISHED: time_elapsed_msec: %d msec, hide_distance_lerp = %f",
//	time_elapsed_msec, hide_distance_lerp
//	);

			$CoEND;
		}
	};

	//
	NwWeaponParamsForHidingDef params = def->params_for_hiding;
	// so that all weapons are beneath the screen in lowered state
	params.hide_distance = 0.03f;
	params.hide_time_msec = 300;

	//
	CoTask_ChangeWeaponHeight* new_task = task_scheduler.task_allocator.new_<CoTask_ChangeWeaponHeight>(
		*this
		, params
		, for_raising_weapon
		, now
		);

	return new_task;
}

bool NwPlayerWeapon::getMuzzlePosAndDirInWorldSpace(
	V3f &pos_
	, V3f &dir_
	//, const NwCameraView& camera_view
	) const
{
	if(mxUNLIKELY(!WeaponType::HasBarrel(this->type))) {
		return false;
	}

	const NwAnimatedModel&	animated_model = *this->anim_model;

	const M44f	local_to_world = animated_model.render_model->local_to_world;

	//
	const V3f muzzle_joint_pos_model_space = animated_model.job_result.barrel_joint_mat_local.translation();

	// correct!
	const V3f muzzle_joint_pos_world_space
		= M44_TransformPoint( local_to_world, muzzle_joint_pos_model_space )
		;

	//
	const V3f muzzle_joint_axis_model_space = V3_Normalized( Vector4_As_V3(
		// Use r[0] (X axis) for all weapons, except rocket launcher
		(this->type != WeaponType::RocketLauncher)
		? animated_model.job_result.barrel_joint_mat_local.r[0]
	: animated_model.job_result.barrel_joint_mat_local.r[2]	// rocket launcher - barrel joint doesn't point straight, so rotate it
	) );

	const V3f muzzle_joint_axis_world_space = M44_TransformNormal( local_to_world, muzzle_joint_axis_model_space );
	const V3f barrel_direction_world_space = V3_Normalized( muzzle_joint_axis_world_space );

	//
	pos_ = muzzle_joint_pos_world_space;

	//
	const V3f player_look_direction_in_world_space = FPSGame::Get().player.camera.m_look_direction;

	// if barrel joint's dir is close to player camera look dir, use the camera's dir:
	if(V3f::dot(barrel_direction_world_space, player_look_direction_in_world_space) > 0.9f)
	{
		//DBGOUT("using camera look");
		dir_ = player_look_direction_in_world_space;
	}
	else
	{
		//DBGOUT("using md5 look");
		dir_ = barrel_direction_world_space;
	}

	return true;
}

void NwPlayerWeapon::AddMuzzleFlashToWorld(
									   ARenderWorld& render_world
									   , const V3f& position
									   , const CoTimestamp& now
									   )
{
	idRenderModel& render_model = *anim_model->render_model;

	// these will be different each fire
	idRandom		rng;
	rng.SetSeed( now.time_msec );

	render_model.shader_parms.f[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( now.time_msec );
	render_model.shader_parms.f[ SHADERPARM_DIVERSITY ]		= rng.RandomFloat(0, 1);

	{
		const NwWeaponMuzzleFlashDef& muzzle_flash_def = def->muzzle_flash;

		TbDynamicPointLight	muzzle_flash_pointlight;
		muzzle_flash_pointlight.position = position;
		muzzle_flash_pointlight.radius = muzzle_flash_def.light_radius;
		muzzle_flash_pointlight.color = muzzle_flash_def.light_color;

		render_world.pushDynamicPointLight(
			muzzle_flash_pointlight
			);
	}
}

void NwPlayerWeapon::DrawCrosshair(
								   ADebugDraw& dbg_draw
								   , const NwCameraView& camera_view
								   )
{
	const V3f point_in_front_of_camera = camera_view.eye_pos + camera_view.look_dir;

	nwTWEAKABLE_CONST(float, CROSSHAIR_LENGTH_SCALE_MIN, 0.01f);
	nwTWEAKABLE_CONST(float, CROSSHAIR_LENGTH_SCALE_MAX, 0.07f);

	const RGBAf crosshair_color = RGBAf::LIGHT_YELLOW_GREEN;

	// horizontal lines
	dbg_draw.DrawLine(
		point_in_front_of_camera + camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera + camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
	dbg_draw.DrawLine(
		point_in_front_of_camera - camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera - camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);


	// vertical lines
	const V3f up_dir = camera_view.getUpDir();
	dbg_draw.DrawLine(
		point_in_front_of_camera + up_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera + up_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
	dbg_draw.DrawLine(
		point_in_front_of_camera - up_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera - up_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
}

void NwPlayerWeapon::Dbg_DrawRayFromMuzzle(
	MyGameRenderer& renderer
	)
{
	V3f		muzzle_joint_pos_world_space, barrel_direction_world_space;

	if(this->getMuzzlePosAndDirInWorldSpace(
		muzzle_joint_pos_world_space
		, barrel_direction_world_space
		))
	{
		//
		renderer._render_system->_debug_renderer.DrawSolidSphere(
			muzzle_joint_pos_world_space
			, 0.001f // the weapon is very close to the near plane and is scaled down to 1e-3 of its size
			, RGBAf::YELLOW
			, 4, 4
			);

		renderer._render_system->_debug_line_renderer.DrawLine(
			muzzle_joint_pos_world_space
			, muzzle_joint_pos_world_space + barrel_direction_world_space * 7.0f
			, RGBAf::WHITE
			, RGBAf::RED
			);
	}
}

void OrthoNormalize(M44f& m)
{
	const V3f r0 = m.v0.asVec3();
	const V3f r1 = m.v1.asVec3();
	const V3f r2 = m.v2.asVec3();

	//
	const V3f r0n = r0.normalized();
	const V3f r2n = V3f::cross( r0n, r1 ).normalized();
	const V3f r1n = V3f::cross( r2n, r0n ).normalized();

	m.v0.set(r0n, 0);
	m.v1.set(r1n, 0);
	m.v2.set(r2n, 0);
}

void NwPlayerWeapon::UpdateOncePerFrame(
	float delta_time_seconds
	, const M44f& camera_world_matrix
	)
{
	const NwWeaponDef& weapon_def = *this->def;

	const M44f camera_world_mat = weapon_def.move_with_camera
		? camera_world_matrix
		: g_MM_Identity
		;

	M44f gun_world_matrix
		= weapon_def.view_offset.toMatrix()
		* camera_world_mat
		;

//LogStream(LL_Warn),"WEAPON WORLDMTX: ",gun_world_matrix;

	M44f final_world_matrix = 
#if 0
		gun_world_matrix
#else
		M44f::createUniformScaling(METERS_TO_DOOM3)
		* gun_world_matrix
#endif
		;

	// Hiding.
	{
		const V3f& local_down_axis = camera_world_mat.v2.asVec3();
		const V3f& local_backward_axis = -camera_world_mat.v1.asVec3();
		const V3f local_hide_dir = local_down_axis * 2.0f + local_backward_axis;

		V3f& translation = final_world_matrix.v3.asVec3();
		translation += V3_Normalized(local_hide_dir) * curr_height_for_hiding;
	}

	//OrthoNormalize(final_world_matrix);

	//
	anim_model->render_model->local_to_world = final_world_matrix;

	//
	if(behavior != nil)
	{
		behavior->UpdateWeapon(*this);
	}
	pressed_buttons = 0;
}

namespace NwPlayerWeapon_
{

ERet Create(
	NwPlayerWeapon *& new_weapon_obj_
	, NwClump & clump
	, const TResPtr<idEntityDef>& entity_def
	, const TResPtr<NwSkinnedMesh>& skinned_mesh
	, const TResPtr<NwWeaponDef>& weapon_def
	, const WeaponType::Enum weapon_type
	)
{
	//
	NwAnimatedModel *	new_anim_model;
	mxDO(NwAnimatedModel_::Create(
		new_anim_model
		, clump
		, skinned_mesh
		, &MyGame::ProcessAnimEventCallback
		//, entity_def2
		));

	//
	NwPlayerWeapon *	new_weapon_obj;
	mxDO(clump.New(new_weapon_obj));

	//
	new_weapon_obj->type = weapon_type;

	//
	new_weapon_obj->anim_model = new_anim_model;

	if(WeaponType::HasBarrel(weapon_type))
	{
		const ozz::animation::Skeleton& skeleton = new_anim_model->_skinned_mesh->ozz_skeleton;
		const ozz::Range<const char* const> joint_names = skeleton.joint_names();

		const int barrel_joint_index = getJointIndexByName(
			"barrel"
			, skeleton
			);
		mxASSERT(barrel_joint_index != INDEX_NONE);
		new_weapon_obj->barrel_joint_index = barrel_joint_index;
	}
	else
	{
		new_weapon_obj->barrel_joint_index = INDEX_NONE;
	}
	
	//
	new_anim_model->barrel_joint_index = new_weapon_obj->barrel_joint_index;

	//
	new_weapon_obj->def = weapon_def;
	mxDO(new_weapon_obj->def.Load( &clump ));

	new_weapon_obj->entity_def = entity_def;
	mxDO(new_weapon_obj->entity_def.Load( &clump ));

	//
	new_weapon_obj_ = new_weapon_obj;

	return ALL_OK;
}


}//namespace







// D:\research\games\HalfLife2\src\game_shared\hl2mp\weapon_rpg.h
// D:\research\games\HalfLife2\src\cl_dll\weapon_selection.h
// D:\research\games\HalfLife2\src\game_shared\hl2mp\weapon_frag.cpp
// D:\research\games\HalfLife2\src\game_shared\sdk\weapon_grenade.cpp
