#pragma once


#include "common.h"


///
class MyVoxelTerrainMgr: NonCopyable
{
	struct MyVoxelTerrainMgrData;

	TPtr<MyVoxelTerrainMgrData> 	_data;

public:

	ERet Initialize(
		const NEngine::LaunchConfig & engine_settings
		, const MyAppSettings& app_settings
		, NwClump* scene_clump
		, Rendering::SpatialDatabaseI* spatial_database
		, const Rendering::VXGI::VoxelGrids* vxgi
		);

	void Shutdown();

	void UpdateOncePerFrame(
		const NwTime& game_time
		, MyGamePlayer& game_player
		, const MyAppSettings& app_settings
		, MyApp& game
		, const V3d& player_camera_position
		);

	void ApplySettings(
		const VX5::RunTimeEngineSettings& voxel_engine_settings
		);

	//void DrawDebugHUD();

public:	// Testing

	VX5::EngineI& Test_GetVoxelEngine() const;
	VX5::OctreeWorld& Test_GetOctreeWorld() const;

	void Test_CSG_Subtract(const V3d& pos) const;
	void Test_CSG_Add(const V3d& pos) const;

public:	// Debugging

	VX5::DebugViewI& GetDbgView() const;
	
	void ClearDebugLines();

	void debugDrawVoxels(
		const Rendering::NwFloatingOrigin& floating_origin
		, const Rendering::NwCameraView& scene_view
		);

	void regenerateVoxelTerrains();
};
