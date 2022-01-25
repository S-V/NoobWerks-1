#pragma once

#include <AssetCompiler/AssetPipeline.h>


namespace AssetBaking
{

/// Performs no processing on the file.
class PassThroughCompiler : public AssetCompilerI
{
public:
	mxDECLARE_CLASS( PassThroughCompiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking
