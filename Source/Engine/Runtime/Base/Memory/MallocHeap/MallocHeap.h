/*
=============================================================================
	File:	MallocHeap.h
	Desc:	
=============================================================================
*/
#pragma once

/// malloc()
class MallocHeap: public AllocatorI
{
public:
	MallocHeap() {}
	virtual void* Allocate( U32 numBytes, U32 alignment ) override;
	virtual void Deallocate( const void* memory ) override;
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
