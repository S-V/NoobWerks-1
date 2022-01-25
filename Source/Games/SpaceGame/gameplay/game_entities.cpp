//
#include "stdafx.h"
#pragma hdrstop

#include <Engine/Engine.h>	// NwTime
#include "gameplay/game_entities.h"
#include "world/world.h"
#include "game_app.h"


mxDEFINE_CLASS(NwLocalTransform);
mxBEGIN_REFLECTION(NwLocalTransform)
	mxMEMBER_FIELD( translation ),
	mxMEMBER_FIELD( uniform_scaling ),
	mxMEMBER_FIELD( orientation ),
mxEND_REFLECTION;
NwLocalTransform::NwLocalTransform()
{
	translation = CV3f(0);
	uniform_scaling = 1;
	orientation.setIdentity();
}

M44f NwLocalTransform::toMat4() const
{
	return M44f::create(
		orientation.toMat3(),
		translation,
		uniform_scaling
		);
}

mxDEFINE_CLASS(NwMeshSocket);
mxBEGIN_REFLECTION(NwMeshSocket)
	mxMEMBER_FIELD( local_transform ),
	mxMEMBER_FIELD( name_hash ),
mxEND_REFLECTION;
NwMeshSocket::NwMeshSocket()
{
	name_hash = 0;
}


mxDEFINE_CLASS(SgSpaceshipSocket);
mxBEGIN_REFLECTION(SgSpaceshipSocket)
	mxMEMBER_FIELD( local_position ),
	mxMEMBER_FIELD( name_hash ),

	mxMEMBER_FIELD( local_direction ),
	mxMEMBER_FIELD( type ),
mxEND_REFLECTION;
SgSpaceshipSocket::SgSpaceshipSocket()
{
	local_position = CV3f(0);
	name_hash = 0;

	local_direction = V3_FORWARD;
	type = 0;
}


mxDEFINE_CLASS(SgShipDef);
mxBEGIN_REFLECTION(SgShipDef)
	mxMEMBER_FIELD( mass ),
	//mxMEMBER_FIELD( model_scale ),

	mxMEMBER_FIELD( max_speed ),
	mxMEMBER_FIELD( linear_thrust_acceleration ),
	mxMEMBER_FIELD( angular_turn_acceleration ),

	mxMEMBER_FIELD( lod_meshes ),
mxEND_REFLECTION;
SgShipDef::SgShipDef()
{
	health = 100;

	//ai_attack_range = 1;
	npc_damage = 30;

	cannon_firerate = 100;
	cannon_horiz_spread = 0.1f;
	cannon_vert_spread = 0;

	mass = 1;
	//model_scale = 1;

	max_speed = 10;

	linear_thrust_acceleration = 4;
	angular_turn_acceleration = 1;
}

ERet SgShipDef::Load(
						  const char* mesh_name
						  , NwClump& storage_clump
						  )
{
	for(int iLOD = 0; iLOD < mxCOUNT_OF(lod_meshes); iLOD++)
	{
		String256	lod_mesh_name;
		Str::Format(lod_mesh_name, "%s_%d", mesh_name, iLOD);
		//
		TResPtr< NwMesh >& lod_mesh_ptr = lod_meshes[ iLOD ];

		lod_mesh_ptr._setId(MakeAssetID(lod_mesh_name.c_str()));

		mxDO(lod_mesh_ptr.Load(&storage_clump));
	}

	//
	const float ship_density = 1.0f;
	mass = lod_meshes[0]->local_aabb.volume() * ship_density;

	return ALL_OK;
}


//tbPRINT_SIZE_OF(SgShip);
