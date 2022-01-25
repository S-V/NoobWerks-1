#pragma once

//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )


#include <Rendering/Public/Globals.h>	// LargePosition, Rendering::NwCameraView
#include <ProcGen/Noise/NwNoiseFunctions.h>
#include <Planets/PlanetsCommon.h>
#include <Planets/Noise.h>	// HybridMultiFractal

#include <VoxelsSupport/VoxelTerrainRenderer.h>

#include <Voxels/public/vx5.h>
#include <Voxels/extensions/lmdb_storage.h>	// ChunkDatabase_LMDB

//
#include "compile_config.h"

//
#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
//#include <Physics/Bullet_Wrapper.h>
#endif // GAME_CFG_WITH_PHYSICS

//
#include "common.h"
#include "experimental/game_experimental.h"


/*
===============================================================================
	VOXELS
===============================================================================
*/










/*
-----------------------------------------------------------------------------
	MyWorld
-----------------------------------------------------------------------------
*/
class MyWorld: NonCopyable
{
public:	// Rendering
	TPtr< Rendering::SpatialDatabaseI >		spatial_database;

public:	// Voxels.

public:	// Physics

#if GAME_CFG_WITH_PHYSICS

	NwPhysicsWorld				physics_world;
	TbBullePhysicsDebugDrawer	_physics_debug_drawer;

#else

	NwPhysicsWorld &			physics_world;

#endif // GAME_CFG_WITH_PHYSICS


public:	// Object storage
	//
	TPtr< NwClump >		_scene_clump;


	//
	DynamicArray<NwModel*>	models_to_delete;
	NwModel*				models_to_delete_embedded_storage[16];

	//
	AllocatorI &		_allocator;

public:
	MyWorld( AllocatorI& allocator );

	ERet Initialize(
		const MyAppSettings& app_settings
		, NwClump* scene_clump
		, AllocatorI& scratchpad
		);

	void Shutdown();

	void UpdateOncePerFrame(
		const NwTime& game_time
		, MyGamePlayer& game_player
		, const MyAppSettings& app_settings
		, MyApp& game
		);

public:
	void RequestDeleteModel(NwModel* model_to_delete);

private:
	void _DeletePendingModels();

private:	// Physics.

#if GAME_CFG_WITH_PHYSICS

	ERet _InitPhysics(
		NwClump * storage_clump 
		, TbPrimitiveBatcher & debug_renderer
		);
	void _DeinitPhysics();

	void _UpdatePhysics(
		const NwTime& game_time
		, MyGamePlayer& game_player
		, const MyAppSettings& app_settings
		);

	void Physics_AddGroundPlane_for_Collision();

#endif // GAME_CFG_WITH_PHYSICS

};
