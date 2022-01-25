//
#include "stdafx.h"
#pragma hdrstop

#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>

#include <Voxels/private/Data/vx5_ChunkConfig.h>
#include <Voxels/private/Meshing/vx5_BuildMesh.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include "app.h"
#include "rendering/renderer.h"
#include "rendering/renderer_data.h"
#include "voxels/voxels.h"


ERet MyVoxels::CreateVoxelBrickOnGPU(
	const VX5::ChunkID& chunk_id
	, VX5::WorldI& world
	)
{
	MyApp& app = MyApp::Get();
	MyGameRenderer& renderer = app.renderer;
	Rendering::VXGI::VoxelGrids& voxel_gi = renderer._data->voxel_grids;
	//

	////
	//Rendering::VXGI::BrickMap::BrickOperation	new_pending_operations;
	//new_pending_operations.ptr = new_voxel_terrain_chunk;
	//new_pending_operations.type = Rendering::VXGI::BrickMap::BrickOperation::Create;
	//voxel_gi.brickmap.pending_operations.add(new_pending_operations);
	// 
	UNDONE;
#if 0
	{
		float	chunk_sdf[nwBRICK_VOLUME];

		mxDO(world.GenerateChunkSDF_Blocking(
			chunk_id
			, nwBRICK_DIM
			, chunk_sdf
			, sizeof(chunk_sdf)
			));

		//
		Rendering::VXGI::BrickHandle new_brick_handle;
		mxDO(voxel_gi.brickmap.AllocBrick(new_brick_handle));

		voxel_gi.brickmap.UpdateBrick(
			new_brick_handle
			, chunk_sdf
			);
	}
#endif

	return ALL_OK;
}

void MyVoxels::DestroyVoxelBrickOnGPU(
	const VX5::ChunkID& chunk_id
	, VX5::WorldI& world
	)
{
	//
}
