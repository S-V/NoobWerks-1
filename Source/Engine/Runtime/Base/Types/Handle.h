#pragma once


/// Programming can be especially tricky using integer indices instead of pointers which
/// offer greater safety due to type checking. Therefore, we wrap integers with structs
/// to safeguard the programmer against using the id of an edge when indexing the vertex list.
///
/// analog of BOOST_STRONG_TYPEDEF(T,VIndex);
#define MAKE_ALIAS( ALIAS, VALUE )\
	struct ALIAS\
	{\
		VALUE	id;\
	public:\
		typedef VALUE TYPE;\
		mxFORCEINLINE ALIAS() {}\
		mxFORCEINLINE explicit ALIAS( const VALUE value ) : id( value ) {}\
		mxFORCEINLINE ALIAS( const ALIAS& other ) : id( other.id ) {}\
        mxFORCEINLINE ALIAS & operator = ( const ALIAS& other ) { id = other.id; return *this;}\
		mxFORCEINLINE operator VALUE & () { return id; }\
		mxFORCEINLINE operator VALUE () const { return id; }\
		mxFORCEINLINE bool operator == ( const ALIAS& other ) const { return this->id == other.id; }\
		mxFORCEINLINE bool operator != ( const ALIAS& other ) const { return this->id != other.id; }\
		mxFORCEINLINE bool operator < ( const ALIAS& other ) const { return id < other.id; }\
	};




mxSTOLEN("Valve's Source Engine/ L4D 2 SDK");
//-------------------------------------------------------------------------
// Declares a type-safe handle type; you can't assign one handle to the next
//-------------------------------------------------------------------------

// 32-bit pointer handles.

// Typesafe 8-bit and 16-bit handles.
template< class HandleType >
class CBaseIntHandle
{
public:
	inline bool	operator==( const CBaseIntHandle &other )	{ return m_Handle == other.m_Handle; }
	inline bool	operator!=( const CBaseIntHandle &other )	{ return m_Handle != other.m_Handle; }

	// Only the code that doles out these handles should use these functions.
	// Everyone else should treat them as a transparent type.
	inline HandleType	GetHandleValue() const				{ return m_Handle; }
	inline void		SetHandleValue( HandleType val )	{ m_Handle = val; }

	typedef HandleType	HANDLE_TYPE;

protected:
	HandleType	m_Handle;
};

template< class DummyType >
class CIntHandle16: public CBaseIntHandle< U16 >
{
public:
	inline			CIntHandle16() {}

	static inline	CIntHandle16<DummyType> MakeHandle( HANDLE_TYPE val )
	{
		return CIntHandle16<DummyType>( val );
	}

protected:
	inline			CIntHandle16( HANDLE_TYPE val )
	{
		m_Handle = val;
	}
};


template< class DummyType >
class CIntHandle32: public CBaseIntHandle< U32 >
{
public:
	inline			CIntHandle32() {}

	static inline	CIntHandle32<DummyType> MakeHandle( HANDLE_TYPE val )
	{
		return CIntHandle32<DummyType>( val );
	}

protected:
	inline			CIntHandle32( HANDLE_TYPE val )
	{
		m_Handle = val;
	}
};


// NOTE: This macro is the same as windows uses; so don't change the guts of it
#define DECLARE_HANDLE_16BIT(name)	typedef CIntHandle16< struct name##__handle * > name;
#define DECLARE_HANDLE_32BIT(name)	typedef CIntHandle32< struct name##__handle * > name;

#define DECLARE_POINTER_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name


// (-1) represents a null value
// (0) (usually) represents the default value
#define mxIMPLEMENT_HANDLE( HANDLE_NAME, STORAGE )\
	struct HANDLE_NAME\
	{\
		STORAGE		id;\
	public:\
		mxFORCEINLINE explicit HANDLE_NAME( STORAGE handle_value = invalid_handle_value ) { this->id = handle_value; }\
		mxFORCEINLINE HANDLE_NAME( const HANDLE_NAME& other ) { this->id = other.id; }\
		mxFORCEINLINE void SetNil()		{ this->id = (STORAGE)~0; }\
		mxFORCEINLINE void SetDefault()	{ this->id = (STORAGE) 0; }\
		mxFORCEINLINE bool IsNil() const	{ return this->id == (STORAGE)~0; }\
		mxFORCEINLINE bool IsValid() const	{ return this->id != (STORAGE)~0; }\
		mxFORCEINLINE bool operator == ( const HANDLE_NAME& other ) const { return this->id == other.id; }\
		mxFORCEINLINE bool operator != ( const HANDLE_NAME& other ) const { return this->id != other.id; }\
		static mxFORCEINLINE HANDLE_NAME MakeHandle( STORAGE _id )	{ return HANDLE_NAME(_id); }\
		static mxFORCEINLINE HANDLE_NAME MakeNilHandle()			{ return HANDLE_NAME(invalid_handle_value); }\
		static const STORAGE invalid_handle_value = ~0;\
		static const STORAGE max_valid_handle_value = ~0 - 1;\
	};


#define mxDECLARE_8BIT_HANDLE( HANDLE_NAME )	mxIMPLEMENT_HANDLE( HANDLE_NAME, U8 )
#define mxINVALID_8BIT_HANDLE					{ MAX_UINT8 }

#define mxDECLARE_16BIT_HANDLE( HANDLE_NAME )	mxIMPLEMENT_HANDLE( HANDLE_NAME, U16 )
#define mxINVALID_16BIT_HANDLE					{ MAX_UINT16 }

#define mxDECLARE_32BIT_HANDLE( HANDLE_NAME )	mxIMPLEMENT_HANDLE( HANDLE_NAME, U32 )
#define mxINVALID_32BIT_HANDLE					{ MAX_UINT32 }

#define mxDECLARE_64BIT_HANDLE( HANDLE_NAME )	mxIMPLEMENT_HANDLE( HANDLE_NAME, U64 )
#define mxINVALID_64BIT_HANDLE					{ MAX_UINT64 }

#define mxDECLARE_POINTER_HANDLE( HANDLE_NAME )	struct HANDLE_NAME { void* ptr; }
#define mxINVALID_POINTER_HANDLE				{ nil }

#define mxDECLARE_OPAQUE_STRUCT( STRUCT_NAME, HANDLE_NAME )	typedef struct STRUCT_NAME* HANDLE_NAME

// The runtime unique identifier assigned to each object by the corresponding Object System.
// ObjectHandle may not be the same when saving/loading.
// ObjectHandle is mostly used in runtime for fast and unique identification of objects.
//
typedef U32 ObjectHandle;

// Use INDEX_NONE to indicate invalid ObjectHandle's.

// Object ID
class OID
{
	U32	m_handleValue;

public:
	inline OID()
	{}
	inline explicit OID( UINT i )
	{ m_handleValue = i; }

	inline bool operator == ( UINT i ) const
	{ return m_handleValue == i; }

	inline bool operator != ( UINT i ) const
	{ return m_handleValue != i; }

	// Only the code that doles out these handles should use these functions.
	// Everyone else should treat them as a transparent type.
	inline U32		GetHandleValue() const			{ return m_handleValue; }
	inline void	SetHandleValue( U32 newValue )	{ m_handleValue = newValue; }
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



// type of object is stored in 8 bits
struct TypedHandleBase
{
	union	// arrrgh... union cannot be used as a base class so we wrap it in a struct.
	{
		struct
		{
			BITFIELD	index : 24;
			BITFIELD	type : 8;
		};
		U32		value;
	};
};

ASSERT_SIZEOF(TypedHandleBase, sizeof(U32));



template< typename TYPE, typename ENUM, int INVALID_VALUE = -1 >
struct TypedHandle : TypedHandleBase
{
	typedef TypedHandle<TYPE,ENUM,INVALID_VALUE>	THIS_TYPE;

	inline TypedHandle()
	{
	}
	inline TypedHandle( ENUM eType, UINT nIndex )
	{
		this->index = nIndex;
		this->type = eType;
	}

	// NOTE: implicit conversions are intentionally disabled
	//
#if 0
	T& operator*();
	T* operator->();
	const T& operator*() const;
	const T* operator->() const;
#endif

	// Dereferencing.

	inline TYPE* Raw() const
	{
		return TYPE::Static_GetPointerByHandle( *this );
	}

	inline ENUM GetType() const
	{
		return static_cast<ENUM>( this->type );
	}

	inline void operator = ( const THIS_TYPE& other )
	{
		this->value = other.value;
	}
	inline bool operator == ( const THIS_TYPE& other ) const
	{
		this->value == other.value;
	}
	inline bool operator != ( const THIS_TYPE& other ) const
	{
		this->value != other.value;
	}

	inline bool IsValid() const
	{
		return this->value != INVALID_VALUE;
	}
	inline void SetInvalid()
	{
		this->value = INVALID_VALUE;
	}
};








/*
-------------------------------------------------------------------------
	Data structure used to do the lookup from ID to system object.
	Requirements that need to be fulfilled:
	- There should be a 1-1 mapping between live objects and IDs.
	- If the system is supplied with an ID to an old object, it should be able to detect that the object is no longer alive.
	- Lookup from ID to object should be very fast (this is the most common operation).
	- Adding and removing objects should be fast.
-------------------------------------------------------------------------
*/
#if 0
mxSTOLEN("http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html");

enum { INDEX_MASK = 0xffff };
enum { NEW_OBJECT_ID_ADD = 0x10000 };

struct SObjIndex
{
	ObjectHandle id;
	U16 index;
	U16 next;
};
mxDECLARE_POD_TYPE(SObjIndex);

/*
 The ID Lookup Table with a fixed array size, a 32 bit ID and a FIFO free list.
*/
template< class OBJECT, UINT MAX_OBJECTS = 64*1024 >
struct TObjectSystem
{
	typedef TStaticArray<OBJECT,MAX_OBJECTS>	ObjectsArray;
	typedef TStaticArray<SObjIndex,MAX_OBJECTS>	IndicesArray;

	UINT			m_num_objects;
	ObjectsArray	m_objects;
	IndicesArray	m_indices;
	UINT			m_freelist_enqueue;
	UINT			m_freelist_dequeue;

public:
	typedef TObjectSystem THIS_TYPE;

	TObjectSystem()
	{
		m_num_objects = 0;
		for( UINT i=0; i<MAX_OBJECTS; ++i )
		{
			m_indices[i].id = i;
			m_indices[i].next = i+1;
		}
		m_freelist_dequeue = 0;
		m_freelist_enqueue = MAX_OBJECTS-1;
	}

	inline bool has( ObjectHandle id )
	{
		//mxASSERT(m_indices.isValidIndex(id));
		SObjIndex &in = m_indices[id & INDEX_MASK];
		return in.id == id && in.index != USHRT_MAX;
	}

	inline OBJECT& lookup( ObjectHandle id )
	{
		return m_objects[ m_indices[ id & INDEX_MASK ].index ];
	}
	inline const OBJECT& lookup_const( ObjectHandle id ) const
	{
		return m_objects[ m_indices[ id & INDEX_MASK ].index ];
	}

	inline ObjectHandle add()
	{
		mxASSERT(m_num_objects < m_objects.capacity());

		SObjIndex & in = m_indices[ m_freelist_dequeue ];
		m_freelist_dequeue = in.next;
		in.id += NEW_OBJECT_ID_ADD;
		in.index = m_num_objects++;

		OBJECT &o = m_objects[ in.index ];
		o.id = in.id;
		return o.id;
	}

	inline UINT Num() const
	{
		return m_num_objects;
	}
	inline bool isFull() const
	{
		//return m_num_objects == MAX_OBJECTS-1;
		return m_num_objects == MAX_OBJECTS;
	}
	inline OBJECT & GetObjAt( UINT index )
	{
		return m_objects[ index ];
	}
	inline const OBJECT & GetObjAt( UINT index ) const
	{
		return m_objects[ index ];
	}

	inline void remove( ObjectHandle id )
	{
		//mxASSERT(m_indices.isValidIndex(id));

		SObjIndex &in = m_indices[id & INDEX_MASK];

		OBJECT &o = m_objects[in.index];
		o = m_objects[--m_num_objects];
		m_indices[o.id & INDEX_MASK].index = in.index;

		in.index = USHRT_MAX;
		m_indices[m_freelist_enqueue].next = id & INDEX_MASK;
		m_freelist_enqueue = id & INDEX_MASK;
	}

public:
	friend inline mxArchive& operator && ( mxArchive & archive, THIS_TYPE & o )
	{
		archive && o.m_num_objects;
		archive && o.m_objects;
		archive && o.m_indices;
		archive && o.m_freelist_enqueue;
		archive && o.m_freelist_dequeue;
		return archive;
	}
};
#endif

//
// Handle manager which always keeps objects in a fixed-size contiguous block of memory
// and supports removal of elements.
// NOTE: objects must be POD types!
//
// code based on:
// "http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html");
//

// See: http://c0de517e.blogspot.com/2011/03/alternatives-to-object-handles.html
// Permutation array.
// We add a second indirection level, to be able to sort resources to cause less cache misses.
// An handle in an index into an array of indices (permutation) of the array that stores the resources.

// this structure introduces another level of indirection
// so that object data stays contiguous after deleting elements at random places
//
struct SRemapObjIndex
{
	U16	objectIndex;	// index into array of objects
	U16	nextFreeSlot;	// index of the next free slot (in the indices array)

public:
	inline bool IsFree() const { return objectIndex != -1; }
};


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


// 'Tagged' pointer which uses the lowest bit for storing user-defined flag.
// NOTE: the pointer must be at least 2-byte aligned!
//
template< typename TYPE >
class TypedPtr2
{
	ULONG		m_ptr;	// LSB stores user tag

public:
	enum { PTR_MASK = (ULONG)((-1)<<1) };
	enum { TAG_MASK = ~PTR_MASK };

	inline void Set( TYPE* ptr, UINT tag = 0 )
	{
		mxASSERT(IS_8_BYTE_ALIGNED( ptr ));
		mxASSERT( tag < (1<<1) );	// must fit into 1 bit
		const ULONG ptrAsInt = (ULONG)ptr;
		m_ptr = ptrAsInt | tag;
	}
	inline TYPE* GetPtr() const
	{
		return c_cast(TYPE*)(m_ptr & PTR_MASK);
	}
	inline UINT GetTag() const
	{
		return (m_ptr & TAG_MASK);
	}
};

// 'Tagged' pointer which uses lower 4 bits for storing additional data.
// NOTE: the pointer must be at least 16-byte aligned!
//
template< typename TYPE >
class TypedPtr16
{
	ULONG		m_ptr;	// 4 LSBs store user tag

public:
	enum { PTR_MASK = (ULONG)((-1)<<4) };
	enum { TAG_MASK = ~PTR_MASK };

	inline void Set( TYPE* ptr, UINT tag = 0 )
	{
		mxASSERT(IS_16_BYTE_ALIGNED( ptr ));
		mxASSERT( tag < (1<<4) );	// must fit into 4 bits

		const ULONG ptrAsInt = (ULONG)ptr;
		m_ptr = ptrAsInt | tag;
	}
	inline TYPE* GetPtr() const
	{
		return c_cast(TYPE*)(m_ptr & PTR_MASK);
	}
	inline UINT GetTag() const
	{
		return (m_ptr & TAG_MASK);
	}
};
