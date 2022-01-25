/*
=============================================================================
	File:	ScratchAllocator.h
	Desc:	A fixed-size circular buffer allocator.
			Assumes that all allocations are short-lived
			and allocated and deallocated in a roughly first-out, first-back-in manner.
=============================================================================
*/
#pragma once

#include <Base/Memory/MemoryBase.h>

mxSTOLEN("Bitsquid foundation library");

/// An allocator used to allocate temporary "scratch" memory. The allocator
/// uses a fixed size ring buffer to services the requests.
///
/// Memory is always always allocated linearly. An allocation pointer is
/// advanced through the buffer as memory is allocated and wraps around at
/// the end of the buffer. Similarly, a free pointer is advanced as memory
/// is freed.
///
/// It is important that the scratch allocator is only used for short-lived
/// memory allocations. A long lived allocator will lock the "free" pointer
/// and prevent the "allocate" pointer from proceeding past it, which means
/// the ring buffer can't be used.
/// 
/// If the ring buffer is exhausted, the scratch allocator will use its backing
/// allocator to allocate memory instead.
class ScratchAllocator: public AllocatorI
{
	/// Pointers to where to allocate memory and where to free memory.
	char *	_alloc_head;	// the next block to allocate
	char *	_free_tail;		// the next block to free up

	/// Start and end of the ring buffer.
	char *	_buffer_start;
	char *	_buffer_end;

	/// an optional backing allocator
	AllocatorI *	_backing_allocator;

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	U32		m_heapId;	//!< MemTrace::HeapId
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

	I32		m_used;		//!< for debugging

public:
	/// Creates a ScratchAllocator. The allocator will use the backing
	/// allocator to create the ring buffer and to service any requests
	/// that don't fit in the ring buffer.
	ScratchAllocator( AllocatorI* backing_allocator = nil );
	virtual ~ScratchAllocator();

	/// 'size' specifies the size of the ring buffer
	void initialize(
		void* memory, U32 size
		, const char* name = nil
		);
	void shutdown();

	char* getBufferStart() const { return _buffer_start; }
	bool isInitialized() const { return _buffer_start != nil; }

	//
	virtual void* Allocate( U32 size, U32 align ) override;
	virtual void Deallocate( const void *p ) override;

	virtual U32 GetUsableSize( const void* p ) const override;
	virtual U32 total_allocated() const override;

	bool IsAllocatedFromRingBuffer( const void* p );
	bool IsAddressInUse( const void* p );

	/// Tries to allocate without using the backing allocator.
	void* AllocateSpaceInRingBuffer( U32 size, U32 align );

public:
	typedef bool ShouldReleaseMemoryBlock_t(
		void* mem, size_t size
		, void* user_ptr
		);

	/// If there is not enough free space for a new allocation,
	/// but existing allocations from the front of the queue can become lost,
	/// they become lost and the allocation succeeds.
	void* AllocateMaybeReleasingOldBlocks(
		size_t size
		, size_t alignment
		, ShouldReleaseMemoryBlock_t* should_release_memory_block
		, void* user_ptr
		);

public:
	void debugCheck() const;
	void dumpFragmentation() const;

private:
	void clear();
	void _MoveFreePointerPastFreeBlocks();
};
