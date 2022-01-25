// Doom 3's *.mtr file reader, e.g. "weapons.mtr".
// https://www.iddevnet.com/doom3/materials.html
#pragma once

#include <Rendering/Public/Core/Material.h>

//#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

/*
-----------------------------------------------------------------------------
	Doom3_MaterialShader_Compiler
-----------------------------------------------------------------------------
*/
struct Doom3_MaterialShader_Compiler : public AssetCompilerI
{
	mxDECLARE_CLASS( Doom3_MaterialShader_Compiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

private:
	ERet parseAndCompileMaterialDecls(
		const char* filename
		, const ShaderCompilerConfig& shader_compiler_settings
		, const Doom3AssetsConfig& doom3_cfg
		, AssetDatabase & asset_database
		, IAssetBundler * asset_bundler
		, AllocatorI &	scratchpad
		);

	ERet parseTableDecl( Lexer & lexer );

	ERet parseMaterialDecl(
		Lexer & lexer
		, AssetDatabase & asset_database
		, IAssetBundler * asset_bundler
		, const ShaderCompilerConfig& shader_compiler_settings
		, const Doom3AssetsConfig& doom3_cfg
		, AllocatorI &	scratchpad
		);

	bool CheckSurfaceParm(
		const TbToken& token
		, int &surfaceFlags
		, int &contentFlags
		);

	ERet ParseSort(
		float &sort
		, int &materialFlags
		, Lexer & lexer
		);

	ERet ParseStereoEye(
		int &stereoEye
		, int &materialFlags
		, Lexer & lexer
		);

private:
	ERet Compile_idMaterial(
		CompiledAssetData &compiled_material_data_
		, Doom3::mtrParsingData_t & mtr_parsing_data
		, const AssetID& material_asset_id
		, AssetDatabase & asset_database
		, const ShaderCompilerConfig& shader_compiler_settings
		, AllocatorI &	scratchpad
		, const Lexer& lexer
		, const DebugParam& dbgparam
	);
	ERet Save_Compiled_idMaterial_Data(
		const CompiledAssetData& compiled_material_data
		, const AssetID& material_asset_id
		, AssetDatabase & asset_database
		, AllocatorI &	scratchpad
		);

	ERet _Compile_idMaterial_MultiplePasses(
		CompiledAssetData &compiled_material_data_
		, const Doom3::mtrParsingData_t& mtr_parsing_data
		, const AssetID& material_asset_id
		, AssetDatabase & asset_database
		, const ShaderCompilerConfig& shader_compiler_settings
		, AllocatorI &	scratchpad
		, const Lexer& lexer
		, const DebugParam& dbgparam
	);
};

}//namespace AssetBaking
