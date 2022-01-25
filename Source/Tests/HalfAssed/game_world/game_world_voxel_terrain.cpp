//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Util/Tweakable.h>

#include <Renderer/Core/MeshInstance.h>
//#include <Voxels/private/Data/vx5_ChunkConfig.h>
#include <Voxels/private/Meshing/vx5_BuildMesh.h>
#include <Voxels/private/base/math/SQEM.h>
#include <Utility/VoxelEngine/VoxelTerrainChunk.h>

#include "game_player.h"
#include "game_settings.h"
#include "FPSGame.h"
#include "game_world/game_world.h"







#define CLEAR_VOXEL_DATABASE_ON_START	(0)


namespace
{

#if MX_DEBUG

	// use low values because DebugHeap uses most of address space in 32-bit mode
	const size_t CHUNK_INDEX_DB_SIZE = mxMiB(8);
	const size_t CHUNK_DATA_DB_SIZE = mxMiB(64);

#else

#if mxARCH_TYPE	== mxARCH_64BIT
	const size_t CHUNK_INDEX_DB_SIZE = mxGiB(1);
	const size_t CHUNK_DATA_DB_SIZE = mxGiB(4);
#else
	const size_t CHUNK_INDEX_DB_SIZE = mxMiB(8);
	const size_t CHUNK_DATA_DB_SIZE = mxMiB(64);
#endif

#endif

};


namespace VX5
{

	ERet WorldDatabases_LMDB2::Open(const char* folder_with_database_files)
	{
		static const char* INDEX_DB_FILENAME = "meta";
		static const char* CHUNK_DB_FILENAME = "data";

		//
		FilePathStringT	temp_str;

		Str::ComposeFilePath(temp_str,
			folder_with_database_files,
			INDEX_DB_FILENAME
			);
		mxDO(index_database_LMDB.open( temp_str.c_str(), CHUNK_INDEX_DB_SIZE ));

		Str::ComposeFilePath(temp_str,
			folder_with_database_files,
			CHUNK_DB_FILENAME
			);
		mxDO(chunk_database_LMDB.open( temp_str.c_str(), CHUNK_DATA_DB_SIZE ));

		return ALL_OK;
	}

	void WorldDatabases_LMDB2::Close()
	{
		index_database_LMDB.close();
		chunk_database_LMDB.close();
	}

}//namespace VX5


/*
-----------------------------------------------------------------------------
	MyGameWorld
-----------------------------------------------------------------------------
*/
ERet MyGameWorld::_InitVoxelEngine(
								   const MyGameSettings& game_settings
								   , NwClump* scene_clump
								   , AllocatorI& scratchpad
								   )
{
	//
	ATaskScheduler* serial_task_scheduler = &TaskScheduler_Serial::s_instance;
	ATaskScheduler* parallel_task_scheduler = &TaskScheduler_Jq::s_instance;

	ATaskScheduler* task_scheduler
		= (0)// || game_settings.dbg_disable_parallel_voxels)
		? serial_task_scheduler
		: parallel_task_scheduler;

#if GAME_CFG_RELEASE_BUILD
	task_scheduler = parallel_task_scheduler;
#endif

	//
	VX5::EngineI::CreateEngineArgs	create_engine_args;
	create_engine_args.client = this;
	create_engine_args.task_scheduler = task_scheduler;
	create_engine_args.debug_renderer = &voxels.voxel_engine_dbg_drawer;

	//
	voxels.engine = VX5::EngineI::create(
		create_engine_args
		, &_allocator
		);

	//
	mxDO(voxels._voxel_terrain_renderer.Initialize(
		GL::getMainRenderContext()
		, scratchpad
		, *scene_clump
		));
	
#if USE_PERSISTENT_VOXEL_DATABASE
	{
		mxDO(OS::IO::MakeDirectoryIfDoesntExist2(game_settings.path_to_userdata.c_str()));

		mxDO(voxels.voxel_databases.Open(game_settings.path_to_userdata.c_str()));

		//
#if CLEAR_VOXEL_DATABASE_ON_START
		voxels.voxel_databases.Drop();
#endif // CLEAR_VOXEL_DATABASE_ON_START

	}
#endif // USE_PERSISTENT_VOXEL_DATABASE

	const VX5::Real test_plane_size_in_meters = 4000;
	const int test_plane_LoD_count = 1;// 8;

	const VX5::WorldCfg test_plane_world_info = VX5::Utilities::GetWorldInfoFromWorldSizeAndLodCount(
		test_plane_size_in_meters
		, test_plane_LoD_count
		);
	voxels._test_plane = PlaneD::createFromPointAndNormal(
		CV3d(0)//CV3d(0,0,-test_plane_world_info.chunk_size_at_LoD0)
		, CV3d(0,0,1)	// UP
		);

	//
	voxels.voxel_world = _CreateVoxelWorld(
		voxels._test_plane
		, voxels.engine
		, &voxels
		, this
		, this
		, this
		, voxels.voxel_databases
		, VoxMat_Grass
		, voxels.world_bounds
		);

	//
	voxels._test_terrain_noise._result_scale = 1;
	voxels._test_terrain_noise._coarsest_frequency = 1;
	voxels._test_terrain_noise._gain = 0.5;
	voxels._test_terrain_noise._lacunarity = 2;
	voxels._test_terrain_noise._min_octaves = 1;
	voxels._test_terrain_noise._max_octaves = 7;
	voxels._test_terrain_noise.recalculateExponents();


	//
	VX5::EngineI::ContourIsosurfaceInputs	contour_inputs;
	contour_inputs.bounds = CubeMLf::fromOther(
		voxels.world_bounds
		);
	contour_inputs.resolution_log2 =
		3//4
		;

	VX5::EngineI::ContourIsosurfaceOutputs	contour_outputs;



	//
	struct HalfspaceIsosurface: VX5::IsosurfaceI
	{
		PlaneF	plane;

	public:
		HalfspaceIsosurface()
		{
			plane = PlaneF::createFromPointAndNormal(
				CV3f(0), CV3f(0,0,1)
				);
		}
		virtual float GetSignedDistance(
			const float x,
			const float y,
			const float z
			) const override
		{
			return plane.distanceToPoint(CV3f(x,y,z));
		}
	};
	HalfspaceIsosurface	halfspace_isosurface;

#if 0 // horizontal plane in the middle of the volume
	halfspace_isosurface.plane = PlaneF::createFromPointAndNormal(
		CV3f(0, 0, 7.5f)	// only upper part is collapsed to 4 cells
		//CV3f(0, 0, 8)	// octree collapses to 8 cells - good
		, CV3f(0,0,1)
		);
#else
	halfspace_isosurface.plane = PlaneF::createFromPointAndNormal(
		contour_inputs.bounds.center()
		, CV3f(1,1,1).normalized()
		);
#endif



	//
	struct SphereIsosurface: VX5::IsosurfaceI
	{
		CV3f	sphere_center;
		float	sphere_radius;
	public:
		SphereIsosurface()
		{
			sphere_center = CV3f(0);
			sphere_radius = 1;
		}
		virtual float GetSignedDistance(
			const float x,
			const float y,
			const float z
			) const override
		{
			float len = (CV3f(x,y,z) - sphere_center).length();
			float dist = len - sphere_radius;
			return dist;
		}
	};
	SphereIsosurface	sphere_isosurface;
#if 1
	sphere_isosurface.sphere_center = contour_inputs.bounds.center()
		+ CV3f(0.7f);//de-center so that no surface-crossing edges!
	sphere_isosurface.sphere_radius = 0.1f;// 4.0f;//4.0f;
#else
	sphere_isosurface.sphere_center = contour_inputs.bounds.center();
	sphere_isosurface.sphere_radius = 4.0f;//8.0f;//0.1f;//
#endif


	//
	struct CubeIsosurface: VX5::IsosurfaceI
	{
		V3f	center;
		F32	radius;	//!< half-size, must be always positive
	public:
		CubeIsosurface()
		{
			center = CV3f(0);
			radius = 1;
		}
		virtual float GetSignedDistance(
			const float x,
			const float y,
			const float z
			) const override
		{
			const V3f relativePosition = CV3f(x,y,z) - center;
#if 0
			const V3f offsetFromCenter = V3_Abs( relativePosition );	// abs() accounts for symmetry/congruence
			return Max3(
				offsetFromCenter.x - radius,
				offsetFromCenter.y - radius,
				offsetFromCenter.z - radius
				);

#else
			//https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
			//vec3 q = abs(p) - b;
			//return length(max(q,0.0)) + min( max(q.x,max(q.y,q.z)), 0.0 );

			const V3f q = V3_Abs( relativePosition ) - CV3f(radius);

			return V3_Length( V3f::max(q, CV3f(0)) )
				+ minf(q.maxComponent(), 0)
				;
#endif
		}
	};
	CubeIsosurface	cube_isosurface;
	cube_isosurface.center = contour_inputs.bounds.center()
		//+ CV3f(0.7f)//de-center so that no surface-crossing edges!
		;
	cube_isosurface.radius = 6;//0.5f;//8;// 4;//5.0f;//0.2f;//1;//



	//
	struct BoxIsosurface: VX5::IsosurfaceI
	{
		V3f	center;
		V3f	half_size;
	public:
		BoxIsosurface()
		{
			center = CV3f(0);
			half_size = CV3f(1);
		}
		virtual float GetSignedDistance(
			const float x,
			const float y,
			const float z
			) const override
		{
			const V3f relativePosition = CV3f(x,y,z) - center;

			//https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
			//vec3 q = abs(p) - b;
			//return length(max(q,0.0)) + min( max(q.x,max(q.y,q.z)), 0.0 );

			const V3f q = V3_Abs( relativePosition ) - half_size;

			return V3_Length( V3f::max(q, CV3f(0)) )
				+ minf(q.maxComponent(), 0)
				;
		}
	};
	BoxIsosurface	box_isosurface;
	box_isosurface.center = contour_inputs.bounds.center()
		//+ CV3f(0.7f)//de-center so that no surface-crossing edges!
		+ CV3f(0, 0, -0.7f)//de-center
		;
	box_isosurface.half_size = CV3f(8,8,4);



	//
	voxels.engine->ContourIsosurface(
		sphere_isosurface//cube_isosurface//box_isosurface//halfspace_isosurface//
		, contour_inputs
		, contour_outputs
		);

	//
	//FPSGame::Get().renderer._render_system->_debug_line_renderer

	//
	////
	//NwModel_::PhysObjParams	phys_obj_params;
	//phys_obj_params.is_static = true;

	////
	//mxDO(NwModel_::Create(
	//	mission_exit_mdl

	//	, MakeAssetID("mission_exit")
	//	, game.runtime_clump

	//	, M44f::createUniformScaling(2.0f)
	//	//* M44_RotationX(DEG2RAD(90))
	//	, *game.world.render_world

	//	, game.world.physics_world
	//	, PHYS_OBJ_MISSION_EXIT

	//	, M44_RotationX(DEG2RAD(90)) * M44f::createTranslation(mission_exit_pos)
	//	, NwModel_::COL_SPHERE

	//	, CV3f(0)
	//	, CV3f(0)

	//	, phys_obj_params
	//	));

	return ALL_OK;
}

void MyGameWorld::_DeinitVoxelEngine()
{

	//
	voxels.voxel_engine_dbg_drawer.shutdown();

	//
	VX5::EngineI::destroy( voxels.engine );
	voxels.engine = nil;

	//
	voxels._voxel_terrain_renderer.Shutdown();

#if USE_PERSISTENT_VOXEL_DATABASE
	//
	voxels.voxel_databases.Close();
#endif // USE_PERSISTENT_VOXEL_DATABASE

}

VX5::WorldI* MyGameWorld::_CreateVoxelWorld(
	const PlaneD& plane
	, VX5::EngineI* voxel_engine
	, VX5::ChunkGeneratorI* chunk_generator
	, MeshBakerI* mesh_baker
	, MeshLoaderI* mesh_loader
	, VX5::WorldListenerI* world_delegate
	, const VX5::WorldStorage& voxel_databases
	, const VX5::MaterialID default_solid_material
	, CubeMLf &world_bounds_
	)
{


#define DETERMINE_WORLD_SIZE_BASED_ON_VOXEL_SIZE	(0)


	//
	VX5::EngineI::CreateWorldArgs	create_world_settings;


#if DETERMINE_WORLD_SIZE_BASED_ON_VOXEL_SIZE

	#if 0 // GAME_CFG_RELEASE_BUILD
		const double voxel_size_in_meters = 0.25;
		const int num_LoDs = 2;
	#else
		const double voxel_size_in_meters = 0.5;
		//const double voxel_size_in_meters = 1;
		const int num_LoDs = 4;
	#endif

#else



#if GAME_CFG_RELEASE_BUILD

	// GOOD CONFIGURATION!
	const double world_size = 250;	// in meters
	const int num_LoDs = 5;	//

#else

	//// SAME AS IN RELEASE
	//const double world_size = 250;	// in meters
	//const int num_LoDs = 5;	//

	//// DEBUG CONF WITH 32^3 chunks
	//const double world_size = 250;	// in meters
	//const int num_LoDs = 4;	//

	//// DEBUG CONF SMALL
	//const double world_size = 80;	// in meters
	//const int num_LoDs = 2;	//

	const double world_size = 16;	// in meters
	const int num_LoDs = 1;	//

#endif

	const VX5::WorldCfg voxel_world_info = VX5::Utilities::GetWorldInfoFromWorldSizeAndLodCount(
		world_size
		, num_LoDs
		);
	const double voxel_size_in_meters = voxel_world_info.cell_size_at_LoD0;

	DBGOUT("[GAME] chunk size = %.3f, cell/voxel size = %.3f",
		voxel_world_info.chunk_size_at_LoD0,
		voxel_world_info.cell_size_at_LoD0
		);

#endif


	//
	create_world_settings.bounds = VX5::Utilities::ComputeWorldBoundsForVoxelSizeAndLodCount(
		voxel_size_in_meters
		, num_LoDs
		);

create_world_settings.bounds.min_corner =
	create_world_settings.bounds.min_corner + CV3d(create_world_settings.bounds.side_length * 0.5f)
	;

	create_world_settings.num_LoDs = num_LoDs;

	world_bounds_ = CubeMLf::fromOther(create_world_settings.bounds);

	//
	VX5::RunTimeWorldSettings	initial_world_settings;
	// always split chunks and use the finest LoD
	initial_world_settings.lod.selection.split_distance_factor = BIG_NUMBER;
	create_world_settings.initial_settings = &initial_world_settings;
	//
	create_world_settings.storage.meta = voxel_databases.meta;
	create_world_settings.storage.data = voxel_databases.data;
	//
	create_world_settings.user_data = (void*) &plane;

	//
	VX5::WorldI* voxel_world = voxel_engine->CreateWorld(
		create_world_settings
		, *chunk_generator
		, *mesh_baker
		, *mesh_loader
		, world_delegate
		);

	return voxel_world;
}

void MyGameWorld::regenerateVoxelTerrains()
{
	VX5::Utilities::reloadAllWorlds(voxels.engine);
}





void MyGameWorld::debugDrawVoxels(
								   const NwFloatingOrigin& floating_origin
								   , const NwCameraView& camera_view
								   )
{
	voxels.voxel_engine_dbg_drawer.begin(
		floating_origin
		, camera_view
		);
	
	const MyGameSettings& game_settings = FPSGame::Get().settings;
	if(game_settings.voxels.engine.debug.flags & VX5::DbgFlag_Draw_LoD_Octree)
	{
		voxels.engine->debugDrawWith( voxels.voxel_engine_dbg_drawer );
	}


	voxels.voxel_engine_dbg_drawer.end();
}

void MyVoxels::GetBoxContentsInfo(
								  VX5::BoxContentsInfo &box_contents_
								  , const CubeMLd& box_in_world_space
								  , const VX5::ChunkGenerationContext& context
								  )
{
#if 1|| GAME_CFG_RELEASE_BUILD
	// for faster generation
	box_contents_.is_homogeneous = true;
	box_contents_.homogeneous_material_id = VX5::EMPTY_SPACE;
#else
	VX5::ChunkGeneratorI::GetBoxContentsInfo(box_contents_, box_in_world_space, context);
	box_contents_.may_have_sharp_features = false;
#endif
}

double MyVoxels::GetSignedDistanceAtPoint(
	const V3d& position_in_world_space
	, const VX5::ChunkGenerationContext& context
	, const VX5::ChunkGenerationScratchpad& scratchpad
	)
{
	//UNDONE;
	return _test_plane.distanceToPoint(position_in_world_space);
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
