#include <algorithm>	// stable_sort()
#include <Base/Base.h>
#include <Base/Template/Algorithm/Search.h>	// FindIndexByName()
#include <Core/Serialization/Serialization.h>
#include <Graphics/Public/graphics_formats.h>
// for graphics assets (up)loading
#include <GPU/Public/graphics_device.h>
#include <Core/Serialization/Text/TxTReader.h>
#include <Core/Serialization/Text/TxTSerializers.h>


mxDEFINE_CLASS( NwTexture );
mxBEGIN_REFLECTION( NwTexture )
	//mxMEMBER_FIELD( pad ),
mxEND_REFLECTION

NwTexture::NwTexture()
{
	m_texture.SetNil();
	m_resource.SetNil();
}

ERet NwTexture::loadFromMemory(
							   const NGpu::Memory* mem_block
							   , const GrTextureCreationFlagsT flags
							   )
{
	m_texture = NGpu::CreateTexture( mem_block, flags );
	m_resource = NGpu::AsInput( m_texture );
	return ALL_OK;
}

mxDEFINE_CLASS(TbTextureSize);
mxBEGIN_REFLECTION(TbTextureSize)
	mxMEMBER_FIELD(size),
	mxMEMBER_FIELD(relative),
mxEND_REFLECTION;

mxDEFINE_CLASS(TbTextureBase);
mxBEGIN_REFLECTION(TbTextureBase)
	mxMEMBER_FIELD(sizeX),
	mxMEMBER_FIELD(sizeY),
	mxMEMBER_FIELD(format),
	mxMEMBER_FIELD(numMips),
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION;
TbTextureBase::TbTextureBase()
{
	format = NwDataFormat::Unknown;
	numMips = 1;
	flags = NwTextureCreationFlags::defaults;
}

mxDEFINE_CLASS(TbTexture2D);
mxBEGIN_REFLECTION(TbTexture2D)
mxEND_REFLECTION;
TbTexture2D::TbTexture2D()
{
	handle.SetNil();
}

mxDEFINE_CLASS(TbColorTarget);
mxBEGIN_REFLECTION(TbColorTarget)
mxEND_REFLECTION;
TbColorTarget::TbColorTarget()
{
	handle.SetNil();
}

mxDEFINE_CLASS(TbDepthTarget);
mxBEGIN_REFLECTION(TbDepthTarget)
	mxMEMBER_FIELD(sample),
mxEND_REFLECTION;
TbDepthTarget::TbDepthTarget()
{
	handle.SetNil();
	sample = false;
}

mxDEFINE_CLASS(FxDepthStencilState);
mxBEGIN_REFLECTION(FxDepthStencilState)
	//mxMEMBER_FIELD(info),
mxEND_REFLECTION;
FxDepthStencilState::FxDepthStencilState()
{
	handle.SetNil();
}

mxDEFINE_CLASS(FxRasterizerState);
mxBEGIN_REFLECTION(FxRasterizerState)
	//mxMEMBER_FIELD(info),
mxEND_REFLECTION;
FxRasterizerState::FxRasterizerState()
{
	handle.SetNil();
}

mxDEFINE_CLASS(FxSamplerState);
mxBEGIN_REFLECTION(FxSamplerState)
	//mxMEMBER_FIELD(info),
mxEND_REFLECTION;
FxSamplerState::FxSamplerState()
{
	handle.SetNil();
}

mxDEFINE_CLASS(FxBlendState);
mxBEGIN_REFLECTION(FxBlendState)
	//mxMEMBER_FIELD(info),
mxEND_REFLECTION;
FxBlendState::FxBlendState()
{
	handle.SetNil();
}

mxDEFINE_CLASS(NwRenderState);
mxBEGIN_REFLECTION(NwRenderState)
	mxMEMBER_FIELD(blend_state),
	mxMEMBER_FIELD(rasterizer_state),
	mxMEMBER_FIELD(depth_stencil_state),
	mxMEMBER_FIELD(stencil_reference_value),
mxEND_REFLECTION;
NwRenderState::NwRenderState()
{
	blend_state.SetNil();
	rasterizer_state.SetNil();
	depth_stencil_state.SetNil();
	stencil_reference_value = 0;
}

mxDEFINE_CLASS( RawVertexStream );
mxBEGIN_REFLECTION( RawVertexStream )
	mxMEMBER_FIELD( data ),
	mxMEMBER_FIELD( stride ),
mxEND_REFLECTION

mxDEFINE_CLASS( RawVertexData );
mxBEGIN_REFLECTION( RawVertexData )
	mxMEMBER_FIELD( streams ),
	mxMEMBER_FIELD( count ),
	mxMEMBER_FIELD( vertex_format_id ),
mxEND_REFLECTION

mxDEFINE_CLASS( RawIndexData );
mxBEGIN_REFLECTION( RawIndexData )
	mxMEMBER_FIELD( data ),
	mxMEMBER_FIELD( stride ),
mxEND_REFLECTION

mxDEFINE_CLASS( RawMeshPart );
mxBEGIN_REFLECTION( RawMeshPart )
	mxMEMBER_FIELD( base_vertex ),
	mxMEMBER_FIELD( start_index ),
	mxMEMBER_FIELD( index_count ),
	mxMEMBER_FIELD( vertex_count ),
	mxMEMBER_FIELD( material_id ),
mxEND_REFLECTION

mxDEFINE_CLASS(Bone);
mxBEGIN_REFLECTION(Bone)
	mxMEMBER_FIELD(orientation),
	mxMEMBER_FIELD(position),
	mxMEMBER_FIELD(parent),
mxEND_REFLECTION
Bone::Bone()
{
	orientation = Quaternion_Identity();
	position = V3_SetAll(0.0f);
	parent = -1;
}

mxDEFINE_CLASS(Skeleton);
mxBEGIN_REFLECTION(Skeleton)
	mxMEMBER_FIELD(bones),
	mxMEMBER_FIELD(boneNames),
	mxMEMBER_FIELD(invBindPoses),
mxEND_REFLECTION
Skeleton::Skeleton()
{
}

mxDEFINE_CLASS( TbRawMeshData );
mxBEGIN_REFLECTION( TbRawMeshData )
	mxMEMBER_FIELD( vertexData ),
	mxMEMBER_FIELD( indexData ),
	mxMEMBER_FIELD( topology ),
	mxMEMBER_FIELD( skeleton ),
	mxMEMBER_FIELD( bounds ),
	mxMEMBER_FIELD( parts ),
mxEND_REFLECTION
TbRawMeshData::TbRawMeshData()
{
	topology = NwTopology::TriangleList;
	AABBf_Clear(&bounds);
}


const char* MISSING_TEXTURE_ASSET_ID = "missing_texture";
