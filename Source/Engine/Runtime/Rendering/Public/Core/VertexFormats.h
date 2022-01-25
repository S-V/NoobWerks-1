// Engine-specific vertex formats descriptions.
// A vertex format is used to describe the contents of vertices
// that are stored interleaved in a single data stream.
#pragma once

/// Compress vertex positions for CLOD/geomorphing?
/// NOTE: must be sync'ed with shader code "_VS.h"!
#define CLOD_USE_PACKED_VERTICES	(0)


#include <Base/Object/Reflection/ClassDescriptor.h>

// for VertexDescription
#include <GPU/Public/graphics_types.h>


typedef void BuildVertexDescriptionFun( NwVertexDescription *description_ );

///
class TbVertexFormat
	: public CStruct
	, public TbMetaClass
	, public TLinkedList< TbVertexFormat >
{
public:
	HInputLayout	input_layout_handle;

	BuildVertexDescriptionFun *	build_vertex_description_fun;

public:
	static TbVertexFormat *	s_all;

public:
	TbVertexFormat(
		const char* class_name,
		const TypeGUID& class_guid,
		const TbMetaClass* const base_class,
		const SClassDescription& class_info,
		const TbClassLayout& reflected_members,
		BuildVertexDescriptionFun* build_vertex_description_fun
	) : TbMetaClass(
		class_name,
		class_guid,
		base_class,
		class_info,
		reflected_members,
		nil /* Asset Loader */
	) {
		this->PrependSelfToList( &s_all );

		input_layout_handle.SetNil();

		this->build_vertex_description_fun = build_vertex_description_fun;
	}
};

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
#define tbDECLARE_VERTEX_FORMAT( CLASS )\
	private:\
		static TbVertexFormat ms_static_vertex_format;\
		friend class TypeHelper;\
	public:\
		mxFORCEINLINE static TbVertexFormat& metaClass() { return ms_static_vertex_format; };\
		typedef CLASS ThisType;\


//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define tbDEFINE_VERTEX_FORMAT( CLASS )\
	TbVertexFormat	CLASS::ms_static_vertex_format(\
					mxEXTRACT_TYPE_NAME( CLASS ), mxEXTRACT_TYPE_GUID( CLASS ),\
					nil /*superclass*/,\
					SClassDescription::For_Class_With_Default_Ctor< CLASS >(),\
					CLASS::staticLayout(),\
					&CLASS::buildVertexDescription\
					);

/*
===============================================================================
*/

#pragma pack(push,1)


/*
===============================================================================
	SIMPLE VERTICES
===============================================================================
*/

/// Position-only
struct P3f: CStruct
{
	V3f	xyz;	// POSITION
public:
	tbDECLARE_VERTEX_FORMAT(P3f);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

struct P3f_TEX2f: CStruct
{
	V3f		xyz;	// POSITION
	V2f		uv;		// TEXCOORD
	//20
public:
	tbDECLARE_VERTEX_FORMAT(P3f_TEX2f);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

struct VTX_P3f_TEX2h: CStruct
{
	V3f		xyz;	//12 POSITION
	Half2	uv;		//4 TEXCOORD
	//16
public:
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

/// for screen-space rendering
struct P4f_TEX2f: CStruct
{
	V4f		xyzw;	// POSITION
	V2f		uv;		// TEXCOORD
};

/// for screen-space quads
struct P3f_TEX2f_N4Ub_T4Ub: CStruct
{
	V3f		xyz;	//12 POSITION
	V2f		uv;		//8 TEXCOORD
	UByte4	N;		//4 NORMAL
	UByte4	T;		//4 TANGENT
};

/// for screen-space quads
struct P3f_TEX2s_N4Ub_T4Ub: CStruct
{
	V3f		xyz;	//12 POSITION
	INT16	uv[2];	//4 TEXCOORD
	UByte4	N;		//4 NORMAL
	UByte4	T;		//4 TANGENT
};

/// for screen-space quads
struct P3f_TEX2f_COL4Ub: CStruct
{
	V3f		xyz;	//12 POSITION
	V2f		uv;		//8 TEXCOORD
	UByte4	rgba;	//4 COLOR
};

/*
===============================================================================
	GUI
===============================================================================
*/

// see ImDrawVert
struct Vertex_ImGui: CStruct
{
	V2f  pos;
	V2f  uv;
	U32  col;
	//20 bytes
public:
	tbDECLARE_VERTEX_FORMAT(Vertex_ImGui);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};


#if 0

struct Vertex_Nuklear: CStruct
{
	V3f  pos;
	V2f  uv;
	U32  col;
	//24 bytes
public:
	tbDECLARE_VERTEX_FORMAT(Vertex_Nuklear);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( VertexDescription *description_ );
};

#else

typedef Vertex_ImGui Vertex_Nuklear;

#endif



/*
===============================================================================
	GENERIC VERTEX TYPE
===============================================================================
*/

/// Generic vertex type used for rendering both static and skinned meshes.
struct DrawVertex: CStruct
{
	V3f		xyz;	//12 POSITION
	Half2	uv;		//4  TEXCOORD		DXGI_FORMAT_R16G16_FLOAT
	UByte4	N;		//4  NORMAL			DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	T;		//4  TANGENT		DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	indices;//4  BLENDINDICES	DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	weights;//4  BLENDWEIGHT	DXGI_FORMAT_R8G8B8A8_UNORM
	//32 bytes
public:
	tbDECLARE_VERTEX_FORMAT(DrawVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

/*
===============================================================================
	SKINNED/SKELETAL MESHES
===============================================================================
*/

/// Skinned mesh (for skeletal animation (GPU/hardware skinning))
struct Vertex_Skinned: CStruct
{
	V3f		position;	//12 POSITION bind pose vertex position
	Half2	texCoord;	//4  TEXCOORD

	UByte4	normal;		//4  NORMAL
	UByte4	tangent;	//4  TANGENT

	UByte4	indices;	//4  BLENDINDICES joint/bone indices
	UByte4	weights;	//4  BLENDWEIGHT joint/bone weights/influences
	//32 bytes

	enum { MAX_BONES = 255, MAX_WEIGHTS = 4 };

public:
	tbDECLARE_VERTEX_FORMAT(Vertex_Skinned);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

/*
===============================================================================
	STATIC GEOMETRY
===============================================================================
*/
/// 32 bytes
struct Vertex_Static: CStruct
{
	V3f		position;
	Half2	tex_coord;

	UByte4	normal;
	UByte4	tangent;

	UByte4	color;

	R10G10B1A2	normal_for_UV_calc;
	//32 bytes
public:
	tbDECLARE_VERTEX_FORMAT(Vertex_Static);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
ASSERT_SIZEOF(Vertex_Static, 32);



#if 0
// NOTE: for storing position could use V2f (DXGI_FORMAT_R16G16B16A16_SNORM, 8 bytes)
struct VTX_STATIC
{
	// stream 0 - 16 bytes
	V3f	pos;	//12 DXGI_FORMAT_R16G16B16A16_SNORM
	Half2	uv;		//4 DXGI_FORMAT_R16G16_SNORM
	// stream 1 - 8 bytes
	UByte4	N;		//4 DXGI_FORMAT_R8G8B8A8_UNORM
	UByte4	T;		//4 DXGI_FORMAT_R8G8B8A8_UNORM
};
struct VTX_SKINNED
{
	V3f	pos;	//12 POSITION
	Half2	uv;		//4  TEXCOORD		DXGI_FORMAT_R16G16_FLOAT

	UByte4	N;		//4  NORMAL			DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	T;		//4  TANGENT		DXGI_FORMAT_R8G8B8A8_UINT

	UByte4	indices;//4  BLENDINDICES	DXGI_FORMAT_R8G8B8A8_UINT
	UByte4	weights;//4  BLENDWEIGHT	DXGI_FORMAT_R8G8B8A8_UNORM
	//32 bytes
};
#endif


/*
===============================================================================
	PARTICLE RENDERING
===============================================================================
*/

/// Point -> Quad expansion in Geometry Shader
struct ParticleVertex: CStruct
{
	// size - radius in world space
	V4f		position_and_size;

	// color - used by shader
	// life: [0..1] is used for alpha
	V4f		color_and_life;

public:
	tbDECLARE_VERTEX_FORMAT(ParticleVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

/*
===============================================================================
	VOXEL TERRAIN
===============================================================================
*/

/// Voxel terrain with 'stitching' (Discrete LoD)
/// this structure must be as small as possible;
/// tangent frame can be derived in shaders.
struct Vertex_DLOD: CStruct
{
	U32		xy;			//!< (16;16) - quantized X and Y of normalized position, relative to the chunk's minimal corner
	U32		z_and_mat;	//!< lower 16 bits - Z pos, remaining bits - material IDs
	UByte4	N;			//!< object-space normal
	UByte4	color;		//!< RGB - vertex color, A - ambient occlusion
	//16
public:
	tbDECLARE_VERTEX_FORMAT(Vertex_DLOD);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
mxSTATIC_ASSERT_ISPOW2(sizeof(Vertex_DLOD));

#define DLOD_Encode_Position(X)	Encode_XYZ01_to_R11G11B10((X))
#define DLOD_Decode_Position(X)	Decode_XYZ01_from_R11G11B10((X))

/// Voxel terrain with geomorphing (Continuous LoD).
/// contains vertex attributes (position, normal, color, etc.)
/// both at this (fine) LoD and at the parent's (coarser) LoD
/// for Continuous Level-Of-Detail (CLOD) via geomorphing.
#if CLOD_USE_PACKED_VERTICES
	/// Quantized fields are unpacked in the vertex shader using bitwise instructions.
	struct Vertex_CLOD
	{
		U32		xyz0;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		U32		xyz1;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		UByte4	N0;		//4 XYZ - fine normal, W - unused
		UByte4	N1;		//4 XYZ - coarse normal, W - unused

		// RGB - vertex color, A - ambient occlusion
		U32		color0;	//4
		U32		color1;	//4

		// material IDs
		UByte4	materials;
		U32		miscFlags;	//!< boundary vertex type, see ECellType
		//32 bytes
	public:
		static const EVertexFormat Type = EVertexFormat::VTX_CLOD;
		static void buildVertexDescription( NwVertexDescription *description_ );	
	};
	mxSTATIC_ASSERT_ISPOW2(sizeof(Vertex_CLOD));
#else
	/// full precision
	struct Vertex_CLOD
	{
		V3f		xyz0;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		UByte4	N0;		//4 XYZ - fine normal, W - unused
		V3f		xyz1;	//4 (11;11;10) - quantized position, relative to the chunk's minimal corner
		UByte4	N1;		//4 XYZ - coarse normal, W - unused

		// RGB - vertex color, A - ambient occlusion
		U32		color0;	//4
		U32		color1;	//4

		// material IDs
		UByte4	materials;
		U32		miscFlags;	//!< boundary vertex type, see ECellType
		//48 bytes
	public:
		static void buildVertexDescription( NwVertexDescription *description_ );
	};
#endif

///
struct Vertex_VoxelTerrainTextured: CStruct
{
	U32		q_pos;	// quantized position
	Half2	uv;		// texture coordinates
	UByte4	N;		// normal
	UByte4	T;		// tangent
	//16 bytes
public:
	tbDECLARE_VERTEX_FORMAT(Vertex_VoxelTerrainTextured);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};


/// for stamping meshes onto voxel terrain and drawing them in the editor

/// disable UVs so that we can use practically all meshes for stamping
#define MESH_STAMP_SUPPORT_UV	(0)


#if MESH_STAMP_SUPPORT_UV

	struct MeshStampVertex: CStruct
	{
		V3f		xyz;	//12
		V2f		uv;		//8
		//20
	public:
		tbDECLARE_VERTEX_FORMAT(MeshStampVertex);
		mxDECLARE_REFLECTION;
		static void buildVertexDescription( NwVertexDescription *description_ );
	};

#else

	struct MeshStampVertex: CStruct
	{
		V3f		xyz;	//12
		//12
	public:
		tbDECLARE_VERTEX_FORMAT(MeshStampVertex);
		mxDECLARE_REFLECTION;
		static void buildVertexDescription( NwVertexDescription *description_ );
	};

#endif



/*
===============================================================================
	GAME-SPECIFIC VERTEX FORMATS (SO THAT THE ASSET PIPELINE SEES THEM HERE)
===============================================================================
*/

/// used for drawing lots of instanced spaceships
struct SpaceshipVertex: CStruct
{
	Half4	position;	// 8
	//Half2	uv;			// 4
	//UByte4	normal;		// 4

public:
	tbDECLARE_VERTEX_FORMAT(SpaceshipVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
ASSERT_SIZEOF(SpaceshipVertex, 8);





/*
===============================================================================
	DEBUG RENDERING
===============================================================================
*/
extern TbVertexFormat	AuxVertex_vertex_format;




/// Point -> OBB expansion in Geometry Shader
struct RrBillboardVertex: CStruct
{
	// size - radius in world space
	V4f		position_and_size;

	// color - used by shader
	// life: [0..1] is used for alpha
	V4f		color_and_life;

public:
	tbDECLARE_VERTEX_FORMAT(RrBillboardVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
ASSERT_SIZEOF(RrBillboardVertex, 32);


#pragma pack(pop)
