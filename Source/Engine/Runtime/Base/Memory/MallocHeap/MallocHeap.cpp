/*
=============================================================================
	File:	MallocHeap.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Memory/MallocHeap/MallocHeap.h>

void* MallocHeap::Allocate( U32 numBytes, U32 alignment )
{
	return Sys_Alloc( numBytes, alignment );
}

void MallocHeap::Deallocate( const void* memory )
{
	Sys_Free( (void*) memory );
}
