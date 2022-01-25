#pragma once

#include <Physics/Bullet_Wrapper.h>


struct idRenderModel;
class EnemyNPC;
struct NwModel;


enum PHYS_OBJ_TYPE
{
	PHYS_OBJ_UNKNOWN,

	/// player character controller
	PHYS_OBJ_PLAYER_CHARACTER,

	/// for playing gib sounds
	PHYS_OBJ_GIB_ITEM,

	/// e.g. medkit, ammo box, valuable item, etc.
	PHYS_PICKABLE_ITEM,

	/// a missile
	PHYS_OBJ_ROCKET,

	/// the mission is ended when the player is walking into this area
	PHYS_OBJ_MISSION_EXIT,
};



mxALIGN_BY_CACHELINE struct NwRigidBody: CStruct
{
	// store rigid bodies together for better cache usage;
	// collision shapes are stored separately and shared as much as possible
	//std::aligned_storage< sizeof(btRigidBody), 16 >	_rigid_body_storage;	// is bugged on MVC++2008
	char	_rigid_body_storage[ sizeof(btRigidBody) ];	// sizeof(btRigidBody) = 704

	mxPREALIGN(16) char	_col_shape_buf[ COLSHAPE_UNION_SIZE ];	// 96

	// HACK HACK HACK:
	TPtr< idRenderModel >	render_model;
	TPtr< EnemyNPC >	npc;
	TPtr< NwModel >		model;

	// PHYS_OBJ_ROCKET
	bool	rocket_did_collide;

	// 32: 896

public:
	enum { ALLOC_GRANULARITY = 128 };
	mxDECLARE_CLASS( NwRigidBody, CStruct );
	mxDECLARE_REFLECTION;

	NwRigidBody();

	mxFORCEINLINE btRigidBody & bt_rb()
	{ return *(btRigidBody*)_rigid_body_storage; }

	mxFORCEINLINE const btRigidBody& bt_rb() const
	{ return *(btRigidBody*)_rigid_body_storage; }


	mxFORCEINLINE btCollisionShape& bt_ColShape()
	{ return *(btCollisionShape*)_col_shape_buf; }

	mxFORCEINLINE const btCollisionShape& bt_ColShape() const
	{ return *(btCollisionShape*)_col_shape_buf; }

public:
	const AABBf GetAabbWorld() const;

	const V3f GetCenterWorld() const;
};


namespace btRigidBody_
{

	void DisableGravity(btRigidBody& bt_rb);

}//namespace btRigidBody_
