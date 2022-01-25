/*
=============================================================================
	File:	Memory.cpp
	Desc:	Memory management.
	ToDo:	use several memory heaps;
			on Windows use heaps with low fragmentation feature turned on.
=============================================================================
*/

/// Use Tracey from r-lyeh for tracking memory usage and finding memory leaks?
/// If this is enabled, then debug heap and scratch heap will be disabled.
#define USE_TRACEY	(0)

#if USE_TRACEY
#include <tracey/tracey.hpp>
#pragma comment( lib, "tracey.lib" )
#endif //#if USE_TRACEY

/// Use a separate heap for temporary, short-lived allocations.
#define USE_SCRATCH_HEAP  (!USE_TRACEY && !USE_DEBUG_HEAP)

#if USE_SCRATCH_HEAP && (USE_TRACEY || USE_DEBUG_HEAP)
# error Incompatible options!
#endif

//---------------------------------------------------------------------

#include <intrin.h>		// _Interlocked*
#include <inttypes.h>	// for PRIXPTR

#include <GlobalEngineConfig.h>
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Memory/ScratchAllocator.h>
#include <Base/Object/Reflection/ClassDescriptor.h>

#include <dlmalloc/malloc.c>

#if ENGINE_CONFIG_USE_IG_MEMTRACE
#include <ig-memtrace/MemTraceInit.h>
#include <ig-memtrace/MemTrace.h>
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

#if MX_DEBUG_MEMORY
	// Visual Leak Detector (VLD) Version 2.3.0
	//#include "VLD/vld.h"
//	#pragma comment(lib, "vld.lib")
#endif // MX_DEBUG_MEMORY

#if MX_OVERRIDE_GLOBAL_NEWDELETE && MX_USE_SSE

	mxMSG(Global 'new' and 'delete' should return 16-byte aligned memory blocks for SIMD.)

#endif //!MX_OVERRIDE_GLOBAL_NEWDELETE

//---------------------------------------------------------------------

//mxBEGIN_REFLECT_ENUM( MemoryHeapT )
//#define DECLARE_MEMORY_HEAP( NAME )		mxREFLECT_ENUM_ITEM( NAME, EMemoryHeap::Heap##NAME )
//#include <Base/Memory/MemoryHeaps.inl>
//#undef DECLARE_MEMORY_HEAP
//mxEND_REFLECT_ENUM

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace //Heaps
{

#if USE_TRACEY

  class TraceyHeap: public AllocatorI {
  public:
	  TraceyHeap() {
	  }
	  virtual void* Allocate( U32 _bytes, U32 alignment ) override {
		  mxASSERT(_bytes > 0);
		  void* result = tracey_checked_amalloc( (size_t)_bytes, alignment );
		  if( !result ) {
			  DBGOUT("Achtung! Out of memory - failed to alloc %u bytes", _bytes);
		  }
      DBGOUT("ALLOC(%u|%u) -> 0x%"PRIXPTR, _bytes, alignment, (uintptr_t)result);
		  return result;
	  }
	  virtual void Deallocate( const void* _memory ) override {
		  if( _memory ) {
        DBGOUT("FREEING 0x%"PRIXPTR, _memory);
			  tracey_checked_free( (void*)_memory );
		  }
	  }
	  virtual U32	GetUsableSize( const void* _memory ) const {
		  return SIZE_NOT_TRACKED;
	  }
  };
  static TraceyHeap g_TraceyHeap;

#endif //#if USE_TRACEY


#if 0
class ThreadSafeScratchHeap: public AllocatorI
{
	mutable SpinWait	m_lock;
	ScratchAllocator	m_client;

public:
	ThreadSafeScratchHeap( AllocatorI &_backing, U32 _heapSize, const char* _name = nil )
		: m_client( _backing, _heapSize, _name )
	{
	}
	ERet Initialize()
	{
		m_lock.Initialize();
		return ALL_OK;
	}
	void Shutdown()
	{
		m_lock.Shutdown();
	}
	virtual void* Allocate( U32 _bytes, U32 alignment ) override
	{
		mxASSERT(_bytes > 0);
		PROFILE;
		SpinWait::Lock	scopedLock( m_lock );
		void* result = m_client.Allocate( _bytes, alignment );
		return result;
	}
	virtual void Deallocate( const void* _memory ) override
	{
		PROFILE;
		if( _memory ) {
			SpinWait::Lock	scopedLock( m_lock );
			m_client.Deallocate( (void*)_memory );
		}
	}
	virtual U32	usableSize( const void* _memory ) const
	{
		SpinWait::Lock	scopedLock( m_lock );
		return m_client.allocated_size( _memory );
	}
};
#endif

}//namespace Heaps

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ERet Memory_Initialize()
{
	//const size_t debug_heap_size = Testbed::g_InitMemorySettings.debug_heap_size;
	//if( debug_heap_size )
	//{
	//	mxCONSTRUCT_IN_PLACE( IGDebugHeap );
	//	IGDebugHeap::Get().Initialize( debug_heap_size );
	//	g_heaps.replicate( IGDebugHeap::Ptr() );
	//}

#if USE_TRACEY
	tracey::enable();
	g_heaps.setAll( &g_TraceyHeap );
#endif

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::InitSocket("127.0.0.1", 9811);	// "localhost"
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

	mxInitializeBase();

	return ALL_OK;
}

void Memory_Shutdown()
{
	//DumpStats();

	mxShutdownBase();

	//if( IGDebugHeap::HasInstance() ) {
	//	IGDebugHeap::Get().Shutdown();
	//}

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::Shutdown();
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

#if USE_TRACEY
	tracey::disable();
	tracey::view( tracey::report() );
	fprintf(stdout, "webserver at http://localhost:%d\n", kTraceyWebserverPort );
	fgetc(stdin);
#endif

//	g_heaps.replicate( nil );
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if ENGINE_CONFIG_USE_IG_MEMTRACE

	MemTraceProxyHeap::MemTraceProxyHeap( const char* _name, AllocatorI &_client )
		: m_client( _client )
		//, m_name( _name )
		, m_heapId(-1)
	{
		m_heapId = MemTrace::HeapCreate( _name );
	}
	MemTraceProxyHeap::~MemTraceProxyHeap()
	{
		MemTrace::HeapDestroy( m_heapId );
	}
	void* MemTraceProxyHeap::Allocate( U32 _bytes, U32 alignment )
	{
		void* mem = m_client.Allocate( _bytes, alignment );
		if( mem ) {
			MemTrace::HeapAllocate( m_heapId, mem, _bytes );
		}
		return mem;
	}
	void MemTraceProxyHeap::Deallocate( const void* _memory )
	{
		if( _memory ) {
			MemTrace::HeapFree( m_heapId, _memory );
			m_client.Deallocate( _memory );
		}
	}
	U32	MemTraceProxyHeap::GetUsableSize( const void* _memory ) const
	{
		return m_client.GetUsableSize( _memory );
	}

#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

#if 0
/// An allocator that uses the default system malloc(). Allocations are
/// padded so that we can store the size of each allocation and align them
/// to the desired alignment.
///
/// (Note: An OS-specific allocator that can do alignment and tracks size
/// does need this padding and can thus be more efficient than the
/// MallocAllocator.)
class MallocAllocator: public AllocatorI
{
	U32 _total_allocated;

	// Returns the size to allocate from malloc() for a given size and align.		
	static inline U32 size_with_padding(U32 size, U32 align) {
		return size + align + sizeof(Header);
	}

public:
	MallocAllocator() : _total_allocated(0) {}

	~MallocAllocator() {
		// Check that we don't have any memory leaks when allocator is
		// destroyed.
		mxASSERT(_total_allocated == 0);
	}

	virtual void *allocate(U32 size, U32 align) {
		const U32 ts = size_with_padding(size, align);
		Header *h = (Header *)malloc(ts);
		void *p = data_pointer(h, align);
		fill(h, p, ts);
		_total_allocated += ts;
		return p;
	}

	virtual void deallocate(void *p) {
		if (!p)
			return;

		Header *h = header(p);
		_total_allocated -= h->size;
		free(h);
	}

	virtual U32 allocated_size(void *p) {
		return header(p)->size;
	}

	virtual U32 total_allocated() {
		return _total_allocated;
	}
};
#endif
#if 0
void* Allocate(
			   size_t _bytes,
			   size_t alignment,
			   EMemoryHeap _heap
			   )
{
	static const size_t MAX_ALLOWED_ALLOCATION_SIZE = mxMiB(64);
	mxASSERT(_bytes <= MAX_ALLOWED_ALLOCATION_SIZE);
	mxASSERT(IsPowerOfTwo(alignment));
//	DBGOUT("Allocate(%s): %d bytes", GetTypeOf_MemoryHeapT().FindString(_heap), _bytes);
	AllocatorI & heap = GetHeap(_heap);
	void * mem = heap.Allocate( _bytes, alignment );
	if( !mem ) {
		DBGOUT("ACHTUNG: OUT OF MEMORY while allocating %d", _bytes);
	}
	return mem;
}

void Free( void* _memory, size_t _size, EMemoryHeap _heap )
{
//	DBGOUT("Free: 0x%"PRIXPTR, (uintptr_t)_memory);
	if( _memory )
	{
		const U32 memory_size = GetHeap(_heap).allocated_size(_memory);
		GetHeap(_heap).Deallocate( _memory );
	}
}

void DumpStats()
{
#if TRACK_MEMUSAGE
	ProxyAllocator* current = ProxyAllocator::s_all;
	while( current ) {
		ptPRINT("'%s': now=%ld, peak=%ld, allocs=%ld, frees=%ld",
			current->m_name, current->m_currently_allocated, current->m_peak_memory_usage, current->m_total_allocations, current->m_total_frees);
		current = current->_next;
	}
#endif
}
#endif

namespace Testbed
{
	InitMemorySettings	g_InitMemorySettings = { 0 };
}

NwSetupMemorySystem::NwSetupMemorySystem()
{
	Memory_Initialize();
}

NwSetupMemorySystem::~NwSetupMemorySystem()
{
	Memory_Shutdown();
}

/*
	Useful keywords:
	Region-based memory management

	References:
	http://blog.molecular-matters.com/2011/07/15/memory-system-part-4/

	A Whole new World (Part 1)
	http://jahej.com/alt/2011_04_12_a-whole-new-world-part-1.html

	https://jfdube.wordpress.com/2011/10/22/memory-management-part-3-memory-allocators-design/

	http://blog.molecular-matters.com/2011/08/03/memory-system-part-5/

	http://upcoder.com/15/measure-memory-allocation-cost-by-eliminating-it
*/
