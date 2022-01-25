// render pipeline - a description of render passes (*.renderer files)
#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_RENDERING_PIPELINE_COMPILER
#include <Core/Memory.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Developer/RendererCompiler/Target_Common.h>

namespace AssetBaking
{

// accepts a render pipeline script and outputs a clump
struct RenderPipelineCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( RenderPipelineCompiler, AssetCompilerI );

	virtual const TbMetaClass* getOutputAssetClass() const override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;
};

static const Chars DEFAULT_RENDER_TARGET("$Default");

struct FxRenderTargetBinding: CStruct
{
	String32	name;
	bool		clear;
public:
	mxDECLARE_CLASS( FxRenderTargetBinding, CStruct );
	mxDECLARE_REFLECTION;
	FxRenderTargetBinding();
};

struct FxDepthStencilBinding: CStruct
{
	String32	name;

	/// 1 by default (normalize Z-buffer), 0 if using reversed Z
	float		depth_clear_value;

	/// Clear depth buffer?
	bool		is_cleared;

	/// Allows to bind a depth buffer as a depth-stencil view AND a shader resource (texture) simultaneously.
	/// Works only on Direct3D 11+.
	/// Limiting a depth-stencil buffer to read-only access allows
	/// more than one depth-stencil view to be bound to the pipeline simultaneously,
	/// since it is not possible to have a read/write conflicts between separate views.
	bool		is_read_only;

public:
	mxDECLARE_CLASS( FxDepthStencilBinding, CStruct );
	mxDECLARE_REFLECTION;
	FxDepthStencilBinding();
};

struct FxSceneLayerDef: CStruct
{
	String32	id;	// converted into string hash at run-time

	//TFixedArray< FxRenderTargetBinding, LLGL_MAX_BOUND_TARGETS > render_targets;
	TArray< FxRenderTargetBinding > render_targets;
	FxDepthStencilBinding	depth_stencil;

	Rendering::SortModeT	sort;
	String32	profiling_scope;

public:
	mxDECLARE_CLASS( FxSceneLayerDef, CStruct );
	mxDECLARE_REFLECTION;
	FxSceneLayerDef();
};

struct RenderPipelineDef : public FxResourceDescriptions
{
	// references to other clumps that must be loaded
	TArray< String32 >			includes;
	TArray< FxSceneLayerDef >	layers;
	TArray< String64 >			shaders;
public:
	mxDECLARE_CLASS( RenderPipelineDef, FxResourceDescriptions );
	mxDECLARE_REFLECTION;
	RenderPipelineDef();
};

}//namespace AssetBaking

#endif // WITH_RENDERING_PIPELINE_COMPILER
