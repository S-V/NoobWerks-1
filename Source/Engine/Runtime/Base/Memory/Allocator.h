/*
=============================================================================
	File:	Allocator.h
	Desc:	
=============================================================================
*/
#pragma once


struct DeallocatorI: NonCopyable
{
	/// Frees an allocation previously made with allocate().
	///@todo: pass the allocated size as an optimization?
	virtual void Deallocate( const void* memory_block ) = 0;

protected:
	virtual ~DeallocatorI() {}
};

mxSTOLEN("based on Bitsquid foundation library");
/// Base class for memory allocators.
///
/// Note: Regardless of which allocator is used, prefer to allocate memory in larger chunks
/// instead of in many small allocations. This helps with data locality, fragmentation,
/// memory usage tracking, etc.
///
/// NOTE: ideally, 'dynamically-resizing' containers and the associated memory manager
/// should know about each other to efficiently work together.
/// We could pass the size of allocated memory to the Free() function (like STL allocators),
/// but this optimization is usually avoided in practice (a new source of bugs).
///
/// Memory allocations/deallocations are expensive operations
/// so use of virtual member functions should be ok.
///
struct AllocatorI: DeallocatorI
{
	/// Allocates the specified amount of memory aligned to the specified alignment.
	virtual void* Allocate( U32 size, U32 alignment ) = 0;



	// Changes the size of a block that was allocated with allocate.
	// Argument _block can be NULL.
	// Reallocate function conforms with standard realloc function specifications.
	virtual void* reallocate( void* memory_block, U32 size, U32 alignment )
	{
		return nil;
	}

	/// Returns the amount of usable memory allocated at 'memory_block'.
	/// 'memory_block' must be a pointer returned by allocate()
	/// that has not yet been deallocated.
	/// The value returned will be at least the size specified to allocate(), but it can be bigger.
	/// (The allocator may round up the allocation to fit into a set of predefined
	/// slot sizes.)
	///
	/// Not all allocators support tracking the size of individual allocations.
	/// An allocator that doesn't support it will return SIZE_NOT_TRACKED.
	virtual U32	GetUsableSize( const void* memory_block ) const
	{
		return SIZE_NOT_TRACKED;
	}

public:
	/// Returns the total amount of memory allocated by this allocator. Note that the 
	/// size returned can be bigger than the size of all individual allocations made,
	/// because the allocator may keep additional structures.
	///
	/// If the allocator doesn't track memory, this function returns SIZE_NOT_TRACKED.
	virtual U32 total_allocated() const
	{
		return SIZE_NOT_TRACKED;
	}

	virtual void Validate() {};	// verify heap (debug mode only)
	virtual void Optimize() {};	// compact heap

	/// writes memory statistics, usage & leak info to the specified file
	virtual void Dump( ALog& log ) {};

	/// \return The name of the allocator implementing this interface
	//virtual const char* GetName() const { return mxSTRING_Unknown; }

	/// A return value indicating that 'the size is not tracked'.
	static const U32 SIZE_NOT_TRACKED = 0;

public:	// An operator new-like function that allocates memory and calls T's constructor with parameters.

	template <class T>
	mxFORCEINLINE T* new_()
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T();
	}

	template <class T, typename P0>
	mxFORCEINLINE T* new_(P0& p0)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0);
	}

	template <class T, typename P0>
	mxFORCEINLINE T* new_(const P0& p0)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0);
	}

	template <class T, typename P0, typename P1>
	mxFORCEINLINE T* new_(P0& p0, P1& p1)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1);
	}

	template <class T, typename P0, typename P1>
	mxFORCEINLINE T* new_(const P0& p0, P1& p1)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1);
	}

	template <class T, typename P0, typename P1>
	mxFORCEINLINE T* new_(P0& p0, const P1& p1)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1);
	}

	template <class T, typename P0, typename P1, typename P2>
	mxFORCEINLINE T* new_(P0& p0, const P1& p1, const P2& p2)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2);
	}

	template <class T, typename P0, typename P1, typename P2>
	mxFORCEINLINE T* new_(P0& p0, const P1& p1, P2& p2)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2);
	}

	template <class T, typename P0, typename P1, typename P2, typename P3>
	mxFORCEINLINE T* new_(P0& p0, P1& p1, P2& p2, P3& p3)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2, p3);
	}

	template <class T, typename P0, typename P1, typename P2, typename P3>
	mxFORCEINLINE T* new_(P0& p0, const P1& p1, P2& p2, P3& p3)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2, p3);
	}

	template <class T, typename P0, typename P1, typename P2, typename P3, typename P4>
	mxFORCEINLINE T* new_(P0& p0, P1& p1, P2& p2, P3& p3, P4& p4)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2, p3, p4);
	}

	template <class T, typename P0, typename P1, typename P2, typename P3, typename P4>
	mxFORCEINLINE T* new_(P0& p0, const P1& p1, P2& p2, P3& p3, P4& p4)
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T(p0, p1, p2, p3, p4);
	}

	template< class T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5 >
	mxFORCEINLINE T* new_( P0& p0, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5 )
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T( p0, p1, p2, p3, p4, p5 );
	}

	template< class T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6 >
	mxFORCEINLINE T* new_( P0& p0, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6 )
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T( p0, p1, p2, p3, p4, p5, p6 );
	}

	template< class T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7 >
	mxFORCEINLINE T* new_( P0& p0, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6, P7& p7 )
	{
		return new ( this->Allocate( sizeof(T), mxALIGNOF(T) ) ) T( p0, p1, p2, p3, p4, p5, p6, p7 );
	}

public:

	template< class T >
	mxFORCEINLINE void delete_( T* ptr )
	{
		if( ptr )
		{
			ptr->~T();
			this->Deallocate(ptr);
		}
	}

protected:
	AllocatorI() {}
	virtual	~AllocatorI() {}
};



class TbMetaClass;

/// Allows objects of a specific class to be allocated from special pools/Clumps.
/// Usually represents a memory heap used for storing {resource objects|asset instances}.
struct ObjectAllocatorI
{
	/// simply allocates storage, doesn't call ctor
	/// NOTE: the returned memory block is uninitialized!
	virtual void* allocateObject( const TbMetaClass& object_type, U32 granularity = 1 ) = 0;

	/// similarly, doesn't call dtor
	virtual void deallocateObject( const TbMetaClass& object_type, void* object_instance ) = 0;

public:	// Utilities

	template< class T >
	ERet allocateObject( T **o_, U32 granularity = 1 )
	{
		void* object_instance = this->allocateObject( T::metaClass(), granularity );
		if( !object_instance ) {
			return ERR_OUT_OF_MEMORY;
		}
		*o_ = static_cast< T* >( object_instance );
		return ALL_OK;
	}

	/// An operator new-like function that allocates memory and calls T's constructor.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T>
	T* new_()
	{
		return new ( this->allocateObject(T::metaClass()) ) T();
	}

	/// An operator new-like function that allocates memory and calls T's constructor with one parameter.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0>
	T* new_(P0& p0)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with one parameter.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0>
	T* new_(const P0& p0)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with two parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1>
	T* new_(P0& p0, P1& p1)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with two parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1>
	T* new_(const P0& p0, P1& p1)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with two parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1>
	T* new_(P0& p0, const P1& p1)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2>
	T* new_(P0& p0, const P1& p1, const P2& p2)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2>
	T* new_(P0& p0, const P1& p1, P2& p2)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2, class P3>
	T* new_(P0& p0, P1& p1, P2& p2, P3& p3)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2, p3);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2, class P3>
	T* new_(P0& p0, const P1& p1, P2& p2, P3& p3)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2, p3);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2, class P3, class P4>
	T* new_(P0& p0, P1& p1, P2& p2, P3& p3, P4& p4)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2, p3, p4);
	}

	/// An operator new-like function that allocates memory and calls T's constructor with the given parameters.
	/**
	* \return A pointer to an initialized instance of T.
	*/
	template <class T, class P0, class P1, class P2, class P3, class P4>
	T* new_(P0& p0, const P1& p1, P2& p2, P3& p3, P4& p4)
	{
		return new ( this->allocateObject(T::metaClass()) ) T(p0, p1, p2, p3, p4);
	}

	template< class T >
	void delete_( T* ptr )
	{
		if( ptr )
		{
			ptr->~T();
			this->deallocateObject( T::metaClass(), ptr );
		}
	}
};









//-----------------------------------------------------------------------
//		HELPER MACROS
//-----------------------------------------------------------------------

mxREMOVE_THIS//("causes crashes if allocator returned nil");

#if mxSUPPORTS_VARIADIC_TEMPLATES

	/// Creates a new object of the given type using the specified allocator.
	template< typename TYPE, typename... ARGs >
	TYPE* mxNEW( AllocatorI& _heap, ARGs&&... _args )
	{
		void * storage = _heap.Allocate( sizeof(TYPE), mxALIGNOF(TYPE) );
		return new(storage) TYPE( std::forward< ARGs >(_args)... );
	}

#else

	/// Creates a new object of the given type using the specified allocator.
	#define mxNEW( HEAP, TYPE, ... )\
		(new\
			((HEAP).Allocate( sizeof(TYPE), GET_ALIGNMENT_FOR_CONTAINERS(TYPE) ))\
				TYPE(__VA_ARGS__))

#endif




#if mxSUPPORTS_VARIADIC_TEMPLATES

	/// Creates a new object of the given type using the specified allocator.
	template< typename TYPE, typename... ARGs >
	ERet nwCREATE_OBJECT(
		TYPE*&o_
		, AllocatorI& allocator
		, ARGs&&... args
		)
	{
		void* mem = allocator.Allocate(
			sizeof(TYPE)
			, mxALIGNOF(TYPE)
			);
		if( mem )
		{
			o_ = new(mem) TYPE(
				std::forward< ARGs >(args)...
				);
			return ALL_OK;
		}
		return ERR_OUT_OF_MEMORY;
	}

#else

	/// Creates a new object of the given type using the specified allocator.
	#define nwCREATE_OBJECT( PTR, HEAP, TYPE, ... )\
		(\
			PTR = (TYPE*) (HEAP).Allocate( sizeof(TYPE), GET_ALIGNMENT_FOR_CONTAINERS(TYPE) ),\
			PTR\
				? (new(PTR) TYPE(__VA_ARGS__), ALL_OK)\
				: ERR_OUT_OF_MEMORY\
		)

#endif

	/// Frees an object allocated with mxNEW.
	template< typename TYPE, class HEAP >
	void nwDESTROY_OBJECT( const TYPE* o, HEAP& heap )
	{
		o->~TYPE();
		heap.Deallocate( o );
	}




template< typename T >
class TAutoDestroy: NonCopyable
{
	T *				_o;
	AllocatorI &	_allocator;

public:
	mxFORCEINLINE TAutoDestroy( T* o, AllocatorI& allocator )
		: _o( o ), _allocator( allocator )
	{}

	mxFORCEINLINE ~TAutoDestroy()
	{
		if(_o) {
			nwDESTROY_OBJECT( _o, _allocator );
		}
	}

	mxFORCEINLINE void Disown()
	{
		_o = nil;
	}
};




	/// Frees an object allocated with mxNEW.
	template< typename TYPE, class HEAP >
	void mxDELETE( TYPE* o, HEAP& _heap )
	{
		if( o != nil )
		{
			o->~TYPE();
			_heap.Deallocate( o );
		}
	}

	/// Frees an object allocated with mxNEW.
	template< typename TYPE, class HEAP >
	void mxDELETE_AND_NIL( TYPE *& o_, HEAP& _heap )
	{
		if( o_ != nil )
		{
			o_->~TYPE();
			_heap.Deallocate( o_ );
			o_ = nil;
		}
	}

///
#define mxTRY_ALLOC_SCOPED( _VAR, _ARRAY_COUNT, _ALLOCATOR )\
	mxDO( __TryAllocate( _VAR, _ARRAY_COUNT, _ALLOCATOR ) );\
	AutoFree __autofree##_VAR##__LINE__( _ALLOCATOR, _VAR );

/// internal function used by the mxTRY_ALLOC_SCOPED macro
template< typename TYPE >
ERet __TryAllocate( TYPE *& _pointer, U32 _count, AllocatorI &_allocator )
{
	TYPE* p = static_cast< TYPE* >( _allocator.Allocate( sizeof(TYPE)*_count, GET_ALIGNMENT_FOR_CONTAINERS(TYPE) ) );
	if( !p ) {
		return ERR_OUT_OF_MEMORY;
	}
	_pointer = p;
	return ALL_OK;
}

/// automatically frees pointed memory; used internally by the mxTRY_ALLOC_SCOPED macro
class AutoFree {
	AllocatorI &	m_heap;
	void *	m_memory;
public:
	AutoFree( AllocatorI & _heap, void* _memory )
		: m_heap( _heap ), m_memory( _memory )
	{}
	~AutoFree() {
		m_heap.Deallocate( m_memory );
	}
};

/// renders any heap thread-safe
class ThreadSafeProxyHeap: public AllocatorI
{
	mutable SpinWait	m_lock;
	AllocatorI &	m_heap; //!< the actual allocator we're using under the hood
public:
	ThreadSafeProxyHeap( AllocatorI &_heap )
		: m_heap( _heap )
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
		SpinWait::Lock	scopedLock( m_lock );
		void* result = m_heap.Allocate( _bytes, alignment );
		return result;
	}
	virtual void Deallocate( const void* _memory ) override
	{
		if( _memory ) {
			SpinWait::Lock	scopedLock( m_lock );
			m_heap.Deallocate( (void*)_memory );
		}
	}
	virtual U32	GetUsableSize( const void* _memory ) const
	{
		SpinWait::Lock	scopedLock( m_lock );
		return m_heap.GetUsableSize( _memory );
	}
};

#if ENGINE_CONFIG_USE_IG_MEMTRACE

  class MemTraceProxyHeap: public AllocatorI
  {
	  AllocatorI &		m_client;	//!< the underlying allocator
	  U32			m_heapId;	//!< MemTrace::HeapId
  public:
	  MemTraceProxyHeap( const char* _name, AllocatorI &_client );
	  ~MemTraceProxyHeap();

	  virtual void* Allocate( U32 _bytes, U32 alignment ) override;
	  virtual void Deallocate( const void* _memory ) override;
	  virtual U32	GetUsableSize( const void* _memory ) const override;
  };
  typedef MemTraceProxyHeap TrackingHeap;

#else

	/// A proxy allocator that simply draws upon another allocator.
	/// http://bitsquid.blogspot.ru/2010/09/custom-memory-allocation-in-c.html
	class TrackingHeap: public AllocatorI {
		AllocatorI & m_allocator;
	public:
		TrackingHeap( const char* _name, AllocatorI & _allocator )
			: m_allocator( _allocator )
		{}
		virtual void* Allocate( U32 _bytes, U32 alignment ) override {
			return m_allocator.Allocate( _bytes, alignment );
		}
		virtual void Deallocate( const void* _memory ) override {
			m_allocator.Deallocate( _memory );
		}
		virtual U32	GetUsableSize( const void* _memory ) const override {
			return m_allocator.GetUsableSize( _memory );
		}
	};

#endif




template< typename TYPE >
ERet nwAllocArray(
				   TYPE *& a_, U32 count
				   , AllocatorI& allocator
				   , unsigned alignment = GET_ALIGNMENT_FOR_CONTAINERS(TYPE)
				   )
{
	TYPE* p = static_cast< TYPE* >( allocator.Allocate( sizeof(TYPE)*count, alignment ) );
	if( !p ) {
		return ERR_OUT_OF_MEMORY;
	}
	a_ = p;
	return ALL_OK;
}

template< typename T >
struct TScopedPtr: NonCopyable
{
	T *				ptr;
	AllocatorI &	allocator;

public:
	mxFORCEINLINE TScopedPtr( AllocatorI& allocator, T* ptr = nil )
		: allocator( allocator ), ptr( ptr )
	{}

	// so that it can be put into arrays and initialized later (with placement new)
	mxFORCEINLINE TScopedPtr()
		: ptr(nil)
		, allocator(*(AllocatorI*)nil)
	{}

	mxFORCEINLINE ~TScopedPtr()
	{
		allocator.Deallocate( ptr );
	}
};
