#include "stdafx.h"
#pragma hdrstop

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Utility/GameFramework/TbPlayerWeapon.h>

#include <AssetCompiler/AssetCompilers/Game/WeaponDef_Compiler.h>

namespace AssetBaking
{

mxDEFINE_CLASS(WeaponDef_Compiler);

const TbMetaClass* WeaponDef_Compiler::getOutputAssetClass() const
{
	return &NwWeaponDef::metaClass();
}

ERet WeaponDef_Compiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	NwWeaponDef	def;
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

#if 0
	FileWriter("R:/out.son").Write(
		outputs.object_data.raw()
		, outputs.object_data.rawSize()
		);
#endif

	return ALL_OK;
}

}//namespace AssetBaking
