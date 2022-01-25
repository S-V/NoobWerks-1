#pragma once

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

struct AudioAssetsCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( AudioAssetsCompiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

private:
	//MeshCompilerSettings	m_settings;
};

}//namespace AssetBaking
