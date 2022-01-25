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
//
#include <Renderer/Renderer.h>
#include <Renderer/Modules/Animation/SkinnedMesh.h>
#include <Renderer/Modules/idTech4/idMaterial.h>
#include <Renderer/Modules/Animation/AnimatedModel.h>
#include <Renderer/Modules/idTech4/idEntity.h>
//
#include <Utility/GameFramework/TbPlayerWeapon.h>

//
//#include "RCCpp_WeaponBehavior.h"
#include "experimental/rccpp.h"
#include "experimental/game_experimental.h"
#include "weapons/game_player_weapons.h"
#include "npc/game_nps.h"
#include "npc/npc_behavior_common.h"
#include "FPSGame.h"


static void DbgPrintRbLookVec(const btRigidBody& bt_rigid_body)
{
	btVector3 localLook = toBulletVec(MD5_MODEL_FORWARD); // X Axis
	btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis
	//
	btTransform transform = bt_rigid_body.getCenterOfMassTransform();
	btQuaternion currentOrientation = transform.getRotation();
	btVector3 currentLook = transform.getBasis() * localLook;

	//
	DBGOUT("RB look dir: x = %f, y = %f, z = %f"
		, currentLook.x(), currentLook.y(), currentLook.z()
		);
}

//=====================================================================


namespace EnemyNPC_
{

	V3f GetCoM(const EnemyNPC& npc)
	{
		NwRigidBody &	rigid_body = *npc.rigid_body;
		btRigidBody &	bt_rigid_body = rigid_body.bt_rb();

		const btTransform& transform = bt_rigid_body.getCenterOfMassTransform();
		return fromBulletVec(transform.getOrigin());
	}

	V3f GetLookDir(const EnemyNPC& npc)
	{
		NwRigidBody &	rigid_body = *npc.rigid_body;
		btRigidBody &	bt_rigid_body = rigid_body.bt_rb();

		const btTransform& transform = bt_rigid_body.getCenterOfMassTransform();
		btQuaternion currentOrientation = transform.getRotation();

		//
		btVector3 localLook = toBulletVec(MD5_MODEL_FORWARD); // X Axis
		btVector3 currentLook = transform.getBasis() * localLook;

		return fromBulletVec(currentLook).normalized();
	}

}//namespace EnemyNPC_



AI_Senses::AI_Senses()
{
	is_seeing_player = false;
	is_player_alive = false;

	did_receive_damage = false;
}

void AI_Senses::Setup(
					  EnemyNPC & npc
					  , AI_Blackboard & blackboard
					  , NwPhysicsWorld& physics_world
					  )
{
	if( blackboard.is_player_alive )
	{
		this->is_seeing_player = AI_Fun::Check_IsSeeingThePlayerThruWorld(
			npc
			, blackboard
			, physics_world
			);
	}
	else
	{
		this->is_seeing_player = false;
	}

	this->is_player_alive = blackboard.is_player_alive;
}


namespace AI_Fun
{
	void TurnTowardsPointInstantly(
		EnemyNPC & npc
		, const V3f& target_pos_ws
		)
	{
		//
		NwRigidBody &	rigid_body = *npc.rigid_body;
		btRigidBody &	bt_rigid_body = rigid_body.bt_rb();
		//
		const btTransform curr_rb_transform = bt_rigid_body.getCenterOfMassTransform();
		const btQuaternion current_rb_orientation = curr_rb_transform.getRotation();

		//
		const V3f currPos = fromBulletVec( curr_rb_transform.getOrigin() );

		V3f targetDir = ( target_pos_ws - currPos );
		const float distToTargetPos = targetDir.normalize();
		(void)distToTargetPos;

		//
		btVector3 localLook = toBulletVec(MD5_MODEL_FORWARD); // X Axis
		btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis

		V3f targetDirAlongHorizPlane = targetDir;
		targetDirAlongHorizPlane.z = 0;
		targetDirAlongHorizPlane.normalize();



		////
		//dbgLineDrawer.DrawLine(
		//					   fromBulletVec(bt_rb_initial_transform.getOrigin()),
		//					   fromBulletVec(bt_rb_initial_transform.getOrigin() + currentLook * distToTargetPos)
		//					   );

		const btVector3 currentLook = curr_rb_transform.getBasis() * localLook;
		btScalar angle = currentLook.angle(toBulletVec(targetDirAlongHorizPlane));

		btQuaternion desiredOrientation( rotationAxis, angle );
	
		//
		btTransform	desired_rb_transform(
			desiredOrientation,
			curr_rb_transform.getOrigin()
			);
		bt_rigid_body.setCenterOfMassTransform(desired_rb_transform);
	}

	CoTaskStatus TurnTowardsPoint(
		EnemyNPC & npc
		, const V3f& target_pos_ws
		, const NwTime& game_time
		, const float turning_speed_degrees_per_seconds
		)
	{
		//
		NwRigidBody &	rigid_body = *npc.rigid_body;
		btRigidBody &	bt_rigid_body = rigid_body.bt_rb();

		const V3f currPos = fromBulletVec( bt_rigid_body.getCenterOfMassPosition() );

		V3f targetDir = ( target_pos_ws - currPos );
		const float distToTargetPos = targetDir.normalize();
		(void)distToTargetPos;

		//

		btVector3 localLook = toBulletVec(MD5_MODEL_FORWARD); // X Axis
		btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis

		V3f targetDirAlongHorizPlane = targetDir;
		targetDirAlongHorizPlane.z = 0;
		targetDirAlongHorizPlane.normalize();

		//
		btTransform transform = bt_rigid_body.getCenterOfMassTransform();
		btQuaternion currentOrientation = transform.getRotation();
		btVector3 currentLook = transform.getBasis() * localLook;

		////
		//dbgLineDrawer.DrawLine(
		//					   fromBulletVec(bt_rb_initial_transform.getOrigin()),
		//					   fromBulletVec(bt_rb_initial_transform.getOrigin() + currentLook * distToTargetPos)
		//					   );

		F32 minimumCorrectionAngle = DEG2RAD(1);

		btScalar angle = currentLook.angle(toBulletVec(targetDirAlongHorizPlane));


		btQuaternion desiredOrientation( rotationAxis, angle );

		btQuaternion inverseCurrentOrientation = currentOrientation.inverse();

		if( mmAbs(angle) > minimumCorrectionAngle )
		{
			// https://stackoverflow.com/a/11022610
			V3f crossProd = V3f::cross( fromBulletVec(currentLook), targetDirAlongHorizPlane );	// right-hand rule

			// shortest arc angle
			F32 signedAngle = angle * signf(crossProd.z);

			btVector3 angularVelocity = (
				mxHACK("don't multiplay by delta time, otherwise, NPCs turn too slowly (esp. in release mode)")
				signedAngle * turning_speed_degrees_per_seconds //* game_time.real.delta_seconds
				) * rotationAxis;
			bt_rigid_body.setAngularVelocity(angularVelocity);

			//LogStream(LL_Warn),"Delta Angle: ",angle,", DEGS: ",RAD2DEG(angle),", signedAngle=",signedAngle;

			// still rotating towards the target
			return Running;
		}
		else
		{
			// already facing the target

			//		LogStream(LL_Warn),"Delta Angle == 0";
			bt_rigid_body.setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));

			return Completed;
		}
	}

	bool HasReachedTargetPosition(
		const EnemyNPC& npc
		, const V3f& target_pos_ws
		)
	{
		const AABBf npc_aabb_ws = npc.rigid_body->GetAabbWorld();

		V3f target_pos_ws_at_NPC_level = target_pos_ws;
		target_pos_ws_at_NPC_level.z = npc_aabb_ws.min_corner.z;

		const bool	has_reached_goal_pos = npc_aabb_ws
			.expanded(CV3f(0.5f))
			.containsPoint( target_pos_ws_at_NPC_level )
			;

		return has_reached_goal_pos;
	}

	bool IsTouchingPlayer(
		const EnemyNPC& npc
		)
	{
		//
		AI_Blackboard &ai_blackboard = AI_Blackboard::Get();

		const btBroadphaseProxy* npb_rigid_body_bp_proxy_bt = npc.rigid_body->bt_rb().getBroadphaseProxy();

UNDONE;
return false;
		//return TestAabbAgainstAabb2(
		//	npb_rigid_body_bp_proxy_bt->m_aabbMin, npb_rigid_body_bp_proxy_bt->m_aabbMax,
		//	ai_blackboard.player_aabb_min_with_margin, ai_blackboard.player_aabb_max_with_margin
		//	);

	}

	ERet PlayOneShotAnimIfNeeded(
						 const NameHash32 anim_id
						 , EnemyNPC & npc
						 )
	{
		const NwAnimClip* anim_clip = npc.anim_model->_skinned_mesh->findAnimByNameHash(anim_id);
		mxENSURE(anim_clip, ERR_OBJECT_NOT_FOUND, "");

		npc.anim_model->anim_player.PlayAnimIfNotPlayingAlready(
			anim_clip
			, NwPlayAnimParams()
			, PlayAnimFlags::DoNotRestart
			);
		return ALL_OK;
	}



	bool Check_IsSeeingThePlayerThruWorld(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, NwPhysicsWorld& physics_world
		)
	{
		if(!_Check_IsPlayerInFrontOfNPC(npc, blackboard)) {
			return false;
		}
		return _Check_CanSeeThePlayerThruWorldNow_RaycastSlow(
			npc
			, blackboard
			, physics_world
			);
	}

	bool _Check_IsPointInFrontOfNPC(
		EnemyNPC & npc
		, const V3f& pos_ws
		, const float min_dp
		)
	{
		const V3f pos_relative_to_NPC
			= pos_ws - EnemyNPC_::GetCoM(npc)
			;
		const float dotp = EnemyNPC_::GetLookDir(npc) * pos_relative_to_NPC;

		return dotp > min_dp;
	}

	bool _Check_IsPlayerInFrontOfNPC(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		)
	{
		return _Check_IsPointInFrontOfNPC(npc, blackboard.player_pos);
	}

	bool _Check_CanSeeThePlayerThruWorldNow_RaycastSlow(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, NwPhysicsWorld& physics_world
		)
	{
		//
		const btVector3	bt_ray_start = toBulletVec(EnemyNPC_::GetCoM(npc));
		//const btVector3	bt_ray_end
		//	= bt_ray_start + toBulletVec(EnemyNPC_::GetLookDir(npc) * obstacle_avoidance_close_dist)
		//	;
		const btVector3	bt_ray_end
			= toBulletVec(blackboard.player_pos)
			;

		const btVector3 ZERO_VECTOR(0.0f, 0.0f, 0.0f);

		mxOPTIMIZE("batched raycasts");
		ClosestNotMeRayResultCallback	ray_result_callback(
			bt_ray_start, bt_ray_end,
			&npc.rigid_body->bt_rb()
			);

		physics_world.m_dynamicsWorld.rayTest(
			bt_ray_start,
			bt_ray_end,
			ray_result_callback
			);
		
		return
			// did not hit anything
			!ray_result_callback.m_collisionObject
			// or hit the player
			|| ray_result_callback.m_collisionObject == blackboard.player_collision_object
			;
	}

}//AI_Fun



#if 0
/*
-----------------------------------------------------------------------------
	EntityAI
-----------------------------------------------------------------------------
*/
EntityAI::EntityAI()
{
	is_targeting_player = false;
	last_seen_player_position = CV3f(0);

	num_remaining_times_to_attack = 3;

	health = 0;
}


///
#define $CoRET_CURR		default: return zz__state; } }



CoTaskStatus EntityAI::Think(
	EnemyNPC & npc
	, const V3f& player_pos
	, const NwTime& game_time
	, const CoTimestamp& clock
	)
{
	$CoBEGIN;
	{
		//
		//if(!HasReachedGivenPosition(
		//	npc
		//	, player_pos
		//	))
		//{
		//	PlayRunAnimIfNeeded(
		//		npc
		//		);
		//}

		////
		//$CoWAIT_UNTIL(_RotateToFacePlayer(
		//	npc
		//	, player_pos
		//	, game_time
		//	));
		//$CoYIELD;
		//
		//
		$CoWAIT_UNTIL(_MoveToPosition(
			npc
			, player_pos
			, game_time
			));
		//$CoYIELD;



		//
		$CoWAIT_WHILE(_AttackPlayer(
			npc
			, player_pos
			, game_time
			));


		//if(is_targeting_player)
		//{
		//	//DBGOUT("is_targeting_player");
		//	//
		//}
		//else
		//{
		//	$CoYIELD;
		//	_CheckIfSeesPlayer(npc, player_pos, clock);
		//}

		//$CoWAIT_WHILE(weapon.anim_model->IsPlayingAnim(idle_anim_name_hash));
	}

	// Never completes.
	zz__state = Initial;
	$CoRET_CURR;

	// Never completes.
	//this->Restart();
}

void EntityAI::PlayRunAnimIfNeeded(
	const EnemyNPC& npc
	)
{
	npc.anim_model->anim_player.PlayAnimIfNotPlayingAlready(
		npc.anims.run
		, NwPlayAnimParams()
		, PlayAnimFlags::Looped
		);
}

void EntityAI::PlayAttackAnimIfNeeded(
									  const EnemyNPC& npc
									  )
{
	if(npc.anim_model->anim_player.PlayAnimIfNotPlayingAlready(
		npc.anims.attack
		))
	{
//		DBGOUT("ATTACK");
		--num_remaining_times_to_attack;
	}	
}

CoTaskStatus EntityAI::_CheckIfSeesPlayer(
	EnemyNPC & npc
	, const V3f& player_pos
	, const CoTimestamp& clock
	)
{
	$CoBEGIN;

	$CoEND;
}

bool EntityAI::_RotateToFacePlayer(
	EnemyNPC & npc
	, const V3f& player_pos
	, const NwTime& game_time
	)
{
	return _TurnTowardsPoint(
		npc
		, player_pos
		, game_time
		);
}

bool EntityAI::_TurnTowardsPoint(
	EnemyNPC & npc
	, const V3f& target_pos_ws
	, const NwTime& game_time
	)
{
	//const NwAnimClip* run_anim = npc.anim_model->_skinned_mesh->findAnimByNameHash(
	//	mxHASH_STR("run")
	//	);
	//mxENSURE(run_anim, false, "");


	//
	NwRigidBody &	rigid_body = *npc.rigid_body;
	btRigidBody &	bt_rigid_body = rigid_body.bt_rb();

	const V3f currPos = fromBulletVec( bt_rigid_body.getCenterOfMassPosition() );

	V3f targetDir = ( target_pos_ws - currPos );
	const float distToTargetPos = targetDir.normalize();

	//

	btVector3 localLook = toBulletVec(MD5_MODEL_FORWARD); // X Axis
	btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis

	V3f targetDirAlongHorizPlane = targetDir;
	targetDirAlongHorizPlane.z = 0;
	targetDirAlongHorizPlane.normalize();

	//
	btTransform transform = bt_rigid_body.getCenterOfMassTransform();
	btQuaternion currentOrientation = transform.getRotation();
	btVector3 currentLook = transform.getBasis() * localLook;

	////
	//dbgLineDrawer.DrawLine(
	//					   fromBulletVec(bt_rb_initial_transform.getOrigin()),
	//					   fromBulletVec(bt_rb_initial_transform.getOrigin() + currentLook * distToTargetPos)
	//					   );

	F32 minimumCorrectionAngle = DEG2RAD(1);

	btScalar angle = currentLook.angle(toBulletVec(targetDirAlongHorizPlane));


	btQuaternion desiredOrientation( rotationAxis, angle );

	btQuaternion inverseCurrentOrientation = currentOrientation.inverse();

	btScalar turning_speed_degrees_per_seconds = 128.0f; // 

	if( mmAbs(angle) > minimumCorrectionAngle )
	{
		// https://stackoverflow.com/a/11022610
		V3f crossProd = V3f::cross( fromBulletVec(currentLook), targetDirAlongHorizPlane );	// right-hand rule

		// shortest arc angle
		F32 signedAngle = angle * signf(crossProd.z);

		btVector3 angularVelocity = (signedAngle * turning_speed_degrees_per_seconds * game_time.real.delta_seconds) * rotationAxis;
		bt_rigid_body.setAngularVelocity(angularVelocity);

		//LogStream(LL_Warn),"Delta Angle: ",angle,", DEGS: ",RAD2DEG(angle),", signedAngle=",signedAngle;

		// still rotating towards the target
		return false;

	}
	else
	{
		// already facing the target

		//		LogStream(LL_Warn),"Delta Angle == 0";
		bt_rigid_body.setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));

		return true;
	}
}


bool EntityAI::HasReachedGivenPosition(
	const EnemyNPC& npc
	, const V3f& target_pos_ws
	)
{
	//const AABBf npc_aabb_ws = npc.rigid_body->GetAabbWorld();

	//const bool	has_reached_goal_pos = npc_aabb_ws
	//	.expanded(CV3f(0.5f))
	//	.containsPoint( target_pos_ws )
	//	;

	//return has_reached_goal_pos;

	return Check_IsTouchingPlayer(npc);
}


bool EntityAI::Check_IsTouchingPlayer(
	const EnemyNPC& npc
	)
{
	//
	AI_Blackboard &ai_blackboard = AI_Blackboard::Get();

	const btBroadphaseProxy* npb_rigid_body_bp_proxy_bt = npc.rigid_body->bt_rb().getBroadphaseProxy();

	return TestAabbAgainstAabb2(
		npb_rigid_body_bp_proxy_bt->m_aabbMin, npb_rigid_body_bp_proxy_bt->m_aabbMax,
		ai_blackboard.player_aabb_min_with_margin, ai_blackboard.player_aabb_max_with_margin
		);
}

//bool fade_out_was_called = false;


bool EntityAI::_MoveToPosition(
	EnemyNPC & npc
	, const V3f& target_pos_ws
	, const NwTime& game_time
	)
{
	//
	const bool	has_reached_goal_pos = HasReachedGivenPosition(
		npc
		, target_pos_ws
		);

	if( has_reached_goal_pos )
	{
		//if(!fade_out_was_called)
		//{
		//	fade_out_was_called = true;

			npc.anim_model->anim_player.FadeOutAnim(
				npc.anims.run
				, 500	// avoid abrupt transition
				);
			//npc.anim_model->anim_player.DbgSimulate();
//		}

		// don't rotate if arrived at destination

		return true;
	}
	else
	{
		//
		PlayRunAnimIfNeeded(
			npc
			);

		// rotate only if not arrived at destination

		// stay on path
		_TurnTowardsPoint(
			npc
			, target_pos_ws
			, game_time
			);
	}


	//
	//fade_out_was_called = false;

	return false;
}

bool EntityAI::_AttackPlayer(
							 EnemyNPC & npc
							 , const V3f& player_pos
							 , const NwTime& game_time
							 )
{
	if(!num_remaining_times_to_attack) {
		return false;
	}

	if(HasReachedGivenPosition(
		npc
		, player_pos
		))
	{
		//
		PlayAttackAnimIfNeeded(
			npc
			);
		return npc.anim_model-> anim_player.IsPlayingAnim(npc.anims.attack);
	}

//	DBGOUT("RESET ATTACK");
	num_remaining_times_to_attack = 3;

	return false;
}
#endif

CoTaskStatus Task_MoveToPosition::Run(
	EnemyNPC & npc
	, const V3f& target_pos_ws

	, const NameHash32 run_anim_id

	, const NwTime& game_time
	, const CoTimestamp& now
	)
{
	const bool	has_reached_goal_pos = AI_Fun::HasReachedTargetPosition(
		npc
		, target_pos_ws
		);

	if( !has_reached_goal_pos )
	{
		this->Restart();
	}

	$CoBEGIN;

	if( has_reached_goal_pos )
	{
		DBGOUT("Fading out RUN anim...");

		npc.anim_model->anim_player.FadeOutAnim(
			npc.anims.run
			, 500	// avoid abrupt transition
			);

		$CoWAIT_WHILE(npc.anim_model->IsPlayingAnim(
			run_anim_id
			));

		$EXIT;
	}
	else
	{
		// rotate only if not arrived at destination

		// stay on path

		// NOTE: don't stop moving when turning!
		//$CoWAIT_WHILE(
		//	TurnTowardsPoint(npc, target_pos_ws, game_time) == Running
		//	);
		AI_Fun::TurnTowardsPoint(
			npc
			, target_pos_ws
			, game_time
			, 60.0f
			);

		//
		AI_Fun::PlayOneShotAnimIfNeeded(
			//mxHASH_STR("walk1")	// BAD: periodically stops when walking
			run_anim_id
			, npc
			);
	}

	// Never completes.
	this->Restart();
	$CoRETURN;
}

CoTaskStatus Task_AttackPlayer::Run(
	EnemyNPC & npc
	, const V3f& target_pos_ws
	
	, const NameHash32 attack_anim_id

	, const NwTime& game_time
	, const CoTimestamp& now
	)
{
	$CoBEGIN;

	$CoWAIT_WHILE(AI_Fun::TurnTowardsPoint(npc, target_pos_ws, game_time)==Running);

	// the player might have moved while we were turning
	//$CoWAIT_UNTIL(AI_Fun::IsTouchingPlayer(npc));

	if(AI_Fun::IsTouchingPlayer(npc))
	{
		//
		AI_Fun::PlayOneShotAnimIfNeeded(
			attack_anim_id
			, npc
			);

		$CoWAIT_WHILE(npc.anim_model->IsPlayingAnim(
			attack_anim_id
			));
	}


	// Never completes.
	this->Restart();
	$CoRETURN;
}

/*
-------------------------------------------------------------------------------
	AI_Brain_Generic_Melee_NPC
-------------------------------------------------------------------------------
*/
mxDEFINE_CLASS(AI_Brain_Generic_Melee_NPC);

AI_Brain_Generic_Melee_NPC::AI_Brain_Generic_Melee_NPC()
{
	config.run_anim_id = mxHASH_STR("run");

	ai_state = NPC_AI_IDLE;

	last_known_player_pos = CV3f(0);
	my_health = 0;

	//
	mxHACK("so that enemies turn towards the player");
	ai_state = NPC_AI_CHASING_PLAYER;
	last_known_player_pos = FPSGame::Get().player.GetEyePosition();
}

CoTaskStatus AI_Brain_Generic_Melee_NPC::Think(
	EnemyNPC & npc
	, const AI_Senses& senses
	, AI_Blackboard & blackboard
	, const NwTime& game_time
	, const CoTimestamp now
	)
{
	if( senses.did_receive_damage ) {
		ai_state = NPC_AI_ALERTED;
	}

	if( senses.is_seeing_player )
	{
		ai_state = NPC_AI_CHASING_PLAYER;
		last_known_player_pos = blackboard.player_pos;
	}

	//
	$CoBEGIN;

	if(ai_state != NPC_AI_IDLE)
	{
#if 0
		$CoWAIT_WHILE(TurnTowardsPoint(
			npc
			, last_known_player_pos
			, game_time
			) == Running
			);

		$CoWAIT_WHILE(task_greet_player.Run(
			npc
			, now
			) == Running
			);
#endif
		// decide if we must chase the player
		ai_state = ( blackboard.is_player_alive )
			? NPC_AI_CHASING_PLAYER
			: NPC_AI_IDLE
			;

		//
		if(ai_state == NPC_AI_CHASING_PLAYER)
		{
			$CoWAIT_WHILE(task_move_to_pos.Run(
				npc
				, last_known_player_pos

				, config.run_anim_id

				, game_time
				, now
				) == Running);

			//
			if(AI_Fun::IsTouchingPlayer(npc)
				&& AI_Fun::TurnTowardsPoint(
					npc,
					blackboard.player_pos,
					game_time
				) == Completed)
			{
				$CoWAIT_WHILE(task_attack_player.Run(
					npc
					, blackboard.player_pos

					, mxHASH_STR("melee_attack1")

					, game_time
					, now
					) == Running);
			}
			else
			{
				$CoYIELD;
			}
		}
	}

	// Never completes.
	this->Restart();
	$CoRETURN;
}
