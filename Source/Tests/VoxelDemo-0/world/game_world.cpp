//
#include "stdafx.h"
#pragma hdrstop

#include <stdint.h>

#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>

#include <Engine/Model.h>
#include <Engine/Engine.h>	// NwTime

//#include <BlobTree/BlobTree.h>

#include "app.h"
#include "game_player.h"
#include "app_settings.h"
#include "experimental/game_experimental.h"
#include "world/game_world.h"


/*
-----------------------------------------------------------------------------
	MyWorld
-----------------------------------------------------------------------------
*/
MyWorld::MyWorld( AllocatorI& allocator )
	: _allocator( allocator )
	, models_to_delete(allocator)

#if !GAME_CFG_WITH_PHYSICS
	, physics_world( *(NwPhysicsWorld*)nil )
#endif

{
	//_fast_noise_scale = 1;
	Arrays::initializeWithExternalStorage(
		models_to_delete,
		models_to_delete_embedded_storage
		);
}

ERet MyWorld::Initialize(
							  const MyAppSettings& app_settings
							  , NwClump* scene_clump
							  , AllocatorI& scratchpad
							  )
{
	_scene_clump = scene_clump;

	//
	spatial_database = Rendering::SpatialDatabaseI::CreateSimpleDatabase( _allocator );

	//
#if GAME_CFG_WITH_PHYSICS

	mxDO(_InitPhysics(
		scene_clump
		, render_system->_debug_renderer
		));

#endif // GAME_CFG_WITH_PHYSICS

	return ALL_OK;
}

void MyWorld::Shutdown()
{
	// collisions/physics must be closed after voxels!!!
#if GAME_CFG_WITH_PHYSICS

	_DeinitPhysics();

#endif // GAME_CFG_WITH_PHYSICS

	//
	Rendering::SpatialDatabaseI::Destroy(spatial_database);
	spatial_database = nil;

	//
	_scene_clump = nil;
}


void MyWorld::UpdateOncePerFrame(
							  const NwTime& game_time
							  , MyGamePlayer& game_player
							  , const MyAppSettings& app_settings
							  , MyApp& game
							  )
{
	_DeletePendingModels();

	const V3d	player_camera_position = game_player.camera._eye_position;





	//// NOTE: advance animations before physics
	//// to avoid unsightly "teleporting" when using root motion!
	//if( !game.is_paused )
	//{
	//	NwAnimatedModel_::UpdateInstances(
	//		*_scene_clump
	//		, game_time
	//		, NwJobScheduler_Serial::s_instance
	//		);
	//}

	//

#if GAME_CFG_WITH_PHYSICS

	if( !game.is_paused )
	{
		_UpdatePhysics(
			game_time
			, game_player
			, app_settings
			);
	}

#endif // GAME_CFG_WITH_PHYSICS

	////
	//spatial_database->UpdateOncePerFrame(
	//	player_camera_position
	//	, game_time.real.delta_seconds
	//	, app_settings.renderer
	//	);
}





void MyWorld::RequestDeleteModel(NwModel* model_to_delete)
{
	mxASSERT(models_to_delete.num() < models_to_delete.capacity());
	models_to_delete.add(model_to_delete);
}

void MyWorld::_DeletePendingModels()
{
	if(models_to_delete.IsEmpty()) {
		return;
	}
	std::sort(models_to_delete.begin(), models_to_delete.end());
	Arrays::RemoveDupesFromSortedArray(models_to_delete);

	nwFOR_EACH(NwModel* model_to_delete, models_to_delete)
	{
		NwModel_::Delete(
			model_to_delete
			, *_scene_clump
			, *spatial_database
			, physics_world
			);
	}

	models_to_delete.RemoveAll();
}
