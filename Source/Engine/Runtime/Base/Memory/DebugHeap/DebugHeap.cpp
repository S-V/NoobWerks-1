/*
=============================================================================
	File:	DebugHeap.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop

#include <ig-debugheap/DebugHeap.h>
#pragma comment( lib, "DebugHeap.lib" )

#include <Base/Base.h>
#include <Base/Memory/DebugHeap/DebugHeap.h>

IGDebugHeap::IGDebugHeap()
{
}

void IGDebugHeap::Initialize( size_t debugHeapSize )
{
	pimpl = DebugHeapInit( debugHeapSize );
	CS.Initialize();
}

void IGDebugHeap::Shutdown()
{
	DebugHeapDestroy( pimpl );
	pimpl = NULL;
	CS.Shutdown();
}

void* IGDebugHeap::Allocate( U32 numBytes, U32 alignment )
{
	mxASSERT(numBytes > 0);
	SpinWait::Lock	scopedLock( CS );
	void* result = DebugHeapAllocate( pimpl, (size_t)numBytes, alignment );
	if( !result ) {
		DBGOUT("Achtung! Out of memory - failed to alloc %u bytes", numBytes);
	}
	return result;
}

void IGDebugHeap::Deallocate( const void* memory )
{
	if( memory ) {
		SpinWait::Lock	scopedLock( CS );
		DebugHeapFree( pimpl, (void*)memory );
	}
}

U32	IGDebugHeap::GetUsableSize( const void* memory ) const
{
	SpinWait::Lock	scopedLock( CS );
	return (U32)DebugHeapGetAllocSize( pimpl, (void*)memory );
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
