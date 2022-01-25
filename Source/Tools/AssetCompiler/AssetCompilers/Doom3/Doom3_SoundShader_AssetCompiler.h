#pragma once

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

struct Doom3_SoundShader_AssetCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( Doom3_SoundShader_AssetCompiler, AssetCompilerI );

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
