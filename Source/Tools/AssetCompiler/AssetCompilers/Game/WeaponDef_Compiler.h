#pragma once

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

struct WeaponDef_Compiler : public AssetCompilerI
{
	mxDECLARE_CLASS( WeaponDef_Compiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking
