// compiler for spaceship meshes used in the game for Gaijin Jam #1
#pragma once

#include <AssetCompiler/AssetPipeline.h>


namespace AssetBaking
{


///
struct SpaceshipBuildSettings: CStruct
{
	/// rotation
	M44f	rotate;

	/// vertex scaling from [-1..+1]
	float	scale;

	bool	flip_winding_order;

public:
	mxDECLARE_CLASS( SpaceshipBuildSettings, CStruct );
	mxDECLARE_REFLECTION;
	SpaceshipBuildSettings();
};


struct Spaceship_Compiler : public AssetCompilerI
{
	mxDECLARE_CLASS( Spaceship_Compiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

}//namespace AssetBaking
