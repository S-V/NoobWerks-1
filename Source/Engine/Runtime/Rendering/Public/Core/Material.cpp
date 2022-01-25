/*
=============================================================================
	Graphics material.
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Serialization_Private.h>
#include <Core/CoreDebugUtil.h>

#include <Graphics/Private/shader_effect_impl.h>
#include <GPU/Public/graphics_system.h>

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Core/Material.h>


namespace Rendering
{
namespace
{
	AllocatorI & materialDataHeap() { return MemoryHeaps::renderer(); }
}


mxDEFINE_CLASS(MaterialPass);
mxBEGIN_REFLECTION(MaterialPass)
	mxMEMBER_FIELD(render_state),
	mxMEMBER_FIELD(filter_index),
	mxMEMBER_FIELD(draw_order),
	mxMEMBER_FIELD(program),
mxEND_REFLECTION;

/*
-----------------------------------------------------------------------------
	Material
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( Material );
mxBEGIN_REFLECTION( Material )
mxEND_REFLECTION

mxDEFINE_CLASS(Material::VariableLengthData);
mxBEGIN_REFLECTION(Material::VariableLengthData)
	mxMEMBER_FIELD(passes),
	mxMEMBER_FIELD(command_buffer),
	mxMEMBER_FIELD(uniforms),
	mxMEMBER_FIELD(referenced_textures),
mxEND_REFLECTION;

Material::Material()
{
}

Material::~Material()
{

}

ERet Material::setUniform(
	const NameHash32 name_hash
	, const void* uniform_data
	)
{
	mxTRY(effect->data->setUniform(
		name_hash
		, uniform_data
		, data->command_buffer
		));

	return ALL_OK;
}

ERet Material::SetInput(
						const NwNameHash& name_hash
						, HShaderInput handle
						)
{
	return effect->data->bindings.SetInputResourceInCmdBuf(
		name_hash
		, handle
		, command_buffer.d.raw()
		);
}

AssetID Material::getDefaultAssetId()
{
	return MakeAssetID("material_default");
}

Material* Material::getFallbackMaterial()
{
	Material* fallback_material = nil;
	Resources::Load( fallback_material, getDefaultAssetId() );
	return fallback_material;
}

/*
----------------------------------------------------------
	MaterialLoader
----------------------------------------------------------
*/
MaterialLoader::MaterialLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( Material::metaClass(), parent_allocator )
{
}

ERet MaterialLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	*new_instance_ = context.object_allocator.new_< Material >();
	return ALL_OK;
}

ERet MaterialLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	AssetReader	stream_reader;
	mxTRY(Resources::OpenFile( context.key, &stream_reader ));

	//
	MaterialHeader_d	header;
	mxDO(stream_reader.Get(header));

	mxENSURE(
		header.fourCC == SIGNATURE,
		ERR_FAILED_TO_PARSE_DATA,
		""
		);

	//
	AssetID	effect_id;
	mxDO(readAssetID( effect_id, stream_reader ));

	//
	NwShaderEffect *	effect;
	mxDO(Resources::Load( effect, effect_id, &context.object_allocator ));

	//
	Material::VariableLengthData *	material_data;
	mxDO(Reader_Align_Forward(
		stream_reader
		, Serialization::NwImageSaveFileHeader::ALIGNMENT
		));
	mxDO(Serialization::LoadMemoryImage(
		material_data
		, stream_reader
		, context.raw_allocator
		));

	//
	for( UINT iSR = 0; iSR < material_data->referenced_textures.num(); iSR++ )
	{
		mxDO(material_data->referenced_textures._data[ iSR ].texture.Load());
	}


	//
	CommandBufferChunk &material_command_buffer = material_data->command_buffer;

	const bool material_has_own_command_buffer = !material_command_buffer.IsEmpty();

	//
	if(material_has_own_command_buffer)
	{
		const NwShaderBindings& shader_bindings = effect->data->bindings;

		shader_bindings.FixUpCBufferHandlesInCmdBuf(
			material_command_buffer.d.raw()
			, NGraphics::GetGlobalResourcesLUT()
			);

		shader_bindings.FixUpSamplerHandlesInCmdBuf(
			material_command_buffer.d.raw()
			, TSpan<HSamplerState>()// effect->data->created_sampler_states.raw()
			);

		shader_bindings.FixUpTextureHandlesInCmdBuf(
			material_command_buffer.d.raw(),
			Arrays::GetSpan(material_data->referenced_textures)
			);
	}
	else
	{
		material_command_buffer.InitializeWithReferenceTo(
			effect->data->command_buffer
			);
	}

//GL::CommandBuffer::DbgPrint(
//	material_data->command_buffer.raw(),
//	material_data->command_buffer_size
//	);

	//
	const RenderPath& render_path = Globals::GetRenderPath();

	//
	const TSpan< const NwShaderEffect::Pass >	effect_passes = effect->getPasses();

	// Fix up material passes.
	const TSpan< MaterialPass > material_passes = Arrays::GetSpan( material_data->passes );
	mxASSERT(!material_passes.IsEmpty());

	for(UINT material_pass_index = 0;
		material_pass_index < material_passes._count;
		material_pass_index++)
	{
		MaterialPass &material_pass = material_passes._data[ material_pass_index ];

		//
		const UINT effect_pass_index = material_pass.filter_index;
		const NwShaderEffect::Pass& effect_pass = effect_passes[ effect_pass_index ];

		//
		const ScenePassInfo pass_info = render_path.getPassInfo( effect_pass.name_hash );

		//
		material_pass.render_state		= effect_pass.render_state;
		material_pass.filter_index		= pass_info.filter_index;
		material_pass.draw_order		= pass_info.draw_order;

		const UINT default_program_index = material_pass.program.id;
		material_pass.program = effect_pass.program_handles[ default_program_index ];
	}

	//
	Material *material_ = static_cast< Material* >( instance );

	material_->passes = material_passes;

	material_->command_buffer.InitializeWithReferenceTo(
		material_command_buffer
		);

	material_->effect = effect;

	material_->data = material_data;

	material_->_id = context.key.id;

	return ALL_OK;
}

void MaterialLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxUNDONE;
}

ERet MaterialLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

}//namespace Rendering
