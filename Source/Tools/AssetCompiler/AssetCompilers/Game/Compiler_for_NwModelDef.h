#pragma once

#include <AssetCompiler/AssetPipeline.h>


namespace AssetBaking
{

struct Compiler_for_NwModelDef : public AssetCompilerI
{
	mxDECLARE_CLASS( Compiler_for_NwModelDef, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking
