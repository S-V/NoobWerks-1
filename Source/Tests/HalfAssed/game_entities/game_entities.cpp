//
#include "stdafx.h"
#pragma hdrstop

#include <Renderer/Core/Mesh.h>
#include <Renderer/Core/MeshInstance.h>
#include <Renderer/Renderer.h>

#include <Engine/Engine.h>	// NwTime
#include "game_entities/game_entities.h"


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

mxDEFINE_CLASS(GameSpaceship);
mxBEGIN_REFLECTION(GameSpaceship)
	mxMEMBER_FIELD( position_in_world_space ),
	mxMEMBER_FIELD( forward_dir ),
	mxMEMBER_FIELD( right_dir ),

	mxMEMBER_FIELD( mesh ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;
GameSpaceship::GameSpaceship()
{
	reset();
}

ERet GameSpaceship::load(
						 NwClump* storage_clump
						 , ARenderWorld* render_world
						 )
{
	mxDO(mesh.Load(storage_clump));

	//
 	M44f test_xfrom = M44f::createTranslation(CV3f(0, 0, 300));
	mxDO(RrMeshInstance::ñreateFromMesh(
		mesh_instance._ptr
		, mesh
		, *storage_clump
		, &test_xfrom
		));
	//mxDO(storage_clump->New(mesh_instance));
	//mesh_instance->setupFromMesh

	//
	AABBd	infinite_aabb;
	infinite_aabb.clear();

	render_world->addEntity(
		mesh_instance,
		RE_MeshInstance,
		AABBd::make(CV3d(-400), CV3d(+400))
		);

	return ALL_OK;
}

void GameSpaceship::unload()
{
	mesh.release();

	mesh_instance = nil;
}

void GameSpaceship::reset()
{
	position_in_world_space = V3d::zero();
	forward_dir = V3_FORWARD;
	right_dir = V3_RIGHT;

	mesh._setId(MakeAssetID("Spaceship"));
}

void GameSpaceship::update(
	const NwTime& args
	)
{
	//_camera.Update( args.real.delta_seconds );
}

float GameSpaceship::calc_debug_camera_speed() const
{
	UNDONE;
	return 0;
	//num_seconds_shift_is_held
}
