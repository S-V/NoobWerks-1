#pragma once

// std::aligned_storage
#include <type_traits>

#include <Core/Tasking/TinyCoroutines.h>

#include <Developer/RunTimeCompiledCpp.h>

#include "game_compile_config.h"
#include "game_forward_declarations.h"

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#endif // GAME_CFG_WITH_PHYSICS



/// Axis X
#define MD5_MODEL_FORWARD	CV3f(1,0,0)


namespace EnemyNPC_
{

	V3f GetCoM(const EnemyNPC& npc);
	V3f GetLookDir(const EnemyNPC& npc);

}//namespace EnemyNPC_

/// 3D Perception
struct AI_Senses
{
	bool	is_seeing_player;
	bool	is_player_alive;

	bool	did_receive_damage;

public:
	AI_Senses();

	void Setup(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, NwPhysicsWorld& physics_world
		);
};

enum NPC_AI_STATE
{
	NPC_AI_IDLE,

	/// has heard the player's sound or received damage
	NPC_AI_ALERTED,

	/// has seen the player and pursuing him
	NPC_AI_CHASING_PLAYER,
};



namespace AI_Fun
{
	void TurnTowardsPointInstantly(
		EnemyNPC & npc
		, const V3f& target_pos_ws
		);

	CoTaskStatus TurnTowardsPoint(
		EnemyNPC & npc
		, const V3f& target_pos_ws
		, const NwTime& game_time
		, const float turning_speed_degrees_per_seconds = 128.0f
		);

	bool HasReachedTargetPosition(
		const EnemyNPC& npc
		, const V3f& target_pos_ws
		);

	bool IsTouchingPlayer(
		const EnemyNPC& npc
		);

	ERet PlayOneShotAnimIfNeeded(
		const NameHash32 anim_id
		, EnemyNPC & npc
		);


	bool Check_IsSeeingThePlayerThruWorld(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, NwPhysicsWorld& physics_world
		);

	bool _Check_IsPointInFrontOfNPC(
		EnemyNPC & npc
		, const V3f& pos_ws
		, const float min_dp = 0
		);
	bool _Check_IsPlayerInFrontOfNPC(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		);

	bool _Check_CanSeeThePlayerThruWorldNow_RaycastSlow(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, NwPhysicsWorld& physics_world
		);

}//AI_Fun


struct Task_MoveToPosition: CoTask
{
	CoTaskStatus Run(
		EnemyNPC & npc
		, const V3f& target_pos_ws

		, const NameHash32 run_anim_id

		, const NwTime& game_time
		, const CoTimestamp& now
		);
};

struct Task_AttackPlayer: CoTask
{
	CoTaskStatus Run(
		EnemyNPC & npc
		, const V3f& target_pos_ws
		
		, const NameHash32 attack_anim_id

		, const NwTime& game_time
		, const CoTimestamp& now
		);
};


/*
-------------------------------------------------------------------------------
	AI_Brain_Generic_Melee_NPC
-------------------------------------------------------------------------------
*/
struct AI_Brain_Generic_Melee_NPC
	: CStruct
	, public CoTask
{
	// State
	NPC_AI_STATE	ai_state;
	V3f				last_known_player_pos;

	int				my_health;

	// Config
	struct Config
	{
		// mxHASH_STR("run") or mxHASH_STR("walk")
		NameHash32	run_anim_id;

	} config;

	// Tasks
	//Task_GreetPlayer		task_greet_player;
	Task_MoveToPosition		task_move_to_pos;
	Task_AttackPlayer		task_attack_player;

public:
	mxDECLARE_CLASS(AI_Brain_Generic_Melee_NPC, CStruct);

	AI_Brain_Generic_Melee_NPC();

	CoTaskStatus Think(
		EnemyNPC & npc
		, const AI_Senses& senses
		, AI_Blackboard & blackboard
		, const NwTime& game_time
		, const CoTimestamp now
		);
};
