/*
=============================================================================
	Graphics model used for rendering.
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Base/Template/Containers/Blob.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/ObjectModel/Clump.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Core/Material.h>

namespace Rendering
{
/*
-----------------------------------------------------------------------------
	Submesh
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( Submesh );
mxBEGIN_REFLECTION( Submesh )
	mxMEMBER_FIELD( start_index ),	
	mxMEMBER_FIELD( index_count ),
	//mxMEMBER_FIELD( base_vertex ),
	//mxMEMBER_FIELD( vertex_count ),
mxEND_REFLECTION

/*
-----------------------------------------------------------------------------
	NwMesh
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( NwMesh );
mxBEGIN_REFLECTION( NwMesh )
	mxMEMBER_FIELD( vertex_format ),
	mxMEMBER_FIELD( flags ),
	mxMEMBER_FIELD( topology ),
	mxMEMBER_FIELD( submeshes ),
	mxMEMBER_FIELD( default_materials_ids ),
	mxMEMBER_FIELD( vertex_count ),
	mxMEMBER_FIELD( index_count ),
	mxMEMBER_FIELD( local_aabb ),
//	mxMEMBER_FIELD( vertex_scale ),
mxEND_REFLECTION

//tbPRINT_SIZE_OF(NwMesh);

NwMesh::NwMesh()
	: raw_vertex_data(MemoryHeaps::renderer())
	, raw_index_data(MemoryHeaps::renderer())
{
	vertex_format = nil;
	flags = (EMeshFlags) 0;
	topology = NwTopology::Undefined;
	vertex_count = 0;
	index_count = 0;

	Arrays::initializeWithExternalStorage( submeshes, z__submeshes_storage );

	local_aabb.clear();
//	vertex_scale = g_MM_One;
}

ERet NwMesh::LoadFromStream(
	NwMesh &new_mesh_
	, AReader& stream
	, const LoadFlagsT load_flags
	)
{
	//
	TbMesh_Header_d	header;
	mxDO(stream.Get(header));

	//
	mxENSURE(
		header.magic == TbMesh_Header_d::MAGIC,
		ERR_FAILED_TO_PARSE_DATA,
		"invalid mesh header"
		);

	mxENSURE(
		header.version == TbMesh_Header_d::VERSION,
		ERR_INCOMPATIBLE_VERSION,
		"invalid mesh version (read: %d, expected: %d)", header.version, TbMesh_Header_d::VERSION
		);

	//
	const TbVertexFormat* vertex_format = (TbVertexFormat*) TypeRegistry::FindClassByGuid( header.vertex_format );
	mxENSURE(vertex_format, ERR_OBJECT_NOT_FOUND, "Unknown vertex format");
	const U32 vertex_stride = vertex_format->GetInstanceSize();

	//
	new_mesh_.flags = header.flags;

	new_mesh_.vertex_count = header.vertex_count;
	new_mesh_.index_count = header.index_count;

	new_mesh_.vertex_format = vertex_format;

	const NwTopology::Enum topology = (NwTopology::Enum) header.topology;
	new_mesh_.topology = topology;

	new_mesh_.local_aabb = header.local_aabb;

	//
	mxDO(new_mesh_.submeshes.setNum( header.submesh_count ));
	mxDO(new_mesh_.default_materials_ids.setNum( header.submesh_count ));

#if MX_DEBUG
	const U32	submeshes_offset = stream.Tell();
#endif

	// Read submeshes.
	mxDO(stream.Read(
		new_mesh_.submeshes.raw()
		, new_mesh_.submeshes.rawSize()
		));


	// Align cursor by vertex stride.
	mxDO(Reader_Align_Forward( stream, vertex_stride ));

#if MX_DEBUG
	const U32	mesh_data_offset = stream.Tell();
#endif

	//
	const U32	index_stride = ( header.flags & MESH_USES_32BIT_IB ) ? sizeof(U32) : sizeof(U16);

	const U32	vertex_data_size = header.vertex_count * vertex_stride;
	const U32	index_data_size = header.index_count * index_stride;

	//
	const NGpu::Memory* vertex_data_update_mem = NGpu::Allocate( vertex_data_size );
	mxENSURE( vertex_data_update_mem, ERR_OUT_OF_MEMORY, "" );

	const NGpu::Memory* index_data_update_mem = NGpu::Allocate( index_data_size );
	mxENSURE( index_data_update_mem, ERR_OUT_OF_MEMORY, "" );

	//
	void *const	raw_vertex_data	= vertex_data_update_mem->data;
	void *const	raw_index_data	= index_data_update_mem->data;

	mxDO(stream.Read( raw_vertex_data, vertex_data_size ));
	mxDO(stream.Read( raw_index_data, index_data_size ));


	// Read submesh materials.
	for( UINT i = 0; i < header.submesh_count; i++ )
	{
		AssetID	material_id;
		mxDO(readAssetID(
			material_id,
			stream
			));

		new_mesh_.default_materials_ids[i] = material_id;
	}

	//
mxOPTIMIZE("use default/immutable VB&IB for skinned characters, use dynamic for voxel terrain");

	mxDO(new_mesh_.geometry_buffers.updateVertexBuffer( vertex_data_update_mem ));
	mxDO(new_mesh_.geometry_buffers.updateIndexBuffer( index_data_update_mem ));

	//
	if(load_flags & KeepMeshDataInRAM)
	{
		mxDO(new_mesh_.raw_vertex_data.setNum(vertex_data_size));
		mxDO(new_mesh_.raw_index_data.setNum(index_data_size));

		memcpy(new_mesh_.raw_vertex_data.raw(), raw_vertex_data, vertex_data_size);
		memcpy(new_mesh_.raw_index_data.raw(), raw_index_data, index_data_size);
	}

	return ALL_OK;
}

ERet NwMesh::CompileMesh(
	AWriter &stream
	, const TbRawMeshData& raw_mesh_data
	, AllocatorI & scratchpad
	)
{
	mxENSURE(
		raw_mesh_data.vertexData.streams.num() == 1,
		ERR_UNSUPPORTED_FEATURE,
		"Only one vertex stream is supported!"
		);

	const RawVertexStream& vertex_stream_0 = raw_mesh_data.vertexData.streams[0];

	const TbVertexFormat* vertex_format = (TbVertexFormat*) TypeRegistry::FindClassByGuid(
		raw_mesh_data.vertexData.vertex_format_id
		);
	mxENSURE(vertex_format, ERR_OBJECT_NOT_FOUND, "Unknown vertex format");

	const U32 vertex_stride = vertex_format->GetInstanceSize();

	// Write header.

	TbMesh_Header_d	header;
	mxZERO_OUT(header);
	{
		header.magic			= TbMesh_Header_d::MAGIC;
		header.version			= TbMesh_Header_d::VERSION;
		header.local_aabb		= raw_mesh_data.bounds;

		header.vertex_count		= raw_mesh_data.vertexData.count;
		header.index_count		= raw_mesh_data.indexData.NumIndices();

		header.submesh_count	= raw_mesh_data.parts.num();
		header.vertex_format	= raw_mesh_data.vertexData.vertex_format_id;

		header.topology			= raw_mesh_data.topology;

		header.flags			= raw_mesh_data.indexData.is32bit() ? MESH_USES_32BIT_IB : 0;
	}
	mxDO(stream.Put( header ));


	// Write submeshes.

	DynamicArray< Submesh >	submeshes( scratchpad );
	mxDO(submeshes.setNum( raw_mesh_data.parts.num() ));

	for( UINT i = 0; i < raw_mesh_data.parts.num(); i++ )
	{
		const RawMeshPart& raw_submesh = raw_mesh_data.parts[i];

		Submesh &dst_submesh = submeshes[i];
		dst_submesh.start_index = raw_submesh.start_index;
		dst_submesh.index_count = raw_submesh.index_count;
	}

	mxDO(AWriter_writeArrayAsRawBytes( submeshes, stream ));


	// Align cursor by vertex stride.
	const U32 bytes_written = sizeof(header) + submeshes.rawSize();
	mxDO(Writer_AlignForward( stream, bytes_written, vertex_stride ));


	// Write vertices.
	mxDO(AWriter_writeArrayAsRawBytes( vertex_stream_0.data, stream ));


	// Write indices.
	mxDO(AWriter_writeArrayAsRawBytes( raw_mesh_data.indexData.data, stream ));


	// Write submesh materials.

	for( UINT i = 0; i < raw_mesh_data.parts.num(); i++ )
	{
		const RawMeshPart& raw_submesh = raw_mesh_data.parts[i];

		mxDO(writeAssetID(
			raw_submesh.material_id,
			stream
			));
	}

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	RrTransform
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( RrTransform );
mxBEGIN_REFLECTION( RrTransform )
	mxMEMBER_FIELD( local_to_world ),
mxEND_REFLECTION


///
TbMeshLoader::TbMeshLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( NwMesh::metaClass(), parent_allocator )
{
}

ERet TbMeshLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	*new_instance_ = context.object_allocator.new_< NwMesh >();
	return ALL_OK;
}

ERet TbMeshLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	NwMesh &new_mesh_ = *static_cast< NwMesh* >( instance );

	//
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	return NwMesh::LoadFromStream(
		new_mesh_
		, stream
		, context.load_flags
		);
}

void TbMeshLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	UNDONE;
}

ERet TbMeshLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

}//namespace Rendering

//tbPRINT_SIZE_OF(NwMesh::TbMesh_Header_d);
