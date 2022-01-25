//@todo: rename this file to "graphics_assets.h"?
#pragma once

//#include <optional-lite/optional.hpp>

//@todo: remove this (but mesh structs still need math types)
#include <Base/Base.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Assets/AssetReference.h>
#include <Core/VectorMath.h>
#include <GPU/Public/graphics_types.h>

/*
=====================================================================
    RENDER TARGETS
=====================================================================
*/

/// the size of a render target in one dimension (X or Y)
struct TbTextureSize: CStruct
{
	float	size;		// either absolute or relative
	bool	relative;	// size mode (default = false)
public:
	mxDECLARE_CLASS( TbTextureSize, CStruct );
	mxDECLARE_REFLECTION;
	TbTextureSize()
	{
		size = 0.0f;
		relative = false;
	}
	void SetAbsoluteSize( int absoluteSize )
	{
		size = absoluteSize;
		relative = false;	// dimensions are constant values
	}
	void SetRelativeSize( float relativeSize )
	{
		size = relativeSize;
		relative = true;	// dimensions depend on back buffer size
	}
};

///NOTE: textures keep initial descriptors, because they can be resized
struct TbTextureBase: NamedObject
{
	TbTextureSize	sizeX;	//!< width
	TbTextureSize	sizeY;	//!< height
	//U8				depth;
	DataFormatT		format;
	U8				numMips;
	GrTextureCreationFlagsT	flags;
public:
	mxDECLARE_CLASS(TbTextureBase, NamedObject);
	mxDECLARE_REFLECTION;
	TbTextureBase();

	bool DependsOnBackBufferSize() const
	{
		return sizeX.relative || sizeY.relative;
	}
};
struct TbTexture2D : TbTextureBase
{
	HTexture	handle;
public:
	mxDECLARE_CLASS(TbTexture2D, TbTextureBase);
	mxDECLARE_REFLECTION;
	TbTexture2D();
};

struct TbColorTarget : TbTextureBase
{
	HColorTarget	handle;	
public:
	mxDECLARE_CLASS(TbColorTarget, TbTextureBase);
	mxDECLARE_REFLECTION;
	TbColorTarget();
};
struct TbDepthTarget : TbTextureBase
{
	HDepthTarget	handle;
	bool			sample;	// Can it be bound as a shader resource?
public:
	mxDECLARE_CLASS(TbDepthTarget, TbTextureBase);
	mxDECLARE_REFLECTION;
	TbDepthTarget();
};

/*
=====================================================================
    RENDER STATES
=====================================================================
*/
struct FxDepthStencilState : NwDepthStencilDescription
{
	HDepthStencilState	handle;
public:
	mxDECLARE_CLASS(FxDepthStencilState, NwDepthStencilDescription);
	mxDECLARE_REFLECTION;
	FxDepthStencilState();
};

struct FxRasterizerState : NwRasterizerDescription
{
	HRasterizerState	handle;
public:
	mxDECLARE_CLASS(FxRasterizerState, NwRasterizerDescription);
	mxDECLARE_REFLECTION;
	FxRasterizerState();
};

struct FxSamplerState : NwSamplerDescription
{
	HSamplerState	handle;
public:
	mxDECLARE_CLASS(FxSamplerState, NwSamplerDescription);
	mxDECLARE_REFLECTION;
	FxSamplerState();
};

struct FxBlendState : NwBlendDescription
{
	HBlendState		handle;
public:
	mxDECLARE_CLASS(FxBlendState, NwBlendDescription);
	mxDECLARE_REFLECTION;
	FxBlendState();
};

struct NwRenderState: NwSharedResource, NwRenderState32
{
	typedef TResPtr< NwRenderState >	Ref;

public:
	mxDECLARE_CLASS(NwRenderState, NwSharedResource);
	mxDECLARE_REFLECTION;
	NwRenderState();

	static AssetID defaultAssetId() { return MakeAssetID("default"); }
};

/*
=====================================================================
    TEXTURE RESOURCE
=====================================================================
*/

/// The texture class was moved from the high-level renderer
/// so that shaders can automatically Initialize their texture bindings.
struct NwTexture: NwSharedResource
{
	HTexture		m_texture;	//!< handle of hardware texture
	HShaderInput	m_resource;	//!< cached shader resource handle
public:
	typedef TPtr< NwTexture >	Ref;
public:
	mxDECLARE_CLASS( NwTexture, NwResource );
	mxDECLARE_REFLECTION;
	NwTexture();

public:
	ERet loadFromMemory(
		const NGpu::Memory* mem_block
		, const GrTextureCreationFlagsT flags = NwTextureCreationFlags::defaults
		);
};

/*
=====================================================================
    SHADERS
=====================================================================
*/


/// a shader feature bit mask
typedef U32 NwShaderFeatureBitMask;

/*
=============================================================================
	GEOMETRY
=============================================================================
*/

enum EAnimationConstants
{
	// Maximum number of bones in a mesh.
	MAX_MESH_BONES = 256,
	// Each skinned vertex has a maximum of 4 bone indices and 4 weights for each bone.
	MAX_INFLUENCES = 4
};

struct BoneWeight
{
	int	index;
	float weight;
};

/*
=======================================================================
	RUN-TIME MESH FORMAT (LEAN & SLIM)
=======================================================================
*/
struct RawVertexStream: CStruct
{
	TBuffer< BYTE >	data;	// this is how raw vertex data is stored on disk
	U32				stride;	//!< the size of a single vertex

public:
	mxDECLARE_CLASS( RawVertexStream, CStruct );
	mxDECLARE_REFLECTION;
	RawVertexStream() : stride(0) {}
	UINT SizeInBytes() const { return data.rawSize(); }
	const BYTE* ToVoidPtr() const { return this->data.raw(); }
};

struct RawVertexData: CStruct
{
	TBuffer< RawVertexStream >	streams;//!< vertex streams
	U32							count;	//!< number of vertices
	TypeGUID					vertex_format_id;	//!< tells us how to interpret raw bytes of vertex data

public:
	mxDECLARE_CLASS( RawVertexData, CStruct );
	mxDECLARE_REFLECTION;
	RawVertexData()
		: count(0), vertex_format_id(mxNULL_TYPE_ID)
	{}
};

/// used for storing index data on disk
struct RawIndexData: CStruct
{
	TBuffer< BYTE >	data;
	U16				stride;	//!< index size - 2 or 4 bytes
public:
	mxDECLARE_CLASS( RawIndexData, CStruct );
	mxDECLARE_REFLECTION;
	RawIndexData()
		: stride(0)
	{}
	// Returns the number of indices in the buffer.
	UINT NumIndices() const			{return data.rawSize() / stride;}
	UINT SizeInBytes() const		{return data.rawSize();}
	bool is32bit() const			{return stride == sizeof(U32);}

	void* ToVoidPtr()				{return data.raw();}
	const void* ToVoidPtr() const	{return data.raw();}
};

/// part of mesh typically associated with a single material
struct RawMeshPart: CStruct
{
	U32		base_vertex;		//!< index of the first vertex
	U32		start_index;		//!< offset of the first index
	U32		index_count;		//!< number of indices
	U32		vertex_count;	//!< number of vertices
	AssetID	material_id;

public:
	mxDECLARE_CLASS( RawMeshPart, CStruct );
	mxDECLARE_REFLECTION;
};

/// bind-pose skeleton's joint
struct Bone: CStruct
{
	Vector4	orientation;	//16 joint's orientation quaternion
	V3f		position;		//12 joint's position in object space
	INT32	parent;			//4 index of the parent joint; (-1) if this is the root joint
public:
	mxDECLARE_CLASS( Bone, CStruct );
	mxDECLARE_REFLECTION;
	Bone();
};

/// Skeleton/Armature/JointTree
/// Describes the hierarchy of the bones and the skeleton's binding pose.
/// and can be deformed by an animation clip.
struct Skeleton: CStruct
{
	TBuffer< Bone >		bones;			//!< bind-pose bones data (in object space)
	TBuffer< String >	boneNames;
	TBuffer< M34f >		invBindPoses;	//!< inverse bind pose matrices
public:
	mxDECLARE_CLASS( Skeleton, CStruct );
	mxDECLARE_REFLECTION;
	Skeleton();
};

/// Raw mesh data that can be used for loading/filling hardware mesh buffers.
struct TbRawMeshData: CStruct
{
	RawVertexData			vertexData;	//!< raw vertex data
	RawIndexData			indexData;	//!< raw index data
	NwTopology8				topology;	//!< primitive type
	Skeleton				skeleton;
	AABBf					bounds;		//!< local-space bounding box
	TBuffer< RawMeshPart >	parts;

public:
	mxDECLARE_CLASS( TbRawMeshData, CStruct );
	mxDECLARE_REFLECTION;
	TbRawMeshData();
};

/*
=====================================================================
    FUNCTIONS
=====================================================================
*/


extern const char* MISSING_TEXTURE_ASSET_ID;
