//
#pragma once

#include <Graphics/ForwardDecls.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>


///
struct NwShaderBindings: CStruct
{
	struct Binding: CStruct
	{
		/// The hash of the name for searching by name.
		NameHash32	name_hash;		// 4

		/// The offset of the corresponding Set* command within the command buffer.
		U16	command_offset;	// 2

		///TBD:
		U16	sampler_index;//additional_index;

	public:
		mxDECLARE_CLASS(Binding, CStruct);
		mxDECLARE_REFLECTION;

		template< class HandleType >
		mxFORCEINLINE HandleType* GetHandlePtrInCmdBuf(void* command_buffer) const
		{
			return reinterpret_cast< HandleType* >(
				mxAddByteOffset(
					command_buffer,
					command_offset + NGpu::Cmd_BindResource::HANDLE_OFFSET_WITHIN_COMMAND
				)
			);
		}
	};
	ASSERT_SIZEOF( Binding, 8 );


	// Global resource bindings are stored first.

	mxOPTIMIZE("use relocatable containers:");
#if 0
	/// Constant buffer bindings
	TRelArray< Binding, U16 >	_CBs;

	/// Shader resource (texture) bindings
	TRelArray< Binding, U16 >	_SRs;

	/// Shader sampler bindings
	TRelArray< Binding, U16 >	_SSs;

	/// Unordered Access Views
	TRelArray< Binding, U16 >	_UAVs;
#endif

	TBuffer< Binding >	_CBs;

	/// Shader resource (texture) bindings
	TBuffer< Binding >	_SRs;

	/// Shader sampler bindings
	TBuffer< Binding >	_SSs;

	/// Unordered Access Views
	TBuffer< Binding >	_UAVs;



	/// The number of references to global constant buffers than need to be patched upon loading.
	U8		_num_global_CBs;

	///
	U8		_num_global_SRs;

	U8		_num_builtin_SSs;


	/// Debug names, empty in release builds.
	struct DebugData: CStruct
	{
		TBuffer< String >	CB_names;
		TBuffer< String >	SR_names;
		TBuffer< String >	SS_names;
		TBuffer< String >	UAV_names;
	public:
		mxDECLARE_CLASS(DebugData, CStruct);
		mxDECLARE_REFLECTION;
	};
	DebugData	_dbg;

public:
	mxDECLARE_CLASS(NwShaderBindings, CStruct);
	mxDECLARE_REFLECTION;

	NwShaderBindings();



	//

	int findTextureBindingIndex(
		const NameHash32 name_hash
		) const;

	ERet SetInputResourceInCmdBuf(
		const NwNameHash& name_hash
		, const HShaderInput new_handle
		, void* command_buffer
		);

	//

	ERet SetSamplerInCmdBuf(
		const NameHash32 name_hash
		, HSamplerState handle
		, void * command_buffer
		);

	//

	ERet SetUavInCmdBuf(
		const NameHash32 name_hash
		, HShaderOutput handle
		, void * command_buffer
		);

public:
	void FixUpCBufferHandlesInCmdBuf(
		void *command_buffer
		, const NGraphics::ShaderResourceLUT& global_cbuffers_LUT
		) const;

	void FixUpSamplerHandlesInCmdBuf(
		void *command_buffer
		, const TSpan<HSamplerState>& local_samplers
		) const;

	void FixUpTextureHandlesInCmdBuf(
		void *command_buffer
		, const TSpan<NwShaderTextureRef>& texture_resources
		) const;

public:
	ERet DbgValidateHandlesInCmdBuf(
		const void* command_buffer
		) const;
};




/// represents variable-length data and is loaded in-place
struct NwShaderEffectData: CStruct
{
	/// shader input resource bindings;
	/// currently, they are the same for all passes and shader program instances
	NwShaderBindings		bindings;


	/// shader effect passes
	TBuffer< NwShaderEffect::Pass >		passes;



	/// Precompiled command buffer:
	/// contains the default values of uniform shader parameters
	/// and commands for setting shader state and binding parameters.
	CommandBufferChunk				command_buffer;



	/// empty if the effect doesn't use any uniform shader parameters
	NwUniformBufferLayout	uniform_buffer_layout;
	U32						uniform_buffer_slot;

	/// Initially contains the default values of uniform shader parameters.
	UniformBufferT			local_uniforms;




	//TBuffer< HSamplerState >			created_sampler_states;

	/// keeps strong references to used textures
	TBuffer< NwShaderTextureRef >		referenced_textures;



	/// Represents a run-time shader [switch|define|selector|pin]
	/// which can be toggled either on or off
	/// to identify a matching shader [variation|combination|instance].
	/// (It's represented by a #define in shader code.)
	struct ShaderFeature: CStruct
	{
		NameHash32				name_hash;
		NwShaderFeatureBitMask	bit_mask;
	public:
		mxDECLARE_CLASS(ShaderFeature, CStruct);
		mxDECLARE_REFLECTION;
	};
	ASSERT_SIZEOF( ShaderFeature, 8 );


	/// available shader features (for composing combination/permutation indices)
	TBuffer< ShaderFeature >	features;

	/// index of the default program permutation
	NwShaderFeatureBitMask		default_feature_mask;


	/// contains all combinations, indexed by NwShaderFeatureBitMask
	TBuffer< HProgram >		programs;

public:
	mxDECLARE_CLASS(NwShaderEffectData, CStruct);
	mxDECLARE_REFLECTION;

public:	// Shader Permutations management

	/// returns 0 if the shader feature wasn't found
	NwShaderFeatureBitMask findFeatureMask( NameHash32 name_hash ) const;

	/// returns 0 if the shader feature wasn't found
	NwShaderFeatureBitMask composeFeatureMask( NameHash32 name_hash, unsigned value ) const;

	ERet bitOrWithFeatureMask(
		NwShaderFeatureBitMask &result_
		, NameHash32 name_hash
		, unsigned value
	) const;

public:
	HUniform FindUniform(const NameHash32 name_hash) const;

	ERet setUniform(
		const NameHash32 name_hash
		, const void* uniform_data
		, CommandBufferChunk & command_buffer
		);

public:
	const NwShaderEffect::Pass* findPass( const NameHash32 name_hash ) const;
	int findPassIndex( const NameHash32 name_hash ) const;
};


///
class TbShaderEffectLoader: public TbAssetLoader_Null
{
public:
	TbShaderEffectLoader( ProxyAllocator & allocator );
  	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void destroy( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;

	enum { ALIGN = 4 };
};
