#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>

#include <VoxelsSupport/VoxelTerrainChunk.h>
#include <VoxelsSupport/VoxelTerrainMaterials.h>
#include <VoxelsSupport/VoxelTerrainRenderer.h>

//TODO: remove
#include <Voxels/private/Meshing/vx5_BuildMesh.h>


namespace NVoxels
{

namespace
{

typedef Vertex_DLOD VoxelTerrainStandardVertexT;
typedef DrawVertex	VoxelTerrainTexturedVertexT;



UShort3 Encode_XYZ01_to_UShort3( const V3f& p ) {
	return CUShort3(
		TQuantize<16>::EncodeUNorm( p.x ),
		TQuantize<16>::EncodeUNorm( p.y ),
		TQuantize<16>::EncodeUNorm( p.z )
	);
}



#pragma pack (push,1)

/// for voxel terrain meshes
struct ChunkDataHeader_d
{
	U32		num_meshes;

	// 0 if the chunk doesn't have a collision mesh (e.g. for coarser LoDs)
	U32		collision_data_size;

	// bounding box of normalized (in seam space), lies within [0..1]
	AABBf	normalized_seam_mesh_aabb;	//24

	// ...followed by meshes.
};
mxSTATIC_ASSERT(sizeof(ChunkDataHeader_d) == 32);

///
struct ChunkMeshHeader_d
{
	U16		is_textured;
	U16		num_textured_submeshes;
	U32		num_vertices;	//4 total number of vertices
	U32		num_indices;	//4 total number of indices
	U32		total_size;
	// ...followed by vertices and indices.
};
mxSTATIC_ASSERT(sizeof(ChunkMeshHeader_d) == 16);

///
struct SubmeshHeader_d
{
	U32	start_index;	//!< Offset of the first index
	U32	index_count;	//!< Number of indices
	U32	texture_id;
	U16	pad[2];
};
mxSTATIC_ASSERT(sizeof(SubmeshHeader_d) == 16);

#pragma pack (pop)




namespace VoxelTerrainUtils
{
	struct MeshInfo
	{
		U32		vertex_data_size;
		U32		index_data_size;
		bool	use_32_bit_index_buffer;
		U32		total_data_size_with_header;
	};

	void computeMeshInfo(
		MeshInfo &mesh_info_
		, const U32 num_vertices
		, const U32 vertex_stride
		, const U32 num_triangles
		, const U32 num_submeshes
		)
	{
		const U32 vertex_data_size = num_vertices * vertex_stride;
		//
		const U32 num_indices = num_triangles * 3u;
		const bool use_32_bit_index_buffer = ( num_vertices > MAX_UINT16 );
		const U32 index_data_size = num_indices * ( use_32_bit_index_buffer ? sizeof(U32) : sizeof(U16) );
		//
		mesh_info_.vertex_data_size = vertex_data_size;
		mesh_info_.index_data_size = index_data_size;
		mesh_info_.use_32_bit_index_buffer = use_32_bit_index_buffer;
		mesh_info_.total_data_size_with_header
			= sizeof(ChunkMeshHeader_d)
			+ num_submeshes * sizeof(SubmeshHeader_d)
			+ vertex_data_size + index_data_size
			;
	}

	static
	void copyIndices(
		void *dst_indices
		, const bool use_32_bit_vertex_indices
		, const TSpan< const UInt3 >& triangles
		)
	{
		if( use_32_bit_vertex_indices )
		{
			for( UINT iTriangle = 0; iTriangle < triangles._count; iTriangle++ )
			{
				const UInt3& triangle = triangles._data[ iTriangle ];
				((U32*) dst_indices)[ iTriangle*3 + 0 ] = triangle.a[0];
				((U32*) dst_indices)[ iTriangle*3 + 1 ] = triangle.a[1];
				((U32*) dst_indices)[ iTriangle*3 + 2 ] = triangle.a[2];
			}
		}
		else
		{
			for( UINT iTriangle = 0; iTriangle < triangles._count; iTriangle++ )
			{
				const UInt3& triangle = triangles._data[ iTriangle ];
				((U16*) dst_indices)[ iTriangle*3 + 0 ] = triangle.a[0];
				((U16*) dst_indices)[ iTriangle*3 + 1 ] = triangle.a[1];
				((U16*) dst_indices)[ iTriangle*3 + 2 ] = triangle.a[2];
			}
		}
	}

	static
	ERet _compileStandardMesh(
		const VX5::OutputStandardMesh& standard_mesh
		, const MeshInfo& mesh_info
		, void * const dst_write_ptr
		, const VX5::MeshBakerI::BakeMeshInputs& inputs
		)
	{
		const U32 num_vertices = standard_mesh.vertices.num();
		const U32 num_triangles = standard_mesh.triangles.num();
		const U32 num_indices = num_triangles * 3u;

		//
		ChunkMeshHeader_d *dst_header = (ChunkMeshHeader_d*) dst_write_ptr;
		dst_header->is_textured = 0;
		dst_header->num_vertices = num_vertices;
		dst_header->num_indices = num_indices;
		dst_header->total_size = mesh_info.total_data_size_with_header;

		//
		VoxelTerrainStandardVertexT* dst_vertices = (VoxelTerrainStandardVertexT*) ( dst_header + 1 );

		for( UINT iVertex = 0; iVertex < num_vertices; iVertex++ )
		{
			const VX5::OutputStandardMesh::Vertex& src_vertex = standard_mesh.vertices._data[ iVertex ];

			VoxelTerrainStandardVertexT &dst_vertex = dst_vertices[ iVertex ];

			const UShort3 packed_pos = Encode_XYZ01_to_UShort3( src_vertex.xyz );
			dst_vertex.xy = (packed_pos.y << 16) | packed_pos.x;

			dst_vertex.z_and_mat = (src_vertex.material_id << 16) | packed_pos.z;

			dst_vertex.N = PackNormal( src_vertex.N );

			dst_vertex.color = src_vertex.color.rgba;
		}

		//
		void* dst_indices = dst_vertices + num_vertices;
		copyIndices( dst_indices, mesh_info.use_32_bit_index_buffer, Arrays::GetSpan(standard_mesh.triangles) );

		return ALL_OK;
	}

	static
	ERet _compileTexturedMesh(
		const VX5::OutputTexturedMesh& textured_mesh
		, const MeshInfo& mesh_info
		, void * const dst_write_ptr
		)
	{
		const U32 num_submeshes = textured_mesh.batches.num();

		const U32 num_vertices = textured_mesh.vertices.num();
		const U32 num_triangles = textured_mesh.triangles.num();
		const U32 num_indices = num_triangles * 3u;

		//
		ChunkMeshHeader_d *dst_header = (ChunkMeshHeader_d*) dst_write_ptr;
		dst_header->is_textured = 1;
		dst_header->num_textured_submeshes = num_submeshes;
		dst_header->num_vertices = num_vertices;
		dst_header->num_indices = num_indices;
		dst_header->total_size = mesh_info.total_data_size_with_header;

		//
		SubmeshHeader_d* dst_submeshes = (SubmeshHeader_d*) ( dst_header + 1 );

		for( UINT iSubmesh = 0; iSubmesh < num_submeshes; iSubmesh++ )
		{
			const VX5::OutputTexturedMesh::Batch& src_batch = textured_mesh.batches._data[ iSubmesh ];
			mxASSERT(src_batch.texture_id.IsValid());
			mxASSERT(src_batch.triangle_count > 0);
			mxASSERT(src_batch.vertex_count > 0);

			SubmeshHeader_d &dst_submesh = dst_submeshes[ iSubmesh ];

			dst_submesh.start_index = src_batch.start_triangle * 3;
			dst_submesh.index_count = src_batch.triangle_count * 3;
			dst_submesh.texture_id = src_batch.texture_id.id;
		}

		//
		VoxelTerrainTexturedVertexT* dst_vertices = (VoxelTerrainTexturedVertexT*) ( dst_submeshes + num_submeshes );

		for( UINT iVertex = 0; iVertex < num_vertices; iVertex++ )
		{
			const VX5::OutputTexturedMesh::Vertex& src_vertex = textured_mesh.vertices._data[ iVertex ];

			VoxelTerrainTexturedVertexT &dst_vertex = dst_vertices[ iVertex ];

			dst_vertex.xyz = src_vertex.xyz;
			dst_vertex.uv.v = src_vertex.TC.q;
			dst_vertex.N = PackNormal( src_vertex.N );
		}

		//
		void* dst_indices = dst_vertices + num_vertices;
		copyIndices( dst_indices, mesh_info.use_32_bit_index_buffer, Arrays::GetSpan(textured_mesh.triangles) );

		return ALL_OK;
	}


}//VoxelTerrainUtils

}//namespace


#pragma region "Baking"

ERet BakeChunkCollisionMesh_AnyThread(
	const VX5::MeshBakerI::BakeCollisionMeshInputs& inputs
	, VX5::MeshBakerI::BakeCollisionMeshOutputs &outputs_
	)
{
//
	NwBlobWriter	blog_writer(outputs_.blob);
	mxDO(blog_writer.Put(ChunkDataHeader_d()));

#if GAME_CFG_WITH_PHYSICS
	if( inputs.chunk_id.getLoD() <= MAX_LOD_FOR_COLLISION_PHYSICS )
	{
		//
		struct VoxelCollisionTriangleMesh: NwBIHBuilder::TriangleMeshI
		{
			const VX5::BuildMesh &	src_mesh;
		public:
			VoxelCollisionTriangleMesh(
				const VX5::BuildMesh& src_mesh
				) : src_mesh(src_mesh)
			{}
			virtual const StridedTrianglesT GetTriangles() const override
			{
				return StridedTrianglesT::FromArray(
					src_mesh.triangles
					);
			}
			virtual const StridedPositionsT GetVertexPositions() const override
			{
				return StridedPositionsT::FromArray(
					src_mesh.vertices
					);
			}
		};
		VoxelCollisionTriangleMesh	voxel_col_mesh(
			inputs.build_mesh
			);

		//
		NwBIHBuilder	bih_builder( inputs.scratchpad );
		NwBIHBuilder::Options	build_options;
		mxDO(bih_builder.Build(
			voxel_col_mesh
			, build_options
			, inputs.scratchpad
			));

		//
		mxDO(bih_builder.SaveToBlob( outputs_.blob ));
	}
#endif // GAME_CFG_WITH_PHYSICS

	//
	ChunkDataHeader_d* header = (ChunkDataHeader_d*) outputs_.blob.raw();
	header->collision_data_size = outputs_.blob.rawSize();

	return ALL_OK;
}

ERet BakeChunkData_AnyThread(
	const VX5::MeshBakerI::BakeMeshInputs& inputs
	, VX5::MeshBakerI::BakeMeshOutputs &outputs_
	)
{
	using namespace VoxelTerrainUtils;

	const VX5::ChunkMesherOutput& chunk_mesher_output = inputs.chunk_mesher_output;

	//
	MeshInfo	mesh_infos[ VX5::ChunkMesherOutput::MAX_MESHES ];
	int			mesh_infos_count = 0;

	//
	if( chunk_mesher_output.standard_mesh.IsValid() )
	{
		computeMeshInfo(
			mesh_infos[ mesh_infos_count++ ]
			, chunk_mesher_output.standard_mesh.vertices.num()
			, sizeof(VoxelTerrainStandardVertexT)
			, chunk_mesher_output.standard_mesh.triangles.num()
			, 0
			);
	}

	//
	if( chunk_mesher_output.textured_mesh.IsValid() )
	{
		computeMeshInfo(
			mesh_infos[ mesh_infos_count++ ]
			, chunk_mesher_output.textured_mesh.vertices.num()
			, sizeof(VoxelTerrainTexturedVertexT)
			, chunk_mesher_output.textured_mesh.triangles.num()
			, chunk_mesher_output.textured_mesh.batches.num()
			);
	}

	//
	mxENSURE(mesh_infos_count > 0, ERR_INVALID_PARAMETER, "");

	//
	U32	mesh_data_offsets[ VX5::ChunkMesherOutput::MAX_MESHES ];
	mesh_data_offsets[ 0 ] = outputs_.blob.rawSize();

	for( int i = 1; i < mesh_infos_count; i++ )
	{
		const U32		prev_mesh_offset = mesh_data_offsets[ i - 1 ];
		const MeshInfo&	prev_mesh_info = mesh_infos[ i - 1 ];
		const U32		unaligned_offset_so_far = prev_mesh_offset + prev_mesh_info.total_data_size_with_header;
		mesh_data_offsets[ i ] = tbALIGN16( unaligned_offset_so_far );
	}

	//
	const U32	total_data_size
		= mesh_data_offsets[ mesh_infos_count - 1 ]
		+ mesh_infos[ mesh_infos_count - 1 ].total_data_size_with_header
		;

	//
	NwBlob &dst_data_blob_ = outputs_.blob;

	//
	NwBlobWriter	blob_writer( dst_data_blob_ );
	mxDO(blob_writer.reserveMoreBufferSpace( total_data_size, Arrays::ResizeExactly ));

	//
	U32	num_written_meshes = 0;

	//
	if( chunk_mesher_output.standard_mesh.IsValid() )
	{
		mxDO(_compileStandardMesh(
			chunk_mesher_output.standard_mesh
			, mesh_infos[ num_written_meshes ]
			, mxAddByteOffset( dst_data_blob_.raw(), mesh_data_offsets[ num_written_meshes ] )
			, inputs
			));
		++num_written_meshes;
	}

	//
	if( chunk_mesher_output.textured_mesh.IsValid() )
	{
		mxDO(_compileTexturedMesh(
			chunk_mesher_output.textured_mesh
			, mesh_infos[ num_written_meshes ]
			, mxAddByteOffset( dst_data_blob_.raw(), mesh_data_offsets[ num_written_meshes ] )
			));
		++num_written_meshes;
	}

	mxASSERT(1 == num_written_meshes);

	//
	{
		ChunkDataHeader_d*	header = (ChunkDataHeader_d*) dst_data_blob_.raw();
		header->num_meshes = num_written_meshes;
		header->normalized_seam_mesh_aabb = chunk_mesher_output.local_bounds;
	}

	return ALL_OK;
}

#pragma endregion





#pragma region "Loading"

namespace
{

static
const TSpan< const SubmeshHeader_d > getSubmeshes( const ChunkMeshHeader_d* mesh_header )
{
	if( mesh_header->is_textured )
	{
		const SubmeshHeader_d *	submeshes = (SubmeshHeader_d*) ( mesh_header + 1 );
		return TSpan< const SubmeshHeader_d >( submeshes, mesh_header->num_textured_submeshes );
	}
	else
	{
		return TSpan< const SubmeshHeader_d >();
	}
}

static
ERet createMeshInstance(
						Rendering::MeshInstance **new_mesh_instance_
						, const ChunkMeshHeader_d* mesh_header
						, const TbVertexFormat& vertex_format
						, const AABBf& local_bounds
						, const VoxelTerrainMaterials& material_table
						, NwClump& storage
						)
{
	Rendering::NwMesh	* new_mesh;
	mxDO(storage.New( new_mesh ));

	//
	const TSpan< const SubmeshHeader_d >	submeshes = getSubmeshes( mesh_header );

	//
	const void* mesh_vertex_data = mxAddByteOffset( mesh_header + 1, submeshes.rawSize() );
	const U32 vertex_stride = vertex_format.GetInstanceSize();
	const U32 vertex_data_size = mesh_header->num_vertices * vertex_stride;
	//
	const void* mesh_index_data = mxAddByteOffset( mesh_vertex_data, vertex_data_size );
	const bool use_32_bit_index_buffer = ( mesh_header->num_vertices > MAX_UINT16 );
	const U32 index_stride = use_32_bit_index_buffer ? sizeof(U32) : sizeof(U16);
	const U32 index_data_size = mesh_header->num_indices * index_stride;

	//
	new_mesh->geometry_buffers.updateWithRawData(
		mesh_vertex_data, vertex_data_size
		, mesh_index_data, index_data_size
		);

	//
	if( use_32_bit_index_buffer ) {
		new_mesh->flags |= Rendering::MESH_USES_32BIT_IB;
	}

	//
	new_mesh->vertex_count = mesh_header->num_vertices;
	new_mesh->index_count = mesh_header->num_indices;

	new_mesh->vertex_format = &vertex_format;

	new_mesh->topology = NwTopology::TriangleList;

	new_mesh->local_aabb = local_bounds;

	//
	Rendering::MeshInstance	* new_mesh_instance;
	mxDO(Rendering::MeshInstance::ñreateFromMesh(
		new_mesh_instance
		, new_mesh
		, storage
		));

	//
	if( !mesh_header->is_textured )
	{
		Rendering::Submesh	submesh;
		{
			submesh.start_index = 0;
			submesh.index_count = mesh_header->num_indices;
		}
		new_mesh->submeshes.add( submesh );
		//
		Rendering::Material* mat_instance = material_table.triplanar_material;
		mxASSERT_PTR(mat_instance);
		new_mesh_instance->materials.add( mat_instance );
	}
	else
	{
		mxASSERT(!submeshes.IsEmpty());

		mxDO(new_mesh->submeshes.setNum( submeshes._count ));
		mxDO(new_mesh_instance->materials.setNum( submeshes._count ));

		//
		for( UINT iSubmesh = 0; iSubmesh < submeshes._count; iSubmesh++ )
		{
			const SubmeshHeader_d&	src_submesh = submeshes._data[ iSubmesh ];

			//
			Rendering::Submesh	new_submesh;
			{
				new_submesh.start_index = src_submesh.start_index;
				new_submesh.index_count = src_submesh.index_count;
			}
			new_mesh->submeshes._data[ iSubmesh ] = new_submesh;

			//
			Rendering::Material* mat_instance = material_table.textured_materials[ src_submesh.texture_id ];
			mxASSERT_PTR(mat_instance);
			new_mesh_instance->materials[ iSubmesh ] = mat_instance;
		}
	}

	//
	*new_mesh_instance_ = new_mesh_instance;

	return ALL_OK;
}

}//namespace

ERet CreateVoxelTerrainChunk(
	VoxelTerrainChunk *&new_voxel_terrain_chunk_
	, const VX5::ChunkLoadingInputs& args
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, NwClump& storage_clump
	)
{
	//nwCREATE_OBJECT(new_voxel_terrain_chunk, MemoryHeaps::global(), VoxelTerrainChunk);

	VoxelTerrainChunk* new_voxel_terrain_chunk = (VoxelTerrainChunk*) voxel_terrain_renderer._chunkPool.AllocateItem();
	mxENSURE(new_voxel_terrain_chunk, ERR_OUT_OF_MEMORY, "");

	new(new_voxel_terrain_chunk) VoxelTerrainChunk();

	new_voxel_terrain_chunk->mesh_inst = nil;

	mxSTATIC_ASSERT(VoxelTerrainChunk::MAX_MESH_TYPES <= VX5::ChunkMesherOutput::MAX_MESHES);

	new_voxel_terrain_chunk_ = new_voxel_terrain_chunk;

	return ALL_OK;
}

void DestroyVoxelTerrainChunk(
	VoxelTerrainChunk* voxel_terrain_chunk
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, Rendering::SpatialDatabaseI& spatial_database
	, NwClump& storage_clump
	)
{
	//
	if( voxel_terrain_chunk->render_entity_handle.IsValid() )
	{
		spatial_database.RemoveEntity( voxel_terrain_chunk->render_entity_handle );
		voxel_terrain_chunk->render_entity_handle.SetNil();
	}

	//
	if(voxel_terrain_chunk->mesh_inst)
	{
		Rendering::MeshInstance* model = voxel_terrain_chunk->mesh_inst;
		{
			Rendering::NwMesh *	mesh = model->mesh;
			mesh->release();
			storage_clump.Destroy( mesh );
		}
		storage_clump.Destroy( model );
	}

	voxel_terrain_chunk->~VoxelTerrainChunk();

	//nwDESTROY_OBJECT(terrain_chunk, MemoryHeaps::global());
	voxel_terrain_renderer._chunkPool.ReleaseItem( voxel_terrain_chunk );
}


ERet LoadChunkMesh(
	VoxelTerrainChunk& voxel_terrain_chunk
	, const VX5::ChunkLoadingInputs& args
	, VoxelTerrainRenderer& voxel_terrain_renderer
	, NwClump& storage_clump
	, Rendering::SpatialDatabaseI& spatial_database
	)
{
	mxASSERT(nil == voxel_terrain_chunk.mesh_inst);
	mxASSERT(args.mesh_data.IsValid());

	//
	const ChunkDataHeader_d* chunk_data_header = (ChunkDataHeader_d*) args.mesh_data.ptr;

	//
	{
		U32	num_remaining_meshes = chunk_data_header->num_meshes;
		mxENSURE(1 == num_remaining_meshes, ERR_INVALID_PARAMETER, "");

		const void* mesh_data_start = mxAddByteOffset(
			args.mesh_data.ptr,
			chunk_data_header->collision_data_size
			);

		const ChunkMeshHeader_d* mesh_header = ( ChunkMeshHeader_d* ) mesh_data_start;
		while( num_remaining_meshes )
		{
			Rendering::MeshInstance	*new_mesh_instance;

			if( !mesh_header->is_textured )
			{
				mxDO(createMeshInstance(
					&new_mesh_instance
					, mesh_header
					, VoxelTerrainStandardVertexT::metaClass()
					, chunk_data_header->normalized_seam_mesh_aabb
					, voxel_terrain_renderer.materials
					, storage_clump
					));
			}
			else
			{
				mxDO(createMeshInstance(
					&new_mesh_instance
					, mesh_header
					, VoxelTerrainTexturedVertexT::metaClass()
					, chunk_data_header->normalized_seam_mesh_aabb
					, voxel_terrain_renderer.materials
					, storage_clump
					));
			}

			//
			//voxel_terrain_chunk.models.add( new_mesh_instance );
			voxel_terrain_chunk.mesh_inst = new_mesh_instance;

			//
			const U32	aligned_mesh_size = tbALIGN16( mesh_header->total_size );

			mesh_header = ( ChunkMeshHeader_d* ) mxAddByteOffset(
				mesh_header
				, aligned_mesh_size
				);

			--num_remaining_meshes;
		}
	}

	////
	//voxel_terrain_chunk.debug_id = args.chunk_id.M;
	//static_assert(
	//	sizeof(voxel_terrain_chunk.debug_id) >= sizeof(args.chunk_id)
	//	, _
	//	);

	//
	//const AABBf seam_octree_aabb_world = args.seam_octree_aabb_world;
	const double	seam_octree_size_in_world_space = args.seam_octree_bounds_in_world_space.side_length;

	//voxel_terrain_chunk.AabbMins = seam_octree_aabb_world.min_corner;
	//voxel_terrain_chunk.AabbSize = chunk_size_in_world_space;

	//
	const AABBd	normalized_aabb_in_mesh_space = AABBd::fromOther( chunk_data_header->normalized_seam_mesh_aabb );

	// scale to world size
	const AABBd	scaled_aabb_in_chunk_space = {
		normalized_aabb_in_mesh_space.min_corner * seam_octree_size_in_world_space,
		normalized_aabb_in_mesh_space.max_corner * seam_octree_size_in_world_space
	};

	// translate to world space
	const AABBd mesh_aabb_in_world_space = scaled_aabb_in_chunk_space
		.shifted( args.seam_octree_bounds_in_world_space.min_corner )
		;


	voxel_terrain_chunk.seam_octree_bounds_in_world_space = args.seam_octree_bounds_in_world_space;
	voxel_terrain_chunk.dbg_mesh_offset = CV3f(0);

	//
	voxel_terrain_chunk.render_entity_handle = spatial_database.AddEntity(
		&voxel_terrain_chunk,
		Rendering::RE_VoxelTerrainChunk,
		mesh_aabb_in_world_space
		);

	return ALL_OK;
}

#pragma endregion

}//namespace NVoxels
