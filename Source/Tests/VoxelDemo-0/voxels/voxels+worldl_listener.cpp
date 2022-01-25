//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Util/Tweakable.h>

#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SceneRenderer.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include "app.h"
#include "app_settings.h"
#include "rendering/renderer.h"
#include "rendering/renderer_data.h"
#include "voxels/voxels.h"


void MyVoxels::VX_OnChunkWasCreated_MainThread(
	const VX5::ChunkID& chunk_id
	, const VX5::ChunkLoadingInputs& loaded_chunk_data
	, VX5::WorldI& world
	)
{
#if 0
	if(chunk_id.ToCoordsAndLOD().w > 0) {
		// skip coarse LoDs
		return;
	}

	//
	MyApp& app = MyApp::Get();
	MyGameRenderer& renderer = app.renderer;
	Rendering::VXGI::VoxelGrids& voxel_gi = renderer._data->voxel_grids;
	//

	//
	if(loaded_chunk_data.contents.is_homogeneous)
	{
		if(loaded_chunk_data.contents.homogeneous_material_id != VX5::EMPTY_SPACE)
		{
			// the chunk is solid
			Rendering::VXGI::BvhNodeHandle	bvh_node_handle;
			voxel_gi.voxel_BVH->AllocSolidLeaf(bvh_node_handle);

			//
		}
		else
		{
			// the chunk is empty
		}
	}
	else
	{
		mxASSERT(loaded_chunk_data.sdf_format == VX5::Distance_Float32);
		mxASSERT(loaded_chunk_data.sdf_data != nil && loaded_chunk_data.sdf_data_size > 0);

		const float* src_sdf_values_f = (float*) loaded_chunk_data.sdf_data;


		DBGOUT("");

		////
		//Rendering::VXGI::BrickHandle new_brick_handle;
		//mxDO(voxel_gi.brickmap.AllocBrick(new_brick_handle));

		//voxel_gi.brickmap.UpdateBrick(
		//	new_brick_handle
		//	, sdf_values
		//	, grid_info.length
		//	);
	}
#endif
}

void MyVoxels::VX_OnChunkWillBeDestroyed_MainThread(
	const VX5::ChunkID& chunk_id
	, VX5::WorldI& world
	)
{
	//DBGOUT("");
}

void MyVoxels::VX_OnChunkContentsDidChange(
	const VX5::ChunkID& chunk_id
	, const VX5::ChunkLoadingInputs& loaded_chunk_data
	, VX5::WorldI& world
	)
{
	DBGOUT("");
}
