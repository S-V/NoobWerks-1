// TODO: shader and program instances are so small (16-bit handles) that they can be stored inside the asset table
// 
#include <Base/Base.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Serialization/Serialization.h>
#include <Graphics/Public/graphics_formats.h>
#include <GPU/Public/graphics_device.h>
#include <GPU/Public/graphics_system.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <Graphics/Private/shader_effect_impl.h>


mxDEFINE_CLASS(NwShaderBindings::Binding);
mxBEGIN_REFLECTION(NwShaderBindings::Binding)
	mxMEMBER_FIELD(name_hash),
	mxMEMBER_FIELD(command_offset),
	mxMEMBER_FIELD(sampler_index),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderBindings::DebugData);
mxBEGIN_REFLECTION(NwShaderBindings::DebugData)
	mxMEMBER_FIELD(CB_names),
	mxMEMBER_FIELD(SR_names),
	mxMEMBER_FIELD(SS_names),
	mxMEMBER_FIELD(UAV_names),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderBindings);
mxBEGIN_REFLECTION(NwShaderBindings)
	mxMEMBER_FIELD(_CBs),
	mxMEMBER_FIELD(_SRs),
	mxMEMBER_FIELD(_SSs),
	mxMEMBER_FIELD(_UAVs),

	mxMEMBER_FIELD(_num_global_CBs),
	mxMEMBER_FIELD(_num_global_SRs),
	mxMEMBER_FIELD(_num_builtin_SSs),

	mxMEMBER_FIELD(_dbg),
mxEND_REFLECTION;


NwShaderBindings::NwShaderBindings()
{
	_num_global_CBs = 0;
	_num_global_SRs = 0;
	_num_builtin_SSs = 0;
}

ERet NwShaderBindings::SetInputResourceInCmdBuf(
	const NwNameHash& name_hash
	, const HShaderInput new_handle
	, void* command_buffer
	)
{
	const unsigned numSRs = _SRs.num();
	const Binding* SRs = _SRs.begin();
mxOPTIMIZE("check generated asm in release - name_hash must be int!");
	for( unsigned i = 0; i < numSRs; i++ )
	{
		if( nwNAMEHASH_VAL(name_hash) == SRs[i].name_hash )
		{
			HShaderInput *handle_ptr = SRs[i].GetHandlePtrInCmdBuf< HShaderInput >(command_buffer);
			*handle_ptr = new_handle;
			return ALL_OK;
		}
	}

	DBGOUT("[%d] Shader resource #'%s' = '0x%X' not found",
		(int)mxGetTimeInMilliseconds()
		, nwNAMEHASH_STR(name_hash)
		, nwNAMEHASH_VAL(name_hash)
		);

	return ERR_OBJECT_NOT_FOUND;
}


int NwShaderBindings::findTextureBindingIndex(
	const NameHash32 name_hash
	) const
{
	const unsigned numSRs = _SRs.num();
	const Binding* SRs = _SRs.begin();

	for( unsigned i = 0; i < numSRs; i++ )
	{
		if( name_hash == SRs[i].name_hash )
		{
			return i;
		}
	}

	return -1;
}

ERet NwShaderBindings::SetSamplerInCmdBuf(
	const NameHash32 name_hash
	, HSamplerState handle
	, void * command_buffer
	)
{
	const unsigned numSSs = _SSs.num();
	const NwShaderBindings::Binding* SSs = _SSs.begin();

	for( unsigned i = 0; i < numSSs; i++ )
	{
		if( name_hash == SSs[i].name_hash )
		{
			HSamplerState* handle_ptr = SSs[i].GetHandlePtrInCmdBuf< HSamplerState >(command_buffer);
			*handle_ptr = handle;
			return ALL_OK;
		}
	}

	DBGOUT("Shader sampler '0x%X' not found", name_hash);

	return ERR_OBJECT_NOT_FOUND;
}

ERet NwShaderBindings::SetUavInCmdBuf(
	const NameHash32 name_hash
	, HShaderOutput handle
	, void * command_buffer
	)
{
	const unsigned numOBs = _UAVs.num();
	const NwShaderBindings::Binding* UAVs = _UAVs.begin();

	for( unsigned i = 0; i < numOBs; i++ )
	{
		if( name_hash == UAVs[i].name_hash )
		{
			HShaderOutput* handle_ptr = UAVs[i].GetHandlePtrInCmdBuf< HShaderOutput >(command_buffer);
			*handle_ptr = handle;
			return ALL_OK;
		}
	}

	DBGOUT("Shader UAV '0x%X' not found", name_hash);

	return ERR_OBJECT_NOT_FOUND;
}

void NwShaderBindings::FixUpCBufferHandlesInCmdBuf(
	void *command_buffer
	, const NGraphics::ShaderResourceLUT& global_cbuffers_LUT
	) const
{
	const Binding* cbuffer_bindings = _CBs.begin();

	for(UINT i = 0; i < _num_global_CBs; i++)
	{
		const Binding& cbuffer_binding = cbuffer_bindings[i];

		if( const HBuffer* cbuffer_handle = global_cbuffers_LUT.GetCBuffer(nwMAKE_NAMEHASH2(
			_dbg.CB_names._data[i].c_str(),
			cbuffer_binding.name_hash
			)) )
		{
			HBuffer *dst_buffer_handle_ptr = cbuffer_binding.GetHandlePtrInCmdBuf< HBuffer >(command_buffer);
			*dst_buffer_handle_ptr = *cbuffer_handle;
		}
	}
}

void NwShaderBindings::FixUpSamplerHandlesInCmdBuf(
	void *command_buffer
	, const TSpan<HSamplerState>& local_samplers
	) const
{
	const Binding* sampler_bindings = _SSs.begin();

	for(UINT i = 0; i < _num_builtin_SSs; i++ )
	{
		const Binding& binding = sampler_bindings[ i ];

		HSamplerState *handle_ptr = binding.GetHandlePtrInCmdBuf<HSamplerState>(command_buffer);

		const BuiltIns::ESampler global_sampler_index = (BuiltIns::ESampler) binding.sampler_index;
		mxASSERT(global_sampler_index < mxCOUNT_OF(BuiltIns::g_samplers));
		const HSamplerState global_sampler_handle = BuiltIns::g_samplers[ global_sampler_index ];
		*handle_ptr = global_sampler_handle;
	}

#if 0
	//
	const UINT num_owned_samplers = _SSs.num() - _num_builtin_SSs;
	for(UINT i = 0; i < num_owned_samplers; i++ )
	{
		const Binding& binding = sampler_bindings[ i ];

		HSamplerState *handle_ptr = binding.GetHandlePtrInCmdBuf<HSamplerState>(command_buffer);
		const UINT local_sampler_index = binding.sampler_index;
		const HSamplerState sampler_handle = local_samplers[ local_sampler_index ];
		*handle_ptr = sampler_handle;
	}
#endif
}

void NwShaderBindings::FixUpTextureHandlesInCmdBuf(
	void *command_buffer
	, const TSpan<NwShaderTextureRef>& texture_resources
	) const
{
	const UINT num_texture_bindings = texture_resources.num();
	mxASSERT(texture_resources.num() <= _SRs.num());

	const Binding* texture_bindings = _SRs.begin();

	for( UINT iTexture = 0; iTexture < num_texture_bindings; iTexture++ )
	{
		const NwShaderTextureRef& texture_ref = texture_resources[ iTexture ];

		if( texture_ref.texture.IsValid() )
		{
			const Binding& binding = texture_bindings[ texture_ref.binding_index ];

			HShaderInput *handle_ptr = binding.GetHandlePtrInCmdBuf<HShaderInput>(command_buffer);

			*handle_ptr = texture_ref.texture->m_resource;
		}
		else
		{
			DEVOUT("Texture '%s' could not be loaded!",
				AssetId_ToChars( texture_ref.texture._id )
				);
		}
	}
}

ERet NwShaderBindings::DbgValidateHandlesInCmdBuf(
	const void* command_buffer
	) const
{
	void* non_const_command_buffer = const_cast<void*>(command_buffer);

	for(UINT i = 0; i < _CBs.num(); i++)
	{
		mxENSURE(
			_CBs[i].GetHandlePtrInCmdBuf<HBuffer>(non_const_command_buffer)->IsValid()
			, ERR_NULL_POINTER_PASSED
			, "CB[%d]'%s' is NIL!", i, _dbg.CB_names._data[i].c_str()
			);
	}

	for(UINT i = 0; i < _SRs.num(); i++)
	{
		mxENSURE(
			_SRs[i].GetHandlePtrInCmdBuf<HShaderInput>(non_const_command_buffer)->IsValid()
			, ERR_NULL_POINTER_PASSED
			, "Resource[%d]'%s' is NIL!", i, _dbg.SR_names._data[i].c_str()
			);
	}

	for(UINT i = 0; i < _SSs.num(); i++)
	{
		mxENSURE(
			_SSs[i].GetHandlePtrInCmdBuf<HSamplerState>(non_const_command_buffer)->IsValid()
			, ERR_NULL_POINTER_PASSED
			, "Sampler[%d]'%s' is NIL!", i, _dbg.SS_names._data[i].c_str()
			);
	}

	for(UINT i = 0; i < _UAVs.num(); i++)
	{
		mxENSURE(
			_UAVs[i].GetHandlePtrInCmdBuf<HShaderInput>(non_const_command_buffer)->IsValid()
			, ERR_NULL_POINTER_PASSED
			, "UAV[%d]'%s' is NIL!", i, _dbg.UAV_names._data[i].c_str()
			);
	}

	return ALL_OK;
}

/*
--------------------------------------------------
	NwShaderEffectData
--------------------------------------------------
*/
//mxDEFINE_CLASS(TbPrecompiledCommandBuffer);
//mxBEGIN_REFLECTION(TbPrecompiledCommandBuffer)
//	mxMEMBER_FIELD(data),
//mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderEffectData::ShaderFeature);
mxBEGIN_REFLECTION(NwShaderEffectData::ShaderFeature)
	mxMEMBER_FIELD(name_hash),
	mxMEMBER_FIELD(bit_mask),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwShaderEffectData);
mxBEGIN_REFLECTION(NwShaderEffectData)
	mxMEMBER_FIELD(passes),

	mxMEMBER_FIELD(bindings),

	mxMEMBER_FIELD(command_buffer),

	mxMEMBER_FIELD(uniform_buffer_layout),
	mxMEMBER_FIELD(uniform_buffer_slot),
	mxMEMBER_FIELD(local_uniforms),

	

	//mxMEMBER_FIELD(created_sampler_states),
	mxMEMBER_FIELD(referenced_textures),

	mxMEMBER_FIELD(features),
	mxMEMBER_FIELD(default_feature_mask),

	mxMEMBER_FIELD(programs),
mxEND_REFLECTION;

NwShaderFeatureBitMask NwShaderEffectData::findFeatureMask( NameHash32 name_hash ) const
{
	for( UINT iFeature = 0; iFeature < this->features.num(); iFeature++ )
	{
		const NwShaderEffectData::ShaderFeature& feature = this->features[ iFeature ];
		if( feature.name_hash == name_hash ) {
			return feature.bit_mask;
		}
	}
	return 0;
}

NwShaderFeatureBitMask NwShaderEffectData::composeFeatureMask( NameHash32 name_hash, unsigned value ) const
{
	const NwShaderFeatureBitMask featureMask = this->findFeatureMask( name_hash );
	mxASSERT2(featureMask != 0, "Feature mask '0x%X' not found", name_hash);

	// Search the mask data from least significant bit (LSB) to the most significant bit (MSB) for a set bit (1).
	DWORD	firstSetBitIndex;
	_BitScanForward( &firstSetBitIndex, featureMask );

	return featureMask & (value << firstSetBitIndex);
}

ERet NwShaderEffectData::bitOrWithFeatureMask(
	NwShaderFeatureBitMask &result_
	, NameHash32 name_hash
	, unsigned value
) const
{
	const NwShaderFeatureBitMask featureMask = this->findFeatureMask( name_hash );
	mxENSURE(featureMask != 0, ERR_OBJECT_NOT_FOUND, "Feature mask '0x%X' not found", name_hash);

	// Search the mask data from least significant bit (LSB) to the most significant bit (MSB) for a set bit (1).
	DWORD	firstSetBitIndex;
	_BitScanForward( &firstSetBitIndex, featureMask );

	result_ |= featureMask & (value << firstSetBitIndex);

	return ALL_OK;
}

HUniform NwShaderEffectData::FindUniform(const NameHash32 name_hash) const
{
	const NwUniform* uniform_fields = this->uniform_buffer_layout.fields.raw();
	const U32 uniform_fields_count = this->uniform_buffer_layout.fields.num();

	for( unsigned i = 0; i < uniform_fields_count; i++ )
	{
		const NwUniform& uniform_field = uniform_fields[i];

		if( name_hash == uniform_field.name_hash )
		{
			return HUniform::MakeHandle(i);
		}
	}

	return HUniform::MakeNilHandle();
}

ERet NwShaderEffectData::setUniform(
	const NameHash32 name_hash
	, const void* uniform_data
	, CommandBufferChunk & command_buffer
	)
{
	const NwUniform* uniform_fields = this->uniform_buffer_layout.fields.raw();
	const U32 uniform_fields_count = this->uniform_buffer_layout.fields.num();

	for( unsigned i = 0; i < uniform_fields_count; i++ )
	{
		const NwUniform& uniform_field = uniform_fields[i];

		if( name_hash == uniform_field.name_hash )
		{
			void *uniform = mxAddByteOffset(
				local_uniforms._data,
				uniform_field.offset
			);
			memcpy( uniform, uniform_data, uniform_field.size );
			return ALL_OK;
		}
	}

	//
	mxASSERT2(false,
		"TbUniform '0x%X' not found",
		name_hash
		);

	return ERR_OBJECT_NOT_FOUND;
}

const NwShaderEffect::Pass* NwShaderEffectData::findPass( const NameHash32 name_hash ) const
{
	nwFOR_EACH(const NwShaderEffect::Pass& pass, passes)
	{
		if( pass.name_hash == name_hash ) {
			return &pass;
		}
	}
	return nil;
}

int NwShaderEffectData::findPassIndex( const NameHash32 name_hash ) const
{
	nwFOR_EACH_INDEXED(const NwShaderEffect::Pass& pass, passes, i)
	{
		if( pass.name_hash == name_hash ) {
			return i;
		}
	}
	return INDEX_NONE;
}

/*
-----------------------------------------------------------------------------
	TbShaderEffectLoader
-----------------------------------------------------------------------------
*/
TbShaderEffectLoader::TbShaderEffectLoader( ProxyAllocator & allocator )
	: TbAssetLoader_Null( NwShaderEffect::metaClass(), allocator )
{
}

ERet TbShaderEffectLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	//NwShaderEffect* technique;
	//mxDO(context.object_allocator.allocateObject( &technique ));

	//*new_instance_ = technique;
*new_instance_ = context.object_allocator.new_<NwShaderEffect>();

//new(&technique->id)AssetID();mxTEMP
	return ALL_OK;
}

namespace
{

#if 0
ERet ReadAndCreateSamplerStates(
	NwShaderEffect* technique
	, AssetReader& stream_reader
	, const TbAssetLoadContext& context
	)
{
	NwShaderEffectData* data = technique->data;

	const UINT num_sampler_states_declared_in_effect = data->created_sampler_states.num();

	for( UINT i = 0; i < num_sampler_states_declared_in_effect; i++ )
	{
		NwSamplerDescription		sampler_description;
		mxDO(stream_reader.Get( sampler_description ));

		data->created_sampler_states._data[i] = GL::CreateSamplerState( sampler_description );
	}

	return ALL_OK;
}
#endif

ERet ReadAndInitPassRenderStates(
	NwShaderEffect* technique
	, AssetReader& stream_reader
	, const TbAssetLoadContext& context
	)
{
	NwShaderEffectData* data = technique->data;

	const U32 num_passes = data->passes.num();
	NwShaderEffect::Pass *dst_passes = data->passes._data;

	//
	for( UINT iPass = 0; iPass < num_passes; iPass++ )
	{
		NwShaderEffect::Pass & pass = dst_passes[ iPass ];

		//
		mxDO(AReader_::Align( stream_reader, TbShaderEffectLoader::ALIGN ));

		//
		const bool pass_has_custom_embedded_render_state = ( pass.render_state.u != 0 );
		if( pass_has_custom_embedded_render_state )
		{
			NwRenderStateDesc	custom_render_state_desc;
			mxDO(stream_reader.Get(custom_render_state_desc));

			NwRenderState32_::Create(
				pass.render_state,
				custom_render_state_desc
				);
		}
		else
		{
			TResPtr< NwRenderState >	referenced_render_state;
			mxDO(readAssetID( referenced_render_state._id, stream_reader ));

			mxDO(referenced_render_state.Load( &context.object_allocator ));

			pass.render_state = *referenced_render_state;
		}
	}//For each pass.

	return ALL_OK;
}

ERet ReadAndLoadProgramPermutations(
	NwShaderEffect* technique
	, AssetReader& stream_reader
	, const TbAssetLoadContext& context
	)
{
	mxDO(AReader_::Align( stream_reader, TbShaderEffectLoader::ALIGN ));

	//
	U32	maxProgramPermutationCountInPasses;
	mxDO(stream_reader.Get( maxProgramPermutationCountInPasses ));

	//
	DynamicArray< AssetID >		programPermutationIds( MemoryHeaps::temporary() );
	mxDO(programPermutationIds.reserve( maxProgramPermutationCountInPasses ));

	//
	NwShaderEffect::Pass * passes = technique->data->passes.raw();

	HProgram *allPrograms = technique->data->programs.raw();

	const U32 numPasses = technique->data->passes.num();

	UINT currentProgramOffset = 0;
	for( UINT iPass = 0; iPass < numPasses; iPass++ )
	{
		stream_reader >> programPermutationIds;

		//
		NwShaderEffect::Pass & pass = passes[ iPass ];

		HProgram *	programsInThisPass = &allPrograms[ currentProgramOffset ];

		//
		const U32 numProgramsInThisPass = programPermutationIds.num();
		for( UINT iProgram = 0; iProgram < numProgramsInThisPass; iProgram++ )
		{
			const AssetID& programId = programPermutationIds[ iProgram ];

			NwShaderProgram *	program;
			mxDO(Resources::Load( program, programId, &context.object_allocator ));

			programsInThisPass[ iProgram ] = program->handle;
		}

		pass.default_program_handle = programsInThisPass[ pass.default_program_index ];

		pass.program_handles = programsInThisPass;

		//
		currentProgramOffset += numProgramsInThisPass;
	}

	return ALL_OK;
}

}//namespace


ERet TbShaderEffectLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	IF_DEVELOPER LogStream(),"!\t Loading technique: ",context.key.id;

	//
	NwShaderEffectData*	loaded_data = nil;
	{
		AssetReader	stream;
		nwDO_MSG(Resources::OpenFile( context.key, &stream )
			, "Failed to load '%s'!", AssetId_ToChars(context.key.id)
			);

		mxDO(Serialization::LoadMemoryImage(
			loaded_data
			, stream
			, context.raw_allocator
			));
	}

	//
	NwShaderEffect* technique = static_cast< NwShaderEffect* >( instance );
	technique->data = loaded_data;

	//
	{
		AssetReader	stream;
		mxDO(Resources::OpenFile( context.key, &stream, AssetPackage::STREAM_DATA0 ));

		//
		if(!loaded_data->command_buffer.IsEmpty())
		{
			//loaded_data->command_buffer.DbgPrint();

			//
			loaded_data->bindings.FixUpCBufferHandlesInCmdBuf(
				loaded_data->command_buffer.d.raw()
				, NGraphics::GetGlobalResourcesLUT()
				);

			//
			//mxDO(ReadAndCreateSamplerStates( technique, stream, context ));

			loaded_data->bindings.FixUpSamplerHandlesInCmdBuf(
				loaded_data->command_buffer.d.raw()
				, TSpan<HSamplerState>()// Arrays::GetSpan(data->created_sampler_states)
				);
		}

		//
		mxDO(ReadAndInitPassRenderStates( technique, stream, context ));

		//
		mxDO(ReadAndLoadProgramPermutations( technique, stream, context ));
	}

	return ALL_OK;
}

void TbShaderEffectLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	NwShaderEffect* technique = static_cast< NwShaderEffect* >( instance );

	context.raw_allocator.delete_( technique->data );

	technique->data = nil;
}

void TbShaderEffectLoader::destroy( NwResource * instance, const TbAssetLoadContext& context )
{
	NwShaderEffect* technique = static_cast< NwShaderEffect* >( instance );
	context.object_allocator.delete_( technique );
	UNDONE;
}

ERet TbShaderEffectLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	this->unload( instance, context );

	this->load( instance, context );

	return ALL_OK;
}
