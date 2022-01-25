#pragma once



//
// Helper templates.
//
template< typename T >
struct Equal
{
	inline bool operator()(const T& a, const T& b)	const { return a==b; }
};
template< typename T >
struct Less
{
	inline bool operator()(const T& a, const T& b)	const { return a<b; }
};
template< typename T >
struct Greater
{
	inline bool operator()(const T& a, const T& b)	const { return a>b; }
};

template< typename T > inline T	Max( T x, T y ) { return ( x > y ) ? x : y; }
template< typename T > inline T	Min( T x, T y ) { return ( x < y ) ? x : y; }
template< typename T > inline int	MaxIndex( T x, T y ) { return  ( x > y ) ? 0 : 1; }
template< typename T > inline int	MinIndex( T x, T y ) { return ( x < y ) ? 0 : 1; }

template< typename T > inline T	Max3( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? x : z ) : ( ( y > z ) ? y : z ); }
template< typename T > inline T	Min3( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? x : z ) : ( ( y < z ) ? y : z ); }
template< typename T > inline int	Max3Index( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ); }
template< typename T > inline int	Min3Index( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? 0 : 2 ) : ( ( y < z ) ? 1 : 2 ); }

template< typename T > inline T	Abs( T x )		{ return ( x >= 0 ) ? x : -x; }
template< typename T > inline T	Sign( T x )		{ return ( x > 0 ) ? 1 : ( ( x < 0 ) ? -1 : 0 ); }
template< typename T > inline T	Square( T x )	{ return x * x; }
template< typename T > inline T	ToCube( T x )	{ return x * x * x; }

template< typename T > inline T	Clamp( const T value, const T Min, const T Max )	{ return (value < Min) ? Min : (value > Max) ? Max : value; }
template< typename T > inline T	Saturate( const T value )						{ return Clamp<T>( value, 0, 1 ); }

template< typename T > inline T	Mean( const T a, const T b )	{ return (a + b) * (T)0.5f; }

template< typename T > inline T	Wrap( const T a, const T min, const T max )	{ return (a < min)? max - (min - a) : (a > max)? min - (max - a) : a; }

template< typename T > inline bool	IsInRange( const T value, const T Min, const T Max )	{ return (value > Min) && (value < Max); }
template< typename T > inline bool	IsInRangeInc( const T value, const T Min, const T Max )	{ return (value >= Min) && (value <= Max); }


template< typename T >
inline void TSetMin( T& a, const T& b ) {
    if( b < a ) {
		a = b;
	}
}

template< typename T >
inline void TSetMax( T& a, const T& b ) {
    if( a < b ) {
		a = b;
	}
}

template< typename T >
inline const T& TGetClamped( const T& a, const T& lo, const T& hi )
{
	return (a < lo) ? lo : (hi < a ? hi : a);
}

/// maps negative values to -1, zero to 0 and positive to +1
template< typename T >
inline T MapToUnitRange( const T& value ) {
	return T( (value > 0) - (value < 0) );
}

//static inline
//FLOAT SafeReciprocal( FLOAT x )
//{
//	return (fabs(x) > 1e-4f) ? (1.0f / x) : 0.0f;
//}

///
///	TSwap - exchanges values of two elements.
///
template< typename type >
inline void TSwap( type & a, type & b )
{
	const type  temp = a;
	a = b;
	b = temp;
}

/*
	swap in C++ 11:

	template <class T> swap(T& a, T& b)
	{
		T tmp(std::move(a));
		a = std::move(b);   
		b = std::move(tmp);
	}
*/

///	TSwap16 - exchanges values of two 16-byte aligned elements.
template< typename type >
inline void TSwap16( type & a, type & b )
{
	mxALIGN_16( const type  temp ) = a;
	a = b;
	b = temp;
}

//template <class T> inline void Exchange( T & X, T & Y )
//{
//	const T Tmp = X;
//	X = Y;
//	Y = Tmp;
//}


// TODO: endian swap
template< typename TYPE >
inline void TOrderPointers( TYPE *& pA, TYPE *& pB )
{
	if( (ULONG)pA > (ULONG)pB )
	{
		TSwap( pA, pB );
	}
}

template< typename TYPE >
inline void TSort2( TYPE & oA, TYPE & oB )
{
	if( oA > oB )
	{
		TSwap( oA, oB );
	}
}

template< typename TYPE >
inline void TSort3( TYPE & a, TYPE & b, TYPE & c )
{
	if( a > b ) {
		TSwap( a, b );
	}
	if( b > c ) {
		TSwap( b, c );
	}
	if( a > b ) {
		TSwap( a, b );
	}
	if( b > c ) {
		TSwap( b, c );
	}
}

template< typename TYPE >
void Sort3IndicesAscending( const TYPE values[3], unsigned indices[3] )
{
	// find the index of the minimal value
	const unsigned imin = Min3Index( values[0], values[1], values[2] );

	indices[0] = imin;

	// find the minimal index in the two remaining values
	const unsigned i2 = (imin + 1) % 3;
	const unsigned i3 = (imin + 2) % 3;

	if( values[i2] < values[i3] ) {
		indices[1] = i2;
		indices[2] = i3;
	} else {
		indices[1] = i3;
		indices[2] = i2;
	}
	//mxASSERT(values[indices[0]] <= values[indices[1]]);
	//mxASSERT(values[indices[1]] <= values[indices[2]]);
}

#if 0
/// Figure out if a type is a pointer
template< typename TYPE > struct is_pointer
{
	enum { VALUE = false };
};
template< typename TYPE > struct is_pointer< TYPE* >
{
	enum { VALUE = true };
};

/// Removes the pointer from a type
template< typename TYPE > struct strip_pointer
{
	typedef TYPE Type;
};
template< typename TYPE > struct strip_pointer< TYPE* >
{
	typedef TYPE Type;
};
#endif

template< typename TYPE >
bool IsSignedType( const TYPE t ) {
	return TYPE( -1 ) < 0;
}





//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template< class T >
inline T* ConstructInPlace( T* mem )
{
	return ::new(mem) T;
}

template< class T >
inline T* ConstructInPlace( T* mem, const T* copy )
{
	return ::new(mem) T( *copy );
}

template< class T, class T2 >
inline T* ConstructInPlace( T* mem, T2 t2 )
{
	return ::new(mem) T( t2 );
}

template< class T, class T2, class T3 >
inline T* ConstructInPlace( T* mem, T2 t2, T3 t3 )
{
	return ::new(mem) T( t2, t3 );
}

template< class T, class T2, class T3, class T4 >
inline T* ConstructInPlace( T* mem, T2 t2, T3 t3, T4 t4 )
{
	return ::new(mem) T( t2, t3, t4 );
}

template< class T, class T2, class T3, class T4, class T5 >
inline T* ConstructInPlace( T* mem, T2 t2, T3 t3, T4 t4, T5 t5 )
{
	return ::new(mem) T( t2, t3, t4, t5 );
}

//-----------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------

template< class T >
static inline void Construct( T* pMemory )
{
	::new( pMemory ) T;
}

template< class T >
static inline void CopyConstruct( T* pMemory, T const& src )
{
	::new( pMemory ) T(src);
}

// Destructs an object without freeing the memory associated with it.
template< class T >
static inline void Destruct( T* ptr )
{
	ptr->~T();
}

template< class T >
static inline void FDestructorCallback( void* o )
{
	((T*)o)->~T();
}

// Constructs an array of objects with placement ::new.
template< typename T >
static inline T * TConstructN( T* basePtr, size_t objectCount )
{
	for( size_t i = 0; i < objectCount; i++ )
	{
		::new ( basePtr + i ) T();
	}
	return basePtr;
}

// Explicitly calls destructors for an array of objects.
//
template< typename T >
static inline void TDestructN( T* basePtr, size_t objectCount )
{
#if 1
	for( size_t i = 0; i < objectCount; i++ )
	{
		(basePtr + i)->~T();
	}
#else
	mxASSERT( objectCount != 0 );

	T* start = basePtr;
	T* current = basePtr + objectCount - 1;

	// Destroy in reverse order, to match construction order.
	while( current-- > start )
	{
		current->~T();
	}
#endif
//#if MX_DEBUG
//	memset( basePtr, FREED_MEM_ID, sizeof(T) * objectCount );
//#endif
}


template< typename T >
static inline void TDestructOne_IfNonPOD( T & o )
{
	if( !TypeTrait<T>::IsPlainOldDataType )
	{
		o.~T();
	}
}

template< typename T >
static inline void TConstructN_IfNonPOD( T* basePtr, size_t objectCount )
{
	if( !TypeTrait<T>::IsPlainOldDataType )
	{
		TConstructN< T >( basePtr, objectCount );
	}
}

template< typename T >
static inline void TConstructN_IfNonPOD( T* basePtr, const T& initial_value, size_t objectCount )
{
	if( TypeTrait<T>::IsPlainOldDataType ) {
		TSetArray( basePtr, objectCount, initial_value );
	} else {
		for( size_t i = 0; i < objectCount; i++ ) {
			new (&basePtr[i]) T( initial_value );
		}
	}
}

template< typename T >
static inline void TDestructN_IfNonPOD( T* basePtr, size_t objectCount )
{
	if( !TypeTrait<T>::IsPlainOldDataType )
	{
		TDestructN< T >( basePtr, objectCount );
	}
}

template< typename T >
static inline void TCopyArray( T* dest, const T* src, size_t count )
{
	if( TypeTrait<T>::IsPlainOldDataType ) {
		memcpy( dest, src, count * sizeof(T) ); }
	else {
		for( size_t i = 0; i < count; i++ ) {
			dest[ i ] = src[ i ];
		}
	}
}

template< typename T >
static inline void TCopyConstructArray( T* dest, const T* src, size_t count )
{
	if( TypeTrait<T>::IsPlainOldDataType ) {
		memcpy( dest, src, count * sizeof(T) );
	} else {
		for( size_t i = 0; i < count; i++ ) {
			new (&dest[i]) T( src[i] );
		}
	}
}

template< typename TYPE1, const UINT SIZE, typename TYPE2 >
static inline void TCopyStaticArray( TYPE1 (&dst)[SIZE], const TYPE2 (&src)[SIZE] )
{
	for( UINT i = 0; i < SIZE; i++ ) {
		dst[i] = src[i];
	}
}
//template< typename TYPE, const UINT SIZE >
//static inline void TCopyStaticArray( TYPE (&dst_)[SIZE], const TYPE (&src)[SIZE] )
//{
//	for( UINT i = 0; i < SIZE; i++ ) {
//		dst_[i] = src[i];
//	}
//}
template< typename TYPE1, typename TYPE2 >
static inline void TCopyStaticArray( TYPE1 *dst, const TYPE2* src, UINT count )
{
	for( UINT i = 0; i < count; i++ ) {
		dst[i] = src[i];
	}
}

template< typename TYPE, size_t SIZE >
static inline
void TSetStaticArray( TYPE (&dest_array)[SIZE], const TYPE& value )
{
	for( int i = 0; i < SIZE; i++ ) {
		dest_array[ i ] = value;
	}
}

template< typename T >
static inline
void TSetArray( T* dest, size_t count, const T& srcValue )
{
	// memset - The value is passed as an int, but the function fills the block of memory
	// using the unsigned char conversion of this value
	//if( TypeTrait<T>::IsPlainOldDataType
	//	&& ( sizeof srcValue <= sizeof int )
	//	)
	//{
	//	//if( count )
	//	memset( dest, &srcValue, count * sizeof(T) );
	//}
	//else
	{
		for( size_t i = 0; i < count; i++ )
		{
			dest[ i ] = srcValue;
		}
	}
}

template< typename T >
static inline
void TMoveArray( T* dest, const T* src, size_t count )
{
	if( TypeTrait<T>::IsPlainOldDataType )
	{
		memmove( dest, src, count * sizeof(T) );
	}
	else
	{
		for( size_t i = 0; i < count; i++ )
		{
			dest[ i ] = src[ i ];
		}
	}
}


template< typename SIZE_TYPE, UINT NUM_BUCKETS >
mxDEPRECATED
void Build_Offset_Table_1D(const SIZE_TYPE (&counts)[NUM_BUCKETS],
						   SIZE_TYPE (&offsets)[NUM_BUCKETS])
{
	UINT	accumulatedOffset = 0;

	for( UINT iBucket = 0; iBucket < NUM_BUCKETS; iBucket++ )
	{
		offsets[ iBucket ] = accumulatedOffset;

		accumulatedOffset += counts[ iBucket ];
	}
}

// builds a 2D table of offsets for indexing into sorted list
// (similar to prefix sum computation in radix sort algorithm)
//
template< UINT NUM_ROWS, UINT NUM_COLUMNS >
void Build_Offset_Table_2D(const UINT counts[NUM_ROWS][NUM_COLUMNS],
						UINT offsets[NUM_ROWS][NUM_COLUMNS])
{
	UINT	accumulatedOffset = 0;

	for( UINT iRow = 0; iRow < NUM_ROWS; iRow++ )
	{
		for( UINT iCol = 0; iCol < NUM_COLUMNS; iCol++ )
		{
			// items in cell [iRow][iCol] start at 'accumulatedOffset'
			offsets[iRow][iCol] = accumulatedOffset;

			accumulatedOffset += counts[iRow][iCol];
		}
	}
}

template< UINT COUNT >
UINT Calculate_Sum_1D( const UINT (&elements)[COUNT] )
{
	UINT totalCount = 0;
	for( UINT i = 0; i < COUNT; i++ )
	{
		totalCount += elements[ i ];
	}
	return totalCount;
}

template< UINT NUM_ROWS, UINT NUM_COLUMNS >
UINT Calculate_Sum_2D( const UINT (&elements)[NUM_ROWS][NUM_COLUMNS] )
{
	UINT totalCount = 0;
	for( UINT iRow = 0; iRow < NUM_ROWS; iRow++ )
	{
		for( UINT iCol = 0; iCol < NUM_COLUMNS; iCol++ )
		{
			totalCount += elements[iRow][iCol];
		}
	}
	return totalCount;
}


template< typename STRUC >
static inline
bool structsBitwiseEqual( const STRUC& a, const STRUC& b )
{
	return 0 == memcmp( &a, &b, sizeof(STRUC) );
}

template< typename A, typename B >
static inline void CopyPOD( A &dest, const B& src )
{
	mxSTATIC_ASSERT( sizeof(dest) == sizeof(src) );
	memcpy( &dest, &src, sizeof(dest) );
}

/// Useful function to clean the structure.
template< class TYPE >
static inline void mxZERO_OUT( TYPE &o )
{
	MemZero( &o, sizeof(o) );
}

template< typename TYPE, int SIZE >
static inline void mxZERO_OUT_ARRAY( TYPE (&a)[SIZE] )
{
	for( int i = 0; i < SIZE; i++ ) {
		a[i] = (TYPE) 0;
	}
}



template< typename BOOL_TYPE >
inline
BOOL_TYPE InvertBool( BOOL_TYPE & o )
{
	return (o = !o);
}



template< typename A, typename B >	// where A : B
inline
void CopyStruct( A& dest, const B &src )
{
	// compile-time inheritance test
	B* dummy = static_cast<B*>(&dest);
	(void)dummy;

	mxSTATIC_ASSERT( sizeof(dest) >= sizeof(src) );
	memcpy( &dest, &src, sizeof(src) );
}



template< typename TYPE >
static inline
void TMemCopyArray( TYPE *dest, const TYPE* src, const UINT numItems )
{
	mxASSERT_PTR( dest );
	mxASSERT_PTR( src );
	mxASSERT( dest != src );
	mxASSERT( numItems > 0 );
	memcpy( dest, src, numItems * sizeof(src[0]) );
}


// Type punning.
// Always cast any type into any other type.
// (This is evil.)
//	See:
//	http://mail-index.netbsd.org/tech-kern/2003/08/11/0001.html
//	http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Optimize-Options.html#Optimize-Options
//

// aka 'Union Cast'
template< typename TO, typename FROM >
mxFORCEINLINE TO UnsafeAnyCast( const FROM _from )
{
	union _ {
		FROM from;
		TO to;
	} u = { _from };
	return u.to;
}

template<  class TargetType, class SourceType > 
mxFORCEINLINE TargetType always_cast( SourceType x )
{
	mxCOMPILE_TIME_WARNING( sizeof(TargetType) == sizeof(SourceType), "Type sizes are different" );
	union {
		SourceType S;
		TargetType T;
	} value;

	value.S = x;
	return value.T;
}


template< typename A, typename B >
A& TypePunning( B &v )
{
    union
    {
        A *a;
        B *b;
    } pun;
    pun.b = &v;
    return *pun.a;
}

template< typename A, typename B >
const A& TypePunning( const B &v )
{
    union
    {
        const A *a;
        const B *b;
    } pun;
    pun.b = &v;
    return *pun.a;
}

//
//	TUtil_Conversion< T, U >
//
template< typename T, typename U >
class TUtil_Conversion
{
	typedef char Small;
	class Big { char dummy[2]; };
	static Small test(U);
	static Big test(...);
	static T makeT();
public:
	enum {
		exists = sizeof(test(makeT()))==sizeof(Small),
		sameType = 0
	};
};

template< typename T >
class TUtil_Conversion< T, T > {
public:
	enum {
		exists = 1,
		sameType = 1
	};
};

#define MX_SUPERSUBCLASS( T,U ) \
	TUtil_Conversion< const U*, const T* >::exists && \
	!TUtil_Conversion< const T*, const void* >::sameType

template< class T, class U>
struct SameType {
    enum {
        Result = false
    };
};

template< class T>
struct SameType<T,T> {
    enum {
        Result = true
    };
};

// you can use __is_base_of in MSCV 2008
template< typename BaseT, typename DerivedT >
struct IsRelated
{
    static DerivedT derived();
    static char test(const BaseT&); // sizeof(test()) == sizeof(char)
    static char (&test(...))[2];    // sizeof(test()) == sizeof(char[2])

	// true if conversion exists
    enum { exists = (sizeof(test(derived())) == sizeof(char)) }; 
};

// compile-time inheritance test, can be used to catch common typing errors
template< typename BASE, typename DERIVED >
inline void T_Check_Hierarchy()
{
	BASE* base;
	DERIVED* derived = static_cast<DERIVED*>( base );
	(void)derived;
}

template
<
	bool B,
	class T,
	class U
>
struct ConditionalType {
    typedef T Type;
};

template
<
	class T,
	class U
>
struct ConditionalType<false,T,U> {
    typedef U Type;
};

template< typename TYPE >
struct IncompleteType;


template< typename T >
inline const T& GetConstObjectReference( const T& o ) {
	return o;
}
template< typename T >
inline const T& GetConstObjectReference( const T* o ) {
	mxASSERT_PTR(o);
	return *o;
}
template< typename T >
inline const T* GetConstObjectPointer( const T& o ) {
	return &o;
}
template< typename T >
inline const T* GetConstObjectPointer( const T* o ) {
	return o;
}

template< typename T >
inline T& GetObjectReference( T& o ) {
	return o;
}
template< typename T >
inline T& GetObjectReference( T* o ) {
	mxASSERT_PTR(o);
	return *o;
}
template< typename T >
inline T* GetObjectPointer( T& o ) {
	return &o;
}
template< typename T >
inline T* GetObjectPointer( T* o ) {
	return o;
}


template< typename TYPE1, typename TYPE2 >
inline bool IsAlignedBy( TYPE1 x, TYPE2 alignment )
{
	const size_t remainder = (size_t)x % alignment;
	return remainder == 0;
}

template< UINT ALIGN, typename TYPE >
inline TYPE TAlignUp( TYPE x )
{
	mxSTATIC_ASSERT_ISPOW2(ALIGN);
	return (x + (ALIGN - 1)) & ~(ALIGN - 1);
}

template< UINT ALIGN, typename TYPE >
inline TYPE TAlignDown( TYPE x )
{
	mxSTATIC_ASSERT_ISPOW2(ALIGN);
	return x - (x & (ALIGN - 1));
}

template< typename TYPE1, typename TYPE2 >
inline size_t AlignUp( TYPE1 x, TYPE2 alignment )
{
	const size_t remainder = x % alignment;
	return remainder ? (x + (alignment - remainder)) : x;
}
