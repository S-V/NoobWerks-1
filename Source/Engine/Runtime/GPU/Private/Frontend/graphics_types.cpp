#include <algorithm>

#include <bx/string.h>	// bx::vsnprintf()

#include <Base/Base.h>

#include <GPU/Public/graphics_device.h>
#include <GPU/Private/Frontend/frontend.h>


#if LLGL_CONFIG_DRIVER_D3D11
	#include "backend_d3d11.h"
#endif


//tbPRINT_SIZE_OF(NGpu::Cmd_Draw);


mxDEFINE_CLASS(NamedObject);
mxBEGIN_REFLECTION(NamedObject)
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( hash ),
mxEND_REFLECTION;
NamedObject::NamedObject()
{
	this->hash = 0;
}
void NamedObject::UpdateNameHash()
{
	this->hash = GetDynamicStringHash(this->name.c_str());
}

mxBEGIN_REFLECT_ENUM( NwComparisonFunc8 )
	mxREFLECT_ENUM_ITEM( Always, NwComparisonFunc::Always ),
	mxREFLECT_ENUM_ITEM( Never, NwComparisonFunc::Never ),
	mxREFLECT_ENUM_ITEM( Less, NwComparisonFunc::Less ),
	mxREFLECT_ENUM_ITEM( Equal, NwComparisonFunc::Equal ),
	mxREFLECT_ENUM_ITEM( Greater, NwComparisonFunc::Greater ),
	mxREFLECT_ENUM_ITEM( Not_Equal, NwComparisonFunc::Not_Equal ),
	mxREFLECT_ENUM_ITEM( Less_Equal, NwComparisonFunc::Less_Equal ),
	mxREFLECT_ENUM_ITEM( Greater_Equal, NwComparisonFunc::Greater_Equal ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwFillMode8 )
	mxREFLECT_ENUM_ITEM( Solid, NwFillMode::Solid ),
	mxREFLECT_ENUM_ITEM( Wireframe, NwFillMode::Wireframe ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwCullMode8 )
	mxREFLECT_ENUM_ITEM( None, NwCullMode::None ),
	mxREFLECT_ENUM_ITEM( Back, NwCullMode::Back ),
	mxREFLECT_ENUM_ITEM( Front, NwCullMode::Front ),
mxEND_REFLECT_ENUM

//mxBEGIN_REFLECT_ENUM( EFilteringMode )
//	mxREFLECT_ENUM_ITEM( Filter_Point, FilteringMode::Filter_Point ),
//	mxREFLECT_ENUM_ITEM( Filter_Linear, FilteringMode::Filter_Linear ),
//	mxREFLECT_ENUM_ITEM( Filter_Anisotropic, FilteringMode::Filter_Anisotropic ),
//mxEND_REFLECT_ENUM
mxBEGIN_REFLECT_ENUM( NwTextureFilter8 )
	mxREFLECT_ENUM_ITEM( Min_Mag_Mip_Point, NwTextureFilter::Min_Mag_Mip_Point ),
	mxREFLECT_ENUM_ITEM( Min_Mag_Point_Mip_Linear, NwTextureFilter::Min_Mag_Point_Mip_Linear ),
	mxREFLECT_ENUM_ITEM( Min_Point_Mag_Linear_Mip_Point, NwTextureFilter::Min_Point_Mag_Linear_Mip_Point ),
	mxREFLECT_ENUM_ITEM( Min_Point_Mag_Mip_Linear, NwTextureFilter::Min_Point_Mag_Mip_Linear ),
	mxREFLECT_ENUM_ITEM( Min_Linear_Mag_Mip_Point, NwTextureFilter::Min_Linear_Mag_Mip_Point ),
	mxREFLECT_ENUM_ITEM( Min_Linear_Mag_Point_Mip_Linear, NwTextureFilter::Min_Linear_Mag_Point_Mip_Linear ),
	mxREFLECT_ENUM_ITEM( Min_Mag_Linear_Mip_Point, NwTextureFilter::Min_Mag_Linear_Mip_Point ),
	mxREFLECT_ENUM_ITEM( Min_Mag_Mip_Linear, NwTextureFilter::Min_Mag_Mip_Linear ),
	mxREFLECT_ENUM_ITEM( Anisotropic, NwTextureFilter::Anisotropic ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwTextureAddressMode8 )
	mxREFLECT_ENUM_ITEM( Wrap, NwTextureAddressMode::Wrap ),
	mxREFLECT_ENUM_ITEM( Clamp, NwTextureAddressMode::Clamp ),
	mxREFLECT_ENUM_ITEM( Border, NwTextureAddressMode::Border ),
	mxREFLECT_ENUM_ITEM( Mirror, NwTextureAddressMode::Mirror ),
	mxREFLECT_ENUM_ITEM( MirrorOnce, NwTextureAddressMode::MirrorOnce ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( BlendOp8 )
	mxREFLECT_ENUM_ITEM( MIN, NwBlendOp::MIN ),
	mxREFLECT_ENUM_ITEM( MAX, NwBlendOp::MAX ),
	mxREFLECT_ENUM_ITEM( ADD, NwBlendOp::ADD ),
	mxREFLECT_ENUM_ITEM( SUBTRACT, NwBlendOp::SUBTRACT ),
	mxREFLECT_ENUM_ITEM( REV_SUBTRACT, NwBlendOp::REV_SUBTRACT ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwBlendFactor8 )
	mxREFLECT_ENUM_ITEM( ZERO, NwBlendFactor::ZERO ),
	mxREFLECT_ENUM_ITEM( ONE, NwBlendFactor::ONE ),
	mxREFLECT_ENUM_ITEM( SRC_COLOR, NwBlendFactor::SRC_COLOR ),
	mxREFLECT_ENUM_ITEM( INV_SRC_COLOR, NwBlendFactor::INV_SRC_COLOR ),
	mxREFLECT_ENUM_ITEM( SRC_ALPHA, NwBlendFactor::SRC_ALPHA ),
	mxREFLECT_ENUM_ITEM( INV_SRC_ALPHA, NwBlendFactor::INV_SRC_ALPHA ),
	mxREFLECT_ENUM_ITEM( DST_ALPHA, NwBlendFactor::DST_ALPHA ),
	mxREFLECT_ENUM_ITEM( INV_DST_ALPHA, NwBlendFactor::INV_DST_ALPHA ),
	mxREFLECT_ENUM_ITEM( DST_COLOR, NwBlendFactor::DST_COLOR ),
	mxREFLECT_ENUM_ITEM( INV_DST_COLOR, NwBlendFactor::INV_DST_COLOR ),
	mxREFLECT_ENUM_ITEM( SRC_ALPHA_SAT, NwBlendFactor::SRC_ALPHA_SAT ),
	mxREFLECT_ENUM_ITEM( BLEND_FACTOR, NwBlendFactor::BLEND_FACTOR ),
	mxREFLECT_ENUM_ITEM( INV_BLEND_FACTOR, NwBlendFactor::INV_BLEND_FACTOR ),
	mxREFLECT_ENUM_ITEM( SRC1_COLOR, NwBlendFactor::SRC1_COLOR ),
	mxREFLECT_ENUM_ITEM( INV_SRC1_COLOR, NwBlendFactor::INV_SRC1_COLOR ),
	mxREFLECT_ENUM_ITEM( SRC1_ALPHA, NwBlendFactor::SRC1_ALPHA ),
	mxREFLECT_ENUM_ITEM( INV_SRC1_ALPHA, NwBlendFactor::INV_SRC1_ALPHA ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwStencilOp8 )
	mxREFLECT_ENUM_ITEM( KEEP, NwStencilOp::KEEP ),
	mxREFLECT_ENUM_ITEM( ZERO, NwStencilOp::ZERO ),
	mxREFLECT_ENUM_ITEM( INCR, NwStencilOp::INCR_WRAP ),
	mxREFLECT_ENUM_ITEM( DECR, NwStencilOp::DECR_WRAP ),
	mxREFLECT_ENUM_ITEM( REPLACE, NwStencilOp::REPLACE ),
	mxREFLECT_ENUM_ITEM( INCR_SAT, NwStencilOp::INCR_SAT ),
	mxREFLECT_ENUM_ITEM( DECR_SAT, NwStencilOp::DECR_SAT ),
	mxREFLECT_ENUM_ITEM( INVERT, NwStencilOp::INVERT ),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( NwColorWriteMask8 )
	mxREFLECT_BIT( ENABLE_RED, NwColorWriteMask::Enable_Red ),
	mxREFLECT_BIT( ENABLE_GREEN, NwColorWriteMask::Enable_Green ),
	mxREFLECT_BIT( ENABLE_BLUE, NwColorWriteMask::Enable_Blue ),
	mxREFLECT_BIT( ENABLE_ALPHA, NwColorWriteMask::Enable_Alpha ),

	mxREFLECT_BIT( ENABLE_ALL, NwColorWriteMask::Enable_All ),
mxEND_FLAGS

mxBEGIN_FLAGS( BClearMask )
	mxREFLECT_BIT( CLEAR_DEPTH, ClearMask::Clear_Depth ),
	mxREFLECT_BIT( CLEAR_STENCIL, ClearMask::Clear_Stencil ),
	mxREFLECT_BIT( CLEAR_COLOR, ClearMask::Clear_Color ),
	mxREFLECT_BIT( CLEAR_ALL, ClearMask::Clear_All ),
mxEND_FLAGS

mxDEFINE_CLASS(NwDepthStencilSide);
mxBEGIN_REFLECTION(NwDepthStencilSide)
	mxMEMBER_FIELD(stencil_comparison),
	mxMEMBER_FIELD(stencil_pass_op),
	mxMEMBER_FIELD(stencil_fail_op),
	mxMEMBER_FIELD(depth_fail_op),
mxEND_REFLECTION
NwDepthStencilSide::NwDepthStencilSide()
{
	stencil_comparison = NwComparisonFunc::Always;
	stencil_pass_op = NwStencilOp::KEEP;
	stencil_fail_op = NwStencilOp::KEEP;
	depth_fail_op = NwStencilOp::KEEP;
}

mxBEGIN_FLAGS( NwDepthStencilFlags8 )
	mxREFLECT_BIT( Enable_DepthTest,	NwDepthStencilFlags::Enable_DepthTest ),
	mxREFLECT_BIT( Enable_DepthWrite,	NwDepthStencilFlags::Enable_DepthWrite ),
	mxREFLECT_BIT( Enable_Stencil,		NwDepthStencilFlags::Enable_Stencil ),
mxEND_FLAGS

mxDEFINE_CLASS(NwDepthStencilDescription);
mxBEGIN_REFLECTION(NwDepthStencilDescription)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(comparison_function),
	mxMEMBER_FIELD(stencil_read_mask),
	mxMEMBER_FIELD(stencil_write_mask),
	mxMEMBER_FIELD(front_face),
	mxMEMBER_FIELD(back_face),
mxEND_REFLECTION
//tbPRINT_SIZE_OF(NwDepthStencilDescription);

NwDepthStencilDescription::NwDepthStencilDescription()
{
	flags = NwDepthStencilFlags::Defaults;

#if 0
	comparison_function = NwComparisonFunc::Less_Equal;
#else
	// we're using reverse z-buffering
	comparison_function = NwComparisonFunc::Greater_Equal;
#endif

	stencil_read_mask = ~0;
	stencil_write_mask = ~0;
}

mxBEGIN_FLAGS( NwRasterizerFlags8 )
	mxREFLECT_BIT( Enable_DepthClip,		NwRasterizerFlags::Enable_DepthClip ),
	mxREFLECT_BIT( Enable_Scissor,			NwRasterizerFlags::Enable_Scissor ),
	mxREFLECT_BIT( Enable_Multisample,		NwRasterizerFlags::Enable_Multisample ),
	mxREFLECT_BIT( Enable_AntialiasedLine,	NwRasterizerFlags::Enable_AntialiasedLine ),
mxEND_FLAGS

mxDEFINE_CLASS(NwRasterizerDescription);
mxBEGIN_REFLECTION(NwRasterizerDescription)
	mxMEMBER_FIELD(fill_mode),
	mxMEMBER_FIELD(cull_mode),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION
//tbPRINT_SIZE_OF(NwRasterizerDescription);

NwRasterizerDescription::NwRasterizerDescription()
{
	fill_mode = NwFillMode::Solid;
	cull_mode = NwCullMode::Back;
	flags = NwRasterizerFlags::Defaults;
}


mxDEFINE_CLASS(NwSamplerDescription);
mxBEGIN_REFLECTION(NwSamplerDescription)
	mxMEMBER_FIELD(filter),
	mxMEMBER_FIELD(address_U),
	mxMEMBER_FIELD(address_V),
	mxMEMBER_FIELD(address_W),
	mxMEMBER_FIELD(max_anisotropy),
	mxMEMBER_FIELD(comparison),
	mxMEMBER_FIELD(border_color),
	mxMEMBER_FIELD(min_LOD),
	mxMEMBER_FIELD(max_LOD),
	mxMEMBER_FIELD(mip_LOD_bias),
mxEND_REFLECTION
//tbPRINT_SIZE_OF(NwSamplerDescription);

NwSamplerDescription::NwSamplerDescription()
{
	filter			= NwTextureFilter::Min_Mag_Mip_Point;
	address_U		= NwTextureAddressMode::Clamp;
	address_V		= NwTextureAddressMode::Clamp;
	address_W		= NwTextureAddressMode::Clamp;
	comparison		= NwComparisonFunc::Never;
	border_color[0]	= 0.0f;
	border_color[1]	= 0.0f;
	border_color[2]	= 0.0f;
	border_color[3]	= 0.0f;
	min_LOD			= FLT_MIN;
	max_LOD			= FLT_MAX;
	mip_LOD_bias		= 0.0f;
	max_anisotropy	= 1;
}

mxDEFINE_CLASS(NwBlendChannel);
mxBEGIN_REFLECTION(NwBlendChannel)
	mxMEMBER_FIELD(operation),
	mxMEMBER_FIELD(source_factor),
	mxMEMBER_FIELD(destination_factor),
mxEND_REFLECTION
NwBlendChannel::NwBlendChannel()
{
	operation = NwBlendOp::ADD;
	source_factor = NwBlendFactor::ONE;
	destination_factor = NwBlendFactor::ZERO;
}

mxBEGIN_FLAGS(NwBlendFlags8)
	mxREFLECT_BIT( Enable_Blending,			NwBlendFlags::Enable_Blending ),
	mxREFLECT_BIT( Enable_AlphaToCoverage,	NwBlendFlags::Enable_AlphaToCoverage ),
mxEND_FLAGS

mxDEFINE_CLASS(NwBlendDescription);
mxBEGIN_REFLECTION(NwBlendDescription)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(color_channel),
	mxMEMBER_FIELD(alpha_channel),
	mxMEMBER_FIELD(write_mask),
mxEND_REFLECTION
//tbPRINT_SIZE_OF(NwBlendDescription);

NwBlendDescription::NwBlendDescription()
{
	flags = NwBlendFlags::Defaults;
	write_mask = NwColorWriteMask::Enable_All;
}

mxDEFINE_CLASS(NwRenderState32);
mxBEGIN_REFLECTION(NwRenderState32)
	mxMEMBER_FIELD(u),
mxEND_REFLECTION


mxBEGIN_REFLECT_ENUM( NwTopology8 )
	mxREFLECT_ENUM_ITEM( Undefined, NwTopology::Undefined ),
	mxREFLECT_ENUM_ITEM( PointList, NwTopology::PointList ),
	mxREFLECT_ENUM_ITEM( LineList, NwTopology::LineList ),
	mxREFLECT_ENUM_ITEM( LineStrip, NwTopology::LineStrip ),
	mxREFLECT_ENUM_ITEM( TriangleList, NwTopology::TriangleList ),
	mxREFLECT_ENUM_ITEM( TriangleStrip, NwTopology::TriangleStrip ),
	mxREFLECT_ENUM_ITEM( TriangleFan, NwTopology::TriangleFan ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwAttributeType8 )
	mxREFLECT_ENUM_ITEM( Byte4, NwAttributeType::Byte4 ),
	mxREFLECT_ENUM_ITEM( UByte4, NwAttributeType::UByte4 ),
	mxREFLECT_ENUM_ITEM( Short2, NwAttributeType::Short2 ),
	mxREFLECT_ENUM_ITEM( UShort2, NwAttributeType::UShort2 ),
	
	mxREFLECT_ENUM_ITEM( Half2, NwAttributeType::Half2 ),
	mxREFLECT_ENUM_ITEM( Half4, NwAttributeType::Half4 ),

	mxREFLECT_ENUM_ITEM( Float1, NwAttributeType::Float1 ),
	mxREFLECT_ENUM_ITEM( Float2, NwAttributeType::Float2 ),
	mxREFLECT_ENUM_ITEM( Float3, NwAttributeType::Float3 ),
	mxREFLECT_ENUM_ITEM( Float4, NwAttributeType::Float4 ),

	mxREFLECT_ENUM_ITEM( UInt1, NwAttributeType::UInt1 ),
	mxREFLECT_ENUM_ITEM( UInt2, NwAttributeType::UInt2 ),
	mxREFLECT_ENUM_ITEM( UInt3, NwAttributeType::UInt3 ),
	mxREFLECT_ENUM_ITEM( UInt4, NwAttributeType::UInt4 ),

	mxREFLECT_ENUM_ITEM( R11G11B10F, NwAttributeType::R11G11B10F ),
	mxREFLECT_ENUM_ITEM( R10G10B10A2_UNORM, NwAttributeType::R10G10B10A2_UNORM ),
	
	//mxREFLECT_ENUM_ITEM( Double, AttributeType::Double ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( NwAttributeUsage8 )
	mxREFLECT_ENUM_ITEM( Position, NwAttributeUsage::Position ),

	mxREFLECT_ENUM_ITEM( Color, NwAttributeUsage::Color ),

	mxREFLECT_ENUM_ITEM( Normal, NwAttributeUsage::Normal ),
	mxREFLECT_ENUM_ITEM( Tangent, NwAttributeUsage::Tangent ),
	mxREFLECT_ENUM_ITEM( Bitangent, NwAttributeUsage::Bitangent ),

	mxREFLECT_ENUM_ITEM( TexCoord, NwAttributeUsage::TexCoord ),

	mxREFLECT_ENUM_ITEM( BoneWeights, NwAttributeUsage::BoneWeights ),
	mxREFLECT_ENUM_ITEM( BoneIndices, NwAttributeUsage::BoneIndices ),	
mxEND_REFLECT_ENUM

NwVertexDescription& NwVertexDescription::begin()
{
	mxZERO_OUT(attribsArray);
	mxZERO_OUT(attribOffsets);
	mxZERO_OUT(streamStrides);
	attribCount = 0;

	return *this;
}

void NwVertexDescription::end()
{
	mxASSERT(attribCount > 0);
}

// gs_attributeSize [ ATTRIB_TYPE ] [ VECTOR_DIMENSION ]
#if (LLGL_Driver == LLGL_Driver_Direct3D_11)
	static const U8 gs_attributeSize[NwAttributeType::Count] = {
		4,	// Byte4
		4,	// UByte4
		4,	// Short2
		4,	// UShort2
		
		4,	// Half2
		8,	// Half4

		4,	// Float1
		8,	// Float2
		12,	// Float3
		16,	// Float4

		4,	// UInt1
		8,	// UInt2
		12,	// UInt4
		16,	// UInt4

		4,	// R11G11B10F
		4,	// R10G10B10A2_UNORM

		//{  8, 16, 24, 32 },	// Double
	};
#elif (LLGL_Driver_Is_OpenGL)
#	error Unimpl
	static const U8 gs_attributeSize[NwAttributeType::Count][4] = {
		{  1,  2,  4,  4 },	// Byte
		{  1,  2,  4,  4 },	// UByte
		{  2,  4,  6,  8 },	// Short
		{  2,  4,  6,  8 },	// UShort
		{  2,  4,  6,  8 },	// Half
		{  4,  8, 12, 16 },	// Float
		//{  8, 16, 24, 32 },	// Double
	};
#else
	#error Unsupported driver type!
#endif

U8 NwAttributeType::GetDimension( Enum _type )
{
	static const U8 gs_attributeDimension[NwAttributeType::Count] =
	{
		4,	// Byte4
		4,	// UByte4
		2,	// Short2
		2,	// UShort2

		2,	// Half2
		4,	// Half4

		1,	// Float1
		2,	// Float2
		3,	// Float3
		4,	// Float4

		1,	// UInt1
		2,	// UInt2
		3,	// UInt4
		4,	// UInt4

		3,	// R11G11B10F
		4,	// R10G10B10A2_UNORM
	};

	return gs_attributeDimension[_type];
}

NwVertexDescription& NwVertexDescription::add(
	NwAttributeType8 type,
	NwAttributeUsage8 usage,
	UINT usageIndex,
	bool normalized /*= false*/,
	UINT inputSlot /*= 0*/
)
{
	const UINT elementIndex = attribCount++;
	NwVertexElement & newElement = attribsArray[ elementIndex ];
	{
		newElement.dataType = type;
		newElement.usageType = usage;
		newElement.usageIndex = usageIndex;
		newElement.inputSlot = inputSlot;
		newElement.normalized = normalized;
	}
	const UINT elementSize = gs_attributeSize[ type ];
	attribOffsets[ elementIndex ] = streamStrides[ inputSlot ];
	streamStrides[ inputSlot ] += elementSize;

	return *this;
}

bool NwVertexDescription::IsValid() const
{
	return attribCount > 0;
}

bool NwVertexDescription::ContainsAttribWithUsage( NwAttributeUsage::Enum usage ) const
{
	for( int i = 0; i < mxCOUNT_OF(attribsArray); i++ ) {
		if( attribsArray[i].usageType == usage ) {
			return true;
		}
	}
	return false;
}


mxBEGIN_REFLECT_ENUM( DataFormatT )
#define DECLARE_DATA_FORMAT( ENUM, BPP, BW, BH, BS, BX, BY, DB, SB )	mxREFLECT_ENUM_ITEM( ENUM, NwDataFormat::ENUM ),
	#include <GPU/Public/graphics_data_formats.inl>
#undef DECLARE_DATA_FORMAT
mxEND_REFLECT_ENUM

const DataFormat_t g_DataFormats[NwDataFormat::MAX] = {
#define DECLARE_DATA_FORMAT( name, bitsPerPixel, blockWidth, blockHeight, blockSize, minBlockX, minBlockY, depthBits, stencilBits )\
								{ bitsPerPixel, blockWidth, blockHeight, blockSize, minBlockX, minBlockY, depthBits, stencilBits },
	#include <GPU/Public/graphics_data_formats.inl>
#undef DECLARE_DATA_FORMAT
};


bool NwDataFormat::IsCompressed( NwDataFormat::Enum _format )
{
	return g_DataFormats[_format].blockWidth > 1 && g_DataFormats[_format].blockHeight > 1;
}
UINT NwDataFormat::BitsPerPixel( NwDataFormat::Enum _format )
{
	return g_DataFormats[_format].bitsPerPixel;
}
UINT NwDataFormat::GetBlockSize( Enum _format )
{
	return g_DataFormats[_format].blockSize;;
}


mxBEGIN_FLAGS( GrTextureCreationFlagsT )
	mxREFLECT_BIT( DYNAMIC, NwTextureCreationFlags::DYNAMIC ),
	mxREFLECT_BIT( RANDOM_WRITES, NwTextureCreationFlags::RANDOM_WRITES ),
	mxREFLECT_BIT( GENERATE_MIPS, NwTextureCreationFlags::GENERATE_MIPS ),
mxEND_FLAGS

mxDEFINE_CLASS(NwTexture2DDescription);
mxBEGIN_REFLECTION(NwTexture2DDescription)
	mxMEMBER_FIELD(format),
	mxMEMBER_FIELD(width),
	mxMEMBER_FIELD(height),
	mxMEMBER_FIELD(numMips),
	mxMEMBER_FIELD(arraySize),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION
NwTexture2DDescription::NwTexture2DDescription()
{
	format = NwDataFormat::Unknown;
	width = 0;
	height = 0;
	numMips = 0;
	arraySize = 1;
	flags = NwTextureCreationFlags::defaults;
}
U32 NwTexture2DDescription::CalcRawSize() const
{
	return estimateTextureSize(
		format
		, width
		, height
		, 1//depth
		, numMips
		);
}

mxDEFINE_CLASS(NwTexture3DDescription);
mxBEGIN_REFLECTION(NwTexture3DDescription)
	mxMEMBER_FIELD(width),
	mxMEMBER_FIELD(height),
	mxMEMBER_FIELD(depth),
	mxMEMBER_FIELD(format),
	mxMEMBER_FIELD(numMips),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION
NwTexture3DDescription::NwTexture3DDescription()
{
	width = 0;
	height = 0;
	depth = 0;
	format = NwDataFormat::Unknown;
	numMips = 0;
	flags = NwTextureCreationFlags::defaults;
}
U32 NwTexture3DDescription::CalcRawSize() const
{
	return estimateTextureSize(
		format
		, width
		, height
		, depth
		, numMips
		);
}

mxDEFINE_CLASS(NwColorTargetDescription);
mxBEGIN_REFLECTION(NwColorTargetDescription)
	mxMEMBER_FIELD(format),
	mxMEMBER_FIELD(width),
	mxMEMBER_FIELD(height),
	mxMEMBER_FIELD(numMips),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION
NwColorTargetDescription::NwColorTargetDescription()
{
	format = NwDataFormat::Unknown;
	width = 0;
	height = 0;
	numMips = 1;
	flags = NwTextureCreationFlags::defaults;
}

bool NwColorTargetDescription::IsValid() const
{
	return format != NwDataFormat::Unknown
		&& width > 0
		&& height > 0
		&& numMips > 0
		;
}

mxDEFINE_CLASS(NwDepthTargetDescription);
mxBEGIN_REFLECTION(NwDepthTargetDescription)
	mxMEMBER_FIELD(format),
	mxMEMBER_FIELD(width),
	mxMEMBER_FIELD(height),
	mxMEMBER_FIELD(sample),
mxEND_REFLECTION
NwDepthTargetDescription::NwDepthTargetDescription()
{
	format = NwDataFormat::Unknown;
	width = 0;
	height = 0;
	sample = false;
}

U32 estimateTextureSize( NwDataFormat::Enum format
						, U16 width
						, U16 height
						, U16 depth
						, U8 numMips
						, U16 arraySize
						)
{
	mxASSERT( width > 0 );
	mxASSERT( height > 0 );
	mxASSERT( depth > 0 );
	mxASSERT( numMips > 0 );
	mxASSERT( arraySize > 0 );

	U32	estimatedSize = 0;

	const UINT bpp = NwDataFormat::BitsPerPixel( format );

	UINT currWidth = width;
	UINT currHeight = height;
	UINT currDepth = depth;

	for( U8 iCurrMipLevel = 0; iCurrMipLevel < numMips; iCurrMipLevel++ )
	{
		currWidth = largest( ( currWidth >> iCurrMipLevel ), 1 );
		currHeight = largest( ( currHeight >> iCurrMipLevel ), 1 );
		currDepth =  largest( currDepth >> iCurrMipLevel, 1 );

		const UINT currMipLevelSize = BITS_TO_BYTES( currWidth * currHeight * currDepth * bpp );
		estimatedSize += currMipLevelSize;
	}

	return estimatedSize * arraySize;
}

mxBEGIN_REFLECT_ENUM( NwShaderType8 )
	mxREFLECT_ENUM_ITEM( Pixel, NwShaderType::Pixel ),
	mxREFLECT_ENUM_ITEM( Vertex, NwShaderType::Vertex ),
	mxREFLECT_ENUM_ITEM( Compute, NwShaderType::Compute ),
	mxREFLECT_ENUM_ITEM( Geometry, NwShaderType::Geometry ),
	mxREFLECT_ENUM_ITEM( Hull, NwShaderType::Hull ),
	mxREFLECT_ENUM_ITEM( Domain, NwShaderType::Domain ),	
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(CBufferBindingOGL);
mxBEGIN_REFLECTION(CBufferBindingOGL)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(slot),
	mxMEMBER_FIELD(size),
mxEND_REFLECTION;
mxDEFINE_CLASS(SamplerBindingOGL);
mxBEGIN_REFLECTION(SamplerBindingOGL)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(slot),
mxEND_REFLECTION;
mxDEFINE_CLASS(ProgramBindingsOGL);
mxBEGIN_REFLECTION(ProgramBindingsOGL)
	mxMEMBER_FIELD(cbuffers),
	mxMEMBER_FIELD(samplers),
mxEND_REFLECTION;

void ProgramBindingsOGL::clear()
{
	cbuffers.RemoveAll();
	samplers.RemoveAll();
	activeVertexAttributes = 0;
}

mxDEFINE_CLASS(NwProgramDescription);
mxBEGIN_REFLECTION(NwProgramDescription)
	mxMEMBER_FIELD(shaders),
mxEND_REFLECTION
NwProgramDescription::NwProgramDescription()
{
	HShader nilShaderHandle( LLGL_NIL_HANDLE );
	TSetStaticArray( shaders, nilShaderHandle );
}

mxBEGIN_REFLECT_ENUM( BufferTypeT )
	mxREFLECT_ENUM_ITEM( Uniform, EBufferType::Buffer_Uniform ),
	mxREFLECT_ENUM_ITEM( Vertex, EBufferType::Buffer_Vertex ),
	mxREFLECT_ENUM_ITEM( Index, EBufferType::Buffer_Index ),
	mxREFLECT_ENUM_ITEM( Data, EBufferType::Buffer_Data ),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( NwBufferFlagsT )
	mxREFLECT_BIT( Sample, NwBufferFlags::Sample ),
	mxREFLECT_BIT( Write, NwBufferFlags::Write ),
	mxREFLECT_BIT( Append, NwBufferFlags::Append ),
	mxREFLECT_BIT( Dynamic, NwBufferFlags::Dynamic ),
	mxREFLECT_BIT( Staging, NwBufferFlags::Staging ),
mxEND_FLAGS

mxDEFINE_CLASS(NwBufferDescription);
mxBEGIN_REFLECTION(NwBufferDescription)
	mxMEMBER_FIELD(type),
	mxMEMBER_FIELD(size),
	mxMEMBER_FIELD(stride),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION
NwBufferDescription::NwBufferDescription(EInitDefault)
{
	type = Buffer_Uniform;
	size = 0;
	stride = 0;
	flags = NwBufferFlags::Default;
}

ERet AddBindings(
	ProgramBindingsOGL &destination,
	const ProgramBindingsOGL &source
)
{
	for( UINT bufferIndex = 0; bufferIndex < source.cbuffers.num(); bufferIndex++ )
	{
		const CBufferBindingOGL& binding = source.cbuffers[ bufferIndex ];
		const CBufferBindingOGL* existing = FindByName( destination.cbuffers, binding.name.raw() );
		if( !existing )
		{
			destination.cbuffers.add( binding );
		}
	}
	for( UINT samplerIndex = 0; samplerIndex < source.samplers.num(); samplerIndex++ )
	{
		const SamplerBindingOGL& binding = source.samplers[ samplerIndex ];
		const SamplerBindingOGL* existing = FindByName( destination.samplers, binding.name.raw() );
		if( !existing )
		{
			destination.samplers.add( binding );
		}
	}
	destination.activeVertexAttributes |= source.activeVertexAttributes;
	return ALL_OK;
}

ERet AssignBindPoints(
	ProgramBindingsOGL & bindings
)
{
	for( UINT bufferIndex = 0; bufferIndex < bindings.cbuffers.num(); bufferIndex++ )
	{
		CBufferBindingOGL & binding = bindings.cbuffers[ bufferIndex ];
		binding.slot = bufferIndex;
	}
	for( UINT samplerIndex = 0; samplerIndex < bindings.samplers.num(); samplerIndex++ )
	{
		SamplerBindingOGL & binding = bindings.samplers[ samplerIndex ];
		binding.slot = samplerIndex;
	}
	return ALL_OK;
}

ERet MergeBindings(
	ProgramBindingsOGL &destination,
	const ProgramBindingsOGL &source
)
{
	for( UINT bufferIndex = 0; bufferIndex < source.cbuffers.num(); bufferIndex++ )
	{
		const CBufferBindingOGL& binding = source.cbuffers[ bufferIndex ];
		const CBufferBindingOGL* existing = FindByName( destination.cbuffers, binding.name.raw() );
		if( existing )
		{
			chkRET_X_IF_NOT( existing->slot == binding.slot, ERR_LINKING_FAILED );
		}
		else
		{
			destination.cbuffers.add( binding );
		}
	}
	for( UINT samplerIndex = 0; samplerIndex < source.samplers.num(); samplerIndex++ )
	{
		const SamplerBindingOGL& binding = source.samplers[ samplerIndex ];
		const SamplerBindingOGL* existing = FindByName( destination.samplers, binding.name.raw() );
		if( existing )
		{
			chkRET_X_IF_NOT( existing->slot == binding.slot, ERR_LINKING_FAILED );
		}
		else
		{
			destination.samplers.add( binding );
		}
	}
	destination.activeVertexAttributes |= source.activeVertexAttributes;
	return ALL_OK;
}

/*
	This code is based on information obtained from the following site:
	http://www.pcidatabase.com/
*/
DeviceVendor::Enum DeviceVendor::FourCCToVendorEnum( U32 fourCC )
{
	switch ( fourCC ) {
		case 0x3D3D	:		return DeviceVendor::Vendor_3DLABS;
		case 0x1002 :		return DeviceVendor::Vendor_ATI;
		case 0x8086 :		return DeviceVendor::Vendor_Intel;
		case 0x102B :		return DeviceVendor::Vendor_Matrox;
		case 0x10DE	:		return DeviceVendor::Vendor_NVidia;
		case 0x5333	:		return DeviceVendor::Vendor_S3;
		case 0x1039	:		return DeviceVendor::Vendor_SIS;
		default :			return DeviceVendor::Vendor_Unknown;
	}
}

/*
	This code is based on information obtained from the following site:
	http://www.pcidatabase.com/
*/
const char* DeviceVendor::GetVendorString( DeviceVendor::Enum vendorId )
{
	switch ( vendorId ) {
		case DeviceVendor::Vendor_3DLABS :
			return "3Dlabs, Inc. Ltd";

		case DeviceVendor::Vendor_ATI :
			return "ATI Technologies Inc. / Advanced Micro Devices, Inc.";

		case DeviceVendor::Vendor_Intel :
			return "Intel Corporation";

		case DeviceVendor::Vendor_Matrox :
			return "Matrox Electronic Globals Ltd.";

		case DeviceVendor::Vendor_NVidia :
			return "NVIDIA Corporation";

		case DeviceVendor::Vendor_S3 :
			return "S3 Graphics Co., Ltd";

		case DeviceVendor::Vendor_SIS :
			return "SIS";

		case DeviceVendor::Vendor_Unknown :
		default:
			;
	}
	return "Unknown vendor";
}

const char* EImageFileFormat::GetFileExtension( EImageFileFormat::Enum e )
{
	switch( e ) {
	case EImageFileFormat::IFF_BMP :	return ".bmp";
	case EImageFileFormat::IFF_JPG :	return ".jpg";
	case EImageFileFormat::IFF_PNG :	return ".png";
	case EImageFileFormat::IFF_DDS :	return ".dds";
	case EImageFileFormat::IFF_TIFF :	return ".tiff";
	case EImageFileFormat::IFF_GIF :	return ".gif";
	case EImageFileFormat::IFF_WMP :	return ".wmp";
	mxNO_SWITCH_DEFAULT;
	}
	return "";
}

namespace BuiltIns
{

HSamplerState g_samplers[Sampler_MAX] = { HSamplerState::MakeNilHandle() };

}//namespace BuiltIns

mxBEGIN_REFLECT_ENUM( SamplerID )
	mxREFLECT_ENUM_ITEM( $default, BuiltIns::ESampler::DefaultSampler ),

	mxREFLECT_ENUM_ITEM( PointSampler, BuiltIns::ESampler::PointSampler ),
	mxREFLECT_ENUM_ITEM( BilinearSampler, BuiltIns::ESampler::BilinearSampler ),
	mxREFLECT_ENUM_ITEM( TrilinearSampler, BuiltIns::ESampler::TrilinearSampler ),
	mxREFLECT_ENUM_ITEM( AnisotropicSampler, BuiltIns::ESampler::AnisotropicSampler ),

	mxREFLECT_ENUM_ITEM( PointWrapSampler, BuiltIns::ESampler::PointWrapSampler ),
	mxREFLECT_ENUM_ITEM( BilinearWrapSampler, BuiltIns::ESampler::BilinearWrapSampler ),
	mxREFLECT_ENUM_ITEM( TrilinearWrapSampler, BuiltIns::ESampler::TrilinearWrapSampler ),

	mxREFLECT_ENUM_ITEM( DiffuseMapSampler, BuiltIns::ESampler::DiffuseMapSampler ),
	mxREFLECT_ENUM_ITEM( DetailMapSampler, BuiltIns::ESampler::DetailMapSampler ),
	mxREFLECT_ENUM_ITEM( NormalMapSampler, BuiltIns::ESampler::NormalMapSampler ),
	mxREFLECT_ENUM_ITEM( SpecularMapSampler, BuiltIns::ESampler::SpecularMapSampler ),
	mxREFLECT_ENUM_ITEM( RefractionMapSampler, BuiltIns::ESampler::RefractionMapSampler ),

	mxREFLECT_ENUM_ITEM( ShadowMapSampler, BuiltIns::ESampler::ShadowMapSampler ),
	mxREFLECT_ENUM_ITEM( ShadowMapSamplerPCF, BuiltIns::ESampler::ShadowMapSamplerPCF ),
mxEND_REFLECT_ENUM

namespace NGpu
{

#if MX_DEBUG || MX_DEVELOPER

	ATextStream & operator << ( ATextStream & log, const Memory& o )
	{
		if( o.is_relative )
		{
			log.PrintF( "size: %d, is_relative: true, offset: %d (ptr: 0x%X)",
				o.size, o.offset._offset, o.getDataPtr()
				);
		}
		else
		{
			log.PrintF( "size: %d, is_relative: true, ptr: 0x%X",
				o.size, o.data
				);
		}

		return log;
	}

	ATextStream & operator << ( ATextStream & log, const Cmd_Draw& o )
	{
#if MX_DEBUG
		if( o.src_loc )
		{
			log.PrintF( "%s: program = %d, base_vtx: %u, num_verts: %u, start_idx: %u, idx_count: %u"
				, o.src_loc

				, o.program.id

				, o.base_vertex
				, o.vertex_count

				, o.start_index
				, o.index_count
				);
		}
		else
#endif // MX_DEBUG
		{
			log.PrintF( "program = %d, vertex_count: %d, index_count: %d",
				o.program.id, o.vertex_count, o.index_count
				);
		}

		return log;
	}

#endif


}//namespace NGpu

mxDEFINE_CLASS(NwUniform);
mxBEGIN_REFLECTION(NwUniform)
	mxMEMBER_FIELD(name_hash),
	mxMEMBER_FIELD(offset_and_size),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwUniformBufferLayout);
mxBEGIN_REFLECTION(NwUniformBufferLayout)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(size),
	mxMEMBER_FIELD(fields),
mxEND_REFLECTION;



namespace NGpu
{


}



namespace NGpu
{

namespace Dbg
{

GpuScope::GpuScope( CommandBuffer & command_buffer_, const char* _label, U32 _rgba )
	: _command_buffer( command_buffer_ )
{
	Cmd_PushMarker	cmd_pushMarker;
	cmd_pushMarker.encode( CMD_PUSH_MARKER, 0, 0 );
	cmd_pushMarker.text = _label;
	cmd_pushMarker.rgba = _rgba;
	cmd_pushMarker.skip = 0;	// the message string is statically allocated

	_command_buffer.put( cmd_pushMarker );
}

GpuScope::GpuScope( CommandBuffer & command_buffer_, U32 _rgba, const char* formatString, ... )
	: _command_buffer( command_buffer_ )
{
	char temp[1024];

	va_list	argPtr;
	va_start( argPtr, formatString );
	const int len = bx::vsnprintf( temp, mxCOUNT_OF(temp), formatString, argPtr );
	va_end( argPtr );

	const U32 stringLength = tbALIGN4( len + 1 );	// NUL
	const U32 bytesToAllocate = sizeof(Cmd_PushMarker) + stringLength;

	// Allocate space in the command buffer.
	Cmd_PushMarker *	cmd;
	if( mxSUCCEDED(_command_buffer.AllocateSpace(
		(void**)&cmd,
		bytesToAllocate,
		CommandBuffer::alignmentForType<Cmd_PushMarker>() - 1 )) )
	{
		char *	destString = (char*) ( cmd + 1 );	// the text string is placed right after the command structure

		// Initialize the command structure.
		cmd->encode( CMD_PUSH_MARKER, 0, 0 );
		cmd->text = destString;
		cmd->rgba = _rgba;
		cmd->skip = stringLength;

		// Copy the source string into the command buffer.
		memcpy( destString, temp, len );
		destString[ len ] = '\0';
	}
}

GpuScope::~GpuScope()
{
	Cmd_PopMarker	cmd_popMarker;
	cmd_popMarker.encode( CMD_POP_MARKER, 0, 0 );

	_command_buffer.put( cmd_popMarker );
}

}//namespace Dbg
}//namespace NGpu


namespace NGpu
{

	SortKey buildSortKey( const LayerID view_id, const SortKey sort_key )
	{
		return (SortKey(view_id) << LLGL_VIEW_SHIFT) | (sort_key & CLEAR_NON_VIEW_BITS_MASK);
	}

	SortKey buildSortKey( const LayerID view_id, HProgram program, U32 less_important_data )
	{
		return (SortKey(view_id) << LLGL_VIEW_SHIFT) | (SortKey(program.id) << 32) | less_important_data;
	}

}//namespace NGpu
