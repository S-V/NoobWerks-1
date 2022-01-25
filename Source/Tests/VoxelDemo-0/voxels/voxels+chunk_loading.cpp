//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Text/TextWriter.h>

#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>

#include <Voxels/private/Data/vx5_ChunkConfig.h>
#include <Voxels/private/Meshing/vx5_BuildMesh.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include "app.h"
#include "rendering/renderer.h"
#include "rendering/renderer_data.h"
#include "voxels/voxels.h"

#if GAME_CFG_WITH_PHYSICS
#include <Physics/Collision/TbQuantizedBIH.h>	
#endif // GAME_CFG_WITH_PHYSICS



#define DBG_PRINT_CHUNK_LOADING	(MX_DEBUG)

#define DBG_LOG_MEMPOOL	(0)


enum {
	MAX_LOD_FOR_COLLISION_PHYSICS = 0,
};


void MyVoxels::VX_BakeCollisionMesh_AnyThread(
	const BakeCollisionMeshInputs& inputs
	, BakeCollisionMeshOutputs &outputs_
	)
{
	VX_BakeCollisionMesh_AnyThread_Internal(
		inputs
		, outputs_
		);
}

void MyVoxels::VX_BakeChunkData_AnyThread(
	const BakeMeshInputs& inputs
	, BakeMeshOutputs &outputs_
	)
{
	VX_BakeChunkData_AnyThread_Internal(inputs, outputs_);
}

VX_ChunkUserData* MyVoxels::VX_LoadChunkUserData_MainThread(
	const VX5::ChunkLoadingInputs& args
	, VX5::WorldI & world
	)
{
	VX_ChunkUserData* mesh = nil;
	VX_LoadChunkData_MainThread_Internal(mesh, args);
	return mesh;
}

void MyVoxels::VX_UnloadChunkData_MainThread(
	VX_ChunkUserData* mesh
	, const VX5::ChunkID& chunk_id
	, VX5::WorldI& world
	)
{
	VX_UnloadChunkData_MainThread_Internal(mesh);
}

//void MyVoxels::VX_OnChunkWasCreated_MainThread(
//	const VX5::ChunkID& chunk_id
//	, const VX5::ChunkContents& chunk_contents
//	)
//{
////#if MX_DEBUG
////	LogStream(LL_Info),"\tVX_OnChunkCreated: ", chunk_id.ToCoordsAndLOD();
////#endif
//
//	//
//	MyApp& app = MyApp::Get();
//	MyGameRenderer& renderer = app.renderer;
//
//#if GAME_CFG_WITH_VOXEL_GI
//	Rendering::VoxelGrids& voxel_gi = renderer._data->voxel_gi;
//
//	//
//#if 0
//	if(chunk_id.ToCoordsAndLOD().w == VX5::FINEST_LOD)
//	{
//#if MX_DEBUG
//		LogStream(LL_Info),"\tVX_OnChunkCreated: ", chunk_id.ToCoordsAndLOD();
//#endif
//		//
//		if(chunk_contents.is_homogeneous)
//		{
//			if(chunk_contents.homogeneous_material_id != VX5::EMPTY_SPACE)
//			{
//				voxel_gi.brickmap
//			}
//		}
//		else
//		{
//			//
//		}
//	}
//#endif
//
//#endif // GAME_CFG_WITH_VOXEL_GI
//
//}

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/


ERet MyVoxels::VX_BakeCollisionMesh_AnyThread_Internal(
	const VX5::MeshBakerI::BakeCollisionMeshInputs& inputs
	, VX5::MeshBakerI::BakeCollisionMeshOutputs &outputs_
	)
{
	return NVoxels::BakeChunkCollisionMesh_AnyThread(inputs, outputs_);
}

ERet MyVoxels::VX_BakeChunkData_AnyThread_Internal(
									  const VX5::MeshBakerI::BakeMeshInputs& inputs
									  , VX5::MeshBakerI::BakeMeshOutputs &outputs_
									  )
{
	return NVoxels::BakeChunkData_AnyThread(inputs, outputs_);
}





static
ERet DbgSaveChunkSdfDataToFile(
							   const VX5::ChunkLoadingInputs& args
							   )
{
	const UInt4 coords = args.chunk_id.ToCoordsAndLOD();

	mxASSERT(coords.w == 0);
	mxASSERT(args.sdf_data.IsValid());

	String256	filename;
	Str::Format(filename, "R:/SDF_x%d_y%d_z%d.son", coords.x, coords.y, coords.z);

	//
	FileWriter	file_writer;
	mxDO(file_writer.Open(filename.c_str()));

	TextWriter	tw(file_writer);

	//
	mxSTATIC_ASSERT(
		VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM8
		||
		VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM16
		);

	const Rendering::VXGI::SDFValue* sdf_vals = (Rendering::VXGI::SDFValue*) args.sdf_data.ptr;

	//
	for( unsigned iVoxelZ = 0; iVoxelZ < args.sdf_grid_dim_1D; iVoxelZ++ )
	{
		tw.Emitf("\n; Z = %d:\n", iVoxelZ);

		for( unsigned iVoxelY = 0; iVoxelY < args.sdf_grid_dim_1D; iVoxelY++ )
		{
			for( unsigned iVoxelX = 0; iVoxelX < args.sdf_grid_dim_1D; iVoxelX++ )
			{
				const unsigned voxel_index_1D = iVoxelZ * (args.sdf_grid_dim_1D * args.sdf_grid_dim_1D)
					+ iVoxelY * (args.sdf_grid_dim_1D)
					+ iVoxelZ;

				const Rendering::VXGI::SDFValue sdf_val = sdf_vals[ voxel_index_1D ];

#if 1 // print ints
				
				tw.Emitf("\t%d ", sdf_val);

#else

#if VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM16
				const float01_t sdf_value01 = sdf_val * (1.0f / 65535.0f);
				tw.Emitf("\t%.2f ", sdf_value01*2.0f - 1.0f);
#else
#	error Unimpl
#endif

#endif

			}//For voxels along X axis.

			tw.Emitf("\t; Y = %d\n", iVoxelY);

		}//For voxels along Y axis.
	}//For voxels along Z axis.

	return ALL_OK;
}

ERet MyVoxels::VX_LoadChunkData_MainThread_Internal(
	VX_ChunkUserData *&new_chunk_userdata_
	, const VX5::ChunkLoadingInputs& args
	)
{
	//DBGOUT("Load mesh; LoD = %d", args.chunk_id.getLoD());

	//
	VoxelTerrainChunk* new_voxel_terrain_chunk = nil;

	//
	if(args.mesh_data.ptr)
	{
		mxDO(NVoxels::CreateVoxelTerrainChunk(
			new_voxel_terrain_chunk
			, args
			, _voxel_terrain_renderer
			, *_scene_clump
			));
		mxDO(NVoxels::LoadChunkMesh(
			*new_voxel_terrain_chunk
			, args
			, _voxel_terrain_renderer
			, *_scene_clump
			, *spatial_database
			));
	}


	//
	MyApp& app = MyApp::Get();
	MyGameRenderer& renderer = app.renderer;
	Rendering::VXGI::VoxelGrids& voxel_gi = renderer._data->voxel_grids;
	//


	// Re-voxelize on GPU / Update shadow maps

	//
#if GAME_CFG_WITH_VOXEL_GI
	{
		MyApp& app = MyApp::Get();
		MyGameRenderer& renderer = app.renderer;
		Rendering::VXGI::VoxelGrids& voxel_gi = renderer._data->voxel_grids;

		voxel_gi.OnRegionChanged(
			AABBf::fromOther(args.chunk_bounds_in_world_space.ToAabbMinMax())
			);

		//CreateVoxelBrickOnGPU(args.chunk_id, args.world);
	}
#endif // GAME_CFG_WITH_VOXEL_GI





	//
	if(args.sdf_data.IsValid())
	{

		//
#if 0
		if(args.chunk_id.u == VX5::ChunkID(2,2,2).u)
		{
			DBGOUT("!");
			voxel_engine_dbg_drawer.VX_Lock();
			voxel_engine_dbg_drawer.VX_AddAABB(
				args.chunk_bounds_in_world_space.ToAabbMinMax()
				, RGBAi::GREEN
				);
			voxel_engine_dbg_drawer.VX_Unlock();

			DbgSaveChunkSdfDataToFile(args);
		}
#endif


		if(!new_voxel_terrain_chunk) {
			mxDO(NVoxels::CreateVoxelTerrainChunk(
				new_voxel_terrain_chunk
				, args
				, _voxel_terrain_renderer
				, *_scene_clump
				));
		}

		Rendering::VXGI::BvhLeafNodeHandle	bvh_node_handle;
		mxDO(voxel_gi.voxel_BVH->AllocHeterogeneousLeaf(
			bvh_node_handle
			, args.chunk_bounds_in_world_space.ToAabbMinMax()
			, args.sdf_data.ptr
			, args.sdf_grid_dim_1D
			, *voxel_gi.brickmap
			));
		new_voxel_terrain_chunk->voxel_bvh_node_handle = bvh_node_handle.id;


#if 0
		if(args.chunk_id.ToCoordsAndLOD().w == 0)
		{
			DbgSaveChunkSdfDataToFile(args);
		}
#endif
	}
	else if(args.chunk_contents.IsFullySolid())
	{

#ifndef VOXEL_BVH_CFG__CREATE_SOLID_LEAFS
#error	Must define VOXEL_BVH_CFG__CREATE_SOLID_LEAFS!
#endif

#if VOXEL_BVH_CFG__CREATE_SOLID_LEAFS
		if(!new_voxel_terrain_chunk) {
			mxDO(NVoxels::CreateVoxelTerrainChunk(
				new_voxel_terrain_chunk
				, args
				, _voxel_terrain_renderer
				, *_scene_clump
				));
		}

		// the chunk is solid
		Rendering::VXGI::BvhLeafNodeHandle	bvh_node_handle;
		mxDO(voxel_gi.voxel_BVH->AllocSolidLeaf(
			bvh_node_handle
			, args.chunk_bounds_in_world_space.ToAabbMinMax()
			));
		new_voxel_terrain_chunk->voxel_bvh_node_handle = bvh_node_handle.id;
#endif // VOXEL_BVH_CFG__CREATE_SOLID_LEAFS

	}

//
#if DBG_LOG_MEMPOOL
	DBGOUT("[GAME] Alloced 0x%p", new_voxel_terrain_chunk);
#endif

	//DBGOUT("Loading chunk @ %p (curr num chunks: %u)",
	//	new_voxel_terrain_chunk,
	//	_voxel_terrain_renderer._chunkPool.NumValidItems()
	//	);

	//
	//mxASSERT(new_voxel_terrain_chunk->models.num() <= VoxelTerrainChunk::MAX_MESH_TYPES);

	if(new_voxel_terrain_chunk != nil)
	{
#if GAME_CFG_WITH_PHYSICS

		if( args.chunk_id.getLoD() <= MAX_LOD_FOR_COLLISION_PHYSICS )
		{
			//
			const NwBIH& bih = *(NwBIH*) mxAddByteOffset(
				args.meshData,
				sizeof(ChunkDataHeader_d)
				);

			const TSpan<BYTE>	bih_data(
				(BYTE*) &bih,
				args.dataSize - (chunk_data_header->collision_data_size + sizeof(ChunkDataHeader_d))
				);

			//
			NwVoxelTerrainCollisionSystem &	voxel_terrain_collision_system
				= physics_world.voxel_terrain_collision_system
				;

			//
			voxel_terrain_collision_system.AllocateCollisionObject(
				new_voxel_terrain_chunk->phys_obj
				, bih_data
				, args.seam_octree_bounds_in_world_space.min_corner
				, seam_octree_size_in_world_space
				, AABBf::fromOther(normalized_aabb_in_mesh_space)
				);

			//
			physics_world.m_dynamicsWorld.addCollisionObject(
				&new_voxel_terrain_chunk->phys_obj->bt_colobj()
				);
		}

#else

		new_voxel_terrain_chunk->phys_obj = nil;

#endif
	}//if

	//
	new_chunk_userdata_ = (VX_ChunkUserData*) new_voxel_terrain_chunk;

	return ALL_OK;
}

void MyVoxels::VX_UnloadChunkData_MainThread_Internal(
	VX_ChunkUserData* mesh
	)
{
	//DBGOUT("Unload mesh; LoD = %d", args.chunk_id.getLoD());
	//
	VoxelTerrainChunk * terrain_chunk = (VoxelTerrainChunk*) mesh;
	mxASSERT_PTR(terrain_chunk);
	//
#if DBG_LOG_MEMPOOL
	DBGOUT("[GAME] Releasing 0x%p", terrain_chunk);
#endif

	//DBGOUT("Unloading chunk @ %p (curr num chunks: %u)",
	//	terrain_chunk,
	//	_voxel_terrain_renderer._chunkPool.NumValidItems()
	//	);

	// Re-voxelize on GPU / Update shadow maps
//	spatial_database->OnRegionChanged(chunk_aabb_with_margin_WS);


	//
#if GAME_CFG_WITH_VOXEL_GI
	{
		MyApp& app = MyApp::Get();
		MyGameRenderer& renderer = app.renderer;
		Rendering::VXGI::VoxelGrids& voxel_gi = renderer._data->voxel_grids;

		if(terrain_chunk->voxel_bvh_node_handle != ~0)
		{
			voxel_gi.voxel_BVH->DeleteLeaf(
				Rendering::VXGI::BvhLeafNodeHandle::MakeHandle(terrain_chunk->voxel_bvh_node_handle)
				, *voxel_gi.brickmap
				);
		}
		////
		//Rendering::VXGI::BrickMap::BrickOperation	new_pending_operations;
		//new_pending_operations.ptr = terrain_chunk;
		//new_pending_operations.type = Rendering::VXGI::BrickMap::BrickOperation::Destroy;
		//voxel_gi.brickmap.pending_operations.add(new_pending_operations);
	}
#endif // GAME_CFG_WITH_VOXEL_GI


	//
#if GAME_CFG_WITH_PHYSICS
	if( args.chunk_id.getLoD() <= MAX_LOD_FOR_COLLISION_PHYSICS )
	{
		if( NwCollisionObject *	colObject = terrain_chunk->phys_obj )
		{
			physics_world.m_dynamicsWorld.removeCollisionObject(
				&colObject->bt_colobj()
				);

			//
			NwVoxelTerrainCollisionSystem &	voxel_terrain_collision_system
				= physics_world.voxel_terrain_collision_system
				;

			//
			voxel_terrain_collision_system.ReleaseCollisionObject(
				colObject
				);

			//
			terrain_chunk->phys_obj = nil;
		}
	}
#endif // GAME_CFG_WITH_PHYSICS

	NVoxels::DestroyVoxelTerrainChunk(
		terrain_chunk
		, _voxel_terrain_renderer
		, *spatial_database
		, *_scene_clump
		);
}
