// parses and compiles *.def files which contain entity definitions
#pragma once

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

///
class Doom3_idEntityDef_Compiler: public AssetCompilerI
{
public:
	mxDECLARE_CLASS( Doom3_idEntityDef_Compiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking
