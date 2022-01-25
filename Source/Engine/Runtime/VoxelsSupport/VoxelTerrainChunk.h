//TODO: move to VoxelsSupport
#pragma once

#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Scene/Entity.h>	// HRenderEntity

#include <Voxels/public/vx_forward_decls.h>
#include <Voxels/public/vx_engine.h>


struct NwCollisionObject;
class VoxelTerrainRenderer;


/// a renderable chunk
mxPREALIGN(16) struct VoxelTerrainChunk: NonCopyable
{
	/// for linking into a free list - don't touch!
	void *	zz__next_free;

	//
	enum
	{
		MAX_MESH_TYPES = 2,	// Triplanar-Textured + UV-Textured
	};
	//TFixedArray< MeshInstance*, MAX_MESH_TYPES >	models;
	Rendering::MeshInstance*	mesh_inst;

	/// octree node bounds are used for de-quantizing vertex positions, can be used for very conservative frustum culling
	/// A vertex's world-space position is computed relative to the chunk's minimal corner.
	CubeMLd		seam_octree_bounds_in_world_space;	// 32

	/// an offset applied to the mesh for rendering
	V3f			dbg_mesh_offset;	// 12

	//U32		nodeAdjacency;	//!< LoD status of neighboring chunks
	//float	morphOverride;	//!< for debugging CLOD

	//U64		debug_id;	// ChunkID

	/// handle to the AABB proxy for frustum culling
	Rendering::HRenderEntity	render_entity_handle;

	U32		voxel_bvh_node_handle;

	NwCollisionObject *	phys_obj;

	// 48 bytes on 32-bit

public:
	VoxelTerrainChunk()
	{
		mesh_inst = nil;
		dbg_mesh_offset = CV3f(0);
		voxel_bvh_node_handle = ~0;
		phys_obj = nil;
	}
};


namespace NVoxels
{

ERet BakeChunkCollisionMesh_AnyThread(
	const VX5::MeshBakerI::BakeCollisionMeshInputs& inputs
	, VX5::MeshBakerI::BakeCollisionMeshOutputs &outputs_
	);

ERet BakeChunkData_AnyThread(
	const VX5::MeshBakerI::BakeMeshInputs& inputs
	, VX5::MeshBakerI::BakeMeshOutputs &outputs_
	);

ERet CreateVoxelTerrainChunk(
	VoxelTerrainChunk *&new_voxel_terrain_chunk_
	, const VX5::ChunkLoadingInputs& args
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, NwClump& storage_clump
	);

void DestroyVoxelTerrainChunk(
	VoxelTerrainChunk* voxel_terrain_chunk
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, Rendering::SpatialDatabaseI& spatial_database
	, NwClump& storage_clump
	);

ERet LoadChunkMesh(
	VoxelTerrainChunk& voxel_terrain_chunk
	, const VX5::ChunkLoadingInputs& args
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, NwClump& storage_clump
	, Rendering::SpatialDatabaseI& spatial_database
	);

}//namespace NVoxels
