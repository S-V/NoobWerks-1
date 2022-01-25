/*
=============================================================================
	Low-level platform-agnostic graphics layer
	Basic types
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/TFixedArray.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/LoadInPlace/LoadInPlaceTypes.h>
#include <GPU/Public/graphics_config.h>

/*
=====================================================================
	OPAQUE HANDLES
=====================================================================
*/
#define LLGL_DEFAULT_ID		(0)
#define LLGL_NIL_HANDLE	(~0)
/// Resource handles
mxDECLARE_8BIT_HANDLE(HDepthStencilState);
mxDECLARE_8BIT_HANDLE(HRasterizerState);
mxDECLARE_8BIT_HANDLE(HSamplerState);
mxDECLARE_8BIT_HANDLE(HBlendState);
mxDECLARE_8BIT_HANDLE(HInputLayout);
mxDECLARE_8BIT_HANDLE(HColorTarget);
mxDECLARE_8BIT_HANDLE(HDepthTarget);
mxDECLARE_16BIT_HANDLE(HTexture);
mxDECLARE_16BIT_HANDLE(HBuffer);	// generic buffer handle (e.g. vertex, index, uniform)
mxDECLARE_16BIT_HANDLE(HShader);	// handle to a shader object (e.g. vertex, pixel)
mxDECLARE_16BIT_HANDLE(HProgram);	// handle to a program object
mxDECLARE_16BIT_HANDLE(HShaderInput);// shader resource is everything that can be sampled in a shader: textures, render targets, etc.
mxDECLARE_16BIT_HANDLE(HShaderOutput);// Direct3D's Unordered Access Views or OpenGL's Indirect Buffers
//mxDECLARE_POINTER_HANDLE(HPipeline);

/// for self-documenting code
typedef HTexture HTexture1D;
typedef HTexture HTexture2D;
typedef HTexture HTexture3D;

mxREFLECT_AS_BUILT_IN_INTEGER(HDepthStencilState);
mxREFLECT_AS_BUILT_IN_INTEGER(HRasterizerState);
mxREFLECT_AS_BUILT_IN_INTEGER(HSamplerState);
mxREFLECT_AS_BUILT_IN_INTEGER(HBlendState);
mxREFLECT_AS_BUILT_IN_INTEGER(HShaderInput);
mxREFLECT_AS_BUILT_IN_INTEGER(HProgram);
mxREFLECT_AS_BUILT_IN_INTEGER(HShader);
mxREFLECT_AS_BUILT_IN_INTEGER(HBuffer);
mxREFLECT_AS_BUILT_IN_INTEGER(HShaderOutput);


mxDEPRECATED
struct NamedObject: CStruct
{
	String		name;	//!< Name. Can be empty in release builds.
	NameHash32	hash;	//!< Name hash, always valid.
public:
	mxDECLARE_CLASS(NamedObject, CStruct);
	mxDECLARE_REFLECTION;
	NamedObject();
	void UpdateNameHash();	// uses GetDynamicStringHash()
};//12/16


// base struct for description structs
typedef CStruct NwDescriptionBase;


/*
=====================================================================
	TEXTURES AND RENDER TARGETS
=====================================================================
*/
/// texture format for storing color information
struct NwDataFormat
{
	enum Enum
	{
#define DECLARE_DATA_FORMAT( ENUM, BPP, BW, BH, BS, BX, BY, DB, SB )	ENUM,
#include <GPU/Public/graphics_data_formats.inl>
#undef DECLARE_DATA_FORMAT
		MAX	//!< Marker. Don't use!
	};
	static bool IsCompressed( Enum _format );	//!< returns true if the given format is block-compressed
	static UINT BitsPerPixel( Enum _format );
	static UINT GetBlockSize( Enum _format );	//!< returns the texture block size in elements
};
mxDECLARE_ENUM( NwDataFormat::Enum, U8, DataFormatT );

struct NwCubemapFace
{
	enum Enum
	{
		PosX,	NegX,
		PosY,	NegY,
		PosZ,	NegZ
	};
};
mxDECLARE_ENUM( NwCubemapFace::Enum, U8, NwCubeFace8 );

/// Flags used for texture creation.
struct NwTextureCreationFlags
{
	enum Enum
	{
		sRGB			= BIT(0),	//!< Texture is encoded in sRGB gamma space
		DYNAMIC			= BIT(1),	//!< Can it be updated by CPU?
		RANDOM_WRITES	= BIT(2),	//!< Can it have unordered access views?
		GENERATE_MIPS	= BIT(3),	//!< Enable automatic mipmap generation?

		none			= 0,
		defaults		= none,
	};
};
mxDECLARE_FLAGS( NwTextureCreationFlags::Enum, U8, GrTextureCreationFlagsT );

struct NwTexture2DDescription: NwDescriptionBase
{
	U16				width;
	U16				height;
	DataFormatT		format;
	U8				numMips;
	U8				arraySize;
	GrTextureCreationFlagsT	flags;
public:
	mxDECLARE_CLASS(NwTexture2DDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwTexture2DDescription(ENoInit) {}
	NwTexture2DDescription();

	U32 CalcRawSize() const;
};
struct NwTexture3DDescription: NwDescriptionBase
{
	U16				width;
	U16				height;
	U16				depth;
	DataFormatT		format;
	U8				numMips;
	GrTextureCreationFlagsT	flags;
public:
	mxDECLARE_CLASS(NwTexture3DDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwTexture3DDescription(ENoInit) {}
	NwTexture3DDescription();

	U32 CalcRawSize() const;
};

struct NwColorTargetDescription: NwDescriptionBase
{
	U16				width;
	U16				height;
	DataFormatT		format;
	U8				numMips;
	GrTextureCreationFlagsT	flags;
public:
	mxDECLARE_CLASS(NwColorTargetDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwColorTargetDescription();
	bool IsValid() const;
};
/// describes a depth-stencil surface
struct NwDepthTargetDescription: NwDescriptionBase
{
	U16			width;
	U16			height;
	DataFormatT	format;
	bool		sample;	//!< Can it be bound as a shader resource?
public:
	mxDECLARE_CLASS(NwDepthTargetDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwDepthTargetDescription();
};

U32 estimateTextureSize( NwDataFormat::Enum format
						, U16 width
						, U16 height
						, U16 depth = 1
						, U8 numMips = 1
						, U16 arraySize = 1
						);

#pragma pack (push,1)

struct NwResolution
{
	U16	width;
	U16	height;
};
union NwViewport
{
	struct {
		U16	width;
		U16	height;
		U16	x;	//!< Top left x.
		U16	y;	//!< Top left y.
	};
	U64	u;
};
union NwRectangle64
{
	struct {
		U16	left;	//!< The upper-left corner x-coordinate.
		U16	top;	//!< The upper-left corner y-coordinate.
		U16	right;	//!< The lower-right corner x-coordinate.
		U16	bottom;	//!< The lower-right corner y-coordinate.
	};
	U64	u;
};
struct NwBoxRegion
{
    U16	left;
    U16	top;
    U16	front;
    U16	right;
    U16	bottom;
    U16	back;
};
struct NwTextureRegion
{
	U16	x;
	U16	y;
	U16	z;
	U16	width;	//!< should always be greater than zero
	U16	height;	//!< should always be greater than zero
	U16	depth;	//!< should always be greater than zero
	U16	arraySlice;	//!< index in an array of textures/cubemap side
	U8	mipMapLevel;//!< mipmap level
	U8	unusedPadding;
};//16 bytes

#pragma pack (pop)

inline
bool isInvalidViewport( const NwViewport& viewport )
{
	return viewport.width == 0 || viewport.height == 0;
}

/*
=====================================================================
	RENDER STATES
=====================================================================
*/
struct NwComparisonFunc
{
	enum Enum {
		Always,
		Never,
		Less,
		Equal,
		Greater,
		Not_Equal,
		Less_Equal,
		Greater_Equal,
	};
};
mxDECLARE_ENUM( NwComparisonFunc::Enum, U8, NwComparisonFunc8 );

struct NwFillMode
{
	enum Enum {
		Solid,
		Wireframe,
	};
};
mxDECLARE_ENUM( NwFillMode::Enum, U8, NwFillMode8 );

struct NwCullMode
{
	enum Enum {
		None,	// Always draw all triangles.
		Back,	// Do not draw triangles that are back-facing (default).
		Front,	// Do not draw triangles that are front-facing.
	};
};
mxDECLARE_ENUM( NwCullMode::Enum, U8, NwCullMode8 );

/// Filtering options to use for minification, magnification and mip-level sampling during texture sampling.
struct NwTextureFilter
{
    enum Enum {
        Min_Mag_Mip_Point,			//!< choose nearest texel in nearest mipmap
        Min_Mag_Point_Mip_Linear,
        Min_Point_Mag_Linear_Mip_Point,
        Min_Point_Mag_Mip_Linear,
        Min_Linear_Mag_Mip_Point,
        Min_Linear_Mag_Point_Mip_Linear,
        Min_Mag_Linear_Mip_Point,
        Min_Mag_Mip_Linear,	//!< interpolate 8 texels from 2 mipmaps
        Anisotropic,
    };
};
mxDECLARE_ENUM( NwTextureFilter::Enum, U8, NwTextureFilter8 );

struct NwTextureAddressMode
{
	enum Enum {
		Wrap,
		Clamp,
		Border,		//!< not supported on all platforms
		Mirror,
		MirrorOnce	//!< not supported on all platforms
	};
};
mxDECLARE_ENUM( NwTextureAddressMode::Enum, U8, NwTextureAddressMode8 );

/// RGB or alpha blending operation.
struct NwBlendOp
{
	enum Enum {
		MIN,
		MAX,
		ADD,
		SUBTRACT,
		REV_SUBTRACT,
	};
};
mxDECLARE_ENUM( NwBlendOp::Enum, U8, BlendOp8 );

/// Blend factors modulate values for the pixel shader and render target.
struct NwBlendFactor
{
	enum Enum {
		ZERO,
		ONE,

		SRC_COLOR,
		DST_COLOR,
		SRC_ALPHA,
		DST_ALPHA,
		SRC1_COLOR,
		SRC1_ALPHA,
		BLEND_FACTOR,

		INV_SRC_COLOR,
		INV_DST_COLOR,
		INV_SRC_ALPHA,		
		INV_DST_ALPHA,
		INV_SRC1_COLOR,
		INV_SRC1_ALPHA,
		INV_BLEND_FACTOR,

		SRC_ALPHA_SAT,		
	};
};
mxDECLARE_ENUM( NwBlendFactor::Enum, U8, NwBlendFactor8 );

struct NwStencilOp
{
	enum Enum {
		KEEP,		//!< Keeps the current value.
		ZERO,		//!< Sets the stencil buffer value to 0.
		INVERT,		//!< Bitwise inverts the current stencil buffer value.
		REPLACE,	//!< Sets the stencil buffer value to the reference value.
		INCR_SAT,	//!< Increment the stencil value by 1, and clamp the result to maximum unsigned value.
		DECR_SAT,	//!< Decrement the stencil value by 1, and clamp the result to zero.
		INCR_WRAP,	//!< Increment the stencil value by 1, and wrap the result if necessary.
		DECR_WRAP,	//!< Decrement the stencil value by 1, and wrap the result if necessary.		
	};
};
mxDECLARE_ENUM( NwStencilOp::Enum, U8, NwStencilOp8 );

struct NwColorWriteMask
{
	enum Flags {
		Enable_Red		= BIT(0),
		Enable_Green	= BIT(1),
		Enable_Blue		= BIT(2),
		Enable_Alpha	= BIT(3),

		Enable_All		= Enable_Red|Enable_Green|Enable_Blue|Enable_Alpha,
	};
};
mxDECLARE_FLAGS( NwColorWriteMask::Flags, U8, NwColorWriteMask8 );

struct ClearMask
{
	enum Flags {
		Clear_Depth		= BIT(0),
		Clear_Stencil	= BIT(1),
		Clear_Color		= BIT(2),
		Clear_All		= Clear_Depth|Clear_Stencil|Clear_Color,
	};
};
mxDECLARE_FLAGS( ClearMask::Flags, U8, BClearMask );

struct NwDepthStencilSide: CStruct
{
	NwComparisonFunc8	stencil_comparison;
	NwStencilOp8		stencil_pass_op;
    NwStencilOp8		stencil_fail_op;
	NwStencilOp8		depth_fail_op;
public:
	mxDECLARE_CLASS(NwDepthStencilSide, CStruct);
	mxDECLARE_REFLECTION;
	NwDepthStencilSide(ENoInit) {}
	NwDepthStencilSide();
};


struct NwDepthStencilFlags
{
	enum Enum
	{
		Enable_DepthTest	= BIT(0),
		Enable_DepthWrite	= BIT(1),
		Enable_Stencil		= BIT(2),

		Defaults			= Enable_DepthTest|Enable_DepthWrite
	};
};
mxDECLARE_FLAGS( NwDepthStencilFlags::Enum, U8, NwDepthStencilFlags8 );

struct NwDepthStencilDescription: NwDescriptionBase
{
	NwDepthStencilFlags8	flags;
	NwComparisonFunc8		comparison_function;// default = Greater (reverse Z)
	U8						stencil_read_mask;	// default = 0xFF
	U8						stencil_write_mask;	// default = 0xFF
	NwDepthStencilSide		front_face;
	NwDepthStencilSide		back_face;
public:
	mxDECLARE_CLASS(NwDepthStencilDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwDepthStencilDescription(ENoInit) : front_face(_NoInit), back_face(_NoInit) {}
	NwDepthStencilDescription();
};
ASSERT_SIZEOF(NwDepthStencilDescription, 12);






struct NwRasterizerFlags
{
	enum Enum
	{
		Enable_DepthClip		= BIT(0),
		Enable_Scissor			= BIT(1),
		Enable_Multisample		= BIT(2),
		Enable_AntialiasedLine	= BIT(3),

		Defaults = Enable_DepthClip
	};
};
mxDECLARE_FLAGS( NwRasterizerFlags::Enum, U8, NwRasterizerFlags8 );

struct NwRasterizerDescription: NwDescriptionBase
{
	NwFillMode8			fill_mode;	// default = solid
	NwCullMode8			cull_mode;	// default = cull back faces
	NwRasterizerFlags8	flags;
	U8					__pad;
public:
	mxDECLARE_CLASS(NwRasterizerDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwRasterizerDescription(ENoInit) {}
	NwRasterizerDescription();
};
ASSERT_SIZEOF(NwRasterizerDescription, 4);





struct NwSamplerDescription: CStruct	// must be POD!
{
	NwTextureFilter8		filter;
	NwTextureAddressMode8	address_U;	//!< The texture addressing mode for the u-coordinate.
	NwTextureAddressMode8	address_V;	//!< The texture addressing mode for the v-coordinate.
	NwTextureAddressMode8	address_W;	//!< The texture addressing mode for the w-coordinate.
	U8						max_anisotropy;	//!< The maximum anisotropy value.
	NwComparisonFunc8		comparison;		//!< Compare the sampled result to the comparison value?
	float				border_color[4];	//!< Border color to use if TextureAddressMode::Border is specified for address_U, address_V, or address_W.
	float				min_LOD;			//!< Lower end of the mipmap range to clamp access to.
	float				max_LOD;			//!< Upper end of the mipmap range to clamp access to.
	float				mip_LOD_bias;		//!< Offset from the calculated mipmap level.
public:
	mxDECLARE_CLASS(NwSamplerDescription, CStruct);
	mxDECLARE_REFLECTION;
	NwSamplerDescription(ENoInit) {}
	NwSamplerDescription();
};
ASSERT_SIZEOF(NwSamplerDescription, 36);





struct NwBlendChannel: CStruct
{
	BlendOp8		operation;
	NwBlendFactor8	source_factor;
	NwBlendFactor8	destination_factor;
public:
	mxDECLARE_CLASS(NwBlendChannel, CStruct);
	mxDECLARE_REFLECTION;
	NwBlendChannel();
};

struct NwBlendFlags
{
	enum Enum
	{
		Enable_Blending			= BIT(0),
		Enable_AlphaToCoverage	= BIT(1),

		Defaults = 0
	};
};
mxDECLARE_FLAGS( NwBlendFlags::Enum, U8, NwBlendFlags8 );

struct NwBlendDescription: NwDescriptionBase
{
	NwBlendFlags8		flags;
	NwBlendChannel		color_channel;
	NwBlendChannel		alpha_channel;
	NwColorWriteMask8	write_mask;
	//8
public:
	mxDECLARE_CLASS(NwBlendDescription, NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwBlendDescription(ENoInit) {}
	NwBlendDescription();
};

/// non-programmable (configurable, FFP) render states
struct NwRenderState32: CStruct
{
	typedef U32 ValueType;	// for reflecting this struct as an unsigned int
	union {
		struct {
			HBlendState			blend_state;
			HRasterizerState	rasterizer_state;
			HDepthStencilState	depth_stencil_state;
			U8					stencil_reference_value;
		};
		ValueType	u;
	};
public:
	mxDECLARE_CLASS(NwRenderState32, CStruct);
	mxDECLARE_REFLECTION;
	void SetNil()
	{
		blend_state.SetNil();
		rasterizer_state.SetNil();
		depth_stencil_state.SetNil();
		stencil_reference_value = 0;
	}
	void setDefaults()
	{
		u = 0;	// zero means default values
	}
};//4
ASSERT_SIZEOF(NwRenderState32, sizeof(NwRenderState32::ValueType));

/*
=====================================================================
	GEOMETRY
=====================================================================
*/

///	enumerates all allowed types of elementary graphics primitives used for rasterization.
struct NwTopology
{
	enum Enum {
		TriangleList  = 0,	//!< A list of triangles, 3 vertices per triangle.
		TriangleStrip = 1,	//!< A string of triangles, 3 vertices for the first triangle, and 1 per triangle after that.
		LineList	  = 2,	//!< A list of points, one vertex per point.
		LineStrip	  = 3,	//!< A strip of connected lines, 1 vertex per line plus one 1 start vertex.
		PointList	  = 4,	//!< A collection of isolated points.
		TriangleFan	  = 5,	//!< A string of triangles, 3 vertices for the first triangle, and 1 per triangle after that.
		Undefined	  = 6,	//!< Error.
		MAX,				//!< Marker. Do not use.
		NumBits = 3
	};
};
mxDECLARE_ENUM( NwTopology::Enum, U8, NwTopology8 );

/** Enumerates data types of vertex attributes:
	Byte,	//!< 8-bit signed integer number
	UByte,	//!< 8-bit unsigned integer number
	Short,	//!< 16-bit signed integer number
	UShort,	//!< 16-bit unsigned integer number
	Half,	//!< 16-bit floating-point number
	Float,	//!< 32-bit floating-point number
	Double,	//!< 64-bit floating-point number
*/
struct NwAttributeType
{
	enum Enum {
		Byte4,	//!< 4 8-bit signed integer number
		UByte4,	//!< 4 8-bit unsigned integer number
		Short2,	//!< 16-bit signed integer number
		UShort2,//!< 16-bit unsigned integer number

		Half2,	//!< 2 16-bit floating-point numbers (1-bit sign, 5-bit exponent, 10-bit mantissa)
		Half4,

		Float1,	//!< 32-bit floating-point number
		Float2,	//!< 2 32-bit floating-point numbers
		Float3,	//!< 3 32-bit floating-point numbers
		Float4,	//!< 4 32-bit floating-point numbers

		UInt1,	//!< 32-bit unsigned integer number
		UInt2,
		UInt3,
		UInt4,

		R11G11B10F,	//!< 3 small floating-point numbers
		R10G10B10A2_UNORM,

		//Double,	//!< 64-bit floating-point number
		Count	//!< Marker. Don't use!
	};
	static U8 GetDimension( Enum _type );
};
mxDECLARE_ENUM( NwAttributeType::Enum, U8, NwAttributeType8 );

/// Describes the meaning (semantics) of vertex components.
struct NwAttributeUsage
{
	enum Enum {
		Position,	//!< Position, 3 floats per vertex.

		Color,		//!< Vertex color.

		Normal,		//!< Normal, 3 floats per vertex.
		Tangent,	//!< X axis if normal is Z
		Bitangent,	//!< Y axis if normal is Z (aka Binormal)

		TexCoord,	//!< Texture coordinates.

		BoneWeights,	//!< 4 weighting factors to matrices
		BoneIndices,	//!< 4 indices to bone/joint matrices		

		Count	//!< Marker. Don't use!
	};
};
mxDECLARE_ENUM( NwAttributeUsage::Enum, U8, NwAttributeUsage8 );

struct NwVertexElement
{
	BITFIELD	dataType	: 4;	//!< NwAttributeType8
	BITFIELD	usageType	: 4;	//!< NwAttributeUsage8
	BITFIELD	usageIndex	: 4;	//!< e.g. TexCoord0, ..., TexCoord7
	BITFIELD	inputSlot	: 3;	//!< [0..LLGL_MAX_VERTEX_STREAMS)
	BITFIELD	normalized	: 1;
};
ASSERT_SIZEOF(NwVertexElement, sizeof(U32));

/// This structure describes the memory layout and format of a vertex buffer.
struct NwVertexDescription: NwDescriptionBase
{
	NwVertexElement	attribsArray[ LLGL_MAX_VERTEX_ATTRIBS ];	//!< array of all vertex elements
	U8				attribOffsets[ LLGL_MAX_VERTEX_ATTRIBS ];	//!< offsets within vertex streams
	U8				streamStrides[ LLGL_MAX_VERTEX_STREAMS ];	//!< strides of each vertex buffer
	U8				attribCount;	//!< total number of vertex components

public:
	NwVertexDescription& begin();
	void end();

	NwVertexDescription& add(
		NwAttributeType8 type,
		NwAttributeUsage8 usage,
		UINT usageIndex,
		bool normalized = false,
		UINT inputSlot = 0
	);

	bool IsValid() const;

	bool ContainsAttribWithUsage( NwAttributeUsage::Enum usage ) const;
};

/*
=====================================================================
    SHADER PROGRAMS
=====================================================================
*/

//NOTE: most frequently used shaders are declared first
struct NwShaderType
{
	enum Enum {
		Pixel,	//!< Fragment shader in OpenGL parlance
		Vertex,
		Compute,	//!< Compute Shader / CUDA kernel
		Geometry,
		Hull,
		Domain,
		//<new shader types can be added here>
		MAX	//!< Marker. Don't use!
	};
};
mxDECLARE_ENUM( NwShaderType::Enum, U8, NwShaderType8 );

struct CBufferBindingOGL: CStruct
{
	String	name;
	U32		slot;//!< constant buffer slot
	U32		size;//!< constant buffer size (used only for debugging)
public:
	mxDECLARE_CLASS(CBufferBindingOGL,CStruct);
	mxDECLARE_REFLECTION;
};
struct SamplerBindingOGL: CStruct
{
	String	name;
	U32		slot;	//!< texture unit index
public:
	mxDECLARE_CLASS(SamplerBindingOGL,CStruct);
	mxDECLARE_REFLECTION;
};
struct ProgramBindingsOGL: CStruct
{
	TArray< CBufferBindingOGL >	cbuffers;	//!< uniform block bindings
	TArray< SamplerBindingOGL >	samplers;	//!< shader sampler bindings
	U32			activeVertexAttributes;	//!< enabled attributes mask
public:
	mxDECLARE_CLASS(ProgramBindingsOGL,CStruct);
	mxDECLARE_REFLECTION;
	void clear();
};

struct NwProgramDescription: NwDescriptionBase
{
	HShader	shaders[NwShaderType::MAX];

	/// platform-specific data for defining binding points;
	/// the data can be discarded after linking the program
	//const ProgramBindingsOGL* bindings;	//!< used only by OpenGL back-end

public:
	mxDECLARE_CLASS(NwProgramDescription,NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwProgramDescription();
};

/*
=====================================================================
    MISCELLANEOUS
=====================================================================
*/
enum EBufferType
{
	Buffer_Uniform,	//!< Shader Constant Buffer
	Buffer_Vertex,
	Buffer_Index,
	Buffer_Data,	//!< Structured Buffer
	Buffer_MAX	//!< Marker. Don't use!
};
mxDECLARE_ENUM( EBufferType, U8, BufferTypeT );

struct NwBufferFlags
{
	enum Enum {
		Sample	= BIT(0),	//!< Can it be accessed in shaders?
		Write	= BIT(1),	//!< Can it have unordered access views?
		Append	= BIT(2),	//!< D3D11_BUFFER_UAV_FLAG_APPEND
		Dynamic	= BIT(3),	//!< Can it be updated on CPU?
		Staging	= BIT(4),	//!< Can it be read by CPU?
		Default = 0,
	};
};
mxDECLARE_FLAGS( NwBufferFlags::Enum, U16, NwBufferFlagsT );

struct NwBufferDescription: NwDescriptionBase
{
	BufferTypeT		type;
	U32				size;
	U16				stride;	//!< Structured Buffer stride
	NwBufferFlagsT	flags;
public:
	mxDECLARE_CLASS(NwBufferDescription,NwDescriptionBase);
	mxDECLARE_REFLECTION;
	NwBufferDescription() {}
	NwBufferDescription(EInitDefault);
};

enum EMapMode
{
    Map_Read,
    Map_Write,
    Map_Read_Write,
    Map_Write_Discard,
    Map_Write_DiscardRange,
};

///
struct NwTransientBuffer
{
	void *	data;	//!< user-writable memory
	U32		size;
	U32		base_index;	//!< starting vertex/index in the VB/IB for this batch
	HBuffer	buffer_handle;	//!< dynamic VB/IB handle
};

// for type safety
struct NwTransientVertexBuffer: NwTransientBuffer {};
struct NwTransientIndexBuffer: NwTransientBuffer {};


#if 0
/*
    The bytecode for all shaders including, vertex, pixel, domain, hull, and geometry shaders.
    The input vertex format.
    The primitive topology type. Note that the input-assembler primitive topology type (point, line, triangle, patch) is set within the PSO using the D3D12_PRIMITIVE_TOPOLOGY_TYPE enumeration. The primitive adjacency and ordering (line list, line strip, line strip with adjacency data, etc.) is set from within a command list using the ID3D12GraphicsCommandList::IASetPrimitiveTopology method.
    The blend state, rasterizer state, depth stencil state.
    The depth stencil and render target formats, as well as the render target count.
    Multi-sampling parameters.
    A streaming output buffer.
    The root signature.
*/
struct PipelineStateDescription
{
};
#endif

/*
=======================================================================
	ENGINE-SPECIFIC FORMATS
=======================================================================
*/

enum ETextureType
{
	TEXTURE_2D		= 0,
	TEXTURE_CUBEMAP	= 1,
	TEXTURE_3D		= 2,
	TEXTURE_1D		= 3,
};

#pragma pack(push,1)
struct TextureHeader_d
{
	U32		magic;	//!< FourCC code
	U32		size;	//!< the total size of image data following this header
	U16		width;	//!< texture width (in texels)
	U16		height;	//!< texture height (in texels)

	union {
		/// The depth of the volume (3D) texture
		/// (arrays of volume textures are not supported).
		U8		depth;

		/// The number of textures in the texture array (6 if cubemap).
		U8		array_size;
	};

	/// ETextureType
	U8		type;

	U8		format;	//!< DataFormatT

	U8		num_mips;//!< mip level count
	
};// ...raw image data is stored right after the header.
#pragma pack(pop)
ASSERT_SIZEOF(TextureHeader_d, 16);

#pragma pack(push,1)
struct ShaderHeader_d
{
	unsigned type: 3;	//!< EShaderType
	unsigned usedCBs: LLGL_MAX_BOUND_UNIFORM_BUFFERS;
	unsigned usedSSs: LLGL_MAX_BOUND_SHADER_SAMPLERS;
	unsigned usedSRs: LLGL_MAX_BOUND_SHADER_TEXTURES;	// Bit-mask of used texture units
};
#pragma pack(pop)
ASSERT_SIZEOF(ShaderHeader_d, 8);

enum MagicCodes: U32
{
	TEXTURE_FOURCC	= MCHAR4('T','M','A','P'),	// "TMAP"
	//SHADER_FOURCC	= MCHAR4('S','H','D','R'),
};

ERet AddBindings(
	ProgramBindingsOGL &destination,
	const ProgramBindingsOGL &source
);
/// simply does register assignment
ERet AssignBindPoints(
	ProgramBindingsOGL & bindings
);
/// validates that the shader input bindings are the same
/// across different shader stages and merges them
ERet MergeBindings(
	ProgramBindingsOGL &destination,
	const ProgramBindingsOGL &source
);

namespace BuiltIns
{
	/// built-in sampler states
	enum ESampler
	{
		DefaultSampler,
		//
		PointSampler,	//!< default sampler
		BilinearSampler,
		TrilinearSampler,
		AnisotropicSampler,

		// these are needed for texturing a voxel terrain
		PointWrapSampler,
		BilinearWrapSampler,
		TrilinearWrapSampler,

		//==
		DiffuseMapSampler,
		DetailMapSampler,
		NormalMapSampler,
		SpecularMapSampler,
		RefractionMapSampler,

		ShadowMapSampler,
		ShadowMapSamplerPCF,	//!< bilinear with HW 2x2 PCF

		Sampler_MAX
	};
	/// initialized upon startup
	extern HSamplerState g_samplers[Sampler_MAX];

	enum EShaderParameterSemantic
	{
		WorldViewMatrix,
		WorldViewProjectionMatrix,
	};
}//namespace BuiltIns

mxDECLARE_ENUM( BuiltIns::ESampler, U32, SamplerID );

struct DataFormat_t
{
	U8 bitsPerPixel;
	U8 blockWidth;	// texels in block
	U8 blockHeight;	// texels in block
	U8 blockSize;	// in bytes
	U8 minBlockX;
	U8 minBlockY;
	U8 depthBits;
	U8 stencilBits;
	//U8 encoding;
};
extern const DataFormat_t g_DataFormats[NwDataFormat::MAX];

/*
=====================================================================
	LEGACY STUFF
=====================================================================
*/
struct DeviceVendor
{
	enum Enum {
		Vendor_Unknown	= 0,
		Vendor_3DLABS	= 1,	//!< 3Dlabs, Inc. Ltd
		Vendor_ATI		= 2,	//!< ATI Technologies Inc. / Advanced Micro Devices, Inc.
		Vendor_Intel	= 3,	//!< Intel Corporation
		Vendor_Matrox	= 4,	//!< Matrox Electronic Globals Ltd.
		Vendor_NVidia	= 5,	//!< NVIDIA Corporation
		Vendor_S3		= 6,	//!< S3 Graphics Co., Ltd
		Vendor_SIS		= 7,	//!< SIS
	};
	static Enum FourCCToVendorEnum( U32 fourCC );
	static const char* GetVendorString( Enum vendorId );
};

struct EImageFileFormat
{
	enum Enum {
		IFF_BMP,
		IFF_JPG,
		IFF_PNG,
		IFF_DDS,
		IFF_TIFF,
		IFF_GIF,
		IFF_WMP,
	};
	// e.g. ".jpg", ".bmp"
	static const char* GetFileExtension( Enum e );
};

namespace NGpu
{
	///
	struct Memory
	{
		union
		{
			//!< A pointer to the data.
			BYTE *					data;

			// The relative offset to the data if it was allocated in the same memory block as this struct was (e.g. inside a command buffer).
			TOffset< BYTE, I32 >	offset;
		};

		/// The size of the referenced memory block, excluding the size of this struct.
		U32			size;

		unsigned	debug_tag : 30;	// for debugging
		unsigned	is_relative : 1;
		//unsigned	owns_memory : 1;

#if mxARCH_TYPE == mxARCH_32BIT
		U32			pad16;
#endif

	public:
		mxFORCEINLINE bool IsValid() const
		{
			return size > 0;
		}

		mxFORCEINLINE const BYTE* getDataPtr() const
		{
			return this->is_relative ? this->offset.ptr() : this->data;
		}

		void clear()
		{
			mxZERO_OUT(*this);
		}

		void setRelativePointer( void* ptr, U32 size )
		{
			this->offset.SetupFromRawPtr( ptr );
			this->size = size;
			this->is_relative = true;
		}

#if MX_DEBUG || MX_DEVELOPER

		friend ATextStream & operator << ( ATextStream & log, const Memory& o );

#endif

	};
	ASSERT_SIZEOF( Memory, 16 );
}//namespace NGpu

/*
=====================================================================
	ASYNC GPU JOB SYSTEM
=====================================================================
*/
// http://www.nickdarnell.com/chaining-compute-shaders/
// https://software.intel.com/en-us/articles/opencl-using-events
// https://stackoverflow.com/questions/33277472/directx-11-compute-shader-device-synchronization
// https://gpuopen.com/performance-tweets-series-barriers-fences-synchronization/
// https://stackoverflow.com/questions/35239204/directx-11-compute-shader-copy-data-from-the-gpu-to-the-cpu
// https://msdn.microsoft.com/en-us/library/windows/desktop/dn899217(v=vs.85).aspx
// https://docs.unity3d.com/ScriptReference/Graphics.DrawMeshInstancedIndirect.html
// https://www.khronos.org/registry/OpenCL/sdk/1.0/docs/man/xhtml/clEnqueueReadBuffer.html
// https://www.khronos.org/registry/OpenCL/sdk/1.1/docs/man/xhtml/clSetEventCallback.html
// http://sa09.idav.ucdavis.edu/docs/SA09-opencl-dg-events-stream.pdf, P.25...
namespace GPU
{
	struct Job
	{
		enum State {
			QUEUED,	//!< Command is in a queue
			SUBMITTED,	//!< Command has been submitted to device
			RUNNING,	//!< Device is currently executing command
			COMPLETE	//!< Command has finished execution
		};
	};
}


/*
=====================================================================
	HIGH-LEVEL TYPES
=====================================================================
*/
//mxDECLARE_16BIT_HANDLE(HModel);
//mxDECLARE_16BIT_HANDLE(HMaterial);



/*
=====================================================================
	SHADER UNIFORMS REFLECTION
=====================================================================
*/

// uniform handle (for use with shader/effects framework)
mxDECLARE_16BIT_HANDLE(HUniform);

/// Describes a uniform parameter in a shader constant buffer.
struct NwUniform: CStruct
{
	NameHash32	name_hash;
	union {
		struct {
			unsigned	offset  : 20;
			unsigned	size    : 12;
		};
		U32	offset_and_size;
	};
	//U8	semantic;	// including types of predefined engine parameters ('auto-parameters')
public:
	mxDECLARE_CLASS(NwUniform, CStruct);
	mxDECLARE_REFLECTION;
};
ASSERT_SIZEOF(NwUniform, 8);

//@todo: sort uniforms by name/hash and use binary search if linear find() is too slow
struct NwUniformBufferLayout: CStruct
{
	String					name;		//!< non-empty in development builds only
	U32						size;		//!< the size of this buffer in bytes
	TBuffer< NwUniform >	fields;		//!< basic reflection for member fields
public:
	mxDECLARE_CLASS(NwUniformBufferLayout, CStruct);
	mxDECLARE_REFLECTION;
};

#if 0
enum EShaderQuality
{
	SQ_Low,
	SQ_Medium,
	SQ_Highest,
};
// from lowest to highest
struct UsageFrequency {
	enum Enum {
		PerFrame,
		PerScene,	// view
		PerObject,	// instance
		PerBatch,	// primitive group
		PerPrimitive,
		PerVertex,
		PerPixel,
	};
};
#endif

/*
=====================================================================
	RENDER CONTEXT
=====================================================================
*/

/// View/Scene pass/layer/stage management
struct NwViewStateFlags
{
	enum Enum
	{
		ClearColor0	= BIT(0),	//!< Should we clear the color target 0?
		ClearColor1	= BIT(1),	//!< Should we clear the color target 1?
		ClearColor2	= BIT(2),	//!< Should we clear the color target 2?
		ClearColor3	= BIT(3),	//!< Should we clear the color target 3?
		ClearColor4	= BIT(4),	//!< Should we clear the color target 4?
		ClearColor5	= BIT(5),	//!< Should we clear the color target 5?
		ClearColor6	= BIT(6),	//!< Should we clear the color target 6?
		ClearColor7	= BIT(7),	//!< Should we clear the color target 7?
		ClearColor	= ClearColor0|ClearColor1|ClearColor2|ClearColor3|ClearColor4|ClearColor5|ClearColor6|ClearColor7,

		ClearDepth	= BIT(8),	//!< Should we clear the depth target?
		ClearStencil= BIT(9),	//!< Should we clear the stencil buffer?
		ClearAll	= ClearColor|ClearDepth|ClearStencil,

		ReadOnlyDepthStencil	= BIT(10),
	};
};

namespace NGpu
{
	struct RenderTargets
	{
		HColorTarget	color_targets[LLGL_MAX_BOUND_TARGETS];//!< render targets to bind
		float			clear_colors[LLGL_MAX_BOUND_TARGETS][4];//!< RGBA colors for clearing each RT
		U8				target_count;	//!< number of color targets to bind
		HDepthTarget	depth_target;	//!< optional depth-stencil surface

	public:
		void clear();	// initializes with invalid values
	};//144

	/// Clears and sets render targets/depth-stencil and the viewport.
	struct ViewState: RenderTargets
	{
//		NwRenderState32		renderState;	//!< default render state

		NwViewport		viewport;	//!< rectangular destination area

		float			depth_clear_value;		//!< the value to clear the depth buffer with (default=0)
		U16				flags;		//!< GfxViewStateFlags
		U8				stencil_clear_value;	//!< the value to clear the stencil buffer with (default=0)

	public:
		void clear();	// initializes with invalid values
		//void Reset();	// initializes with default values
	};

	/// shader input bindings
	struct ShaderInputs
	{
		HBuffer			CBs[LLGL_MAX_BOUND_UNIFORM_BUFFERS];	//!< Constant buffers to bind
		HSamplerState	SSs[LLGL_MAX_BOUND_SHADER_SAMPLERS];	//!< Shader samplers to bind
		HShaderInput	SRs[LLGL_MAX_BOUND_SHADER_TEXTURES];	//!< Shader resources to bind
	public:
		void clear();
	};

	/// global rendering order ([scene pass|stage|view] | [render list]), must be less than LLGL_MAX_VIEWS
	typedef U8 LayerID;



}//namespace NGpu






/*
=====================================================================
	PERFORMANCE COUNTERS
=====================================================================
*/
namespace NGpu
{
	struct GPUCounters: CStruct
	{
		double	GPU_Time_Milliseconds;
		U64 IAVertices;
		U64 IAPrimitives;
		U64 VSInvocations;
		U64 GSInvocations;
		U64 GSPrimitives;
		U64 CInvocations;
		U64 CPrimitives;
		U64 PSInvocations;
		U64 HSInvocations;
		U64 DSInvocations;
		U64 CSInvocations;
	public:
		mxDECLARE_CLASS(GPUCounters, CStruct);
		mxDECLARE_REFLECTION;
	};

	struct BackEndCounters: CStruct
	{
		U32		c_draw_calls;	//!< total number of submitted batches
		U32		c_triangles;	//!< number of rendered triangles
		U32		c_vertices;
		GPUCounters	gpu;
	public:
		mxDECLARE_CLASS(BackEndCounters, CStruct);
		mxDECLARE_REFLECTION;

		void reset()
		{
			mxZERO_OUT(*this);
		}
	};

}//namespace NGpu
