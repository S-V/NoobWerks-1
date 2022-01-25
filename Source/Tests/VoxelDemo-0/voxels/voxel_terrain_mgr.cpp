//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Memory/MemoryHeaps.h>

#include "app_settings.h"
#include "app.h"

#include "voxels/voxel_terrain_mgr.h"
#include "voxels/voxels.h"



struct MyVoxelTerrainMgr::MyVoxelTerrainMgrData
{
	MyVoxels	voxels;

public:
	MyVoxelTerrainMgrData( AllocatorI& allocator )
		: voxels( allocator )
	{
	}
};


namespace
{
	static ProxyAllocator& GetAllocator() { return MemoryHeaps::voxels(); }
}


ERet MyVoxelTerrainMgr::Initialize(
								 const NEngine::LaunchConfig & engine_settings
								 , const MyAppSettings& app_settings
								 , NwClump* scene_clump
								 , Rendering::SpatialDatabaseI* spatial_database
								 , const Rendering::VXGI::VoxelGrids* vxgi
								 )
{
	AllocatorI& allocator = GetAllocator();

	mxDO(nwCREATE_OBJECT(
		_data
		, allocator
		, MyVoxelTerrainMgrData
		, allocator
		));

	//
	{
		MyVoxels& voxels = _data->voxels;
		mxDO(voxels.Initialize(
			engine_settings
			, app_settings
			, scene_clump
			, spatial_database
			, vxgi
			));
	}

	return ALL_OK;
}

void MyVoxelTerrainMgr::Shutdown()
{
	AllocatorI& allocator = GetAllocator();

	//
	{
		MyVoxels& voxels = _data->voxels;
		voxels.Shutdown();
	}


	//
	nwDESTROY_OBJECT(_data._ptr, allocator);
	_data = nil;
}

void MyVoxelTerrainMgr::UpdateOncePerFrame(
	const NwTime& game_time
	, MyGamePlayer& game_player
	, const MyAppSettings& app_settings
	, MyApp& game
	, const V3d& player_camera_position
	)
{
	MyVoxels& voxels = _data->voxels;

#if 0
	//
	voxels.voxel_engine->ApplySettings(
		app_settings.voxels.engine
		);
#endif

	//
	const V3d terrain_region_of_interest_position
		= app_settings.dbg_use_3rd_person_camera_for_terrain_RoI
		? app_settings.dbg_3rd_person_camera_position
		: player_camera_position
		;

	voxels.voxel_world->SetRegionOfInterest(terrain_region_of_interest_position);

	voxels.voxel_engine->UpdateOncePerFrame();

	//
	{
		voxels.debug_drawer.begin(
			Rendering::NwFloatingOrigin::GetDummy_TEMP_TODO_REMOVE()
			, game.renderer.GetCameraView()
			);

		voxels.voxel_world->DebugDraw(
			voxels.debug_drawer,
			game.settings.voxel_world_debug_settings.world_debug_draw_settings
			);

		voxels.debug_drawer.end();
	}

}

void MyVoxelTerrainMgr::ApplySettings(
	const VX5::RunTimeEngineSettings& voxel_engine_settings
	)
{
	_data->voxels.voxel_engine->ApplySettings(voxel_engine_settings);

	VX5::RunTimeWorldSettings	world_settings;
	world_settings.lod = voxel_engine_settings.lod;
	world_settings.mesher = voxel_engine_settings.mesher;
	world_settings.debug = voxel_engine_settings.debug;
	_data->voxels.voxel_world->ApplySettings(world_settings);
}

//void MyVoxelTerrainMgr::DrawDebugHUD()
//{
//	_data->voxels.
//}

VX5::EngineI& MyVoxelTerrainMgr::Test_GetVoxelEngine() const
{
	return *_data->voxels.voxel_engine;
}

VX5::OctreeWorld& MyVoxelTerrainMgr::Test_GetOctreeWorld() const
{
	VX5::OctreeWorld & octree_world = *((VX5::OctreeWorld*) _data->voxels.voxel_world._ptr);
	return octree_world;
}

void MyVoxelTerrainMgr::Test_CSG_Subtract(const V3d& pos) const
{
	VX5::EditingBrush	editing_brush;
	editing_brush.op.type = VX5::EditingBrush::CSG_Remove;
	//editing_brush.op.csg_remove.volume_material = VoxMat_Floor;

	editing_brush.shape.type = VX5::EditingBrush::Cube;
	editing_brush.shape.cube.center = pos;
	editing_brush.shape.cube.half_size = 4;

	_data->voxels.voxel_world->ApplyBrush(
		editing_brush
		);
}

void MyVoxelTerrainMgr::Test_CSG_Add(const V3d& pos) const
{
	VX5::EditingBrush	editing_brush;
	editing_brush.op.type = VX5::EditingBrush::CSG_Add;
	editing_brush.op.csg_add.volume_material = VoxMat_Floor;
	editing_brush.shape.type = VX5::EditingBrush::Cube;
	editing_brush.shape.cube.center = pos;
	editing_brush.shape.cube.half_size = 4;

	_data->voxels.voxel_world->ApplyBrush(
		editing_brush
		);
}

VX5::DebugViewI& MyVoxelTerrainMgr::GetDbgView() const
{
	VX5::MyDebugDraw& dbg_view = _data->voxels.debug_drawer;
	return dbg_view;
}

void MyVoxelTerrainMgr::ClearDebugLines()
{
	VX5::MyDebugDraw& dbg_view = _data->voxels.debug_drawer;
	dbg_view.VX_Lock();
	dbg_view.VX_Clear();
	dbg_view.VX_Unlock();
}

void MyVoxelTerrainMgr::regenerateVoxelTerrains()
{
	VX5::Utilities::reloadAllWorlds(_data->voxels.voxel_engine);
}

