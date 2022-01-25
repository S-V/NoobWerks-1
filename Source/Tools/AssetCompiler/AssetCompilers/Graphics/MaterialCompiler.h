// graphics material compiler
#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_MATERIAL_COMPILER

#include <Core/Text/Token.h>
#include <Graphics/Public/graphics_formats.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>	// NwRenderStateDesc
#include <Rendering/Public/Core/Material.h>
#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <Developer/RendererCompiler/Target_Common.h>


namespace AssetBaking
{

struct idMaterialDef;



struct StringNameValuePair: CStruct
{
	String64	name;
	String64	value;
public:
	mxDECLARE_CLASS( StringNameValuePair, CStruct );
	mxDECLARE_REFLECTION;
};

///
struct MaterialTextureBinding: CStruct
{
	String64	name;	// as declared in the shader (e.g. "t_diffuseMap")
	String64	asset;	// default resource to bind (e.g. "test_diffuse")
	//SamplerID	sampler;// one of built-in engine samplers (e.g. "Bilinear")
public:
	mxDECLARE_CLASS( MaterialTextureBinding, CStruct );
	mxDECLARE_REFLECTION;
	MaterialTextureBinding();
};

/*
Materials can share the same shader.
So, when compiling a material, we create the corresponding shader asset
based on static permutations.
*/
struct MaterialDef: NonCopyable
{
	/// shader source file name (e.g. "material_plastic")
	String64	effect_filename;

	/// run-time shader permutation switches (e.g. "ENABLE_SPECULAR_MAP" = "1")
	TInplaceArray< FeatureSwitch, 8 >		feature_switches;

	/// material shader #defines (e.g. "DCC_TOOL_ONLY" = "1")
	TInplaceArray< FxDefine, 32 >		defines;

	///
	TInplaceArray< StringNameValuePair, 32 >		uniform_parameters;

	/// shader resource bindings
	TInplaceArray< MaterialTextureBinding, 32 >	textures;

	///
	struct SelectedPass
	{
		NameHash32	shader_pass_id;	// ScenePasses
		AssetID		render_state_id;
	};
	TInplaceArray< SelectedPass, 4 >	filtered_passes;

public:
	MaterialDef();

	ERet parseFrom(
		AReader & file_stream
		, const char* file_name_for_debugging
		);

private:
	ERet parseShaderFeatures(
		ATokenReader& lexer
	);

	ERet parseMaterialParameters(
		ATokenReader& lexer
		);

	ERet parseTextureBindings(
		ATokenReader& lexer
		);

	ERet parseResourceBindings(
		ATokenReader& lexer
		);
};

/*
----------------------------------------------------------
	MaterialCompiler
----------------------------------------------------------
*/
struct MaterialCompiler
	: public CompilerFor< Rendering::Material >
	, TGlobal< MaterialCompiler >
{
	mxDECLARE_CLASS( MaterialCompiler, AssetCompilerI );

	MaterialCompiler();

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

private:
	ERet compileMaterialEx(
		CompiledAssetData &outputs
		, const MaterialDef& material_def
		, const AssetID& material_asset_id
		, AssetDatabase & asset_database
		, const ShaderCompilerConfig& shader_compiler_settings
		, AllocatorI& temp_alloc
		, const DebugParam& dbgparam = DebugParam()
	) const;

	ERet compileMaterial(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
	) const;

	ERet evaluateUniforms(
		void * uniform_buffer_data_start_
		, const ShaderCBuffer& uniform_buffer
		, const MaterialDef& material_def
		) const;

public_internal:
	mutable float	_evalulated_uniforms[16];
	mutable int		_evalulated_uniforms_count;
};

}//namespace AssetBaking

#endif // WITH_MATERIAL_COMPILER
