#include "stdafx.h"
#pragma hdrstop

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Engine/Model.h>

#include <AssetCompiler/AssetCompilers/Game/Compiler_for_NwModelDef.h>


namespace AssetBaking
{

mxDEFINE_CLASS(Compiler_for_NwModelDef);

const TbMetaClass* Compiler_for_NwModelDef::getOutputAssetClass() const
{
	return &NwModelDef::metaClass();
}

ERet Compiler_for_NwModelDef::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	NwModelDef	def;
	mxDO(SON::Load(
		inputs.reader
		, def
		, MemoryHeaps::temporary()
		, inputs.path.c_str()
		));

	mxDO(SaveObjectData(
		def
		, outputs.object_data
		));

	return ALL_OK;
}

}//namespace AssetBaking
