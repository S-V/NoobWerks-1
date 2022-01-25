//
#include "stdafx.h"
#pragma hdrstop

#include <bx/easing.h>

#include <Core/Util/Tweakable.h>
#include <Engine/Engine.h>	// NwTime
#include <Renderer/Renderer.h>
//
#include "experimental/game_experimental.h"
#include "experimental/rccpp.h"
#include "game_world/game_world.h"
#include "FPSGame.h"
#include "gameplay_constants.h"
#include "game_player.h"


/*
-------------------------------------------------------------------------------
	NgPlayerInventory
-------------------------------------------------------------------------------
*/
NgPlayerInventory::NgPlayerInventory()
{
	has_BFG_briefcase = false;

#if GAME_CFG_RELEASE_BUILD
	//weapons_mask = BIT(WeaponType::Fists);
	weapons_mask = ~0;
#else
	weapons_mask = ~0;
#endif

}

ERet NgPlayerInventory::LoadModels(
								   NwClump& clump
								   )
{
	////
	//mxDO(_LoadWeaponModel(
	//	WeaponType::Fists
	//	
	//	, TResPtr<idEntityDef>(MakeAssetID("weapon_fists"))
	//	, MakeAssetID("weapon_fists")
	//	, MakeAssetID("viewmodel_fists")
	//	//, MakeAssetID("atdm_weapon_shortsword")
	//	//, MakeAssetID("viewmodel_shortsword")

	//	, clump
	//	));

	////
	//mxDO(_LoadWeaponModel(
	//	WeaponType::Shotgun
	//	, MakeAssetID("weapon_shotgun")
	//	, clump
	//	));

	////
	//mxDO(_LoadWeaponModel(
	//	WeaponType::Chaingun
	//	, MakeAssetID("weapon_chaingun")
	//	, clump
	//	));

	//
	mxDO(_LoadWeaponModel(
		WeaponType::Chaingun
		, TResPtr<idEntityDef>(MakeAssetID("weapon_chaingun"))
		, MakeAssetID("weapon_chaingun")
		, MakeAssetID("viewmodel_chaingun")
		, clump
		));

	//
	mxDO(_LoadWeaponModel(
		WeaponType::Plasmagun
		, TResPtr<idEntityDef>(MakeAssetID("weapon_plasmagun"))
		, MakeAssetID("weapon_plasmagun")
		, MakeAssetID("viewmodel_plasmagun")
		, clump
		));

	////
	//mxDO(_LoadWeaponModel(
	//	WeaponType::Railgun
	//	, TResPtr<idEntityDef>(MakeAssetID("weapon_railgun"))
	//	, MakeAssetID("weapon_railgun")
	//	, MakeAssetID("viewmodel_railgun")
	//	, clump
	//	));

	//
	mxDO(_LoadWeaponModel(
		WeaponType::RocketLauncher
		, TResPtr<idEntityDef>(MakeAssetID("weapon_rocketlauncher"))
		, MakeAssetID("weapon_rocketlauncher")
		, MakeAssetID("viewmodel_rocketlauncher")
		, clump
		));

	return ALL_OK;
}

ERet NgPlayerInventory::_LoadWeaponModel(
	const WeaponType::Enum weapon_type
	, const TResPtr<idEntityDef>& entity_def
	, const AssetID& weapon_def_id	// NwWeaponDef
	, const AssetID& weapon_mesh_id	// NwSkinnedMesh
	, NwClump& clump
	)
{
	NwPlayerWeapon *	weapon_model;

	mxDO(NwPlayerWeapon_::Create(
		weapon_model
		, clump
		, entity_def
		, TResPtr<NwSkinnedMesh>(weapon_mesh_id)
		, TResPtr<NwWeaponDef>(weapon_def_id)
		, weapon_type
		));
	weapon_models[ weapon_type ] = weapon_model;

	//
	weapon_model->behavior = g_SystemTable.weapon_behaviors[ weapon_type ];

	// Always play an Idle anim.
	mxDO(weapon_model->anim_model->PlayAnim(
		mxHASH_STR("idle")
		, NwPlayAnimParams().SetPriority(NwAnimPriority::Lowest)
		, PlayAnimFlags::Looped
		));

	// Render as a first-person weapon.
	weapon_model->anim_model->render_model->flags.raw_value |= idRenderModelFlags::NeedsWeaponDepthHack;

	return ALL_OK;
}

void NgPlayerInventory::UnloadModels(
									 NwClump& clump
									 )
{
	//
}





WeaponSway::WeaponSway()
{
	time_accumulator = 0;

	//horiz_sway = 0;
	//vert_sway = 0;
	//linear_momentum = CV3f(0);

	prev_pos = CV3f(0);
	prev_look_dir = V3_FORWARD;
}

/// heavy weapons turn slower
static SecondsF GetWeaponTurnCatchupTimeMultiplier(WeaponType::Enum weapon_type)
{
	switch(weapon_type)
	{
//	case WeaponType::Chaingun:
	case WeaponType::RocketLauncher:
		return 1.2f;

	default:
		return 1.0f;
	}
}

M44f WeaponSway::UpdateOncePerFrame(
	const NwTime& game_time
	, const NwFlyingCamera& camera
	, const MyGamePlayer& player
	, WeaponType::Enum weapon_type
	)
{
	// how slow the weapon is turning
	const SecondsF	weapon_sway_rot_catchup_time_mul = GetWeaponTurnCatchupTimeMultiplier(weapon_type);

	mxBUG("WHY DIFFERENT VALUES?");
#if MX_DEBUG
	nwTWEAKABLE_CONST(SecondsF, WEAPON_SWAY_ROT_CATCHUP_TIME, 0.02f);
#else
	nwTWEAKABLE_CONST(SecondsF, WEAPON_SWAY_ROT_CATCHUP_TIME, 0.003f);
#endif

	const SecondsF	weapon_sway_rot_catchup_time_final = weapon_sway_rot_catchup_time_mul * WEAPON_SWAY_ROT_CATCHUP_TIME;

	//
	const float rot_lerp_factor = (weapon_sway_rot_catchup_time_final > 0)
		? (game_time.real.delta_seconds / weapon_sway_rot_catchup_time_final)
		: 0
		;

	const V3f	curr_look_dir = camera.m_look_direction;
	const V3f	curr_right_dir = camera.m_rightDirection;
	const V3f	curr_up_dir = camera.m_upDirection;

	const V3f	new_look_dir = /*V3_Normalized*/(
		V3_Lerp(prev_look_dir, curr_look_dir, bx::easeInOutCubic(rot_lerp_factor))
		);
	const V3f	new_right_dir = /*V3_Normalized*/(
		V3f::cross(new_look_dir, curr_up_dir)
		);
	const V3f	new_up_dir = curr_up_dir;

	//
	const V3f curr_pos = player.GetEyePosition();
	//
	const V3f	interp_pos = curr_pos
		+ GunAcceleratingOffset( game_time.real.total_msec )
		;
	//
	prev_pos = interp_pos;
	prev_look_dir = new_look_dir;

	return M44f::CreateWorldMatrix(
			new_right_dir,
			new_look_dir,
			new_up_dir,
			interp_pos
		);
}

mxSTOLEN("Based on Doom3's code, idPlayer::GunAcceleratingOffset() in Player.cpp");
/*
==============
idPlayer::GunAcceleratingOffset

generate a positional offset for the gun based on the movement
history in loggedAccelerations
==============
*/
V3f WeaponSway::GunAcceleratingOffset(const MillisecondsU32 curr_time)
{
	CV3f	ofs(0);

	static MillisecondsU32 weaponOffsetTime = 500;
	HOT_VAR((int&)weaponOffsetTime);

	static float WEAPON_SWAY_OFFSET_SCALE = 0.003f;	// the weapon is scaled to like 1/1000th of its size
	HOT_VAR(WEAPON_SWAY_OFFSET_SCALE);

	//
	int stop = accel.num - LoggedAccelerations::NUM;
	if ( stop < 0 ) {
		stop = 0;
	}
	for ( int i = accel.num-1 ; i > stop ; i-- )
	{
		LoggedAccelerations::LoggedAccel& acc = accel.items[i&(LoggedAccelerations::NUM-1)];

		float	f;
		float	t = curr_time - acc.time_when_added;
		if ( t >= weaponOffsetTime ) {
			break;	// remainder are too old to care about
		}

		f = t / weaponOffsetTime;
		f = ( cos( f * 2.0f * mxPI ) - 1.0f ) * 0.5f;
		ofs += acc.dir * (f * WEAPON_SWAY_OFFSET_SCALE);
	}

	return ofs;
}

/*
-------------------------------------------------------------------------------
	MyGamePlayer
-------------------------------------------------------------------------------
*/
mxDEFINE_CLASS(MyGamePlayer);
mxBEGIN_REFLECTION(MyGamePlayer)
	mxMEMBER_FIELD( camera ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;
MyGamePlayer::MyGamePlayer()
{
	comp_health._health = Gameplay::PLAYER_HEALTH;

	//position_in_world_space = V3d::zero();
	num_seconds_shift_is_held = 0;


#if GAME_CFG_WITH_PHYSICS
	phys_movement_flags.raw_value = 0;
	was_in_air = false;
	time_in_air = 0;
	_ResetFootCycle();
#endif

	time_when_received_damage = 0;
	time_when_map_was_loaded = 0;
}

ERet MyGamePlayer::Init(
						 NwClump& storage
						 , MyGameWorld& world
						 , const MyGameSettings& game_settings
						 )
{
	mxDO(task_scheduler.Init());

#if GAME_CFG_WITH_PHYSICS

	DynamicCharacterController::Settings	char_ctrl_settings;
	char_ctrl_settings.mass = 100;
	char_ctrl_settings.radius = 0.5f;
	char_ctrl_settings.height = 1.86f;

	phys_char_controller.create(
		nil
		, &world.physics_world
		, char_ctrl_settings
		, nil
	);

	btTransform	t;
	t.setIdentity();
	t.setOrigin(toBulletVec(V3f::fromXYZ(camera._eye_position)));
	phys_char_controller.getRigidBody()->setCenterOfMassTransform(t);

	//
	phys_movement_flags.raw_value = 0;

	//
	const bool no_phys_cheats = !game_settings.cheats.enable_noclip;
	phys_char_controller.setCollisionsEnabled( no_phys_cheats );
	phys_char_controller.setGravityEnabled( no_phys_cheats );

#endif // GAME_CFG_WITH_PHYSICS


	mxDO(inventory.LoadModels(storage));

	//
	nwFOR_LOOP(UINT, i, WeaponType::Count)
	{
		inventory.weapon_models[i]->dbg_SetFullyLoaded();
	}

	//
	_current_weapon = inventory.weapon_models[
		//WeaponType::Default
		WeaponType::Chaingun
		//WeaponType::Plasmagun
		//WeaponType::RocketLauncher
	];

	return ALL_OK;
}

void MyGamePlayer::unload(NwClump& storage_clump)
{
//	storage_clump.Destroy(_weapon_plasmagun._ptr);

	inventory.UnloadModels(storage_clump);

#if GAME_CFG_WITH_PHYSICS

	phys_char_controller.destroy();

#endif // GAME_CFG_WITH_PHYSICS

	task_scheduler.Shutdown();
}

void MyGamePlayer::UpdateOncePerFrame(
	const NwTime& game_time
	, const GameCheatsSettings& game_cheat_settings
	)
{
	task_scheduler.UpdateOncePerFrame(game_time);

	//
	camera.Update( game_time.real.delta_seconds );

	if(FPSGame::Get().settings.cheats.enable_noclip)
	{
		camera.UpdateVelocityAndPositionFromInputs( game_time.real.delta_seconds );
	}

	if(!game_cheat_settings.enable_noclip)
	{
		// don't let the player look up or down more than 90 degrees
		camera.m_pitchAngle = clampf(
			camera.m_pitchAngle,
			DEG2RAD(-85),
			DEG2RAD(+88)
			);
	}

	//
	if(_current_weapon != nil)
	{
		const M44f weapon_model_local_to_world_mat = weapon_sway.UpdateOncePerFrame(
			game_time
			, camera
			, *this
			, _current_weapon->type
			);
		
		_current_weapon->UpdateOncePerFrame(
			game_time.real.delta_seconds
			, weapon_model_local_to_world_mat//camera_view.inverse_view_matrix
			);
	}
}

V3f MyGamePlayer::GetEyePosition() const
{
	// height from the center of the rigid body
	nwTWEAKABLE_CONST(float, PLAYER_EYE_HEIGHT, 1.3f);

	V3f center_pos = V3f::fromXYZ(camera._eye_position);
	return center_pos + camera.m_upDirection * PLAYER_EYE_HEIGHT;
}

void MyGamePlayer::SetCameraPosition(
	const V3d new_position_in_world_space
	)
{
	camera._eye_position = new_position_in_world_space;
	phys_char_controller.setPos(V3f::fromXYZ(new_position_in_world_space));
}

const V3f MyGamePlayer::GetPhysicsBodyCenter() const
{
	return phys_char_controller.getPos();
}

void MyGamePlayer::RenderFirstPersonView(
	const NwCameraView& camera_view
	, const RrRuntimeSettings& renderer_settings
	, GL::NwRenderContext & render_context
	, const NwTime& args
	, ARenderWorld* render_world
	) const
{
	const MyGameSettings& game_settings = FPSGame::Get().settings;

	const bool enable_player_collisions = !game_settings.cheats.enable_noclip;
	const bool draw_player_weapon = enable_player_collisions;

	if( enable_player_collisions && _current_weapon != nil )
	{
		//
		idRenderModel& render_model = *_current_weapon->anim_model->render_model;

		idRenderModel_::RenderInstance(
			render_model
			, render_model.local_to_world
			, camera_view
			, renderer_settings
			, render_context
			, _current_weapon->def->FoV_Y_in_degrees
			);
	}

	//
	_RenderFullScreenDamageIndicatorIfNeeded(
		render_context
		, args
		);

	//
	_FadeInUponLevelLoadingIfNeeded(
		render_context
		, args
		);
}

ERet MyGamePlayer::_RenderFullScreenDamageIndicatorIfNeeded(
	GL::NwRenderContext & render_context
	, const NwTime& args
	) const
{
	//
	nwTWEAKABLE_CONST(int, PLAYER_DAMAGE_INDICATOR_DURATION_MSEC, 700);

	//
	const MillisecondsU32 curr_time_msec = mxGetTimeInMilliseconds();
	const MillisecondsU32 time_elapsed_since_received_damage_msec = curr_time_msec - time_when_received_damage;

	if(time_elapsed_since_received_damage_msec <= PLAYER_DAMAGE_INDICATOR_DURATION_MSEC)
	{
		const float01_t alpha
			= clampf(
			1.0f - float(time_elapsed_since_received_damage_msec) / float(PLAYER_DAMAGE_INDICATOR_DURATION_MSEC)
			, 0
			, 0.8f // don't draw it fully opaque
			)
			;

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(
			technique
			, MakeAssetID("player_indicate_damage")
			, &FPSGame::Get().runtime_clump
			));

		//
		V4f	red_color = {1,0,0, alpha};
		technique->setUniform(
			mxHASH_STR("u_color")
			, &red_color
			);

		//
		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
		const U32 draw_order = TbRenderSystemI::Get()
			.getRenderPath()
			.getPassDrawingOrder( ScenePasses::DebugTextures )
			;

		//
		GL::NwPushCommandsOnExitingScope	submitCommands(
			render_context,
			GL::buildSortKey(
			draw_order,
			pass0.default_program_handle,
			GL::SortKey(0)
			)
			);
		mxDO(technique->pushAllCommandsInto( render_context._command_buffer ));

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, pass0.default_program_handle
			));
	}

	return ALL_OK;
}

ERet MyGamePlayer::_FadeInUponLevelLoadingIfNeeded(
	GL::NwRenderContext & render_context
	, const NwTime& args
	) const
{
	//
	nwNON_TWEAKABLE_CONST(int, LEVEL_LOADED_FADE_IN_DURATION_MSEC, 2000);

	//
	const MillisecondsU32 curr_time_msec = mxGetTimeInMilliseconds();
	const MillisecondsU32 time_elapsed_since_level_was_loaded = curr_time_msec - time_when_map_was_loaded;

	if(time_elapsed_since_level_was_loaded <= LEVEL_LOADED_FADE_IN_DURATION_MSEC)
	{
		const float01_t alpha
			= clampf(
			1.0f - float(time_elapsed_since_level_was_loaded) / float(LEVEL_LOADED_FADE_IN_DURATION_MSEC)
			, 0
			, 1.0f
			)
			;

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(
			technique
			, MakeAssetID("player_indicate_damage")
			, &FPSGame::Get().runtime_clump
			));

		//
		V4f	black_color = {0,0,0, alpha};
		technique->setUniform(
			mxHASH_STR("u_color")
			, &black_color
			);

		//
		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
		const U32 draw_order = TbRenderSystemI::Get()
			.getRenderPath()
			.getPassDrawingOrder( ScenePasses::DebugTextures )
			;

		//
		GL::NwPushCommandsOnExitingScope	submitCommands(
			render_context,
			GL::buildSortKey(
			draw_order,
			pass0.default_program_handle,
			GL::SortKey(0)
			)
			);
		mxDO(technique->pushAllCommandsInto( render_context._command_buffer ));

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, pass0.default_program_handle
			));
	}

	return ALL_OK;
}

void MyGamePlayer::Dbg_RenderFirstPersonView(
	const NwCameraView& camera_view
	, MyGameRenderer& renderer
	) const
{
	_current_weapon->Dbg_DrawRayFromMuzzle(renderer);
}

float MyGamePlayer::calc_debug_camera_speed() const
{
	UNDONE;
	return 0;
	//num_seconds_shift_is_held
}

ERet MyGamePlayer::SelectWeapon( WeaponType::Enum type_of_weapon_to_select )
{
	//
	if(task_scheduler.IsBusy()) {
		return ERR_SYSTEM_IS_BUSY;
	}

	DBGOUT("Selecting weapon of type '%d'...", type_of_weapon_to_select);

	//
	if( _current_weapon != nil )
	{
		DBGOUT("[Currently selected weapon: '%d'.", _current_weapon->type);

		if( _current_weapon->type == type_of_weapon_to_select )
		{
			DBGOUT("Weapon of type '%d' is already selected.", type_of_weapon_to_select);
			return ALL_OK;
		}
	}

	//
	const bool has_weapon_of_this_type = inventory.weapons_mask & BIT(type_of_weapon_to_select);
	if( !has_weapon_of_this_type )
	{
		DBGOUT("Couldn't select weapon of type '%d'! (not in inventory)", type_of_weapon_to_select);
		return ERR_OBJECT_NOT_FOUND;
	}

	//
	if(task_scheduler.IsBusy()) {
		return ERR_SYSTEM_IS_BUSY;
	}

	//
	NwPlayerWeapon* weapon_to_select = inventory.weapon_models[ type_of_weapon_to_select ];


	//
	const CoTimestamp now = CoTimestamp::GetRealTimeNow();


	mxTODO("TaskPtr - ref-counted");
	CoTaskI* task_lower_curr_weapon = _current_weapon->CreateTask_LowerAnimated(task_scheduler, now);
	mxENSURE(task_lower_curr_weapon, ERR_OUT_OF_MEMORY, "");

	CoTaskI* task_raise_next_weapon = weapon_to_select->CreateTask_RaiseAnimated(task_scheduler, now);
	mxENSURE(task_raise_next_weapon, ERR_OUT_OF_MEMORY, "");

	//
	struct WeaponTask_SwitchWeapon: CoTaskI
	{
		TPtr< MyGamePlayer >	player;
		TPtr< NwPlayerWeapon >	weapon_to_select;
	public:
		virtual CoTaskStatus Run( void* user_data, const CoTimestamp& now ) override
		{
			$CoBEGIN;
			player->_current_weapon = weapon_to_select;
			$CoEND;
		}
	};

	WeaponTask_SwitchWeapon* switch_weapon_task = task_scheduler.task_allocator.new_<WeaponTask_SwitchWeapon>();
	mxENSURE(switch_weapon_task, ERR_OUT_OF_MEMORY, "");
	switch_weapon_task->player = this;
	switch_weapon_task->weapon_to_select = weapon_to_select;

	//
	task_scheduler.QueueTask(task_lower_curr_weapon);
	task_scheduler.QueueTask(switch_weapon_task);
	task_scheduler.QueueTask(task_raise_next_weapon);

	return ALL_OK;
}

void MyGamePlayer::SelectPreviousWeapon()
{
	if(NwPlayerWeapon* active_weapon = _current_weapon._ptr)
	{
		this->SelectWeapon(
			WeaponType::IntToEnumSafe( active_weapon->type + 1 )
			);
	}
	else
	{
		this->SelectWeapon(WeaponType::Default);
	}
}

void MyGamePlayer::SelectNextWeapon()
{
	if(NwPlayerWeapon* active_weapon = _current_weapon._ptr)
	{
		this->SelectWeapon(
			WeaponType::IntToEnumSafe( active_weapon->type - 1 )
			);
	}
	else
	{
		this->SelectWeapon(WeaponType::Default);
	}
}

void MyGamePlayer::ReloadWeapon()
{
	DBGOUT("Reloading weapon...");
	if(NwPlayerWeapon* active_weapon = _current_weapon._ptr)
	{
//		active_weapon->pressed_buttons |= WEAPON_BUTTON_RELOAD;
	}
}

void MyGamePlayer::PlaySound3D(
							   const AssetID& sound_id
							   )
{
	FPSGame& game = FPSGame::Get();

#if GAME_CFG_WITH_SOUND	
	
	game.sound.PlaySound3D(
		sound_id
		, this->GetEyePosition()
		);

#endif // #if GAME_CFG_WITH_SOUND
}

void MyGamePlayer::DealDamage(
							  int damage
							  )
{
	DBGOUT("[Player] DealDamage: %d", damage);

	const bool was_alive = comp_health.IsAlive();

	if(was_alive)
	{
		time_when_received_damage = mxGetTimeInMilliseconds();

		if(damage > 50)
		{
			this->PlaySound3D(
				MakeAssetID("player_sounds_pain_huge")
				);
		}
		else if(damage > 20)
		{
			this->PlaySound3D(
				MakeAssetID("player_sounds_pain_big")
				);
		}
		else if(damage > 10)
		{
			this->PlaySound3D(
				MakeAssetID("player_sounds_pain_medium")
				);
		}
		else
		{
			this->PlaySound3D(
				MakeAssetID("player_sounds_pain_small")
				);
		}
	}

	//
	comp_health.DealDamage(damage);

	const bool is_now_dead = comp_health.IsDead();
	if(was_alive && is_now_dead)
	{
		this->PlaySound3D(
			MakeAssetID("player_sounds_death")
			);
	}
}

bool MyGamePlayer::HasFullHealth() const
{
	return comp_health._health >= Gameplay::PLAYER_HEALTH;
}

bool MyGamePlayer::_PickUp_Ammo_for_Weapon(
	WeaponType::Enum weapon_type
	, int num_ammo_in_item
	)
{
	NwPlayerWeapon* weapon = inventory.weapon_models[ weapon_type ];
	const NwWeaponDef& weapon_def = *weapon->def;

	if(weapon->recharge_ammo >= weapon_def.max_recharge_ammo) {
		return false;
	}

	//
	this->PlaySound3D(
		MakeAssetID("player_pickup_weapon")
		);

	weapon->recharge_ammo += num_ammo_in_item;

	weapon->recharge_ammo = smallest(
		weapon->recharge_ammo,
		weapon_def.max_recharge_ammo
		);
	return true;
}

bool MyGamePlayer::PickUp_Item_RocketAmmo()
{
	return _PickUp_Ammo_for_Weapon(
		WeaponType::RocketLauncher
		, Gameplay::ROCKETS_IN_AMMOPACK
		);
}

bool MyGamePlayer::PickUp_Item_BFG_Case()
{
	if( !inventory.has_BFG_briefcase )
	{
		this->PlaySound3D(
			MakeAssetID("player_pickup_BFG_case")
			);

		inventory.has_BFG_briefcase = true;
		return true;
	}

	return false;
}

bool MyGamePlayer::PickUp_Weapon_Plasmagun()
{
	mxTODO("pick up weapon!");
	return _PickUp_Ammo_for_Weapon(
		WeaponType::Plasmagun
		, Gameplay::PLASMACELLS_IN_AMMOPACK
		);
}

void MyGamePlayer::ReplenishHealth()
{
	comp_health._health = Gameplay::PLAYER_HEALTH;
}

void MyGamePlayer::ReplenishAmmo()
{
	//
	for(int i = 0; i < mxCOUNT_OF(inventory.weapon_models); i++)
	{
		inventory.weapon_models[i]->ResetAmmoOnRespawn();
	}
}

void MyGamePlayer::SetGravityEnabled(bool affected_by_gravity)
{
	phys_char_controller.setGravityEnabled(affected_by_gravity);
}

void MyGamePlayer::RespawnAtPos(
								const V3f& spawn_pos
								)
{
	this->SetCameraPosition(
		V3d::fromXYZ(spawn_pos)
		);

	//
	this->ReplenishHealth();

	//
	this->ReplenishAmmo();
}

#if GAME_CFG_WITH_PHYSICS

void MyGamePlayer::_ResetFootCycle()
{
	left_foot = false;
	foot_cycle = 0;
}

#endif // GAME_CFG_WITH_PHYSICS
