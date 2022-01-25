// Draw command sorting.
#pragma once

#include <GPU/Public/graphics_commands.h>


namespace NGpu
{
	/// user-defined parameters for sorting batches (e.g. depth, material hash/id)
	/// NOTE: the topmost LLGL_NUM_VIEW_BITS must be set to the corresponding LayerID
	typedef U64 SortKey;

	/// Normally, you shouldn't use zero as a sort key for draw calls;
	/// the first command is reserved for setting up render targets/states.
	enum { FIRST_SORT_KEY = 1 };


	/// Render [Context|Queue] or Command [Buffer|List|Queue]:
	/// a (sortable) collection of rendering operations that make up a full frame.
	/// Command chains are queued up and sorted before execution.
	/// Separate command lists can be built in different threads.
	class NwRenderContext: NonCopyable
	{
	public:
		/// a sortable command
		struct SortItem
		{
			SortKey	sortkey;//!< sort key (viewId, user-defined parameters (program, view-space depth, VB/IB, sequence, etc))
			U32		start;	//!< byte offset of the command data in the command buffer
			U32		size;	//!< size of data allocated within the command buffer
#if GFX_CFG_DEBUG_COMMAND_BUFFER
			const char*	dbgloc;
#endif
		};
#if !GFX_CFG_DEBUG_COMMAND_BUFFER
		ASSERT_SIZEOF(SortItem,16);
#endif

		DynamicArray< SortItem >	_sort_items;//!< keys for sorting commands
		CommandBuffer				_command_buffer;

	public:
		NwRenderContext( AllocatorI & _allocator );

		ERet reserve(
			U32 num_sort_items
			, U32 command_buffer_size
			);

		void reset();

		/// Inserts a command chain.
		ERet Submit(
			const U32 first_command_offset,
			const SortKey sort_key,
			const char* debug_linenum = nil
		);

		/// the sorter cannot reorder commands between sequences
		//void nextSequence();

		void Sort();

	public:
		ERet DbgPrint() const;
	};

	///
	class RenderCommandWriter: public CommandBufferWriter
	{
	public_internal:
		NGpu::NwRenderContext &	_render_context;
		const U32	_start_command_offset;

	public:
		mxFORCEINLINE RenderCommandWriter( NGpu::NwRenderContext & render_context )
			: CommandBufferWriter( render_context._command_buffer )
			, _render_context( render_context )
			, _start_command_offset( render_context._command_buffer.currentOffset() )
		{}

		void SubmitCommandsWithSortKey(
			const NGpu::SortKey sort_key
			, const char* debug_linenum = nil
			)
		{
			_render_context.Submit(
				_start_command_offset
				, sort_key
				, debug_linenum
				);
		}

		bool didWriteAnything() const
		{
			return _render_context._command_buffer.currentOffset() > _start_command_offset;
		}
	};


	// Dynamic ('Transient') geometry rendering.


	// CPU-GPU synchronization.

	// A fence is used so that the CPU waits until the GPU has finished.
	// A fence is used to monitor the completion of graphics commands.
//uint32_t create_fence();
//void wait_for_fence(uint32_t fence);
//
//Synchronization methods for making sure the renderer is finished processing up to a certain point. Used to handle blocking calls and to make sure the simulation doesn’t run more than one frame ahead of the renderer.



	// An event is used so that the GPU waits until the CPU has finished.






/*
=====================================================================
	UTILITIES
=====================================================================
*/


///
class NwPushCommandsOnExitingScope
{
public_internal:
	NGpu::NwRenderContext &	_render_context;
	const U32				_start_command_offset;
	const NGpu::SortKey		_sortKey;

	const char* _debug_linenum;

public:
	NwPushCommandsOnExitingScope(
		NGpu::NwRenderContext & render_context
		, const NGpu::SortKey sortKey// = 0
		, const char* debug_linenum = nil
		)
		: _render_context( render_context )
		, _start_command_offset( render_context._command_buffer.currentOffset() )
		, _sortKey( sortKey )
		, _debug_linenum(debug_linenum)
	{
	}

	~NwPushCommandsOnExitingScope()
	{
		mxASSERT(didWriteAnything());
		_render_context.Submit(
			_start_command_offset
			, _sortKey
			, _debug_linenum
			);
	}

	bool didWriteAnything() const
	{
		return _render_context._command_buffer.currentOffset() > _start_command_offset;
	}

	void DbgPrintRecordedCommands() const;
};

}//namespace NGpu






namespace NGpu
{
	/// first sorts by the shader program and then by less important data
	mxFORCEINLINE SortKey buildSortKey( HProgram program, U32 less_important_data )
	{
		return (SortKey(program.id) << 32) | less_important_data;
	}

	//TODO: FIXME: these cannot be inlined, because LLGL_VIEW_SHIFT is private
	SortKey buildSortKey( const LayerID view_id, const SortKey sort_key );

	SortKey buildSortKey( const LayerID view_id, HProgram program, U32 less_important_data );

	//mxFORCEINLINE SortKey buildSortKey( const LayerID view_id, const U16 sequence, HProgram program, U32 least_important_data )
	//{
	//	return (SortKey(view_id) << LLGL_VIEW_SHIFT) | (SortKey(sequence) << 48) | (SortKey(program.id) << 32) | least_important_data;
	//}

}//namespace NGpu
