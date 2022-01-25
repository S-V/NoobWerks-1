// Graphics command buffer (software cmd buf, GAPI-independent).
#pragma once

#include <Base/Util/Color.h>	// RGBAi
#include <GPU/Public/graphics_types.h>


/*
=====================================================================
	COMMAND BUFFER
=====================================================================
*/
namespace NGpu
{

/*
 * Command buffers may be filled by the application with instructions
 * to execute along with other render commands.
 */

/// These render commands represent data dependencies.
/// They usually perform resource updates and state setup.
/// (enum values should be ordered from most common to least frequent)
enum ECommandType
{
	/// Must be zero! used for catching errors
	CMD_BAD = 0,


	// Set* commands are compiled into material assets' command buffers
	// and should be stable.

#pragma region "Set* commands"

	CMD_SET_CBUFFER,	//!< Set Constant Buffer
	CMD_SET_RESOURCE,	//!< Set Shader Resource
	CMD_SET_SAMPLER,	//!< Set Sampler State
	CMD_SET_UAV,		//!< Set Unordered Access View
	CMD_SET_RENDER_TARGETS,
	CMD_SET_VIEWPORT,
	CMD_SET_SCISSOR,
	CMD_SET_RENDER_STATE,

	/// Ensure that the uniform buffer at the specified slot is filled with the given data.
	CMD_BIND_CONSTANTS,
	/// ditto, but constants are allocated within the command buffer
	CMD_BIND_PUSH_CONSTANTS,

#pragma endregion



#pragma region "Draw* commands"

	CMD_DRAW,


	// Shader effects can generate drawcalls
	// (so that the user doesn't have to write them in code).

	/// Emit 3 points (used for drawing fullscreen triangles without vertex/index buffers)
	CMD_DRAW_TRIANGLE,

	/// Emit 14 points (used for drawing cubes without vertex/index buffers)
	CMD_DRAW_CUBE_TRISTRIP,

#pragma endregion



	//!< Compute shader: executes a command list from a thread group.
	CMD_DISPATCH_CS,



#pragma region "Update* commands"

	CMD_UPDATE_BUFFER_OLD,	//!< GfxCommand_UpdateBuffer_OLD

	/// Ensures that the shader constant buffer is filled with the given data.
	CMD_UPDATE_BUFFER,	//!< Cmd_UpdateBuffer




	CMD_UPDATE_TEXTURE,	//!< Cmd_UpdateTexture

	CMD_GENERATE_MIPS,	//!< Cmd_GenerateMips

	CMD_CLEAR_RENDER_TARGETS,
	CMD_CLEAR_DEPTH_STENCIL_VIEW,


	CMD_COPY_TEXTURE,	// e.g. copy to staging
	//TODO:rename to copy buffer or smth
	CMD_COPY_RESOURCE,	// e.g. copy to staging
	
#pragma endregion



#pragma region "Retrieve* commands"

	//RC_RESOLVE_TEXTURE,
	//RC_READ_PIXELS,	// e.g. read hit test render target
	CMD_MAP_READ,	// Copy data from GPU to CPU via mapped memory.

	//// Dynamic ('Transient') geometry rendering (aka DrawUP).
	//CMD_DRAW_DYNAMIC_GEOMETRY,

#pragma endregion



#pragma region "Debugging / Performance tools"

	CMD_PUSH_MARKER,	//!< Cmd_PushMarker
	CMD_POP_MARKER,		//!< Cmd_PopMarker
	CMD_SET_MARKER,		//!< Cmd_SetMarker
	CMD_DBG_PRINT,		//!< Cmd_DbgPrint

#pragma endregion


	//// Delete* commands

	// Special commands
	CMD_NOP,
	////RC_BeginFrame,	// begin rendering a new frame, reset internal states/counters
	////RC_END_OF_LIST

	CMD_NUM_BITS = 5
};

#if LLGL_DEBUG_LEVEL != LLGL_DEBUG_LEVEL_NONE
const char* dbg_commandToString( const ECommandType command_type );
#endif

#pragma pack (push,1)
/// NOTE: a render command cannot be smaller than this; all commands must be at least 4-byte aligned
struct RenderCommand
{
	// lowest 'CMD_NUM_BITS' bits = command type (e.g. CMD_UPDATE_BUFFER_OLD, BIND_SHADER)
	// highest 16 bits = resource handle (e.g. a buffer/shader handle)
	// input slot can be, e.g., a constant buffer bind point (register index);
	// shader stages mask tells which shader stages (e.g. VS, PS, CS) use this resource.
	//  +------------------+-------------------+----------------+------------------+
	//  | Resource ID (16) | Shader Stages (5) | Input Slot (6) | Command Type (5) |
	//  +------------------+-------------------+----------------+------------------+

	union
	{
		U32	header;

		struct
		{
			ECommandType type : 5;
			unsigned slot : 6;
			unsigned stages : 5;
			unsigned resource : 16;
		} bind_cmd;
	};

public:
	mxFORCEINLINE void encode( ECommandType command_type
		, U8 input_slot
		, U16 resource_handle )
	{
		header = (resource_handle << 16) | (input_slot << 5) | command_type;
	}

	mxFORCEINLINE ECommandType decode(
		U32 *input_slot_, U32 *resource_handle_
		) const
	{
		*input_slot_ = (header >> 5) & ((1<<6)-1);
		*resource_handle_ = header >> 16;
		return (ECommandType) ( header & ((1<<CMD_NUM_BITS)-1) );
	}
};
ASSERT_SIZEOF(RenderCommand, sizeof(U32));

/// CMD_SET_CBUFFER, CMD_SET_SAMPLER, CMD_SET_RESOURCE
struct Cmd_BindResource: RenderCommand
{
	enum { HANDLE_OFFSET_WITHIN_COMMAND = 2 };

	mxFORCEINLINE void setResourceHandle( U16 resource_handle ) {
		header = (header & 0x0000FFFF) | (resource_handle << 16);
	}
};

///
struct Cmd_UpdateResource: RenderCommand
{
};

///
struct GfxCommand_UpdateBuffer_OLD: Cmd_UpdateResource
{
	// (the buffer handle (HBuffer) is encoded in the header)

	const Memory *	new_contents;	//!< source data, must be 16-byte aligned

	U32				bytes_to_skip_after_this_command;

	//U32			size;
	//const void* data;			//!< source data, must be 16-byte aligned
	//AllocatorI *		deallocator;	//!< nil if the data doesn't need to be freed (e.g. it was allocated within the command buffer or stays constant during execution)
};


struct Cmd_UpdateBuffer: Cmd_UpdateResource
{
	// (the buffer handle (HBuffer) is encoded in the header)

	/// NOTE: The memory must be 16-byte aligned!
	/// After submitting, the memory is supposed to be touched only by the render thread!
	const void *	data;		//!< source data, must be 16-byte aligned
	U32				size;

	/// NOTE: must be threadsafe, because the data will be freed on the render thread!
	/// Can be nil if the data doesn't need to be freed.
	/// (e.g. it was allocated within the command buffer or stays constant during execution)
	DeallocatorI *	deallocator;

	/// NOTE: must be a static string, won't be copied!
	const char *	dbgname;

	//32:20
};


/// Ensures that the shader constant buffer is filled with the given data.
struct Cmd_BindConstants: Cmd_UpdateResource	// 4
{
	/// NOTE: The memory must be 16-byte aligned!
	/// After submitting, the memory is supposed to be touched only by the render thread!
	const void *	data;		//!< source data, must be 16-byte aligned
	U32				size;

	/// If not zero, this command is followed by a null-terminated string.
	U32	size_of_debug_string_after_this_command;

	/// NOTE: must be threadsafe, because the data will be freed on the render thread!
	/// Can be nil if the data doesn't need to be freed.
	/// (e.g. it was allocated within the command buffer or stays constant during execution)
	DeallocatorI *	deallocator;


	// ... followed by the data block with "push constants".


	mxFORCEINLINE char* GetDebugMessageStringIfAny() const
	{
		return size_of_debug_string_after_this_command
			? ((char*) (this + 1))
			: nil
			;
	}

	mxFORCEINLINE const TSpan<BYTE> GetConstantsData() const
	{
		return TSpan<BYTE>(
			(BYTE*) data
			, size
			);
	}

	mxFORCEINLINE RenderCommand* GetNextCommandPtr() const
	{
		return (RenderCommand*) (
			((char*) (this + 1)) + size_of_debug_string_after_this_command
			);
	}
};


/// Ensures that the shader constant buffer is filled with the given data.
struct Cmd_BindPushConstants: Cmd_UpdateResource	// 4
{
	/// If not zero, this command is followed by a null-terminated string.

	U8	size_of_debug_string_after_this_command;

	///
	U8	offset_of_push_constants_after_debug_string;

	///
	U16	size_of_push_constants;	// 16-byte aligned


	// ... followed by the data block with "push constants".


	mxFORCEINLINE char* GetDebugMessageStringIfAny() const
	{
		return size_of_debug_string_after_this_command
			? ((char*) (this + 1))
			: nil
			;
	}

	mxFORCEINLINE const TSpan<BYTE> GetConstantsData() const
	{
		const char* push_constants_data
			= (char*) (this + 1)
			+ size_of_debug_string_after_this_command
			+ offset_of_push_constants_after_debug_string
			;
		return TSpan<BYTE>(
			(BYTE*) push_constants_data
			, size_of_push_constants
			);
	}

	mxFORCEINLINE RenderCommand* GetNextCommandPtr() const
	{
		const TSpan<BYTE> push_constants = this->GetConstantsData();
		return (RenderCommand*) ( push_constants._data + push_constants._count );
	}
};
ASSERT_SIZEOF( Cmd_BindPushConstants, 8 );





///
struct Cmd_UpdateTexture: Cmd_UpdateResource
{
	NwTextureRegion	updateRegion;	//!<
	const void *	data;			//!< source data (size is calculated from the texture update region)
	AllocatorI *			deallocator;	//!< nil if the data was allocated using the command buffer
};

/// contains a HShaderInput handle corresponding to the texture resource
struct Cmd_GenerateMips: Cmd_UpdateResource
{
};

///
struct Cmd_ClearRenderTargets: RenderCommand
{
	HColorTarget	color_targets[LLGL_MAX_BOUND_TARGETS];//!< render targets to clear
	float			clear_colors[LLGL_MAX_BOUND_TARGETS][4];//!< RGBA colors for clearing each RT
	U32				target_count;	//!< number of color targets to clear

	Cmd_ClearRenderTargets()
	{
		this->encode( CMD_CLEAR_RENDER_TARGETS, 0, 0 );
	}
};
mxSTATIC_ASSERT(sizeof(Cmd_ClearRenderTargets) % 4 == 0);

///
struct Cmd_ClearDepthStencilView: RenderCommand
{
	HDepthTarget	depth_target;	//!< the depth-stencil surface
	char			padding[3];

	float			depth_clear_value;		//!< value to clear depth buffer with (default=1.)
	U32				stencil_clear_value;	//!< value to clear stencil buffer with (default=0)

	Cmd_ClearDepthStencilView()
	{
		this->encode( CMD_CLEAR_DEPTH_STENCIL_VIEW, 0, 0 );
	}
};
mxSTATIC_ASSERT(sizeof(Cmd_ClearDepthStencilView) == 16);

///
struct Cmd_CopyResource: RenderCommand
{
	HBuffer	source;
	HBuffer	destination;
};
mxSTATIC_ASSERT(sizeof(Cmd_CopyResource) % 4 == 0);

struct Cmd_CopyTexture: RenderCommand
{
	HTexture src_texture_handle;
	HTexture dst_texture_handle;
};

///
struct Cmd_MapRead: RenderCommand
{
	HBuffer	buffer;
	U16		padding;

	void	(*callback)( void* userData, void *mappedData );
	void *	userData;
};
mxSTATIC_ASSERT(sizeof(Cmd_MapRead) % 4 == 0);


///
struct Cmd_SetRenderTargets: RenderCommand
{
	RenderTargets	render_targets;
};//144


///
struct Cmd_SetViewport: RenderCommand
{
	// Viewport Top left
	float top_left_x;
	float top_left_y;

	// Viewport Dimensions
	float width;
	float height;

	// Min/max of clip Volume
	float min_depth;
	float max_depth;
};

///
struct Cmd_SetScissor: RenderCommand
{
	NwRectangle64		rect;//!< clipping rectangle
};



/// CMD_SET_RENDER_STATE
struct Cmd_SetRenderState: RenderCommand, NwRenderState32
{
};
ASSERT_SIZEOF(Cmd_SetRenderState, sizeof(U64));


///
typedef void DrawcallCallback( void* userData );


/// contains all geometry info so that it can be cached in a mesh object
struct Cmd_Draw: RenderCommand
{
	/// The number of instances to draw; default = 0
	//U16		instance_count;

	U32		base_vertex;	//!< index of the first vertex
	U32		vertex_count;	//!< number of vertices

	// Ignored for non-indexed draw:
	U32		start_index;	//!< offset of the first index
	U32		index_count;	//!< number of indices per instance

	//
	HProgram	program;	// 2

	//
	HInputLayout	input_layout;		// 1

	//
	U8	primitive_topology : 7;	// NwTopology8
	U8	use_32_bit_indices : 1;

	//
	HBuffer			VB;	// 2
	HBuffer			IB;	// 2

	// 28

mxOPTIMIZE("separate from draw cmd?");
#if MX_DEBUG
	char	dbgname[28];
	void (*debug_callback)();
	const char *	src_loc;
#endif

	//32: 64

public:
	mxFORCEINLINE Cmd_Draw()
	{
		RenderCommand::header = CMD_DRAW;

		base_vertex = 0;
		vertex_count = 0;

		start_index = 0;
		index_count = 0;

		program.SetNil();

		input_layout.SetNil();

		VB.SetNil();
		IB.SetNil();

#if MX_DEBUG
		dbgname[0] = 0;
		debug_callback = nil;
		src_loc = nil;
#endif
	}

	/// Sets the number of instances to draw; default = 0.
	void SetInstanceCount(
		const U16 instance_count
		)
	{
		RenderCommand::encode(
			NGpu::CMD_DRAW
			, 0
			, instance_count
			);
	}

	void SetMeshState(
		const HInputLayout input_layout
		, const HBuffer vertex_buffer_handle
		, const HBuffer index_buffer_handle
		, const NwTopology::Enum topology
		, const bool use_32_bit_indices
		)
	{
		this->input_layout = input_layout;

		this->primitive_topology = topology;
		this->use_32_bit_indices = use_32_bit_indices;

		this->VB = vertex_buffer_handle;
		this->IB = index_buffer_handle;
	}

	bool IsValid() const
	{
		return program.IsValid();
	}

#if MX_DEBUG || MX_DEVELOPER

	friend ATextStream & operator << ( ATextStream & log, const Cmd_Draw& o );

	static void dbg_callback_default_impl();
#endif

};


///
/// Compute Shader
///
/// NOTE: texture LOD is not defined in compute shaders
struct Cmd_DispatchCS: RenderCommand
{
	U16	groupsX;	//!< The number of groups dispatched in the X direction.
	U16	groupsY;	//!< The number of groups dispatched in the Y direction.
	U16	groupsZ;	//!< The number of groups dispatched in the Z direction.

	HProgram	program;	//!< The Compute shader to bind

	IF_DEBUG DrawcallCallback *	debug_callback;
	IF_DEBUG void *				debug_userData;

public:
	Cmd_DispatchCS()
	{
		this->clear();
	}

	void clear();

	bool IsValid() const
	{
		return program.IsValid()
			&& groupsX > 0
			&& groupsY > 0
			&& groupsZ > 0
			;
	}
};

struct Cmd_NOP: RenderCommand
{
	Memory	dbgmsg;
	U32		bytes_to_skip_after_this_command;
};

// Debugging and performance monitoring tools.

struct Cmd_SetMarker: RenderCommand
{
	RGBAi			rgba;
	const char *	text;
	U32				skip;	//!< non-zero if the text was allocated within the command buffer
};

struct Cmd_PushMarker: RenderCommand
{
	U32				rgba;
	const char *	text;
	U32				skip;	//!< non-zero if the text was allocated within the command buffer
};
struct Cmd_PopMarker: RenderCommand
{
};

struct Cmd_DbgPrint: RenderCommand
{
	U32				tag;	//!< for toggling breakpoints
	U32				skip;	//!< non-zero if the text was allocated within the command buffer
	const char *	text;		
};
#pragma pack (pop)





///
/// List of graphics commands to execute.
///
class CommandBuffer: NonCopyable
{
	char *		_start;		//!< starting address of the command buffer
	U32			_cursor;	//!< current write cursor, always increasing and, at least, 4-byte aligned
	U32			_allocated;	//!< size of allocated memory (capacity)
//		U32			_highWaterAllocated;	//!< max used on any frame
	AllocatorI &		_allocator;
public:
	CommandBuffer( AllocatorI & allocator );
	~CommandBuffer();

	ERet reserve( U32 command_buffer_size );
	void releaseMemory();

	void reset();

	/// Marks the beginning of a series of commands.
	/// returns the current 'Put' pointer
	mxFORCEINLINE U32	currentOffset() const { return _cursor; }

	mxFORCEINLINE bool	IsEmpty() const { return 0 == _cursor; }

public:	// Writing
	enum { MINIMUM_ALIGNMENT = 4 };

	static mxFORCEINLINE bool isValidAlignment( const U32 x )
	{
		return x >= MINIMUM_ALIGNMENT && IsPowerOfTwo( x );
	}
	static mxFORCEINLINE bool IsProperlyAligned( const void* ptr )
	{
		return IS_ALIGNED_BY( ptr, MINIMUM_ALIGNMENT );
	}
	template< class TYPE > static mxFORCEINLINE U32 alignmentForType()
	{
		return largest( mxALIGNOF(TYPE), MINIMUM_ALIGNMENT );
	}

	mxFORCEINLINE bool IsProperlyAligned( U32 alignment = MINIMUM_ALIGNMENT ) const
	{
		return IS_ALIGNED_BY( this->getCurrent(), alignment );
	}

	/// returns a pointer to the space allocated in the command buffer
	ERet AllocateSpace(
		void **memory_,
		const U32 size,
		const U32 align_mask = 3
	);

	/// use this only if you know that there is enough space
	void* AllocateSpace_Unsafe(
		const U32 size,
		const U32 align_mask = 3
	);

	ERet AllocateHeaderWithAlignedPayload(
		void **header_ptr_,
		const U32 header_size,
		void **aligned_payload_ptr_,
		const U32 aligned_payload_size,
		const U32 payload_align_mask = 3
	);

	/// copies the data into the command buffer
	ERet WriteCopy(
		const void* data,
		const U32 size,
		const U32 align_mask = 3
	);

	mxFORCEINLINE char* getCurrent() const { return _start + _cursor; }
	mxFORCEINLINE char* getStart() const { return _start; }
	mxFORCEINLINE char* getEnd() const { return _start + _allocated; }

public:

	///
	template< typename TYPE >
	ERet Allocate( TYPE **o_ )
	{
		return this->AllocateSpace(
			(void**) o_,
			sizeof(TYPE),
			alignmentForType<TYPE>()  - 1
		);
	}

	mxTODO("TYPE should be bitwise-copyable! get rid of NamedObject base")
	template< typename TYPE >	// where TYPE: RenderCommand
	ERet put( const TYPE& o )
	{
		void *	allocated_command;
		mxTRY(this->AllocateSpace(
			&allocated_command
			, sizeof(TYPE)
			, alignmentForType<TYPE>() - 1
			));

		// NOTE: init with default ctor to prevent crashes during assignment,
		// e.g. NamedObject::operator = () cannot write to uninitialized memory
		new(allocated_command) TYPE(o);

		return ALL_OK;
	}

	///// adjust current pointer to N-byte aligned boundary
	//mxFORCEINLINE void AlignTo( U32 _alignment )
	//{
	//	const U32 mask = _alignment - 1;
	//	const U32 alignedOffset = (m_used + mask) & ~mask;
	//	m_used = alignedOffset;
	//}
public:
	static ERet DbgEnsureHandlesAreValid(
		const void* command_buffer_start
		, const U32 command_buffer_size
		);
	static ERet DbgPrint(
		const void* command_buffer_start
		, const U32 command_buffer_size
		, const char* caller_func = nil
		);
};

}//namespace NGpu





/*
=====================================================================
	COMMAND BUFFER RECORDING
=====================================================================
*/
namespace NGpu {
namespace Commands {

ERet BindCBufferData(
	Vector4 **allocated_push_constants_
	, const U32 push_constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if the handle is nil, a uniform buffer will be allocated from the pool
	, const HBuffer optional_buffer_handle

	// if not nil, the name will be copied into the command buffer
	, const char* dbgmsg = nil
	);

ERet BindCBufferDataCopy(
	const void* push_constants
	, const U32 push_constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if the handle is nil, a uniform buffer will be allocated from the pool
	, const HBuffer optional_buffer_handle

	// if not nil, the name will be copied into the command buffer
	, const char* dbgmsg = nil
	);

ERet BindConstants(
	const void* constants
	, const U32 constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if not nil, the name will be copied into the command buffer
	, const char* dbgmsg = nil
	);

mxFORCEINLINE
ERet SetRenderState(
	const NwRenderState32& render_state
	, CommandBuffer & command_buffer_
	)
{
	NGpu::Cmd_SetRenderState	new_cmd;
	new_cmd.encode( NGpu::CMD_SET_RENDER_STATE, 0 /* slot */, 0 );
	TCopyBase( new_cmd, render_state );

	return command_buffer_.put( new_cmd );
}


mxFORCEINLINE
ERet Draw(
		  const NGpu::Cmd_Draw& draw_command
		  , CommandBuffer & command_buffer_
		  )
{
	mxASSERT(draw_command.IsValid());
	mxASSERT(command_buffer_.IsProperlyAligned());
	return command_buffer_.put( draw_command );
}

}//namespace Commands
}//namespace NGpu





namespace NGpu {

/// a helper class for command buffer recording
class CommandBufferWriter: NonCopyable
{
public:
	CommandBuffer &	_command_buffer;

public:
	mxFORCEINLINE CommandBufferWriter( CommandBuffer & command_buffer )
		: _command_buffer( command_buffer )
	{}

	///
	ERet WriteCopy( const void* data, const U32 size );

	ERet setResource( const UINT input_slot, HShaderInput handle );
	ERet setSampler( const UINT input_slot, HSamplerState handle );
	ERet SetCBuffer( const UINT input_slot, HBuffer buffer_handle );
	ERet setUav( const UINT input_slot, HShaderOutput handle );

	///
	ERet UpdateBuffer(
		const HBuffer buffer_handle

		, const void* src_data
		, const U32 src_data_size

		, AllocatorI* deallocator

		// if not nil, the name will be copied into the command buffer
		, const char* dbgname = nil
		);



	template< class ConstantBufferType >
	ERet BindCBufferDataCopy(
		const ConstantBufferType& uniform_data
		, const UINT buffer_slot
		, const HBuffer buffer_handle
		, const char* dbgname = nil
		)
	{
		mxSTATIC_ASSERT2(sizeof(ConstantBufferType) >= 16, Cannot_pass_pointers);
		return Commands::BindCBufferDataCopy(
			&uniform_data
			, sizeof(ConstantBufferType)
			, _command_buffer
			, buffer_slot
			, buffer_handle
			, dbgname
			);
	}

	/// Inserts an 'Update Buffer' command into the command buffer
	/// and stores the data immediately after the command.
	ERet allocateUpdateBufferCommandWithData(
		HBuffer buffer_handle
		, const U32 size
		, void **new_contents_	// the returned memory will be 16-byte aligned
		);

	template< typename BufferType >
	ERet allocateUpdateBufferCommandWithData(
		HBuffer buffer_handle
		, BufferType **o_
		)
	{
		BufferType *	o;
		mxDO(this->allocateUpdateBufferCommandWithData( buffer_handle, sizeof(*o), (void**) &o ));
		*o_ = o;
		return ALL_OK;
	}

	ERet SetRenderTargets(
		const RenderTargets& render_targets
		);

	ERet SetViewport(
		float width,
		float height,
		float top_left_x = 0,
		float top_left_y = 0,
		float min_depth = 0,
		float max_depth = 1
		);

	//
	ERet SetRenderState(
		const NwRenderState32& render_state
		);

	//
	ERet setScissor(
		const bool enabled,
		const NwRectangle64& clipping_rectangle
		);

	///
	ERet Draw( const NGpu::Cmd_Draw& draw_command );

	ERet DispatchCS(
		const HProgram program_handle
		, const U16 num_thread_groups_x
		, const U16 num_thread_groups_y
		, const U16 num_thread_groups_z
		);

	ERet CopyTexture(
		const HTexture dst_texture_handle
		, const HTexture src_texture_handle
		);

	//
	// Debugging and performance monitoring tools.
	//

	ERet SetMarker( const RGBAi& color, const char* format, ... );
	ERet pushMarker( U32 rgba, const char* format, ... );
	ERet popMarker();

	///
	ERet dbgPrintf( U32 tag, const char* format, ... );
};
}//namespace NGpu



/*
=====================================================================
	TESTING & DEBUGGING
=====================================================================
*/

// TODO: move

// so that the programmer can navigate to the source line by clicking on it in the Visual C++ console:
// printf( "%s(%d): %s", file, line, expression );
#define GFX_SRCFILE_STRING	(__FILE__ "(" TO_STR(__LINE__) "): " __FUNCTION__)

#if GFX_CFG_DEBUG_COMMAND_BUFFER
	#define nwDBG_CMD_SRCFILE_STRING	, GFX_SRCFILE_STRING
#else
	#define nwDBG_CMD_SRCFILE_STRING
#endif




namespace NGpu
{
namespace Dbg
{

// Debugging and performance monitoring tools.
class GpuScope
{
	CommandBuffer &	_command_buffer;
public:
	GpuScope( CommandBuffer & command_buffer_, const char* _label, U32 _rgba = ~0 );
	GpuScope( CommandBuffer & command_buffer_, U32 _rgba, const char* formatString, ... );
	~GpuScope();
};

}//namespace Dbg
}//namespace NGpu


#define GFX_SCOPED_MARKER( COMMAND_BUFFER, RGBA32_COLOR )\
	NGpu::Dbg::GpuScope	perfScope##__LINE__( COMMAND_BUFFER, __FUNCTION__, RGBA32_COLOR )

