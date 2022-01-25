// graphics material compiler
#include "stdafx.h"
#pragma hdrstop

#include <Base/Util/PathUtils.h>

#include <AssetCompiler/AssetCompilers/Graphics/_MaterialShaderEffectsCompilation.h>


#if WITH_MATERIAL_COMPILER

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Serialization_Private.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Developer/RendererCompiler/Target_Common.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Private/shader_effect_impl.h>
#include <Rendering/BuildConfig.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Material.h>

#include <Scripting/Scripting.h>
#include <Scripting/Lua/FunctionBinding.h>

#include <AssetCompiler/ShaderCompiler/ShaderCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/ShaderEffectCompiler.h>



namespace AssetBaking
{

static const bool FORCE_RECOMPILE_SHADER_EFFECTS = false;

namespace
{
	AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }
}



///
static
ERet compileMaterialEffectIfNeeded(
								   const char* source_file_name	// e.g. "skinned.fx" (as written in material description file)
								   , const AssetID& effect_asset_id	// e.g. "skinned"
								   , const ShaderCompilerConfig& shader_compiler_settings
								   , AssetDatabase & asset_database

								   , ShaderMetadata &shader_metadata_
								   , NwShaderEffectData &shader_effect_data_
								   , const DebugParam& dbgparam
									  )
{
	// Compile the effect or load the compiled data from the database.

	CompiledAssetData	compiled_effect_data;

	if (
		dbgparam.flag
		|| FORCE_RECOMPILE_SHADER_EFFECTS
		|| mxFAILED(asset_database.loadCompiledAssetData(
			compiled_effect_data
			, effect_asset_id
			, NwShaderEffect::metaClass()
			))
		)
	{
		mxDO(ShaderCompilation::CompileEffect(
			source_file_name
			, shader_compiler_settings
			, asset_database
			, compiled_effect_data
			));

		//
		mxDO(asset_database.addOrUpdateGeneratedAsset(
			effect_asset_id
			, NwShaderEffect::metaClass()
			, compiled_effect_data
			));
	}

	mxASSERT(compiled_effect_data.isOk());

	//

	// Deserialize run-time effect data.

	NwShaderEffectData *	shader_effect_data;
	mxDO(Serialization::LoadMemoryImage(
		shader_effect_data
		, NwBlobReader(compiled_effect_data.object_data)
		, tempMemHeap()
		));

	if( dbgparam.flag )
	{
		UNDONE;
		//const void* uniforms = shader_effect_data->command_buffer.getUniformsData();
		//GL::Dbg::printUniforms(
		//	uniforms, shader_effect_data->uniform_buffer_layout
		//	);
	}


	shader_effect_data_ = *shader_effect_data;

	mxDELETE_AND_NIL( shader_effect_data, tempMemHeap() );


	// Deserialize shader metadata.

	mxDO(SON::Load(
		NwBlobReader(compiled_effect_data.metadata)
		, shader_metadata_
		, tempMemHeap()
		));

	return ALL_OK;
}

ERet CompileMaterialEffectIfNeeded(
						   const char* effect_filename	// e.g. "skinned.fx" (as written in material description file)
						   , const AssetID& effect_asset_id	// e.g. "skinned"
						   , const TSpan< const FxDefine > shader_defines
						   , const ShaderCompilerConfig& shader_compiler_settings
						   , AssetDatabase & asset_database

						   , ShaderMetadata &shader_metadata_
						   , NwShaderEffectData &shader_effect_data_
						   , const DebugParam& dbgparam
						   )
{
	mxASSERT2(shader_defines.IsEmpty(),
		"material-specific #defines are not yet supported in effects!"
		);

	// e.g. "skinned.fx" (as written in material description file)
	FilePathStringT	effect_source_file_name;
	Str::AppendS( effect_source_file_name, effect_filename );
	Str::setFileExtension( effect_source_file_name, ".fx" );

	//
	mxDO(compileMaterialEffectIfNeeded(
		effect_source_file_name.c_str()
		, effect_asset_id
		, shader_compiler_settings
		, asset_database

		, shader_metadata_
		, shader_effect_data_
		, dbgparam
		));

	return ALL_OK;
}


// Select shader permutation.
ERet GetDefaultShaderFeatureMask(
								 NwShaderFeatureBitMask &shader_feature_mask_
								 , const TSpan< const FeatureSwitch > feature_switches
								 , const NwShaderEffectData& shader_effect_data
								 )
{
	NwShaderFeatureBitMask	shader_feature_mask = shader_effect_data.default_feature_mask;

	nwFOR_EACH( const FeatureSwitch& feature_switch, feature_switches )
	{
		const NameHash32 feature_name_hash = GetDynamicStringHash( feature_switch.key.c_str() );

		mxDO(shader_effect_data.bitOrWithFeatureMask(
			shader_feature_mask
			,feature_name_hash
			,feature_switch.value
			));
	}

	shader_feature_mask_ = shader_feature_mask;

	return ALL_OK;
}

}//namespace AssetBaking

#endif // WITH_MATERIAL_COMPILER
