/*
=============================================================================
	File:	ScratchAllocator.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <inttypes.h>
#include <Base/Base.h>
#include <Base/Memory/ScratchAllocator.h>
#include <GlobalEngineConfig.h>


/// log allocs and frees? VERY SLOW!!!
#define DBG_LOG_MEM	(0)
#define DBG_MIN_LOGGED_SIZE	(1048580)
#define DBG_MAX_LOGGED_SIZE	(1048580)


namespace
{
	/// Header stored in front of a memory allocation
	/// to indicate the size of the allocated data.
	struct MemBlockHeader
	{
		U32 size: 31;

		/// if this bit is set, this memory block is free
		U32 is_free: 1;
		//... can be padded with HEADER_PADDING for alignment ...
	};
	ASSERT_SIZEOF(MemBlockHeader, 4);

	/// If we need to align the memory allocation we pad the header with this
	/// value after storing the size. That way we can ...
	static const U32 HEADER_PADDING = ~0;
	/// ...given a pointer to the data, returns a pointer to the header before it.
	static inline
	MemBlockHeader* GetHeader( const void *data )
	{

#if 0

		U32 *p = (U32*) data;
		while( p[-1] == HEADER_PADDING ) {
			--p;
		}
		return ((MemBlockHeader*) p) - 1;

#else 1	// more optimized:

		U32 *p = (U32*) data;
		while( *--p == HEADER_PADDING )
			;
		return (MemBlockHeader*) p;

#endif

	}

	/// Given a pointer to the header, return a pointer to the data that follows it.
	static inline
	char* GetDataPtrAfterHeader( MemBlockHeader *header, U32 align )
	{
		void *p = header + 1;
		return (char*) memory::align_forward( p, align );
	}

	/// Stores the size in the header and pads with HEADER_PADDING up to the data pointer.
	static inline
	void FillHeader( MemBlockHeader *header, void *data, U32 size )
	{
		header->size = size;
		header->is_free = false;

		U32 *p = (U32*) (header + 1);
		while( p < data ) {
			*p++ = HEADER_PADDING;
		}
	}

}//namespace


ScratchAllocator::ScratchAllocator( AllocatorI* backing_allocator )
	: _backing_allocator( backing_allocator )
{
	this->clear();
}

ScratchAllocator::~ScratchAllocator()
{
	mxASSERT2(_free_tail == _alloc_head, "_free_tail:[%u] != _alloc_head:[%u]", _free_tail-_buffer_start, _alloc_head-_buffer_start);
	mxASSERT(0 == m_used);
}

void ScratchAllocator::clear()
{
	_alloc_head = nil;
	_free_tail = nil;

	_buffer_start = nil;
	_buffer_end = nil;

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	m_heapId = ~0;
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

	m_used = 0;
}

void ScratchAllocator::initialize(
								  void* memory, U32 size
								  , const char* name /*= nil*/
								  )
{
	_buffer_start = (char*) memory;
	_buffer_end = _buffer_start + size;
	_alloc_head = _buffer_start;
	_free_tail = _buffer_start;

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	m_heapId = MemTrace::HeapCreate( name );
	MemTrace::HeapAddCore( m_heapId, _buffer_start, size );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

	m_used = 0;
}

void ScratchAllocator::shutdown()
{
	mxASSERT2(_free_tail == _alloc_head, "_free_tail:[%u] != _alloc_head:[%u]", _free_tail-_buffer_start, _alloc_head-_buffer_start);
	mxASSERT(0 == m_used);

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::HeapRemoveCore( m_heapId, _buffer_start, _buffer_end-_buffer_start );
	MemTrace::HeapDestroy( m_heapId );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

	this->clear();
}

void* ScratchAllocator::Allocate( U32 size, U32 align )
{
	const U32 capacity = _buffer_end - _buffer_start;
	if( size < capacity )
	{
		void* mem = this->AllocateSpaceInRingBuffer( size, align );
		if( mem )
		{
#if DBG_LOG_MEM
			if(size >= DBG_MIN_LOGGED_SIZE && size <= DBG_MAX_LOGGED_SIZE)
			{
				DBGOUT("[MEM] Alloced %u bytes -> 0x%"PRIXPTR" ( align: %u)",
					size, mem, align
					);
			}
#endif
			return mem;
		}
		// If the buffer is exhausted, use the backing allocator instead.
//		mxASSERT2( mem, "Couldn't allocate %u bytes (%u used), calling backup", size, m_used );
		//IF_DEBUG static bool dump_once = true;
		//IF_DEBUG if( dump_once ) { dumpFragmentation(); dump_once = false; }
#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::UserMark( "Couldn't allocate %u bytes (%u used), calling backup", size, m_used );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE
	} else {
#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::UserMark( "Using backup allocator for large alloc (%u bytes; %u used)", size, m_used );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE
	}

	if( _backing_allocator ) {
		return _backing_allocator->Allocate( size, align );
	}

	return nil;
}

void* ScratchAllocator::AllocateSpaceInRingBuffer( U32 size, U32 align )
{
	mxASSERT(align % MINIMUM_ALIGNMENT == 0);
	size = TAlignUp< MINIMUM_ALIGNMENT >(size);

	/// Store header in front of allocation
	MemBlockHeader *h = (MemBlockHeader*) _alloc_head;
	/// get a pointer to the usable allocated data
	char *data = GetDataPtrAfterHeader( h, align );
	char *p = data + size;

	// Reached the end of the buffer, wrap around to the beginning.
	if( p >= _buffer_end )
	{
		// mark the old block as free so that it will be released
		h->is_free = true;
		h->size = _buffer_end - (char*)h;

		//
		p = _buffer_start;
		h = (MemBlockHeader*) p;
		data = GetDataPtrAfterHeader( h, align );
		p = data + size;
	}

	if( !IsAddressInUse( p ) )
	{
		const U32 allocated_size = p - (char*)h;
		FillHeader( h, data, allocated_size );
		_alloc_head = p;
#if ENGINE_CONFIG_USE_IG_MEMTRACE
		MemTrace::HeapAllocate( m_heapId, h, allocated_size );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE
		m_used += allocated_size;
		return data;
	}

	return nil;
}

void ScratchAllocator::Deallocate( const void *p )
{
	if( p )
	{
		if(this->IsAllocatedFromRingBuffer(p))
		{
			// Mark this slot as free

			MemBlockHeader* h = GetHeader( p );

			const size_t alloc_size = h->size;
			const U32 offset_from_header_to_mem = mxGetByteOffset32(h, p);

#if DBG_LOG_MEM
			if(alloc_size >= DBG_MIN_LOGGED_SIZE && alloc_size <= DBG_MAX_LOGGED_SIZE)
			{
				DBGOUT("[MEM] Freeing 0x%"PRIXPTR" (%u bytes)", p, alloc_size);
			}
#endif

			mxASSERT2(!h->is_free,
				"Double free: 0x%"PRIXPTR", size: %u",
				(uintptr_t)p, alloc_size
				);

			h->is_free = true;

			//
			mxASSERT(m_used > 0);
			m_used -= alloc_size;
			mxASSERT(m_used >= 0);

#if ENGINE_CONFIG_USE_IG_MEMTRACE
			MemTrace::HeapFree( m_heapId, h );
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE


			// Advance the free pointer past all free slots.
			_MoveFreePointerPastFreeBlocks();

		}
		else
		{
			if( _backing_allocator ) {
				_backing_allocator->Deallocate( p );
			} else {
				mxUNREACHABLE;
			}
		}
	}//if ptr is not null
}

void ScratchAllocator::_MoveFreePointerPastFreeBlocks()
{
	while( _free_tail != _alloc_head )
	{
		MemBlockHeader* h = (MemBlockHeader*) _free_tail;

		if( h->is_free )
		{
			// this memory block is free
			_free_tail += h->size;

			// handle wraparound
			if( _free_tail == _buffer_end ) {
				_free_tail = _buffer_start;
			}
		}
		else
		{
			break;	// this memory block is busy/used
		}
	}
}

bool ScratchAllocator::IsAllocatedFromRingBuffer( const void* p )
{
	//return mxPointerInRange( p, _buffer_start, _buffer_end );
	return p >= _buffer_start && p < _buffer_end;
}

bool ScratchAllocator::IsAddressInUse( const void* p )
{
	mxASSERT2( this->IsAllocatedFromRingBuffer(p), "The pointer must come from this allocator!" );
	if( _free_tail == _alloc_head ) {
		return false;	// no allocations
	}
	if( _alloc_head > _free_tail ) {
		return p >= _free_tail && p < _alloc_head;
	}
	return p >= _free_tail || p < _alloc_head;
}

U32 ScratchAllocator::GetUsableSize( const void* p ) const
{
	MemBlockHeader *h = GetHeader( (void*)p );
	return h->size - mxGetByteOffset32( h, p );
}

U32 ScratchAllocator::total_allocated() const {
	return _buffer_end - _buffer_start;
}

struct MemoryRange
{
	char *	start;
	size_t	length;

public:
	void Reset(void* addr)
	{
		start = (char*) addr;
		length = 0;
	}
	
	void IncreaseSizeBy(size_t free_block_size)
	{
		length += free_block_size;
	}

	void* TryAllocate(
		size_t size, size_t alignment
		)
	{
		MemBlockHeader* mem_block_header = (MemBlockHeader*) start;
		char* aligned_data_ptr = GetDataPtrAfterHeader(mem_block_header, alignment);

		return (aligned_data_ptr + size) <= (start + length)
			? aligned_data_ptr
			: nil;
	}
};

void* ScratchAllocator::AllocateMaybeReleasingOldBlocks(
	size_t size
	, size_t alignment
	, ShouldReleaseMemoryBlock_t* should_release_memory_block
	, void* user_ptr
	)
{
	size = TAlignUp< MINIMUM_ALIGNMENT >(size);

	//
	{
		void* mem = this->AllocateSpaceInRingBuffer( size, alignment );
		if( mem )
		{
			return mem;
		}
	}

	// There was not enough room for this allocation.

	// Try to release some old allocations to free up some space in the ring buffer.

	//
	MemoryRange	free_space;
	free_space.Reset(_free_tail);

	//
	char* tail = _free_tail;

	while( tail != _alloc_head )
	{
		MemBlockHeader* h = (MemBlockHeader*) tail;

		const size_t mem_block_size = h->size;

		//
		if( !h->is_free )
		{
			//
			U32* data_ptr = (U32*) (h + 1);
			while( *data_ptr == HEADER_PADDING ) {
				++data_ptr;
			}

			h->is_free |= should_release_memory_block(
				data_ptr, mem_block_size, user_ptr
				);
		}

		if( h->is_free )
		{
			tail += mem_block_size;
			free_space.IncreaseSizeBy(mem_block_size);

			//// check wraparound
			mxASSERT(tail != _buffer_end);
			//if( _free_tail == _buffer_end )
			//{
			//	_free_tail = _buffer_start;
			//	free_space.Reset(_free_tail);
			//}

			//
			void* aligned_data_ptr = free_space.TryAllocate( size, alignment );
			if( aligned_data_ptr )
			{
				MemBlockHeader* h = (MemBlockHeader*) free_space.start;
				FillHeader( h, aligned_data_ptr, free_space.length );
				return aligned_data_ptr;
			}
		}
		else
		{
			free_space.Reset(tail);
			break;	// this memory block is busy/used
		}
	}

	UNDONE;
	return nil;
}

void ScratchAllocator::debugCheck() const
{
	mxASSERT( mxPointerInRange( _alloc_head, _buffer_start, _buffer_end ) );
	mxASSERT( mxPointerInRange( _free_tail, _buffer_start, _buffer_end ) );
	mxASSERT( m_used >= 0 );
}

void ScratchAllocator::dumpFragmentation() const
{
	UNDONE;
	DBGOUT("BEGIN ScratchAllocator::dumpFragmentation:");

	size_t	memFree = 0;
	size_t	memUsed = 0;

	char* curr = _free_tail;

	while( curr != _alloc_head )
	{
		MemBlockHeader* h = (MemBlockHeader*) curr;
		const size_t mem_block_size = h->size;

		if( h->is_free ) {
			memFree += mem_block_size;
			DBGOUT("Free mem block @%p: %u bytes", h, mem_block_size);
		} else {
			memUsed += mem_block_size;
		}
		curr += mem_block_size;
		if( curr > _buffer_end ) {
			curr = _buffer_start;
		}
	}
	DBGOUT("END ScratchAllocator::dumpFragmentation: %u free, %u used", memFree, memUsed);
}
