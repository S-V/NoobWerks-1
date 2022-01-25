//
#include "stdafx.h"
#pragma hdrstop

#include <stdint.h>

#include <Core/Util/Tweakable.h>
#include <Engine/Engine.h>	// NwTime
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>

#include "game_app.h"
#include "human_player.h"
#include "game_settings.h"
#include "experimental/game_experimental.h"
#include "world/world.h"



#if GAME_CFG_WITH_PHYSICS

static bool HandleDynamicObject(
								const PHYS_OBJ_TYPE obj0_type
								, const btRigidBody* rigid_body0
								, btPersistentManifold* const& manifold
								, const PHYS_OBJ_TYPE obj2_type
								, SgGameApp& game
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
	SgGameApp& game = SgGameApp::Get();

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
	//SgGameApp& game = SgGameApp::Get();

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
	btRigidBody* player_char_ctrl_rb = SgGameApp::Get().player.phys_char_controller.getRigidBody();

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

ERet SgWorld::_InitPhysics(
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

void SgWorld::_DeinitPhysics()
{
	physics_world.Shutdown(*_scene_clump);
}

void SgWorld::_UpdatePhysics(
	const NwTime& game_time
	, SgHumanPlayer& game_player
	, const SgAppSettings& game_settings
	)
{
	physics_world.Update(
		game_time.real.delta_seconds
	);
}

void SgWorld::Physics_AddGroundPlane_for_Collision()
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
