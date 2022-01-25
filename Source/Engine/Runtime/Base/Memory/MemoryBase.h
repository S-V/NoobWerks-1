/*
=============================================================================
	File:	Memory.h
	Desc:	Dynamic memory management.
	ToDo:	Memory limits and watchdogs, thread-safety,
			allow users to define their own OutOfMemory handlers, etc.
=============================================================================
*/
#pragma once

//-----------------------------------------------------------------------
//		Defines.
//-----------------------------------------------------------------------

// 1 - Track each memory allocation/deallocation, record memory usage statistics and detect memory leaks and so on.
#define MX_DEBUG_MEMORY			MX_DEBUG

// 1 - Redirect the global 'new' and 'delete' to our memory manager.
// this is dangerous, it may be better to define them on per-type basis or use special macros.
#define MX_OVERRIDE_GLOBAL_NEWDELETE	(0)

// 1 - prevent the programmer from using malloc/free.
#define MX_HIDE_MALLOC_AND_FREE		(0)

//-----------------------------------------------------------------------
//		Declarations.
//-----------------------------------------------------------------------

//class TbMetaClass;
//
//// memory heaps/arenas/areas
//// split by size/lifetime
//enum EMemoryHeap
//{
//#define DECLARE_MEMORY_HEAP( NAME )		Heap##NAME
//#include <Base/Memory/MemoryHeaps.inl>
//#undef DECLARE_MEMORY_HEAP
//
//	HeapCount,	//!<= Marker. Don't use.
//};
//mxDECLARE_ENUM( EMemoryHeap, UINT8, MemoryHeapT );

mxSTOLEN("Bitsquid foundation library");
mxTEMP
namespace memory {
	inline void *align_forward(void *p, U32 align);
	inline void *pointer_add(void *p, U32 bytes);
	inline const void *pointer_add(const void *p, U32 bytes);
	inline void *pointer_sub(void *p, U32 bytes);
	inline const void *pointer_sub(const void *p, U32 bytes);
}

// -----------------------------------------------------------
// Inline function implementations
// -----------------------------------------------------------

// Aligns p to the specified alignment by moving it forward if necessary
// and returns the result.
inline void *memory::align_forward(void *p, U32 align)
{
	uintptr_t pi = uintptr_t(p);
	const U32 mod = pi % align;
	if (mod)
		pi += (align - mod);
	return (void *)pi;
}

/// Returns the result of advancing p by the specified number of bytes
inline void *memory::pointer_add(void *p, U32 bytes)
{
	return (void*)((char *)p + bytes);
}

inline const void *memory::pointer_add(const void *p, U32 bytes)
{
	return (const void*)((const char *)p + bytes);
}

/// Returns the result of moving p back by the specified number of bytes
inline void *memory::pointer_sub(void *p, U32 bytes)
{
	return (void*)((char *)p - bytes);
}

inline const void *memory::pointer_sub(const void *p, U32 bytes)
{
	return (const void*)((const char *)p - bytes);
}

ERet Memory_Initialize();
void Memory_Shutdown();	// shutdown and assert on memory leaks

//-----------------------------------------------------------------------
//		HELPER MACROS
//-----------------------------------------------------------------------

#define mxALLOC( size, align )			(Sys_Alloc( (size), (align) ))
#define mxFREE( memory, size )			(Sys_Free( (memory)/*, (size)*/ ))

/// declare our own 'new' and 'delete' so that these can be found via 'find All References'

#if defined(new_one) || defined(free_one)
#	error 'new_one' and 'free_one' have already been  defined - shouldn't happen!
#endif

#define new_one( x )	new x
#define free_one( x )	delete x

// Array operators

#if defined(new_array) || defined(free_array)
#	error 'new_array' and 'free_array' have already been defined - shouldn't happen!
#endif

#define new_array( x, num )		new x [num]
#define free_array( x )			delete[] x

#if MX_OVERRIDE_GLOBAL_NEWDELETE
	#error Incompatible options: overriden global 'new' and 'delete' and per-class memory heaps.
#endif

//
//	mxDECLARE_CLASS_ALLOCATOR
//
//	'className' can be used to track instances of the class.
//
#define mxDECLARE_CLASS_ALLOCATOR( memClass, className )\
public:\
	typedef className THIS_TYPE;	\
	static EMemoryHeap GetHeap() { return memClass; }	\
	mxFORCEINLINE void* operator new      ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete   ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new      ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete   ( void*, void* )		{ }												\
	mxFORCEINLINE void* operator new[]    ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete[] ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new[]    ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete[] ( void*, void* )		{ }												\

//
//	mxDECLARE_VIRTUAL_CLASS_ALLOCATOR
//
#define mxDECLARE_VIRTUAL_CLASS_ALLOCATOR( memClass, className )\
public:\
	typedef className THIS_TYPE;	\
	mxFORCEINLINE void* operator new      ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete   ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new      ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete   ( void*, void* )		{ }												\
	mxFORCEINLINE void* operator new[]    ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete[] ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new[]    ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete[] ( void*, void* )		{ }

//
//	mxDECLARE_NONVIRTUAL_CLASS_ALLOCATOR
//
#define mxDECLARE_NONVIRTUAL_CLASS_ALLOCATOR( memClass, className )\
public:\
	typedef className THIS_TYPE;	\
	mxFORCEINLINE void* operator new      ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete   ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new      ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete   ( void*, void* )		{ }												\
	mxFORCEINLINE void* operator new[]    ( size_t sizeInBytes ){ return mxAllocX( memClass, sizeInBytes ); }	\
	mxFORCEINLINE void  operator delete[] ( void* ptr )			{ mxFreeX( memClass, ptr ); }					\
	mxFORCEINLINE void* operator new[]    ( size_t, void* ptr )	{ return ptr; }									\
	mxFORCEINLINE void  operator delete[] ( void*, void* )		{ }

//-----------------------------------------------------------------------
//		Prevent usage of plain old 'malloc' & 'free' if needed.
//-----------------------------------------------------------------------

#if MX_HIDE_MALLOC_AND_FREE

	#define malloc( size )			ptBREAK
	#define free( mem )				ptBREAK
	#define calloc( num, size )		ptBREAK
	#define realloc( mem, newsize )	ptBREAK

#endif // MX_HIDE_MALLOC_AND_FREE

//--------------------------------------------------------------------
//	Useful macros
//--------------------------------------------------------------------

#ifndef SAFE_DELETE
#define SAFE_DELETE( p )		{ if( p != nil ) { delete (p);     (p) = nil; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY( p )	{ if( p != nil ) { delete[] (p);   (p) = nil; } }
#endif

#define mxNULLIFY_POINTER(p)	{(p) = nil;}

/*
template<vm::PageType pageType = vm::kDefault, int prot = PROT_READ|PROT_WRITE>
struct Allocator_VM
{
	void* allocate(size_t size)
	{
		return vm::Allocate(size, pageType, prot);
	}
	void deallocate(void* p, size_t size)
	{
		vm::Free(p, size);
	}
};

template<size_t commitSize = largePageSize, vm::PageType pageType = vm::kDefault, int prot = PROT_READ|PROT_WRITE>
struct Allocator_AddressSpace
{
	void* allocate(size_t size)
	{
		return vm::ReserveAddressSpace(size, commitSize, pageType, prot);
	}
	void deallocate(void* p, size_t size)
	{
		vm::ReleaseAddressSpace(p, size);
	}
};
*/

namespace Testbed
{
	struct InitMemorySettings
	{
		/// The size of the thread-safe ring buffer allocator that can be used for passing data between threads.
		/// Set to 0 to disable the global locking heap for temporary allocations.
		U32		temporaryHeapSizeMiB;

		/// if > 0 then use debug heap
		U32		debugHeapSizeMiB;
	};
	extern InitMemorySettings	g_InitMemorySettings;
}
