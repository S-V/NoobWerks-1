//
#include "stdafx.h"
#pragma hdrstop

#include <Engine/Model.h>

#include "FPSGame.h"
#include "game_world/pickable_items.h"	// SpawnItem()
#include "gameplay_constants.h"	// PickableItem8
#include "npc/npc_behavior_common.h"
#include "game_world/game_level.h"


mxBEGIN_REFLECT_ENUM( GameMode8 )
	mxREFLECT_ENUM_ITEM( REACH_EXIT, GM_REACH_EXIT ),	
	mxREFLECT_ENUM_ITEM( SURVIVE_WAVES, GM_SURVIVE_WAVES ),
mxEND_REFLECT_ENUM


mxBEGIN_FLAGS( AttackWavesT )
	mxREFLECT_BIT( 0, EAttackWave::Wave0 ),
	mxREFLECT_BIT( 1, EAttackWave::Wave1 ),
	mxREFLECT_BIT( 2, EAttackWave::Wave2 ),
	mxREFLECT_BIT( 3, EAttackWave::Wave3 ),
	mxREFLECT_BIT( 4, EAttackWave::Wave4 ),
	//mxREFLECT_BIT( All, EAttackWave::WaveAll ),
mxEND_FLAGS


mxDEFINE_CLASS(MyLevelData);
mxBEGIN_REFLECTION(MyLevelData)
	mxMEMBER_FIELD( game_mode ),
	mxMEMBER_FIELD( num_attack_waves ),
	mxMEMBER_FIELD( background_music ),

	mxMEMBER_FIELD( item_spawn_points ),
	mxMEMBER_FIELD( npc_spawn_points ),

	mxMEMBER_FIELD( player_spawn_pos ),
	mxMEMBER_FIELD( mission_exit_pos ),
	mxMEMBER_FIELD( next_level ),
mxEND_REFLECTION;
MyLevelData::MyLevelData(AllocatorI& allocator)
: item_spawn_points(allocator)
, npc_spawn_points(allocator)
{
	game_mode = GM_REACH_EXIT;
	num_attack_waves = 1;
	background_music = MakeAssetID("music_rmr_level4");
	player_spawn_pos = CV3f(0);
	mission_exit_pos = CV3f(0);
}

//mxDEFINE_CLASS(MyLevelData::Sky);
//mxBEGIN_REFLECTION(MyLevelData::Sky)
//	//mxMEMBER_FIELD( pos ),
//	//mxMEMBER_FIELD( music_volume ),
//	//mxMEMBER_FIELD( effects_volume ),
//	//mxMEMBER_FIELD( use_stereo ),
//mxEND_REFLECTION;
//MyLevelData::Sky::Sky()
//{
//	//pos = CV3f(0);
//}
//

mxDEFINE_CLASS(MyLevelData::ItemSpawnPoint);
mxBEGIN_REFLECTION(MyLevelData::ItemSpawnPoint)
	mxMEMBER_FIELD( pos ),
	mxMEMBER_FIELD( type ),
	mxMEMBER_FIELD( waves ),
mxEND_REFLECTION;
MyLevelData::ItemSpawnPoint::ItemSpawnPoint()
{
	pos = CV3f(0);
	waves = Wave0;
	type = Gameplay::EPickableItem::ITEM_MEDKIT;
}


mxDEFINE_CLASS(MyLevelData::NpcSpawnPoint);
mxBEGIN_REFLECTION(MyLevelData::NpcSpawnPoint)
	mxMEMBER_FIELD( pos ),
	mxMEMBER_FIELD( type ),
	mxMEMBER_FIELD( count ),
	mxMEMBER_FIELD( waves ),
	mxMEMBER_FIELD( comment ),
mxEND_REFLECTION;
MyLevelData::NpcSpawnPoint::NpcSpawnPoint()
{
	pos = CV3f(0);
	type = MONSTER_TRITE;
	count = 1;
	waves = Wave0;
}

namespace MyLevelData_
{

	V3f GetPosForSpawnPoint()
	{
		const MyGamePlayer& player = FPSGame::Get().player;
return player.GetEyePosition();
		V3f pos_in_front_of_player = player.GetPhysicsBodyCenter() + player.camera.m_look_direction;
		return pos_in_front_of_player;
	}

	ERet CreateMissionExitIfNeeded(const V3f& mission_exit_pos)
	{
		FPSGame& game = FPSGame::Get();

		//
		if( game._mission_exit_mdl == nil )
		{
			NwModel *	mission_exit_mdl;

			//
#if GAME_CFG_WITH_SOUND	
			game.sound.PlayAudioClip(
				MakeAssetID("Flag_Pickup")
				);
#endif // #if GAME_CFG_WITH_SOUND

			//
			NwModel_::PhysObjParams	phys_obj_params;
			phys_obj_params.is_static = true;

			//
			mxDO(NwModel_::Create(
				mission_exit_mdl

				, MakeAssetID("mission_exit")
				, game.runtime_clump

				, M44f::createUniformScaling(2.0f)
					//* M44_RotationX(DEG2RAD(90))
				, *game.world.render_world

				, game.world.physics_world
				, PHYS_OBJ_MISSION_EXIT

				, M44_RotationX(DEG2RAD(90)) * M44f::createTranslation(mission_exit_pos)
				, NwModel_::COL_SPHERE

				, CV3f(0)
				, CV3f(0)

				, phys_obj_params
				));

			//
			btRigidBody& bt_rb = mission_exit_mdl->rigid_body->bt_rb();
			btRigidBody_::DisableGravity(bt_rb);
			bt_rb.setCollisionFlags(0);

			//
			game._mission_exit_mdl = mission_exit_mdl;
		}

		return ALL_OK;
	}

}//namespace MyLevelData_


void FPSGame::_SpawnItemsAndMonstersForCurrentWave()
{
	const EAttackWave curr_attack_wave = (EAttackWave) BIT(_curr_attack_wave);

	// 2) Spawn monsters and items.
	nwFOR_EACH(const MyLevelData::NpcSpawnPoint& npc_spawn_point, _loaded_level.npc_spawn_points)
	{
		if(npc_spawn_point.waves & curr_attack_wave)
		{
			const int num_to_create = npc_spawn_point.count;

			if( num_to_create == 1 )
			{
				EnemyNPC	*	new_npc = nil;

				MyGameUtils::SpawnMonster(
					npc_spawn_point.pos
					, npc_spawn_point.type
					, &new_npc
					);
				if(new_npc) {
					AI_Fun::TurnTowardsPointInstantly(*new_npc, player.GetEyePosition());
				}
			}
			else if(num_to_create > 0)
			{
				const int approx_dim1D = (int) sqrtf((float)npc_spawn_point.count);
				//
				const float spacing = 4.0f;
				const float perturb = spacing * 0.5f;
				const float size = spacing * approx_dim1D;
				const float radius = size * 0.5f;
				const V3f center_offs = CV3f(radius,radius,0);

				//
				idRandom	rng(curr_attack_wave ^ num_to_create);

				//
				int num_created = 0;

				for( int iX = 0; iX < approx_dim1D; iX++ )
				{
					for( int iY = 0; iY < approx_dim1D; iY++ )
					{
						CV3f	rnd_perturb(
							rng.RandomFloat(-perturb, +perturb),
							rng.RandomFloat(-perturb, +perturb),
							0
							);

						const V3f spawn_pos
							= npc_spawn_point.pos
							+ CV3f(iX * spacing, iY * spacing, 0)
							- center_offs
							+ rnd_perturb
							;

						//
						EnemyNPC	*	new_npc = nil;

						MyGameUtils::SpawnMonster(
							spawn_pos
							, npc_spawn_point.type
							, &new_npc
							);
						if(new_npc) {
							AI_Fun::TurnTowardsPointInstantly(*new_npc, player.GetEyePosition());
						}

						++num_created;
						if(num_created >= num_to_create) {
							goto L_End;
						}
					}
				}

L_End:
				;
			}
		}
	}

	nwFOR_EACH(const MyLevelData::ItemSpawnPoint& item_spawn_point, _loaded_level.item_spawn_points)
	{
		if(item_spawn_point.waves & curr_attack_wave)
		{
			MyGameUtils::SpawnItem(
				item_spawn_point.type
				, item_spawn_point.pos
				);
		}
	}
}

void FPSGame::EvaluateLevelLogicOnNpcKilled()
{
	enum {
		min_num_NPCs_to_consider_wave_done = 3
	};

	if(EnemyNPC::NumInstances() <= min_num_NPCs_to_consider_wave_done)
	{
		DBGOUT("Current attack wave is finished!");

		++_curr_attack_wave;

		//
		if(_curr_attack_wave >= _loaded_level.num_attack_waves)
		{
			DBGOUT("No more attack waves! Showing the exit...");
			// show exit!
			MyLevelData_::CreateMissionExitIfNeeded(_loaded_level.mission_exit_pos);
		}
		else
		{
			DBGOUT("Starting wave %u!", _curr_attack_wave);

			//
			_SpawnItemsAndMonstersForCurrentWave();
		}
	}
}

void FPSGame::OnPlayerReachedMissionExit()
{
	_player_reached_mission_exit = true;

	// prevent NPCs from attacking the player, etc.
	is_paused = true;
}
