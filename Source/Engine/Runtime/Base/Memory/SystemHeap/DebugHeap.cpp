/*
=============================================================================
	File:	DebugHeap.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <ig-debugheap/DebugHeap.h>
#include <Base/Memory/DebugHeap/DebugHeap.h>

namespace
{
	/// wraps Insomniac Games' DebugHeap
	class IGDebugHeap: public AllocatorI, public TSingleInstance<IGDebugHeap>
	{
		TPtr< DebugHeap >	pimpl;
		mutable SpinWait	CS;
	public:
		IGDebugHeap()
		{
		}
		void Initialize( U32 _heapSize )
		{
			pimpl = DebugHeapInit( _heapSize );
			CS.Initialize();
		}
		void Shutdown()
		{
			DebugHeapDestroy( pimpl );
			pimpl = NULL;
			CS.Shutdown();
		}
		virtual void* Allocate( U32 _bytes, U32 alignment ) override
		{
			mxASSERT(_bytes > 0);
			SpinWait::Lock	scopedLock( CS );
			void* result = DebugHeapAllocate( pimpl, (size_t)_bytes, alignment );
			if( !result ) {
				DBGOUT("Achtung! Out of memory - failed to alloc %u bytes", _bytes);
			}
			return result;
		}
		virtual void Deallocate( const void* _memory ) override
		{
			if( _memory ) {
				SpinWait::Lock	scopedLock( CS );
				DebugHeapFree( pimpl, (void*)_memory );
			}
		}
		virtual U32	GetUsableSize( const void* _memory ) const
		{
			SpinWait::Lock	scopedLock( CS );
			return (U32)DebugHeapGetAllocSize( pimpl, (void*)_memory );
		}
	};
}//namespace

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
