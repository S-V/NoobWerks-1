//
#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/BoundingVolumes/VectorAABB.h>
#include <Base/Math/Utils.h>	// CalcDamping

#include <Core/Util/Tweakable.h>

#include <Renderer/Core/Mesh.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Private/ShaderInterop.h>

#include "gameplay/gameplay_mgr.h"
#include "game_app.h"



//tbPRINT_SIZE_OF(SgShip);


namespace
{
	enum
	{
		//MAX_FLYING_BULLETS = 2048,

		/// can use very cheap, sloppy physics
		MAX_PLASMA_BOLTS_FROM_NPCs = (1<<14),

		/// need accurate collision detection
		MAX_PLASMA_BOLTS_FROM_PLAYER = 64,
		//MAX_FLYING_ROCKETS = 64,
	};


	// cannot fire faster than this
	const int SHIP_CANNON_COOLDOWN_MSEC = 90;

	nwNON_TWEAKABLE_CONST(float, PLASMA_BOLT_VELOCITY, 90.0f);

	//
	static const int ENEMY_WAVE_TIMES_MSEC[] = {

#if GAME_CFG_RELEASE_BUILD
    0,
		2*60*1000,
    5*60*1000,
    20*60*1000,
#else
		0,
		10*1000,
		20*1000,
		//60*1000,
#endif
	};

	//
	struct SgShipAI
	{
		I32		life;	// dead if <= 0

		// (yeah, it can overflow, but whatever)
		U32		timestamp_msec_when_can_fire;

		SgShipHandle	ship_handle;

		// so that my ships rest at their initial positions
		U32		has_ever_seen_enemies;
	};
	ASSERT_SIZEOF(SgShipAI, 16);


	mxFORCEINLINE void ClampLength( V3f& v, float length )
	{
		mxOPTIMIZE("len")
        if (v.length() > length) {
            v = v.normalized() * length;
        }
	}

}//namespace

struct SgGameplayMgrData: NwNonCopyable
{
	NwHandleTable	ship_handle_mgr;
	SgShip *		ships;
	SgShipAI *		ships_ai;

	bool			has_ships_to_destroy;

	U32			num_remaining_enemy_ships;

	//
	//bool	is_mission_completed;

	//
	SgShipHandle	h_my_ship_to_defend;
	SgShipHandle	h_enemy_ship_to_attack;

	//
	MillisecondsU32		last_time_when_we_should_have_played_fight_music;
	float				last_weight_for_fight_music;

	//
	MillisecondsU32 time_when_current_session_started;
	int				current_enemy_wave;

	//
	SgGameplayConfig	cfg;

	//
	SgShipDef		ship_defs[obj_type_count];
	float			ship_ai_collision_radii[obj_type_count];
	float			ship_ai_squared_attack_range[obj_type_count];
	ColShapeHandle	ship_col_shapes[obj_type_count];

#if SG_USE_SDF_VOLUMES
	NwTexture::Ref	ship_sdf_textures[obj_type_count];
#endif

	//
	TFixedArray< SgProjectile, MAX_PLASMA_BOLTS_FROM_NPCs >		npc_projectiles;
	TFixedArray< SgProjectile, MAX_PLASMA_BOLTS_FROM_PLAYER >	player_projectiles;

	//
	AllocatorI& allocator;

public:
	SgGameplayMgrData(AllocatorI& allocator)
		: ship_handle_mgr(allocator)
		, ships(nil)
		, ships_ai(nil)
		, has_ships_to_destroy(false)
		, time_when_current_session_started(0)
		, current_enemy_wave(0)
		, last_time_when_we_should_have_played_fight_music(0)
		, last_weight_for_fight_music(0)
		, allocator(allocator)
	{
	}
};

namespace
{
	mxFORCEINLINE const TSpan< SgShip > GetSpaceships(SgGameplayMgrData* mydata)
	{
		return TSpan< SgShip >(
			mydata->ships,
			mydata->ship_handle_mgr._num_alive_items
		);
	}

	mxFORCEINLINE const TSpan< SgShipAI > GetSpaceshipAIs(SgGameplayMgrData* mydata)
	{
		return TSpan< SgShipAI >(
			mydata->ships_ai,
			mydata->ship_handle_mgr._num_alive_items
		);
	}
}


SgGameplayMgr::SgGameplayMgr()
{
}


ERet SgGameplayMgr::Initialize(
	const SgGameplayConfig& gameplay_config
	, const SgAppSettings& game_settings
	, SgPhysicsMgr& physics_mgr
	, NwClump& storage_clump
	, AllocatorI& allocator
	)
{
	nwCREATE_OBJECT(_data._ptr, allocator, SgGameplayMgrData, allocator);
	_data->cfg = gameplay_config;

	mxDO(nwAllocArray(
		_data->ships
		, gameplay_config.max_spaceships
		, allocator
		, EFFICIENT_ALIGNMENT
		));

	mxDO(_data->ship_handle_mgr.InitWithMaxCount(gameplay_config.max_spaceships));

	mxDO(nwAllocArray(
		_data->ships_ai
		, gameplay_config.max_spaceships
		, allocator
		, EFFICIENT_ALIGNMENT
		));

	//mxDO(_data->npc_projectiles.reserveExactly(gameplay_config.max_projectiles));

	{
		{
			SgShipDef& ship_def = _data->ship_defs[obj_freighter];
			mxDO(ship_def.Load(
				"freighter"
				, storage_clump
				));
			ship_def.angular_turn_acceleration = 0.1f;
			ship_def.health = 10000;

			ship_def.cannon_firerate = 20;
			ship_def.cannon_horiz_spread = 0.1f;
			ship_def.cannon_vert_spread = 0.1f;
		}

		//
		{
			SgShipDef& ship_def = _data->ship_defs[obj_heavy_battleship_ship];

			mxDO(ship_def.Load(
				"heavy_battleship"
				, storage_clump
				));
			ship_def.angular_turn_acceleration = 0.2f;
			ship_def.health = 5000;

			ship_def.cannon_firerate = 20;
			ship_def.cannon_horiz_spread = 0.1f;
			ship_def.cannon_vert_spread = 0.1f;
		}

		//
		{
			SgShipDef& ship_def = _data->ship_defs[obj_small_fighter_ship];

			mxDO(ship_def.Load(
				"small_fighter"
				, storage_clump
				));
			ship_def.health = 100;
		}

		//
		{
			SgShipDef& ship_def = _data->ship_defs[obj_combat_fighter_ship];

			mxDO(ship_def.Load(
				"nave_orion"
				, storage_clump
				));
			ship_def.health = 200;

			ship_def.cannon_firerate = 90;
			ship_def.cannon_horiz_spread = 0.4f;
			ship_def.cannon_vert_spread = 0.4f;
		}

		//
		//mxDO(_data->ship_defs[obj_capital_ship].Load(
		//	"heavy_battleship2"
		//	, storage_clump
		//	));
		//_data->ship_defs[obj_capital_ship].angular_turn_acceleration = 0.2f;

		////
		//mxDO(_data->ship_defs[obj_spacestation].Load(
		//	"needle"
		//	, storage_clump
		//	));
		//_data->ship_defs[obj_spacestation].angular_turn_acceleration = 0.1f;
	
	
		{
			SgShipDef& ship_def = _data->ship_defs[obj_alien_fighter_ship];

			mxDO(ship_def.Load(
				"spaceship-alien-fighter"
				, storage_clump
				));
			ship_def.health = 90;
		}

		//
		{
			SgShipDef& ship_def = _data->ship_defs[obj_alien_skorpion_ship];

			mxDO(ship_def.Load(
				"alien_skorpion_ship"
				, storage_clump
				));
			ship_def.health = 70;
		}

		//mxDO(_data->ship_defs[obj_alien_spiky_spaceship].Load(
		//	"alien_spiky_spaceship"
		//	, storage_clump
		//	));
	}

	//
	for(int i = 0; i < obj_type_count; i++)
	{
		const SgShipDef& ship_def = _data->ship_defs[i];

		const AABBf& local_aabb = ship_def.lod_meshes[0]->local_aabb;

		const float local_aabb_max_radius = local_aabb.halfSize().maxComponent();

		//
		_data->ship_ai_collision_radii[i] = local_aabb_max_radius;

		_data->ship_ai_squared_attack_range[i] = local_aabb_max_radius * 256.0f;

		//
		mxDO(physics_mgr.CreateColShape_Box(
			_data->ship_col_shapes[i],
			_data->ship_defs[i].lod_meshes[0]->local_aabb
			));
	}

	//
#if SG_USE_SDF_VOLUMES
	for(int i = 0; i < obj_type_count; i++)
	{
		mxDO(Resources::Load(
			_data->ship_sdf_textures[i]._ptr
			, _data->ship_defs[i].mesh._id
			));
	}
#endif

	//
	h_spaceship_under_crosshair.setNil();

	return ALL_OK;
}

void SgGameplayMgr::Shutdown()
{
	_data->ship_handle_mgr.Shutdown();

	mxDELETE_AND_NIL(_data->ships_ai, _data->allocator);
	mxDELETE_AND_NIL(_data->ships, _data->allocator);

	mxDELETE_AND_NIL(_data._ptr, _data->allocator);
}

ERet SgGameplayMgr::StartNewGame(
								 SgWorld& world
								 , SgPhysicsMgr& physics_mgr
								 )
{
	_ClearGameState(
		world
		, physics_mgr
		);

	_data->time_when_current_session_started = mxGetTimeInMilliseconds();
	_data->current_enemy_wave = 0;

	//
	mxDO(_SetupPlayerBase(
		world
		, physics_mgr
		));
	mxDO(_SetupEnemyBase(
		world
		, physics_mgr
		));

	//
	player.camera.setPosAndLookDir(
		CV3d(-70, -71, 139)
		, CV3f(0.1737f, 0.8895f, -0.4225f).normalized()
		);

	return ALL_OK;
}



ERet SgGameplayMgr::_StartEnemyWave(
	const int enemy_wave
	, const NwTime& game_time
	, SgWorld& world
	, SgPhysicsMgr& physics_mgr
	, NwSoundSystemI& snd_sys
	, const SgUserSettings& user_settings
	)
{
	// CRAP CRAP CRAP
	if( enemy_wave >= mxCOUNT_OF(ENEMY_WAVE_TIMES_MSEC) ) {
		// will be counted as mission success
		return ALL_OK;
	}

	DBGOUT("STARTING WAVE %d...", enemy_wave);
	_data->current_enemy_wave = enemy_wave;

	switch(enemy_wave)
	{
	case 0:
		{
#if GAME_CFG_RELEASE_BUILD
			int num_ships_x = 2;
			int num_ships_y = 2;
#else
			int num_ships_x = 1;
			int num_ships_y = 1;
#endif

			CV3f	pos_offset(-1000, 0, 0);
			float	spacing = 8;

			//
			for(int i=0; i<num_ships_x; i++)
			{
				for(int j=0; j<num_ships_y; j++)
				{
					SgShipHandle	ship_handle;
					mxDO(CreateSpaceship(
						ship_handle
						, obj_alien_skorpion_ship
						, world
						, physics_mgr
						, CV3f(i,j,0) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					FindSpaceship(ship_handle)->longterm_goal_handle = _data->h_my_ship_to_defend;
				}
			}
		}
		break;

	case 1:
		{
#if GAME_CFG_RELEASE_BUILD
			int num_ships_x = 5;
			int num_ships_y = 3;
#else
			int num_ships_x = 2;
			int num_ships_y = 2;
#endif
			CV3f	pos_offset(1000, 0, 0);
			float	spacing = 8;

			//
			for(int i=0; i<num_ships_x; i++)
			{
				for(int j=0; j<num_ships_y; j++)
				{
					SgShipHandle	ship_handle;
					mxDO(CreateSpaceship(
						ship_handle
						, obj_small_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,0) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					FindSpaceship(ship_handle)->longterm_goal_handle = _data->h_my_ship_to_defend;
				}
			}
		}
		break;

	case 2:
		{
#if GAME_CFG_RELEASE_BUILD
			int num_ships_x = 10;
			int num_ships_y = 10;
#else
			int num_ships_x = 2;
			int num_ships_y = 2;
#endif
			CV3f	pos_offset(0, 0, -1000);
			float	spacing = 20;

			//
			for(int i=0; i<num_ships_x; i++)
			{
				for(int j=0; j<num_ships_y; j++)
				{
					SgShipHandle	ship_handle;
					mxDO(CreateSpaceship(
						ship_handle
						, obj_small_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,0) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					FindSpaceship(ship_handle)->longterm_goal_handle = _data->h_my_ship_to_defend;
				}
			}
		}
		break;

	default:
		{
#if GAME_CFG_RELEASE_BUILD
			int num_ships_x = 10;
			int num_ships_y = 10;
			int num_ships_z = 10;
#else
			int num_ships_x = 3;
			int num_ships_y = 3;
			int num_ships_z = 3;
#endif
			DEVOUT("Creating %d ships...", num_ships_x*num_ships_y*num_ships_z);

			CV3f	pos_offset(7, 2, 3);
			float	spacing = 8;

			//
			for(int i=0; i<num_ships_x; i++)
			{
				for(int j=0; j<num_ships_y; j++)
				{
					for(int k=0; k<num_ships_z; k++)
					{
						SgShipHandle	ship_handle;
						mxDO(CreateSpaceship(
							ship_handle
							, obj_alien_skorpion_ship
							, world
							, physics_mgr
							, CV3f(i,j,k) * spacing + pos_offset
							, fl_ship_is_my_enemy
							));
						FindSpaceship(ship_handle)->longterm_goal_handle = _data->h_my_ship_to_defend;
					}
				}
			}
		}
		break;
	}

	return ALL_OK;
}

void SgGameplayMgr::UpdateOncePerFrame(
	const NwTime& game_time
	, const SgUserSettings& user_settings
	, const Vector4& eye_position
	, SgWorld& world
	, SgPhysicsMgr& physics_mgr
	, NwSoundSystemI& snd_sys
	)
{
	SgGameApp& game_app = SgGameApp::Get();

	//
	if(!game_app.is_paused)
	{
		_TickAI(
			game_time
			, world
			, eye_position
			, physics_mgr
			, snd_sys
			);
	}

	//
	if(!spaceship_controller.IsControllingSpaceship()) {
		_FindShipToControl();
	}

	//
	_ChooseWhichMusicToPlayInBackground(
		game_time
		, snd_sys
		, user_settings
		);

#if GAME_CFG_RELEASE_BUILD
	_StartNextEnemyWaveIfNeeded(
		game_time
		, world
		, physics_mgr
		, snd_sys
		, user_settings
		);
#endif

}

ERet SgGameplayMgr::HandleSpaceshipUnderCrosshair()
{
	if(h_spaceship_under_crosshair.isNil()) {
		return ERR_INVALID_FUNCTION_CALL;
	}

	const SgShip* spaceship_under_crosshair = FindSpaceship(h_spaceship_under_crosshair);
	if(!spaceship_under_crosshair) {
		h_spaceship_under_crosshair.setNil();
		return ERR_OBJECT_NOT_FOUND;
	}

	const bool is_enemy = (spaceship_under_crosshair->flags & fl_ship_is_my_enemy);
	if(is_enemy)
	{
		AttackEnemySpaceship(h_spaceship_under_crosshair);
	}
	else
	{
		TakeControlOfSpaceship(h_spaceship_under_crosshair);
	}

	return ALL_OK;
}

const SgGameplayDisplayedStats SgGameplayMgr::GetDisplayedStats() const
{
	const SgShip* mothership = FindSpaceship(_data->h_my_ship_to_defend);

	//
	SgGameplayDisplayedStats	stats;
	stats.num_friendly_ships = _data->ship_handle_mgr._num_alive_items - _data->num_remaining_enemy_ships;

	if(mothership)
	{
		stats.mothership_health = _data->ships_ai[ mothership->ship_ai_index ].life;
	}
	else
	{
		stats.mothership_health = 0;
	}

	stats.num_enemy_ships = _data->num_remaining_enemy_ships;
	return stats;
}

bool SgGameplayMgr::IsGameMissionCompleted() const
{
	return (_data->num_remaining_enemy_ships == 0)
		&& (FindSpaceship(_data->h_my_ship_to_defend) != 0)
		;
}

bool SgGameplayMgr::IsGameOverMissionFailed() const
{
	return !_data->ship_handle_mgr.IsValidHandle(
		_data->h_my_ship_to_defend.id
		);
}

void SgGameplayMgr::_ClearGameState(
									SgWorld& world
									, SgPhysicsMgr& physics_mgr
									)
{
	_data->ship_handle_mgr.ResetState();
	
	_data->has_ships_to_destroy = false;
	_data->num_remaining_enemy_ships = 0;

	_data->h_my_ship_to_defend.setNil();
	_data->h_enemy_ship_to_attack.setNil();


	_data->last_time_when_we_should_have_played_fight_music = 0;
	_data->last_weight_for_fight_music = 0;
	_data->npc_projectiles.setEmpty();
	_data->player_projectiles.setEmpty();

	physics_mgr.RemoveAllRigidBodies();
}

void SgGameplayMgr::_FindShipToControl()
{
	SgGameApp& game_app = SgGameApp::Get();
	SgRenderer& game_renderer = game_app.renderer;

	//
	const NwCameraView& camera_view = game_renderer.scene_view;

	//
	CulledObjectID	hit_obj;
	TempNodeID		hit_node_id;

	const bool ray_hit_anything = game_app.physics_mgr._aabb_tree->CastRay(
		camera_view.eye_pos
		, camera_view.look_dir
		, hit_obj
		, &hit_node_id
		);

	if(ray_hit_anything)
	{
		const UINT hit_obj_type = (hit_obj >> CULLED_OBJ_TYPE_SHIFT);
		const UINT hit_obj_idx = (hit_obj & CULLED_OBJ_INDEX_MASK);

		h_spaceship_under_crosshair = _data->ships[ hit_obj_idx ].my_handle;

		mxASSERT(_data->ship_handle_mgr.IsValidHandle(h_spaceship_under_crosshair.id));
	}
	else
	{
		h_spaceship_under_crosshair.setNil();
	}
}

SgShipHandle SgGameplayMgr::GetShipToDefend() const
{
	return _data->h_my_ship_to_defend;
}

void SgGameplayMgr::SetShipToDefend(const SgShipHandle h_ship_to_defend) const
{
	_data->h_my_ship_to_defend = h_ship_to_defend;
}

#if GAME_CFG_WITH_SOUND

ERet SgGameplayMgr::PlayAmbientMusic(
	NwSoundSystemI& snd_sys
	, const SgUserSettings& user_settings
	, const float01_t rel_volume
	)
{
	snd_sys.PlayAudioClip(
		MakeAssetID("horrorambience3")
		, PlayAudioClip_Looped
		, rel_volume * user_settings.ingame.sound.music_volume
		, SND_CHANNEL_MUSIC
		);
	return ALL_OK;
}

ERet SgGameplayMgr::PlayFightMusic(
	NwSoundSystemI& snd_sys
	, const SgUserSettings& user_settings
	, const float01_t rel_volume
	, const bool reset_to_beginning
	)
{
	snd_sys.PlayAudioClip(
		MakeAssetID("alexander-nakarada-chase")
		, PlayAudioClip_Looped// PlayAudioClip_Looped | (reset_to_beginning ? PlayAudioClip_Reset : 0)
		, rel_volume * user_settings.ingame.sound.music_volume
		, SND_CHANNEL_MUSIC_FIGHT
		);
	return ALL_OK;
}

#endif


void SgGameplayMgr::_ChooseWhichMusicToPlayInBackground(
	const NwTime& game_time
	, NwSoundSystemI& snd_sys
	, const SgUserSettings& user_settings
	)
{
	//
#if GAME_CFG_WITH_SOUND

	const MillisecondsU32 time_elapsed_since_shots_were_fired_by_NPCs =
		game_time.real.total_msec - _data->last_time_when_we_should_have_played_fight_music
		;

	//
	nwNON_TWEAKABLE_CONST(float, MUSIC_LERP_FACTOR, 0.9f);

	nwNON_TWEAKABLE_CONST(int, AMBIENT_VS_FIGHT_MUSIC_CROSSFADE_DURATION_MSEC, 3000);

	const float01_t weight_for_fight_music
			= clampf(
			1.0f - float(time_elapsed_since_shots_were_fired_by_NPCs) / float(AMBIENT_VS_FIGHT_MUSIC_CROSSFADE_DURATION_MSEC) * CalcDamping(0.5, game_time.real.delta_seconds)
			, 0
			, 1.0f
			)
			;
	//
	const float01_t music_volume01 = user_settings.ingame.sound.music_volume * 0.01f;

	snd_sys.SetVolume(
		SND_CHANNEL_MUSIC
		, (1.0f - weight_for_fight_music) * music_volume01
		);
	snd_sys.SetVolume(
		SND_CHANNEL_MUSIC_FIGHT
		, weight_for_fight_music * music_volume01
		);

#if 0
	if(weight_for_fight_music > 1e-4f)
	{
		// unmute
		
		if(_data->last_weight_for_fight_music < 1e-4f)
		{
			// TODO: we need rewind the audio clip to the beginning
			DBGOUT("need to restart!!!!");
			PlayFightMusic(
				snd_sys
				, user_settings
				, weight_for_fight_music
				, true
				);
		}
	}
	else if (weight_for_fight_music < 1e-4f)
	{
		//mute
	}
#endif
	_data->last_weight_for_fight_music = weight_for_fight_music;

#endif // GAME_CFG_WITH_SOUND

}

namespace
{
	void FirePlasmaFromNPC(
		SgGameplayMgrData& mydata
		, const V3f& position_in_world
		, const V3f& direction_in_world
		, const float velocity
		, const EGameObjType ship_type
		)
	{
		SgProjectile	new_projectile;
		{
			new_projectile.pos = position_in_world;
			new_projectile.velocity = direction_in_world * velocity;
			new_projectile.life = 0;
			new_projectile.ship_type = ship_type;
		}

		mydata.npc_projectiles.AddWithWraparoundOnOverflow(new_projectile);
	}

	void FirePlasmaFromPlayer(
		SgGameplayMgrData& mydata
		, const V3f& position_in_world
		, const V3f& velocity_in_world
		, const EGameObjType ship_type
		)
	{
		SgProjectile	new_projectile;
		{
			new_projectile.pos = position_in_world;
			new_projectile.velocity = velocity_in_world;
			new_projectile.life = 0;
			new_projectile.ship_type = ship_type;
		}

		mydata.player_projectiles.AddWithWraparoundOnOverflow(new_projectile);
	}
}

ERet SgGameplayMgr::_TickAI(
	const NwTime& game_time
	, SgWorld& world
	, const Vector4& eye_position
	, SgPhysicsMgr& physics_mgr
	, NwSoundSystemI& snd_sys
	)
{
	_AdvanceShips(
		game_time
		, world
		, eye_position
		, physics_mgr
		);

	_AdvanceProjectilesFromNPCs(
		game_time
		, eye_position
		, world
		, physics_mgr
		, snd_sys
		);

	_AdvanceAndCollideProjectilesFromPlayer(
		game_time
		, world
		, physics_mgr
		);

	return ALL_OK;
}


struct SgShipAI_LOD_Setup: NwNonCopyable
{
	Vector4	ai_lod_distances_squared;

public:
	SgShipAI_LOD_Setup()
	{
		nwTWEAKABLE_CONST(float, AI_LOD0_RANGE, 20.f);
		nwTWEAKABLE_CONST(float, AI_LOD1_RANGE, 40.f);
		nwTWEAKABLE_CONST(float, AI_LOD2_RANGE, 100.f);
		nwTWEAKABLE_CONST(float, AI_LOD3_RANGE, 200.f);

		ai_lod_distances_squared = Vector4_Set(
			squaref(AI_LOD0_RANGE),
			squaref(AI_LOD1_RANGE),
			squaref(AI_LOD2_RANGE),
			squaref(AI_LOD3_RANGE)
			);
	}

	mxFORCEINLINE BOOL ShouldSkipUpdatingAI(
		const Vector4& v_ship_pos
		, const U32 ship_index
		, const Vector4& v_eye_position
		, const U32 frame_number
		) const
	{
		const Vector4 v_diff = V4_SUB( v_ship_pos, v_eye_position );
		const Vector4 v_dist_sq = Vector4_Dot3( v_diff, v_diff );

		const Vector4 v_cmp_mask = V4_CMPGT(v_dist_sq, ai_lod_distances_squared);

		// 0, 1, 3, 7 or 0xF
		const U32 mask = (U32)_mm_movemask_ps(v_cmp_mask);

		return ( (frame_number + ship_index) & mask );
	}
};


ERet SgGameplayMgr::_AdvanceShips(
								  const NwTime& game_time
								  , const SgWorld& world
								  , const Vector4& eye_position
								  , SgPhysicsMgr& physics_mgr
								  )
{
	mxASSERT(!_data->has_ships_to_destroy);

	//
	nwTWEAKABLE_CONST(float, SHIP_PERCEPTION_RADIUS, 10.0f);

	// 
	nwTWEAKABLE_CONST(float, SHIP_SEPARATION_DISTANCE_MARGIN, 10.f);
	nwTWEAKABLE_CONST(float, SHIP_SEPARATION_WEIGHT, 1.f);
	nwTWEAKABLE_CONST(float, SHIP_ALIGNMENT_WEIGHT, 0.6f);
	nwTWEAKABLE_CONST(float, SHIP_COHESION_WEIGHT, 0.5f);
	nwTWEAKABLE_CONST(float, SHIP_STEERING_WEIGHT, 0.9f);

	nwTWEAKABLE_CONST(float, SHIP_MAX_ACCELERATION, 5);
	nwTWEAKABLE_CONST(float, SHIP_MAX_VELOCITY, 50);
	nwTWEAKABLE_CONST(float, SHIP_MOVE_IMPULSE_SCALE, 40);

	// 1 == no damping
	nwTWEAKABLE_CONST(float, LinearVelocityDamping, 0.98);	// 0..1
	nwTWEAKABLE_CONST(float, AngularVelocityDamping, 0.9);	// 0..1

	nwTWEAKABLE_CONST(float, FIGHTER_SHIP_ROTATION_SPEED_PER_SEC, 1.5f);
	
	// 2 hits enough to bring down a fighter
	nwTWEAKABLE_CONST(int, SHIP_PLASMA_CANNON_DAMAGE, 50);

	//
#if MX_DEVELOPER
	TbPrimitiveBatcher& dbgdraw = SgGameApp::Get().renderer._render_system->_debug_renderer;
#endif

	//
	TSpan<SgShip> spaceships_span = GetSpaceships(_data);
	SgShip* ships = spaceships_span._data;
	const UINT num_spaceships = spaceships_span._count;

	//
	SgShipAI* ships_ai = _data->ships_ai;

	//
	SgShipAI_LOD_Setup	ai_lod_setup;

	const U32 frame_number = game_time.real.frame_number;

	//
	const StridedRBInstanceData rb_instances_data = physics_mgr.GetRBInstancesData();

	//
	CulledObjectsIDs	nearby_objects_ids(MemoryHeaps::temporary());

	CulledObjectID		nearby_objects_ids_stack_storage[128+64];//131
	Arrays::initializeWithExternalStorage( nearby_objects_ids, nearby_objects_ids_stack_storage );


	//
	bool has_ships_to_destroy = false;
	bool should_play_fight_music = false;

	U32	num_ships_ticked = 0;

	U32	num_remaining_enemy_ships = 0;

	//
	const SgShip* ship_to_defend = FindSpaceship( _data->h_my_ship_to_defend );

	const Vector4 v_my_base_center
		= ship_to_defend
		? physics_mgr.GetRigidBodyCenterOfMassPosition(ship_to_defend->rigid_body_handle)
		: Vector4_Zero()
		;

	//
	SgShip* user_specified_enemy_ship_to_attack = FindSpaceship( _data->h_enemy_ship_to_attack );



	//
	for(UINT curr_ship_ai_idx = 0;
		curr_ship_ai_idx < num_spaceships;
		curr_ship_ai_idx++)
	{
		SgShipAI & curr_ship_ai = ships_ai[ curr_ship_ai_idx ];

		// if this ship is dead:
		if(curr_ship_ai.life <= 0)
		{
			has_ships_to_destroy = true;
			continue;
		}

		// skip if this ship is controlled by the player
		if(mxUNLIKELY(spaceship_controller._spaceship_handle == curr_ship_ai.ship_handle))
		{
			SgShip & curr_ship = GetSpaceship(curr_ship_ai.ship_handle);

			// prevent the ship from rotating too fast
			physics_mgr.DampAngularVelocity(
				curr_ship.rigid_body_handle
				, 0.99f
				);
			// don't dampen linear velocity!
			continue;
		}

		//
		SgShip & curr_ship = GetSpaceship(curr_ship_ai.ship_handle);
		const U32 curr_ship_rb_idx = &curr_ship - ships;

		// ship indices are equal to rigid body indices
		const SgRBInstanceData& rb_inst = rb_instances_data.GetAt(curr_ship_rb_idx);

		//
		const Vector4	v_curr_ship_center = rb_inst.pos;


		//
		const bool curr_ship_is_my_enemy = (curr_ship.flags & fl_ship_is_my_enemy);
		num_remaining_enemy_ships += !!curr_ship_is_my_enemy;

		// spread the update across multiple frames
		if( ai_lod_setup.ShouldSkipUpdatingAI(v_curr_ship_center, curr_ship_rb_idx, eye_position, frame_number ) )
		{
			continue;
		}

		++num_ships_ticked;


		//
		//const SgShipDef& curr_ship_def = _data->ship_defs[ curr_ship.type ];

		//
		float curr_ship_ai_collision_radius = _data->ship_ai_collision_radii[ curr_ship.type ];


		// separate attacking ships
		if(curr_ship.target_handle.isValid() ) {
			curr_ship_ai_collision_radius *= 16.0f;
		}


		//
		const Vector4	v_curr_ship_fwddir = physics_mgr.GetRigidBodyForwardDirection(
			curr_ship.rigid_body_handle
			);

		//
		const V3f& curr_ship_center = Vector4_As_V3(v_curr_ship_center);


		// Find nearest ships for flocking behavior.
		// NOTE: this is the most expensive part.
		{
			SimdAabb	enemy_search_region_aabb;
			SimdAabb_CreateFromSphere(
				&enemy_search_region_aabb
				, v_curr_ship_center
				, SHIP_PERCEPTION_RADIUS
				);

			physics_mgr._aabb_tree->GatherObjectsInAABB(
				enemy_search_region_aabb
				, nearby_objects_ids
				);
			mxASSERT(nearby_objects_ids._count <= mxCOUNT_OF(nearby_objects_ids_stack_storage));
		}


		//
		CV3f separation_sum(0);
		CV3f velocity_sum(0);	// used for alignment
		CV3f position_sum(curr_ship_center);	// used for cohesion

		int	num_of_all_nearby_ships = 0;	// including enemies
		int	num_of_nearby_flockmates = 0;

		// NOTE: not necessarily the nearest enemy!
		SgShip* nearby_enemy = nil;
		float	squared_distance_to_nearby_enemy = BIG_NUMBER;

		//
		for(UINT nearby_obj_idx = 0;
			nearby_obj_idx < nearby_objects_ids._count;
			nearby_obj_idx++)
		{
			const CulledObjectID nearby_obj_id = nearby_objects_ids._data[ nearby_obj_idx ];
			const U32 nearby_ship_idx = nearby_obj_id & CULLED_OBJ_INDEX_MASK;

			SgShip& nearby_ship = ships[ nearby_ship_idx ];

			//
			if(nearby_ship.my_handle == curr_ship.my_handle)
			{
				continue;
			}

			// ship indices are equal to rigid body indices
			const SgRBInstanceData& nearby_ship_rb_inst = rb_instances_data.GetAt(nearby_ship_idx);

			//
			const Vector4 v_vector_from_nearby_ship_to_this_ship = V4_SUB( v_curr_ship_center, nearby_ship_rb_inst.pos );

			const float distance_to_nearby_ship_squared = Vector4_Get_X( Vector4_Length3Squared(
				V4_SUB( v_curr_ship_center, nearby_ship_rb_inst.pos )
			) );

			//
			const float nearby_ship_ai_collision_radius = _data->ship_ai_collision_radii[ nearby_ship.type ];

			float combined_collision_radius = curr_ship_ai_collision_radius + nearby_ship_ai_collision_radius;

			const bool nearby_spaceship_is_enemy = (
				curr_ship_is_my_enemy
				^
				(nearby_ship.flags & fl_ship_is_my_enemy)
				);
			if(nearby_spaceship_is_enemy)
			{
				nearby_enemy = &nearby_ship;
				squared_distance_to_nearby_enemy = distance_to_nearby_ship_squared;
			}
			else if(nearby_ship.target_handle.isNil())
			{
				nearby_ship.target_handle = curr_ship.target_handle;
			}

			//
			if( distance_to_nearby_ship_squared < squaref(combined_collision_radius) )
			{
				separation_sum += Vector4_As_V3(v_vector_from_nearby_ship_to_this_ship);

				// don't stick to enemy ships
				if(!nearby_spaceship_is_enemy)
				{
					velocity_sum += Vector4_As_V3(nearby_ship_rb_inst.linear_velocity);
					position_sum += Vector4_As_V3(nearby_ship_rb_inst.pos);
					++num_of_nearby_flockmates;
				}

				++num_of_all_nearby_ships;
			}

		}//process nearby ships



		//
		V3f steering_target = curr_ship_center;


		// attack a nearby enemy ship by default
		SgShip* enemy_ship_to_attack = nearby_enemy;


		//
		if(!curr_ship_is_my_enemy && user_specified_enemy_ship_to_attack)
		{
			enemy_ship_to_attack = user_specified_enemy_ship_to_attack;
		}


		// if this ship already has an enemy:
		if(!enemy_ship_to_attack && curr_ship.target_handle.isValid())
		{
			SgShip* targeted_enemy_ship = FindSpaceship( curr_ship.target_handle );

			// if the enemy is alive:
			if(targeted_enemy_ship)
			{
				enemy_ship_to_attack = targeted_enemy_ship;
			}
		}


		//
		if(!enemy_ship_to_attack && curr_ship.longterm_goal_handle.isValid())
		{
			enemy_ship_to_attack = FindSpaceship( curr_ship.longterm_goal_handle );
		}



		// if the enemy is alive:

		float	sq_distance_to_chosen_enemy = BIG_NUMBER;
		V3f		enemy_ship_center = { 0, 0, 0 };

		if(enemy_ship_to_attack)
		{
			// pursue the enemy
			curr_ship.target_handle = enemy_ship_to_attack->my_handle;

			curr_ship_ai.has_ever_seen_enemies = true;

			//
			const SgRBInstanceData& enemy_ship_rb_inst = rb_instances_data.GetAt( enemy_ship_to_attack - ships );

			enemy_ship_center = Vector4_As_V3(enemy_ship_rb_inst.pos);
			steering_target = enemy_ship_center;

			sq_distance_to_chosen_enemy = Vector4_Get_X( Vector4_Length3Squared(
				V4_SUB( v_curr_ship_center, enemy_ship_rb_inst.pos )
				) );

			// if the enemy ship is close, we need to play the fight music
			if(curr_ship_is_my_enemy
			&& sq_distance_to_chosen_enemy < 250000.0f)
			{
				should_play_fight_music = true;
			}
		}
		else
		{
			// the enemy is dead
			curr_ship.target_handle.setNil();
			curr_ship.longterm_goal_handle.setNil();

			// if this is my ship, return to the mothership
			if(!curr_ship_is_my_enemy
				&& curr_ship_ai.has_ever_seen_enemies
				)
			{
				//
				if(Vector4_Get_X( Vector4_Length3Squared( V4_SUB( v_curr_ship_center, v_my_base_center ) ) ) > 10000.0f)
				{
					steering_target = Vector4_As_V3(v_my_base_center);
				}
			}
		}


		//
		const float inv_num_nearby_ships = num_of_all_nearby_ships ? (1.0f / num_of_all_nearby_ships) : 1.0f;
		const float inv_num_nearby_flockmates = num_of_nearby_flockmates ? (1.0f / num_of_nearby_flockmates) : 1.0f;

		// Separation: steer to avoid crowding
		const V3f separation = (separation_sum * inv_num_nearby_ships).normalized();

		// Alignment: steer towards the average heading of local flockmates
		const V3f alignment = (velocity_sum * inv_num_nearby_flockmates).normalized();

		// Cohesion: steer to move toward the average position of local flockmates
		const V3f average_position = (num_of_nearby_flockmates > 0)
			? (position_sum * inv_num_nearby_flockmates)
			: curr_ship_center
			;
		
		// Get direction toward center of mass
		const V3f cohesion = (average_position - curr_ship_center).normalized();

		// Steering: steer towards the nearest target location (like a moth to the light)
		const V3f steering = (steering_target - curr_ship_center).normalized();


		// calculate boid acceleration

		const V3f	separation_force = separation * SHIP_SEPARATION_WEIGHT;
		const V3f	alignment_force = alignment * SHIP_ALIGNMENT_WEIGHT;
		const V3f	cohesion_force = cohesion * SHIP_COHESION_WEIGHT;
		const V3f	steering_force = steering * SHIP_STEERING_WEIGHT;

		V3f goal_acceleration = separation_force + alignment_force + cohesion_force + steering_force;
		ClampLength(goal_acceleration, SHIP_MAX_ACCELERATION);

		//
		physics_mgr.ApplyCentralImpulse(
			curr_ship.rigid_body_handle
			, (goal_acceleration * SHIP_MOVE_IMPULSE_SCALE) * game_time.real.delta_seconds
			);

		//
		physics_mgr.ClampLinearVelocityTo(
			curr_ship.rigid_body_handle
			, SHIP_MAX_VELOCITY
			);


		// rotate to face the target

		physics_mgr.ChangeAngularVelocityToFaceDirection(
			curr_ship.rigid_body_handle
			, Vector4_LoadFloat3_Unaligned( steering.a )
			, FIGHTER_SHIP_ROTATION_SPEED_PER_SEC * game_time.real.delta_seconds
			);

		// if no enemy or goal, allow the ship to freely rotate/float in space
		if(enemy_ship_to_attack)
		{
			physics_mgr.DampAngularVelocity(
				curr_ship.rigid_body_handle
				, CalcDamping( AngularVelocityDamping, game_time.real.delta_seconds )
				);
		}

		//if(DampVelocity)
		{
			physics_mgr.DampLinearVelocity(
				curr_ship.rigid_body_handle
				, CalcDamping( LinearVelocityDamping, game_time.real.delta_seconds )
				);
		}


		// try to attack the chosen enemy
		if(
			enemy_ship_to_attack
			&&
			// if we can fire
			curr_ship_ai.timestamp_msec_when_can_fire <= game_time.real.total_msec
			&&
			// if the enemy is in range of our weapon
			sq_distance_to_chosen_enemy < _data->ship_ai_squared_attack_range[ curr_ship.type ]
			)
		{
			//
			const V3f dir_to_enemy_ship = (enemy_ship_center - curr_ship_center).normalized();
			const float cos_angle_between_my_fwd_dir_and_dir_to_enemy_ship = Vector4_As_V3(v_curr_ship_fwddir) * dir_to_enemy_ship;

			// shoot if the angle is small enough
			if(cos_angle_between_my_fwd_dir_and_dir_to_enemy_ship > 0.8f)
			{
				curr_ship_ai.timestamp_msec_when_can_fire = game_time.real.total_msec + SHIP_CANNON_COOLDOWN_MSEC;

				//
				FirePlasmaFromNPC(
					*_data
					, curr_ship_center
					, Vector4_As_V3(v_curr_ship_fwddir)
					, PLASMA_BOLT_VELOCITY
					, curr_ship.type
					);

				//
				if(cos_angle_between_my_fwd_dir_and_dir_to_enemy_ship > 0.95f)
				{
					SgShipAI& enemy_ship_ai = ships_ai[ enemy_ship_to_attack->ship_ai_index ];
					enemy_ship_ai.life -= SHIP_PLASMA_CANNON_DAMAGE;
				}
			}
		}//if enemy/target handle is valid

	}//for each ship AI

	//
	_data->has_ships_to_destroy = has_ships_to_destroy;

	if(should_play_fight_music) {
		_data->last_time_when_we_should_have_played_fight_music = game_time.real.total_msec;
	}

	_data->num_remaining_enemy_ships = num_remaining_enemy_ships;

	//
	//DBGOUT("frame = %d, num_ships_ticked = %d", frame_number, num_ships_ticked);

	return ALL_OK;
}

ERet SgGameplayMgr::CreateSpaceship(
	SgShipHandle &new_ship_handle_
	, const EGameObjType spaceship_type
	, SgWorld& world
	, SgPhysicsMgr& physics_mgr
	, const V3f& pos
	, const int flags
	)
{
	const SgShipDef& ship_def = _data->ship_defs[ spaceship_type ];
	mxUNUSED(ship_def);

	//
	const UINT new_spaceship_index = _data->ship_handle_mgr._num_alive_items;

	//
	SgShipHandle	new_ship_handle;
	mxDO(_data->ship_handle_mgr.AllocHandle(
		new_ship_handle.id
		));

	//
	SgShip& new_spaceship = _data->ships[ new_spaceship_index ];

	//
	new_ship_handle_ = new_ship_handle;

	//
	new_spaceship.flags = flags;
	new_spaceship.type = spaceship_type;

	//
	new_spaceship.my_handle = new_ship_handle;

	new_spaceship.target_handle.setNil();
	new_spaceship.longterm_goal_handle.setNil();

	//
	mxDO(physics_mgr.CreateRigidBody(
		new_spaceship.rigid_body_handle
		, _data->ship_col_shapes[ spaceship_type ]
		, spaceship_type
		, new_ship_handle.id
		, ship_def.mass
		, pos
		));

	mxASSERT2(
		new_spaceship.rigid_body_handle.id == new_ship_handle_.id
		, "spaceship and rigid body indices must match! otherwise, frustum culling will be wrong"
		);

	//
	//
	new_spaceship.ship_ai_index = new_spaceship_index;

	//
	SgShipAI& new_spaceship_ai = _data->ships_ai[ new_spaceship_index ];
	new_spaceship_ai.ship_handle = new_ship_handle;
	new_spaceship_ai.life = ship_def.health;
	new_spaceship_ai.timestamp_msec_when_can_fire = 0;
	new_spaceship_ai.has_ever_seen_enemies = false;

	return ALL_OK;
}

void SgGameplayMgr::RemoveDeadShips(
									 const NwTime& game_time
									 , SgWorld& world
									 , SgPhysicsMgr& physics_mgr
									 , NwSoundSystemI& snd_sys
									 )
{
	if(!_data->has_ships_to_destroy) {
		return;
	}

	SgGameApp& game_app = SgGameApp::Get();

#if MX_DEVELOPER
	TbPrimitiveBatcher& dbgdraw = game_app.renderer._render_system->_debug_renderer;
#endif

	//
	TSpan<SgShip> ships = GetSpaceships(_data);
	TSpan< SgShipAI > ships_ai = GetSpaceshipAIs(_data);

	// deleted ships are moved to the end of the array
	U32 num_ships_to_iterate = ships_ai._count;

	//
	for(UINT ship_ai_idx = 0;
		ship_ai_idx < num_ships_to_iterate;
		ship_ai_idx++)
	{
		SgShipAI & ai_of_ship_to_destroy = ships_ai._data[ ship_ai_idx ];

		SgShip & ship_to_destroy = GetSpaceship( ai_of_ship_to_destroy.ship_handle );
		mxASSERT(ship_to_destroy.ship_ai_index == ship_ai_idx);
		mxASSERT(ship_to_destroy.flags != mxDBG_FREED_MEMORY_TAG);

		if(ai_of_ship_to_destroy.life > 0) {
			continue;
		}

		//
		SimdAabb	ship_aabb;
		physics_mgr.GetRigidBodyAABB(
			ship_aabb
			, ship_to_destroy.rigid_body_handle
			);

		//
		//dbgdraw.DrawAABB(SimdAabb_Get_AABBf(ship_aabb), RGBAf::WHITE);

		//
		const Vector4 v_ship_center = physics_mgr.GetRigidBodyCenterOfMassPosition(ship_to_destroy.rigid_body_handle);

		//
		game_app.renderer.explosions.AddExplosion(
			Vector4_As_V3(v_ship_center)
			, _data->ship_ai_collision_radii[ ship_to_destroy.type ] * 4.0f
			, 1.0f
			);

		//
#if GAME_CFG_WITH_SOUND
		snd_sys.PlaySound3D(
			MakeAssetID("ship_explosion")
			, Vector4_As_V3(v_ship_center)
			);
#endif

		// delete the rigid body
		physics_mgr.DeleteRigidBody(ship_to_destroy.rigid_body_handle);

		// delete the ship
		U32	indices_of_swapped_items[2];
		_data->ship_handle_mgr.DestroyHandle(
			ship_to_destroy.my_handle.id
			, ships._data
			, indices_of_swapped_items
			);

		// delete the AI brain
		SgShip & the_ship_that_was_moved_in_place_of_destroyed_ship = ships[
			indices_of_swapped_items[0]
		];
	
		ai_of_ship_to_destroy
			= ships_ai._data[ the_ship_that_was_moved_in_place_of_destroyed_ship.ship_ai_index ]
			;

		the_ship_that_was_moved_in_place_of_destroyed_ship.ship_ai_index = ship_ai_idx;

		//
		--num_ships_to_iterate;

	}//for

	//
	_data->has_ships_to_destroy = false;

	// Check if the target enemy ship has been destroyed.
	if(!_data->ship_handle_mgr.IsValidHandle(_data->h_enemy_ship_to_attack.id))
	{
		_data->h_enemy_ship_to_attack.setNil();
	}
}

void SgGameplayMgr::_StartNextEnemyWaveIfNeeded(
	const NwTime& game_time
	, SgWorld& world
	, SgPhysicsMgr& physics_mgr
	, NwSoundSystemI& snd_sys
	, const SgUserSettings& user_settings
	)
{
	const MillisecondsU32 time_elapsed_since_current_session_started
		= mxGetTimeInMilliseconds() - _data->time_when_current_session_started
		;

	for(int enemy_wave_index = _data->current_enemy_wave+1;
		enemy_wave_index < mxCOUNT_OF(ENEMY_WAVE_TIMES_MSEC);
		enemy_wave_index++)
	{
		if(time_elapsed_since_current_session_started > ENEMY_WAVE_TIMES_MSEC[enemy_wave_index])
		{
			_StartEnemyWave(
				enemy_wave_index
				, game_time
				, world
				, physics_mgr
				, snd_sys
				, user_settings
				);
			break;
		}
	}
}

ERet SgGameplayMgr::_AdvanceProjectilesFromNPCs(
										const NwTime& game_time
										, const Vector4& eye_position
										, SgWorld& world
										, SgPhysicsMgr& physics_mgr
										, NwSoundSystemI& snd_sys
										)
{
	if(_data->npc_projectiles.isEmpty())
	{
		return ALL_OK;
	}

	const SecondsF plasma_life_span_seconds = 7.0f;
	const F32 plasma_lifespan_inv = 1.0f / plasma_life_span_seconds;

	//
	SgProjectile* projectiles_array = _data->npc_projectiles.raw();
	UINT num_remaining_projectles = _data->npc_projectiles.num();

#if MX_DEVELOPER
	TbPrimitiveBatcher& dbgdraw = SgGameApp::Get().renderer._render_system->_debug_renderer;
#endif

	//
	UINT projectile_idx = 0;

	while( projectile_idx < num_remaining_projectles )
	{
		SgProjectile &	projectile = projectiles_array[ projectile_idx ];

		const F32 plasma_life = projectile.life;
		const F32 plasma_life_new = plasma_life + game_time.real.delta_seconds * plasma_lifespan_inv;

		if( plasma_life_new < 1.0f )
		{
			const V3f plasma_position = projectile.pos;
			const V3f plasma_position_new = plasma_position + projectile.velocity * game_time.real.delta_seconds;
			//
			projectile.pos = plasma_position_new;
			projectile.life = plasma_life_new;

			// the projectile is still alive
			++projectile_idx;

			// Play shoot sound
#if GAME_CFG_WITH_SOUND
			if( plasma_life == 0 && (Vector4_As_V3(eye_position) - projectile.pos).lengthSquared() < 2500.0f )
			{
				snd_sys.PlaySound3D(
					MakeAssetID("enemy_ship_plasma_cannon")
					, plasma_position
					);
			}
#endif
		}
		else
		{
			// the projectile is dead - remove it
			TSwap( projectile, projectiles_array[ --num_remaining_projectles ] );
		}
	}

	//
	_data->npc_projectiles.setNum( num_remaining_projectles );

	return ALL_OK;
}


ERet SgGameplayMgr::_AdvanceAndCollideProjectilesFromPlayer(
										const NwTime& game_time
										, SgWorld& world
										, SgPhysicsMgr& physics_mgr
										)
{
	if(_data->player_projectiles.isEmpty())
	{
		return ALL_OK;
	}

	const SecondsF plasma_life_span_seconds = 8.0f;
	const F32 plasma_lifespan_inv = 1.0f / plasma_life_span_seconds;

	//
	SgProjectile* projectiles_array = _data->player_projectiles.raw();
	UINT num_remaining_projectles = _data->player_projectiles.num();

#if MX_DEVELOPER
	TbPrimitiveBatcher& dbgdraw = SgGameApp::Get().renderer._render_system->_debug_renderer;
#endif

	//
	UINT projectile_idx = 0;

	while( projectile_idx < num_remaining_projectles )
	{
		SgProjectile &	projectile = projectiles_array[ projectile_idx ];

		const F32 plasma_life = projectile.life;
		const F32 plasma_life_new = plasma_life + game_time.real.delta_seconds * plasma_lifespan_inv;

		if( plasma_life_new < 1.0f )
		{
			//
			const V3f plasma_position = projectile.pos;
			const V3f plasma_position_new = plasma_position + projectile.velocity * game_time.real.delta_seconds;

			//
			projectile.pos = plasma_position_new;
			projectile.life = plasma_life_new;

mxOPTIMIZE("deferred collision checks");

			// check if some ship is hit
			CulledObjectID	hit_ship_id;
			if(physics_mgr._aabb_tree->CastRay(
				plasma_position
				, projectile.velocity.normalized()
				, hit_ship_id
				))
			{
				const U32 hit_spaceship_idx = hit_ship_id & CULLED_OBJ_INDEX_MASK;
				SgShip& closest_hit_ship = _data->ships[ hit_spaceship_idx ];

				// if this is not the player's ship
				if(closest_hit_ship.my_handle != spaceship_controller._spaceship_handle)
				{
					const Vector4 closest_hit_ship_pos = physics_mgr.GetRigidBodyCenterOfMassPosition(
						closest_hit_ship.rigid_body_handle
						);

					const float sq_dist_between_projectile_and_enemy_ship = Vector4_Get_X( Vector4_LengthSquared(
						V4_SUB(closest_hit_ship_pos, Vector4_LoadFloat3_Unaligned(plasma_position.a) )
						) );

					if( sq_dist_between_projectile_and_enemy_ship < squaref(20) )
					{
						SgShipAI& hit_ship_ai = _data->ships_ai[ closest_hit_ship.ship_ai_index ];
						hit_ship_ai.life -= 101;// SHIP_PLASMA_CANNON_DAMAGE;
						goto L_RemoveDeadProjectile;
					}
				}
			}

			// the projectile is still alive
			++projectile_idx;
		}
		else
		{
L_RemoveDeadProjectile:
			// the projectile is dead - remove it
			TSwap( projectile, projectiles_array[ --num_remaining_projectles ] );
		}
	}

	//
	_data->player_projectiles.setNum( num_remaining_projectles );

	return ALL_OK;
}

const SgGameplayRenderData SgGameplayMgr::GetRenderData() const
{
	SgGameplayRenderData	result = {
		TSpan< const SgShip >(_data->ships, _data->ship_handle_mgr._num_alive_items),

		Arrays::getSpan(_data->ship_defs),

		Arrays::getSpan(_data->npc_projectiles),
		Arrays::getSpan(_data->player_projectiles),
	};

#if SG_USE_SDF_VOLUMES
	for(int i =0; i < mxCOUNT_OF(_data->ship_sdf_textures); i++) {
		const NwTexture::Ref& tex_ref = _data->ship_sdf_textures[i];
		result.ship_sdf_volume_textures_by_ship_type[i] = tex_ref->m_texture;
	}
#endif
	return result;
}

const NwHandleTable& SgGameplayMgr::GetShipHandleMgr() const
{
	return _data->ship_handle_mgr;
}

const SgShipDef& SgGameplayMgr::GetShipDef(
	const EGameObjType obj_type
	) const
{
	return _data->ship_defs[ obj_type ];
}

SgShip* SgGameplayMgr::FindSpaceship(
	const SgShipHandle handle
	) const
{
	return _data->ship_handle_mgr.SafeGetItemByHandle(handle.id, _data->ships);
}

SgShip& SgGameplayMgr::GetSpaceship(
	const SgShipHandle handle
	) const
{
	const U32 ship_index = _data->ship_handle_mgr.GetItemIndexByHandle(handle.id);
	return _data->ships[ ship_index ];
}

ERet SgGameplayMgr::AttackEnemySpaceship(
	const SgShipHandle h_ship_to_attack
	)
{
	DBGOUT("Attack enemy ship: handle = %d", h_ship_to_attack.id);
	_data->h_enemy_ship_to_attack = h_ship_to_attack;
	return ALL_OK;
}

ERet SgGameplayMgr::TakeControlOfSpaceship(
	const SgShipHandle spaceship_to_control
	)
{
	const SgShip* spaceship = FindSpaceship(spaceship_to_control);
	mxENSURE(spaceship
		, ERR_OBJECT_NOT_FOUND
		, ""
		);

	//
	const SgShipDef& ship_def = _data->ship_defs[ spaceship->type ];

	//
	spaceship_controller.AssumeControlOfSpaceship(*spaceship);

	// place the camera outside the ship
	spaceship_controller._following_camera.SetOrbitDistance(
		ship_def.lod_meshes[0]->local_aabb.size().maxComponent()
		);

	return ALL_OK;
}

ERet SgGameplayMgr::ShootCannonOfPlayerControlledShip(
	const SgShipHandle ship_handle
	, const V3f& ship_origin
	, const V3f& ship_forward_vec
	, const V3f& spaceship_right_vec
	, const V3f& spaceship_up_vec
	, const NwTime& game_time
	, const V3f& ship_velocity
	, SgPhysicsMgr& physics_mgr
	, NwSoundSystemI& snd_sys
	)
{
	SgShip* ship = FindSpaceship(ship_handle);
	mxENSURE(ship, ERR_OBJECT_NOT_FOUND, "");

	//
	SgShipAI & ship_ai = _data->ships_ai[ ship->ship_ai_index ];

	// if we can fire
	if(ship_ai.timestamp_msec_when_can_fire <= game_time.real.total_msec)
	{
		const SgShipDef& ship_def = _data->ship_defs[ ship->type ];

		//
		ship_ai.timestamp_msec_when_can_fire = game_time.real.total_msec + ship_def.cannon_firerate;

		// start firing in front of the ship, not its center
		const V3f mesh_aabb_halfsize = ship_def.lod_meshes[0]->local_aabb.halfSize();
		const float gun_offset_fwd = mesh_aabb_halfsize.maxComponent();
		const float gun_offset_side = gun_offset_fwd * ship_def.cannon_horiz_spread;
		const float gun_offset_vert = gun_offset_fwd * ship_def.cannon_vert_spread;

		//
		nwTWEAKABLE_CONST(float, PLAYER_WEAPON_PLASMA_CANNON_KICKBACK, 0.1f);
		nwTWEAKABLE_CONST(float, PLAYER_WEAPON_PLASMA_CANNON_SIDEWAYS_KICK, 0.1f);

		//
		static int cannon = 0;
		cannon = (cannon + 1) & 3;

		//
		NwRandom	rng(
			game_time.real.frame_number ^ ship_handle.id
			);

		const V3f sideways_kick = spaceship_right_vec * (PLAYER_WEAPON_PLASMA_CANNON_SIDEWAYS_KICK * ((cannon&1) ? -1.f : +1.f));

		physics_mgr.ApplyCentralImpulse(
			ship->rigid_body_handle
			, sideways_kick - ship_forward_vec * PLAYER_WEAPON_PLASMA_CANNON_KICKBACK
			);

		//
		const V3f plasma_initial_pos
			= ship_origin
			+ ship_forward_vec * gun_offset_fwd
			+ spaceship_right_vec * gun_offset_side * ((cannon&1) ? -1.f : +1.f)
			+ spaceship_up_vec * gun_offset_vert * ((cannon&2) ? -1.f : +1.f)
			;

		//
#if GAME_CFG_WITH_SOUND
		const V3f plasma_velocity_vec = ship_forward_vec * PLASMA_BOLT_VELOCITY;
		snd_sys.PlaySound3D(
			MakeAssetID("ship_plasma_cannon")
			, plasma_initial_pos
			, &plasma_velocity_vec
			);
#endif

		//
		FirePlasmaFromPlayer(
			*_data
			
			, plasma_initial_pos
			, ship_velocity + ship_forward_vec * PLASMA_BOLT_VELOCITY
			, ship->type
			);

		return ALL_OK;
	}

	return ERR_NEED_TO_WAIT;
}

ERet SgGameplayMgr::DbgSpawnFriendlyShip(
	const V3f& pos
	, const EGameObjType spaceship_type
	)
{
	SgGameApp& app = SgGameApp::Get();

	SgShipHandle	ship_handle;
	mxDO(app.gameplay.CreateSpaceship(
		ship_handle
		, spaceship_type
		, app.world
		, app.physics_mgr
		, pos
		));
	return ALL_OK;
}

ERet SgGameplayMgr::DbgSpawnEnemyShip(
									  const V3f& pos
									  )
{
	SgGameApp& app = SgGameApp::Get();

	SgShipHandle	ship_handle;
	mxDO(app.gameplay.CreateSpaceship(
		ship_handle
		, obj_alien_fighter_ship
		, app.world
		, app.physics_mgr
		, pos
		, fl_ship_is_my_enemy
		));
	return ALL_OK;
}
