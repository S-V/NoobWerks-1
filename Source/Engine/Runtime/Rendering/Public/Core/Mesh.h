/*
	Graphics mesh.
*/
#pragma once

// NwBlob
#include <Base/Template/Containers/Blob.h>

// TResourceBase<>
#include <Core/Assets/AssetManagement.h>

#include <Rendering/BuildConfig.h>
#include <Rendering/ForwardDecls.h>
// for VertexFormatT
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Core/Geometry.h>
#include <Rendering/Public/Core/Material.h>


class TbRawMeshData;

namespace Rendering
{

/// A Submesh is a part of a mesh typically associated with a single material.
/// [Submesh|MeshPart|MeshSection|MeshSegment|MeshSubset|MeshCluster|VertexIndexRange|AttributeRange|ModelSurface|PrimitiveGroup|PolygonGroup]
struct Submesh: CStruct
{
	U32		start_index;	//!< Offset of the first index
	U32		index_count;	//!< Number of indices

//TODO?:
//	U32		base_vertex;	//!< Index of the first vertex
//	U32		vertex_count;//!< Number of vertices
public:
	mxDECLARE_CLASS( Submesh, CStruct );
	mxDECLARE_REFLECTION;
};
mxSTATIC_ASSERT_ISPOW2(sizeof(Submesh));

///
enum EMeshFlags {
	MESH_USES_32BIT_IB	= BIT(0),	//!< Is the mesh using a 32-bit index buffer?
	MESH_USES_CLOD		= BIT(1),	//!< Continuous Level-Of-Detail (with geomorphing) or Discrete Level-Of-Detail?
};
mxDECLARE_FLAGS( EMeshFlags, U32, FDrawCallFlags );

/*
-----------------------------------------------------------------------------
	NwMesh - our universal mesh type.
	represents a renderable mesh; doesn't keep shadow copy in system memory.
	it's basically a collection of hardware mesh buffers used for rendering.
	VBs and IB are created from raw mesh data which is loaded in-place.
-----------------------------------------------------------------------------
*/
class NwMesh: public NwSharedResource
{
public:
	typedef TResPtr< NwMesh >	Ref;

public:
	mxOPTIMIZE("cached draw cmd to avoid pointer chasing");
	//NGpu::Cmd_Draw	cached_draw_item;

	/// Vertex and Index buffers.
	NwMeshBuffers	geometry_buffers;

	/// array of mesh subsets, always has one or more elements
	TArray< Submesh >	submeshes;

	/// EMeshFlags (e.g. index buffer format)
	U32		flags;

	/// The total number of vertices.
	U32		vertex_count;

	/// The total number of indices.
	U32		index_count;

	///
	const TbVertexFormat *	vertex_format;

	/// Primitive type.
	NwTopology8	topology;

	/// Local-space bounding box (local AABB in bind pose if this is a skinned mesh).
	AABBf		local_aabb;


	Submesh		z__submeshes_storage[nwRENDERER_NUM_RESERVED_SUBMESHES];


	/// Default materials - these are IDs (i.e. not pointers to materials)
	/// so that different material classes can be instantiated
	TInplaceArray< AssetID, nwRENDERER_NUM_RESERVED_SUBMESHES >	default_materials_ids;

	//Vector4		vertex_scale;	//!< for unpacking vertex positions in shaders (usually = 'dequantize * object_scale')

	// only if the mesh is loaded with KeepMeshDataInRAM flag
	NwBlob	raw_vertex_data;
	NwBlob	raw_index_data;

	// 160 bytes in 32-bit

public:
	mxDECLARE_CLASS( NwMesh, NwSharedResource );
	mxDECLARE_REFLECTION;

	NwMesh();

public:	// Loading

	enum LoadFlags
	{
		/// used for voxelizing triangle meshes on CPU; default = NO
		KeepMeshDataInRAM = BIT(0),
	};

	enum { VERTEX_DATA_ALIGNMENT = 16 };

	static ERet LoadFromStream(
		NwMesh &new_mesh_
		, AReader& stream
		, const LoadFlagsT load_flags = 0
		);


	static ERet CompileMesh(
		AWriter &stream
		, const TbRawMeshData& raw_mesh_data
		, AllocatorI & scratchpad
		);


#pragma pack (push,1)
	/// declarations for loading/parsing renderer assets
	struct TbMesh_Header_d
	{
		U32			magic;		//4
		U32			version;	//4
		AABBf		local_aabb;	//24 local-space bounding box

		TypeGUID	vertex_format;	// TbVertexFormat
		U32			vertex_count;
		U32			index_count;

		U16			submesh_count;
		U8			topology;	// NGpu::Topology
		U8			flags;		// EMeshFlags

	public:
		static const U32 MAGIC = MCHAR4('M','E','S','H');
		static const U32 VERSION = 0;
	};
	ASSERT_SIZEOF(TbMesh_Header_d, 48);
	#pragma pack (pop)

public:
	void release()
	{
		geometry_buffers.release();
		vertex_count = 0;
		index_count = 0;
	}
};

//M34f
//graphicsTransform = M34_Pack( initialTransform );
//M44f m4x4 = M44_From_Float3x4(*_graphicsTransform);
//const M44f worldMatrix = M34_Unpack( *model.m_transform );
mxTODO("M34_Unpack vs M44_From_Float3x4")

struct RrTransform: CStruct
{
	M44f	local_to_world;
public:
	mxDECLARE_CLASS( RrTransform, CStruct );
	mxDECLARE_REFLECTION;

	void setLocalToWorldMatrix( const M44f& m ) { local_to_world = m; }
	const M44f& getLocalToWorld4x4Matrix() const { return local_to_world; }
};

}//namespace Rendering



namespace Rendering
{

///
class TbMeshLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	TbMeshLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;
};

}//namespace Rendering


namespace Rendering
{
ERet submitMesh(
	const NwMesh& mesh
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const TbPassMaskT allowed_passes_mask = ~0
	);

ERet submitMeshWithCustomMaterial(
	const NwMesh& mesh
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, const Material* material
	, NGpu::NwRenderContext & render_context
	, const TbPassMaskT allowed_passes_mask = ~0
	);

mxFORCEINLINE
void SetMeshState(
	NGpu::Cmd_Draw &draw_command_
	, const NwMesh& mesh
	)
{
	mxOPTIMIZE("cache render state in mesh to reduce pointer chasing");
	draw_command_.SetMeshState(
		mesh.vertex_format->input_layout_handle
		, mesh.geometry_buffers.VB->buffer_handle
		, mesh.geometry_buffers.IB->buffer_handle
		, mesh.topology
		, mesh.flags & Rendering::MESH_USES_32BIT_IB
		);
}

}//namespace Rendering
