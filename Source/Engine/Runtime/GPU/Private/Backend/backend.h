// common header file to be included by each back-end
#pragma once

namespace NGpu
{

class Frame;

///
struct UniformBufferPoolHandles
{
	enum CONSTANTS
	{
		// less than LLGL_MAX_UNIFORM_BUFFER_SIZE to waste less memory
		MAX_POOLED_CB_SIZE = mxKiB(1),

		MIN_SIZE_LOG2 = log2i(LLGL_MIN_UNIFORM_BUFFER_SIZE),
		MAX_SIZE_LOG2 = log2i(MAX_POOLED_CB_SIZE),
		NUM_SIZE_BINS = MAX_SIZE_LOG2 - MIN_SIZE_LOG2 + 1,
	};

	HBuffer	handles[LLGL_MAX_BOUND_UNIFORM_BUFFERS][NUM_SIZE_BINS];

public:
	static mxFORCEINLINE U32 getBufferSizeByBinIndex( const U32 bin_index )
	{
		return ( 1 << ( bin_index + MIN_SIZE_LOG2 ) );
	}

	static mxFORCEINLINE U32 getBinIndexForBufferOfSize( const U32 buffer_size )
	{
		const U32 rounded_up_buffer_size = CeilPowerOfTwo( buffer_size );
		const U32 bin_index = log2i( rounded_up_buffer_size ) - MIN_SIZE_LOG2;
		return bin_index;
	}
};

/// The lowest level immediate-mode drawing/rendering interface.
/// These functions are meant to be called from the render thread:
struct mxNO_VTABLE AGraphicsBackend abstract
{
	struct CreationInfo
	{
		//!< Window handle (HWND on Windows)
		const void *	window_handle;

		/// D3D11_CREATE_DEVICE_DEBUG
		bool	create_debug_device;

		DeviceSettings	device;


		AllocatorI *	allocator;

		//
		HColorTarget	framebuffer_handle;

		UniformBufferPoolHandles	uniform_buffer_pool_handles;

	public:
		CreationInfo()
		{
			window_handle = nil;
			create_debug_device = false;
		}
	};

	virtual ERet Initialize(
		const CreationInfo& settings
		, Capabilities &caps_
		, PlatformData &platform_data_
		) = 0;

	virtual void Shutdown() = 0;

	virtual bool isDeviceRemoved() const = 0;

	//
	virtual void createColorTarget(
		const unsigned index
		, const NwColorTargetDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteColorTarget( const unsigned index ) = 0;

	virtual void createDepthTarget(
		const unsigned index
		, const NwDepthTargetDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteDepthTarget( const unsigned index ) = 0;

	//
	virtual void createTexture(
		const unsigned index
		, const Memory* texture_data
		, const GrTextureCreationFlagsT flags
		, const char* debug_name
		) = 0;

	virtual void createTexture2D(
		const unsigned index
		, const NwTexture2DDescription& description
		, const Memory* initial_data
		, const char* debug_name
		) = 0;

	virtual void createTexture3D(
		const unsigned index
		, const NwTexture3DDescription& description
		, const Memory* initial_data
		, const char* debug_name
		) = 0;

	virtual void deleteTexture( const unsigned index ) = 0;

	//
	virtual void createDepthStencilState(
		const unsigned index
		, const NwDepthStencilDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteDepthStencilState( const unsigned index ) = 0;

	virtual void createRasterizerState(
		const unsigned index
		, const NwRasterizerDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteRasterizerState( const unsigned index ) = 0;

	virtual void createSamplerState(
		const unsigned index
		, const NwSamplerDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteSamplerState( const unsigned index ) = 0;

	virtual void createBlendState(
		const unsigned index
		, const NwBlendDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteBlendState( const unsigned index ) = 0;

	//
	virtual void createInputLayout(
		const unsigned index
		, const NwVertexDescription& description
		, const char* debug_name
		) = 0;
	virtual void deleteInputLayout( const unsigned index ) = 0;

	virtual ERet createBuffer(
		const unsigned index
		, const NwBufferDescription& description
		, const Memory* initial_data
		, const char* debug_name
		) = 0;
	virtual void deleteBuffer( const unsigned index ) = 0;

	//
	virtual void createShader( const unsigned index, const Memory* shader_binary ) = 0;
	virtual void deleteShader( const unsigned index ) = 0;

	virtual void createProgram( const unsigned index, const NwProgramDescription& description ) = 0;
	virtual void deleteProgram( const unsigned index ) = 0;

	//
	virtual void updateBuffer( const unsigned index, const void* data, const unsigned size ) = 0;
	virtual void UpdateTexture( const unsigned index, const NwTextureRegion& region, const Memory* new_contents ) = 0;

	//
	virtual void renderFrame( Frame & frame, const RunTimeSettings& settings ) = 0;

	virtual ~AGraphicsBackend() {}
};

///
#if LLGL_DEBUG_LEVEL >= LLGL_DEBUG_LEVEL_HIGH
	#define LLGL_DECLARE_STATIC_ARRAY( ElementType, variable_name, ArraySize )		TStaticArray< ElementType, ArraySize >	variable_name
#else
	#define LLGL_DECLARE_STATIC_ARRAY( ElementType, variable_name, ArraySize )		ElementType		variable_name[ ArraySize ]
#endif

template< class CommandType >
static mxFORCEINLINE
const CommandType& ReadCommandOfType( const char* & command_ptr )
{
	const CommandType& command = *(CommandType*) command_ptr;
	command_ptr += sizeof(CommandType);
	return command;
}

}//namespace NGpu
