#pragma once

#include <Rendering/Public/Core/Mesh.h>

#include <Voxels/public/vx5.h>


namespace VXExt
{

///// 
//enum EWorldDebugFlags
//{
//	/// draw LoD octrees/clipmaps?
//	DbgFlag_Draw_LoD_Octree = BIT(2),
//};
//mxDECLARE_FLAGS( EWorldDebugFlags, U32, WorldDebugFlagsT );

///
struct WorldDebugSettingsExt: CStruct
{
	//WorldDebugFlagsT	flags;

	VX5::WorldDebugDrawSettings	world_debug_draw_settings;
	
public:
	mxDECLARE_CLASS(WorldDebugSettingsExt, CStruct);
	mxDECLARE_REFLECTION;

	WorldDebugSettingsExt();
};


ERet ImGui_DrawVoxelEngineDebugHUD(
	VX5::WorldI& voxel_world
	, WorldDebugSettingsExt & world_debug_settings

	, const V3d& eye_pos
	, const V3f& look_direction
	, VX5::DebugViewI& dbgview
	);

}//namespace VXExt
