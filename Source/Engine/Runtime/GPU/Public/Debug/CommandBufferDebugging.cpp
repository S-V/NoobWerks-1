#include <Base/Base.h>
#pragma hdrstop

#include <bx/string.h>	// bx::vsnprintf()

#include <GPU/Public/graphics_commands.h>
#include <GPU/Private/Frontend/frontend.h>


namespace NGpu
{



ERet CommandBuffer::DbgEnsureHandlesAreValid(
	const void* command_buffer_start
	, const U32 command_buffer_size
	)
{
	mxASSERT(command_buffer_start != nil && command_buffer_size > 0);


	const char* current_command = (char*) command_buffer_start;
	const char* command_list_end = (char*) mxAddByteOffset( command_buffer_start, command_buffer_size );

	int	i = 0;

	do
	{
		const RenderCommand* command_header = (RenderCommand*) current_command;
		mxASSERT(IS_4_BYTE_ALIGNED(command_header));

		U32					input_slot;
		U32					resource_handle_value;
		const ECommandType	command_type = command_header->decode(
			&input_slot, &resource_handle_value
			);

		//
		String64		temp_string;
		StringStream	temp_string_stream( temp_string );

		LogStream	log(LL_Info);

		//
		switch( command_type )
		{
		case CMD_DRAW:
			{
				const Cmd_Draw& cmd = *(Cmd_Draw*) current_command;
				mxENSURE(cmd.IsValid(), ERR_INVALID_PARAMETER, "");
				current_command += sizeof(Cmd_Draw);
			} break;

		case CMD_DISPATCH_CS:
			{
				const Cmd_DispatchCS& cmd_DispatchCS = *(Cmd_DispatchCS*) current_command;
				mxENSURE(cmd_DispatchCS.IsValid(), ERR_INVALID_PARAMETER, "");
				current_command += sizeof(Cmd_DispatchCS);
			} break;


#pragma region "Set* Commands"

		case CMD_SET_CBUFFER:
			{
				const HBuffer buffer_handle (resource_handle_value);
				mxENSURE(buffer_handle.IsValid(), ERR_INVALID_PARAMETER, "");
				mxASSERT(input_slot < LLGL_MAX_BOUND_UNIFORM_BUFFERS);
				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_RESOURCE :
			{
				const HShaderInput handle (resource_handle_value);
				mxENSURE(handle.IsValid(), ERR_INVALID_PARAMETER, "");
				mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_TEXTURES);
				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_SAMPLER :
			{
				const HSamplerState handle (resource_handle_value);
				mxENSURE(handle.IsValid(), ERR_INVALID_PARAMETER, "");
				mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_SAMPLERS);
				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_UAV :
			{
				const HShaderOutput handle (resource_handle_value);
				mxENSURE(handle.IsValid(), ERR_INVALID_PARAMETER, "");
				mxASSERT(input_slot < LLGL_MAX_BOUND_OUTPUT_BUFFERS);
				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_SCISSOR :
			current_command += sizeof(Cmd_SetScissor);
			break;


		case CMD_SET_RENDER_STATE:
			current_command += sizeof(Cmd_SetRenderState);
			break;

#pragma endregion


#pragma region "Update* commands"

		case CMD_UPDATE_BUFFER_OLD :
			{
				const GfxCommand_UpdateBuffer_OLD* cmd_UpdateBuffer = (GfxCommand_UpdateBuffer_OLD*) current_command;
				const Memory *	new_contents = cmd_UpdateBuffer->new_contents;

				log.PrintF("[%d]: CMD_UPDATE_BUFFER_OLD: size=%d",
					i, new_contents->size
					);

				current_command += sizeof(GfxCommand_UpdateBuffer_OLD) + cmd_UpdateBuffer->bytes_to_skip_after_this_command;
			} break;

		case CMD_BIND_PUSH_CONSTANTS:
			{
				const Cmd_BindPushConstants* cmd = (Cmd_BindPushConstants*) current_command;
				UNDONE;
				//current_command += sizeof(Cmd_BindPushConstants) + cmd->bytes_to_skip_after_this_command;
			} break;

		case CMD_UPDATE_TEXTURE :
			{
				UNDONE;
			} break;

		case CMD_GENERATE_MIPS :
			{
				const Cmd_GenerateMips& cmd_GenerateMips = *(Cmd_GenerateMips*) current_command;
				const HShaderInput handle (resource_handle_value);
				mxENSURE(handle.IsValid(), ERR_INVALID_PARAMETER, "");
				current_command += sizeof(Cmd_GenerateMips);
			} break;

		case CMD_CLEAR_RENDER_TARGETS :
			{
				const Cmd_ClearRenderTargets& cmdClearRenderTargets = *reinterpret_cast< const Cmd_ClearRenderTargets* >( current_command );

				mxENSURE(cmdClearRenderTargets.target_count > 0, ERR_INVALID_PARAMETER, "");
				for(UINT iRT = 0; iRT < cmdClearRenderTargets.target_count; iRT++)
				{
					mxENSURE(cmdClearRenderTargets.color_targets[iRT].IsValid(), ERR_INVALID_PARAMETER, "");
				}

				current_command += sizeof(Cmd_ClearRenderTargets);
			} break;

		case CMD_CLEAR_DEPTH_STENCIL_VIEW :
			{
				const Cmd_ClearDepthStencilView& cmdClearDepthStencilView = *reinterpret_cast< const Cmd_ClearDepthStencilView* >( current_command );
				mxENSURE(cmdClearDepthStencilView.depth_target.IsValid(), ERR_INVALID_PARAMETER, "");
				current_command += sizeof(Cmd_ClearDepthStencilView);
			} break;

#pragma endregion

		case CMD_PUSH_MARKER:
			current_command += sizeof(Cmd_PushMarker);
			break;
		case CMD_POP_MARKER:
			current_command += sizeof(Cmd_PopMarker);
			break;
		case CMD_SET_MARKER:
			current_command += sizeof(Cmd_SetMarker);
			break;

		case CMD_DBG_PRINT:
			{
				const Cmd_DbgPrint& cmd = *(Cmd_DbgPrint*) current_command;
				current_command += sizeof(Cmd_DbgPrint) + cmd.skip;
			} break;

		default:
#if MX_DEBUG
			ptWARN("Unknown command: %d ('%s')",
				command_type, dbg_commandToString(command_type)
				);
#endif // MX_DEBUG
			Unimplemented;
			return ERR_UNEXPECTED_TOKEN;
		}

		++i;
	}
	while( current_command < command_list_end );

	return ALL_OK;
}

ERet CommandBuffer::DbgPrint(
			const void* command_buffer_start
			, const U32 command_buffer_size
			, const char* caller_func
			)
{
	mxENSURE(
		command_buffer_size, ERR_INVALID_PARAMETER,
		"\n=== CommandBuffer::DbgPrint(): empty command buffer!"
		);

#if MX_DEVELOPER || MX_DEBUG

	if( caller_func )
	{
		ptPRINT("%s\n=== CommandBuffer::DbgPrint(): start = 0x%X, size = %d:",
			caller_func, command_buffer_start, command_buffer_size
			);
	}
	else
	{
		DBGOUT("\n=== CommandBuffer::DbgPrint(): start = 0x%X, size = %d:",
			command_buffer_start, command_buffer_size
			);
	}

	mxASSERT(CommandBuffer::IsProperlyAligned( command_buffer_start ));

	const char* current_command = (char*) command_buffer_start;
	const char* command_list_end = (char*) mxAddByteOffset( command_buffer_start, command_buffer_size );

	int	i = 0;

	do
	{
		const RenderCommand* command_header = (RenderCommand*) current_command;
		mxASSERT(IS_4_BYTE_ALIGNED(command_header));

		U32					input_slot;
		U32					resource_handle_value;
		const ECommandType	command_type = command_header->decode(
			&input_slot, &resource_handle_value
			);

		//
		String64		temp_string;
		StringStream	temp_string_stream( temp_string );

		LogStream	log(LL_Info);

		//
		switch( command_type )
		{
		case CMD_BAD:
			mxDBG_UNREACHABLE;
			return ERR_UNEXPECTED_TOKEN;

		case CMD_DRAW:
			{
				const Cmd_Draw& draw_cmd = *(Cmd_Draw*) current_command;

				temp_string_stream << draw_cmd;

				log.PrintF("[%d]: CMD_DRAW: %s",
					i, temp_string.c_str()
					);

				mxASSERT(draw_cmd.IsValid());

				current_command += sizeof(Cmd_Draw);
			} break;

		case CMD_DISPATCH_CS:
			{
				const Cmd_DispatchCS& cmd_DispatchCS = *(Cmd_DispatchCS*) current_command;
				log.PrintF("[%d]: CMD_DISPATCH_CS: groups=(%d, %d, %d)",
					i
					, cmd_DispatchCS.groupsX
					, cmd_DispatchCS.groupsY
					, cmd_DispatchCS.groupsZ
					);
				current_command += sizeof(Cmd_DispatchCS);
			} break;


#pragma region "Set* Commands"

		case CMD_SET_CBUFFER:
			{
				const HBuffer buffer_handle (resource_handle_value);

				log.PrintF("[%d]: CMD_SET_CBUFFER: slot=%d, handle=%d",
					i, input_slot, resource_handle_value
					);

				mxASSERT(input_slot < LLGL_MAX_BOUND_UNIFORM_BUFFERS);

				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_RESOURCE :
			{
				const HShaderInput handle (resource_handle_value);

				log.PrintF("[%d]: CMD_SET_RESOURCE: slot=%d, handle=%d",
					i, input_slot, resource_handle_value
					);

				mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_TEXTURES);

				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_SAMPLER :
			{
				const HSamplerState handle (resource_handle_value);

				log.PrintF("[%d]: CMD_SET_SAMPLER: slot=%d, handle=%d",
					i, input_slot, resource_handle_value
					);

				mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_SAMPLERS);

				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_UAV :
			{
				const HShaderOutput handle (resource_handle_value);

				log.PrintF("[%d]: CMD_SET_UAV: slot=%d, handle=%d",
					i, input_slot, resource_handle_value
					);

				mxASSERT(input_slot < LLGL_MAX_BOUND_OUTPUT_BUFFERS);

				current_command += sizeof(Cmd_BindResource);
			} break;

		case CMD_SET_RENDER_TARGETS:
			{
				log.PrintF("[%d]: CMD_SET_RENDER_TARGETS",
					i
					);
				current_command += sizeof(Cmd_SetRenderTargets);
			} break;

		case CMD_SET_VIEWPORT:
			{
				log.PrintF("[%d]: CMD_SET_VIEWPORT",
					i
					);
				current_command += sizeof(Cmd_SetViewport);
			} break;

		case CMD_SET_SCISSOR :
			{
				const Cmd_SetScissor& cmd_SetScissor = *(Cmd_SetScissor*) current_command;
				const bool scissorRectEnabled = !!resource_handle_value;

				log.PrintF("[%d]: CMD_SET_SCISSOR: scissorRectEnabled=%d",
					i, scissorRectEnabled
					);

				current_command += sizeof(Cmd_SetScissor);
			} break;

		case CMD_SET_RENDER_STATE:
			{
				const Cmd_SetRenderState& cmd = *(Cmd_SetRenderState*) current_command;

				log.PrintF("[%d]: CMD_SET_RENDER_STATE",
					i
					);

				current_command += sizeof(Cmd_SetRenderState);
			} break;

#pragma endregion


#pragma region "Update* commands"

		case CMD_UPDATE_BUFFER_OLD :
			{
				const GfxCommand_UpdateBuffer_OLD* cmd_UpdateBuffer = (GfxCommand_UpdateBuffer_OLD*) current_command;
				const Memory *	new_contents = cmd_UpdateBuffer->new_contents;

				DEVOUT("[%d]: CMD_UPDATE_BUFFER_OLD: size=%d",
					i, new_contents->size
					);

				current_command += sizeof(GfxCommand_UpdateBuffer_OLD) + cmd_UpdateBuffer->bytes_to_skip_after_this_command;
			} break;

		case CMD_BIND_PUSH_CONSTANTS:
			{
				const Cmd_BindPushConstants* cmd = (Cmd_BindPushConstants*) current_command;

				const TSpan<BYTE> push_constants = cmd->GetConstantsData();

				const char* dbgmsg = cmd->GetDebugMessageStringIfAny();

				DEVOUT("[%d]: CMD_BIND_PUSH_CONSTANTS: %s, size=%u, memaddr=0x%p",
					i
					, dbgmsg ? dbgmsg : "<no msg>"
					, cmd->size_of_push_constants
					, (void*) push_constants._data
					);

				current_command = (char*) cmd->GetNextCommandPtr();
			} break;

		case CMD_UPDATE_TEXTURE :
			{
				UNDONE;
			} break;

		case CMD_GENERATE_MIPS :
			{
				const Cmd_GenerateMips& cmd_GenerateMips = *(Cmd_GenerateMips*) current_command;
				const HShaderInput resourceHandle (resource_handle_value);
				log.PrintF("[%d]: CMD_GENERATE_MIPS: resourceHandle=%d",
					i, resourceHandle.id
					);
				current_command += sizeof(Cmd_GenerateMips);
			} break;

		case CMD_CLEAR_RENDER_TARGETS :
			{
				const Cmd_ClearRenderTargets& cmdClearRenderTargets = *reinterpret_cast< const Cmd_ClearRenderTargets* >( current_command );

				log.PrintF("[%d]: CMD_CLEAR_RENDER_TARGETS: target_count=%d",
					i, cmdClearRenderTargets.target_count
					);

				current_command += sizeof(Cmd_ClearRenderTargets);
			} break;

		case CMD_CLEAR_DEPTH_STENCIL_VIEW :
			{
				const Cmd_ClearDepthStencilView& cmdClearDepthStencilView = *reinterpret_cast< const Cmd_ClearDepthStencilView* >( current_command );

				log.PrintF("[%d]: CMD_CLEAR_DEPTH_STENCIL_VIEW: depthValue=%.3f, stencilValue=%d",
					i
					, cmdClearDepthStencilView.depth_clear_value
					, cmdClearDepthStencilView.stencil_clear_value
					);

				current_command += sizeof(Cmd_ClearDepthStencilView);
			} break;

#pragma endregion

		case CMD_PUSH_MARKER :
			{
				const Cmd_PushMarker& cmd_PushMarker = *(Cmd_PushMarker*) current_command;

				log.PrintF("[%d]: CMD_PUSH_MARKER: %s",
					i, cmd_PushMarker.text
					);

				current_command += sizeof(Cmd_PushMarker) + cmd_PushMarker.skip;
			} break;

		case CMD_POP_MARKER :
			{
				log.PrintF("[%d]: CMD_POP_MARKER",
					i
					);

				current_command += sizeof(Cmd_PopMarker);
			} break;

		case CMD_SET_MARKER :
			{
				const Cmd_SetMarker& cmd_SetMarker = *(Cmd_SetMarker*) current_command;

				log.PrintF("[%d]: CMD_SET_MARKER: %s",
					i, cmd_SetMarker.text
					);

				current_command += sizeof(Cmd_SetMarker) + cmd_SetMarker.skip;
			} break;

		default:
#if MX_DEBUG
			ptWARN("Unknown command: %d ('%s')",
				command_type, dbg_commandToString(command_type)
				);
#endif // MX_DEBUG
			Unimplemented;
			return ERR_UNEXPECTED_TOKEN;
		}

		++i;
	}
	while( current_command < command_list_end );

#endif

	return ALL_OK;
}


#if LLGL_DEBUG_LEVEL != LLGL_DEBUG_LEVEL_NONE

const char* dbg_commandToString( const ECommandType command_type )
{
	switch( command_type )
	{
	case CMD_DRAW:	return "CMD_DRAW";

	case CMD_DISPATCH_CS:	return "CMD_DISPATCH_CS";

	// Set* commands
	case CMD_SET_CBUFFER:	return "CMD_SET_CBUFFER";
	case CMD_SET_RESOURCE:	return "CMD_SET_RESOURCE";
	case CMD_SET_SAMPLER:	return "CMD_SET_SAMPLER";
	case CMD_SET_UAV:		return "CMD_SET_UAV";
	case CMD_SET_RENDER_TARGETS:	return "CMD_SET_RENDER_TARGETS";
	case CMD_SET_VIEWPORT:	return "CMD_SET_VIEWPORT";
	case CMD_SET_SCISSOR:	return "CMD_SET_SCISSOR";
	case CMD_SET_RENDER_STATE:	return "CMD_SET_RENDER_STATE";

	// Update* commands
	case CMD_UPDATE_BUFFER_OLD:	return "CMD_UPDATE_BUFFER_OLD";

	case CMD_UPDATE_BUFFER:	return "CMD_UPDATE_BUFFER";

	/// ensure that the uniform buffer has the specified data
	case CMD_BIND_PUSH_CONSTANTS:	return "CMD_BIND_PUSH_CONSTANTS";

	case CMD_UPDATE_TEXTURE:	return "CMD_UPDATE_TEXTURE";

	case CMD_GENERATE_MIPS:	return "CMD_GENERATE_MIPS";

	case CMD_CLEAR_RENDER_TARGETS:		return "CMD_CLEAR_RENDER_TARGETS";
	case CMD_CLEAR_DEPTH_STENCIL_VIEW:	return "CMD_CLEAR_DEPTH_STENCIL_VIEW";

	case CMD_COPY_RESOURCE:	return "CMD_COPY_RESOURCE";


	// Retrieve* commands
	//RC_RESOLVE_TEXTURE,
	//RC_READ_PIXELS,	// e.g. read hit test render target
	case CMD_MAP_READ:	return "CMD_MAP_READ";

	//// Dynamic ('Transient') geometry rendering.
	//CMD_UPDATE_DYNAMIC_VB,
	//CMD_UPDATE_DYNAMIC_IB,
	//CMD_DRAW_DYNAMIC_GEOMETRY,

	// Debugging / Performance tools.
	case CMD_PUSH_MARKER:	return "CMD_PUSH_MARKER";
	case CMD_POP_MARKER:	return "CMD_POP_MARKER";
	case CMD_SET_MARKER:	return "CMD_SET_MARKER";
	case CMD_DBG_PRINT:		return "CMD_DBG_PRINT";

	//// Delete* commands

	// Special commands
	case CMD_NOP:	return "CMD_NOP";
	////RC_BeginFrame,	// begin rendering a new frame, reset internal states/counters
	////RC_END_OF_LIST

	mxNO_SWITCH_DEFAULT;
	}
	return "?";
}

#endif

}//namespace NGpu
