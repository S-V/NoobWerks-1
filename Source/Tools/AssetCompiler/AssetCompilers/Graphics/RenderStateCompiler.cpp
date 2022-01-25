#include "stdafx.h"
#pragma hdrstop

#include <Graphics/Public/graphics_formats.h>	// NwRenderState
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>	// NwRenderStateDesc

#include <Core/Serialization/Text/TxTSerializers.h>

#include <AssetCompiler/AssetCompilers/Graphics/RenderStateCompiler.h>

namespace AssetBaking
{

mxDEFINE_CLASS(RenderStateCompiler);

const TbMetaClass* RenderStateCompiler::getOutputAssetClass() const
{
	return &NwRenderState::metaClass();
}

static ERet compileRenderState(
					const NwRenderStateDesc& input
					, CompiledAssetData & output_
					)
{
	return NwBlobWriter(output_.object_data).Put(input);
}

ERet RenderStateCompiler::CompileAsset( const AssetCompilerInputs& inputs, AssetCompilerOutputs &_outputs )
{
	NwRenderStateDesc	def;
	mxDO(SON::Load(
		inputs.reader
		, def
		, MemoryHeaps::temporary()
		, inputs.path.c_str()
		));

	mxDO(compileRenderState( def, _outputs ));

	return ALL_OK;
}

ERet RenderStateCompiler::compile(
								  const AssetID& suggested_asset_id	// pass empty Asset ID to generate an Asset ID automatically
								  , const NwRenderStateDesc& render_state_d
								  , AssetDatabase & asset_database
								  , AssetID *returned_asset_id_ /*= nil*/
								  )
{
	CompiledAssetData	compiled_render_state(MemoryHeaps::temporary());

	mxDO(compileRenderState( render_state_d, compiled_render_state ));

	//
	mxDO(asset_database.addOrUpdateGeneratedAsset(
		suggested_asset_id
		, NwRenderState::metaClass()
		, compiled_render_state
		, returned_asset_id_
		));

	return ALL_OK;
}

}//namespace AssetBaking
