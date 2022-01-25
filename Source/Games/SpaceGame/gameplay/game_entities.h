#pragma once

#include <Utility/Camera/NwFlyingCamera.h>
#include <Developer/ModelEditor/ModelEditor.h>	// NwLocalTransform
#include "common.h"
#include "physics/physics_mgr.h"
#include "rendering/lod_models.h"


mxDECLARE_32BIT_HANDLE(SgShipHandle);


///
struct NwMeshSocket: CStruct
{
	NwLocalTransform	local_transform;
	NameHash32			name_hash;

public:
	mxDECLARE_CLASS(NwMeshSocket, CStruct);
	mxDECLARE_REFLECTION;
	NwMeshSocket();
};


///
struct SgSpaceshipSocket: CStruct
{
	V3f			local_position;
	NameHash32	name_hash;
	
	V3f			local_direction;
	U32			type;

public:
	mxDECLARE_CLASS(SgSpaceshipSocket, CStruct);
	mxDECLARE_REFLECTION;
	SgSpaceshipSocket();
};


struct SgShipDef: NwSharedResource
{
	// Visuals:

	TResPtr< NwMesh >	lod_meshes[MAX_MESH_LODS];


	//// AI:
	//float	ai_collision_radius;

	//
	int		health;


	//float	ai_attack_range;
	int		npc_damage;

	int		cannon_firerate;
	float	cannon_horiz_spread;
	float	cannon_vert_spread;



	// Physics:
	
	float	mass;

	/// all mesh vertices are in [-1..+1] range
	//float	model_scale;

	float	max_speed;

	float	linear_thrust_acceleration;
	float	angular_turn_acceleration;


	////
	//TFixedArray< SgSpaceshipSocket, 8 >	thrusters;
	//TFixedArray< SgSpaceshipSocket, 8 >	weapons;




public:
	mxDECLARE_CLASS(NwSharedResource, CStruct);
	mxDECLARE_REFLECTION;
	SgShipDef();

	ERet Load(
		const char* mesh_name
		, NwClump& storage_clump
		);
};


enum EGameObjType
{
	// NOTE: largest ships are placed first
	// so that they are rendered earlier than small ones

	//obj_spacestation,

	// friendlies:
	obj_freighter,
	obj_heavy_battleship_ship,
	//obj_capital_ship,
	obj_small_fighter_ship,
	obj_combat_fighter_ship,

	// enemies:
	obj_alien_fighter_ship,
	obj_alien_skorpion_ship,
	//obj_alien_spiky_spaceship,

	obj_type_count,
};


enum SpaceshipFlags
{
	fl_ship_is_my_enemy = BIT(0),

	fl_ship_is_mission_goal = BIT(1),
};

struct SgShip
{
	I32					flags;

	EGameObjType		type;

	SgShipHandle	my_handle;

	RigidBodyHandle		rigid_body_handle;

	//

	//
	U32			ship_ai_index;

	/// we will attack the spaceship with this handle:
	SgShipHandle	target_handle;

	///
	SgShipHandle	longterm_goal_handle;

	//
	U32			unused_padding;

	//32
};
ASSERT_SIZEOF(SgShip, 32);



struct SgProjectile
{
	V3f			pos;
	float01_t	life;	// 0..1, dead if > 1

	V3f				velocity;
	EGameObjType	ship_type;
};
ASSERT_SIZEOF(SgProjectile, 32);
