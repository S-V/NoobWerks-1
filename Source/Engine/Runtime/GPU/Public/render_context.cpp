#include <Base/Base.h>
#pragma hdrstop
#include <algorithm>	// std::sortkey(), std::stable_sort()
#include <GPU/Public/render_context.h>


namespace NGpu
{

NwRenderContext::NwRenderContext( AllocatorI & _allocator )
	: _sort_items( _allocator )
	, _command_buffer( _allocator )
{
}

ERet NwRenderContext::reserve( U32 num_sort_items, U32 command_buffer_size )
{
	mxDO(_sort_items.reserve( num_sort_items ));
	mxDO(_command_buffer.reserve( command_buffer_size ));
	return ALL_OK;
}

void NwRenderContext::reset()
{
	_sort_items.RemoveAll();
	_command_buffer.reset();
}

//void NwRenderContext::begin()
//{
//	mxASSERT(_sort_items.IsEmpty());
////	mxASSERT(_sequence == 0);
//}
//
//void NwRenderContext::finish()
//{
//	_sort_items.RemoveAll();
//}

void NwRenderContext::Sort()
{
	struct Predicate
	{
		static inline bool Compare( const SortItem& a, const SortItem& b )
		{
			return a.sortkey < b.sortkey;
		}
	};

	std::sort( _sort_items.begin(), _sort_items.end(), Predicate::Compare );
	//std::stable_sort( _sort_items.begin(), _sort_items.end(), Predicate::Compare );
}

ERet NwRenderContext::Submit(
	const U32 first_command_offset,
	const SortKey sort_key,
	const char* debug_linenum /*= nil*/
)
{
	const U32 command_length_in_bytes = _command_buffer.currentOffset() - first_command_offset;

	//
	SortItem	new_command;
	new_command.sortkey = sort_key;
	new_command.start = first_command_offset;
	new_command.size = command_length_in_bytes;
#if GFX_CFG_DEBUG_COMMAND_BUFFER
	new_command.dbgloc = debug_linenum;
#endif

	mxDO(_sort_items.add( new_command ));

	return ALL_OK;
}

ERet NwRenderContext::DbgPrint() const
{
	DBGOUT("!>");

	for( UINT i = 0; i < _sort_items.num(); i++ )
	{
		const SortItem& item = _sort_items._data[i];

#if GFX_CFG_DEBUG_COMMAND_BUFFER
		DEVOUT("!\t Printing draw item %u:", i);
		DEVOUT("%s:\n", item.dbgloc);
#endif

		//
		const void* first_command = mxAddByteOffset( _command_buffer.getStart(), item.start );
		mxASSERT(item.size);

		CommandBuffer::DbgPrint(
			first_command,
			item.size,
			GFX_SRCFILE_STRING
			);
	}
	return ALL_OK;
}

void NwPushCommandsOnExitingScope::DbgPrintRecordedCommands() const
{
	const U32 size_of_recorded_commands
		= _render_context._command_buffer.currentOffset() - _start_command_offset
		;

	const RenderCommand* start_cmd = (RenderCommand*) mxAddByteOffset(
		_render_context._command_buffer.getStart()
		, _start_command_offset
		);

	U32	input_slot, res_handle;
	const ECommandType cmd_type = start_cmd->decode(&input_slot, &res_handle);


	//
	CommandBuffer::DbgPrint(
		start_cmd,
		size_of_recorded_commands,
		_debug_linenum
		);
}

}//namespace NGpu
