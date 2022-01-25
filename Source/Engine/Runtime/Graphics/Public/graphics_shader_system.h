// Graphics Shader/"Effects" system.
#pragma once

#include <Base/Base.h>
#include <Base/Template/LoadInPlace/LoadInPlaceTypes.h>
#include <Base/Util/Color.h>

#include <Core/Assets/AssetReference.h>
#include <Core/NameHash.h>

#include <GPU/Public/graphics_commands.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_formats.h>



/*
=======================================================================
	SHADER
=======================================================================
*/

/// a shader feature bit mask
typedef U32 NwShaderFeatureBitMask;

struct NwVertexShader: NwResource
{
	HShader	handle;
public:
	mxDECLARE_CLASS(NwVertexShader,NwResource);
	mxDECLARE_REFLECTION;
};

struct NwPixelShader: NwResource
{
	HShader	handle;
public:
	mxDECLARE_CLASS(NwPixelShader,NwResource);
	mxDECLARE_REFLECTION;
};

struct NwGeometryShader: NwResource
{
	HShader	handle;
public:
	mxDECLARE_CLASS(NwGeometryShader,NwResource);
	mxDECLARE_REFLECTION;
};

struct NwComputeShader: NwResource
{
	HShader	handle;
public:
	mxDECLARE_CLASS(NwComputeShader,NwResource);
	mxDECLARE_REFLECTION;
};

struct NwShaderProgram: NwResource
{
	HProgram	handle;
public:
	mxDECLARE_CLASS(NwShaderProgram,NwResource);
	mxDECLARE_REFLECTION;
};


namespace Testbed {
namespace Graphics {

const TbMetaClass& shaderAssetClassForShaderTypeEnum( const NwShaderType::Enum shaderType );

}//namespace Graphics
}//namespace Testbed

struct NwShaderEffectData;

/// Precompiled command buffer:
/// contains commands for setting shader state and binding parameters.
/// Command buffer memory layout:
/// 0) BindCBuffers commands;
/// 1) BindTextures commands;
/// 2) BindSamplers commands;
/// 3) BindUAVs commands;
///
struct CommandBufferChunk: CStruct
{
	TBuffer< U32 >	d;

public:
	mxDECLARE_CLASS(CommandBufferChunk, CStruct);
	mxDECLARE_REFLECTION;

	mxFORCEINLINE bool IsEmpty() const
	{
		return d.IsEmpty();
	}

	void InitializeWithReferenceTo( CommandBufferChunk& other )
	{
		this->d.initializeWithExternalStorageAndCount(
			other.d.raw(),
			other.d.num()
			);
	}

	ERet CopyFromMemory( const void* src, const U32 src_size )
	{
		mxASSERT(IS_4_BYTE_ALIGNED(src_size));

		mxDO(d.setNum(
			tbALIGN4(src_size) / sizeof(d[0])
			));

		memcpy(d._data, src, src_size);

		return ALL_OK;
	}

	//ERet copyFrom( const CommandBufferChunk& other )
	//{
	//	mxDO(Arrays::copyFrom(
	//		this->d,
	//		other.d
	//		));

	//	this->size = other.size;
	//	this->size_of_bind_commands = other.size_of_bind_commands;
	//	this->uniform_parameters_offset = other.uniform_parameters_offset;

	//	return ALL_OK;
	//}

	ERet PushCopyInto( NGpu::CommandBuffer &command_buffer ) const
	{
		mxREFACTOR("remove check:");
		mxOPTIMIZE("copy using SSE words instead of memcpy");
		const U32 raw_size = d.rawSize();
		if( raw_size )
		{
			return command_buffer.WriteCopy(
				this->d._data,
				raw_size
				);
		}
		return ALL_OK;
	}

	void DbgPrint() const;
};


typedef TBuffer< Vector4 >	UniformBufferT;


/// keeps strong references to the textures used by the given shader effect/material
struct NwShaderTextureRef: CStruct
{
	TResPtr< NwTexture >	texture;

	/// index into the array of shader texture bindings within the effect/material
	U32						binding_index;

public:
	mxDECLARE_CLASS(NwShaderTextureRef, CStruct);
	mxDECLARE_REFLECTION;
};


/// Represents a shader technique.
/// Shader techniques define how to combine one or more shader programs
/// and other render state (e.g. alpha-blending behaviour) to render a model.
/// (Usually a technique only uses a single program, but sometimes it needs to perform multiple passes
/// over each model with a different program each time.) 
///
/// A note to myself: arrays cannot be resized after loading, only their elements can be updated
/// -> do as much work as possible offline, i.e. precompile and store a command buffer chunk
/// (with commands for setting shader state and parameters).
///
class NwShaderEffect: public NwSharedResource
{
public:
	///
	struct Pass: CStruct
	{
		// for searching by 'name'
		NameHash32			name_hash;	// 4

		//

		// non-programmable states
		NwRenderState32		render_state;	// 4

		//

		//
		HProgram			default_program_handle;	// 2

		//
		U16					default_program_index;	// 2

		// program permutations (inited upon loading)
		HProgram *			program_handles;	// 4/8

		// 32: 16
		// 64: 24

	public:
		mxDECLARE_CLASS(Pass, CStruct);
		mxDECLARE_REFLECTION;
	};

	// variable-length data - it can be hot-reloaded during development:
	NwShaderEffectData *	data;

	//AssetID		id;	// for debugging

public:
	mxDECLARE_CLASS(NwShaderEffect,NwResource);
	mxDECLARE_REFLECTION;
	NwShaderEffect();
	~NwShaderEffect();

	//
	ERet setUniform( const NameHash32 name_hash, const void* uniform_data );
	ERet setSampler( const NameHash32 name_hash, HSamplerState handle );
	ERet setUav( const NameHash32 name_hash, HShaderOutput handle );

	/// sets resource for reading
	ERet SetInput(
		const NwNameHash& name_hash,
		const HShaderInput handle
		);

public:	// Updating constants/uniforms.


	template< class ConstantBufferType >
	ERet AllocatePushConstants(
		ConstantBufferType **allocated_push_constants_
		, NGpu::CommandBuffer &command_buffer_
	) const
	{
		return AllocatePushConstants(
			(void**) allocated_push_constants_
			, sizeof(ConstantBufferType)
			, command_buffer_
			);
	}

	ERet AllocatePushConstants(
		void **allocated_push_constants_
		, const U32 push_constants_size
		, NGpu::CommandBuffer &command_buffer_
	) const;



	template< class ConstantBufferType >
	ERet BindConstants(
		const ConstantBufferType* constants
		, NGpu::CommandBuffer &command_buffer_
	) const
	{
		return BindConstants(
			constants
			, sizeof(ConstantBufferType)
			, command_buffer_
			);
	}

	ERet BindConstants(
		const void* constants
		, const U32 constants_size
		, NGpu::CommandBuffer &command_buffer_
	) const;


	///
	ERet SetCBuffer(
		const NwNameHash& name_hash,
		const HBuffer handle
		);

	/// returns -1 if the constant buffer wasn't found
	int GetCBufferBindSlot( const NameHash32 name_hash ) const;

public:	// Permutations

	struct Variant
	{
		HProgram			program_handle;
		NwRenderState32		render_state;
	};
	const Variant getDefaultVariant() const;

	/// returns 0 if the shader feature wasn't found
	NwShaderFeatureBitMask findFeatureMask( NameHash32 name_hash ) const;

	/// returns 0 if the shader feature wasn't found
	NwShaderFeatureBitMask composeFeatureMask(
		const NameHash32 name_hash,
		const unsigned value = 1
		) const;

public:
	UINT numPasses() const;
	const TSpan< const Pass > getPasses() const;

	//const Pass* findPassByName( const char* pass_name ) const;
	const Pass* findPass( const NameHash32 name_hash ) const;

public:	// command buffer submission

	/// binds textures, samplers, UAVs and constant buffer (if any)
	ERet pushShaderParameters(
		NGpu::CommandBuffer & command_buffer_
	) const;

	/// pushes all commands: parameter binding and updating uniform buffer
	ERet pushAllCommandsInto(
		NGpu::CommandBuffer &command_buffer_
	) const;


public:	//

	ERet BindCBufferData(
		void **allocated_uniform_data_
		, const U32 uniform_data_size
		, NGpu::CommandBuffer &command_buffer_
	) const;

	/// Inserts an 'Ensure Buffer Contents' command into the command buffer
	/// and allocates the uniform data immediately after the command.
	template< class ConstantBufferType >
	ERet BindCBufferData(
		ConstantBufferType **uniform_buffer_data_
		, NGpu::CommandBuffer &command_buffer_
	) const
	{
		return BindCBufferData(
			(void**) uniform_buffer_data_
			, sizeof(ConstantBufferType)
			, command_buffer_
			);
	}




#if 0	// TODO: when we support multiple local constant buffers
	ERet UpdateConstantBufferNamed(
		const NwNameHash& cbuffer_name_hash
		, GL::CommandBuffer & command_buffer
		, const void* source_data
		, const U32 source_data_size
		) const;

	template< struct ConstantBufferType >
	ERet UpdateConstantBufferNamed(
		const NwNameHash& cbuffer_name_hash
		, GL::CommandBuffer & command_buffer
		, const ConstantBufferType& source_data
		)
	{
		return UpdateConstantBufferNamed(
			cbuffer_name_hash
			, command_buffer
			, &source_data
			, sizeof(ConstantBufferType)
			);
	}
#endif



public:
	void debugPrint() const;
};


#pragma pack (push,1)
/// Blob header as stored on disk.
struct AssetHeader_d
{
	U32	dataSize;	// the size of the object data to allocate, excluding the header
};
ASSERT_SIZEOF(AssetHeader_d, 4);
#pragma pack (pop)



#pragma pack (push,1)
struct ShaderCacheHeader_d
{
	U16		numShaders[ NwShaderType::MAX ];	//12
	U32		numPrograms;//4
};
ASSERT_SIZEOF(ShaderCacheHeader_d, 16);
#pragma pack (pop)

