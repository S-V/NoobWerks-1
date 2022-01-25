//
#include "stdafx.h"
#pragma hdrstop

#include <Core/ObjectModel/Clump.h>
//
#include <Renderer/Modules/Animation/AnimatedModel.h>
#include <Renderer/Modules/idTech4/idEntity.h>

#include "experimental/rccpp.h"	// g_SystemTable
#include "game_animation/game_animation.h"
#include "game_rendering/game_renderer.h"
#include "game_world/game_world.h"
#include "npc/game_nps.h"
#include "FPSGame.h"


mxBEGIN_REFLECT_ENUM( MonsterType8 )
	mxREFLECT_ENUM_ITEM( Imp, MONSTER_IMP ),
	mxREFLECT_ENUM_ITEM( Trite, MONSTER_TRITE ),
	mxREFLECT_ENUM_ITEM( Maggot, MONSTER_MAGGOT ),
	mxREFLECT_ENUM_ITEM( Mancubus, MONSTER_MANCUBUS ),
mxEND_REFLECT_ENUM
//const char* g_monster_def_file_by_type[MONSTER_TYPE_COUNT] = {
//	"monster_demon_imp",
//	"monster_demon_trite",
//	"monster_demon_mancubus"
//};

/*
-----------------------------------------------------------------------------
	HealthComponent
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(HealthComponent);
mxBEGIN_REFLECTION(HealthComponent)
mxEND_REFLECTION;

HealthComponent::HealthComponent()
	//: EntityComponent( HealthComponent::metaClass() )
{
	_health = 100;
}

HealthComponent::~HealthComponent()
{

}

void HealthComponent::DealDamage( int damage_points )
{
	_health -= damage_points;
}

/*
-----------------------------------------------------------------------------
	EnemyNPC
-----------------------------------------------------------------------------
*/
//tbPRINT_SIZE_OF(EnemyNPC);

mxDEFINE_CLASS(EnemyNPC);
mxBEGIN_REFLECTION(EnemyNPC)
	//mxMEMBER_FIELD( camera ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;
EnemyNPC::EnemyNPC()
{
	monster_type = MONSTER_TYPE_COUNT;	// must be set later

	is_driven_by_animation = true;
	is_touching_ground = false;
	previous_root_position = CV3f(0);
}

ERet MyGameNPCs::Init(
					  NwClump* scene_clump
					  )
{
	_clump = scene_clump;

	return ALL_OK;
}

void MyGameNPCs::Deinit(
						MyGameWorld& game_world
						)
{
	//
	{
		NwClump::Iterator< NwRigidBody >	rigid_body_iterator( *_clump );
		while( rigid_body_iterator.isValid() )
		{
			NwRigidBody & rigid_body = rigid_body_iterator.Value();

			game_world.physics_world.m_dynamicsWorld.removeRigidBody(
				&rigid_body.bt_rb()
				);

			rigid_body_iterator.MoveToNext();
		}
	}

	_clump = nil;
}

void MyGameNPCs::RTCPP_OnConstructorsAdded()
{
	NwClump::Iterator< EnemyNPC >	npc_iterator( *_clump );
	while( npc_iterator.isValid() )
	{
		EnemyNPC & npc = npc_iterator.Value();

		//
		npc.behavior->DeleteDataForNPC(npc);
		npc.behavior = g_SystemTable.npc_behaviors[ npc.monster_type ];
		npc.behavior->CreateDataForNPC(npc);

		npc_iterator.MoveToNext();
	}
}

static
void UpdateGraphicsTransformFromPhysics(
										const btRigidBody& bt_rigid_body
										, idRenderModel& render_model
										, const V3f& current_root_joint_pos
										, ARenderWorld* render_world
										//, const NwTime& game_time
										)
{
	const btTransform& bt_rb_xform = bt_rigid_body.getCenterOfMassTransform();
	const M44f	rigid_body_local_to_world_matrix = M44f_from_btTransform( bt_rb_xform );

	//
	const AABBf	aabb_in_model_space = render_model.mesh->local_aabb;

	// the pivot of the monster's model is around its origin, and not around the bounding
	// box's origin, so we have to compensate for this when the model is offset so that
	// the monster still appears to rotate around it's origin.

	V3f	gfx_center_in_world_space = rigid_body_local_to_world_matrix.translation();
	gfx_center_in_world_space.z -= aabb_in_model_space.halfSize().z;

	const AABBf	aabb_in_world_space = aabb_in_model_space
		.shifted(gfx_center_in_world_space);
	//
	render_world->updateEntityAabbWorld(
		render_model.world_proxy_handle,
		AABBd::fromOther(aabb_in_world_space)
		);

	//
#if 1 // works!
	
	M44f	gfx_local_to_world_matrix = rigid_body_local_to_world_matrix;
	gfx_local_to_world_matrix.setTranslation(
		gfx_center_in_world_space
		);
	
	M44f m2 = M44f::createTranslation(-current_root_joint_pos);

	render_model.local_to_world = m2 * gfx_local_to_world_matrix;

#else // not working

	M44f	gfx_local_to_world_matrix = rigid_body_local_to_world_matrix;
	gfx_local_to_world_matrix.setTranslation(
		gfx_center_in_world_space-current_root_joint_pos
		);
	render_model.local_to_world = gfx_local_to_world_matrix;

#endif
}

static
V3f TransformVelocityFromModelToWorldSpace(
	const V3f& velocity_in_model_space
	, const btRigidBody& bt_rigid_body
	)
{
	const M44f bt_rb_local_to_world = M44f_from_btTransform(bt_rigid_body.getCenterOfMassTransform());
	//
	float	velocity_mag;
	const V3f normalized_velocity_direction_in_model_space = V3_Normalized(velocity_in_model_space, velocity_mag);

	return V3_Normalized(M44_TransformNormal(
		bt_rb_local_to_world
		, normalized_velocity_direction_in_model_space
		)) * velocity_mag
		;
}

void MyGameNPCs::UpdateGraphicsTransformsFromAnimationsAndPhysics(
	const NwTime& game_time
	, ARenderWorld* render_world
	, NwPhysicsWorld& physics_world
	)
{
	NwClump::Iterator< EnemyNPC >	npc_iterator( *_clump );
	while( npc_iterator.isValid() )
	{
		EnemyNPC& npc = npc_iterator.Value();

		//
		NwRigidBody & rigid_body = *npc.rigid_body;
		btRigidBody& bt_rigid_body = rigid_body.bt_rb();

		const NwAnimPlayer& anim_player = npc.anim_model->anim_player;
		const NwAnimationJobResult& anim_job_result = npc.anim_model->job_result;

		const V3f current_root_joint_pos = anim_job_result.blended_root_joint_pos_in_model_space;

		nwFOR_EACH(const NwAnimPlayer::AnimState& anim_state, anim_player._active_anims)
		{
			const NwPlaybackController& playback_ctrl = anim_state.playback_controller;
			if( playback_ctrl.didWrapAroundDuringLastUpdate() )
			{
				//NOTE: cannot just reset root offset - the looping transition will be jerky!
				//npc.previous_root_position = current_root_joint_pos;

				//mxASSERT(anim_state.current_blend_weight > 0);
				//if(anim_state.current_blend_weight > 0)
					npc.previous_root_position -= anim_state.anim->root_joint_pos_in_last_frame;
			}
		}

		// Update visual bt_rb_initial_transform.
		{
			mxOPTIMIZE("batched raycasts");
			btVector3 bt_ray_start = bt_rigid_body.getCenterOfMassPosition();
			btVector3 bt_ray_end = bt_ray_start - btVector3(0, 0, 2.0f);	// In our coord system, Z is UP.

			ClosestNotMeRayResultCallback	ray_result_callback(
				bt_ray_start, bt_ray_end
				, &bt_rigid_body
				);

			physics_world.m_dynamicsWorld.rayTest(
				bt_ray_start, bt_ray_end, ray_result_callback
				);
			npc.is_touching_ground = ray_result_callback.hasHit();

			//
			if(npc.is_driven_by_animation && npc.is_touching_ground)
			{
				// applying velocity is much smoother than changing the rigid body's position directly
				const V3f root_joint_delta_pos_in_model_space
					//= anim_state0.current_blend_weight >= 1.0f
					//? current_root_joint_pos - npc.previous_root_position
					//: npc.previous_root_position - current_root_joint_pos
					//;
					= current_root_joint_pos - npc.previous_root_position
					;

				const V3f root_joint_velocity_in_model_space = root_joint_delta_pos_in_model_space * (1.0f / game_time.real.delta_seconds);

				const V3f root_joint_velocity_in_world_space = TransformVelocityFromModelToWorldSpace(
					root_joint_velocity_in_model_space
					, bt_rigid_body
					);

				////if(root_joint_delta_pos_in_model_space.x < -0.1f)
				//if(root_joint_delta_pos_in_model_space.x < 0)
				//{
				//	DEVOUT("Applying velocity: %f, %f, %f, curr_root_pos: %f, %f, %f, prev_root_pos: %f, %f, %f, num anims: %d",
				//		//root_joint_velocity_in_world_space.x, root_joint_velocity_in_world_space.y, root_joint_velocity_in_world_space.z,
				//		root_joint_delta_pos_in_model_space.x, root_joint_delta_pos_in_model_space.y, root_joint_delta_pos_in_model_space.z,
				//		current_root_joint_pos.x, current_root_joint_pos.y, current_root_joint_pos.z,
				//		npc.previous_root_position.x, npc.previous_root_position.y, npc.previous_root_position.z,
				//		anim_player._active_anims.num()
				//		);

				//	//DEVOUT("Anim[0]: weight: %f", anim_player._active_anims._data[0].)
				//}

				mxHACK("TODO: FIX ROOT MOTION CAUSING THE MODEL TO TELEPORT BACK WHEN THE ANIM IS FADING OUT");
				mxHACK("TO AVOID TELEPORTING, WE APPLY VELOCITY ONLY IF THE ANIM IS MOVING FORWARD");
				const bool anim_is_moving_forward = root_joint_delta_pos_in_model_space.x > 0;	// in Doom 3, forward movement = X axis.
				if( anim_is_moving_forward )
				{
					//
					btVector3	desired_velocity = toBulletVec(root_joint_velocity_in_world_space);

					desired_velocity += physics_world.m_dynamicsWorld.getGravity() * game_time.real.delta_seconds;

					bt_rigid_body.setLinearVelocity(
						desired_velocity
						);
				}
				else
				{
					// prevent foot sliding, when anim is "idle", but the body's center is moving
					bt_rigid_body.setLinearVelocity(toBulletVec(CV3f(0)));
				}
			}//If anim driven

			//
			UpdateGraphicsTransformFromPhysics(
				bt_rigid_body
				, *rigid_body.render_model
				, current_root_joint_pos
				, render_world
				);

			//
			npc.previous_root_position = current_root_joint_pos;
		}

		//
		npc_iterator.MoveToNext();
	}//For each EnemyNPC.
}

void MyGameNPCs::TickAI(
	const NwTime& game_time
	, const V3f& player_pos
	, const CubeMLf& world_bounds
	)
{
	const CoTimestamp	now = CoTimestamp::GetRealTimeNow();

	//
	NwClump::Iterator< EnemyNPC >	npc_iterator( *_clump );
	while( npc_iterator.isValid() )
	{
		EnemyNPC & npc = npc_iterator.Value();

		const bool npc_is_inside_world_bounds
			= world_bounds.containsPoint( npc.rigid_body->GetCenterWorld() )
			;

		if(npc_is_inside_world_bounds)
		{
			// Update animations.
			//EnemyNPC_::UpdateWalkingAnim(
			//	npc
			//	, game_time
			//	);

			npc.behavior->Think(
				npc
				, blackboard
				, game_time
				, now
				);
		}
		else
		{
			DBGOUT("Deleting NPC '%s' falling outside the world!",
				MonsterType8_Type().GetString(npc.monster_type)
				);
			DeleteNPC(npc);
		}

		npc_iterator.MoveToNext();
	}
}

void MyGameNPCs::DebugDrawAI(
	const NwTime& game_time
	) const
{
	//
}

ERet MyGameNPCs::Spawn_Spider(
							  EnemyNPC *&new_npc_
							  , MyGameWorld& game_world
							  , const V3f& position
							  )
{
	EnemyNPC *	new_npc;
	//
	mxDO(_SpawnMonster(
		new_npc
		, game_world
		, position
		, MONSTER_TRITE
		, TResPtr<NwSkinnedMesh>(MakeAssetID("monster_demon_trite"))
		));
	//
	{
		const NwSkinnedMesh& skel_mesh = *new_npc->anim_model->_skinned_mesh;

		StandardAnims &std_anims = new_npc->anims;

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.run._ptr,
			mxHASH_STR("run")
			));
		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.attack._ptr,
			mxHASH_STR("melee_attack1")	// jump
			));
		//melee_attack2

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.pain._ptr,
			mxHASH_STR("pain")
			));
		// pain_head
		// pain_left_arm
		// pain_right_arm
	}
	new_npc_ = new_npc;
	return ALL_OK;
}

ERet MyGameNPCs::Spawn_Imp(
	EnemyNPC *&new_npc_
	, MyGameWorld& game_world
	, const V3f& position
	)
{
	EnemyNPC *	new_npc;

	//
	mxDO(_SpawnMonster(
		new_npc
		, game_world
		, position
		, MONSTER_IMP
		, TResPtr<NwSkinnedMesh>(MakeAssetID("monster_demon_imp"))
		));
	//
	{
		const NwSkinnedMesh& skel_mesh = *new_npc->anim_model->_skinned_mesh;

		StandardAnims &std_anims = new_npc->anims;

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.run._ptr,
			mxHASH_STR("run")
			));
		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.attack._ptr,
			mxHASH_STR("melee_attack1")
			));

		//melee_attack2
	}

	new_npc_ = new_npc;
	return ALL_OK;
}

ERet MyGameNPCs::Spawn_Mancubus(
	EnemyNPC *&new_npc_
	, MyGameWorld& game_world
	, const V3f& position
	)
{
	EnemyNPC *	new_npc;

	//
	mxDO(_SpawnMonster(
		new_npc
		, game_world
		, position
		, MONSTER_MANCUBUS
		, TResPtr<NwSkinnedMesh>(MakeAssetID("monster_demon_mancubus"))
		));
	//
	{
		const NwSkinnedMesh& skel_mesh = *new_npc->anim_model->_skinned_mesh;

		StandardAnims &std_anims = new_npc->anims;

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.run._ptr,
			mxHASH_STR("walk")
			));
		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.attack._ptr,
			mxHASH_STR("melee_attack1")
			));

		//melee_attack2

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.pain._ptr,
			mxHASH_STR("pain")
			));
	}

	new_npc_ = new_npc;
	return ALL_OK;
}

ERet MyGameNPCs::Spawn_Maggot(
	EnemyNPC *&new_npc_
	, MyGameWorld& game_world
	, const V3f& position
	)
{
	EnemyNPC *	new_npc;

	//
	mxDO(_SpawnMonster(
		new_npc
		, game_world
		, position
		, MONSTER_MAGGOT
		, TResPtr<NwSkinnedMesh>(MakeAssetID("monster_demon_maggot"))
		));
	//
	{
		const NwSkinnedMesh& skel_mesh = *new_npc->anim_model->_skinned_mesh;

		StandardAnims &std_anims = new_npc->anims;

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.run._ptr,
			mxHASH_STR("run")
			));
		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.attack._ptr,
			mxHASH_STR("melee_attack1")
			));

		//melee_attack2

		mxDO(skel_mesh.GetAnimByNameHash(
			std_anims.pain._ptr,
			mxHASH_STR("pain")
			));
	}

	new_npc_ = new_npc;
	return ALL_OK;
}

ERet MyGameNPCs::_SpawnMonster(
								   EnemyNPC *&new_npc_
								   , MyGameWorld& game_world
								   , const V3f& position
								   , const EMonsterType monster_type
								   , TResPtr<NwSkinnedMesh> skinned_mesh
								   )
{
	//
	mxDO(skinned_mesh.Load(_clump));

	TResPtr< idEntityDef >	entity_def(skinned_mesh._id);
	mxDO(entity_def.Load(_clump));


	//
	NwRigidBody* new_rigid_body;
	mxDO(_clump->New(new_rigid_body, NwRigidBody::ALLOC_GRANULARITY));


	//
	const V3f bindPoseLocalAabbSize = skinned_mesh->bind_pose_aabb.size();
	const V3f bindPoseLocalAabbHalfSize = bindPoseLocalAabbSize * 0.5f;


	// Create rigid body (physics).

	const float collision_cylinder_height_ws = bindPoseLocalAabbHalfSize.z;

	//
#if 1 // Use cylinder so that the character's legs are on the floor.

	// Use the min radius so that legs are sticking a bit out of the collision cylinder.
	const float collision_cylinder_radius_ws = Min3(
		bindPoseLocalAabbHalfSize.x,
		bindPoseLocalAabbHalfSize.y,
		bindPoseLocalAabbHalfSize.z
		);

	btCollisionShape* col_shape = new(new_rigid_body->_col_shape_buf) btCylinderShapeZ(
		toBulletVec(
		CV3f(
		collision_cylinder_radius_ws,
		collision_cylinder_radius_ws,
		collision_cylinder_height_ws
		)
		)
		);
#else

	const float collision_cylinder_radius_ws = Max3(
		bindPoseLocalAabbHalfSize.x,
		bindPoseLocalAabbHalfSize.y,
		bindPoseLocalAabbHalfSize.z
		);

	const btScalar radius = collision_cylinder_radius_ws;
	const btScalar height = collision_cylinder_height_ws - collision_cylinder_radius_ws*0.5f;
	mxASSERT(height > 0);

	btCollisionShape* col_shape = new(new_rigid_body->_col_shape_buf) btCapsuleShape(
		radius,
		height
		);
#endif

	//
	float _mass = bindPoseLocalAabbSize.boxVolume() * 80.0f;

	V3f _initialPosition = position;	// could be stuck in the ground, if 'initial_position' is zero vector
	_initialPosition += CV3f(0,0, collision_cylinder_height_ws * 0.5f);	// standing on the ground

	V3f _initialVelocity = CV3f(0);




	//
	//
	btVector3	localInertia;
	col_shape->calculateLocalInertia(_mass, localInertia);

	btRigidBody* bt_rigid_body = new(new_rigid_body->_rigid_body_storage) btRigidBody(
		_mass,
		nil,
		col_shape,
		localInertia
		);

	{
		btTransform bt_rb_initial_transform;
		bt_rb_initial_transform.setIdentity();
		bt_rb_initial_transform.setOrigin(btVector3( _initialPosition.x, _initialPosition.y, _initialPosition.z ));

		bt_rigid_body->setCenterOfMassTransform( bt_rb_initial_transform );

		bt_rigid_body->setLinearVelocity(btVector3(_initialVelocity.x, _initialVelocity.y, _initialVelocity.z));
	}

	//

	bt_rigid_body->setFriction(0.5f);
	bt_rigid_body->setRestitution(0.5f);

	// objects fall too slowly with high linear damping
	bt_rigid_body->setDamping( 1e-2f, 1e-4f );


	// NOTE: disable rotation - is solves many problems caused by physics <-> graphics desync.
#if 0
	// prevent the character from toppling over - restrict the rigid body to rotate only about the UP axis (Z)
	bt_rigid_body->setAngularFactor(btVector3(0.0f, 0.0f, 1.0f));
#else
	bt_rigid_body->setAngularFactor(btVector3(0,0,0)); // No rotation allowed. We only rotate the visuals.
#endif

	// never sleep!
	bt_rigid_body->setActivationState(DISABLE_DEACTIVATION);


	game_world.physics_world.m_dynamicsWorld.addRigidBody(
		bt_rigid_body
	);

	//
	new_rigid_body->bt_rb().setUserPointer(new_rigid_body);
	//NwRigidBody* my_rigid_body = (NwRigidBody*) new_rigid_body->bt_rb().getUserPointer();


	//
	NwAnimatedModel *	new_anim_model;
	mxDO(NwAnimatedModel_::Create(
		new_anim_model
		, *_clump
		, skinned_mesh
		, &MyGame::ProcessAnimEventCallback
		, entity_def
		));


	// start anims at different time so that they don't look the same
	idRandom	rnd(
		(int)new_anim_model ^ (int)new_rigid_body
		);

	new_rigid_body->render_model = new_anim_model->render_model;

	//
	new_anim_model->render_model
		->local_to_world.setTranslation(position);


	//
	NwAnimatedModel_::AddToWorld(
		new_anim_model
		, game_world.render_world
		);

	//
	EnemyNPC *	new_npc;
	mxDO(_clump->New(new_npc, EnemyNPC::ALLOC_GRAN));

	new_npc->rigid_body = new_rigid_body;
	new_npc->anim_model = new_anim_model;
	new_anim_model->npc = new_npc;

	new_npc->monster_type = monster_type;

	//
	NPC_BehaviorI* ai_behavior = g_SystemTable.npc_behaviors[ monster_type ];
	new_npc->behavior = ai_behavior;
	new_npc->behavior->CreateDataForNPC(*new_npc);

	new_rigid_body->npc = new_npc;

	//
	new_npc_ = new_npc;


	// Always play the "idle" anim.
	mxDO(new_anim_model->PlayAnim(
		mxHASH_STR("idle")
		, NwPlayAnimParams().SetPriority(NwAnimPriority::Lowest)
		, PlayAnimFlags::Looped
		, NwStartAnimParams().SetStartTimeRatio(rnd.RandomFloat())
		));


	//
	const NwSkinnedMesh& skel_mesh = *new_anim_model->_skinned_mesh;

	//new_npc->anim_fsm.idle_anim = skel_mesh.findAnimByNameHash(mxHASH_STR("idle"));
	//new_npc->anim_fsm.walk_anim = skel_mesh.findAnimByNameHash(mxHASH_STR("walk2"));
	//new_npc->anim_fsm.run_anim = skel_mesh.findAnimByNameHash(mxHASH_STR("run"));

	//{
	//	StandardAnims &std_anims = new_npc->anims;

	//	mxDO(skel_mesh.GetAnimByNameHash(
	//		std_anims.run._ptr,
	//		mxHASH_STR("run")
	//		));
	//	mxDO(skel_mesh.GetAnimByNameHash(
	//		std_anims.attack._ptr,
	//		mxHASH_STR("melee_attack1")
	//		));

	//	//melee_attack2
	//}

	return ALL_OK;
}

void MyGameNPCs::KillNPC(EnemyNPC & npc)
{
	DeleteNPC(npc);

	//
	FPSGame& game = FPSGame::Get();
	game.EvaluateLevelLogicOnNpcKilled();
}

void MyGameNPCs::DeleteNPC(EnemyNPC & npc)
{
	FPSGame& game = FPSGame::Get();

	npc.behavior->DeleteDataForNPC(npc);

	game.world.physics_world.DeleteRigidBody(npc.rigid_body);

	//
	NwAnimatedModel_::RemoveFromWorld(
		npc.anim_model,
		game.world.render_world
		);

	NwAnimatedModel_::Destroy(
		npc.anim_model._ptr,
		game.runtime_clump
		);

	//
	_clump->delete_(&npc);
}

void MyGameNPCs::DeleteAllNPCs()
{
	int num_npcs_deleted = 0;

	NwClump::Iterator< EnemyNPC >	npc_iterator( *_clump );
	while( npc_iterator.isValid() )
	{
		EnemyNPC & npc = npc_iterator.Value();

		this->DeleteNPC(npc);
		++num_npcs_deleted;

		npc_iterator.MoveToNext();
	}

	DBGOUT("Deleted %d NPCs.", num_npcs_deleted);
}


namespace MyGameUtils
{
	ERet SpawnMonster(
		const V3f& position
		, const EMonsterType monster_type
		, EnemyNPC **new_npc_ /*= nil*/
		)
	{
		FPSGame& game = FPSGame::Get();

		//
		EnemyNPC *	new_npc = nil;

		switch(monster_type)
		{
		case MONSTER_IMP:
			game.world.NPCs.Spawn_Imp(
				new_npc
				, game.world, position
				);
			break;

		case MONSTER_TRITE:
			game.world.NPCs.Spawn_Spider(
				new_npc
				, game.world, position
				);
			break;

		case MONSTER_MAGGOT:
			game.world.NPCs.Spawn_Maggot(
				new_npc
				, game.world, position
				);
			break;

		case MONSTER_MANCUBUS:
			game.world.NPCs.Spawn_Mancubus(
				new_npc
				, game.world, position
				);
			break;

			mxNO_SWITCH_DEFAULT;
		}

		if(new_npc_)
		{
			*new_npc_ = new_npc;
		}

		return ALL_OK;
	}
}
