/*
==========================================================
	File:	FreeList.h
	Desc:	Free lists.
==========================================================
*/
#pragma once

#include <Base/Template/Containers/LinkedList/TLinkedList.h>

/**
----------------------------------------------------------
	FreeList

	Fast allocator for fixed size memory blocks
	based on a non-growing free list.
	NOTE: stored items should reserve a pointer at the start of the structure
		  for the Free List's purposes!
	NOTE: Doesn't own used memory.
	NOTE: No destructor / constructor is called for the type !
----------------------------------------------------------
*/
struct FreeList: CStruct//, NonCopyable
{
	void *	_first_free;//!< pointer to the first free element
	BYTE *	m_start;	//!< pointer to the start of the memory block
	U32		m_size;		//!< total size of the memory block
	//U32	m_liveCount;//!< number of valid elements
public:
	mxDECLARE_CLASS(FreeList,CStruct);
	mxDECLARE_REFLECTION;
	FreeList();
	~FreeList();

	void Initialize( void* memory, int itemSize, int maxItems );
	void clear();

	void* allocItem();
	bool release( void* item );
	void releaseSorted( void* item );	// for address-ordered (iteratable) free lists

	bool ItemInFreeList( const void* item ) const;
	bool hasAddress( const void* ptr ) const;

	void* GetBuffer();
	void* GetFirstFree();
	void** GetFirstFreePtr();
};

inline bool FreeList::hasAddress( const void* ptr ) const
{
	return mxPointerInRange( ptr, m_start, m_size );
}

inline void* FreeList::GetBuffer()
{
	return m_start;
}

inline void* FreeList::GetFirstFree()
{
	return _first_free;
}

inline void** FreeList::GetFirstFreePtr()
{
	return &_first_free;
}

/**
----------------------------------------------------------
	TStaticFreeList< TYPE, MAX_COUNT >
	non-growing free list allocator
----------------------------------------------------------
*/
template< typename TYPE, const UINT MAX_COUNT >
class TStaticFreeList: NonCopyable
{
	FreeList	m_allocator;
	U32			m_allocated;
	TYPE		m_storage[ MAX_COUNT ];

public:
	TStaticFreeList()
	{
		m_allocator.Initialize( m_storage, sizeof(TYPE), MAX_COUNT );
		m_allocated = 0;
	}
	~TStaticFreeList()
	{
		mxASSERT(m_allocated == 0);
	}
	TYPE* Allocate()
	{
		TYPE* o = (TYPE*)m_allocator.Allocate();
		if( o ) {
			m_allocated++;
			new(o) TYPE();
		}
		return o;
	}
	void Deallocate( TYPE* o )
	{
		o->~TYPE();
		m_allocator.Deallocate( o );
		m_allocated--;
	}
	U32 NumAllocated() const
	{
		return m_allocated;
	}
};

union UListHead
{
	UListHead *	next;	//!< pointer to next element in a list

#if MX_DEBUG
	/// for inspecting memory in debugger
	char	dbg[64];
#endif // MX_DEBUG

private:
	UListHead() {}
};

namespace FreeListUtil
{
	void AddItemsToFreeList( void** freeListHead, UINT itemSize, void* itemsArray, UINT numItems );

}//namespace FreeListUtil

/**
----------------------------------------------------------
	FreeListAllocator
	Dynamically growing free list block allocator.
	(Uses space inside free blocks to maintain the freelist.)
	No destructor / constructor is called for the type!
	Freshly allocated memory is uninitialized!
----------------------------------------------------------
*/
class FreeListAllocator//: NonCopyable
{
	mxPREALIGN(16) struct SMemoryBlock: public TLinkedList< SMemoryBlock >
	{
		U32		capacity;	//!< max number of elements in this memory block
		U32		m_pad16[ (mxARCH_TYPE == mxARCH_64BIT) ? 1 : 2 ];	// pad to 16 bytes
		// The elements are stored right after this header.
		// The size of this header must be a multiple of 16 bytes.
	};
	mxSTATIC_ASSERT(sizeof(SMemoryBlock) == 16);

	void *				_first_free;//!< pointer to the first free element
	U32					_num_valid_items;	//!< number of 'live' items - just for stats
	U16					_item_stride;	//!< aligned size of a single element, in bytes
	U16					_mem_block_size;	//!< allocation granularity (minimum allocation block size)
	SMemoryBlock::Head	_allocated_blocks_list;	//!< linked list of all allocated blocks (each can hold '_mem_block_size' items)
	AllocatorI *		_custom_allocator;

public:
	FreeListAllocator();
	~FreeListAllocator();

	ERet Initialize(
		UINT item_size
		, UINT item_granularity
		, AllocatorI* custom_allocator = nil
		);
	bool releaseMemory();

	void* AllocateItem();
	void ReleaseItem( void* item );
	UINT NumValidItems() const;

	void* AllocateNewBlock( UINT numItems );

	bool AddItemsToFreeList( void* items, UINT count );

	bool ItemInFreeList( const void* item ) const;
	bool HasItem( const void* item ) const;
	bool hasAddress( const void* ptr ) const;
	bool hasAddress( SMemoryBlock* block, const void* ptr ) const;

	SMemoryBlock::Head GetAllocatedBlocksList() const;
	UINT CalculateAllocatedMemorySize() const;

	void SetGranularity( UINT newBlockSize );

public:
	template< typename TYPE >
	TYPE* New()
	{
		mxASSERT(_item_stride >= sizeof(TYPE));
		void* storage = this->AllocateItem();
		chkRET_NIL_IF_NIL(storage);
		return new(storage) TYPE();
	}

	template< typename TYPE >
	void Delete( TYPE * item )
	{
		mxASSERT_PTR(item);
		mxASSERT(this->HasItem(item));
		item->~TYPE();
		this->ReleaseItem( item );
	}

private:
	void* _allocateMemory( U32 size );
	void _freeMemory( void* ptr, U32 numBytesToFree );

private:PREVENT_COPY(FreeListAllocator);
};

mxFORCEINLINE
UINT FreeListAllocator::NumValidItems() const {
	return _num_valid_items;
}

/**
----------------------------------------------------------
	IndexBasedFreeList

	Fast allocator for fixed size memory blocks
	based on a non-growing free list.

	NOTE: stored items should reserve a pointer at the start of the structure
		  for the Free List's purposes!
	NOTE: Doesn't own used memory.
	NOTE: No destructor / constructor is called for the type !
----------------------------------------------------------
*/
template< typename INDEX >
struct TFreeListIndex
{
	INDEX	next_free;

public:
	static mxFORCEINLINE TFreeListIndex* castToPtr( void* ptr )
	{
		return reinterpret_cast< TFreeListIndex* >( ptr );
	}
};


template< typename INDEX >
void TBuildIndexBasedFreeList(
	void * array_start		// the start of the whole array
	, const U32 array_count	// the size of the array
	, const U32 item_stride
	, const U32 first_index = 0
	, const INDEX null_handle = INDEX(-1)
	)
{
	mxASSERT(array_count > 0);
	mxASSERT2(item_stride >= sizeof(INDEX),
		"items are too small to store freelist indices!"
		);

	typedef TFreeListIndex<INDEX> FreeListIndexT;

	U32 i = first_index;
	U32 current_offset = 0;

	while( i < array_count )
	{
		FreeListIndexT::castToPtr(
			mxAddByteOffset( array_start, current_offset )
		)->next_free = INDEX( i + 1 );

		i++;
		current_offset += item_stride;
	}

	if(current_offset)	// if the list not empty
	{
		// The tail of the free list points to NULL.
		FreeListIndexT::castToPtr(
			mxAddByteOffset( array_start, current_offset - item_stride )
			)->next_free = null_handle;
	}
}

/**
----------------------------------------------------------
	IndexBasedFreeList

	Fast allocator for fixed size memory blocks
	based on a non-growing free list.

	NOTE: stored items should reserve a pointer at the start of the structure
		  for the Free List's purposes!
	NOTE: Doesn't own used memory.
	NOTE: No destructor / constructor is called for the type !
----------------------------------------------------------
*/
struct IndexBasedFreeList: NonCopyable
{
	/// pointer to the start of the memory block
	void *		_data;

	/// index of the first free item (head of the linked list)
	U32			_first_free;

	U32			_capacity;

	const U32	_item_stride;

	//
	typedef U32 IndexT;
	typedef TFreeListIndex<IndexT> FreeListIndexT;
	static const IndexT NIL_INDEX = ~0;

public:
	IndexBasedFreeList( U32	item_stride )
		: _item_stride( item_stride )
	{
	}

	~IndexBasedFreeList()
	{
		//mxASSERT2( _count == 0, "Some handles haven't been released!" );
	}

	void setupWithMemoryBuffer( void* memory, int max_count )
	{
		mxASSERT_PTR(memory);
		mxASSERT(max_count > 0);

		_data		= memory;
		_first_free	= 0;
		_capacity	= max_count;

		TBuildIndexBasedFreeList( memory, max_count, _item_stride, 0, NIL_INDEX );
	}

	void empty()
	{
		_data		= 0;
		_first_free	= 0;
		_capacity	= 0;
	}

	mxFORCEINLINE IndexT getFreeIndex()
	{
		const IndexT free_index = _first_free;
		mxASSERT( free_index != NIL_INDEX );

		const FreeListIndexT* taken_index = this->getFreeListIndexPtr( _first_free );
		_first_free = taken_index->next_free;

		return free_index;
	}

	/// Returns the item to the free list.
	mxFORCEINLINE void ReleaseItemAtIndex( const IndexT index )
	{
		FreeListIndexT *index_to_release = this->getFreeListIndexPtr( index );
		index_to_release->next_free = _first_free;

		_first_free = index;
	}

private:
	mxFORCEINLINE FreeListIndexT* getFreeListIndexPtr( const IndexT index )
	{
		return FreeListIndexT::castToPtr(
			mxAddByteOffset( _data, index * _item_stride )
			);
	}
};

/// Can be used as a base class for free-listed objects.
/// NOTE: Should be declared as the first base class
/// if it is used inside a FreeList* class!
struct FreeListElement
{
	void *	__nextfree;	// [internal] for linking into a free list
};
