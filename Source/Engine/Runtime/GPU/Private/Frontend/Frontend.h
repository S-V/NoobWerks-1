// renderer front-end
#pragma once

#include <GPU/Public/graphics_types.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_utilities.h>	// TransientBuffer
#include <GPU/Private/Backend/backend.h>


namespace NGpu
{

// At the start of the frame, the simulation thread begins to fill a render buffer with commands.
// At the same time, the render thread is processing the render buffer from the previous simulation frame.
// At the end of the frame, both threads synchronize and the buffers switch
// (i.e. one buffer can be filled with render commands, while the other is being executed, and vice versa).
// See:
// http://flohofwoe.blogspot.com/2012/08/twiggys-low-level-render-pipeline.html
// http://devmaster.net/forums/topic/9067-multi-threaded-rendering/
//
enum { NUM_BUFFERED_FRAMES = LLGL_MULTITHREADED ? 2 : 1 };

/*
Cost of State Changes (in decreasing order):
1) Render Targets
2) Shader Programs
3) ROP
4) Texture Bindings
5) Vertex Format
6) UBO Bindings
7) Vertex Bindings
8) Uniform Updates
*/

//
//  A sort key is a 64-bit value which is the combination of
//  a view ID, a program ID, a user-defined key and a reserved area.
//  The bits are laid out as follows:
//
//  +-------------+-----------------+--------------+--------------------+
//  | View ID (6) | Program ID (16) | Reserved (8) |  User-Defined Key  |
//  +-------------+-----------------+--------------+--------------------+
//  63            56                40             32                   0  bit
// MSB                                                                     LSB
//
//  User-Defined Key is usually view-space depth.
//

#define LLGL_VIEW_SHIFT	(64 - LLGL_NUM_VIEW_BITS)	// 58
#define LLGL_VIEW_MASK	((U64(LLGL_MAX_VIEWS-1)) << LLGL_VIEW_SHIFT)

#define CLEAR_NON_VIEW_BITS_MASK	((1ull << LLGL_VIEW_SHIFT) - 1)

#define LLGL_PROGRAM_SHIFT	(40u)	// 16 bits for program ID
#define LLGL_PROGRAM_MASK	((U64(LLGL_MAX_PROGRAMS-1)) << LLGL_PROGRAM_SHIFT)

static inline U64 EncodeSortKey( const U8 view_id, const U16 program_handle, const SortKey user_sort_key )
{
	const U64 view = U64(view_id) << LLGL_VIEW_SHIFT;
	const U64 program = U64(program_handle) << LLGL_PROGRAM_SHIFT;
	return view|program|user_sort_key;
}
static inline HProgram DecodeSortKey( const U64 key, U32 &view_id )
{
	view_id = ((key & LLGL_VIEW_MASK) >> LLGL_VIEW_SHIFT);
	return HProgram(
		(key & LLGL_PROGRAM_MASK) >> LLGL_PROGRAM_SHIFT
	);
}

typedef TLocalString< LLGL_MAX_VIEW_NAME_LENGTH >	ViewNameT;

///
struct TransientBuffer : NwDynamicBuffer
{
	void *	m_data;	//!< source data for updating the GPU buffer
public:
	TransientBuffer();

	ERet Initialize( EBufferType bufferType, U32 bufferSize, const char* name = nil );
	void Shutdown();

	ERet Allocate( U32 _stride, U32 _count, U32 *_offset, void **_writePtr );
	void Flush( AGraphicsBackend * backend );
};

///
mxPREALIGN(16) struct Frame
{
	// primary rendering context - for draw* commands
	NwRenderContext	main_context;

	// command queue for create/draw-frame/destroy commands
	// resource creation/deletion (so that texture uploads happen before draw* commands), device init/Shutdown
	NwRenderContext	frame_context;

	TransientBuffer	transient_vertex_buffer;
	TransientBuffer	transient_index_buffer;

	ViewState		view_states[LLGL_MAX_VIEWS];
	ShaderInputs	view_inputs[LLGL_MAX_VIEWS];
#if LLGL_ENABLE_PERF_HUD
	ViewNameT		view_names[LLGL_MAX_VIEWS];
#endif // LLGL_ENABLE_PERF_HUD

	BackEndCounters	perf_counters;

public:
	Frame( AllocatorI & allocator )
		: frame_context( allocator )
		, main_context( allocator )
	{
		for( int i = 0; i < mxCOUNT_OF(view_states); i++ ) {
			view_states[i].clear();
		}
		for( int i = 0; i < mxCOUNT_OF(view_states); i++ ) {
			view_inputs[i].clear();
		}
#if LLGL_ENABLE_PERF_HUD
		for( int i = 0; i < mxCOUNT_OF(view_states); i++ ) {
			Str::Format( view_names[i], "View_%d", i );
		}
#endif // LLGL_ENABLE_PERF_HUD

		perf_counters.reset();
	}

	~Frame()
	{
		//
	}

	ERet init()
	{
		//TODO: configure

		mxDO(main_context.reserve(
			1<<14,	// num_sort_items
			mxMiB(8)
			));

		mxDO(frame_context.reserve(
			1<<14,	// num_sort_items
			mxKiB(64)
			));

		return ALL_OK;
	}

	ERet initTransientBuffers();

	void releaseTransientBuffers();

} mxPOSTALIGN(16);

///
enum EShaderResourceType
{
	SRT_Buffer,
	SRT_Texture,
	SRT_ColorSurface,
	SRT_DepthSurface,

	SRT_NumBits = 2
};

/// NOTE: sorted by execution order, resource 'cost' and usage frequency
enum EFrameCommandType
{
	// INIT BACKEND

	eFrameCmd_RendererInit,

	// CREATE RESOURCE

	eFrameCmd_CreateColorTarget,
	eFrameCmd_CreateDepthTarget,

	eFrameCmd_CreateTexture2D,
	eFrameCmd_CreateTexture3D,
	eFrameCmd_CreateTexture,

	eFrameCmd_CreateBuffer,

	eFrameCmd_CreateShader,
	eFrameCmd_CreateProgram,

	eFrameCmd_CreateInputLayout,

	eFrameCmd_CreateDepthStencilState,
	eFrameCmd_CreateRasterizerState,
	eFrameCmd_CreateSamplerState,
	eFrameCmd_CreateBlendState,

	// UPDATE RESOURCE

	eFrameCmd_UpdateBuffer,
	eFrameCmd_UpdateTexture,

	//
	// DRAW
	eFrameCmd_RenderFrame,
	//

	// DESTROY RESOURCE

	eFrameCmd_DeleteBlendState,
	eFrameCmd_DeleteSamplerState,
	eFrameCmd_DeleteRasterizerState,
	eFrameCmd_DeleteDepthStencilState,

	eFrameCmd_DeleteInputLayout,

	eFrameCmd_DeleteProgram,
	eFrameCmd_DeleteShader,

	eFrameCmd_DeleteBuffer,

	eFrameCmd_DeleteTexture,

	eFrameCmd_DeleteDepthTarget,
	eFrameCmd_DeleteColorTarget,

	// SHUTDOWN BACKEND

	eFrameCmd_RendererShutdown,

	// Marker
	eFrameCmd_FirstCommandToExecuteAtFrameEnd = eFrameCmd_DeleteBlendState
};

static inline
SortKey encodeFrameCommandSortKey( EFrameCommandType command_type, unsigned handle_value )
{
	// first sort by command id, then by resource handle
	return (SortKey(command_type) << 32u) | handle_value;
}

static inline
void decodeFrameCommandSortKey( const SortKey sort_key, EFrameCommandType *command_type_, unsigned *handle_value_ )
{
	*command_type_ = (EFrameCommandType) (sort_key >> 32u);
	*handle_value_ = sort_key & 0xFFFFFFFF;
}

///
struct SDeviceCmd_InitBackend
{
	AGraphicsBackend::CreationInfo	creation_parameters;
};

struct SDeviceCmd_CreateInputLayout
{
	NwVertexDescription			description;
	const char *				debug_name;
};

struct SDeviceCmd_CreateTexture
{
	const Memory *				texture_data;
	GrTextureCreationFlagsT		creation_flags;
	const char *				debug_name;
};

struct SDeviceCmd_CreateTexture2D
{
	NwTexture2DDescription		description;
	const Memory *				optional_initial_data;
	const char *				debug_name;
};

struct SDeviceCmd_CreateTexture3D
{
	NwTexture3DDescription		description;
	const Memory *				optional_initial_data;
	const char *				debug_name;
};

struct SDeviceCmd_CreateColorTarget
{
	NwColorTargetDescription	description;
	const char *				debug_name;
};

struct SDeviceCmd_CreateDepthTarget
{
	NwDepthTargetDescription	description;
	const char *				debug_name;
};


struct SDeviceCmd_CreateDepthStencilState
{
	NwDepthStencilDescription	description;
	const char *				debug_name;
};

struct SDeviceCmd_CreateRasterizerState
{
	NwRasterizerDescription		description;
	const char *				debug_name;
};

struct SDeviceCmd_CreateSamplerState
{
	NwSamplerDescription		description;
	const char *				debug_name;
};

struct SDeviceCmd_CreateBlendState
{
	NwBlendDescription			description;
	const char *				debug_name;
};


struct SDeviceCmd_CreateBuffer
{
	NwBufferDescription			description;
	const Memory *				optional_initial_data;
	const char *				debug_name;
};

struct SDeviceCmd_CreateShader
{
	const Memory *	shader_binary;
};
struct SDeviceCmd_CreateProgram
{
	NwProgramDescription	description;
};


struct SDeviceCmd_UpdateBuffer
{
	const Memory *	new_contents;
	U32				debug_tag;	// for setting debug breakpoints
	U32				bytes_to_skip_after_this_command;
};
struct SDeviceCmd_UpdateTexture
{
	NwTextureRegion	update_region;	//!<
	const Memory *	new_contents;
};

template< class COMMAND >
ERet allocateCommandWithAlignedData(
	CommandBuffer & command_buffer_
	, COMMAND **command_
	, Memory **mem_
	, const U32 data_size
	, void **alloced_data_
	, const U32 debug_tag
	)
{
	// NOTE: the memory block header follows the command!
	COMMAND *	command;
	mxDO(command_buffer_.AllocateSpace(
		(void**) &command,
		sizeof(COMMAND) + sizeof(Memory),
		CommandBuffer::alignmentForType<COMMAND>() - 1
		));

	//
	Memory *	mem = (Memory*) ( command + 1 );

	//
	void* alloced_data;
	mxDO(command_buffer_.AllocateSpace( &alloced_data, tbALIGN16(data_size), 15 ));

	//
	mem->setRelativePointer( alloced_data, data_size );
	mem->debug_tag = debug_tag;

	//
	*command_ = command;
	*mem_ = mem;
	*alloced_data_ = alloced_data;

	return ALL_OK;
}

/// returns false if the data doesn't need to be freed
/// (e.g. it was allocated within the command buffer or stays constant during execution)
///
template< class COMMAND >
bool memoryFollowsCommand( const Memory* mem, const COMMAND* command )
{
	return (BYTE*)mem == (BYTE*)( command + 1 );
}

///
const Memory* allocateHeaderWithSize( AllocatorI & allocator, U32 size, U32 alignment );
void releaseMemory( AllocatorI & allocator, const Memory* mem );
void releaseMemory( const Memory* mem );	// scratch heap

}//namespace NGpu
