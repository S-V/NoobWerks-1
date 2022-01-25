/*
=============================================================================
	File:	Types.h
	Desc:	Definitions for common types for scalability, portability, etc.
	Note:	This file uses other base types defined
			in platform-specific headers.
=============================================================================
*/
#pragma once

/*
=============================================================================
	Low-level type definitions
=============================================================================
*/

/// The null pointer type.
#ifndef NULL
#define NULL	0
#endif


#ifdef HAVE_STDINT_H
# include <stdint.h>    /* C99 */
typedef uint8_t             U8;
typedef uint16_t            U16;
typedef uint32_t            U32;
typedef uint64_t            U64;
typedef int8_t              I8;
typedef int16_t             I16;
typedef int32_t             I32;
typedef int64_t             I64;
#else
typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long long  U64;
typedef signed char         I8;
typedef signed short        I16;
typedef signed int          I32;
typedef signed long long    I64;
#endif

typedef float F32;
typedef double F64;

//
//	Boolean types.
//

///
///	mxBool32 - guaranteed to be 32 bytes in size (for performance-related reasons).
///	Thus comparisons like bool32 == true will not work as expected.
///
typedef INT32	mxBool32;

///
///	FASTBOOL - 'false' is zero, 'true' is any non-zero value.
///	Thus comparisons like bool32 == true will not work as expected.
///
typedef signed int		FASTBOOL;

/// 'false' is zero, 'true' is one.
typedef unsigned int	mxBool01;



// must be signed!
typedef I32	MillisecondsT;

typedef I16	MillisecondsSInt16;
typedef U16	MillisecondsUInt16;

typedef U32	MillisecondsU32;



/*
=============================================================================
	Numeric Limits
=============================================================================
*/

/// Minimum 8-bit signed integer.
#define MIN_INT8	((UINT8)~MAX_INT8)
/// Maximum 8-bit signed integer.// 0x7F
#define	MAX_INT8	((INT8)(MAX_UINT8 >> 1))
/// Maximum 8-bit unsigned integer.// 0xFFU
#define	MAX_UINT8	((UINT8)~((UINT8)0))

/// Minimum 16-bit signed integer.
#define	MIN_INT16	((INT16)~MAX_INT16)
/// Maximum 16-bit signed integer.// 0x7FFF
#define	MAX_INT16	((INT16)(MAX_UINT16 >> 1))
/// Maximum 16-bit unsigned integer.// 0xFFFFU
#define	MAX_UINT16	((UINT16)~((UINT16)0))

/// Minimum 32-bit signed integer.
#define	MIN_INT32	((INT32)~MAX_INT32)
/// Maximum 32-bit signed integer.// 0x7FFFFFFF
#define	MAX_INT32	((INT32)(MAX_UINT32 >> 1))
/// Maximum 32-bit unsigned integer.// 0xFFFFFFFFU
#define	MAX_UINT32	((UINT32)~((UINT32)0))

/// Minimum 64-bit signed integer.
#define	MIN_INT64	((INT64)~MAX_INT64);
/// Maximum 64-bit signed integer.//0x7FFFFFFFFFFFFFFF
#define	MAX_INT64	((INT64)(MAX_UINT64 >> 1))
/// Maximum 64-bit unsigned integer.//0xFFFFFFFFFFFFFFFUL
#define	MAX_UINT64	((UINT64)~((UINT64)0))

#define MAX_FLOAT32		3.402823466e+38F
#define MIN_FLOAT32		1.175494351e-38F

#define MAX_FLOAT64		179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0
#define MIN_FLOAT64		2.225073858507201383090e-308

/// smallest positive number such that (1.0 + FLOAT_EPSILON) != 1.0
#define	FLOAT32_EPSILON		1.192092896e-07f
#define FLOAT64_EPSILON		2.220446049250313080847e-16

//-----------------------------------------------------------------------
//	Definitions of useful mathematical constants.
//-----------------------------------------------------------------------

//
// Pi
//
#define		mxPI			float( 3.1415926535897932384626433832795028841971693993751 )
#define		mxPI_f64		DOUBLE( 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798 )
#define		mxTWO_PI		float( 6.28318530717958647692528676655901 )	// pi * 2
#define		mxFOUR_PI		float( 12.566370614359172953850573533118 )	// pi * 4
#define		mxHALF_PI		float( 1.5707963267948966192313216916395 )	// pi / 2
#define		mxPI_DIV_4		float( 0.78539816339744830961566084581975 )	// pi / 4
#define		mxINV_PI		float( 0.31830988618379067153776752674508 )	// 1 / pi
#define		mxINV_TWOPI		float( 0.15915494309189533576888376337254 )	// 1 / (pi * 2)
#define		mx2_DIV_PI		float( 0.63661977236758134307553505349016 )	// 2 / pi
#define		mxSQRT_PI		float( 1.772453850905516027298167483341 )	// sqrt(pi)
#define		mxINV_SQRT_PI	float( 0.564189583547756286948f )			// 1 / sqrt(pi)
#define		mx2_SQRT_PI		float( 1.1283791670955125738961589031216 )	// 2 / sqrt(pi)
#define		mxPI_SQUARED	float( 9.869604401089358618834490999873 )	// pi * pi

#define		mxTANGENT_30	float( 0.57735026918962576450914878050196f )// tan(30)
#define		mx2_TANGENT_30	float( (mxTANGENT_30 * 2.0f)	// 2*tan(30)

//
// Golden ratio ( phi = float( 0.5 * (Sqrt(5) + 1) ).
//
#define		mxPHI			float( 1.61803398874989484820f )
#define		mxPHI_f64		DOUBLE( 1.6180339887498948482045868343656 )

#define		mxPHI_MINUS_1	float( 0.61803398874989484820f )

//
// e
//
#define		mxE				float( 2.71828182845904523536f )
#define		mxE_f64			DOUBLE( 2.7182818284590452353602874713526624977572470936999 )
#define		mxLOG2E			float( 1.44269504088896340736f )	// log2(e)
#define		mxLOG10E		float( 0.434294481903251827651f )	// log10(e)

#define		mxLN10			float( 2.30258509299404568402f )	// ln(10)

#define		mxINV_LOG2		( 3.32192809488736234787f )	// 1.0 / log10(2)
#define		mxLN2			( 0.69314718055994530941723212145818 )	// ln(2)
#define		mxINV_LN2		( 1.4426950408889634073599246810019 )	// 1.0f / ln(2)

#define		mxINV_3			float( 0.33333333333333333333f )	// 1/3
#define		mxINV_6			float( 0.16666666666666666666f )	// 1/6
#define		mxINV_7			float( 0.14285714285714285714f )	// 1/7
#define		mxINV_9			float( 0.11111111111111111111f )	// 1/9
#define		mxINV_255		float( 0.00392156862745098039f )	// 1/255

#define		mxSQRT_2		float( 1.4142135623730950488016887242097f )	// sqrt(2)
#define		mxINV_SQRT_2	float( 0.7071067811865475244008443621048490f )	// 1 / sqrt(2)

#define		mxSQRT_3		float( 1.7320508075688772935274463415059f )	// sqrt(3)
#define		mxINV_SQRT_3	float( 0.57735026918962576450f )	// 1 / sqrt(3)
#define		mxSQRT3_DIV_2	float( 0.86602540378443864676372317075294 )	// sqrt(3) / 2

#define		mxSQRT5			float( 2.236067977499789696409173668731f )

/// Euler's constant
#define		mxEULER			float( 0.5772156649015328606065 )

/// degrees to radians multiplier
#define		mxDEG2RAD		float( 0.0174532925199432957692369076848861 )	// mxPI / 180.0f

/// radians to degrees multiplier
#define		mxRAD2DEG		float( 57.2957795130823208767981548141052 )	// 180.0f / mxPI

/// seconds to milliseconds multiplier
#define		mxSEC2MS		1000

/// milliseconds to seconds multiplier
#define		mxMS2SEC		float( 0.001f )


#define mxSLERP_DELTA	0.999f

//--------------------------------------------------
//	Universal constants.
//--------------------------------------------------

//
// Atomic physics constants.
//
namespace KernPhysik
{
	/// Unified atomic mass.
	const double	u  = 1.66053873e-27f;

	/// Electron mass.
	const double	me = 9.109373815276e-31f;

	/// Electron charge.
	const double	e  = 1.602176462e-16f;

	/// Proton mass.
	const double	mp = 1.67262158e-27f;

	/// Neutron mass.
	const double	mn = 1.67492716e-27f;

};//End of KernPhysik

//
// Bulk physics constants.
//
namespace Physik
{
	/// Boltzmann constant
	const double k	= 1.3806503e-23f;

	/// Electric field constant / permittivity of free space
	const double e	= 8.854187817e-12f;

	/// Universal Gravitational Constant [m^3 kg^(-1) s^(-2)]
	const double G = 6.67384e-11;

	/// Impedance of free space
	const double Z	= 376.731f;

	/// Speed of light in vacuum
	const double c	= 2.99792458e8f;

	/// Magnetic field constant / Permeability of a vacuum
	const double mu	= 1.2566370614f;

	/// Planck constant
	const double h = 6.62606876e-34f;

	/// Stefan-Boltzmann constant
	const double sigma = 5.670400e-8f;

	/// Astronomical unit
	const double AU = 149.59787e11f;

	/// Standard gravity acceleration.
	const double EARTH_GRAVITY	= 9.81f;
	/// mass of the Earth [kg]
	const double EARTH_MASS = 5.97219e+24;
	/// radius of the Earth [m]
	const double EARTH_RADIUS = 6.3781e+6;

};//End of Physik

//
//	Conversions.
//
#define DEG2RAD( a )		( (a) * mxDEG2RAD )
#define RAD2DEG( a )		( (a) * mxRAD2DEG )

#define SEC2MS( t )			( mxFloatToInt( (t) * mxSEC2MS ) )
#define MS2SEC( t )			( mxIntToFloat( (t) ) * mxMS2SEC )

#define ANGLE2SHORT( x )	( mxFloatToInt( (x) * 65536.0f / 360.0f ) & 65535 )
#define SHORT2ANGLE( x )	( (x) * ( 360.0f / 65536.0f ) ); }

#define ANGLE2BYTE( x )		( mxFloatToInt( (x) * 256.0f / 360.0f ) & 255 )
#define BYTE2ANGLE( x )		( (x) * ( 360.0f / 256.0f ) )

//----------------------------------------------------------//

#define BIG_NUMBER			9999999.0f
#define SMALL_NUMBER		1.e-6f

/// Value indicating invalid index.
/// Use "-1" with unsigned values to indicate a bad/uninitialized state.
#define INDEX_NONE			INDEX_NONE_32
#define INDEX_NONE_SIZE_T	INDEX_NONE_64

#define INDEX_NONE_16	(UINT16)(~0u)
#define INDEX_NONE_32	(~0u)
#define INDEX_NONE_64	(~0ull)

/// ENoInit - Tag for suppressing initialization (also used to mark uninitialized variables).
enum ENoInit { _NoInit };

/// EInitDefault - means 'initialize with default value'.
enum EInitDefault { _InitDefault = 0 };

/// EInitZero - Used to mark data initialized with invalid values.
enum EInitInvalid { _InitInvalid = -1 };

/// EInitZero - Used to mark data initialized with zeros.
enum EInitZero { _InitZero = 0 };

/// EInitIdentity - Used to mark data initialized with default Identity/One value.
enum EInitIdentity { _InitIdentity = 1 };

/// EInitInfinity - Provided for convenience.
enum EInitInfinity { _InitInfinity = -1 };

/// for creating (slow) named constructors
enum EInitSlow { _InitSlow = -1 };

enum EInitCustom { _InitCustom };

// tag for in-place loading
struct _FinishedLoadingFlag { _FinishedLoadingFlag(){} };

enum EDontAddRef { _DontAddRef };
enum EDontCopy { _DontCopy };

/// for making "move" ctors in C++03
struct TagStealOwnership {};

// relative or absolute mode
enum ERelAbs
{
	kAbsolute,
	kRelative
};

///
///	'Bitfield' type.
///
typedef unsigned	BITFIELD;


///
/// The "empty Member" C++ Optimization
/// See:
/// http://www.cantrip.org/emptyopt.html
/// http://www.tantalon.com/pete/cppopt/final.htm#EmptyMemberOptimization
///
template< class Base, class Member >
struct BaseOpt : Base {
	Member m;
	BaseOpt( Base const& b, Member const& mem ) 
		: Base(b), m(mem)
	{ }
};




/// memory usage stats
struct MemStatsCollector
{
	size_t	staticMem;	// size of non-allocated memory, in bytes
	size_t	dynamicMem;	// size of allocated memory, in bytes

public:
	inline MemStatsCollector()
	{
		Reset();
	}
	inline void Reset()
	{
		staticMem = 0;
		dynamicMem = 0;
	}
};


//-----------------------------------------------------------------------


struct PtDateTime
{
	U8	year;	// years since 1900
	U8	month;	// months since January [0..11]
	U8	day;	// day of month [0..30]
	U8	hour;	// hours since midnight [0..23]
	U8	minute;	// [0..59]
	U8	second;	// [0..59]
public:
	PtDateTime( int year, int month, int day, int hour, int minute, int second )
	{
		this->year = year - 1900;
		this->month = month;
		this->day = day;
		this->hour = hour;
		this->minute = minute;
		this->second = second;
	}
};

inline
void ConvertMicrosecondsToMinutesSeconds(
	U64 microseconds,
	U32 &minutes, U32 &seconds
)
{
	const U64 totalSeconds = microseconds / (1000*1000);
	const U64 totalMinutes = totalSeconds / 60;

	minutes = totalMinutes;

	seconds = totalSeconds - totalMinutes * 60;
}
inline
void ConvertMicrosecondsToHoursMinutesSeconds(
	U64 microseconds,
	U32 &hours, U32 &minutes, U32 &seconds
)
{
	const U64 totalSeconds = microseconds / (1000*1000);
	const U64 totalMinutes = totalSeconds / 60;
	const U64 totalHours = totalMinutes / 60;

	hours = totalHours;
	minutes = totalMinutes - totalHours * 60;
	seconds = totalSeconds - totalMinutes * 60;
}

//
//	FCallback
//
typedef void FCallback( void* pUserData );


struct EmptyClass {};


struct AObjectHeader {
	void *	pVTBL;
};
struct VoidPointer {
	void *	o;
};
class PointerSizedPadding {
	void *	pad;
public:
	inline PointerSizedPadding() {
		pad = nil;
	}
};

struct SMemoryParam
{
	void *	data;
	U32	size;
};

// use for non-POD types that can be streamed via << and >> operators.
//
#define mxIMPLEMENT_SERIALIZE_FUNCTION( className, serializeFuncName )\
	inline mxArchive& serializeFuncName( mxArchive & serializer, className & o )\
	{\
		if( AWriter* saver = serializer.IsWriter() )\
		{\
			*saver << o;\
		}\
		if( AReader* Load = serializer.IsReader() )\
		{\
			*Load >> o;\
		}\
		return serializer;\
	}\
	inline mxArchive& operator && ( mxArchive & serializer, className & o )\
	{\
		return serializeFuncName( serializer, o );\
	}

// dummy function used for in-place saving/loading
#define mxNO_SERIALIZABLE_POINTERS\
	template< class S, class P  > inline void CollectPointers( S & s, P p ) const\
	{}

//
//	TStaticBlob<T>
//
template< typename TYPE >
class TStaticBlob
{
	BYTE	storage[ sizeof(TYPE) ];
public:
	inline TStaticBlob()			{}
	inline TStaticBlob(EInitZero)	{ mxZERO_OUT( storage ); }

	inline TYPE* Ptr()				{ return c_cast(TYPE*)&storage; }
	inline const TYPE* Ptr () const	{ return c_cast(TYPE*)&storage; }

	inline TYPE& Get()				{ return *Ptr(); }
	inline const TYPE& Get () const	{ return *Ptr(); }

	inline operator TYPE& ()				{ return Get(); }
	inline operator const TYPE& () const	{ return Get(); }

	inline TYPE* Construct()	{ return new (storage) TYPE(); }
	inline void Destruct()		{ Get().~TYPE(); }
};

//TAlignedBlob
// maybe, use something like std::aligned_storage< N * sizeof(T), alignof(T) > rawStorage; ?
// @todo: keep a bool flag, check for initialized on access via Get()/Ptr()?
//
template< typename TYPE >
class TStaticBlob16
{
	mxPREALIGN(16) BYTE storage[sizeof(TYPE)] mxPOSTALIGN(16);
public:
	inline TStaticBlob16()			{}
	inline TStaticBlob16(EInitZero)	{ mxZERO_OUT( storage ); }

	inline TYPE* Ptr()				{ return c_cast(TYPE*)&storage; }
	inline const TYPE* Ptr () const	{ return c_cast(TYPE*)&storage; }

	inline TYPE& Get()				{ return *Ptr(); }
	inline const TYPE& Get () const	{ return *Ptr(); }

	inline operator TYPE& ()				{ return Get(); }
	inline operator const TYPE& () const	{ return Get(); }

	inline TYPE* Construct()	{ return ::new (&storage) TYPE(); }
	inline void Destruct()		{ Get().~TYPE(); }
};

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// supply class name and ctor arguments
// it uses placement new on a static array to avoid dynamic memory allocations
// @TODO: use std::aligned_storage?
//
#define mxCONSTRUCT_IN_PLACE( TYPE, ... )\
	{\
		static TStaticBlob16< TYPE >	TYPE##storage;\
		new( &TYPE##storage ) TYPE( ## __VA_ARGS__ );\
	}

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// @TODO: use std::aligned_storage?
//
#define mxCONSTRUCT_IN_PLACE_X( PTR, TYPE, ... )\
	{\
		mxASSERT( PTR == nil );\
		static TStaticBlob16<TYPE>	TYPE##storage;\
		PTR = new( &TYPE##storage ) TYPE( ## __VA_ARGS__ );\
	}

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#if MX_DEBUG
	#define mxDECLARE_PRIVATE_DATA( STRUCT, VAR )	static TPtr< STRUCT >	VAR
	#define mxINITIALIZE_PRIVATE_DATA( VAR )		VAR.ConstructInPlace()
	#define mxSHUTDOWN_PRIVATE_DATA( VAR )			VAR.Destruct()
	#define mxGET_PRIVATE_DATA( STRUCT, VAR )		(*VAR)
#else
	#define mxDECLARE_PRIVATE_DATA( STRUCT, VAR )	static TStaticBlob16< STRUCT >	VAR
	#define mxINITIALIZE_PRIVATE_DATA( VAR )		VAR.Construct()
	#define mxSHUTDOWN_PRIVATE_DATA( VAR )			VAR.Destruct()
	#define mxGET_PRIVATE_DATA( STRUCT, VAR )		(*(STRUCT*) &(VAR))
#endif

//-----------------------------------------------------------------------

//
//	Triple
// 
template< typename T >
class Triple {
public:
	union {
		struct { T   iA, iB, iC; };
		struct { T   Points[ 3 ]; };
	};

	inline Triple()
	{}
	inline Triple( const T & A, const T & B, const T & C )
		: iA( A ), iB( B ), iC( C )
	{}
	inline T & operator [] ( UINT index )
	{
		//mxASSERT( index >= 0 && index <= 3 );
		return Points[ index ];
	}
	inline const T & operator [] ( UINT index ) const
	{
		//mxASSERT( index >= 0 && index <= 3 );
		return Points[ index ];
	}
};

/*
-------------------------------------------------------------------------
	NiftyCounter

	Intent:
		Ensure a non-local static object is initialized before its first use
		and destroyed only after last use of the object.

	Also Known As:
		Schwarz Counter.

	To do:
		Locking policies for thread safety.
-------------------------------------------------------------------------
*/
class NiftyCounter
{
	// this tells how many times the system has been requested to initialize
	// (think 'reference-counting')
	volatile int	m_count;

public:
	NiftyCounter()
	{
		m_count = 0;
	}
	~NiftyCounter()
	{
		//mxASSERT( m_count == 0 );
	}

	//-----------------------------------------------------
	// returns true if one time initialization is needed
	bool IncRef()
	{
		if( m_count++ > 0 )
		{
			// already initialized
			return false;
		}

		// has been initialized for the first time

		mxASSERT(m_count == 1);

		return true;
	}
	//-----------------------------------------------------
	// returns true if one time destruction is needed
	bool DecRef()
	{
		mxASSERT(m_count > 0);

		if( --m_count > 0 )
		{
			// still referenced, needs to stay initialized
			return false;
		}

		mxASSERT(m_count == 0);

		// no external references, can shut down

		return true;
	}
	//-----------------------------------------------------
	int GetCount() const
	{
		return m_count;
	}
	//-----------------------------------------------------
	bool IsOpen() const
	{
		return this->GetCount() > 0;
	}
};


//--------------------------------------------------------------------------

template< class TYPE >
struct TStaticCounter
{
	inline TStaticCounter()
	{
		ms__numInstances++;
		ms__maxInstances++;
	}
	inline ~TStaticCounter()
	{
		ms__numInstances--;
	}
	static inline UINT NumInstances() { return ms__numInstances; }
	static inline UINT MaxInstances() { return ms__maxInstances; }
private:
	static UINT	ms__numInstances;
	static UINT	ms__maxInstances;
};
template< typename TYPE >
UINT TStaticCounter< TYPE >::ms__numInstances = 0;
template< typename TYPE >
UINT TStaticCounter< TYPE >::ms__maxInstances = 0;


template< class TYPE >
struct TCountedObject
{
	inline TCountedObject()
		: m__serialNumber( ms__numInstances++ )
	{
	}

	const UINT	m__serialNumber;
private:
	static UINT	ms__numInstances;
};
template< typename TYPE >
UINT TCountedObject< TYPE >::ms__numInstances = 0;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Randy Gaul:
// Constructs a singly link list of any type of object. Objects are inserted into the
// list upon construction. Intended use with CRTP:

// Example:
// class Object: public AutoList<Object>
// {
// };

// The above is all that is needed to auto-generate a list of objects

template< typename T >
class TAutoList
{
	const T * const	_next;
public:
	TAutoList()
		: _next( Head() )
	{
		Head() = static_cast< const T* >( this );
	}
	inline const T* Next() const
	{
		return _next;
	}
	/// returns the global initialized static list
	static const T *& Head()
	{
		static const T* p = nullptr;
		return p;
	}
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
==========================================================
	This can be used to ensure that a particular function
	only gets called one time.
==========================================================
*/
#if MX_DEBUG

	#define DBG_ENSURE_ONLY_ONE_CALL					\
	{													\
		static bool Has_Already_Been_Called = false;	\
		if ( Has_Already_Been_Called )					\
		{												\
			mxUNREACHABLE2( "singleton error" );		\
		}												\
		Has_Already_Been_Called = true;					\
	}

#else

	#define DBG_ENSURE_ONLY_ONE_CALL

#endif //MX_DEBUG


//===========================================================================

template< class TYPE >
struct InstanceCounter
{
	static UINT TotalNumInstances()
	{
		return msTotalNumInstances;
	}

protected:
	InstanceCounter()
	{
		++msTotalNumInstances;
	}
	~InstanceCounter()
	{
		--msTotalNumInstances;
	}
private:
	static UINT msTotalNumInstances;
};

template< typename TYPE >
UINT InstanceCounter<TYPE>::msTotalNumInstances = 0;

//===========================================================================

inline UINT BitsToBytes( UINT numBits )
{
	return (numBits >> 3) + ((numBits & 7) ? 1 : 0);
}
inline UINT BitsToUInts( UINT numBits )
{
	return (numBits >> 5) + ((numBits & 31) ? 1 : 0);
}

template< typename TYPE >
static inline void SetUpperBit( TYPE & o ) {
	o |= (1 << (sizeof(TYPE)*BITS_IN_BYTE - 1));
}
template< typename TYPE >
static inline bool GetUpperBit( const TYPE o ) {
	return o & (1 << (sizeof(TYPE)*BITS_IN_BYTE - 1));
}
template< typename TYPE >
static inline void ClearUpperBit( TYPE & o ) {
	o &= ~(1 << (sizeof(TYPE)*BITS_IN_BYTE - 1));
}

/// stack-based allocation
#define mxSTACK_ALLOC(BYTES)	alloca(BYTES)

// Utility functions helpful when dealing with memory buffers and 
// pointers, especially when it is useful to go back and forth 
// between thinking of the buffer as bytes and as its type without
// a lot of casting.

// get the byte offset of B - A, as an int (64bit issues, so here for easy code checks)
inline U32 mxGetByteOffset32( const void* base, const void* pntr )
{
	const BYTE* from8 = (const BYTE*)base;
	const BYTE* to8 = (const BYTE*)pntr;
	const ptrdiff_t diff = to8 - from8;
	const U32 diff32 = (U32)diff;
#ifdef _M_X64
	mxASSERT( diff == (ptrdiff_t)diff32 );
#endif
	return diff32;
}

template< typename TYPE >
inline TYPE* mxAddByteOffset( TYPE* base, long offset )
{
	return reinterpret_cast<TYPE*>( reinterpret_cast<char*>(base) + offset );
}
template< typename TYPE >
inline const TYPE* mxAddByteOffset( const TYPE* base, long offset )
{
	return reinterpret_cast< const TYPE* >( reinterpret_cast< const char* >(base) + offset );
}

inline bool mxPointerInRange( const void* pointer, const void* start, const void* end )
{
	const BYTE* bytePtr = (const BYTE*) pointer;
	const BYTE* start8 = (const BYTE*) start;
	const BYTE* end8 = (const BYTE*) end;
	return (bytePtr >= start8) && (bytePtr < end8);
}
inline bool mxPointerInRange( const void* pointer, const void* start, size_t size )
{
	const BYTE* bytePtr = (const BYTE*) pointer;
	const BYTE* start8 = (const BYTE*) start;
	const BYTE* end8 = mxAddByteOffset( start8, size );
	return (bytePtr >= start8) && (bytePtr < end8);
}

//
// Test if two given memory areas are overlapping.
//
inline bool TestMemoryOverlap( const void* mem1, size_t size1, const void* mem2, size_t size2 )
{
	const BYTE* bytePtr1 = static_cast< const BYTE* >( mem1 );
	const BYTE* bytePtr2 = static_cast< const BYTE* >( mem2 );
	if ( bytePtr1 == bytePtr2 ) {
		return true;
	} else if ( bytePtr1 > bytePtr2 ) {
		return ( bytePtr2 + size2 ) > bytePtr1;
	} else {
		return ( bytePtr1 + size1 ) > bytePtr2;
	}
}

inline bool MemoryIsZero( const void* pMem, size_t size )
{
	const BYTE* ptr = (const BYTE*) pMem;
	while( size > 0 )
	{
		if( *ptr != 0 ) {
			return false ;
		}
		ptr++;
		size--;
	}
	return true;
}

//
//	IsValidAlignment
//
inline bool IsValidAlignment( size_t alignmentInBytes )
{
	return (alignmentInBytes >= MINIMUM_ALIGNMENT)
		&& (alignmentInBytes <= MAXIMUM_ALIGNMENT)
		&& (alignmentInBytes & (alignmentInBytes - 1)) == 0// alignment must be a power of two
		;
}

// Forces prefetch of memory.
inline void mxTouchMemory( void const* ptr )
{
	(void) *(char const volatile*) ptr;
}

#define IS_4_BYTE_ALIGNED( X )		(( (UINT_PTR)(X) & 3) == 0)
#define IS_8_BYTE_ALIGNED( X )		(( (UINT_PTR)(X) & 7) == 0)
#define IS_16_BYTE_ALIGNED( X )		(( (UINT_PTR)(X) & 15) == 0)
#define IS_32_BYTE_ALIGNED( X )		(( (UINT_PTR)(X) & 31) == 0)
#define IS_64_BYTE_ALIGNED( X )		(( (UINT_PTR)(X) & 63) == 0)
#define IS_ALIGNED_BY( X, bytes )	(( (UINT_PTR)(X) & ((bytes) - 1)) == 0)

/// aligns forward/up
#define tbALIGN( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) //  need macro for constant expression

#define tbALIGN2( len ) (( len+1)&~1) /// round up to 16 bits
#define tbALIGN4( len ) (( len+3)&~3) /// round up to 32 bits
#define tbALIGN8( len ) (( len+7)&~7) /// round up to 64 bits
#define tbALIGN16( len ) (( len + 15 ) & ~15 ) /// round up to 128 bits

namespace MemAlignUtil
{
	enum { alignment = 16 };	// 16 bytes

	inline size_t GetAlignedMemSize( size_t nBytes )
	{
		// we need to store a pointer to original (unaligned) memory block
		// so that we can free() it later
		// we can store it in the allocated memory block

		// Allocate a bigger buffer for alignment purpose,
		// stores the original allocated address just before the aligned buffer for a later call to free().
		const size_t expandSizeBytes = nBytes + (sizeof(void*) + (alignment-1));
		return expandSizeBytes;
	}

	inline void* GetAlignedPtr( void* allocatedMem )
	{
		BYTE* rawAddress = c_cast(BYTE*) allocatedMem;
		BYTE* alignedAddress = c_cast(BYTE*) (size_t(rawAddress + alignment + sizeof(void*)) & ~(alignment-1));

		(c_cast(void**)alignedAddress)[-1] = rawAddress;	// store pointer to original (allocated) memory block

		return alignedAddress;	// return aligned pointer
	}

	inline void* GetUnalignedPtr( void* alignedPtr )
	{
		// We need a way to go from our aligned pointer to the original pointer.
		BYTE* alignedAddress = c_cast(BYTE*) alignedPtr;

		// To accomplish this, we store a pointer to the memory returned by malloc
		// immediately preceding the start of our aligned memory block.
		BYTE* rawAddress = c_cast(BYTE*) (c_cast(void**)alignedAddress)[-1];

		return rawAddress;
	}
}


//----------------------------------------------------------//
//	String constants
//----------------------------------------------------------//

// "Unknown"
extern const char* mxSTRING_Unknown;

// string denoting unknown error
extern const char* mxSTRING_UNKNOWN_ERROR;

// "$"
extern const char* mxSTRING_DOLLAR_SIGN;
extern const char* mxSTRING_QUESTION_MARK;

// ""
extern const char* mxEMPTY_STRING;


enum { MAX_PRINTF_CHARS = 1024 };


//----------------------------------------------------------//
//	String functions
//----------------------------------------------------------//

/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

/// Cross platform implementation of vsnprintf that returns number of
/// characters which would have been written to the final string if
/// enough space had been available.
inline int My_vsnprintf(char* _str, size_t _count, const char* _format, va_list _argList)
{
#if mxCOMPILER == mxCOMPILER_MSVC
	int len = ::vsnprintf_s(_str, _count, _TRUNCATE, _format, _argList);
	return (len < 0) ? ::_vscprintf(_format, _argList) : len;
#else
	return ::vsnprintf(_str, _count, _format, _argList);
#endif // mxCOMPILER == mxCOMPILER_MSVC
}

/// Cross platform implementation of My_vsnwprintf that returns number of
/// characters which would have been written to the final string if
/// enough space had been available.
inline int My_vsnwprintf(wchar_t* _str, size_t _count, const wchar_t* _format, va_list _argList)
{
#if BX_COMPILER_MSVC
	int len = ::_vsnwprintf_s(_str, _count, _count, _format, _argList);
	return -1 == len ? ::_vscwprintf(_format, _argList) : len;
#elif defined(__MINGW32__)
	return ::My_vsnwprintf(_str, _count, _format, _argList);
#else
	return ::vswprintf(_str, _count, _format, _argList);
#endif // BX_COMPILER_MSVC
}

inline int snprintf(char* _str, size_t _count, const char* _format, ...) // BX_PRINTF_ARGS(3, 4)
{
	va_list argList;
	va_start(argList, _format);
	int len = My_vsnprintf(_str, _count, _format, argList);
	va_end(argList);
	return len;
}

inline int swnprintf(wchar_t* _out, size_t _count, const wchar_t* _format, ...)
{
	va_list argList;
	va_start(argList, _format);
	int len = My_vsnwprintf(_out, _count, _format, argList);
	va_end(argList);
	return len;
}

//
//	mxSafeGetVarArgsANSI - returns the number of characters written or 0 if truncated.
//	appends NULL to the output buffer.
//
inline int SafeGetVarArgsANSI( ANSICHAR *outputBuffer, size_t maxChars, const ANSICHAR* formatString, va_list argList )
{
	mxASSERT( outputBuffer );
	mxASSERT( maxChars > 0 );
	mxASSERT( formatString );
	mxASSERT( argList );

	// returns the number of characters written, not including the terminating null, or a negative value if an output error occurs.
	// causes a crash in case of buffer overflow
	int result = _vsnprintf_s(
		outputBuffer,
		maxChars * sizeof(outputBuffer[0]),
		maxChars,
		formatString,
		argList
		);

	return result;
}

template< int SIZE >
int tMy_sprintfA( char (&buf)[SIZE], const char* fmt, ... )
{
	va_list	 argPtr;
	va_start( argPtr, fmt );
	int len = My_vsnprintf( buf, mxCOUNT_OF(buf), fmt, argPtr );
	va_end( argPtr );
	return len;
}

template< int SIZE >
int tMy_sprintfW( wchar_t (&buf)[SIZE], const wchar_t* fmt, ... )
{
	va_list	 argPtr;
	va_start( argPtr, fmt );
	int len = My_vsnwprintf( buf, mxCOUNT_OF(buf), fmt, argPtr );
	va_end( argPtr );
	return len;
}

// Parameters:
//    FMT - const char*
//    ARGS - va_list
//    XXX - user statement
#define ptPRINT_VARARGS_BODY( FMT, ARGS, XXX )\
	{\
		char	tmp_[2048];\
		char *	ptr_ = tmp_;\
		int		len_ = My_vsnprintf( ptr_, mxCOUNT_OF(tmp_), FMT, ARGS );\
		/* len_ is the number of characters that would have been written,*/\
		/* not counting the terminating null character.*/\
		if( len_+1 > mxCOUNT_OF(tmp_) )\
		{\
			ptr_ = (char*) alloca(len_+1);\
			len_ = My_vsnprintf( ptr_, len_+1, FMT, ARGS );\
		}\
		ptr_[len_] = '\0';\
		XXX;\
	}\

//
//	mxGET_VARARGS_A where fmt is (char* , ...)
//
#define mxGET_VARARGS_A( buffer, fmt )\
	{\
		va_list	 argPtr;\
		va_start( argPtr, fmt );\
		SafeGetVarArgsANSI( buffer, mxCOUNT_OF(buffer), fmt, argPtr );\
		va_end( argPtr );\
	}


#if mxPLATFORM == mxPLATFORM_WINDOWS

//
//	PtUnicodeToAnsi
//
//	Converts UNICODE string to ANSI string.
//
//	pSrc [in] : Pointer to the wide character string to convert.
//	pDest [out] : Pointer to a buffer that receives the converted string.
//	destSize [in] : Size, in bytes, of the buffer indicated by pDest.
//
inline ANSICHAR* PtUnicodeToAnsi( ANSICHAR* pDest, const UNICODECHAR* pSrc, int destChars )
{
	::WideCharToMultiByte(
		CP_ACP,				// UINT     CodePage, CP_ACP - The system default Windows ANSI code page.
		0,					// DWORD    dwFlags
		pSrc,				// LPCWSTR  lpWideCharStr
		-1,					// int      cchWideChar
		pDest,				// LPSTR   lpMultiByteStr
		destChars,			// int      cbMultiByte
		NULL,				// LPCSTR   lpDefaultChar
		NULL				// LPBOOL  lpUsedDefaultChar
	);
	return pDest;
}

//
//	PtAnsiToUnicode
//
//	Converts ANSI string to UNICODE string.
//
inline UNICODECHAR* PtAnsiToUnicode( UNICODECHAR* pDest, const ANSICHAR* pSrc, int destChars )
{
	::MultiByteToWideChar(
		CP_ACP,				// UINT     CodePage, CP_ACP - The system default Windows ANSI code page.
		0,					// DWORD    dwFlags
		pSrc,				// LPCSTR   lpMultiByteStr
		-1,					// int      cbMultiByte, -1 means null-terminated source string
		pDest,				// LPWSTR  lpWideCharStr
		destChars			// int      cchWideChar
	);
	return pDest;
}

inline int GetLengthUnicode( const UNICODECHAR* pStr )
{
	return ::WideCharToMultiByte( CP_ACP, 0, pStr, -1, NULL, 0, NULL, NULL );
}

// converts ANSI string to UNICODE string; don't use for big strings or else it can crash;
// FIXME: the passed string's length is evaluated twice
//
#define mxTO_UNICODE( pSrc )\
	PtAnsiToUnicode(\
		(UNICODECHAR*)_alloca(sizeof(UNICODECHAR) * strlen(pSrc)),\
		(const ANSICHAR*)pSrc,\
		strlen(pSrc) )

// converts UNICODE string to ANSI string; don't use for big strings or else it can crash;
// FIXME: the passed string's length is evaluated twice
//
#define mxTO_ANSI( pSrc )\
	PtUnicodeToAnsi(\
		(ANSICHAR*)_alloca(sizeof(ANSICHAR) * GetLengthUnicode(pSrc)),\
		(const UNICODECHAR*)pSrc,\
		GetLengthUnicode(pSrc) )

#if UNICODE
	#define mxCHARS_AS_ANSI( pSrc )		mxTO_ANSI( pSrc )
	#define mxCHARS_AS_UNICODE( pSrc )	pSrc
#else
	#define mxCHARS_AS_ANSI( pSrc )		pSrc
	#define mxCHARS_AS_UNICODE( pSrc )	mxTO_UNICODE( pSrc )
#endif

#endif // mxPLATFORM == mxPLATFORM_WINDOWS

struct FileLine
{
	const char*	file;
	int			line;
};

template< class TYPE >
TYPE CalculateMaximum( const TYPE* _array, const UINT _count )
{
	TYPE maximumValue( 0 );
	for( UINT i = 0; i < _count; i++ )
	{
		if( maximumValue < _array[i] ) {
			maximumValue = _array[i];
		}
	}
	return maximumValue;
}

/// e.g. for DynamicArray< char >
#define GET_ALIGNMENT_FOR_CONTAINERS(X)		(largest(mxALIGNOF(X), MINIMUM_ALIGNMENT))

/// name hashes are used throughout the project
typedef U32 NameHash32;




template< typename FIRST, typename SECOND >
struct TPair {
	FIRST	first;
	SECOND	second;
};


/// C++03 analog of std::aligned_storage<>,
/// it is used to reduce dynamic memory allocations in containers.
template< typename T, int COUNT >
struct TReservedStorage//: StaticMem< sizeof(T) * COUNT, mxALIGNOF(T) >
{
	// MSVC 2008 doesn't allow __declspec( align( __alignof(MyType) ) )
	// https://stackoverflow.com/questions/5134217/aligning-data-on-the-stack-c
	//mxPREALIGN(mxALIGNOF(T)) BYTE data[ sizeof(T) * COUNT ];

	// let's just use EFFICIENT_ALIGNMENT=16
	mxPREALIGN(16) BYTE data[ sizeof(T) * COUNT ];

public:
	template< class O >
	mxFORCEINLINE O* getMemoryForObject()
	{
		mxSTATIC_ASSERT( sizeof(O) <= sizeof(data) );
		return reinterpret_cast< O* >( data );
	}

	mxFORCEINLINE T* asTypedArray() { return reinterpret_cast< T* >( data ); }
	mxFORCEINLINE int capacity() const { return COUNT; }
};

/// in milliseconds
typedef U64 IntegerTimestampT;

/// for passing to functions
struct DebugParam
{
	bool	flag;
public:
	DebugParam(bool flag_value = false)
		: flag(flag_value)
	{
	}
};




// STL iterator support.

#ifndef _HAS_CPP0X

// C++11
namespace std
{
	template< typename C, size_t sz >
	C* begin(C (&ctr)[sz]) { return &ctr[0]; }

	template< typename C, size_t sz >
	const C* begin(const C (&ctr)[sz]) { return &ctr[0]; }

	template< typename C, size_t sz >
	C* end(C (&ctr)[sz]) { return &ctr[0] + sz; }

	template< typename C, size_t sz >
	const C* end(const C (&ctr)[sz]) { return &ctr[0] + sz; }
}//namespace std

#endif
