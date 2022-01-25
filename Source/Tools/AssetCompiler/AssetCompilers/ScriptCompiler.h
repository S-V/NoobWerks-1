#pragma once

#include <AssetCompiler/AssetPipeline.h>


#if WITH_SCRIPT_COMPILER


namespace AssetBaking
{

class ScriptCompiler: public AssetCompilerI
{
public:
	mxDECLARE_CLASS( ScriptCompiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking

#endif
