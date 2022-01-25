#pragma once

#include <AssetCompiler/AssetPipeline.h>

struct NwRenderStateDesc;

namespace AssetBaking
{

struct RenderStateCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( RenderStateCompiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset( const AssetCompilerInputs& inputs, AssetCompilerOutputs &_outputs ) override;

public:
	static ERet compile(
		const AssetID& suggested_asset_id	// pass empty Asset ID to generate an Asset ID automatically
		, const NwRenderStateDesc& render_state_d
		, AssetDatabase & asset_database
		, AssetID *returned_asset_id_ = nil
		);
};

}//namespace AssetBaking
