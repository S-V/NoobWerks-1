#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_MESH_COMPILER



namespace Meshok
{
struct MeshImportSettings;
};//namespace Meshok

class TcModel;



namespace AssetBaking
{

struct MeshCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( MeshCompiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

public:
	static ERet CompileMeshAsset(
		const AssetCompilerInputs& inputs
		, AssetCompilerOutputs &outputs
		, TcModel &imported_model_
		, const Meshok::MeshImportSettings&	mesh_import_settings
		);
};

}//namespace AssetBaking
#endif
