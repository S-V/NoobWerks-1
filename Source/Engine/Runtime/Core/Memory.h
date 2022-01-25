#pragma once

#include <Base/Memory/MemoryBase.h>
#include <Base/Memory/ScratchAllocator.h>

#if MX_DEBUG_MEMORY

	struct MemoryAllocatorStats
	{
		AtomicInt	currently_allocated;	// doesn't take alignment into account
		AtomicInt	peak_memory_usage;		// ditto

		AtomicInt	total_allocations;
		AtomicInt	total_frees;

	public:
		MemoryAllocatorStats()
		{
			mxZERO_OUT(*this);
		}

		void onAllocation( U32 bytes_allocated );
		void onDeallocation( U32 bytes_freed );
	};

	/// A proxy allocator that simply draws upon another allocator.
	/// http://bitsquid.blogspot.ru/2010/09/custom-memory-allocation-in-c.html
	struct ProxyAllocator: TrackingHeap, TLinkedList< ProxyAllocator >
	{
		AllocatorI &					_underlying_allocator;
		const char * const		_name;

		ProxyAllocator *		_sibling;
		ProxyAllocator *		_first_child;

		MemoryAllocatorStats	_stats;

	public:
		static ProxyAllocator::Head s_all;

	public:
		ProxyAllocator( const char* name, AllocatorI & tracked_allocator );
		ProxyAllocator( const char* name, ProxyAllocator & parent_allocator );
		virtual ~ProxyAllocator();

		virtual void* Allocate( U32 _bytes, U32 _alignment ) override;
		virtual void Deallocate( const void* _memory ) override;
		virtual U32	GetUsableSize( const void* _memory ) const override;
	};

#else

	typedef TrackingHeap	ProxyAllocator;

#endif


#if 0
class SetupMemoryHeapsUtil: DependsOn_Base
{
	DefaultHeap		defaultHeap;

	ProxyAllocator	globalHeap;
	ProxyAllocator	processHeap;
	ProxyAllocator	clumpsHeap;
	ProxyAllocator	stringsHeap;

public:
	SetupMemoryHeapsUtil();
	~SetupMemoryHeapsUtil();
};

class SetupScratchHeap : SetupMemoryHeapsUtil
{
	ScratchAllocator  scratchHeap;
	ThreadSafeProxyHeap	threadSafeWrapper;
	AllocatorI *	oldScratchHeap;

public:
	SetupScratchHeap( AllocatorI & _allocator, U32 sizeMiB );
	~SetupScratchHeap();
};

#endif


mxREMOVE_THIS
#define TRY_ALLOCATE_SCRACH( _VAR_TYPE, _VARNAME, _ARRAY_COUNT )\
	_VAR_TYPE _VARNAME = (_VAR_TYPE) MemoryHeaps::temporary().Allocate( sizeof(_VARNAME[0]) * _ARRAY_COUNT, mxALIGNOF(_VAR_TYPE) );\
	if( !_VARNAME ) { return ERR_OUT_OF_MEMORY; }\
	AutoFree Scratch##_VARNAME##__LINE__( MemoryHeaps::temporary(), _VARNAME );

//#define mxTRY_ALLOC_SCOPED( _VAR_TYPE, _VARNAME, _ARRAY_COUNT, _ALLOCATOR )\
//	_VAR_TYPE _VARNAME = (_VAR_TYPE) _ALLOCATOR.Allocate( sizeof(_VARNAME[0]) * _ARRAY_COUNT, mxALIGNOF(_VAR_TYPE) );\
//	if( !_VARNAME ) { return ERR_OUT_OF_MEMORY; }\
//	AutoFree __free##_VARNAME##__LINE__( _ALLOCATOR, _VARNAME );
