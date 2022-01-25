/*
=============================================================================
	File:	FreeListAllocator.cpp
	Desc:
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Memory/FreeList/FreeList.h>


#define DBG_FREE_LIST (MX_DEBUG)

/*
----------------------------------------------------------
	FreeList
----------------------------------------------------------
*/
mxDEFINE_CLASS( FreeList );
mxBEGIN_REFLECTION( FreeList )
	mxMEMBER_FIELD( _first_free ),
	mxMEMBER_FIELD( m_start ),
	mxMEMBER_FIELD( m_size ),
	//mxMEMBER_FIELD( m_end ),
mxEND_REFLECTION

FreeList::FreeList()
{
	this->clear();
}

FreeList::~FreeList()
{
}

void FreeList::Initialize( void* memory, int itemSize, int maxItems )
{
	mxASSERT_PTR(memory);
	mxASSERT(IS_ALIGNED_BY(memory, sizeof(void*)));
	mxASSERT2(itemSize >= sizeof(void*), "stride is too small for pointer-based free list!");
	mxASSERT(maxItems > 0);
	//mxASSERT(m_start == NULL);

	_first_free = memory;
	m_start = (BYTE*)memory;
	m_size = maxItems * itemSize;
	//m_end = m_start + maxItems * itemSize;
	//m_num = maxItems;

	void* p = m_start;
	int count = maxItems;

	// setup a (singly) linked list
	while( --count )
	{
		void* next = mxAddByteOffset( p, itemSize );
		*(void**)p = next;
		p = next;
	}
	*(void**)p = NULL;
	//m_liveCount = 0;
}

void FreeList::clear()
{
	_first_free = NULL;
	m_start = NULL;
	m_size = 0;
	//m_liveCount = 0;
}

void* FreeList::allocItem()
{
	if( !_first_free ) {
		return NULL;
	}
	void* result = _first_free;
	_first_free = *(void**)_first_free;
	//m_liveCount++;
	return result;
}

bool FreeList::release( void* item )
{
	mxASSERT(this->hasAddress(item));
	*(void**)item = _first_free;
	_first_free = item;
	//m_liveCount--;
	return true;
}

/// Frees the given element and inserts it into the sorted free list;
/// use this if the free list should be sorted by increasing addresses.
void FreeList::releaseSorted( void* item )
{
	// iterate over the dead elements in the free list and find the correct position for insertion

	void* prevFree = NULL;
	void* currFree = _first_free;

	while( currFree != NULL )
	{
		mxASSERT2( currFree != item, "The item has already been freed" );

		if( currFree > item ) {
			break;	// item should be inserted between 'previous' and 'current'
		}

		prevFree = currFree;
		currFree = *(void**)currFree;
	}

	if( prevFree != NULL )
	{
		// Now: previous->next == current
		mxASSERT(*(void**)prevFree == currFree);
		*(void**)item = currFree;	// item->next = current
		*(void**)prevFree = item;	// previous->next = item
	}
	else
	{
		*(void**)item = _first_free;// item->next = firstFree
		_first_free = item;
	}

#if DBG_FREE_LIST
	for( void *prevFree = NULL, *currFree = _first_free;
		currFree != NULL;
		currFree = *(void**)currFree )
	{
		mxASSERT2(currFree > prevFree, "The free list should be sorted by increasing object addresses!");
		prevFree = currFree;
	}
#endif
	//m_liveCount--;
}

bool FreeList::ItemInFreeList( const void* item ) const
{
	const void* current = _first_free;
	while(PtrToBool(current))
	{
		mxASSERT(this->hasAddress(current));
		if( current == item )
		{
			return true;	// the item has been marked as free
		}
		current = *(void**) current;
	}
	return false;
}

namespace FreeListUtil
{
	void AddItemsToFreeList( void** freeListHead, UINT itemSize, void* itemsArray, UINT numItems )
	{
		mxASSERT_PTR(freeListHead);
		mxASSERT(itemSize > 0);
		mxASSERT_PTR(itemsArray);
		mxASSERT(numItems > 0);

		void* p = itemsArray;
		while( --numItems )
		{
			void* next = mxAddByteOffset( p, itemSize );
			*(void**)p = next;
			p = next;
		}
		// set the last item's next pointer to _first_free
		*(void**)p = *freeListHead;
		// set _first_free to the first item
		*freeListHead = itemsArray;
	}
}//namespace FreeListUtil

/*
----------------------------------------------------------
	FreeListAllocator
----------------------------------------------------------
*/
enum { MIN_ITEM_SIZE = sizeof(void*) + sizeof(U32) };
enum { MIN_ALIGNMENT = sizeof(void*) };

FreeListAllocator::FreeListAllocator()
{
	_first_free = NULL;
	_num_valid_items = 0;
	_item_stride = 0;
	_mem_block_size = 0;
	_allocated_blocks_list = NULL;
	_custom_allocator = NULL;
}

FreeListAllocator::~FreeListAllocator()
{
	this->releaseMemory();
}

// if 'item_granularity' is 0, then we cannot dynamically allocate new memory blocks
ERet FreeListAllocator::Initialize(
								   UINT item_size
								   , UINT item_granularity
								   , AllocatorI* custom_allocator
								   )
{
	mxENSURE( item_size >= MIN_ITEM_SIZE, ERR_INVALID_PARAMETER, "" );
	mxENSURE( item_granularity > 0, ERR_INVALID_PARAMETER, "" );
	mxENSURE( _allocated_blocks_list == NULL, ERR_INVALID_FUNCTION_CALL, "" );
	_item_stride = item_size;
	_mem_block_size = item_granularity;
	_custom_allocator = custom_allocator;
	return ALL_OK;
}

bool FreeListAllocator::releaseMemory()
{
	SMemoryBlock* current = _allocated_blocks_list;
	while(PtrToBool(current)) {
		SMemoryBlock* next = current->_next;
		this->_freeMemory( current, sizeof(SMemoryBlock) + current->capacity * _item_stride );
		current = next;
	}
	_allocated_blocks_list = NULL;
	_mem_block_size = 0;
	_item_stride = 0;
	_first_free = NULL;
	_num_valid_items = 0;
	return true;
}

void* FreeListAllocator::AllocateItem()
{
	// if no free items available...
	if( !PtrToBool(_first_free) )//&& _mem_block_size )
	{
		// ...Allocate a new memory block and add its items to the free list
		void* newItemsArray = this->AllocateNewBlock( _mem_block_size );
		this->AddItemsToFreeList( newItemsArray, _mem_block_size );
	}
	mxASSERT_PTR(_first_free);
	void* result = _first_free;
	_first_free = *(void**)_first_free;
	++_num_valid_items;

#if DBG_FREE_LIST
	memset(result, mxDBG_UNINITIALIZED_MEMORY_TAG, _item_stride);
#endif

	return result;
}

void FreeListAllocator::ReleaseItem( void* item )
{
	mxASSERT(item && this->HasItem(item));

#if DBG_FREE_LIST
	memset(item, mxDBG_UNINITIALIZED_MEMORY_TAG, _item_stride);
#endif

	*(void**)item = _first_free;
	_first_free = item;

	--_num_valid_items;
}

/// grabs a new memory block of the given size
/// doesn't add all its items to the free list
void* FreeListAllocator::AllocateNewBlock( UINT numItems )
{
	chkRET_NIL_IF_NOT(numItems > 0);
	const UINT blockHeaderSize = sizeof(SMemoryBlock);
	const UINT blockDataSize = _item_stride * numItems;
	void* newBlockStorage = this->_allocateMemory( blockHeaderSize + blockDataSize );
	chkRET_NIL_IF_NIL(newBlockStorage);

	SMemoryBlock* newBlock = new (newBlockStorage) SMemoryBlock();
	newBlock->capacity = numItems;
	newBlock->PrependSelfToList( &_allocated_blocks_list );

	void* blockData = newBlock + 1;

//#if DBG_FREE_LIST
//	MemZero( blockData, blockDataSize );
//#endif

	return blockData;
}

bool FreeListAllocator::AddItemsToFreeList( void* items, UINT count )
{
	chkRET_FALSE_IF_NIL(items);
	chkRET_FALSE_IF_NOT(count > 0);
	// add the new entries to the free list
	FreeListUtil::AddItemsToFreeList( &_first_free, _item_stride, items, count );
	return true;
}

// returns true if the given item is currently free
bool FreeListAllocator::ItemInFreeList( const void* item ) const
{
	const void* current = _first_free;
	while(PtrToBool(current)) {
		if( current == item ) {
			return true;
		}
		current = *(void**) current;
	}
	return false;
}

// returns true if the given item was allocated from my memory blocks
bool FreeListAllocator::HasItem( const void* item ) const
{
	chkRET_FALSE_IF_NIL(item);
	SMemoryBlock* current = _allocated_blocks_list;
	while(PtrToBool(current)) {
		if( this->hasAddress(current, item) ) {
			const U32 byteOffset = mxGetByteOffset32( current + 1, item );
			mxASSERT( byteOffset % _item_stride == 0 );
			return true;
		}
		current = current->_next;
	}
	return false;
}

bool FreeListAllocator::hasAddress( const void* ptr ) const
{
	SMemoryBlock* current = _allocated_blocks_list;
	while(PtrToBool(current)) {
		if( this->hasAddress( current, ptr ) ) {
			return true;
		}
		current = current->_next;
	}
	return false;
}

bool FreeListAllocator::hasAddress( SMemoryBlock* block, const void* ptr ) const
{
	const BYTE* start = (BYTE*) (block + 1);
	const BYTE* end = mxAddByteOffset( start, block->capacity * _item_stride );
	const BYTE* bytePtr = (const BYTE*)ptr;
	return (bytePtr >= start) && (bytePtr < end);
}

FreeListAllocator::SMemoryBlock::Head FreeListAllocator::GetAllocatedBlocksList() const
{
	return _allocated_blocks_list;
}

UINT FreeListAllocator::CalculateAllocatedMemorySize() const
{
	UINT result = 0;
	SMemoryBlock* current = _allocated_blocks_list;
	while(PtrToBool(current)) {
		const UINT blockDataSize = current->capacity * _item_stride;
		result += sizeof(SMemoryBlock) + blockDataSize;
		current = current->_next;
	}
	return result;
}

void FreeListAllocator::SetGranularity( UINT newBlockSize )
{
	chkRET_IF_NOT( newBlockSize > 0 );
	_mem_block_size = newBlockSize;
}

void* FreeListAllocator::_allocateMemory( U32 size )
{
	if( _custom_allocator ) {
		return _custom_allocator->Allocate( size, EFFICIENT_ALIGNMENT );
	}
	return mxALLOC( size, EFFICIENT_ALIGNMENT );
}

void FreeListAllocator::_freeMemory( void* ptr, U32 numBytesToFree )
{
	if( _custom_allocator ) {
		return _custom_allocator->Deallocate( ptr );
	}
	return mxFREE( ptr, numBytesToFree );
}
