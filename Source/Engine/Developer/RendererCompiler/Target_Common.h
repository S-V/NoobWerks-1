#pragma once

#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>	// NwRenderStateDesc
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include "Effect_Compiler.h"
#include <Developer/UniqueNameHashChecker.h>
#include <Developer/ShaderCompiler/ShaderCompiler.h>

using namespace Shaders;

//enum UIWidget
//{
//	UI_ComboBox,
//};

/// high-level render state
struct FxStateBlockDescription: NamedObject
{
	RGBAf	blendFactorRGBA;
	U32		sampleMask;
	String	blendState;
	String	depthStencilState;
	U8	stencilRef;
	String	rasterizerState;
public:
	mxDECLARE_CLASS(FxStateBlockDescription, NamedObject);
	mxDECLARE_REFLECTION;
	FxStateBlockDescription();
};

struct FxColorTargetDescription : TbColorTarget
{
public:
	mxDECLARE_CLASS(FxColorTargetDescription, TbColorTarget);
	mxDECLARE_REFLECTION;
	FxColorTargetDescription();
};

struct FxDepthTargetDescription : TbDepthTarget
{
public:
	mxDECLARE_CLASS(FxDepthTargetDescription, TbDepthTarget);
	mxDECLARE_REFLECTION;
	FxDepthTargetDescription();
};

/// static shader switches are set by the artist during development;
/// they are used to create different material shader variations;
struct FxStaticSwitch: CStruct
{
	String32	name;		//!< name of the corresponding #define macro in the shader code
	String32	tooltip;	//!< description
	String32	UIName;		//!< editor name
	String32	UIWidget;	//!< editor widget
	String32	defaultValue;
public:
	mxDECLARE_CLASS( FxStaticSwitch, CStruct );
	mxDECLARE_REFLECTION;
	FxStaticSwitch();
};

//// used to identify variations (permutations) of some shader program
//// (it's actually a bit mask constructed from #define's)
//typedef U32 rxShaderInstanceId;


/// A set of shader features is used to identify a specific variation of
/// a shader. For performance reasons, several shader features are
/// combined into a bit mask, so that finding a matching variation
/// can be done by bit-mask operations.
/// An application may define up to 32 unique features (corresponding
/// to the number of bits in an unsigned int).
struct FxShaderFeature: CStruct
{
	String32	name;		//!< name of the corresponding #define macro in the shader code
	String32	tooltip;	//!< description
	//String32	UIName;		//!< editor name
	//String32	UIWidget;	//!< editor widget
	bool		defaultValue;
public:
	mxDECLARE_CLASS( FxShaderFeature, CStruct );
	mxDECLARE_REFLECTION;
	FxShaderFeature();
};


struct FxShaderEntry: CStruct
{
	String64	file;	//!< shader source file name (e.g. "cool_shader.hlsl")
	// shader function entry points (empty == bind a NULL shader to that stage)
	String32	vertex;
	String32	pixel;
	String32	hull;
	String32	domain;
	String32	geometry;
	String32	compute;
public:
	mxDECLARE_CLASS(FxShaderEntry,CStruct);
	mxDECLARE_REFLECTION;
	const String& GetEntryFunction( NwShaderType::Enum _type ) const;
};

struct FxSamplerInitializer : CStruct
{
	String64	name;	// as declared in the shader (e.g. "s_diffuseMap")

	//
	U32			sampler_state_index;

	/// true if it's a SamplerID - one of built-in engine samplers (e.g. "Bilinear").
	/// otherwise, it's a custom sampler declared in the shader effect file.
	bool		is_built_in_sampler_state;

public:
	mxDECLARE_CLASS( FxSamplerInitializer, CStruct );
	mxDECLARE_REFLECTION;
};

struct FxTextureInitializer : CStruct
{
	String64	name;	// as declared in the shader (e.g. "t_diffuseMap")
	AssetID		texture_asset_name;	// default resource to bind (e.g. "test_diffuse")
public:
	mxDECLARE_CLASS( FxTextureInitializer, CStruct );
	mxDECLARE_REFLECTION;
};

struct FxScenePass: CStruct
{
	String32	name;	//!< optional (e.g. "FillGBuffer")

	String32	state;	//!< non-programmable states (empty string == default states)

	/// 'static' [compilation switches|defines]
	TInplaceArray< String32, 8 >	defines;	//!< e.g. "TARGET_HAS_SM5", "POST_PROCESS_ONLY"

	/// Run-time shader [compilation switches|defines|selectors|pins]
	/// e.g. "ENABLE_SHADOWS", "ENABLE_SPECULAR"
	TInplaceArray< FxShaderFeature, 8 >	features;

public:
	mxDECLARE_CLASS( FxScenePass, CStruct );
	mxDECLARE_REFLECTION;

	FxScenePass()
	{
	}

	/// calculates the total number of unique shader instances
	U32 NumPermutations() const
	{
		return (1UL << features.num());
	}
};

struct FxShaderDescription: CStruct
{
	FxShaderEntry		entry;

	TInplaceArray< FxScenePass, 8 >	scene_passes;

	TInplaceArray< FxSamplerInitializer, 8 >	bind_samplers;
	TInplaceArray< FxTextureInitializer, 8 >	bind_textures;

	/// switches or 'pins' for generating static permutations
	//TInplaceArray< FxStaticSwitch, 32 >	static_permutations;
	//TInplaceArray< VertexTypeT, 8 >	supportedVertexTypes;

	// do not create backing store for these constant buffers
	// (you want to do this if you know the layout of a constant buffer and it is very large)
	TInplaceArray< String32, 16 >	not_reflected_uniform_buffers;

	//StringListT			mirrored_uniform_buffers;	// create backing store to allow setting shader constants one by one
public:
	mxDECLARE_CLASS(FxShaderDescription,CStruct);
	mxDECLARE_REFLECTION;
	FxShaderDescription();

	U32 CalcNumPermutations() const {
		U32 num_unique_programs = 0;
		for( U32 pass = 0; pass < scene_passes.num(); pass++ ) {
			num_unique_programs += scene_passes[ pass ].NumPermutations();
		}
		return num_unique_programs;
	}

	const FxSamplerInitializer* findSamplerInitializerByName( const char* samplerNameInShaderCode ) const {
		return FindByName( bind_samplers, samplerNameInShaderCode );
	}
	const FxTextureInitializer* findTextureBindingByName( const char* texture_name_in_shader ) const {
		return FindByName( bind_textures, texture_name_in_shader );
	}
};






struct FxShaderEntry2: CStruct
{
	// shader function entry points (empty == bind a NULL shader to that stage)
	String32	vertex;
	String32	pixel;
	String32	hull;
	String32	domain;
	String32	geometry;
	String32	compute;
public:
	mxDECLARE_CLASS(FxShaderEntry,CStruct);
	mxDECLARE_REFLECTION;
	const String& GetEntryFunction( NwShaderType::Enum _type ) const;
};

struct FxShaderPassDesc: CStruct
{
	String32		name;	//!< e.g. "FillGBuffer" (see Rendering::ScenePasses enum)

	FxShaderEntry2	entry;


	/// non-programmable render states (empty string == default states)
	mxDEPRECATED String32		state;

	NwRenderStateDesc	custom_render_state_desc;
	bool				has_custom_render_state;


	/// 'static' [compilation switches|defines]
	TInplaceArray< String32, 8 >	defines;	//!< e.g. "DEFERRED_PASS", "POST_PROCESS_ONLY"

public:
	mxDECLARE_CLASS( FxShaderPassDesc, CStruct );
	mxDECLARE_REFLECTION;

	FxShaderPassDesc()
	{
		has_custom_render_state = false;
	}
};

/// A set of shader features is used to identify a specific variation of
/// a shader. For performance reasons, several shader features are
/// combined into a bit mask, so that finding a matching variation
/// can be done by bit-mask operations.
/// An application may define up to 32 unique features (corresponding
/// to the number of bits in an unsigned int).
struct FxShaderFeatureDecl: CStruct
{
	String64	name;			//!< name of the corresponding #define macro in the shader code
	String64	tooltip;		//!< description
	//String32	UIName;			//!< editor name
	//String32	UIWidget;		//!< editor widget
	U32			bitWidth;		//!< >= 1, default=1 (i.e. a boolean flag)
	U32			defaultValue;	//!< default=0 (i.e. false/off)
public:
	mxDECLARE_CLASS( FxShaderFeatureDecl, CStruct );
	mxDECLARE_REFLECTION;
	FxShaderFeatureDecl();
};

struct FxSamplerStateDecl: CStruct
{
	String64			name;			//!< name of the corresponding #define macro in the shader code
	NwSamplerDescription	desc;
	bool				is_used;
public:
	mxDECLARE_CLASS( FxSamplerStateDecl, CStruct );
	mxDECLARE_REFLECTION;
	FxSamplerStateDecl();
};

///
struct FxShaderEffectDecl: CStruct
{
	TInplaceArray< FxShaderPassDesc, 8 >	passes;

	/// Run-time shader [compilation switches|defines|selectors|pins]
	/// e.g. "ENABLE_SHADOWS", "ENABLE_SPECULAR"
	TInplaceArray< FxShaderFeatureDecl, 8 >	features;
	/// switches or 'pins' for generating static permutations (aka "USE_HACK_FOR_DEMO_LEVEL", "TARGET_HAS_SM5")
	//TInplaceArray< FxStaticSwitch, 32 >	static_permutations;
	//TInplaceArray< VertexTypeT, 8 >	supportedVertexTypes;

	//
	TInplaceArray< FxSamplerStateDecl, 8 >	sampler_descriptions;

	//
	TInplaceArray< FxSamplerInitializer, 8 >	sampler_bindings;
	TInplaceArray< FxTextureInitializer, 8 >	texture_bindings;

	// do not create backing store for these constant buffers
	// (you want to do this if you know the layout of a constant buffer and it is very large)
	//TInplaceArray< String32, 16 >	not_reflected_uniform_buffers;

	//StringListT			mirrored_uniform_buffers;	// create backing store to allow setting shader constants one by one
public:
	mxDECLARE_CLASS(FxShaderEffectDecl,CStruct);
	mxDECLARE_REFLECTION;
	FxShaderEffectDecl();

	//U32 CalcNumPermutations() const {
	//	U32 num_unique_programs = 0;
	//	for( U32 pass = 0; pass < passes.num(); pass++ ) {
	//		num_unique_programs += passes[ pass ].NumPermutations();
	//	}
	//	return num_unique_programs;
	//}

	/// calculates the total number of unique shader instances
	U32 NumPermutations() const
	{
		UNDONE;
		return (1UL << features.num());
	}

	const int findSamplerStateDeclarationIndexByName( const char* sampler_state_declaration_name ) const {
		return FindIndexByName( sampler_descriptions, sampler_state_declaration_name );
	}

	const FxSamplerInitializer* findSamplerInitializerByName( const char* samplerNameInShaderCode ) const {
		return FindByName( sampler_bindings, samplerNameInShaderCode );
	}

	const FxTextureInitializer* findTextureBindingByName( const char* texture_name_in_shader ) const {
		return FindByName( texture_bindings, texture_name_in_shader );
	}
};


struct FxResourceDescriptions: CStruct
{
	// global graphics resources (declarations)
	TArray< FxColorTargetDescription >	color_targets;
	TArray< FxDepthTargetDescription >	depth_targets;
	TArray< TbTexture2D >				textures;

	// immutable render state blocks
	TArray< NwDepthStencilDescription >	depth_stencil_states;
	TArray< NwRasterizerDescription >		rasterizer_states;
	TArray< NwSamplerDescription >		sampler_states;
	TArray< NwBlendDescription >			blend_states;
	TArray< FxStateBlockDescription >	state_blocks;

public:
	mxDECLARE_CLASS( FxResourceDescriptions, CStruct );
	mxDECLARE_REFLECTION;
	FxResourceDescriptions();
};

// initially parsed data - platform-independent shader library/pipeline
// from which Direct3D- or OpenGL-specific implementations can be built.
struct FxLibraryDescription : public FxResourceDescriptions
{
	// references to other clumps that must be loaded
	TInplaceArray< String32, 8 >		includes;

	// shader programs (imperative code)
	TInplaceArray< FxShaderDescription, 64 >	shader_programs;

public:
	mxDECLARE_CLASS( FxLibraryDescription, FxResourceDescriptions );
	mxDECLARE_REFLECTION;
	FxLibraryDescription();
};




ERet CreateResourceObjects(
						   const FxResourceDescriptions& library,
						   NwClump &clump
						   );

ERet CreateBackingStore(
						const FxLibraryDescription& description,
						NwClump& clump
						);



struct CompiledShaderBytecode
{
	TArray< BYTE >	code;
};

template<>
struct THashTrait< CompiledShaderBytecode >
{
	static inline U32 ComputeHash32( const CompiledShaderBytecode& x )
	{
		return MurmurHash32( x.code.raw(), x.code.rawSize() );
	}
};

template<>
struct TEqualsTrait< CompiledShaderBytecode >
{
	static inline bool Equals( const CompiledShaderBytecode& a, const CompiledShaderBytecode& b )
	{
		return a.code.rawSize() == b.code.rawSize()
			&& 0 == memcmp( a.code.raw(), b.code.raw(), a.code.rawSize() );
	}
};

typedef THashMap< CompiledShaderBytecode, U32 >	CompiledShaderBytecodeToUniqueIndexMapT;

