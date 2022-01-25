#include <Base/Base.h>
#pragma hdrstop
#include <GPU/Public/graphics_types.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <Graphics/Private/shader_effect_impl.h>

using namespace Testbed::Graphics;

mxDEFINE_CLASS(NwVertexShader);
mxBEGIN_REFLECTION(NwVertexShader)
	mxMEMBER_FIELD(handle),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwPixelShader);
mxBEGIN_REFLECTION(NwPixelShader)
	mxMEMBER_FIELD(handle),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwGeometryShader);
mxBEGIN_REFLECTION(NwGeometryShader)
	mxMEMBER_FIELD(handle),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwComputeShader);
mxBEGIN_REFLECTION(NwComputeShader)
	mxMEMBER_FIELD(handle),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderProgram);
mxBEGIN_REFLECTION(NwShaderProgram)
	mxMEMBER_FIELD(handle),
mxEND_REFLECTION;

namespace Testbed {
namespace Graphics {

	const TbMetaClass& shaderAssetClassForShaderTypeEnum( const NwShaderType::Enum shaderType )
	{
		switch( shaderType )
		{
		case NwShaderType::Vertex:
			return NwVertexShader::metaClass();

		case NwShaderType::Pixel:
			return NwPixelShader::metaClass();

		case NwShaderType::Compute:
			return NwComputeShader::metaClass();

		case NwShaderType::Geometry:
			return NwGeometryShader::metaClass();

		mxDEFAULT_UNREACHABLE(break);
		}

		return *(TbMetaClass*)nil;
	}

}//namespace Graphics
}//namespace Testbed


mxDEFINE_CLASS(CommandBufferChunk);
mxBEGIN_REFLECTION(CommandBufferChunk)
	mxMEMBER_FIELD(d),
mxEND_REFLECTION;

void CommandBufferChunk::DbgPrint() const
{
	NGpu::CommandBuffer::DbgPrint(
		this->d.raw(),
		this->d.rawSize()
		);
}

mxDEFINE_CLASS(NwShaderTextureRef);
mxBEGIN_REFLECTION(NwShaderTextureRef)
	mxMEMBER_FIELD(texture),
	//mxMEMBER_FIELD(input_slot),
	mxMEMBER_FIELD(binding_index),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderEffect::Pass);
mxBEGIN_REFLECTION(NwShaderEffect::Pass)
	mxMEMBER_FIELD(name_hash),
	mxMEMBER_FIELD(render_state),
	mxMEMBER_FIELD(default_program_handle),
	mxMEMBER_FIELD(default_program_index),
mxEND_REFLECTION;
//tbPRINT_SIZE_OF(NwShaderEffect::Pass);

mxDEFINE_CLASS(NwShaderEffect);
mxBEGIN_REFLECTION(NwShaderEffect)
mxEND_REFLECTION;

NwShaderEffect::NwShaderEffect()
{
}
NwShaderEffect::~NwShaderEffect()
{
}

UINT NwShaderEffect::numPasses() const
{
	return data->passes.num();
}

const TSpan< const NwShaderEffect::Pass > NwShaderEffect::getPasses() const
{
	return Arrays::GetSpan( data->passes );
}

NwShaderFeatureBitMask NwShaderEffect::findFeatureMask( NameHash32 name_hash ) const
{
	return data->findFeatureMask( name_hash );
}

NwShaderFeatureBitMask NwShaderEffect::composeFeatureMask(
	const NameHash32 name_hash,
	const unsigned value ) const
{
	return data->composeFeatureMask( name_hash, value );
}

const NwShaderEffect::Variant NwShaderEffect::getDefaultVariant() const
{
	NwShaderEffect::Pass& pass = data->passes.getFirst();

	NwShaderEffect::Variant	result;
	result.program_handle = pass.default_program_handle;
	result.render_state = pass.render_state;
	return result;
}

ERet NwShaderEffect::setUniform( const NameHash32 name_hash, const void* uniform_data )
{
	return data->setUniform(
		name_hash
		, uniform_data
		, data->command_buffer
		);
}

//ERet NwShaderEffect::setResource( const NameHash32 name_hash, HShaderInput handle )
//{
//	UNDONE;
//	return 
//	//return data->bindings.SetInputResourceInCmdBuf(
//	//	name_hash
//	//	, handle
//	//	, data->command_buffer.d.raw()
//	//	);
//}

ERet NwShaderEffect::setSampler( const NameHash32 name_hash, HSamplerState handle )
{
	return data->bindings.SetSamplerInCmdBuf(
		name_hash
		, handle
		, data->command_buffer.d.raw()
		);
}

ERet NwShaderEffect::setUav( const NameHash32 name_hash, HShaderOutput handle )
{
	return data->bindings.SetUavInCmdBuf(
		name_hash
		, handle
		, data->command_buffer.d.raw()
		);
}

ERet NwShaderEffect::SetInput(
	const NwNameHash& name_hash,
	const HShaderInput handle
	)
{
	return data->bindings.SetInputResourceInCmdBuf(
		name_hash
		, handle
		, data->command_buffer.d.raw()
		);
}

int NwShaderEffect::GetCBufferBindSlot( const NameHash32 name_hash ) const
{
	UNDONE;
	//const TBuffer< NwShaderBindings::BindPoint >& cbuffer_bindings = data->bindings.CBs;
	//for( UINT i = 0; i < cbuffer_bindings.num(); i++) {
	//	const NwShaderBindings::BindPoint& binding = cbuffer_bindings._data[i];
	//	if( binding.name_hash == name_hash ) {
	//		return binding.;
	//	}
	//}

	return -1;
}

ERet NwShaderEffect::AllocatePushConstants(
	void **allocated_push_constants_
	, const U32 push_constants_size
	, NGpu::CommandBuffer &command_buffer_
) const
{
	const NwUniformBufferLayout& uniform_buffer_layout = data->uniform_buffer_layout;

	mxENSURE(
		uniform_buffer_layout.size
		, ERR_INVALID_FUNCTION_CALL
		, "Attempt to update a non-existent uniform buffer!"
		);

	mxENSURE(
		uniform_buffer_layout.size == push_constants_size
		, ERR_INVALID_PARAMETER
		, ""
		);

	return command_buffer_.AllocateSpace(
		allocated_push_constants_
		, push_constants_size
		, LLGL_CBUFFER_ALIGNMENT_MASK
		);
}

ERet NwShaderEffect::BindConstants(
	const void* constants
	, const U32 constants_size
	, NGpu::CommandBuffer &command_buffer_
) const
{
	const NwUniformBufferLayout& uniform_buffer_layout = data->uniform_buffer_layout;
	mxASSERT(constants_size == uniform_buffer_layout.size);

	return NGpu::Commands::BindConstants(
	 	constants
		, constants_size
		, command_buffer_
		, data->uniform_buffer_slot
		, uniform_buffer_layout.name.c_str()
		);
}


ERet NwShaderEffect::pushAllCommandsInto(
	NGpu::CommandBuffer &command_buffer_
	) const
{

#if MX_DEBUG
	data->bindings.DbgValidateHandlesInCmdBuf(
		data->command_buffer.d.raw()
		);
#endif

	return command_buffer_.WriteCopy(
		data->command_buffer.d.raw(),
		data->command_buffer.d.rawSize()
		);
}

ERet NwShaderEffect::pushShaderParameters(
	NGpu::CommandBuffer & command_buffer_
) const
{
	return pushAllCommandsInto(
		command_buffer_
	);
}

ERet NwShaderEffect::BindCBufferData(
	void **allocated_uniform_data_
	, const U32 uniform_data_size
	, NGpu::CommandBuffer &command_buffer_
) const
{
	const NwUniformBufferLayout& uniform_buffer_layout = data->uniform_buffer_layout;

	mxENSURE(
		uniform_buffer_layout.size
		, ERR_INVALID_FUNCTION_CALL
		, "Attempt to update a non-existent uniform buffer!"
		);

	mxENSURE(
		uniform_buffer_layout.size <= uniform_data_size
		, ERR_BUFFER_TOO_SMALL
		, ""
		);

	return NGpu::Commands::BindCBufferData(
	 	(Vector4**) allocated_uniform_data_
		, uniform_data_size
		, command_buffer_
		, data->uniform_buffer_slot
		, HBuffer::MakeNilHandle()
		, uniform_buffer_layout.name.c_str()
		);
}

void NwShaderEffect::debugPrint() const
{

#if MX_DEVELOPER

	if( !data->command_buffer.IsEmpty() )
	{
		data->command_buffer.DbgPrint();
	}

	//
	const NwUniformBufferLayout& ubo = data->uniform_buffer_layout;
	if( ubo.size )
	{
		LogStream(LL_Info)<<"UBO: size=",ubo.size;
	}

#endif // MX_DEVELOPER

}

/*
References:

GPU Gems
Chapter 36. Integrating Shaders into Applications (John O'Rorke, Monolith Productions)
http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch36.html
*/
