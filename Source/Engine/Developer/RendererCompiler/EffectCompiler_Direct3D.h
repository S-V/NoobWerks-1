#pragma once

#include <Base/Template/Containers/Blob.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>

#include "Target_Common.h"

/// allows to share shader and program asset instances
class AShaderDatabase
{
public:
	/// ensures that shaders are unique
	virtual const AssetID addShaderBytecode(
		const NwShaderType::Enum shaderType
		, const void* compiledBytecodeData
		, const size_t compiledBytecodeSize
		) = 0;

	///
	virtual const AssetID addProgramPermutation(
		const TbProgramPermutation& programPermutation
		) = 0;

	virtual ~AShaderDatabase() {}
};

ERet CompileEffect_D3D11(
						 // inputs:
						 const FxOptions& options
						 ,const FxShaderEffectDecl& techniqueDescription

						 //[in-out]
						 ,AShaderDatabase* shaderDatabase

						 // outputs:
						 ,NwBlob *shaderObjectDataBlob_	// shader object
						 ,NwBlob *shaderStreamingDataBlob_	// compiled shader bytecode
						 ,ShaderMetadata &metadata	// merged from all shader combinations/permutations/variations

						 //, const DebugParam& dbgparm
							);
