//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Util/Tweakable.h>

#include <Rendering/Public/Scene/MeshInstance.h>
//#include <Voxels/private/Data/vx5_ChunkConfig.h>
#include <Voxels/private/data/vx5_chunk_id.h>
#include <Voxels/private/Meshing/vx5_BuildMesh.h>
//#include <Voxels/private/Meshing/contouring/vx5_DualContouring.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include "human_player.h"
#include "game_settings.h"
#include "game_app.h"
#include "world/world.h"



#if GAME_CFG_WITH_VOXELS



static const bool USE_PERSISTENT_VOXEL_DATABASE = false;


static const int NUM_LODS = 1;//16;//max

static const int DATA_CHUNK_RESOLUTION_LOG2 = 3;

//static const double VOXEL_SIZE_IN_METERS = 100;//1;




#define CLEAR_VOXEL_DATABASE_ON_START	(0)


namespace
{

#if MX_DEBUG

	// use low values because DebugHeap uses most of address space in 32-bit mode
	const size_t VX_META_DB_SIZE = mxMiB(8);
	const size_t VX_DATA_DB_SIZE = mxMiB(64);
	const size_t VX_LODS_DB_SIZE = mxMiB(64);

#else

#if mxARCH_TYPE	== mxARCH_64BIT
	const size_t VX_META_DB_SIZE = mxMiB(256);
	const size_t VX_DATA_DB_SIZE = mxGiB(1);
	const size_t VX_LODS_DB_SIZE = mxGiB(4);
#else
	const size_t VX_META_DB_SIZE = mxMiB(8);
	const size_t VX_DATA_DB_SIZE = mxMiB(64);
	const size_t VX_LODS_DB_SIZE = mxMiB(64);
#endif

#endif

};


namespace VX5
{
	ERet WorldDatabases_LMDB2::Open(const char* folder_with_database_files)
	{
		static const char* INDEX_DB_FILENAME = "meta";
		static const char* CHUNK_DB_FILENAME = "data";
		static const char* LODS_DB_FILENAME = "lods";

		//
		FilePathStringT	temp_str;

		//
		Str::ComposeFilePath(temp_str,
			folder_with_database_files,
			INDEX_DB_FILENAME
			);
		mxDO(index_database_LMDB.open( temp_str.c_str(), VX_META_DB_SIZE ));

		//
		Str::ComposeFilePath(temp_str,
			folder_with_database_files,
			CHUNK_DB_FILENAME
			);
		mxDO(chunk_database_LMDB.open( temp_str.c_str(), VX_DATA_DB_SIZE ));

		//
		Str::ComposeFilePath(temp_str,
			folder_with_database_files,
			LODS_DB_FILENAME
			);
		mxDO(lods_database_LMDB.open( temp_str.c_str(), VX_LODS_DB_SIZE ));

		return ALL_OK;
	}

	void WorldDatabases_LMDB2::Close()
	{
		index_database_LMDB.close();
		chunk_database_LMDB.close();
		lods_database_LMDB.close();
	}

}//namespace VX5


/*
-----------------------------------------------------------------------------
	SgWorld
-----------------------------------------------------------------------------
*/
ERet SgWorld::_InitVoxelEngine(
								   const SgAppSettings& game_settings
								   , NwClump* scene_clump
								   , AllocatorI& scratchpad
								   )
{
//	VX5::Offline_Generate_LUT__s_adjacentCellOffsets();

	//
	NwJobSchedulerI* serial_task_scheduler = &NwJobScheduler_Serial::s_instance;
	NwJobSchedulerI* parallel_task_scheduler = &NwJobScheduler_Parallel::s_instance;

	NwJobSchedulerI* task_scheduler
		= (1)// || game_settings.dbg_disable_parallel_voxels)
		? serial_task_scheduler
		: parallel_task_scheduler;

#if GAME_CFG_RELEASE_BUILD
	task_scheduler = parallel_task_scheduler;
#endif

	//
	VX5::EngineConfiguration	create_engine_args;
	//create_engine_args.allocator = ?;
	create_engine_args.listener = this;
	create_engine_args.task_scheduler = task_scheduler;
	create_engine_args.debug_view = &voxels.voxel_engine_dbg_drawer;

	//
	voxels.voxel_engine = VX5::EngineI::Create(create_engine_args);

	//
	mxDO(voxels._voxel_terrain_renderer.Initialize(
		GL::getMainRenderContext()
		, scratchpad
		, *scene_clump
		));
	
	if(USE_PERSISTENT_VOXEL_DATABASE)
	{
		mxDO(OS::IO::MakeDirectoryIfDoesntExist2(game_settings.path_to_userdata.c_str()));
		mxDO(voxels.voxel_databases.Open(game_settings.path_to_userdata.c_str()));

		//
#if CLEAR_VOXEL_DATABASE_ON_START
		voxels.voxel_databases.Drop();
#endif // CLEAR_VOXEL_DATABASE_ON_START

	}



	//
	voxels._test_plane = PlaneD::createFromPointAndNormal(
		CV3d(0), 
		CV3d(0,0,1)	// UP
		);

	//
	const CubeMLd world_bounds = CubeMLd::fromCenterRadius(
		CV3d(0),
		2000.0f
		//VOXEL_SIZE_IN_METERS * (1ull << NUM_LODS)
		);

	//
	voxels._test_sphere.center = CV3d(-10,0,-1000);
	voxels._test_sphere.radius = 50;//900;

	//
	VX5::RunTimeWorldSettings	initial_world_settings;
	initial_world_settings.lod = game_settings.voxels.engine.lod;
	initial_world_settings.mesher = game_settings.voxels.engine.mesher;
	initial_world_settings.debug = game_settings.voxels.engine.debug;

	//
	VX5::WorldStorageConfiguration	world_storage_config;
	if(USE_PERSISTENT_VOXEL_DATABASE)
	{
		world_storage_config.use_persistent_storage = true;
		world_storage_config.persistent.meta = &voxels.voxel_databases.index_database_LMDB;
		world_storage_config.persistent.data = &voxels.voxel_databases.chunk_database_LMDB;
		world_storage_config.persistent.lods = &voxels.voxel_databases.lods_database_LMDB;
	}
	else
	{
		world_storage_config.cache.chunk_cache_size_in_bytes = 
			//vxMiB(1)//vxKiB(64);//
			vxMiB(16)
			;
	}

	//
	voxels.voxel_world = _CreateVoxelPlanet(
		world_bounds
		, voxels._test_sphere
		, voxels.voxel_engine
		, voxels
		, *this
		, *this
		, this
		, world_storage_config
		, VoxMat_Grass
		, voxels.world_bounds
		, &initial_world_settings
		);
	//
	voxels._test_terrain_noise._result_scale = 1;
	voxels._test_terrain_noise._coarsest_frequency = 1;
	voxels._test_terrain_noise._gain = 0.5;
	voxels._test_terrain_noise._lacunarity = 2;
	voxels._test_terrain_noise._min_octaves = 1;
	voxels._test_terrain_noise._max_octaves = 7;
	voxels._test_terrain_noise.recalculateExponents();

	return ALL_OK;
}

void SgWorld::_DeinitVoxelEngine()
{

	//
	voxels.voxel_engine_dbg_drawer.shutdown();

	//
	VX5::EngineI::Destroy( voxels.voxel_engine );
	voxels.voxel_engine = nil;

	//
	voxels._voxel_terrain_renderer.Shutdown();

#if USE_PERSISTENT_VOXEL_DATABASE
	//
	voxels.voxel_databases.Close();
#endif // USE_PERSISTENT_VOXEL_DATABASE

}

VX5::WorldI* SgWorld::_CreateVoxelPlanet(
	const CubeMLd& world_bounds
	, const BSphereD& sphere
	, VX5::EngineI* voxel_engine
	, VX5::ProcGen::ChunkGeneratorI& chunk_generator
	, MeshBakerI& mesh_baker
	, MeshLoaderI& mesh_loader
	, VX5::WorldListenerI* world_delegate
	, const VX5::WorldStorageConfiguration& world_storage_config
	, const VX5::MaterialID default_solid_material
	, CubeMLf &world_bounds_
	, VX5::RunTimeWorldSettings* initial_world_settings /*= nil*/
	)
{
	//
	VX5::WorldConfiguration	create_world_settings;

	//
	create_world_settings.num_LoDs = NUM_LODS;
	create_world_settings.data_chunk_resolution_log2 = DATA_CHUNK_RESOLUTION_LOG2;

	create_world_settings.bounds = world_bounds;
	world_bounds_ = CubeMLf::fromOther(create_world_settings.bounds);

	//
	create_world_settings.initial_settings = initial_world_settings;
	//
	create_world_settings.listener = world_delegate;

	create_world_settings.storage = world_storage_config;

	create_world_settings.user_data = (VX_UserDefinedWorldData*) &sphere;

	//
	VX5::WorldI* voxel_world = voxel_engine->CreateWorld(
		create_world_settings
		, chunk_generator
		, mesh_baker
		, mesh_loader
		);

	return voxel_world;
}

void SgWorld::regenerateVoxelTerrains()
{
	VX5::Utilities::reloadAllWorlds(voxels.voxel_engine);
}





void SgWorld::debugDrawVoxels(
								   const NwFloatingOrigin& floating_origin
								   , const NwCameraView& camera_view
								   ) const
{
	//TODO: refactor
	VX5::MyDebugDraw& non_const_dbg_draw = const_cast<VX5::MyDebugDraw&>(voxels.voxel_engine_dbg_drawer);

	non_const_dbg_draw.begin(
		floating_origin
		, camera_view
		);
	
	const SgAppSettings& game_settings = SgGameApp::Get().settings;
	if(game_settings.voxels.engine.debug.flags & VX5::DbgFlag_Draw_LoD_Octree)
	{
		VX5::WorldDebugDrawSettings	dbg_draw_settings;
		dbg_draw_settings.draw_lod_octree = true;

		voxels.voxel_world->DebugDraw(
			non_const_dbg_draw
			, dbg_draw_settings
			);
	}

	non_const_dbg_draw.end();
}

void MyVoxels::VX_GetBoxContentsInfo(
								  VX5::ProcGen::BoxContentsInfo &box_contents_
								  , const CubeMLd& box_in_world_space
								  , const VX5::ProcGen::ChunkContext& context
								  )
{
#if 0|| GAME_CFG_RELEASE_BUILD
	// for faster generation
	box_contents_.is_homogeneous = true;
	box_contents_.homogeneous_material_id = VX5::EMPTY_SPACE;
#endif
}

VX5::ProcGen::ProvidedChunkData MyVoxels::VX_StartGeneratingChunk(
	VX5::ProcGen::ChunkConstants &chunk_constants_
	, const VX5::ProcGen::ChunkContext& context
	)
{
	//DEVOUT("VX_StartGeneratingChunk: %s",
	//	VX5::ToMyChunkID(context.chunk_id).ToString().c_str()
	//	);
	return VX5::ProcGen::ProvidedChunkData();
}

double MyVoxels::VX_GetSignedDistanceAtPoint(
	const V3d& position_in_world_space
	, const VX5::ProcGen::ChunkContext& context
	, const VX5::ProcGen::ChunkConstants& scratchpad
	)
{
	//UNDONE;
	//return _test_plane.distanceToPoint(position_in_world_space);
	return _test_sphere.signedDistanceToPoint(position_in_world_space);
}

float01_t MyVoxels::GetStrength(const V3f& pos)
{
	// good values for voxel_size = 0.25
	nwTWEAKABLE_CONST(float, CRATER_NOISE_SCALE, 2);
	nwTWEAKABLE_CONST(float, CRATER_NOISE_OCTAVES, 1);

	return _test_terrain_noise.evaluate2D(
		pos.x * CRATER_NOISE_SCALE, pos.y * CRATER_NOISE_SCALE,
		_perlin_noise,
		CRATER_NOISE_OCTAVES
		);
}

#endif // #if GAME_CFG_WITH_VOXELS
