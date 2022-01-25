// compiler for animated (skinned) models (e.g. idTech 4's MD5)
#pragma once

#include <AssetCompiler/AssetPipeline.h>


namespace Rendering
{
	struct NwSkinnedMesh;
}

namespace Doom3
{
	struct ModelDef;
}


namespace AssetBaking
{

///
class Doom3_AnimatedModelCompiler: public AssetCompilerI
{
public:
	mxDECLARE_CLASS( Doom3_AnimatedModelCompiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

public:
	static ERet Static_CompileModelFromDef(
		const Doom3::ModelDef& model_def

		/// folder with .md5mesh and .md5anim files
		, const char* path_to_md5_files

		, const TResPtr< Rendering::NwSkinnedMesh >& skinned_mesh_id

		, const Doom3AssetsConfig& doom3_ass_cfg

		, AssetDatabase & asset_database

		, AllocatorI& scratchpad
		);
};

}//namespace AssetBaking
