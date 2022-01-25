//
#pragma once


class NwHandleTable: NonCopyable
{
public:
	union HandleSlot
	{
		struct {
			/// index into the 'dense' array of objects;
			/// index of the next free slot if this handle is free
			U32	item_index: 16;	// max 2^16 objects

			/// for identifying invalid handles
			U32	item_version: 16;
		};
		U32	u;
	};
	ASSERT_SIZEOF(HandleSlot, 4);

	HandleSlot *	_handle_slots;	// 'sparse' array of entries
	U32				_first_free_slot;
	U32				_num_alive_items;
	U32				_max_item_count;
	AllocatorI &	_allocator;

public:
	NwHandleTable(AllocatorI& allocator)
		: _handle_slots(nil)
		, _first_free_slot(0)
		, _num_alive_items(0)
		, _max_item_count(0)
		, _allocator(allocator)
	{
	}

	ERet InitWithMaxCount(U32 max_count)
	{
		mxASSERT(!_handle_slots);
		mxDO(nwAllocArray(_handle_slots, max_count, _allocator));

		_max_item_count = max_count;

		ResetState();

		return ALL_OK;
	}

	void Shutdown()
	{
		mxDELETE_AND_NIL(_handle_slots, _allocator);
		_num_alive_items = 0;
		_max_item_count = 0;
	}

	void ResetState()
	{
		// build a free list
		for(UINT i = 0; i < _max_item_count; i++) {
			_handle_slots[i].item_index = i + 1;
			_handle_slots[i].item_version = 0;
		}
		_first_free_slot = 0;

		_num_alive_items = 0;
	}

	mxFORCEINLINE U32 GetItemIndexByHandle(const U32 handle_value) const
	{
		mxASSERT(IsValidHandle(handle_value));
		return _handle_slots[ ((HandleSlot&)handle_value).item_index ].item_index;
	}

	bool IsValidHandle(const U32 handle_value) const
	{
		HandleSlot	handle_u;
		handle_u.u = handle_value;

		if(handle_u.item_index < _max_item_count)
		{
			const HandleSlot stored_entry = _handle_slots[ handle_u.item_index ];
			return handle_u.item_version == stored_entry.item_version;
		}

		return false;
	}

	template< class ItemType >
	ItemType* SafeGetItemByHandle(
		const U32 handle_value
		, ItemType* items
		) const
	{
		HandleSlot	handle_u;
		handle_u.u = handle_value;

		if(handle_u.item_index < _max_item_count)
		{
			const HandleSlot stored_entry = _handle_slots[ handle_u.item_index ];
			if( handle_u.item_version == stored_entry.item_version )
			{
				mxASSERT(stored_entry.item_index < _num_alive_items);
				return &items[ stored_entry.item_index ];
			}
		}

		return nil;
	}

	ERet AllocHandle(
		U32 &new_handle_
		)
	{
		if(_num_alive_items < _max_item_count)
		{
			const U32 index_of_new_item = _num_alive_items;
			const U32 index_of_new_slot = _first_free_slot;

			HandleSlot & new_item_slot = _handle_slots[ index_of_new_slot ];
			_first_free_slot = new_item_slot.item_index;
			new_item_slot.item_index = index_of_new_item;

			_num_alive_items++;

			((HandleSlot&)new_handle_).item_index = index_of_new_slot;
			((HandleSlot&)new_handle_).item_version = new_item_slot.item_version;

			return ALL_OK;
		}

		return ERR_TOO_MANY_OBJECTS;
	}

	//
	template< class ItemType >	// where ItemType is POD, ItemType has 'my_handle' field
	ERet DestroyHandle(
		const U32 handle_value
		, ItemType* items
		, U32 indices_of_swapped_items_[2]
		)
	{
		const HandleSlot handle_of_removed_item = (HandleSlot&) handle_value;
		mxASSERT(IsValidHandle(handle_value));

		// move the last alive object into the slot being destroyed
		HandleSlot& slot_of_removed_item = _handle_slots[ handle_of_removed_item.item_index ];
		const U32 index_of_item_being_removed = slot_of_removed_item.item_index;
		mxASSERT(index_of_item_being_removed < _num_alive_items);
		ItemType& item_being_removed = items[ index_of_item_being_removed ];

		//
		const U32 index_of_last_alive_item = --_num_alive_items;
		ItemType& last_alive_item = items[ index_of_last_alive_item ];
		const HandleSlot handle_of_last_alive_item = (HandleSlot&) last_alive_item.my_handle.id;
		mxASSERT(IsValidHandle(last_alive_item.my_handle.id));

		HandleSlot& slot_of_last_alive_item = _handle_slots[ handle_of_last_alive_item.item_index ];

		// if we are not deleting the last item:
		if(index_of_item_being_removed != index_of_last_alive_item)
		{
			item_being_removed = last_alive_item;

#if MX_DEBUG && MX_DEVELOPER
		memset(&last_alive_item, mxDBG_FREED_MEMORY_TAG, sizeof(items[0]));
#endif
		}

		// the last slot/handle entry now points at the slot where the old item was stored
		slot_of_last_alive_item.item_index = index_of_item_being_removed;

		// invalidate the slot where the old item was stored
		slot_of_removed_item.item_version++;

		// enqueue the slot into the free list
		slot_of_removed_item.item_index = _first_free_slot;
		_first_free_slot = &slot_of_removed_item - _handle_slots;
		mxASSERT(_first_free_slot == handle_of_removed_item.item_index);

		//
		indices_of_swapped_items_[0] = index_of_item_being_removed;
		indices_of_swapped_items_[1] = index_of_last_alive_item;

		return ALL_OK;
	}

	void DbgPrint(UINT num_slots_to_print) const
	{
		DBGOUT("Handle Mgr: %d alive items", _num_alive_items);

		const UINT safe_num_slots_to_print = smallest(num_slots_to_print, _max_item_count);
		for(UINT i = 0; i < safe_num_slots_to_print; i++)
		{
			const HandleSlot stored_entry = _handle_slots[ i ];

			DEVOUT("Slot[%d]: index=%d, version=%d",
				i, stored_entry.item_index, stored_entry.item_version
				);
		}
	}
};
