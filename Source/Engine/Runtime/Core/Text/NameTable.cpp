/*
=============================================================================
	File:	NameTable.cpp
	Desc:	global string table,
			shared reference-counted strings for saving memory
	Note:	NOT thread-safe!
=============================================================================
*/

#include <Core/Core_PCH.h>
#pragma hdrstop

#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include "NameTable.h"

// Static variables.

size_t  NameID::strnum = 0;
size_t  NameID::strmemory = 0;
NameID::StringPointer  NameID::empty = { 0, 1, 0, 0, 0, 0, 0, 0 };
NameID::StringPointer *NameID::table[HASH_SIZE] = {0};
NameID::ListNode *NameID::pointers[CACHE_SIZE] = {0};
NameID::ListNode *NameID::allocs = 0;
static bool gs_NameTableIsInitialized = false;

void NameID::StaticInitialize() throw()
{
	mxASSERT(!gs_NameTableIsInitialized);
	NameID::empty.next = NULL;
	NameID::empty.refcounter = 1;
	NameID::empty.length = 0;
	NameID::empty.hash = 0;
	mxZERO_OUT(NameID::empty.body);
	gs_NameTableIsInitialized = true;
}

void NameID::StaticShutdown() throw()
{
	mxASSERT(gs_NameTableIsInitialized);
	gs_NameTableIsInitialized = false;
}

void* NameID::Alloc( size_t bytes )
{
	mxASSERT_MAIN_THREAD;
	return MemoryHeaps::strings().Allocate(bytes, STRING_ALIGNMENT);
}

void NameID::Free( void* ptr, size_t bytes )
{
	mxASSERT_MAIN_THREAD;
	MemoryHeaps::strings().Deallocate(ptr);
}

NameID::StringPointer* NameID::alloc_string( U32 length )
{
	mxASSERT_MAIN_THREAD;
	U32 alloc = sizeof( StringPointer ) + length - 3; 
	U32 p = ( alloc - 1 ) / ALLOC_GRAN + 1;

	if( p >= CACHE_SIZE )
	{
		return (StringPointer *)Alloc( alloc );
	}

	alloc = p * ALLOC_GRAN;

	if( pointers[p] == 0 )
	{
		char *data = (char*) Alloc( ALLOC_SIZE );

		strmemory += ALLOC_SIZE;

		ListNode *chunk = (ListNode *)( data );
		chunk->next = allocs;
		allocs = chunk;

		for( size_t i = sizeof( ListNode ); i + alloc <= ALLOC_SIZE; i += alloc )
		{
			ListNode *result = (ListNode *)( data + i );
			result->next = pointers[p]; 
			pointers[p] = result;
		}
	}

	ListNode *result = pointers[p];
	pointers[p] = result->next;
	return (StringPointer *)result;
}

void NameID::release_string( StringPointer *ptr )
{
	mxASSERT_MAIN_THREAD;
	U32 alloc = sizeof( StringPointer ) + ptr->length - 3; 
	U32 p = ( alloc - 1 ) / ALLOC_GRAN + 1;

	if( p >= CACHE_SIZE )
	{   
		Free( ptr, alloc );
		return;
	}

	ListNode *result = (ListNode *)ptr;
	result->next = pointers[p];
	pointers[p] = result;
}

void NameID::clear()
{
	U32 position = pointer->hash % HASH_SIZE;
	StringPointer *entry = table[position];
	StringPointer **prev = &table[position];
	while( entry )
	{
		if( entry == pointer )
		{
			*prev = pointer->next;
			release_string( pointer );

			if( --strnum == 0 )
			{
				while( allocs )
				{
					ListNode *next = allocs->next;
					Free( allocs, 0 );
					allocs = next;
					strmemory -= ALLOC_SIZE;
				}

				for( size_t i = 0; i < CACHE_SIZE; ++i )
				{
					pointers[i] = 0;
				}
			}

			return;
		}
		prev = &entry->next;
		entry = entry->next;
	}
}

AWriter& operator << ( AWriter& file, const NameID& o )
{
	const U16 len = o.size();
	file << len;
	if( len > 0 ) {
		file.Write( o.raw(), len );
	}
	return file;
}

AReader& operator >> ( AReader& file, NameID& o )
{
	U16 len;
	file >> len;

	if( len > 0 )
	{
		char buffer[ NameID::MAX_LENGTH ];
		mxASSERT(len < mxCOUNT_OF(buffer));
		len = smallest(len, mxCOUNT_OF(buffer)-1);

		file.Read( buffer, len );

		buffer[ len ] = 0;

		o = NameID( buffer );
	}
	else
	{
		o = NameID();
	}

	return file;
}

#if 0
// Static variables.
NameID2::Entry NameID2::empty_string;

void NameID2::Static_Initialize( AllocatorI &_heap )
{
	empty_string._next = nullptr;
	empty_string.ref_count = 1;
	empty_string.length = 0;
	empty_string.hash = 0;
	mxZERO_OUT(empty_string.body);
}
void NameID2::Static_Shutdown()
{

}
#endif

/*
immutable const_string<> :
http://conststring.sourceforge.net/

llvm::StringPool
http://llvm.org/docs/doxygen/html/StringPool_8h_source.html
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
