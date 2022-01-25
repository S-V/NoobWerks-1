#include <Base/Base.h>
#pragma hdrstop

#include <bx/string.h>	// bx::vsnprintf()

#include <GPU/Public/graphics_commands.h>
#include <GPU/Private/Frontend/frontend.h>


namespace NGpu
{

//tbPRINT_SIZE_OF(Cmd_Draw);//64
//TIncompleteType< OFFSET_OF(Cmd_Draw, VB) >	VB_offset;	// 6
//TIncompleteType< OFFSET_OF(Cmd_Draw, instance_count) >	instance_count_offset;	// 10

void Cmd_DispatchCS::clear()
{
	groupsX = groupsY = groupsZ = 0;
	program.SetNil();
	IF_DEBUG debug_callback = nil;
	IF_DEBUG debug_userData = nil;
}

//tbPRINT_SIZE_OF(Cmd_SetRenderTargets);

//=================================================================

CommandBuffer::CommandBuffer( AllocatorI & _allocator )
	: _allocator( _allocator )
{
	_start = nil;
	_cursor = 0;
	_allocated = 0;
}

CommandBuffer::~CommandBuffer()
{
	releaseMemory();
}

ERet CommandBuffer::reserve( U32 command_buffer_size )
{
	mxASSERT(_cursor == 0);
	if( command_buffer_size > _allocated )
	{
		void * new_mem = _allocator.Allocate( command_buffer_size, EFFICIENT_ALIGNMENT );
		chkRET_X_IF_NIL( new_mem, ERR_OUT_OF_MEMORY );

		releaseMemory();

		_start = (char*) new_mem;
		_allocated = command_buffer_size;
	}
	return ALL_OK;
}

void CommandBuffer::releaseMemory()
{
	_allocator.Deallocate( _start );
	_start = nil;

	_allocated = 0;
	_cursor = 0;
}

void CommandBuffer::reset()
{
	_cursor = 0;
}

ERet CommandBuffer::AllocateSpace(
	void **memory_,
	const U32 size,
	const U32 align_mask /*= 3*/
)
{
	mxASSERT(isValidAlignment( align_mask + 1 ));

	const U32 current_offset = _cursor;
	const U32 aligned_offset = ( current_offset + align_mask ) & ~align_mask;
	const U32 new_offset = aligned_offset + size;
	//DBGOUT("CmdBuf: allocing %u bytes (curr current_offset=%u)", size, current_offset);
	mxASSERT2( new_offset <= _allocated,
		"Command buffer overflow: requested %u bytes, written so far: %u", size, current_offset );

	if( new_offset <= _allocated ) {
		_cursor = new_offset;
		*memory_ = mxAddByteOffset( _start, aligned_offset );
		return ALL_OK;
	}

	*memory_ = nil;

	// the command buffer should be resized at the end of the current_command frame
	return ERR_BUFFER_TOO_SMALL;
}

void* CommandBuffer::AllocateSpace_Unsafe(
	const U32 size,
	const U32 align_mask /*= 15*/
)
{
	mxASSERT(isValidAlignment( align_mask + 1 ));

	const U32 offset = _cursor;
	const U32 aligned_offset = ( offset + align_mask ) & ~align_mask;
	const U32 new_offset = aligned_offset + size;
	mxASSERT2( new_offset <= _allocated,
		"Command buffer overflow: requested %u bytes, written so far: %u", size, offset );
	_cursor = new_offset;

	return mxAddByteOffset( _start, aligned_offset );
}

ERet CommandBuffer::AllocateHeaderWithAlignedPayload(
	void **header_ptr_,
	const U32 header_size,
	void **aligned_payload_ptr_,
	const U32 aligned_payload_size,
	const U32 payload_align_mask
)
{
	mxASSERT(isValidAlignment( payload_align_mask + 1 ));

	const U32 current_offset = _cursor;

	//
	const U32 header_offset = current_offset;
	const U32 aligned_data_offset = ( header_offset + header_size + payload_align_mask ) & ~payload_align_mask;
	const U32 new_offset = aligned_data_offset + aligned_payload_size;

	mxENSURE(
		new_offset <= _allocated,
		ERR_BUFFER_TOO_SMALL,
		"Command buffer overflow: written so far: %u, allocated: %u", current_offset, _allocated
		);

	_cursor = new_offset;

	*header_ptr_ = mxAddByteOffset( _start, header_offset );
	*aligned_payload_ptr_ = mxAddByteOffset( _start, aligned_data_offset );

	//DBGOUT("hrd_size = %u, payload_size = %u, cursor = %u -> %u"
	//	, header_size, aligned_payload_size
	//	, current_offset, new_offset
	//	);

	return ALL_OK;
}

ERet CommandBuffer::WriteCopy(
	const void* data,
	const U32 size,
	const U32 align_mask /*= 3*/
)
{
	void *	allocated_space;
	mxTRY(this->AllocateSpace( &allocated_space, size, align_mask ));

	memcpy( allocated_space, data, size );

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	CommandBufferWriter
-----------------------------------------------------------------------------
*/

ERet CommandBufferWriter::WriteCopy( const void* data, const U32 size )
{
	return _command_buffer.WriteCopy( data, size );
}

ERet CommandBufferWriter::setResource( const UINT input_slot, HShaderInput handle )
{
	Cmd_BindResource	cmd;
	cmd.encode( CMD_SET_RESOURCE, input_slot, handle.id );
	return _command_buffer.put( cmd );
}

ERet CommandBufferWriter::setSampler( const UINT input_slot, HSamplerState handle )
{
	Cmd_BindResource	cmd;
	cmd.encode( CMD_SET_SAMPLER, input_slot, handle.id );
	return _command_buffer.put( cmd );
}

ERet CommandBufferWriter::SetCBuffer( const UINT input_slot, HBuffer buffer_handle )
{
	Cmd_BindResource	cmd;
	cmd.encode( CMD_SET_CBUFFER, input_slot, buffer_handle.id );
	return _command_buffer.put( cmd );
}

ERet CommandBufferWriter::setUav( const UINT input_slot, HShaderOutput handle )
{
	Cmd_BindResource	cmd;
	cmd.encode( CMD_SET_UAV, input_slot, handle.id );
	return _command_buffer.put( cmd );
}

//tbPRINT_SIZE_OF(NGpu::Cmd_UpdateBuffer);

ERet CommandBufferWriter::UpdateBuffer(
	const HBuffer buffer_handle

	, const void* src_data
	, const U32 src_data_size

	, AllocatorI* deallocator

	, const char* dbgname /*= nil*/
	)
{
	NGpu::Cmd_UpdateBuffer *new_command;
	mxDO(_command_buffer.Allocate(&new_command));

	new_command->encode(
		CMD_UPDATE_BUFFER,
		0,
		buffer_handle.id
		);

	new_command->data = src_data;
	new_command->size = src_data_size;

	new_command->deallocator= deallocator;

	new_command->dbgname = dbgname;

	return ALL_OK;
}



namespace Commands {

ERet BindCBufferData(
	Vector4 **allocated_push_constants_

	, const U32 push_constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if the handle is nil, a uniform buffer will be allocated from the pool
	, const HBuffer optional_buffer_handle

	// if not nil, the name will be copied into the command buffer
	, const char* dbgmsg /*= nil*/
	)
{
	mxASSERT(push_constants_size);
	mxASSERT(push_constants_size <= MAX_UINT16);

	//
	U32	dbg_msg_len_clamped = 0;

#if LLGL_DEBUG_LEVEL > LLGL_DEBUG_LEVEL_NONE
	if( dbgmsg ) {
		const size_t dbg_msg_len = strlen(dbgmsg);
		dbg_msg_len_clamped = smallest( dbg_msg_len, 255 );
	}
#endif

	//
	const U32 aligned_len_of_dbg_msg = dbg_msg_len_clamped
		? tbALIGN4(dbg_msg_len_clamped + 1)
		: 0
		;

	//
	Cmd_BindPushConstants *	cmd;
	void *	allocated_push_constants;

	const U32 header_size
		= sizeof(Cmd_BindPushConstants)
		+ aligned_len_of_dbg_msg
		;

	mxDO(command_buffer_.AllocateHeaderWithAlignedPayload(
		(void**) &cmd
		, header_size
		, &allocated_push_constants
		, push_constants_size
		, LLGL_CBUFFER_ALIGNMENT - 1
	));
	mxASSERT(IS_16_BYTE_ALIGNED(allocated_push_constants));

	//
	cmd->encode(
		CMD_BIND_PUSH_CONSTANTS,
		constant_buffer_slot,
		optional_buffer_handle.id
		);

	//
	cmd->size_of_debug_string_after_this_command = aligned_len_of_dbg_msg;

	//
	const U32 offset_of_push_constants_after_debug_string
		= mxGetByteOffset32(
		((char*) (cmd + 1)) + aligned_len_of_dbg_msg
		, allocated_push_constants
		);
	mxASSERT(offset_of_push_constants_after_debug_string <= 256);

	cmd->offset_of_push_constants_after_debug_string = offset_of_push_constants_after_debug_string;
	cmd->size_of_push_constants = push_constants_size;


#if LLGL_DEBUG_LEVEL > LLGL_DEBUG_LEVEL_NONE
	if( dbgmsg && dbg_msg_len_clamped ) {
		char* dbg_msg = cmd->GetDebugMessageStringIfAny();
		strncpy( dbg_msg, dbgmsg, dbg_msg_len_clamped );
		dbg_msg[ dbg_msg_len_clamped ] = '\0';
	}
#endif


	*allocated_push_constants_ = (Vector4*) allocated_push_constants;

	return ALL_OK;
}

ERet BindCBufferDataCopy(
	const void* push_constants
	, const U32 push_constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if the handle is nil, a uniform buffer will be allocated from the pool
	, const HBuffer optional_buffer_handle

	// if not nil, the name will be copied into the command buffer
	, const char* dbgname /*= nil*/
	)
{
	Vector4* allocated_uniform_data;

	mxDO(NGpu::Commands::BindCBufferData(
	 	&allocated_uniform_data
		, push_constants_size
		, command_buffer_
		, constant_buffer_slot
		, optional_buffer_handle
		, dbgname
		));

	memcpy(allocated_uniform_data, push_constants, push_constants_size);

	return ALL_OK;
}

ERet BindConstants(
	const void* constants
	, const U32 constants_size

	, CommandBuffer & command_buffer_

	, const UINT constant_buffer_slot

	// if not nil, the name will be copied into the command buffer
	, const char* dbgmsg /*= nil*/
	)
{
	mxASSERT(constants_size);

	//
	U32	dbg_msg_len = 0;

#if LLGL_DEBUG_LEVEL > LLGL_DEBUG_LEVEL_NONE
	if( dbgmsg ) {
		dbg_msg_len = strlen(dbgmsg);
	}
#endif

	//
	const U32 aligned_len_of_dbg_msg = dbg_msg_len
		? tbALIGN4(dbg_msg_len + 1)
		: 0
		;

	//
	Cmd_BindConstants *	cmd;
	mxDO(command_buffer_.AllocateSpace(
		(void**) &cmd
		, sizeof(*cmd) + aligned_len_of_dbg_msg
	));

	//
	cmd->encode(
		CMD_BIND_CONSTANTS,
		constant_buffer_slot,
		0
		);

	//
	cmd->data = constants;
	cmd->size = constants_size;
	cmd->size_of_debug_string_after_this_command = aligned_len_of_dbg_msg;
	cmd->deallocator = nil;//TODO

#if LLGL_DEBUG_LEVEL > LLGL_DEBUG_LEVEL_NONE
	if( dbgmsg && dbg_msg_len ) {
		char* dbg_msg = cmd->GetDebugMessageStringIfAny();
		strncpy( dbg_msg, dbgmsg, dbg_msg_len );
		dbg_msg[ dbg_msg_len ] = '\0';
	}
#endif

	return ALL_OK;
}

}//namespace Commands


/// Inserts an 'Update Buffer' command into the command buffer
/// and stores the data immediately after the command.
ERet CommandBufferWriter::allocateUpdateBufferCommandWithData(
	HBuffer buffer_handle
	, const U32 size
	, void **new_contents_	// the returned memory will be 16-byte aligned
	)
{
	GfxCommand_UpdateBuffer_OLD *	command;
	Memory *	mem;

	mxDO(allocateCommandWithAlignedData(
		_command_buffer
		, &command
		, &mem
		, size
		, new_contents_
		, 0 // debug_tag
		));

	//
	command->encode( CMD_UPDATE_BUFFER_OLD, 0, buffer_handle.id );
	command->new_contents = mem;
	command->bytes_to_skip_after_this_command = mxGetByteOffset32(
		command + 1,
		_command_buffer.getCurrent()
	);

	mxASSERT(IS_4_BYTE_ALIGNED(command->bytes_to_skip_after_this_command));

	return ALL_OK;
}

ERet CommandBufferWriter::SetRenderTargets(
	const RenderTargets& render_targets
	)
{
	NGpu::Cmd_SetRenderTargets	new_cmd;
	new_cmd.encode( NGpu::CMD_SET_RENDER_TARGETS, 0 /* slot */, 0 );
	new_cmd.render_targets = render_targets;

	return _command_buffer.put( new_cmd );
}

ERet CommandBufferWriter::SetViewport(
	float width,
	float height,
	float top_left_x,
	float top_left_y,
	float min_depth,
	float max_depth
	)
{
	Cmd_SetViewport	new_cmd;
	new_cmd.encode( NGpu::CMD_SET_VIEWPORT, 0 /* slot */, 0 );

	new_cmd.top_left_x = top_left_x;
	new_cmd.top_left_y = top_left_y;
	
	new_cmd.width = width;
	new_cmd.height = height;
	
	new_cmd.min_depth = min_depth;
	new_cmd.max_depth = max_depth;

	return _command_buffer.put( new_cmd );
}

ERet CommandBufferWriter::SetRenderState(
	const NwRenderState32& render_state
	)
{
	NGpu::Cmd_SetRenderState	new_cmd;
	new_cmd.encode( NGpu::CMD_SET_RENDER_STATE, 0 /* slot */, 0 );
	TCopyBase( new_cmd, render_state );

	return _command_buffer.put( new_cmd );
}

ERet CommandBufferWriter::setScissor(
				const bool enabled,
				const NwRectangle64& clipping_rectangle
				)
{
	NGpu::Cmd_SetScissor	cmd_SetScissor;
	cmd_SetScissor.encode( NGpu::CMD_SET_SCISSOR, 0 /* slot */, enabled );
	cmd_SetScissor.rect	= clipping_rectangle;

	return _command_buffer.put( cmd_SetScissor );
}

ERet CommandBufferWriter::Draw( const NGpu::Cmd_Draw& draw_command )
{
	mxASSERT(_command_buffer.IsProperlyAligned() );
	return _command_buffer.put( draw_command );
}

ERet CommandBufferWriter::DispatchCS(
	const HProgram program_handle
	, const U16 num_thread_groups_x
	, const U16 num_thread_groups_y
	, const U16 num_thread_groups_z
	)
{
	NGpu::Cmd_DispatchCS cmd_dispatch_compute_shader;
	{
		cmd_dispatch_compute_shader.clear();
		cmd_dispatch_compute_shader.encode( NGpu::CMD_DISPATCH_CS, 0, 0 );

		cmd_dispatch_compute_shader.program = program_handle;
		cmd_dispatch_compute_shader.groupsX = num_thread_groups_x;
		cmd_dispatch_compute_shader.groupsY = num_thread_groups_y;
		cmd_dispatch_compute_shader.groupsZ = num_thread_groups_z;
	}
	return _command_buffer.put( cmd_dispatch_compute_shader );
}

ERet CommandBufferWriter::CopyTexture(
	const HTexture dst_texture_handle
	, const HTexture src_texture_handle
	)
{
	Cmd_CopyTexture	cmd;
	cmd.encode( NGpu::CMD_COPY_TEXTURE, 0, 0 );
	cmd.src_texture_handle = src_texture_handle;
	cmd.dst_texture_handle = dst_texture_handle;
	//
	return _command_buffer.put( cmd );
}

ERet CommandBufferWriter::SetMarker( const RGBAi& color, const char* format, ... )
{
	char temp[1024];

	va_list	argPtr;
	va_start( argPtr, format );
	const int len = bx::vsnprintf( temp, mxCOUNT_OF(temp), format, argPtr );
	va_end( argPtr );

	const U32 stringLength = tbALIGN4( len + 1 );	// NUL
	const U32 bytesToAllocate = sizeof(Cmd_SetMarker) + stringLength;

	// Allocate space in the command buffer.
	Cmd_SetMarker *	cmd;
	if( mxSUCCEDED(_command_buffer.AllocateSpace(
		(void**)&cmd
		, bytesToAllocate
		)) )
	{
		char *	destString = (char*) ( cmd + 1 );	// the text string is placed right after the command structure

		// Initialize the command structure.
		cmd->encode( CMD_SET_MARKER, 0, 0 );
		cmd->text = destString;
		cmd->rgba = color;
		cmd->skip = stringLength;

		// Copy the source string into the command buffer.
		memcpy( destString, temp, len );
		destString[ len ] = '\0';
	}

	return ALL_OK;
}

ERet CommandBufferWriter::pushMarker( U32 rgba, const char* format, ... )
{
	char temp[1024];

	va_list	argPtr;
	va_start( argPtr, format );
	const int len = bx::vsnprintf( temp, mxCOUNT_OF(temp), format, argPtr );
	va_end( argPtr );

	const U32 stringLength = tbALIGN4( len + 1 );	// NUL
	const U32 bytesToAllocate = sizeof(Cmd_PushMarker) + stringLength;

	// Allocate space in the command buffer.
	Cmd_PushMarker *	cmd;
	if( mxSUCCEDED(_command_buffer.AllocateSpace(
		(void**)&cmd
		, bytesToAllocate
		, mxALIGNOF(Cmd_PushMarker)-1
		)) )
	{
		char *	destString = (char*) ( cmd + 1 );	// the text string is placed right after the command structure

		// Initialize the command structure.
		cmd->encode( CMD_PUSH_MARKER, 0, 0 );
		cmd->text = destString;
		cmd->rgba = rgba;
		cmd->skip = stringLength;

		// Copy the source string into the command buffer.
		memcpy( destString, temp, len );
		destString[ len ] = '\0';
	}

	return ALL_OK;
}

ERet CommandBufferWriter::popMarker()
{
	Cmd_PopMarker *	cmd_popMarker;
	mxDO(_command_buffer.Allocate( &cmd_popMarker ));

	cmd_popMarker->encode( CMD_POP_MARKER, 0, 0 );

	return ALL_OK;
}

ERet CommandBufferWriter::dbgPrintf( U32 tag, const char* format, ... )
{
	char temp[1024];

	va_list	argPtr;
	va_start( argPtr, format );
	const int len = bx::vsnprintf( temp, mxCOUNT_OF(temp), format, argPtr );
	va_end( argPtr );

	//
	NGpu::Cmd_NOP	*command =
		(NGpu::Cmd_NOP*) ( _command_buffer.getCurrent() )
		;
	mxASSERT(IS_4_BYTE_ALIGNED(command));

	// Allocate space for the command.
	mxDO(_command_buffer.put( Cmd_NOP() ));

	// Fill in the command.
	command->encode(
		CMD_NOP,
		0,
		0
		);

	//
	const U32 string_size = tbALIGN4( len + 1 );

	char * allocated_string;
	mxDO(_command_buffer.AllocateSpace(
		(void**) &allocated_string
		, string_size
		));
	allocated_string[len] = '\0';

	command->dbgmsg.setRelativePointer(
		allocated_string
		, string_size
		);

	memcpy(
		allocated_string,
		temp,
		len
		);

	//
	command->bytes_to_skip_after_this_command = mxGetByteOffset32(
		command + 1,
		allocated_string + string_size
		);

	return ALL_OK;
}

}//namespace NGpu
