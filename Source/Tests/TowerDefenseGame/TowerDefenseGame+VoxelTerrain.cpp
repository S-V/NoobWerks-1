#include <complex>

#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t


#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Assets/AssetBundle.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/GraphicsSystem.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Renderer/private/render_world.h>	// G_VoxelTerrain

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/Meshok/Octree.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/VoxelEngine/VoxelizedMesh.h>


//
#include <Developer/RunTimeCompiledCpp.h>

#include <DirectXMath/DirectXMath.h>


#include <Voxels/Common/vx5_LinearOctrees.h>	// Morton32_getRootAABB()
#include <Voxels/Data/vx5_chunk_data.h>
#include <Voxels/Base/Math/vx5_Quadrics.h>
#include <Voxels/Common/vx5_debug.h>
#include <Voxels/Meshing/vx5_BuildMesh.h>

#include <Scripting/Scripting.h>
#include <Scripting/FunctionBinding.h>
// Lua G(L) crap
#undef G


#include "TowerDefenseGame.h"

#include "RCCpp_WeaponBehavior.h"



mxDEFINE_CLASS(DevVoxelTools);
mxBEGIN_REFLECTION(DevVoxelTools)
	mxMEMBER_FIELD( small_geomod_sphere ),
	mxMEMBER_FIELD( small_add_sphere ),

	mxMEMBER_FIELD( large_geomod_sphere ),
	mxMEMBER_FIELD( large_add_sphere ),

	mxMEMBER_FIELD( small_add_cube ),
	mxMEMBER_FIELD( small_geomod_cube ),

	mxMEMBER_FIELD( smooth ),
	mxMEMBER_FIELD( paint_material ),
mxEND_REFLECTION;
DevVoxelTools::DevVoxelTools()
{
	//
}

void DevVoxelTools::resetAllToDefaultSettings(
	const float chunk_length_units_in_world_space
	)
{
	const float chunkSizeAtLoD0 = chunk_length_units_in_world_space;
	float radius = chunkSizeAtLoD0*0.3;//0.4;//4f;

	float microCsgRadius = chunkSizeAtLoD0 * 0.1;
	float tinyCsgRadius = chunkSizeAtLoD0 * 0.2;
	float smallCsgRadius = chunkSizeAtLoD0 * 0.5;
	//smallCsgRadius=tinyCsgRadius;
	float mediumCsgRadius = chunkSizeAtLoD0 * 0.8;
	float largeCsgRadius = chunkSizeAtLoD0 * 1.2;


	//
	{
		small_geomod_sphere.brush_radius_in_world_units = microCsgRadius;
		small_geomod_sphere.brush_shape_type = VX::VoxelBrushShape_Cube;
		small_geomod_sphere.custom_surface_material = VoxMat_None;
	}
	//
	//if( input_state.keyboard.held[KEY_V] )
	{
		small_add_sphere.brush_radius_in_world_units = tinyCsgRadius;
		small_add_sphere.brush_shape_type = VX::VoxelBrushShape_Sphere;
		small_add_sphere.fill_material = VoxMat_Wall;
		small_add_sphere.custom_surface_material = VoxMat_None;
	}


	//if( input_state.keyboard.held[KEY_G] )
	{
		large_geomod_sphere.brush_radius_in_world_units = largeCsgRadius;
		large_geomod_sphere.brush_shape_type = VX::VoxelBrushShape_Sphere;
		large_geomod_sphere.custom_surface_material = VoxMat_None;
	}
	//if( input_state.keyboard.held[KEY_U] )
	{
		large_add_sphere.brush_radius_in_world_units = largeCsgRadius;
		large_add_sphere.brush_shape_type = VX::VoxelBrushShape_Sphere;
		large_add_sphere.fill_material = VoxMat_Floor;
		large_add_sphere.custom_surface_material = VoxMat_None;
	}


	//if( input_state.keyboard.held[KEY_B] )
	{
		small_add_cube.brush_radius_in_world_units = smallCsgRadius;
		small_add_cube.brush_shape_type = VX::VoxelBrushShape_Cube;
		small_add_cube.fill_material = VoxMat_Floor;
		small_add_cube.custom_surface_material = VoxMat_None;
	}
	//if( input_state.keyboard.held[KEY_N] )
	{
		small_geomod_cube.brush_radius_in_world_units = smallCsgRadius;
		small_geomod_cube.brush_shape_type = VX::VoxelBrushShape_Cube;
		small_geomod_cube.custom_surface_material = VoxMat_Sand;	// 'geomod' material
	}


	//if( input_state.keyboard.held[KEY_Q] )
	{
		smooth.brush_radius_in_world_units = smallCsgRadius;
		smooth.strength = 1;
		smooth.custom_surface_material = VoxMat_None;
	}

	//if( input_state.keyboard.held[KEY_T] )
	{
		//paint_material.brush_radius_in_world_units = mediumCsgRadius;
		paint_material.brush_shape_type = VX::VoxelBrushShape_Sphere;
		paint_material.brush_radius_in_world_units = tinyCsgRadius;
		paint_material.fill_material = VoxMat_Wall;
		paint_material.custom_surface_material = VoxMat_None;
		paint_material.paint_surface_only = false;
	}
}


//NOTE: Don't forget to AND with MaxInteger mask when decoding.
template< UINT NUM_BITS >
struct TQuantize_
{
	static const int MaxInteger = (1u << NUM_BITS) - 1;

	// [0..1] float => [0..max] int
	mxFORCEINLINE static UINT EncodeUNorm( float f )
	{
		mxASSERT(f >= 0.0f && f <= 1.0f);
		return floor( f * MaxInteger );
	}
	// [0..max] int => [0..1] float
	mxFORCEINLINE static float DecodeUNorm( UINT i )
	{
//		i &= MaxInteger;
		return i * (1.0f / MaxInteger);
	}

	// [-1..+1] float => [0..max] int
	mxFORCEINLINE static UINT EncodeSNorm( float f )
	{
		mxASSERT(f >= -1.0f && f <= +1.0f);
		// lodScale and bias: ((f + 1.0f) * 0.5f) * MaxInteger
		float halfRange = MaxInteger * 0.5f;
		return mmFloorToInt( f * halfRange + halfRange );
	}
	// [0..max] int -> [-1..+1] float
	mxFORCEINLINE static float DecodeSNorm( UINT i )
	{
//		i &= MaxInteger;
		float invHalfRange = 2.0f / MaxInteger;
		return i * invHalfRange - 1.0f;
	}
};

//11,11,10
mxFORCEINLINE
U32 Encode_XYZ01_to_R11G11B10_( float x, float y, float z ) {
	return
		(TQuantize_<11>::EncodeUNorm( x )) |
		(TQuantize_<11>::EncodeUNorm( y ) << 11u) |
		(TQuantize_<10>::EncodeUNorm( z ) << 22u);
}

/// Quantizes normalized coords into 32-bit integer (11,11,10).
mxFORCEINLINE
U32 Encode_XYZ01_to_R11G11B10_( const V3f& p ) {
	return Encode_XYZ01_to_R11G11B10_( p.x, p.y, p.z );
}

/// De-quantizes normalized coords from 32-bit integer.
mxFORCEINLINE
V3f Decode_XYZ01_from_R11G11B10_( U32 u ) {
	return CV3f(
		TQuantize_<11>::DecodeUNorm( u & 2047 ),
		TQuantize_<11>::DecodeUNorm( (u >> 11u) & 2047 ),
		TQuantize_<10>::DecodeUNorm( (u >> 22u) & 1023 )
	);
}

U64 Encode_XYZ01_to_16bits( const V3f& p ) {
	return
		(U64(TQuantize_<16>::EncodeUNorm( p.x ))) |
		(U64(TQuantize_<16>::EncodeUNorm( p.y )) << 16) |
		(U64(TQuantize_<16>::EncodeUNorm( p.z )) << 32);
}
V3f Decode_XYZ01_from_16bits( U64 u ) {
	return CV3f(
		TQuantize_<16>::DecodeUNorm( u & 0xFFFF ),
		TQuantize_<16>::DecodeUNorm( (u >> 16) & 0xFFFF ),
		TQuantize_<16>::DecodeUNorm( (u >> 32) & 0xFFFF )
	);
}



UShort3 Encode_XYZ01_to_UShort3( const V3f& p ) {
	return CUShort3(
		TQuantize_<16>::EncodeUNorm( p.x ),
		TQuantize_<16>::EncodeUNorm( p.y ),
		TQuantize_<16>::EncodeUNorm( p.z )
	);
}


// http://www.pbr-book.org/3ed-2018/Color_and_Radiometry/Working_with_Radiometric_Integrals.html#SphericalTheta
inline float SphericalTheta(const V3f &v) {
    return std::acos(clampf(v.z, -1, 1));
}

inline float SphericalPhi(const V3f &v) {
    float p = std::atan2(v.y, v.x);
    return (p < 0) ? (p + 2 * mxPI) : p;
}
V2f SphericalMapping2D_sphereUV(const V3f& dir) {
    float theta = SphericalTheta(dir);	//vertical angle
	float phi = SphericalPhi(dir);
    return V2f(
		phi * mxINV_TWOPI,
		theta * mxINV_PI
		);
}

template< typename Scalar >
inline Tuple2<Scalar> calc_UV_from_Sphere_Normal( const Tuple3<Scalar>& N )
{
	return Tuple2<Scalar>(
		atan2( N.y, N.x ) * mxINV_TWOPI + Scalar(0.5),
		N.z * Scalar(0.5) + Scalar(0.5)
		);
}







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
		}

		//
		void* dst_indices = dst_vertices + num_vertices;
		copyIndices( dst_indices, mesh_info.use_32_bit_index_buffer, Arrays::getSpan(standard_mesh.triangles) );

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
			mxASSERT(src_batch.texture_id.isValid());
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
		copyIndices( dst_indices, mesh_info.use_32_bit_index_buffer, Arrays::getSpan(textured_mesh.triangles) );

		return ALL_OK;
	}


}//VoxelTerrainUtils



///
ERet TowerDefenseGame::VX_CompileMesh_AnyThread(
	const VX5::ChunkMeshingOutput& chunk_meshing_output
	, const VX::ChunkID& chunk_id
	, NwBlob &dst_data_blob_
)
{
	using namespace VoxelTerrainUtils;

	//
	MeshInfo	mesh_infos[ VX5::ChunkMeshingOutput::MAX_MESHES ];
	int			mesh_infos_count = 0;

	//
	if( chunk_meshing_output.standard_mesh.isValid() )
	{
		computeMeshInfo(
			mesh_infos[ mesh_infos_count++ ]
			, chunk_meshing_output.standard_mesh.vertices.num()
			, sizeof(VoxelTerrainStandardVertexT)
			, chunk_meshing_output.standard_mesh.triangles.num()
			, 0
			);
	}

	//
	if( chunk_meshing_output.textured_mesh.isValid() )
	{
		computeMeshInfo(
			mesh_infos[ mesh_infos_count++ ]
			, chunk_meshing_output.textured_mesh.vertices.num()
			, sizeof(VoxelTerrainTexturedVertexT)
			, chunk_meshing_output.textured_mesh.triangles.num()
			, chunk_meshing_output.textured_mesh.batches.num()
			);
	}

	//
	mxENSURE(mesh_infos_count > 0, ERR_INVALID_PARAMETER, "");

	//
	U32	mesh_data_offsets[ VX5::ChunkMeshingOutput::MAX_MESHES ];
	mesh_data_offsets[ 0 ] = sizeof(ChunkDataHeader_d);

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
	BlobWriter	blob_writer( dst_data_blob_ );
	mxDO(blob_writer.reserveMoreBufferSpace( total_data_size, Arrays::ResizeExactly ));

	//
	U32	num_written_meshes = 0;

	//
	if( chunk_meshing_output.standard_mesh.isValid() )
	{
		mxDO(_compileStandardMesh(
			chunk_meshing_output.standard_mesh
			, mesh_infos[ num_written_meshes ]
			, mxAddByteOffset( dst_data_blob_.raw(), mesh_data_offsets[ num_written_meshes ] )
			));
		++num_written_meshes;
	}

	//
	if( chunk_meshing_output.textured_mesh.isValid() )
	{
		mxDO(_compileTexturedMesh(
			chunk_meshing_output.textured_mesh
			, mesh_infos[ num_written_meshes ]
			, mxAddByteOffset( dst_data_blob_.raw(), mesh_data_offsets[ num_written_meshes ] )
			));
		++num_written_meshes;
	}

	//
	{
		ChunkDataHeader_d*	header = (ChunkDataHeader_d*) dst_data_blob_.raw();
		header->num_meshes = num_written_meshes;
		header->aabb_local = chunk_meshing_output.local_bounds;
	}

	//
#if VOXEL_TERRAIN5_WITH_PHYSICS
	AllocatorI & scratch = threadLocalHeap();
	NwBIH	quantizedBIH( scratch );

	NwBIH::BuildOptions	build_options;
	mxDO(quantizedBIH.build( srcMesh, build_options, scratch ));

	mxDO(quantizedBIH.saveTo( dst_data_blob_ ));
#endif

	return ALL_OK;
}

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
						RrMeshInstance **new_mesh_instance_
						, const ChunkMeshHeader_d* mesh_header
						, const TbVertexFormat& vertex_format
						, const AABBf& local_bounds
						, const VoxelTerrainMaterials& material_table
						, NwClump& storage
						)
{
	NwMesh	* new_mesh;
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
		new_mesh->m_flags |= MESH_USES_32BIT_IB;
	}

	//
	new_mesh->m_numVertices = mesh_header->num_vertices;
	new_mesh->m_numIndices = mesh_header->num_indices;

	new_mesh->vertex_format = &vertex_format;

	new_mesh->m_topology = Topology::TriangleList;

	new_mesh->m_localBounds = local_bounds;

	//
	RrMeshInstance	* new_mesh_instance;
	mxDO(RrMeshInstance::ñreateFromMesh(
		new_mesh_instance
		, new_mesh
		, storage
		));

	//
	if( !mesh_header->is_textured )
	{
		TbSubmesh	submesh;
		{
			submesh.start_index = 0;
			submesh.index_count = mesh_header->num_indices;
		}
		new_mesh->_submeshes.add( submesh );
		//
		new_mesh_instance->materials.add( material_table.triplanar_material_instance );
	}
	else
	{
		mxASSERT(!submeshes.isEmpty());

		mxDO(new_mesh->_submeshes.setNum( submeshes._count ));
		mxDO(new_mesh_instance->materials.setNum( submeshes._count ));

		//
		for( UINT iSubmesh = 0; iSubmesh < submeshes._count; iSubmesh++ )
		{
			const SubmeshHeader_d&	src_submesh = submeshes._data[ iSubmesh ];

			//
			TbSubmesh	new_submesh;
			{
				new_submesh.start_index = src_submesh.start_index;
				new_submesh.index_count = src_submesh.index_count;
			}
			new_mesh->_submeshes._data[ iSubmesh ] = new_submesh;

			//
			new_mesh_instance->materials[ iSubmesh ] = material_table.textured_materials[ src_submesh.texture_id ];
		}
	}

	//
	*new_mesh_instance_ = new_mesh_instance;

	return ALL_OK;
}

ERet TowerDefenseGame::VX_loadMesh(
								   const VX5::AClient::CreateMeshArgs& args
								   , void *&mesh_
								   )
{
	PROFILE;

	//
	VoxelTerrainChunk* new_voxel_terrain_chunk = (VoxelTerrainChunk*) _voxel_terrain_renderer._chunkPool.AllocateItem();
	mxENSURE(new_voxel_terrain_chunk, ERR_OUT_OF_MEMORY, "");


	mxSTATIC_ASSERT(VoxelTerrainChunk::MAX_MESH_TYPES <= VX5::ChunkMeshingOutput::MAX_MESHES);

	//
	const ChunkDataHeader_d* chunk_data_header = (ChunkDataHeader_d*) args.meshData;

	//
	{
		U32	num_remaining_meshes = chunk_data_header->num_meshes;

		const ChunkMeshHeader_d* mesh_header = ( ChunkMeshHeader_d* ) ( chunk_data_header + 1 );
		while( num_remaining_meshes )
		{
			RrMeshInstance	*new_mesh_instance;

			if( !mesh_header->is_textured )
			{
				mxDO(createMeshInstance(
					&new_mesh_instance
					, mesh_header
					, VoxelTerrainStandardVertexT::metaClass()
					, chunk_data_header->aabb_local
					, _voxel_terrain_renderer.materials
					, m_runtime_clump
					));
			}
			else
			{
				mxDO(createMeshInstance(
					&new_mesh_instance
					, mesh_header
					, VoxelTerrainTexturedVertexT::metaClass()
					, chunk_data_header->aabb_local
					, _voxel_terrain_renderer.materials
					, m_runtime_clump
					));
			}

			//
			new_voxel_terrain_chunk->models.add( new_mesh_instance );

			//
			const U32	aligned_mesh_size = tbALIGN16( mesh_header->total_size );

			mesh_header = ( ChunkMeshHeader_d* ) mxAddByteOffset(
				mesh_header
				, aligned_mesh_size
				);

			--num_remaining_meshes;
		}
	}



	// The chunk mesh lies in unit cube [0..1]^3 relative the chunk's seam octree space.

	const VX::ChunkID chunk_id = args.chunk_id;

	const VX5::Stitching::SeamConfig	seam_config( chunk_id );

	const VX5::ChunkPathInSeamOctree chunk_address_prefix = seam_config.chunkPathInSeamOctree();

	const AABBf chunk_aabb_world_no_margins = AABBf::fromOther(_voxel_world._octree.getNodeAABB(chunk_id));
	const AABBf chunk_aabb_with_margin_WS = AABBf::fromOther(_voxel_world.getBBoxWorldWithMargins( chunk_id ));
	const AABBf seam_octree_aabb_world = Morton32_getRootAABB( chunk_address_prefix.M, chunk_aabb_world_no_margins );

	//new_voxel_terrain_chunk->debug_id = chunk_id.M;

	//
	const float	chunk_size_in_world_space = seam_octree_aabb_world.size().x;

	new_voxel_terrain_chunk->AabbMins = seam_octree_aabb_world.min_corner;
	new_voxel_terrain_chunk->AabbSize = chunk_size_in_world_space;

	//
	const AABBf	normalized_aabb_in_mesh_space = chunk_data_header->aabb_local;	// inside [0..1]

	// scale to world size
	const AABBf	scaled_aabb_in_chunk_space = {
		normalized_aabb_in_mesh_space.min_corner * chunk_size_in_world_space,
		normalized_aabb_in_mesh_space.max_corner * chunk_size_in_world_space
	};

	// translate to world space
	const AABBf chunk_aabb_in_world_space = scaled_aabb_in_chunk_space
		.shifted( seam_octree_aabb_world.min_corner )
		;



	new_voxel_terrain_chunk->render_entity_handle = _render_world->addEntity(
		new_voxel_terrain_chunk,
		RE_VoxelTerrainChunk,
		chunk_aabb_in_world_space
		);

	//
	mesh_ = new_voxel_terrain_chunk;


	/// Re-voxelize on GPU / Update shadow maps
	_render_world->regionChanged(chunk_aabb_with_margin_WS);



#if VOXEL_TERRAIN5_WITH_PHYSICS && VOXEL_TERRAIN5_CREATE_TERRAIN_COLLISION

	if( args.chunk_id.getLoD() <= MAX_LOD_FOR_COLLISION_PHYSICS )
	{
		AllocatorI & scratchpad = threadLocalHeap();

		const NwBIH::Header_d* collision_data_header =
			(NwBIH::Header_d*) mxAddByteOffset( mesh_index_data, tbALIGN16(index_data_size) );

		//
		Physics::VoxelTerrainChunkCollidable *	collsion_object;

		_physics_world.createStaticVoxelTerrainChunk(
			collision_data_header,
			seam_octree_aabb_world.min_corner,
			normalized_aabb_in_mesh_space,
			chunk_size_in_world_space,
			m_runtime_clump,
			scratchpad,
			collsion_object
			);

		new_voxel_terrain_chunk->phys_obj = collsion_object;
	}

#else
	
	new_voxel_terrain_chunk->phys_obj = nil;

#endif

	return ALL_OK;
}

void TowerDefenseGame::VX_unloadMesh(
	const VX5::ChunkID& chunk_id,
	void* mesh
)
{
	//
	VoxelTerrainChunk * terrainChunk = static_cast< VoxelTerrainChunk* >( mesh );
	mxASSERT_PTR(terrainChunk);

	// Re-voxelize on GPU / Update shadow maps
//	_render_world->regionChanged(chunk_aabb_with_margin_WS);


	//
#if VOXEL_TERRAIN5_WITH_PHYSICS && VOXEL_TERRAIN5_CREATE_TERRAIN_COLLISION
	if( chunk_id.getLoD() <= MAX_LOD_FOR_COLLISION_PHYSICS )
	{
		Physics::VoxelTerrainChunkCollidable *	colObject = (Physics::VoxelTerrainChunkCollidable*) terrainChunk->phys_obj;
		if( colObject )
		{
			_physics_world.destroyStaticVoxelTerrainChunk( colObject, m_runtime_clump );
			terrainChunk->phys_obj = nil;
		}
	}
#endif

	if( terrainChunk->render_entity_handle.isValid() ) {
		_render_world->removeEntity( terrainChunk->render_entity_handle );
		terrainChunk->render_entity_handle.setNil();
	}

	//
	for( UINT iModel = 0; iModel < terrainChunk->models.num(); iModel++ )
	{
		RrMeshInstance* model = terrainChunk->models[ iModel ];

		//
		{
			NwMesh *	mesh = model->mesh;
			mesh->release();
			m_runtime_clump.Destroy( mesh );
		}

		//
		m_runtime_clump.Destroy( model );
	}

	terrainChunk->models.empty();

	//
	_voxel_terrain_renderer._chunkPool.ReleaseItem( terrainChunk );
}

ERet TowerDefenseGame::VX_generateChunk_AnyThread(
	const VX5::ChunkID& chunk_id
	, const CubeMLd& region_bounds_world
	, VX5::ChunkInfo &metadata_
	, VX5::AChunkDB * DB_voxels
	, AllocatorI & scratchpad
	)
{

#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	mxDO(this->_generateChunk_for_semiprocedural_Moon(
		 chunk_id
		 , region_bounds_world
		 , metadata_
		 , DB_voxels
		 , scratchpad
		 ));

#else // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	#if TEST_VOXEL_TERRAIN_PROCEDURAL_TERRAIN_MODE
	
		mxDO(this->_generateChunk_from_Noise_in_App_settings(
			 chunk_id
			 , region_bounds_world
			 , metadata_
			 , DB_voxels
			 , scratchpad
			 ));

	#else

		mxDO(this->_generateChunk_from_SDF_in_Lua_file(
			 chunk_id
			 , region_bounds_world
			 , metadata_
			 , DB_voxels
			 , scratchpad
			 ));

	#endif

#endif // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	return ALL_OK;
}

ERet TowerDefenseGame::_generateChunk_for_semiprocedural_Moon(
	const VX5::ChunkID& chunk_id
	, const CubeMLd& region_bounds_world
	, VX5::ChunkInfo &metadata_
	, VX5::AChunkDB * DB_voxels
	, AllocatorI & scratchpad
)
{

#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	using namespace VX5;

	const UInt4 chunk_coords_and_LoD = chunk_id.toCoordsAndLoD();
	const UINT iChunkLoD = chunk_coords_and_LoD.w;

	//LogStream(LL_Info) << "Generating chunk " << chunk_id;

	const AABBf chunk_aabb_with_margin_WS = AABBf::fromOther(_voxel_world.getBBoxWorldWithMargins( chunk_id ));
	//const AABBf chunk_aabb_WS = _voxel_world._octree.getNodeAABB( chunk_id );

	//
	ChunkData		chunk_data( scratchpad );	//!< voxel data at the current_cell_info LoD

	ChunkGenStats	chunk_gen_stats;


	//
	struct ProcGenCallbacks
	{
		static int log2( double x )
		{
			double tmp = ::log( x ) * mxINV_LN2;
			double res = ::floor( tmp );
			return res;
		}

		static double getSignedDistanceAtPoint(
			const V3d& voxel_center_in_world_space
			, const ChunkGenerationContext& context
			)
		{
			const Planets::PlanetProcGenData& planet_procgen_data = *static_cast< Planets::PlanetProcGenData* >( context.user_data );
			//
			const V3d vector_from_center = voxel_center_in_world_space - planet_procgen_data.parameters.center_in_world_space;
			const double distance_from_sphere_center = ::sqrt( vector_from_center.lengthSquared() );
			const double signed_distance_to_sphere_surface = distance_from_sphere_center - planet_procgen_data.parameters.radius_in_world_units;
#if 1
			//
			const V3f N = V3f::fromXYZ( V3d::div( vector_from_center, distance_from_sphere_center ) );
			const V2f uv = calc_UV_from_Sphere_Normal( N );
			// Select the mip level so that the texel size in world space
			// is approximately equal to the cell size in world space.
			const UINT chunk_LoD = context.chunk_id.getLoD();
			const float cell_size_at_this_LoD_in_meters = VX5::getCellSizeAtLoD( chunk_LoD );
			//
			const int mip_index_starting_from_finest_mip
				= log2( cell_size_at_this_LoD_in_meters / planet_procgen_data.texel_size );
			//
			const int clamped_mip_index_starting_from_finest_mip
				= smallest( largest( mip_index_starting_from_finest_mip, 0 ), planet_procgen_data.heightmap.num_mipmaps - 1);

			const int mip_index_starting_from_coarsest_mip = planet_procgen_data.heightmap.num_mipmaps - clamped_mip_index_starting_from_finest_mip - 1;

			const float sampled_height = planet_procgen_data.heightmap.sample_Slow( uv.x, uv.y, mip_index_starting_from_coarsest_mip );

			mxBUG("cracks when height range is > 0");
			return signed_distance_to_sphere_surface + sampled_height;// * 4.0f;
#else
			return signed_distance_to_sphere_surface;
#endif
		}
	};


	//
	VX5::ChunkGenerationContext	context;
	context.chunk_id = chunk_id;
	context.user_data = &_planet_procgen_data;
	//
	context.callbacks.getSignedDistanceAtPoint = &ProcGenCallbacks::getSignedDistanceAtPoint;

	//
	CubeMLd chunk_aabb_with_margins_WS;
	chunk_aabb_with_margins_WS.min_corner = V3d::fromXYZ( chunk_aabb_with_margin_WS.min_corner );
	chunk_aabb_with_margins_WS.side_length = VX5::getChunkSizeWithMarginsAtLoD( iChunkLoD );

	//
	mxDO(chunk_data.generateWithUserCallbacks( context
		, chunk_aabb_with_margins_WS
		, VX5::CHUNK_SIZE_CELLS
		, scratchpad
		, chunk_gen_stats
		, m_dbgView
		));

#if 0
	//
	chunk_data.dbg_showVoxels(
		chunk_aabb_with_margins_WS,
		chunk_aabb_with_margins_WS.sideLength() / VX5::CHUNK_SIZE_CELLS,
		VX5::CHUNK_SIZE_VOXELS,
		m_dbgView
		);
#endif




	// The chunk's signed octree is used for contouring, (memoryless) LoD generation and LoD "stitching".
	VX5::ContouringData	contouring_data( scratchpad );

	VX5::ChunkMesher::buildCellOctree(
		contouring_data.cells
		, chunk_id
		, chunk_data
		, scratchpad
		, m_dbgView
		);

	// Save editable voxel data only for the finest, most-detailed LoDs.
	if( iChunkLoD == 0 )
	{
		NwBlob	voxel_data_blob( scratchpad );
		BlobWriter	blob_writer( voxel_data_blob );

		ChunkSaveStats	chunk_stats;// always gather stats about the chunk's contents
		chunk_data.saveToBlob( blob_writer, chunk_stats );

		mxDO(DB_voxels->Write( chunk_id, voxel_data_blob.raw(), voxel_data_blob.rawSize() ));
	}//If this is the finest, most-detailed LoD.

	//
	updateMetadata( metadata_, chunk_gen_stats );

	// Generate contouring data at twice coarser resolution for stitching to coarser LoDs.
	VX5::LoD::generate_Contouring_Data_for_Stitching_to_Coarser_Neighbors(
		contouring_data
		, chunk_id
		, context
		, _voxel_world
		, _voxel_world._settings.meshing
		, scratchpad
		, m_dbgView
		);

	// Save contouring data to DB.
	if( !contouring_data.isEmpty() )
	{
		contouring_data.saveToDB_AnyThread( chunk_id, _voxel_world.octreeDB, scratchpad );

		updateMetadataFromContouringData( metadata_, contouring_data );
	}

#else // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	
	mxUNREACHABLE;

#endif // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	return ALL_OK;
}

ERet TowerDefenseGame::_generateChunk_from_Noise_in_App_settings(
	const VX5::ChunkID& chunk_id
	, const CubeMLd& region_bounds_world
	, VX5::ChunkInfo &metadata_
	, VX5::AChunkDB * DB_voxels
	, AllocatorI & scratchpad
)
{
	using namespace VX5;

	const UInt4 chunk_coords_and_LoD = chunk_id.toCoordsAndLoD();
	const UINT iChunkLoD = chunk_coords_and_LoD.w;

	//LogStream(LL_Info) << "Generating chunk " << chunk_id;

	const AABBf chunk_aabb_with_margin_WS = AABBf::fromOther(_voxel_world.getBBoxWorldWithMargins( chunk_id ));
	//const AABBf chunk_aabb_WS = _voxel_world._octree.getNodeAABB( chunk_id );

	//
	ChunkData		chunk_data( scratchpad );	//!< voxel data at the current_cell_info LoD

	ChunkGenStats	chunk_gen_stats;

	//
	struct ProcGenCallbacks
	{
		static double getSignedDistanceAtPoint(
			const V3d& voxel_center_in_world_space
			, const ChunkGenerationContext& context
			)
		{
			TowerDefenseGame & app = *(TowerDefenseGame*) context.user_data;

			/* NOT WORKING!
			AABBf	inf_aabb;
			inf_aabb.clear();

			implicit::V3f_SoA		positions;
			positions.xs[0] = voxel_center_in_world_space.x;
			positions.ys[0] = voxel_center_in_world_space.y;
			positions.zs[0] = voxel_center_in_world_space.z;

			implicit::Array_of_F32	distances;
			implicit::Array_of_I32	materials;

			implicit::evaluate(
				app.m_blobTree, inf_aabb, 1, positions, distances, materials
				);
			return distances[0];
			*/

#if 0
			// WORKS GREAT!
			/*
			SDF::Cube	cube;
			cube.center = V3f::fromXYZ( app._voxel_world._octree._cube.center() );
			cube.extent = app._voxel_world._octree._cube.side_length * 0.4f;
			return cube.DistanceTo( V3f::fromXYZ( voxel_center_in_world_space ) );
			*/

/* JAGGIES!!! */
			const V3f rotated_pos = M33_Transform( app._test_rotation_matrix, V3f::fromXYZ( voxel_center_in_world_space ) );
			SDF::Cube	cube;
			cube.center = V3f::fromXYZ( app._voxel_world._octree._world_bounds.center() );
			cube.extent = app._voxel_world._octree._world_bounds.side_length * 0.25f;
			return cube.DistanceTo( rotated_pos );
#endif

#if 0
			return app._fast_noise.GetNoise(
				voxel_center_in_world_space.x, voxel_center_in_world_space.y
				) * app.m_settings.fast_noise_result_scale
				+ voxel_center_in_world_space.z//app.m_settings.fast_noise_result_offset
				;
#endif

			const V3d vector_from_center = voxel_center_in_world_space - app._voxel_world._octree._cube.center();
			const double distance_from_sphere_center = ::sqrt( vector_from_center.lengthSquared() );
			const double signed_distance_to_sphere_surface = distance_from_sphere_center - app._voxel_world._octree._cube.side_length * 0.3;

			const V3f N = V3f::fromXYZ( V3d::div( vector_from_center, distance_from_sphere_center ) );

			return signed_distance_to_sphere_surface + app._fast_noise.GetNoise(
				voxel_center_in_world_space.x, voxel_center_in_world_space.y, voxel_center_in_world_space.z
				//N.x * app.m_settings.fast_noise_frequency_mul,
				//N.y * app.m_settings.fast_noise_frequency_mul
				) * app.m_settings.fast_noise_result_scale
				;
		}
	};

	//
	VX5::ChunkGenerationContext	context;
	context.chunk_id = chunk_id;
	context.user_data = this;
	//
	context.callbacks.getSignedDistanceAtPoint = &ProcGenCallbacks::getSignedDistanceAtPoint;

	//
	CubeMLd chunk_aabb_with_margins_WS;
	chunk_aabb_with_margins_WS.min_corner = V3d::fromXYZ( chunk_aabb_with_margin_WS.min_corner );
	chunk_aabb_with_margins_WS.side_length = VX5::getChunkSizeWithMarginsAtLoD( iChunkLoD );

	//
	mxDO(chunk_data.generateWithUserCallbacks( context
		, chunk_aabb_with_margins_WS
		, VX5::CHUNK_SIZE_CELLS
		, scratchpad
		, chunk_gen_stats
		, m_dbgView
		));

	// The chunk's signed octree is used for contouring, (memoryless) LoD generation and LoD "stitching".
	VX5::ContouringData	contouring_data( scratchpad );

	VX5::ChunkMesher::buildCellOctree(
		contouring_data.cells
		, chunk_id
		, chunk_data
		, scratchpad
		, m_dbgView
		);

	// Save editable voxel data only for the finest, most-detailed LoDs.
	if( iChunkLoD == 0 )
	{
		NwBlob	voxel_data_blob( scratchpad );
		BlobWriter	blob_writer( voxel_data_blob );

		ChunkSaveStats	chunk_stats;// always gather stats about the chunk's contents
		chunk_data.saveToBlob( blob_writer, chunk_stats );

		mxDO(DB_voxels->Write( chunk_id, voxel_data_blob.raw(), voxel_data_blob.rawSize() ));
	}//If this is the finest, most-detailed LoD.

	//
	updateMetadata( metadata_, chunk_gen_stats );

	// Generate contouring data at twice coarser resolution for stitching to coarser LoDs.
	VX5::LoD::generate_Contouring_Data_for_Stitching_to_Coarser_Neighbors(
		contouring_data
		, chunk_id
		, context
		, _voxel_world
		, _voxel_world._settings.meshing
		, scratchpad
		, m_dbgView
		);

	// Save contouring data to DB.
	if( !contouring_data.isEmpty() )
	{
		contouring_data.saveToDB_AnyThread( chunk_id, _voxel_world.octreeDB, scratchpad );

		updateMetadataFromContouringData( metadata_, contouring_data );
	}

	return ALL_OK;
}

ERet TowerDefenseGame::_generateChunk_from_SDF_in_Lua_file(
	const VX5::ChunkID& chunk_id
	, const CubeMLd& region_bounds_world
	, VX5::ChunkInfo &metadata_
	, VX5::AChunkDB * DB_voxels
	, AllocatorI & scratchpad
)
{
using namespace VX;
	using namespace VX5;

	const UInt4 chunk_coords_and_LoD = chunk_id.toCoordsAndLoD();
	const UINT iChunkLoD = chunk_coords_and_LoD.w;

//LogStream(LL_Info) << "Generating chunk " << chunk_id;

const AABBf chunk_aabb_with_margin_WS = AABBf::fromOther(_voxel_world.getBBoxWorldWithMargins( chunk_id ));
const AABBf chunk_aabb_WS = AABBf::fromOther(_voxel_world._octree.getNodeAABB( chunk_id ));

#if DEBUG_USE_SERIAL_JOB_SYSTEM
	//M44_buildTranslationMatrix( _args.bounds.min_corner )
	//const M44f worldTransform = M44_BuildTS( _args.bounds.min_corner, (1 << iChunkLoD) * CHUNK_SIZE0 );
	DebugViewProxy	dbgViewProxy( &m_dbgView, g_MM_Identity );

	const M44f	chunkMeshTransform = M44_BuildTS( chunk_aabb_with_margin_WS.min_corner, chunk_aabb_with_margin_WS.size().x );
	DebugViewProxy	dbgViewProxyForChunkMesh( &m_dbgView, chunkMeshTransform );
	ADebugView &dbgViewProxyForContouring = ( iChunkLoD == 0 ) ? dbgViewProxyForChunkMesh : ADebugView::ms_dummy;
	//ADebugView &dbgViewProxyForContouring = ( chunk_id == DBG_CHUNK_ID ) ? dbgViewProxyForChunkMesh : ADebugView::ms_dummy;
#else
	ADebugView & dbgViewProxy = ADebugView::ms_dummy;
	ADebugView &dbgViewProxyForContouring = ADebugView::ms_dummy;
#endif

//if(chunk_id == DBG_CHUNK_ID) {
//dbgViewProxy.addBox(chunk_aabb_WS, RGBAf::BLACK);
//dbgViewProxy.addBox(chunk_aabb_with_margin_WS, RGBAf::GREEN);
//}
	ChunkData		chunk_data( scratchpad );	//!< voxel data at the current_cell_info LoD

	ChunkGenStats	buildStats;

	mxDO(chunk_data.generateFromBlobTree( m_blobTree
		, chunk_aabb_with_margin_WS
		, VX5::CHUNK_SIZE_CELLS
		, scratchpad, buildStats
		, m_dbgView//( iChunkLoD == 0 ) ? m_dbgView : ADebugView::ms_dummy
		//, ( iChunkLoD == 0 && m_settings.dbg_viz_SDF ) ? m_dbgView : ADebugView::ms_dummy
	));

	//

	// The chunk's signed octree is used for contouring, (memoryless) LoD generation and LoD "stitching".
	VX5::ContouringData	contouring_data( scratchpad );

	ChunkMesher::buildCellOctree(
		contouring_data.cells
		, chunk_id
		, chunk_data
		, scratchpad
		, m_dbgView
		);


#if 0
	// if octree simplification is enabled:
	if( _voxel_world._settings.meshing.octree_simplification != NoSimplification )
	{
		if( !contouring_data.cells.isEmpty() )
		{
			contouring_data.cells.sortByDecreasingAddresses(scratchpad);
			contouring_data.cells.repackWithQuadrics(scratchpad);

const AABBf chunk_aabb_local_with_margins = OctreeUtils::UNIT_CUBE;
const AABBf chunk_aabb_local_no_margins = {
	chunk_aabb_local_with_margins.mins + CV3f(OctreeUtils::LOCAL_CELL_SIZE * CHUNK_MARGIN),
	chunk_aabb_local_with_margins.maxs - CV3f(OctreeUtils::LOCAL_CELL_SIZE * CHUNK_MARGIN)
};

			OctreeSimplifier	simplifier( scratchpad );
			simplifier._settings.setup_for_simplification_after_procedural_generation();

			mxDO(simplifier.simplifyChunkWithGhostCells(
				contouring_data.cells
				, chunk_aabb_local_no_margins
				, chunk_id
				, _voxel_world
				, scratchpad 
				, ADebugView::ms_dummy//m_dbgView//VX5::getDbgView_AnyThread()
				));
		}
	}
#endif

	//
	if(
		// Save editable voxel data only for the finest, most-detailed LoDs.
		iChunkLoD == 0

		// And if the chunk is not fully empty or fully solid.
		&& buildStats.intersectsSurface()
		)
	{
		NwBlob	voxel_data_blob( scratchpad );
		BlobWriter	blob_writer( voxel_data_blob );

		ChunkSaveStats	chunk_stats;// always gather stats about the chunk's contents
		chunk_data.saveToBlob( blob_writer, chunk_stats );

		mxDO(DB_voxels->Write( chunk_id, voxel_data_blob.raw(), voxel_data_blob.rawSize() ));
	}//If this is the finest, most-detailed LoD.

	updateMetadata( metadata_, buildStats );

	//
	VX5::generateContouringData_for_Stitching_to_CoarserNeighbors(
		contouring_data
		, chunk_id
		, m_blobTree
		, _voxel_world
		, _voxel_world._settings.meshing
		, scratchpad
		, m_dbgView
		);

	// Save contouring data to DB.

	if( !contouring_data.isEmpty() )
	{
		contouring_data.saveToDB_AnyThread( chunk_id, _voxel_world.octreeDB, scratchpad );

		updateMetadataFromContouringData( metadata_, contouring_data );

	}//If the chunk intersects the isosurface.

	return ALL_OK;
}

void TowerDefenseGame::createDebris( const VX5::Debris& debris )
{
	_createDebris( debris );
}

ERet TowerDefenseGame::_createDebris( const VX5::Debris& debris )
{
	AllocatorI & scratch = MemoryHeaps::temporary();
UNDONE;
#if 0
	//
	const Meshok::TriMesh& debris_mesh = *debris.mesh;

	//
	const float	chunk_size_world = VX5::getChunkSize( 0 );
	const AABBf chunk_aabb_local_no_margins = { CV3f(0), CV3f(chunk_size_world) };
	const AABBf chunk_aabb_world_no_margins = _voxel_world._octree.getBBoxWorld( debris.chunk_id );

	//
	const AABBf mesh_aabb_normalized = debris_mesh.localAabb;
	const AABBf mesh_aabb_in_object_space = {
		AABBf_GetOriginalPosition( chunk_aabb_local_no_margins, mesh_aabb_normalized.mins ),
		AABBf_GetOriginalPosition( chunk_aabb_local_no_margins, mesh_aabb_normalized.maxs )
	};

	//
	const AABBf mesh_aabb_in_world_space = {
		AABBf_GetOriginalPosition( chunk_aabb_world_no_margins, mesh_aabb_normalized.mins ),
		AABBf_GetOriginalPosition( chunk_aabb_world_no_margins, mesh_aabb_normalized.maxs )
	};

	//
	const M44f transform_from_normalized_mesh_space_into_object_space =
		M44_BuildTS( CV3f(-mesh_aabb_normalized.mins - mesh_aabb_normalized.halfSize()), chunk_size_world );

	//
	const U32 num_vertices = debris_mesh.vertices.num();
	const U32 num_triangles = debris_mesh.triangles.num();

	const U32 gpu_vertex_data_size = num_vertices * sizeof(VertexType);

	const U32 num_indices = num_triangles * 3u;
	const bool use_32_bit_vertex_indices = (num_vertices > MAX_UINT16);
	const U32 gpu_index_data_size = num_indices * ( use_32_bit_vertex_indices ? sizeof(U32) : sizeof(U16) );

	//
	VertexType *	gpu_vertices;
	mxTRY_ALLOC_SCOPED( gpu_vertices, num_vertices, scratch );

	BYTE *	gpu_indices;
	mxTRY_ALLOC_SCOPED( gpu_indices, gpu_index_data_size, scratch );

	//
	quantizeMeshVertices( gpu_vertices, debris_mesh, debris.chunk_id );

	Meshok::CopyIndices( gpu_indices, use_32_bit_vertex_indices, debris_mesh.triangles.getSlice() );

	//
	TPtr< NwMesh >			gpu_mesh;
	mxDO(m_runtime_clump.New( gpu_mesh.Ptr ));

	gpu_mesh->_submeshes.setNum(1);
	{
		TbSubmesh & submesh = gpu_mesh->_submeshes[0];
		submesh.start_index = 0;
		submesh.index_count = num_indices;
	}

	gpu_mesh->m_flags = 0;
	if( use_32_bit_vertex_indices ) {
		gpu_mesh->m_flags |= MESH_USES_32BIT_IB;
	}

	gpu_mesh->m_numVertices = num_vertices;
	gpu_mesh->m_numIndices = num_indices;

	gpu_mesh->m_vertexFormat = VTX_DLOD;

	gpu_mesh->m_topology = Topology::TriangleList;

	mxDO(DynamicBufferPool::Get().createDynamicVertexBuffer( &gpu_mesh->m_vertexBuffer._ptr, gpu_vertex_data_size ));
	mxDO(DynamicBufferPool::Get().createDynamicIndexBuffer( &gpu_mesh->m_indexBuffer._ptr, gpu_index_data_size ));
UNDONE;
	//GL::UpdateBuffer_ASAP( gpu_mesh->m_vertexBuffer->_handle, gpu_vertices, gpu_vertex_data_size );
	//GL::UpdateBuffer_ASAP( gpu_mesh->m_indexBuffer->_handle, gpu_indices, gpu_index_data_size );

	//
	const V3f initial_position_in_world_space = mesh_aabb_in_world_space.center();

m_dbgView.addPoint( initial_position_in_world_space, RGBAf::GREEN, 3.f );

	//
	const M44f	initial_graphics_transform =
		transform_from_normalized_mesh_space_into_object_space	// from normalized mesh space [0..1] into chunk local space
		* M44_BuildTS( CV3f(chunk_aabb_world_no_margins.mins), CV3f(1) )	// into world space
		;

	const V3f initialVelocity = CV3f(0);

	//
	RrMeshInstance *	renderModel;
	mxDO(RrMeshInstance::CreateFromMesh(
		renderModel,
		gpu_mesh,
		initial_graphics_transform,
		m_runtime_clump
		));

	//NwMaterial* renderMat = nil;
	//mxDO(Resources::Load(renderMat, MakeAssetID("material_plastic"), &m_runtime_clump));
	renderModel->_materials[0] = m_voxelTerrain._materials.mat_terrain_DLOD;

	//
m_dbgView.addBox( mesh_aabb_in_world_space, RGBAf::WHITE );

	//
	renderModel->m_proxy = _render_world->addEntity( renderModel, RE_RigidModel, mesh_aabb_in_world_space );

#if VOXEL_TERRAIN5_WITH_PHYSICS
	_physics_world.spawnRigidBox( initial_position_in_world_space
		, CV3f( mesh_aabb_in_object_space.halfSize() )
		, renderModel->_transform, renderModel->m_proxy.id
		, m_runtime_clump, debris.mass
		, initialVelocity
		, M44_Multiply( transform_from_normalized_mesh_space_into_object_space, M44_buildTranslationMatrix(-mesh_aabb_in_object_space.center()) )
		);
#endif
#endif
	return ALL_OK;
}
