//
#include "stdafx.h"
#pragma hdrstop

#include <stdint.h>

#include <Core/Util/Tweakable.h>
#include <Engine/Engine.h>	// NwTime
#include <Renderer/Core/MeshInstance.h>
#include <Renderer/Modules/Animation/AnimatedModel.h>

#include "FPSGame.h"
#include "game_player.h"
#include "game_settings.h"
#include "experimental/game_experimental.h"
#include "game_world/game_world.h"
#include "game_world/pickable_items.h"
#include "game_world/explosions.h"
#include "game_animation/game_animation.h"



#if GAME_CFG_WITH_PHYSICS

static bool HandleDynamicObject(
								const PHYS_OBJ_TYPE obj0_type
								, const btRigidBody* rigid_body0
								, btPersistentManifold* const& manifold
								, const PHYS_OBJ_TYPE obj2_type
								, FPSGame& game
								)
{
	switch(obj0_type)
	{
	case PHYS_OBJ_GIB_ITEM:
		game.world.gibs.PlayMeatSplatSound(
			fromBulletVec(rigid_body0->getWorldTransform().getOrigin())
			);
		return true;

	case PHYS_OBJ_ROCKET:
		// ignore collisions with the player
		if(obj2_type != PHYS_OBJ_PLAYER_CHARACTER)
		{

			((NwRigidBody*)rigid_body0)->rocket_did_collide = true;
#if 0
			int numContacts = manifold->getNumContacts()
			btManifoldPoint& contact_point0 = manifold->getContactPoint(0);
#endif
		}

		return true;
	}

	return false;
}

static void MyContactStartedCallback(
									 btPersistentManifold* const& manifold
									 )
{
	const btCollisionObject* colObj0 = manifold->getBody0();
	const btCollisionObject* colObj1 = manifold->getBody1();
	const PHYS_OBJ_TYPE obj0_type = (PHYS_OBJ_TYPE) colObj0->getUserIndex();
	const PHYS_OBJ_TYPE obj1_type = (PHYS_OBJ_TYPE) colObj1->getUserIndex();

	//
	const btRigidBody* rigid_body0 = btRigidBody::upcast(colObj0);
	const btRigidBody* rigid_body1 = btRigidBody::upcast(colObj1);

	//
	FPSGame& game = FPSGame::Get();

	if(rigid_body0 && HandleDynamicObject(obj0_type, rigid_body0, manifold, obj1_type, game))
	{
		return;
	}
	if(rigid_body1 && HandleDynamicObject(obj1_type, rigid_body1, manifold, obj0_type, game))
	{
		return;
	}






	if(rigid_body0 && rigid_body1)
	{
		if(obj0_type == PHYS_OBJ_PLAYER_CHARACTER && obj1_type == PHYS_PICKABLE_ITEM)
		{
			if(MyGameUtils::PickItemIfShould(
				colObj1->getUserIndex2()
				))
			{
				game.world.RequestDeleteModel(
					((NwRigidBody*)rigid_body1)->model
					);
			}
			return;
		}

		if(obj0_type == PHYS_PICKABLE_ITEM && obj1_type == PHYS_OBJ_PLAYER_CHARACTER)
		{
			if(MyGameUtils::PickItemIfShould(
				colObj0->getUserIndex2()
				))
			{
				game.world.RequestDeleteModel(
					((NwRigidBody*)rigid_body0)->model
					);
			}
			return;
		}

		if(obj0_type == PHYS_OBJ_PLAYER_CHARACTER && obj1_type == PHYS_OBJ_MISSION_EXIT)
		{
			game.OnPlayerReachedMissionExit();
		}
		if(obj0_type == PHYS_OBJ_MISSION_EXIT && obj1_type == PHYS_OBJ_PLAYER_CHARACTER)
		{
			game.OnPlayerReachedMissionExit();
		}

	}//if both are rigid bodies
}

static bool MyContactAddedCallback(
								   btManifoldPoint& cp,
								   const btCollisionObjectWrapper* colObj0Wrap,
								   int partId0, int index0,
								   const btCollisionObjectWrapper* colObj1Wrap,
								   int partId1, int index1
								   )
{
	const btCollisionObject* colObj0 = colObj0Wrap->m_collisionObject;
	const btCollisionObject* colObj1 = colObj1Wrap->m_collisionObject;

	//const bool are_gib_items_involed_in_collision
	//	= (colObj0->getUserIndex() == PHYS_OBJ_GIB_ITEM)
	//	| (colObj1->getUserIndex() == PHYS_OBJ_GIB_ITEM)
	//	;

UNDONE;
	//FPSGame& game = FPSGame::Get();

	//if(colObj0->getUserIndex() == PHYS_OBJ_GIB_ITEM)
	//{
	//	game.world.gibs.PlayMeatSplatSound(
	//		fromBulletVec(colObj0->getWorldTransform().getOrigin())
	//		);
	//}
	//if(colObj1->getUserIndex() == PHYS_OBJ_GIB_ITEM)
	//{
	//	game.world.gibs.PlayMeatSplatSound(
	//		fromBulletVec(colObj1->getWorldTransform().getOrigin())
	//		);
	//}


	////
	//NwRigidBody* my_rigid_body0 = nil;
	//NwRigidBody* my_rigid_body1 = nil;

	////
	//if(btRigidBody* bt_rigid_body0 = (btRigidBody*) btRigidBody::upcast(colObj0))
	//{
	//	if(NwRigidBody* my_rigid_body0 = (NwRigidBody*) bt_rigid_body0->getUserPointer())
	//	{
	//		//my_rigid_body0
	//	}
	//}

	// accept this contact
	return true;
}

static void MyInternalTickCallback(
								   btDynamicsWorld* world,
								   btScalar timeStep
								   )
{
#if 0
	btRigidBody* player_char_ctrl_rb = FPSGame::Get().player.phys_char_controller.getRigidBody();

	//
	btDispatcher* coll_dispatcher = world->getDispatcher();
	btPersistentManifold** manifolds_pptr = coll_dispatcher->getInternalManifoldPointer();
	const int num_manifolds = coll_dispatcher->getNumManifolds();

	for (int i = 0; i < num_manifolds; i++)
	{
		btPersistentManifold* manifold = manifolds_pptr[i];

		const btCollisionObject* colObj0 = static_cast<const btCollisionObject*>(manifold->getBody0());
		const btCollisionObject* colObj1 = static_cast<const btCollisionObject*>(manifold->getBody1());

		const bool touching_player_char_ctrl = (player_char_ctrl_rb == colObj0) || (player_char_ctrl_rb == colObj1);

		//
		if(btRigidBody* bt_rigid_body = (btRigidBody*) btRigidBody::upcast(colObj0))
		{
			if(NwRigidBody* my_rigid_body = (NwRigidBody*) bt_rigid_body->getUserPointer())
			{
				if(EnemyNPC* enemy_npc = my_rigid_body->npc._ptr)
				{
					enemy_npc->is_touching_player_body = touching_player_char_ctrl;
				}
			}
		}
	}
#endif
}

ERet MyGameWorld::_InitPhysics(
							   NwClump * storage_clump 
							   , TbPrimitiveBatcher & debug_renderer
							   )
{
	mxDO(physics_world.Initialize(
		storage_clump
		));

	//
	physics_world.m_dynamicsWorld.setInternalTickCallback(
		&MyInternalTickCallback
		, nil
		);

	gContactStartedCallback = &MyContactStartedCallback;
	gContactAddedCallback = &MyContactAddedCallback;

	//
	physics_world.m_dynamicsWorld.setDebugDrawer(&_physics_debug_drawer);

	_physics_debug_drawer.init(&debug_renderer);

	//
	//Physics_AddGroundPlane_for_Collision();

	return ALL_OK;
}

void MyGameWorld::_DeinitPhysics()
{
	physics_world.Shutdown(*_scene_clump);
}


static
V3f ComputePlayerInputAcceleration(const MyGamePlayer& game_player)
{
	CV3f vInputAcceleration(0);
	if( game_player.phys_movement_flags & PlayerMovementFlags::FORWARD ) {
		vInputAcceleration += game_player.camera.m_look_direction;
	}
	if( game_player.phys_movement_flags & PlayerMovementFlags::BACKWARD ) {
		vInputAcceleration -= game_player.camera.m_look_direction;
	}
	if( game_player.phys_movement_flags & PlayerMovementFlags::LEFT ) {
		vInputAcceleration -= game_player.camera.m_rightDirection;
	}
	if( game_player.phys_movement_flags & PlayerMovementFlags::RIGHT ) {
		vInputAcceleration += game_player.camera.m_rightDirection;
	}

	return vInputAcceleration.normalized();
}


void MyGameWorld::_UpdatePhysics(
	const NwTime& game_time
	, MyGamePlayer& game_player
	, const MyGameSettings& game_settings
	)
{
	//
	game_player.phys_char_controller.preStep(game_time.real.delta_seconds);

	//
	physics_world.Update(
		game_time.real.delta_seconds
		);

	//
	game_player.phys_char_controller.postStep();


	// Clamp player velocity (e.g. to prevent NPCs from pushing the player too fast).
	{
		nwTWEAKABLE_CONST(float, PLAYER_MAX_VELOCITY, 20);

		btRigidBody& player_char_ctrl_rb = *game_player.phys_char_controller.getRigidBody();
		const btVector3	linear_velocity = player_char_ctrl_rb.getLinearVelocity();
		const btScalar player_velocity_mag = linear_velocity.length();
		if(player_velocity_mag > PLAYER_MAX_VELOCITY)
		{
			DBGOUT("Clamping player velocity!!! was %f", player_velocity_mag);

			const btVector3	new_linear_velocity = linear_velocity.normalized() * PLAYER_MAX_VELOCITY;

			player_char_ctrl_rb.setLinearVelocity(new_linear_velocity);
		}
	}


	//
	const bool enable_player_collisions = !game_settings.cheats.enable_noclip;
	const bool enable_player_gravity = !game_settings.cheats.disable_gravity;

	game_player.phys_char_controller.setCollisionsEnabled(
		enable_player_collisions
		);

	game_player.phys_char_controller.setGravityEnabled(
		enable_player_gravity
		);

	//
	const float DBG_PLAYER_FLYING_SPEED = 10.0f;

	//
	if(enable_player_collisions)
	{
		V3f player_movement_accel_3d = ComputePlayerInputAcceleration(game_player);

		//
		if(enable_player_gravity)
		{
			const bool player_is_on_ground = game_player.phys_char_controller.onGround();
			const bool player_can_climb = game_player.phys_char_controller.CanClimb();
			const bool player_is_moving = game_player.phys_char_controller.getLinearVelocity().length2() > 1e-3f;

			// don't allow vertical movement
			player_movement_accel_3d.z = 0;

			//
			const V2f player_movement_accel
				= V2f(player_movement_accel_3d.x, player_movement_accel_3d.y)
				* game_settings.player_movement.move_impulse
				;

			game_player.phys_char_controller.setMovementXZ(
				player_movement_accel
				);

			//
			if(
				(player_is_on_ground
				||
				player_can_climb)

				&& V2_Length(player_movement_accel) > 0
				&& player_is_moving)
			{
				game_player.foot_cycle -= game_time.real.delta_seconds;

				if(game_player.foot_cycle <= 0)
				{
					// weapon swaying
					if(const NwPlayerWeapon* curr_weapon = game_player._current_weapon._ptr)
					{
						const NwWeaponDef& weapon_def = *curr_weapon->def;

						const V3f horiz_accel
							= game_player.camera.m_rightDirection * (game_player.left_foot ? -1.0f : +1.0f)
							* weapon_def.horiz_sway
							;
						const V3f vert_accel
							= game_player.camera.m_upDirection * 0.2f
							* weapon_def.vert_sway
							;

						game_player.weapon_sway.accel.Add(
							horiz_accel - vert_accel,
							game_time.real.total_msec
							);
					}

					// play footstep sound

#if GAME_CFG_WITH_SOUND
					FPSGame::Get().sound.PlayAudioClip(
						MakeAssetID(
						game_player.left_foot ? "walk_rain_1" : "walk_rain_2"
						)
						);
#endif // GAME_CFG_WITH_SOUND

					// use another foot
					game_player.left_foot ^= 1;

					game_player.foot_cycle = game_settings.player_movement.run_foot_cycle_interval;
				}
			}

			//
			if(!player_is_moving)
			{
				game_player._ResetFootCycle();
			}

			//
			if( game_player.phys_movement_flags & PlayerMovementFlags::JUMP )
			{
				const bool did_jump = game_player.phys_char_controller.jump(
					toBulletVec(V3_UP),
					game_settings.player_movement.jump_impulse
					);

				if(did_jump)
				{
					game_player.weapon_sway.accel.Add(
						-V3_UP,
						game_time.real.total_msec
						);
#if GAME_CFG_WITH_SOUND
					FPSGame::Get().sound.PlayAudioClip(
						//MakeAssetID("player_jump")
						MakeAssetID("jump_1")
						);
#endif // GAME_CFG_WITH_SOUND


				}
			}//If player pressed "jump".

			//
			const bool just_landed
				= player_is_on_ground
				&& game_player.was_in_air
				;

			if( just_landed )
			{
				DBGOUT("LANDED!");
				game_player.weapon_sway.accel.Add(
					V3_UP,
					game_time.real.total_msec
					);
			}

			game_player.was_in_air = !player_is_on_ground;
		}
		else
		{
			game_player.phys_char_controller.getRigidBody()->setLinearVelocity(
				toBulletVec(player_movement_accel_3d * DBG_PLAYER_FLYING_SPEED)
				);
		}
	}
	else
	{
		// no collisions

		if(game_player.camera.m_movementFlags)
		{
			game_player.phys_char_controller.getRigidBody()->applyCentralImpulse(
				toBulletVec(game_player.camera.m_linearVelocity * DBG_PLAYER_FLYING_SPEED)
				);
		}
	}


	// prevent "sticky" keys
	game_player.phys_movement_flags.raw_value = 0;


	//
	if(enable_player_collisions)
	{
		V3f new_pos_f = game_player.phys_char_controller.getPos();
		//LogStream(LL_Warn),"new_pos_f: ",new_pos_f;

		V3d new_pos = V3d::fromXYZ(new_pos_f);
		
		//
		game_player.SetCameraPosition(new_pos);
	}
	else
	{
		// no collision detection

		if(1)
		{
			const V3d	camera_position_in_world_space = game_player.camera._eye_position;

			const V3f cam_pos_f = V3f::fromXYZ(camera_position_in_world_space);

			game_player.phys_char_controller.setPos(cam_pos_f);

			//
			btTransform	t;
			t.setIdentity();
			t.setOrigin(toBulletVec(cam_pos_f));
			game_player.phys_char_controller.getRigidBody()->setCenterOfMassTransform(t);

			game_player.phys_char_controller.getRigidBody()->setLinearVelocity(
				btVector3(0,0,0)
				);
		}
	}
}

void MyGameWorld::Physics_AddGroundPlane_for_Collision()
{
	btStaticPlaneShape* plane_shape = new btStaticPlaneShape(
		toBulletVec(V3_UP), 0
		);

	btCollisionObject* col_obj = new btCollisionObject();
	col_obj->setCollisionShape(plane_shape);
	col_obj->setFriction(0.5f);
	col_obj->setRollingFriction(0.5f);
	col_obj->setSpinningFriction(0.5f);

	physics_world.m_dynamicsWorld.addCollisionObject(
		col_obj
		);
}

#endif // GAME_CFG_WITH_PHYSICS
