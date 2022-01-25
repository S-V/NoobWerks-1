//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Util/Tweakable.h>

#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SceneRenderer.h>
//#include <Voxels/private/Data/vx5_ChunkConfig.h>
#include <Voxels/private/data/vx5_chunk_id.h>
#include <Voxels/private/Meshing/vx5_BuildMesh.h>
//#include <Voxels/private/Meshing/contouring/vx5_DualContouring.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include "app_settings.h"
#include "voxels/voxels.h"


#define CLEAR_VOXEL_DATABASE_ON_START	(0)


namespace
{
static const bool USE_PERSISTENT_VOXEL_DATABASE = 1;



#if 0
	static const int NUM_LODS = 3;//16;//max
	static const int DATA_CHUNK_RESOLUTION_LOG2 = 3;
#else
	static const int NUM_LODS = 10;//16;//max
	static const int DATA_CHUNK_RESOLUTION_LOG2 = 3;//max res
#endif

static const float VOXEL_SIZE_WORLD = 1;



#if MX_DEBUG

	// use low values because DebugHeap uses most of address space in 32-bit mode
	const size_t VX_META_DB_SIZE = mxMiB(8);
	const size_t VX_DATA_DB_SIZE = mxMiB(128);
	const size_t VX_LODS_DB_SIZE = mxMiB(256);

#else

#if mxARCH_TYPE	== mxARCH_64BIT
	const size_t VX_META_DB_SIZE = mxGiB(1);
	const size_t VX_DATA_DB_SIZE = mxGiB(4);
	const size_t VX_LODS_DB_SIZE = mxGiB(8);
#else
	const size_t VX_META_DB_SIZE = mxMiB(8);
	const size_t VX_DATA_DB_SIZE = mxMiB(128);
	const size_t VX_LODS_DB_SIZE = mxMiB(256);
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



namespace VX5
{
	ChunkDatabase_InMemoryCache::ChunkDatabase_InMemoryCache()
	{
		//
	}

	ChunkDatabase_InMemoryCache::~ChunkDatabase_InMemoryCache()
	{
		//
	}

	ERet ChunkDatabase_InMemoryCache::Read(
		const ChunkID& chunk_id
		, void *buffer, U32 buffer_size
		, U32 *size_read_
		) const 
	{
		return ERR_OBJECT_NOT_FOUND;
	}

	ERet ChunkDatabase_InMemoryCache::Write(
		const ChunkID& chunk_id
		, const void* data, U32 size
		) 
	{
		return ALL_OK;
	}

	ERet ChunkDatabase_InMemoryCache::Erase(
		const ChunkID& chunk_id
		) 
	{
		return ALL_OK;
	}

	ERet ChunkDatabase_InMemoryCache::Drop() 
	{
		return ALL_OK;
	}


	MyDebugDraw::MyDebugDraw( AllocatorI& allocator )
		: _points( allocator )
		, _lines( allocator )
		, _aabbs( allocator )
	{
		//
	}

	void MyDebugDraw::Initialize(
		TbPrimitiveBatcher* prim_renderer
		, Rendering::SpatialDatabaseI* spatial_database
		)
	{
		_prim_renderer = prim_renderer;
		_render_world = spatial_database;

		_crit_section.Initialize();
		_is_locked = 0;
	}

	void MyDebugDraw::Shutdown()
	{
		_crit_section.Shutdown();
		_is_locked = 0;

		_prim_renderer = nil;
	}

	ERet MyDebugDraw::begin(
		const Rendering::NwFloatingOrigin& floating_origin
		, const Rendering::NwCameraView& scene_view
		)
	{
		_floating_origin = floating_origin;
		this->scene_view = scene_view;

		// enable depth testing
		NwRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("default")));

		_prim_renderer->PushState();
		_prim_renderer->SetStates( *renderState );


		//
		const V3f up = scene_view.getUpDir();

		//
		//_dbgview.draw_internal(
		//	scene_view,
		//	*_prim_renderer
		//	);

		//DEVOUT("Drawing %u colored points...", _colored_points.num());

		//
		{
			SpinWait::Lock	scoped_lock(_crit_section);

			nwFOR_EACH(const DebugPoint& point, _points)
			{
				_prim_renderer->DrawSolidCircle(
					_floating_origin.getRelativePosition( point.position_in_world_space )
					, scene_view.right_dir, up
					, RGBAf::FromRGBAi(point.color)
					, point.scale
					, 4	// num sides
					);
			}

			//
			nwFOR_EACH(const DebugLine& line, _lines)
			{
				_prim_renderer->DrawLine(
					_floating_origin.getRelativePosition( line.start_position_in_world_space )
					, _floating_origin.getRelativePosition( line.end_position_in_world_space )
					, RGBAf::FromRGBAi( line.start_color )
					, RGBAf::FromRGBAi( line.end_color )
					);
			}

			//
			nwFOR_EACH(const DebugAABB& aabb, _aabbs)
			{
				_prim_renderer->DrawAABB(
					_floating_origin.getRelativeAABB( aabb.aabb_in_world_space )
					, RGBAf::FromRGBAi( aabb.color )
					);
			}
		}

		return ALL_OK;
	}

	void MyDebugDraw::end()
	{
		_prim_renderer->PopState();
	}

	void MyDebugDraw::VX_DrawLine_MainThread(
		const V3d& start_pos_in_world_space
		, const RGBAi& start_color
		, const V3d& end_pos_in_world_space
		, const RGBAi& end_color
		)
	{
		_prim_renderer->DrawLine(
			V3f::fromXYZ(start_pos_in_world_space)
			, V3f::fromXYZ(end_pos_in_world_space)
			, RGBAf::fromRgba32( start_color.u )
			, RGBAf::fromRgba32( end_color.u )
			);
	}

	void MyDebugDraw::VX_DrawAABB_MainThread(
		const AABBd& aabb_in_world_space,
		const RGBAf& color
		)
	{
		const AABBf aabb_relative_to_camera = _floating_origin.getRelativeAABB( aabb_in_world_space );

		_prim_renderer->DrawAABB(
			aabb_relative_to_camera
			, color
			);

#if 0
		// Cull bounding boxes that take very little space on the screen (contribution culling).
		// This improves increases performance when looking at lots of nested LoDs from the distance.

		V2f min_corner_in_screen_space;
		V2f max_corner_in_screen_space;


		// BUG: NANs when projectViewSpaceToScreenSpace() returns false

		const bool min_corner_in_view = scene_view.projectViewSpaceToScreenSpace(
			aabb_relative_to_camera.min_corner, &min_corner_in_screen_space
			);
		const bool max_corner_in_view = scene_view.projectViewSpaceToScreenSpace(
			aabb_relative_to_camera.max_corner, &max_corner_in_screen_space
			);

		// If the box is not behind the near plane:
		if( min_corner_in_view || max_corner_in_view )
		{
			const float box_area = 
				fabs( max_corner_in_screen_space.x - min_corner_in_screen_space.x )
				*
				fabs( max_corner_in_screen_space.y - min_corner_in_screen_space.y )
				;

			if( box_area > 2.0f )
			{
				_prim_renderer->DrawAABB(
					aabb_relative_to_camera
					, color
					);
			}
		}
#endif
	}

	void MyDebugDraw::VX_Lock()
	{
		_crit_section.Enter();
		_is_locked = 1;
	}

	void MyDebugDraw::VX_Unlock()
	{
		_crit_section.Leave();
		_is_locked = 0;
	}

	void MyDebugDraw::VX_AddPoint(
		const V3d& position_in_world_space
		, const RGBAi& color
		, double point_scale
	)
	{
		//SpinWait::Lock	scoped_lock(_crit_section);
		mxASSERT(_is_locked);

		DebugPoint	new_point;
		new_point.position_in_world_space = position_in_world_space;
		new_point.scale = point_scale;
		new_point.color = color;

		_points.add( new_point );
	}

	void MyDebugDraw::VX_AddLine(
		const V3d& start_pos_in_world_space
		, const RGBAi& start_color
		, const V3d& end_pos_in_world_space
		, const RGBAi& end_color
		)
	{
		//SpinWait::Lock	scoped_lock(_crit_section);
		mxASSERT(_is_locked);

		DebugLine	new_line;
		{
			new_line.start_position_in_world_space = start_pos_in_world_space;
			new_line.start_color = start_color;
			new_line.end_position_in_world_space = end_pos_in_world_space;
			new_line.end_color = end_color;
		}
		_lines.add(new_line);
	}

	void MyDebugDraw::VX_AddAABB(
		const AABBd& aabb_in_world_space
		, const RGBAi& color
		)
	{
		//SpinWait::Lock	scoped_lock(_crit_section);
		mxASSERT(_is_locked);

		DebugAABB	new_aabb;
		new_aabb.aabb_in_world_space = aabb_in_world_space;
		new_aabb.color = color;

		_aabbs.add( new_aabb );
	}

	void MyDebugDraw::VX_Clear()
	{
		//SpinWait::Lock	scoped_lock(_crit_section);
		mxASSERT(_is_locked);

		_points.RemoveAll();
		_lines.RemoveAll();
		_aabbs.RemoveAll();
	}

}//namespace VX5






namespace
{

VX5::WorldI* _CreateVoxelPlanet(
	const CubeMLd& world_bounds
	, const BSphereD& sphere
	, VX5::EngineI* voxel_engine
	, VX5::ProcGen::ChunkGeneratorI& chunk_generator
	, VX5::MeshBakerI& mesh_baker
	, VX5::MeshLoaderI& mesh_loader
	, VX5::WorldControllerI* world_controller
	, VX5::WorldListenerI* world_delegate
	, const VX5::WorldStorageConfiguration& world_storage_config
	, const VX5::MaterialID default_solid_material
	, VX5::RunTimeWorldSettings* initial_world_settings = nil
	)
{
	//
	VX5::WorldConfiguration	create_world_settings;

	//
	create_world_settings.num_LoDs = NUM_LODS;
	create_world_settings.data_chunk_resolution_log2 = DATA_CHUNK_RESOLUTION_LOG2;

	create_world_settings.bounds = world_bounds;

	//
	create_world_settings.initial_settings = initial_world_settings;
	//
	create_world_settings.controller = world_controller;
	create_world_settings.listener = world_delegate;

	create_world_settings.storage = world_storage_config;

	create_world_settings.user_data = (VX_WorldUserData*) &sphere;

	//
	VX5::WorldI* voxel_world = voxel_engine->CreateWorld(
		create_world_settings
		, chunk_generator
		, mesh_baker
		, mesh_loader
		);

	return voxel_world;
}

}






ERet MyVoxels::Initialize(
						  const NEngine::LaunchConfig & engine_settings
						  , const MyAppSettings& app_settings
						  , NwClump* scene_clump
						  , Rendering::SpatialDatabaseI* spatial_database
						  , const Rendering::VXGI::VoxelGrids* vxgi
						  )
{
	AllocatorI& scratchpad = MemoryHeaps::temporary();

	//
	//const Rendering::RrGlobalSettings& renderer_settings = app_settings.renderer;

	//
	MyVoxels& voxels = *this;

	//
	debug_drawer.Initialize(
		&Rendering::Globals::GetImmediateDebugRenderer()
		, spatial_database
		);

	//
	this->spatial_database = spatial_database;
	_scene_clump = scene_clump;


	//
	NwJobSchedulerI* serial_job_scheduler = &NwJobScheduler_Serial::s_instance;
	NwJobSchedulerI* parallel_job_scheduler = &NwJobScheduler_Parallel::s_instance;

	NwJobSchedulerI* job_scheduler
		= (1)// || app_settings.dbg_disable_parallel_voxels)
		? serial_job_scheduler
		: parallel_job_scheduler;

#if GAME_CFG_RELEASE_BUILD
	job_scheduler = parallel_job_scheduler;
#endif

	//
	VX5::EngineConfiguration	create_engine_args;
	//create_engine_args.allocator = ?;
	create_engine_args.listener = this;
	create_engine_args.job_scheduler = job_scheduler;
	create_engine_args.debug_view = &voxels.debug_drawer;

	//
	voxels.voxel_engine = VX5::EngineI::Create(create_engine_args);

	//
	mxDO(voxels._voxel_terrain_renderer.Initialize(
		NGpu::getMainRenderContext()
		, scratchpad
		, *scene_clump
		, vxgi
		));

	if(USE_PERSISTENT_VOXEL_DATABASE)
	{
		mxDO(OS::IO::MakeDirectoryIfDoesntExist2(app_settings.path_to_userdata.c_str()));
		mxDO(voxels.voxel_databases.Open(app_settings.path_to_userdata.c_str()));

		//
#if CLEAR_VOXEL_DATABASE_ON_START
		voxels.voxel_databases.index_database_LMDB.Drop();
		voxels.voxel_databases.chunk_database_LMDB.Drop();
		voxels.voxel_databases.lods_database_LMDB.Drop();
#endif // CLEAR_VOXEL_DATABASE_ON_START

	}



	//
	const int MAX_LOD = NUM_LODS - 1;
	const double world_size = (1 << MAX_LOD) * (1 << DATA_CHUNK_RESOLUTION_LOG2) * VOXEL_SIZE_WORLD;
	const CubeMLd world_bounds = CubeMLd::fromCenterRadius(
		CV3d(0)
		, world_size * 0.5
		);



	//
	const V3d test_box_halfsize = {
		world_size * 0.3f,
		world_size * 0.4f,
		world_size * 0.1f,
	};
	_test_box.center = world_bounds.center() + CV3d(world_size * 0.1f, 0, 0);
	_test_box.extent = test_box_halfsize;

	//
	voxels._test_plane = PlaneD::createFromPointAndNormal(
		//CV3d(0)
		//CV3d(-world_size*0.2f)
		world_bounds.center() + CV3d(0, 0, world_size * 0.125)

		//CV3d(0.1)
		//CV3d(-0.1)	//bug!
		//CV3d(-0.9)	//bug!
		//CV3d(-1.9)	//bug!

		//CV3d(0,0,-test_plane_world_info.chunk_size_at_LoD0)
		, 
		CV3d(0,0,1)	// UP

		//CV3d(1,1,1).normalized()	// a duplicate quad in the center?
		//CV3d(-1,-1,-1).normalized()

		//CV3d(-1,-1,0).normalized()	// no bug in hash table version!


		//CV3d(-1,-0.4,0).normalized()	// BUG!
		//CV3d(-1,-0.4,0).normalized()	// BUG!


		//CV3d(0,-1,0).normalized()	// no bug!

		//CV3d(1,0,0)	// BUG in the center in hash table version!

		//CV3d(0,1,1).normalized()	// no bug!
		);



	//
	voxels._test_sphere.center = CV3d(
		world_bounds.side_length*0.1,
		-world_bounds.side_length*0.1,
		0
		);
	voxels._test_sphere.radius = world_bounds.side_length*0.27;


	//
	VX5::RunTimeWorldSettings	initial_world_settings;
	initial_world_settings.lod = app_settings.voxel_engine_settings.lod;
initial_world_settings.lod.selection.split_distance_factor = 2.0f;
initial_world_settings.lod.selection.split_distance_factor = 1.5f;

	initial_world_settings.mesher = app_settings.voxel_engine_settings.mesher;

	initial_world_settings.mesher.flags.raw_value |= VX5::MesherFlags::Simplify_Chunk_Octree;

	initial_world_settings.debug = app_settings.voxel_engine_settings.debug;

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
			vxMiB(1)//vxKiB(64);//
			//vxMiB(4)
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
		, this	// VX5::WorldControllerI*
		, world_storage_config
		, VoxMat_Grass
		, &initial_world_settings
		);


	////
	//voxels._test_terrain_noise._result_scale = 1;
	//voxels._test_terrain_noise._coarsest_frequency = 1;
	//voxels._test_terrain_noise._gain = 0.5;
	//voxels._test_terrain_noise._lacunarity = 2;
	//voxels._test_terrain_noise._min_octaves = 1;
	//voxels._test_terrain_noise._max_octaves = 7;
	//voxels._test_terrain_noise.recalculateExponents();

	return ALL_OK;
}

void MyVoxels::Shutdown()
{
	MyVoxels& voxels = *this;
	//
	voxels.debug_drawer.Shutdown();

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
