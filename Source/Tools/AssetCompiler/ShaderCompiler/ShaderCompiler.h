#pragma once

#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <Developer/RendererCompiler/EffectCompiler_Direct3D.h>
#include <Developer/RendererCompiler/Target_Common.h>

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{
namespace ShaderCompilation
{
	ERet parseTechniqueDescriptionFromSourceFile(
		const NwBlob& fileData,
		const char* filename,
		FxShaderEffectDecl &shaderDef_
		);

	ERet compileEffectFromFile(
		const char* source_file_path,
		const ShaderCompilerConfig& settings,
		const FxShaderEffectDecl& effect_decl,
		const TArray< FxDefine >& defines,
		const char* _dependentFilePath,
		AssetDatabase * assetDatabase

		,NwBlob *shaderObjectBlob	// shader object
		,NwBlob *shaderBytecodeBlob	// compiled shader bytecode
		,ShaderMetadata &shader_metadata_
		);

	ERet compileEffectFromBlob(
		const NwBlob& fileData
		, const char* source_file_path
		, const ShaderCompilerConfig& shader_compiler_settings
		, AssetDatabase & asset_database
		, CompiledAssetData &compiled_effect_data_
		);

	ERet CompileEffect(
		const char* source_file_name	// e.g. "skinned.fx"
		, const ShaderCompilerConfig& shader_compiler_settings	// for searching shader source files
		, AssetDatabase & asset_database
		, CompiledAssetData &compiled_effect_data_
		, const char* material_file_path = nil
		);

}//namespace ShaderCompilation
}//namespace AssetBaking


