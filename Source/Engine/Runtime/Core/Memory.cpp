#include <Core/Core_PCH.h>
#pragma hdrstop
#include <intrin.h>		// _Interlocked*
#include <inttypes.h>	// for PRIXPTR
#include <Core/Core.h>
#include <Core/Memory.h>
#include <Base/Memory/ScratchAllocator.h>
#include <GlobalEngineConfig.h>

#if 0
#define DISABLE_SCRATCH_HEAP  (0)

///
SetupMemoryHeapsUtil::SetupMemoryHeapsUtil()
	: globalHeap( "global", defaultHeap )
	, processHeap( "process", globalHeap )
	, clumpsHeap( "clumps", globalHeap )
	, stringsHeap( "strings", globalHeap )
{
	Memory_Initialize();

	for( int iHeap = 0; iHeap < g_heaps.num(); iHeap++ ) {
		g_heaps[ iHeap ] = &globalHeap;
	}
#if !DISABLE_SCRATCH_HEAP
	g_heaps[ HeapClumps ] = &clumpsHeap;
	g_heaps[ HeapString ] = &stringsHeap;
	//g_heaps[ HeapTemporary ] = &scratchHeap;
	g_heaps[ HeapProcess ] = &processHeap;
#endif
}

SetupMemoryHeapsUtil::~SetupMemoryHeapsUtil()
{
	Memory_Shutdown();
}

SetupScratchHeap::SetupScratchHeap( AllocatorI & _allocator, U32 sizeMiB )
	: scratchHeap( _allocator, mxMiB(sizeMiB), "scratch" )
	, threadSafeWrapper( scratchHeap )
{
	threadSafeWrapper.Initialize();
	oldScratchHeap = g_heaps[ HeapTemporary ];
#if !DISABLE_SCRATCH_HEAP
	g_heaps[ HeapTemporary ] = &threadSafeWrapper;
#endif
}

SetupScratchHeap::~SetupScratchHeap()
{
	g_heaps[ HeapTemporary ] = oldScratchHeap;
	threadSafeWrapper.Shutdown();
}
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if MX_DEBUG_MEMORY

	static
	void AtomicMax( AtomicInt* destination, AtomicInt other_value )
	{
		// interlocked max(a,b);
		// NOTE: creates deadlocks on platforms without priority inversion
		AtomicInt	old_value, new_value;
		do {
			old_value = *destination;
			new_value = largest( old_value, other_value );
		} while( _InterlockedCompareExchange( destination, new_value, old_value ) != old_value );
	}

	void MemoryAllocatorStats::onAllocation( U32 bytes_allocated )
	{
		const AtomicInt	allocated_now = _InterlockedExchangeAdd( &currently_allocated, bytes_allocated );
		AtomicMax( &peak_memory_usage, allocated_now );

		InterlockedIncrement( &total_allocations );
	}

	void MemoryAllocatorStats::onDeallocation( U32 bytes_freed )
	{
		_InterlockedExchangeAdd( &currently_allocated, -bytes_freed );

		InterlockedIncrement( &total_frees );
	}

	/*static*/ ProxyAllocator::Head ProxyAllocator::s_all = nil;

	ProxyAllocator::ProxyAllocator( const char* name, AllocatorI & tracked_allocator )
		: TrackingHeap( name, tracked_allocator )
		, _underlying_allocator( tracked_allocator ), _name( name )
	{
		//mxASSERT_MAIN_THREAD;

		//
		_sibling = nil;
		_first_child = nil;

		//
		_next = s_all;
		s_all = this;
	}

	ProxyAllocator::ProxyAllocator( const char* name, ProxyAllocator & parent_allocator )
		: TrackingHeap( name, parent_allocator )
		, _underlying_allocator( parent_allocator ), _name( name )
	{
		//mxASSERT_MAIN_THREAD;

		//
		_sibling = parent_allocator._first_child;
		parent_allocator._first_child = this;

		_first_child = nil;

		//
		_next = s_all;
		s_all = this;
	}

	ProxyAllocator::~ProxyAllocator()
	{
		mxTEMP mxTODO(:)
#if 0//!RELEASE_BUILD
		mxASSERT2( _stats.currently_allocated == 0 && _stats.total_allocations == _stats.total_frees,
			"Detected memory leaks in '%s' (%d bytes, %d unfreed allocs)!", _name, _stats.currently_allocated, _stats.total_allocations - _stats.total_frees );
#endif // RELEASE_BUILD
	}

	void* ProxyAllocator::Allocate( U32 _bytes, U32 _alignment )
	{
		void * ptr = _underlying_allocator.Allocate( _bytes, _alignment );

		if( ptr )
		{
			//const U32 real_size = _underlying_allocator.usableSize( ptr );
			//mxASSERT( real_size != SIZE_NOT_TRACKED );

			_stats.onAllocation( _bytes );
		}

		return ptr;
	}

	void ProxyAllocator::Deallocate( const void* _memory )
	{
		if( !_memory ) {
			return;
		}

		const U32 num_bytes_to_free = this->GetUsableSize( _memory );
		//mxASSERT( num_bytes_to_free != SIZE_NOT_TRACKED );

		_stats.onDeallocation( num_bytes_to_free );

		_underlying_allocator.Deallocate( _memory );
	}

	U32	ProxyAllocator::GetUsableSize( const void* _memory ) const
	{
		return _underlying_allocator.GetUsableSize( _memory );
	}

#else

	//

#endif // !MX_DEBUG_MEMORY
