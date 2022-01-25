#include "stdafx.h"
#pragma hdrstop
#include <AssetCompiler/AssetCompilers/PassThroughCompiler.h>


namespace AssetBaking
{

mxDEFINE_CLASS(PassThroughCompiler);

ERet PassThroughCompiler::Initialize()
{
	return ALL_OK;
}

void PassThroughCompiler::Shutdown()
{
}

const TbMetaClass* PassThroughCompiler::getOutputAssetClass() const
{
	return nil;
}

ERet PassThroughCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	mxDO(NwBlob_::loadBlobFromStream( inputs.reader, outputs.object_data ));
	return ALL_OK;
}

}//namespace AssetBaking
