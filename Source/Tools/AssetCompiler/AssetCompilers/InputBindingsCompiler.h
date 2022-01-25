#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_INPUT_BINDINGS_COMPILER
#include <Core/Client.h>

namespace AssetBaking
{

struct InputBindingsCompiler : public MemoryImageWriterFor< NwInputBindings >
{
	mxDECLARE_CLASS( InputBindingsCompiler, AssetCompilerI );
};

}//namespace AssetBaking

#endif // WITH_INPUT_BINDINGS_COMPILER
