#include <algorithm>	// std::sortkey(), std::stable_sort()
#include <bx/string.h>

#include <GlobalEngineConfig.h>
#include <Base/Base.h>
#include <Base/Template/HandleAllocator.h>
#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/ObjectModel/Clump.h>

#include <GPU/Public/graphics_config.h>
#include <GPU/Public/Debug/GraphicsDebugger.h>
#include "Frontend.h"


#if (LLGL_Driver == LLGL_Driver_Direct3D_11)
	#include <GPU/Private/Backend/d3d11/backend_d3d11.h>
#endif

#if (LLGL_Driver == LLGL_Driver_OpenGL_4plus)
	#include <GPU/Private/Backend/opengl/backend_opengl4.h>
#endif

#define DBG_CODE(...)	(__VA_ARGS__)


namespace NGpu
{

/// 
typedef U64 RenderStateHashT;

template< class RenderStateDescription >
RenderStateHashT calcRenderStateHash( const RenderStateDescription& o )
{
	return MurmurHash64( &o, sizeof(o) );
}

template< class RenderStateDescription, typename Handle >
Handle createUniqueRenderState(
							   const RenderStateDescription& description
							   , THashMap< RenderStateHashT, Handle > & state_cache
							   , bx::HandleAlloc & handle_alloc
							   )
{
	const RenderStateHashT hash = calcRenderStateHash( description );
	if( const Handle* existing_handle = state_cache.FindValue( hash ) )
	{
#if LLGL_VERBOSE
	DBGOUT("Created unique '%s' -> '%d'", typeid(description).name(), existing_handle->id);
#endif
		return *existing_handle;
	}

	const Handle new_handle ( handle_alloc.alloc() );
	mxASSERT(new_handle.IsValid());

	if( new_handle.IsValid() )
	{
		state_cache.Insert( hash, new_handle );
	}

	return new_handle;
}

static
char* ReallocateCommandBuffer( void* mem, U32 newSize, AllocatorI & heap ) {
	if( mem ) {
		heap.Deallocate( mem );
		mem = nil;
	}
	if( newSize ) {
		mem = heap.Allocate( newSize, 16 );	// all memory must be 16-byte aligned (for passing float4's, etc.)
	}
	return (char*) mem;
}


ERet Frame::initTransientBuffers()
{
#if MX_DEVELOPER
	mxDO(transient_vertex_buffer.Initialize(
		Buffer_Vertex, LLGL_TRANSIENT_VERTEX_BUFFER_SIZE
		IF_DEVELOPER , "TransientVB"
	));
	mxDO(transient_index_buffer.Initialize(
		Buffer_Index, LLGL_TRANSIENT_INDEX_BUFFER_SIZE
		IF_DEVELOPER , "TransientIB"
	));
#else
	mxDO(transient_vertex_buffer.Initialize(
		Buffer_Vertex, LLGL_TRANSIENT_VERTEX_BUFFER_SIZE
	));
	mxDO(transient_index_buffer.Initialize(
		Buffer_Index, LLGL_TRANSIENT_INDEX_BUFFER_SIZE
	));
#endif
	return ALL_OK;
}

void Frame::releaseTransientBuffers()
{
	transient_vertex_buffer.Shutdown();
	transient_index_buffer.Shutdown();
}

///
struct GraphicsFrontEnd
{
	char		frames_data[ sizeof(Frame) * NUM_BUFFERED_FRAMES ];
	Frame *		frame_to_submit;
	Frame *		frame_to_render;

	U64			num_rendered_frames;	// total number of rendered frames

	Capabilities	device_caps;

	BackEndCounters	last_rendered_frame_counters;

	//U32			videoMode;	// VideoModeFlags
	//Resolution	resolution;	// back buffer size
	//U32			flags;
	RunTimeSettings	runtime_settings;

	//
	AllocatorI &		object_allocator;	// for long-lived allocations
	AllocatorI &		scratch_allocator;	// for temporary buffers (like passing data between threads)

	// we should return the same handles for identical render states
	// TODO: deletion & reference counting?
	mxOPTIMIZE("linear hash table");
	THashMap< RenderStateHashT, HDepthStencilState >	depth_stencil_state_cache;
	THashMap< RenderStateHashT, HRasterizerState >		rasterizer_state_cache;
	THashMap< RenderStateHashT, HBlendState >			blend_state_cache;

	//
	bx::HandleAllocT<LLGL_MAX_DEPTH_STENCIL_STATES>	depth_stencil_state_handles;
	bx::HandleAllocT<LLGL_MAX_RASTERIZER_STATES>	rasterizer_state_handles;
	bx::HandleAllocT<LLGL_MAX_SAMPLER_STATES>		sampler_state_handles;
	bx::HandleAllocT<LLGL_MAX_BLEND_STATES>			blend_state_handles;

	bx::HandleAllocT<LLGL_MAX_INPUT_LAYOUTS>	input_layout_handles;

	bx::HandleAllocT<LLGL_MAX_DEPTH_TARGETS>	depth_target_handles;
	bx::HandleAllocT<LLGL_MAX_COLOR_TARGETS>	color_target_handles;

	bx::HandleAllocT<LLGL_MAX_SHADERS>			shader_handles;
	bx::HandleAllocT<LLGL_MAX_PROGRAMS>			program_handles;
	bx::HandleAllocT<LLGL_MAX_TEXTURES>			texture_handles;
	bx::HandleAllocT<LLGL_MAX_BUFFERS>			buffer_handles;

	//
	PlatformData	platform_data;

	// Data accessible from the Render thread only:
	//
	//AGraphicsBackend *	backend;

public:
	GraphicsFrontEnd( AllocatorI & object_allocator, AllocatorI & scratch_allocator )
		: object_allocator( object_allocator )
		, scratch_allocator( scratch_allocator )

		, depth_stencil_state_cache( object_allocator )
		, rasterizer_state_cache( object_allocator )
		, blend_state_cache( object_allocator )
	{
		last_rendered_frame_counters.reset();
	}

	~GraphicsFrontEnd()
	{
		releaseMemory();
	}

	ERet init( const Settings& settings )
	{
		for( int i = 0; i < NUM_BUFFERED_FRAMES; i++ ) {
			Frame &frame = ((Frame*) frames_data) [i];
			new( &frame ) Frame( object_allocator );
			mxDO(frame.init());
		}

		frame_to_submit = & ((Frame*) frames_data) [0];
		frame_to_render = & ((Frame*) frames_data) [NUM_BUFFERED_FRAMES-1];

		num_rendered_frames = 0;

#if (mxPLATFORM == mxPLATFORM_WINDOWS)
		HWND hWnd = (HWND) settings.window_handle;

		RECT rect;
		::GetClientRect(hWnd, &rect);

		const UINT width = rect.right - rect.left;
		const UINT height = rect.bottom - rect.top;

		runtime_settings.width = width;
		runtime_settings.height = height;
		runtime_settings.device = settings.device;
#else
#		error Unsupported platform!
#endif

		//
		mxZERO_OUT(device_caps);

		//
		mxDO(depth_stencil_state_cache.Initialize( LLGL_MAX_DEPTH_STENCIL_STATES, LLGL_MAX_DEPTH_STENCIL_STATES/2 ));
		mxDO(rasterizer_state_cache.Initialize( LLGL_MAX_RASTERIZER_STATES, LLGL_MAX_RASTERIZER_STATES/2 ));
		mxDO(blend_state_cache.Initialize( LLGL_MAX_BLEND_STATES, LLGL_MAX_BLEND_STATES/2 ));
		return ALL_OK;
	}

	ERet postInit( const Settings& settings )
	{
		for( int i = 0; i < NUM_BUFFERED_FRAMES; i++ ) {
			Frame &frame = ((Frame*) frames_data) [i];
			mxDO(frame.initTransientBuffers());
		}
		return ALL_OK;
	}

	void Reset()
	{
//		frame.main_context._sort_items.RemoveAll();
//		frame.main_context.m_current = 0;
	}

private:
	void releaseMemory()
	{
		for( int i = 0; i < NUM_BUFFERED_FRAMES; i++ ) {
			Frame &frame = ((Frame*) frames_data) [i];
			frame.~Frame();
		}
	}
};

static GraphicsFrontEnd *	s_frontend = nil;
static AGraphicsBackend *	s_backend = nil;

Settings::Settings()
{
	window_handle = nil;

	create_debug_device = false;

	max_gfx_cmds = 4096;
	cmd_buf_size = mxMiB(24);

	//flags = 0;

	scratch_allocator = nil;
	object_allocator = nil;
}

static void addCommandToInitializeBackEnd( const Settings& settings );
static void addCommandToShutdownBackEnd();
static void createBuiltInSamplerStates();
static void waitForRenderThreadToFinish();

static inline
Frame& getFrameToSubmit()
{
	return *s_frontend->frame_to_submit;
}

ERet Initialize( const Settings& settings )
{
	chkRET_X_IF_NIL(settings.window_handle, ERR_NULL_POINTER_PASSED);

	AllocatorI &	object_allocator = (settings.object_allocator != nil)
		? *settings.object_allocator : MemoryHeaps::graphics();

	AllocatorI &	scratch_allocator = (settings.scratch_allocator != nil)
		? *settings.scratch_allocator : MemoryHeaps::temporary();

	//
	s_frontend = mxNEW( object_allocator, GraphicsFrontEnd, object_allocator, scratch_allocator );

	mxDO(s_frontend->init( settings ));

	//

	mxDO(s_frontend->postInit( settings ));


	// Start a thread dedicated to rendering.


	//
	s_backend = mxNEW( object_allocator, BackendD3D11, object_allocator );

	addCommandToInitializeBackEnd( settings );

//TIncompleteType<sizeof(VertexDescription)> wow;
//TIncompleteType<sizeof(ShaderInputs)> wow;
//TIncompleteType<sizeof(Batch)> wow;
//TIncompleteType<sizeof(RenderCommands::Command)> wow;
DBGOUT("sizeof(Frame) = %u", sizeof(Frame));
DBGOUT("sizeof(VertexDescription) = %u", sizeof(NwVertexDescription));
DBGOUT("sizeof(ShaderInputs) = %u", sizeof(ShaderInputs));
DBGOUT("sizeof(ViewState) = %u", sizeof(ViewState));
DBGOUT("sizeof(Cmd_Draw) = %u", sizeof(Cmd_Draw));
DBGOUT("sizeof(Cmd_DispatchCS) = %u", sizeof(Cmd_DispatchCS));

	createBuiltInSamplerStates();

#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
	Graphics::DebuggerTool::Initialize( MemoryHeaps::graphics() );
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER

	return ALL_OK;
}

static void createBuiltInSamplerStates()
{
	using namespace BuiltIns;
	{
		NwSamplerDescription samplerDesc;
		samplerDesc.address_U = NwTextureAddressMode::Clamp;
		samplerDesc.address_V = NwTextureAddressMode::Clamp;
		samplerDesc.address_W = NwTextureAddressMode::Clamp;
		samplerDesc.filter = NwTextureFilter::Min_Mag_Mip_Point;
		g_samplers[PointSampler] = NGpu::CreateSamplerState(samplerDesc);

		samplerDesc.filter = NwTextureFilter::Min_Mag_Linear_Mip_Point;
		g_samplers[BilinearSampler] = NGpu::CreateSamplerState(samplerDesc);

		samplerDesc.filter = NwTextureFilter::Min_Mag_Mip_Linear;
		g_samplers[TrilinearSampler] = NGpu::CreateSamplerState(samplerDesc);

		samplerDesc.filter = NwTextureFilter::Anisotropic;
		samplerDesc.max_anisotropy = 4;	// max anisotropy must be 4 for FXAA
		g_samplers[AnisotropicSampler] = NGpu::CreateSamplerState(samplerDesc);


		samplerDesc.address_U = NwTextureAddressMode::Wrap;
		samplerDesc.address_V = NwTextureAddressMode::Wrap;
		samplerDesc.address_W = NwTextureAddressMode::Wrap;
		samplerDesc.max_anisotropy = 0;

		samplerDesc.filter = NwTextureFilter::Min_Mag_Mip_Point;
		g_samplers[PointWrapSampler] = NGpu::CreateSamplerState(samplerDesc);

		samplerDesc.filter = NwTextureFilter::Min_Mag_Linear_Mip_Point;
		g_samplers[BilinearWrapSampler] = NGpu::CreateSamplerState(samplerDesc);

		samplerDesc.filter = NwTextureFilter::Min_Mag_Mip_Linear;
		g_samplers[TrilinearWrapSampler] = NGpu::CreateSamplerState(samplerDesc);
	}

	g_samplers[DefaultSampler] = g_samplers[PointSampler];

	g_samplers[DiffuseMapSampler] = g_samplers[TrilinearWrapSampler];
	g_samplers[DetailMapSampler] = g_samplers[TrilinearWrapSampler];
	g_samplers[NormalMapSampler] = g_samplers[TrilinearWrapSampler];
	g_samplers[SpecularMapSampler] = g_samplers[TrilinearWrapSampler];
	g_samplers[RefractionMapSampler] = g_samplers[TrilinearWrapSampler];

	{
		NwSamplerDescription samplerDesc;
		samplerDesc.address_U = NwTextureAddressMode::Clamp;
		samplerDesc.address_V = NwTextureAddressMode::Clamp;
		samplerDesc.address_W = NwTextureAddressMode::Clamp;

		// D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D11_COMPARISON_LESS_EQUAL
		samplerDesc.filter = NwTextureFilter::Min_Mag_Mip_Point;
		samplerDesc.comparison = NwComparisonFunc::Less_Equal;

		g_samplers[ShadowMapSampler] = NGpu::CreateSamplerState(samplerDesc);

		// D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_COMPARISON_LESS_EQUAL
		samplerDesc.filter = NwTextureFilter::Min_Mag_Linear_Mip_Point;
		samplerDesc.comparison = NwComparisonFunc::Less_Equal;

		g_samplers[ShadowMapSamplerPCF] = NGpu::CreateSamplerState(samplerDesc);
	}
}

template <U16 MaxHandlesT>
static void checkHandleLeaks( const bx::HandleAllocT<MaxHandlesT>& handle_alloc
							 , const char* name
							 , void (AGraphicsBackend::*release_fun)( const unsigned index )
							 , AGraphicsBackend * backend )
{
	if( UINT num_handles = handle_alloc.getNumHandles() )
	{
		DEVOUT("LEAK: %s %d (max: %d)"
			, name
			, handle_alloc.getNumHandles()
			, handle_alloc.getMaxHandles() );  

		for( UINT i = 0; i < num_handles; i++ )
		{
			U16 handle = handle_alloc.getHandleAt(i);
			DEVOUT( "\t%3d: %4d", i, handle );
			(backend->*release_fun)( handle );
		}
	}
}

static void checkHandleLeaks()
{
	checkHandleLeaks( s_frontend->depth_stencil_state_handles, "depth_stencil_state_handles", &AGraphicsBackend::deleteDepthStencilState, s_backend );
	//checkHandleLeaks( s_frontend->rasterizer_state_handles, "rasterizer_state_handles" );
	//checkHandleLeaks( s_frontend->sampler_state_handles, "sampler_state_handles" );
	//checkHandleLeaks( s_frontend->blend_state_handles, "blend_state_handles" );

	//checkHandleLeaks( s_frontend->input_layout_handles, "input_layout_handles" );

	//checkHandleLeaks( s_frontend->depth_target_handles, "depth_target_handles" );
	//checkHandleLeaks( s_frontend->color_target_handles, "color_target_handles" );

	//checkHandleLeaks( s_frontend->shader_handles, "shader_handles" );
	//checkHandleLeaks( s_frontend->program_handles, "program_handles" );
	//checkHandleLeaks( s_frontend->texture_handles, "texture_handles" );
	//checkHandleLeaks( s_frontend->buffer_handles, "buffer_handles" );
}

void Shutdown()
{
	DEVOUT("Graphics: shutting down...");

#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
	Graphics::DebuggerTool::Shutdown();
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER

	getFrameToSubmit().releaseTransientBuffers();
	addCommandToShutdownBackEnd();
	NextFrame();

	if( LLGL_CREATE_RENDER_THREAD )
	{
		getFrameToSubmit().releaseTransientBuffers();
		NextFrame();
	}

	checkHandleLeaks();

	mxDELETE_AND_NIL( s_backend, s_frontend->object_allocator );
	mxDELETE_AND_NIL( s_frontend, s_frontend->object_allocator );
}

bool isInitialized()
{
	return s_frontend != nil;
}

NGpu::NwRenderContext& getMainRenderContext()
{
	return s_frontend->frame_to_submit->main_context;
}

void modifySettings( const RunTimeSettings& settings )
{
	s_frontend->runtime_settings = settings;
}

#pragma region "Memory management"

	static enum { DEFAULT_ALIGNMENT = 16 };

	const Memory* allocateHeaderWithSize( AllocatorI & allocator, U32 size, U32 alignment )
	{
		mxASSERT2(0 < size, "Invalid memory operation. size is 0.");

		Memory* header = (Memory*) allocator.Allocate( sizeof(Memory) + size, alignment );

		if( header )
		{
			header->data = (BYTE*) ( header + 1 );
			header->size = size;
			header->debug_tag = 0;
			header->is_relative = 0;
		}
		else
		{
			mxDBG_UNREACHABLE;
		}

		return header;
	}

	const Memory* Allocate( U32 size, U32 alignment )
	{
		return allocateHeaderWithSize(
			MemoryHeaps::temporary()//s_frontend->scratch_allocator
			, size
			, alignment
			);
	}

	const Memory* copy( const void* source_data, U32 size )
	{
		mxASSERT2(0 < size, "Invalid memory operation. size is 0.");
		const Memory* mem = Allocate(size);
		if( mem ) {
			memcpy( mem->data, source_data, size );
		}
		return mem;
	}

	struct MemoryRef
	{
		Memory mem;
		ReleaseFn releaseFn;
		void* userData;
	};
  
	const Memory* makeRef( const void* source_data
						  , const U32 data_size
						  , ReleaseFn release_callback
						  , void* user_data )
	{
		mxASSERT_PTR(source_data);
		mxASSERT(data_size > 0);

		MemoryRef* memRef = (MemoryRef*)s_frontend->scratch_allocator.Allocate(
			sizeof(MemoryRef)
			, DEFAULT_ALIGNMENT
			);
		memRef->mem.size  = data_size;
		memRef->mem.data  = (BYTE*) source_data;
		memRef->releaseFn = release_callback;
		memRef->userData  = user_data;
		return &memRef->mem;
	}

	static
	bool memoryBlockShouldBeFreed( const Memory* _mem )
	{
		return _mem->data != (BYTE*)( _mem + 1 );
	}

	void releaseMemory( AllocatorI & allocator, const Memory* header )
	{
		mxASSERT2(NULL != header, "memory can't be NULL");
		Memory* mem = const_cast<Memory*>(header);

		if( memoryBlockShouldBeFreed( mem ) )
		{
			MemoryRef* memRef = reinterpret_cast<MemoryRef*>(mem);
			if( NULL != memRef->releaseFn )
			{
				memRef->releaseFn(mem->data, memRef->userData);
			}
		}

		allocator.Deallocate( mem );
	}

	void releaseMemory( const Memory* header )
	{
		releaseMemory( s_frontend->scratch_allocator, header );
	}

#pragma endregion

#pragma region "Utilities"

static inline
NwRenderContext& getFrameContext()
{
	return getFrameToSubmit().frame_context;
}

static inline
CommandBuffer& getFrameCommandBuffer()
{
	return getFrameContext()._command_buffer;
}

template< class COMMAND_TYPE >
static
ERet addFrameCommand( EFrameCommandType command_type, const COMMAND_TYPE& command, unsigned handle_value )
{
	mxASSERT_MAIN_THREAD;
	mxSTATIC_ASSERT( IS_ALIGNED_BY( sizeof(COMMAND_TYPE), CommandBuffer::MINIMUM_ALIGNMENT ) );

	Frame &	frame_to_submit = getFrameToSubmit();
	NwRenderContext &	frame_context = frame_to_submit.frame_context;

	//
	mxDO(frame_context._command_buffer.put( command ));

	//
	const UINT curr_cmd_buf_offset = frame_context._command_buffer.currentOffset();

	frame_context.Submit(
		curr_cmd_buf_offset - sizeof(COMMAND_TYPE)
		, encodeFrameCommandSortKey( command_type, handle_value )
		nwDBG_CMD_SRCFILE_STRING
		);

	return ALL_OK;
}

template< class HANDLE_TYPE >
static
void addResourceDeletionCommand( EFrameCommandType command_type, HANDLE_TYPE handle )
{
	mxASSERT_MAIN_THREAD;
	mxASSERT(handle.IsValid());

	if( handle.IsValid() )
	{
		NwRenderContext &	frame_context = getFrameContext();

		frame_context.Submit(
			0
			, encodeFrameCommandSortKey( command_type, handle.id )
			nwDBG_CMD_SRCFILE_STRING
			);
	}
}

static void createPooledUniformBuffers( UniformBufferPoolHandles & uniform_buffer_pool_handles_ )
{
	DBGOUT("%d", LLGL_MAX_BOUND_UNIFORM_BUFFERS);
	DBGOUT("Initializing constant buffer pool: min size = %d, max size = %d, %d slots, %d bins per slot..."
		, (int)LLGL_MIN_UNIFORM_BUFFER_SIZE, (int)LLGL_MAX_UNIFORM_BUFFER_SIZE
		, (int)LLGL_MAX_BOUND_UNIFORM_BUFFERS, (int)UniformBufferPoolHandles::NUM_SIZE_BINS
		);

	// allocate larger buffers first to reduce fragmentation

	for(
		UINT cbuffer_size_log2 = UniformBufferPoolHandles::MAX_SIZE_LOG2;
		cbuffer_size_log2 >= UniformBufferPoolHandles::MIN_SIZE_LOG2;
		cbuffer_size_log2--
		)
	{
		const UINT cbuffer_size = ( 1 << cbuffer_size_log2 );

		const UINT bin_index = cbuffer_size_log2 - UniformBufferPoolHandles::MIN_SIZE_LOG2;

		//
		for( UINT slot = 0; slot < LLGL_MAX_BOUND_UNIFORM_BUFFERS; slot++ )
		{
			// NOTE: cannot use CreateBuffer() - pooled buffers must be created before other buffer commands are executed.
			//const HBuffer buffer_handle = CreateBuffer( buffer_description );	//<= this is wrong - it will try to update not-yet-created buffers
			const HBuffer buffer_handle( s_frontend->buffer_handles.alloc() );

			DBGOUT("Created uniform buffer of size %d bytes (slot: %d, bin: %d) -> %d"
				, cbuffer_size, slot, bin_index, buffer_handle.id
				);

			uniform_buffer_pool_handles_.handles[ slot ][ bin_index ] = buffer_handle;
		}
	}
}

static void addCommandToInitializeBackEnd( const Settings& settings )
{
	const HColorTarget	framebuffer_handle( s_frontend->color_target_handles.alloc() );

	SDeviceCmd_InitBackend	command;
	command.creation_parameters.window_handle = settings.window_handle;
	command.creation_parameters.create_debug_device = settings.create_debug_device;
	command.creation_parameters.device = settings.device;
	command.creation_parameters.allocator = settings.object_allocator;
	command.creation_parameters.framebuffer_handle = framebuffer_handle;
	createPooledUniformBuffers( command.creation_parameters.uniform_buffer_pool_handles );

	addFrameCommand( eFrameCmd_RendererInit, command, framebuffer_handle.id );
}

static void addCommandToShutdownBackEnd()
{
	getFrameContext().Submit(
		0
		, encodeFrameCommandSortKey( eFrameCmd_RendererShutdown, 0 )
		nwDBG_CMD_SRCFILE_STRING
		);
}

static void checkTextureFlags( const GrTextureCreationFlagsT flags, const Memory* initial_data )
{
	//bool isDynamic = (flags & GrTextureCreationFlags::DYNAMIC);
	//if( !isDynamic ) {
	//	mxASSERT( initial_data != nil );
	//}
}

#pragma endregion

HInputLayout createInputLayout(
							   const NwVertexDescription& description
							   , const char* debug_name
							   )
{
	mxASSERT(description.IsValid());

	const HInputLayout	new_handle( s_frontend->input_layout_handles.alloc() );
	mxASSERT2(new_handle.IsValid(),"failed to allocate input layout handle");

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateInputLayout	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateInputLayout, command, new_handle.id );
	}

	return new_handle;
}

void destroyInputLayout( HInputLayout handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteInputLayout, handle );
}

HTexture CreateTexture(
					   const Memory* texture_data
					   , const GrTextureCreationFlagsT flags
					   , const char* debug_name
					   )
{
	mxASSERT( texture_data->IsValid() );

#if LLGL_VERBOSE
	DBGOUT("CreateTexture(FromMemory): size=%u", texture_data->size);
#endif

	const HTexture	new_handle( s_frontend->texture_handles.alloc() );
	mxASSERT2(new_handle.IsValid(),"failed to allocate texture handle");

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateTexture	command = { texture_data, flags, debug_name };
		addFrameCommand( eFrameCmd_CreateTexture, command, new_handle.id );
	}

	return new_handle;
}

HTexture createTexture2D(
						 const NwTexture2DDescription& description
						 , const Memory* initial_data
						 , const char* debug_name
						 )
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateTexture(): ";
	DBG_DumpFields(&description,description.metaClass(),log);}
#endif
	checkTextureFlags( description.flags, initial_data );

	const HTexture	new_handle( s_frontend->texture_handles.alloc() );
	mxASSERT2(new_handle.IsValid(),"failed to allocate Texture2D handle");

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateTexture2D	command = { description, initial_data, debug_name };
		addFrameCommand( eFrameCmd_CreateTexture2D, command, new_handle.id );
	}

	return new_handle;
}

HTexture createTexture3D(
						 const NwTexture3DDescription& description
						 , const Memory* initial_data
						 , const char* debug_name
						 )
{
	const HTexture	new_handle( s_frontend->texture_handles.alloc() );
	mxASSERT2(new_handle.IsValid(),"failed to allocate Texture3D handle");

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateTexture3D	command = { description, initial_data, debug_name };
		addFrameCommand( eFrameCmd_CreateTexture3D, command, new_handle.id );
	}

	return new_handle;
}

void DeleteTexture( HTexture handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteTexture, handle );
}

#pragma region "Render Targets"

HColorTarget CreateColorTarget(
							   const NwColorTargetDescription& description
							   , const char* debug_name
							   )
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateColorTarget(): ";
	DBG_DumpFields(&description, description.metaClass(),log);}
#endif
  mxASSERT(description.IsValid());

	const HColorTarget	new_handle( s_frontend->color_target_handles.alloc() );
	mxASSERT2(new_handle.IsValid(),"failed to allocate ColorTarget handle");

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateColorTarget	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateColorTarget, command, new_handle.id );

#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
		Graphics::DebuggerTool::addColorTarget( new_handle, description, debug_name );
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
	}

	return new_handle;
}

void DeleteColorTarget( HColorTarget handle )
{
#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
		Graphics::DebuggerTool::removeColorTarget( handle );
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER

	addResourceDeletionCommand( eFrameCmd_DeleteColorTarget, handle );
}

HDepthTarget CreateDepthTarget(
							   const NwDepthTargetDescription& description
							   , const char* debug_name
							   )
{
	const HDepthTarget	new_handle( s_frontend->depth_target_handles.alloc() );
	mxASSERT(new_handle.IsValid());

#if LLGL_VERBOSE
	{
		LogStream log(LL_Debug);
		log << "CreateDepthTarget(): ";
		DBG_DumpFields(&description, description.metaClass(),log);
		log << " -> " << new_handle.id;
	}
#endif

	if( new_handle.IsValid() )
	{
		const SDeviceCmd_CreateDepthTarget	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateDepthTarget, command, new_handle.id );

#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
		Graphics::DebuggerTool::addDepthTarget( new_handle, description, debug_name );
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
	}

	return new_handle;
}

void DeleteDepthTarget( HDepthTarget handle )
{
#if LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER
	Graphics::DebuggerTool::removeDepthTarget( handle );
#endif // LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER

	addResourceDeletionCommand( eFrameCmd_DeleteDepthTarget, handle );
}

#pragma endregion

#pragma region "Render States"

HDepthStencilState CreateDepthStencilState(
	const NwDepthStencilDescription& description
	, const char* debug_name
	)
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateDepthStencilState(): ";
	DBG_DumpFields(&description, description.metaClass(),log);}
#endif

	const HDepthStencilState	new_handle = createUniqueRenderState(
		description,
		s_frontend->depth_stencil_state_cache,
		s_frontend->depth_stencil_state_handles
		);

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateDepthStencilState	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateDepthStencilState, command, new_handle.id );
	}

	return new_handle;
}

HRasterizerState CreateRasterizerState(
									   const NwRasterizerDescription& description
									   , const char* debug_name
									   )
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateRasterizerState(): ";
	DBG_DumpFields(&description, description.metaClass(),log);}
#endif

	const HRasterizerState	new_handle = createUniqueRenderState(
		description,
		s_frontend->rasterizer_state_cache,
		s_frontend->rasterizer_state_handles
		);

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateRasterizerState	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateRasterizerState, command, new_handle.id );
	}

	return new_handle;
}

HSamplerState CreateSamplerState(
								 const NwSamplerDescription& description
								 , const char* debug_name
								 )
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateSamplerState(): ";
	DBG_DumpFields(&description, description.metaClass(),log);}
#endif
	const HSamplerState	new_handle( s_frontend->sampler_state_handles.alloc() );
	mxASSERT(new_handle.IsValid());

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateSamplerState	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateSamplerState, command, new_handle.id );
	}

	return new_handle;
}

HBlendState CreateBlendState(
							 const NwBlendDescription& description
							 , const char* debug_name
							 )
{
#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateBlendState(): ";
	DBG_DumpFields(&description, description.metaClass(),log);}
#endif

	const HBlendState	new_handle = createUniqueRenderState(
		description,
		s_frontend->blend_state_cache,
		s_frontend->blend_state_handles
		);

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateBlendState	command = { description, debug_name };
		addFrameCommand( eFrameCmd_CreateBlendState, command, new_handle.id );
	}

	return new_handle;
}

void DeleteDepthStencilState( HDepthStencilState handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteDepthStencilState, handle );
}

void DeleteRasterizerState( HRasterizerState handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteRasterizerState, handle );
}

void DeleteSamplerState( HSamplerState handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteSamplerState, handle );
}

void DeleteBlendState( HBlendState handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteBlendState, handle );
}

#pragma endregion

#pragma region "Buffers"

HBuffer CreateBuffer(
					 const NwBufferDescription& description
					 , const Memory* initial_data
					 , const char* name
					 )
{
	const HBuffer	new_handle( s_frontend->buffer_handles.alloc() );
	mxASSERT(new_handle.IsValid());

#if LLGL_VERBOSE
	{LogStream log(LL_Debug);
	log << "CreateBuffer() -> " << new_handle.id << ": ";
	DBG_DumpFields(&description, description.metaClass(), log);}
#endif

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateBuffer	command = { description, initial_data };
		addFrameCommand( eFrameCmd_CreateBuffer, command, new_handle.id );
	}

	return new_handle;
}

void DeleteBuffer( HBuffer handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteBuffer, handle );
}

#pragma endregion

#pragma region "Shaders and Programs"

HShader CreateShader( const Memory* shader_binary )
{
	const HShader	new_handle( s_frontend->shader_handles.alloc() );
	mxASSERT(new_handle.IsValid());

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateShader	command = { shader_binary };
		addFrameCommand( eFrameCmd_CreateShader, command, new_handle.id );
	}

	return new_handle;
}

void DeleteShader( HShader handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteShader, handle );
}

HProgram CreateProgram( const NwProgramDescription& description )
{
	mxTODO("Prevent creation of duplicate shader programs?");
//#if LLGL_VERBOSE
//	DBGOUT("CreateProgram(%s) -> %u", pd.name.c_str(), handle.id);
//#endif
	const HProgram	new_handle( s_frontend->program_handles.alloc() );
	mxASSERT(new_handle.IsValid());

	if( new_handle.IsValid() )
	{
		SDeviceCmd_CreateProgram	command = { description };
		addFrameCommand( eFrameCmd_CreateProgram, command, new_handle.id );
	}

	return new_handle;
}

void DeleteProgram( HProgram & handle )
{
	addResourceDeletionCommand( eFrameCmd_DeleteProgram, handle );
}

#pragma endregion

#pragma region "Resource Handles -> Shader Inputs casting"

	template< class BINDING_HANDLE, class RESOURCE_HANDLE >
	static inline
	BINDING_HANDLE makeBindingHandle( EShaderResourceType type, RESOURCE_HANDLE resource_handle )
	{
		mxASSERT2(resource_handle.IsValid(), "Did you forget to init the resource handle?");
		BINDING_HANDLE handle;
		handle.id = (resource_handle.id << SRT_NumBits) | type;
		return handle;
	}

	HShaderInput AsInput( HBuffer handle )
	{
		return makeBindingHandle< HShaderInput >( SRT_Buffer, handle );
	}
	HShaderInput AsInput( HTexture handle )
	{
		return makeBindingHandle< HShaderInput >( SRT_Texture, handle );
	}
	HShaderInput AsInput( HColorTarget handle )
	{
		return makeBindingHandle< HShaderInput >( SRT_ColorSurface, handle );
	}
	HShaderInput AsInput( HDepthTarget handle )
	{
		return makeBindingHandle< HShaderInput >( SRT_DepthSurface, handle );
	}

	//

	HShaderOutput AsOutput( HBuffer handle )
	{
		return makeBindingHandle< HShaderOutput >( SRT_Buffer, handle );
	}
	HShaderOutput AsOutput( HTexture handle )
	{
		return makeBindingHandle< HShaderOutput >( SRT_Texture, handle );
	}
	HShaderOutput AsOutput( HColorTarget handle )
	{
		return makeBindingHandle< HShaderOutput >( SRT_ColorSurface, handle );
	}

#pragma endregion

#pragma region "Resource Updates"

	void updateBuffer( HBuffer handle, const Memory* new_contents )
	{
		mxASSERT(s_frontend->buffer_handles.IsValid( handle.id ));
		mxASSERT_PTR(new_contents);

		SDeviceCmd_UpdateBuffer	command = { new_contents };
		addFrameCommand( eFrameCmd_UpdateBuffer, command, handle.id );
	}

	ERet updateGlobalBuffer(
		HBuffer handle
		, const U32 size
		, void **new_contents_	// the returned memory will be 16-byte aligned
		, const U32 debug_tag /*= 0*/
		)
	{
		CommandBuffer &command_buffer = getFrameCommandBuffer();
		//
		SDeviceCmd_UpdateBuffer *	command;
		Memory *					mem;

		mxDO(allocateCommandWithAlignedData(
			command_buffer
			, &command
			, &mem
			, size
			, new_contents_
			, 0 //tag
			));
		command->new_contents = mem;
		command->bytes_to_skip_after_this_command = mxGetByteOffset32(
			command + 1,
			command_buffer.getCurrent()
			);
		command->debug_tag = debug_tag;

		//
		getFrameContext().Submit(
			mxGetByteOffset32( command_buffer.getStart(), command ),
			encodeFrameCommandSortKey( eFrameCmd_UpdateBuffer, handle.id )
			nwDBG_CMD_SRCFILE_STRING
			);

		return ALL_OK;
	}

	void UpdateTexture( HTexture handle, const NwTextureRegion& region, const Memory* new_contents )
	{
		SDeviceCmd_UpdateTexture	command = { region, new_contents };
		addFrameCommand( eFrameCmd_UpdateTexture, command, handle.id );
	}

#pragma endregion

#pragma region "Transient Buffers for Dynamic Geometry Rendering"

	bool EnoughSpaceInTransientVertexBuffer( U32 count, U32 stride )
	{
		return getFrameToSubmit().transient_vertex_buffer.HasSpace( stride, count );
	}

	bool EnoughSpaceInTransientIndexBuffer( U32 count, U32 stride )
	{
		return getFrameToSubmit().transient_index_buffer.HasSpace( stride, count );
	}

	ERet allocateTransientVertexBuffer( NwTransientBuffer &vertex_buffer_, U32 count, U32 stride )
	{
		U32 offset;
		mxTRY(getFrameToSubmit().transient_vertex_buffer.Allocate(
			stride, count
			, &offset, &vertex_buffer_.data
			));
		mxASSERT(offset % stride == 0);
		vertex_buffer_.size = count * stride;
		vertex_buffer_.base_index = offset / stride;
		vertex_buffer_.buffer_handle = getFrameToSubmit().transient_vertex_buffer.m_handle;
		return ALL_OK;
	}

	ERet allocateTransientIndexBuffer( NwTransientBuffer &index_buffer_, U32 count, U32 stride )
	{
		U32 offset;
		mxTRY(getFrameToSubmit().transient_index_buffer.Allocate(
			stride, count
			, &offset, &index_buffer_.data
			));
		mxASSERT(offset % stride == 0);
		index_buffer_.size = count * stride;
		index_buffer_.base_index = offset / stride;
		index_buffer_.buffer_handle = getFrameToSubmit().transient_index_buffer.m_handle;
		return ALL_OK;
	}

	ERet AllocateTransientBuffers(
		NwTransientBuffer &vertex_buffer, U32 vertex_count, U32 vertex_stride,
		NwTransientBuffer &index_buffer, U32 index_count, U32 index_stride
	)
	{
		if( EnoughSpaceInTransientVertexBuffer( vertex_count, vertex_stride )
			&&
			EnoughSpaceInTransientVertexBuffer( index_count, index_stride ) )
		{
			mxDO(allocateTransientVertexBuffer( vertex_buffer, vertex_count, vertex_stride ));
			mxDO(allocateTransientIndexBuffer( index_buffer, index_count, index_stride ));
			return ALL_OK;
		}
		return ERR_OUT_OF_MEMORY;
	}

#pragma endregion

#pragma region "Frame Submission"

void SetViewParameters(
	const LayerID view_id,
	const ViewState& view_state
)
{
	mxASSERT(view_id < LLGL_MAX_VIEWS);
	s_frontend->frame_to_submit->view_states[ view_id ] = view_state;
}

void SetViewShaderInputs(
	const LayerID view_id,
	const ShaderInputs& shader_inputs
)
{
	mxASSERT(view_id < LLGL_MAX_VIEWS);
	s_frontend->frame_to_submit->view_inputs[ view_id ] = shader_inputs;
}

void SetLayerConstantBuffer(
	const LayerID layer_id
	, const HBuffer constant_buffer
	, const U8 constant_buffer_slot
	)
{
	mxASSERT(layer_id < LLGL_MAX_VIEWS);
	ShaderInputs &layer_inputs = s_frontend->frame_to_submit->view_inputs[ layer_id ];
	mxASSERT(constant_buffer_slot < mxCOUNT_OF(layer_inputs.CBs));
	layer_inputs.CBs[ constant_buffer_slot ] = constant_buffer;
}

void SetViewDebugName(
	const LayerID view_id,
	const char* profiling_scope
)
{
#if LLGL_ENABLE_PERF_HUD
	Str::CopyS( s_frontend->frame_to_submit->view_names[ view_id ], profiling_scope );
#endif // LLGL_ENABLE_PERF_HUD
}

static
void sortAndExecuteFrameCommands_RT( Frame & frame_to_render
									, const RunTimeSettings& runtime_settings
									, AGraphicsBackend * backend )
{
	frame_to_render.perf_counters.reset();

	//
	NwRenderContext & frame_context = frame_to_render.frame_context;

	//
	frame_context.Sort();

	//
	const NwRenderContext::SortItem *	sorted_items = frame_context._sort_items.raw();
	const UINT						num_sorted_items = frame_context._sort_items.num();

	const NwRenderContext::SortItem *	end = sorted_items + num_sorted_items;

	char *	command_buffer_start = frame_context._command_buffer.getStart();

	const NwRenderContext::SortItem* current = sorted_items;

	while( current < end )
	{
		const NwRenderContext::SortItem& item = *current++;

		EFrameCommandType	command_type;
		unsigned			resource_handle;
		decodeFrameCommandSortKey( item.sortkey, &command_type, &resource_handle );

		const char* command_start = mxAddByteOffset( command_buffer_start, item.start );

		switch( command_type )
		{
		case eFrameCmd_RendererInit:
			{
				const SDeviceCmd_InitBackend* command = (SDeviceCmd_InitBackend*) command_start;
				backend->Initialize(
					command->creation_parameters
					, s_frontend->device_caps
					, s_frontend->platform_data
					);
			} break;

#pragma region "CREATE"

		case eFrameCmd_CreateColorTarget:
			{
				const SDeviceCmd_CreateColorTarget* command = (SDeviceCmd_CreateColorTarget*) command_start;
        mxASSERT(command->description.IsValid());
				backend->createColorTarget( resource_handle, command->description, command->debug_name );
			} break;

		case eFrameCmd_CreateDepthTarget:
			{
				const SDeviceCmd_CreateDepthTarget* command = (SDeviceCmd_CreateDepthTarget*) command_start;
				backend->createDepthTarget( resource_handle, command->description, command->debug_name );
			} break;


		case eFrameCmd_CreateTexture2D:
			{
				const SDeviceCmd_CreateTexture2D* command = (SDeviceCmd_CreateTexture2D*) command_start;
				backend->createTexture2D(
					resource_handle
					, command->description
					, command->optional_initial_data
					, command->debug_name
					);
				if( command->optional_initial_data ) {
					releaseMemory( command->optional_initial_data );
				}
			} break;

		case eFrameCmd_CreateTexture3D:
			{
				const SDeviceCmd_CreateTexture3D* command = (SDeviceCmd_CreateTexture3D*) command_start;
				backend->createTexture3D(
					resource_handle
					, command->description
					, command->optional_initial_data
					, command->debug_name
					);
				if( command->optional_initial_data ) {
					releaseMemory( command->optional_initial_data );
				}
			} break;

		case eFrameCmd_CreateTexture:
			{
				const SDeviceCmd_CreateTexture* command = (SDeviceCmd_CreateTexture*) command_start;
				backend->createTexture(
					resource_handle
					, command->texture_data
					, command->creation_flags
					, command->debug_name
					);
				releaseMemory( command->texture_data );
			} break;


		case eFrameCmd_CreateBuffer:
			{
				const SDeviceCmd_CreateBuffer* command = (SDeviceCmd_CreateBuffer*) command_start;
#if LLGL_DEBUG_LEVEL >= LLGL_DEBUG_LEVEL_LOW
				//ptPRINT("eFrameCmd_CreateBuffer: size=%d, name=%s", command->description.size, command->description.name.SafeGetPtr());
				mxASSERT(command->description.size > 0);
#endif
				backend->createBuffer(
					resource_handle
					, command->description
					, command->optional_initial_data
					, command->debug_name
					);
				if( command->optional_initial_data ) {
					releaseMemory( command->optional_initial_data );
				}
			} break;

		case eFrameCmd_CreateShader:
			{
				const SDeviceCmd_CreateShader* command = (SDeviceCmd_CreateShader*) command_start;
				backend->createShader( resource_handle, command->shader_binary );
				releaseMemory( command->shader_binary );
			} break;

		case eFrameCmd_CreateProgram:
			{
				const SDeviceCmd_CreateProgram* command = (SDeviceCmd_CreateProgram*) command_start;
				backend->createProgram( resource_handle, command->description );
			} break;

		case eFrameCmd_CreateInputLayout:
			{
				const SDeviceCmd_CreateInputLayout* command = (SDeviceCmd_CreateInputLayout*) command_start;
				backend->createInputLayout( resource_handle, command->description, command->debug_name );
			} break;

		case eFrameCmd_CreateDepthStencilState:
			{
				const SDeviceCmd_CreateDepthStencilState* command = (SDeviceCmd_CreateDepthStencilState*) command_start;
				backend->createDepthStencilState( resource_handle, command->description, command->debug_name );
			} break;

		case eFrameCmd_CreateRasterizerState:
			{
				const SDeviceCmd_CreateRasterizerState* command = (SDeviceCmd_CreateRasterizerState*) command_start;
				backend->createRasterizerState( resource_handle, command->description, command->debug_name );
			} break;

		case eFrameCmd_CreateSamplerState:
			{
				const SDeviceCmd_CreateSamplerState* command = (SDeviceCmd_CreateSamplerState*) command_start;
				backend->createSamplerState( resource_handle, command->description, command->debug_name );
			} break;

		case eFrameCmd_CreateBlendState:
			{
				const SDeviceCmd_CreateBlendState* command = (SDeviceCmd_CreateBlendState*) command_start;
				backend->createBlendState( resource_handle, command->description, command->debug_name );
			} break;

#pragma endregion

#pragma region "UPDATE"

		case eFrameCmd_UpdateBuffer:
			{
				const SDeviceCmd_UpdateBuffer* command = (SDeviceCmd_UpdateBuffer*) command_start;
				const Memory *	new_contents = command->new_contents;
//mxASSERT(command->debug_tag != 777);
				backend->updateBuffer(
					resource_handle,
					new_contents->getDataPtr(), new_contents->size
					);

				if( !memoryFollowsCommand( new_contents, command ) ) {
					releaseMemory( new_contents );
				}
			} break;

		case eFrameCmd_UpdateTexture:
			{
				const SDeviceCmd_UpdateTexture* command = (SDeviceCmd_UpdateTexture*) command_start;
				backend->UpdateTexture( resource_handle, command->update_region, command->new_contents );
				releaseMemory( command->new_contents );
			} break;

#pragma endregion

#pragma region "DRAW"
		case eFrameCmd_RenderFrame:
			{
				NwRenderContext & main_context = frame_to_render.main_context;

				//
#if GFX_CFG_DEBUG_COMMAND_BUFFER
				main_context.DbgPrint();
#endif // GFX_CFG_DEBUG_COMMAND_BUFFER

				main_context.Sort();

				backend->renderFrame( frame_to_render, runtime_settings );

				main_context.reset();
			} break;
#pragma endregion

#pragma region "DESTROY"

		case eFrameCmd_DeleteBlendState:
			backend->deleteBlendState( resource_handle );
			break;

		case eFrameCmd_DeleteSamplerState:
			backend->deleteSamplerState( resource_handle );
			break;

		case eFrameCmd_DeleteRasterizerState:
			backend->deleteRasterizerState( resource_handle );
			break;

		case eFrameCmd_DeleteDepthStencilState:
			backend->deleteDepthStencilState( resource_handle );
			break;

		case eFrameCmd_DeleteBuffer:
			backend->deleteBuffer( resource_handle );
			break;

		case eFrameCmd_DeleteInputLayout:
			backend->deleteInputLayout( resource_handle );
			break;

		case eFrameCmd_DeleteTexture:
			backend->deleteTexture( resource_handle );
			break;

		case eFrameCmd_DeleteProgram:
			backend->deleteProgram( resource_handle );
			break;

		case eFrameCmd_DeleteShader:
			backend->deleteShader( resource_handle );
			break;

		case eFrameCmd_DeleteDepthTarget:
			backend->deleteDepthTarget( resource_handle );
			break;

		case eFrameCmd_DeleteColorTarget:
			backend->deleteColorTarget( resource_handle );
			break;

#pragma endregion

		case eFrameCmd_RendererShutdown:
			backend->Shutdown();
			break;

			mxDEFAULT_UNREACHABLE(;);
		}//switch
	}//for
}

static
void executeBackEndCommands_RenderThread( Frame & frame_to_render, const RunTimeSettings& runtime_settings )
{
	NwRenderContext & frame_context = frame_to_render.frame_context;

	//
	sortAndExecuteFrameCommands_RT( frame_to_render, runtime_settings, s_backend );

	//
	frame_context.reset();
}

static void waitForRenderThreadToFinish()
{

#if LLGL_CREATE_RENDER_THREAD
	UNDONE
#endif

}

static
void kickRenderThread()
{

#if LLGL_CREATE_RENDER_THREAD

	UNDONE


#endif

}

static
void swapFrames()
{
	TSwap( s_frontend->frame_to_render, s_frontend->frame_to_submit );


}

ERet NextFrame()
{
	mxASSERT_MAIN_THREAD;

	//DBGOUT("\n\nNext Frame!!!\n\n");

	//
	getFrameContext().Submit(
		0
		, encodeFrameCommandSortKey( eFrameCmd_RenderFrame, 0 )
		nwDBG_CMD_SRCFILE_STRING
		);

	//
	waitForRenderThreadToFinish();

	s_frontend->last_rendered_frame_counters = s_frontend->frame_to_render->perf_counters;

	swapFrames();

	//
	kickRenderThread();

	//
//	if( !LLGL_CREATE_RENDER_THREAD || (s_frontend->runtime_settings.flags & SingleThreaded) )
	{
		executeBackEndCommands_RenderThread( *s_frontend->frame_to_render, s_frontend->runtime_settings );
	}

	//s_frontend->Reset();

	++s_frontend->num_rendered_frames;

	return ALL_OK;
}

U64 getNumRenderedFrames()
{
	return s_frontend->num_rendered_frames;
}

void getLastFrameCounters( BackEndCounters *last_rendered_frame_counters_ )
{
	*last_rendered_frame_counters_ = s_frontend->last_rendered_frame_counters;
}

#pragma endregion


void RenderTargets::clear()
{
	memset(color_targets, LLGL_NIL_HANDLE, sizeof(color_targets));
	mxZERO_OUT(clear_colors);
	target_count = 0;
	depth_target.SetNil();
}

void ViewState::clear()
{
	RenderTargets::clear();

//		renderState.setDefaults();
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = 0;
	viewport.height = 0;
	flags = 0;
	depth_clear_value = 0;
	stencil_clear_value = 0;
}


//=================================================================

void ShaderInputs::clear()
{
	memset( this, LLGL_NIL_HANDLE, sizeof(*this) );
}

//=================================================================

TransientBuffer::TransientBuffer()
{
	m_data = NULL;
}

ERet TransientBuffer::Initialize( EBufferType bufferType, U32 bufferSize, const char* name )
{
	mxDO(NwDynamicBuffer::Initialize( bufferType, bufferSize, name ));
	m_data = MemoryHeaps::graphics().Allocate( bufferSize, 16 );
	if( !m_data ) {
		return ERR_OUT_OF_MEMORY;
	}
	DBGOUT("Created transient '%s' buffer '%s' (%u bytes, 0x%p)",
		BufferTypeT_Type().FindString( m_type ), name ? name : "?", bufferSize, m_data );
	return ALL_OK;
}

void TransientBuffer::Shutdown()
{
	MemoryHeaps::graphics().Deallocate( m_data/*, m_capacity*/ );
	m_data = NULL;
	NwDynamicBuffer::Shutdown();
}

ERet TransientBuffer::Allocate( U32 _stride, U32 _count, U32 *_offset, void **_writePtr )
{
	U32 offset;
	mxTRY(NwDynamicBuffer::Allocate( _stride, _count, &offset ));
	*_writePtr = mxAddByteOffset( m_data, offset );
	*_offset = offset;
//DEVOUT("[%s] allocate '%u' items, '%u' bytes each (%u bytes), first=%u at [%u]", BufferTypeT_Type().FindString( m_type ), _count, _stride, _stride * _count, offset/_stride, offset);
	return ALL_OK;
}

void TransientBuffer::Flush( AGraphicsBackend * backend )
{
	if( m_current > 0 )
	{
		backend->updateBuffer( m_handle.id, m_data, m_current );

		this->Reset();
	}
}

//=================================================================

mxDEFINE_CLASS(GPUCounters);
mxBEGIN_REFLECTION(GPUCounters)
	mxMEMBER_FIELD(GPU_Time_Milliseconds),
	mxMEMBER_FIELD(IAVertices),
	mxMEMBER_FIELD(IAPrimitives),
	mxMEMBER_FIELD(VSInvocations),
	mxMEMBER_FIELD(GSInvocations),
	mxMEMBER_FIELD(GSPrimitives),
	mxMEMBER_FIELD(CInvocations),
	mxMEMBER_FIELD(CPrimitives),
	mxMEMBER_FIELD(PSInvocations),
	mxMEMBER_FIELD(HSInvocations),
	mxMEMBER_FIELD(DSInvocations),
	mxMEMBER_FIELD(CSInvocations),
mxEND_REFLECTION;

mxDEFINE_CLASS(BackEndCounters);
mxBEGIN_REFLECTION(BackEndCounters)
	mxMEMBER_FIELD(c_draw_calls),
	mxMEMBER_FIELD(c_triangles),
	mxMEMBER_FIELD(c_vertices),
	mxMEMBER_FIELD(gpu),
mxEND_REFLECTION;

void getFrontEndStats( FrontEndStats* front_end_stats_ )
{
	front_end_stats_->num_created_textures = s_frontend->texture_handles.getNumHandles();
	front_end_stats_->num_created_buffers = s_frontend->buffer_handles.getNumHandles();

	front_end_stats_->num_created_shaders = s_frontend->shader_handles.getNumHandles();
	front_end_stats_->num_created_programs = s_frontend->program_handles.getNumHandles();
}

const Capabilities getCapabilities()
{
	mxASSERT(s_frontend->num_rendered_frames > 0);
	return s_frontend->device_caps;
}

}//namespace NGpu

/*
Stingray Renderer Walkthrough #3: Render Contexts
http://bitsquid.blogspot.com/2017/02/stingray-renderer-walkthrough-3-render.html
https://gamedev.autodesk.com/blogs/1/post/2938410017602456387


https://github.com/ebkalderon/amethyst/blob/master/doc/Renderer%20Design%20Document.md
https://github.com/AnalyticalGraphicsInc/cesium/wiki/Data-Driven-Renderer-Details
Advanced Render Queue API - Gamedev Forums
Firaxis LORE And Other Uses of Direct3D - GDC 2011
Implementing a Render Queue For Games - Ploobs
Mantle Programming Guide and API Reference - AMD
Nitrous & Mantle: Combining Efficient Engine Design with a Modern API - GDC 2014
Order Your Draw Calls Around! - RTCD Blog
Parallel Rendering - Bitsquid Blog
Stateless Rendering - Jendrik Illner
Stateless Rendering Parts 1-4 - Molecular Musings

http://c0de517e.blogspot.ru/2014/04/how-to-make-rendering-engine.html

Lost Planet Multithreaded Rendering
http://nickporcino.com/meshula-net-archive/posts/post112.html

Immediate Mode Rendering
http://nickporcino.com/meshula-net-archive/posts/post130.html

http://mortoray.com/2014/05/29/wait-free-queueing-and-ultra-low-latency-logging/

A Novel Multithreaded Rendering System based on a Deferred Approach 
http://www2.ic.uff.br/~esteban/files/papers/SBGames09_Lorezon.pdf

The Rendering Technology of Skysaga: Infinite Isles
by Kostas Anagnostou, Principal Graphics Programmer at Radiant Worlds
http://www.radiantworlds.com/the-rendering-technology-of-skysaga-infinite-isles/

Implementations:
bgfx!
https://xp-dev.com/sc/113239/615/%2Ftrunk%2FSource%20Code%2FPlatform%2FVideo%2FHeaders%2FGFXDevice.h
https://xp-dev.com/sc/113239/615/%2Ftrunk%2FSource%20Code%2FPlatform%2FVideo%2FHeaders%2FRenderAPIWrapper.h
*/ 
