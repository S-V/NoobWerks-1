/*
=============================================================================
	File:	Helpers.h
	Desc:	(Mostly) platform-agnostic macros and miscellaneous helper stuff
			that is used throughout the engine.
	Note:	Some of these tricks won't be needed when C++0x becomes widespread.
=============================================================================
*/
#pragma once

#if mxCPP_VERSION < mxCPP_VERSION_11
	#ifndef constexpr
		#define constexpr const
	#endif

	#define nullptr	(0)

	#define noexcept throw()
#endif

/// this was introduced to do less typing
#define nil		nullptr

/// macro for converting pointers to boolean values
#if CPP_0X
	#define PtrToBool(pointer)	((pointer) != nil)
#else
	#define PtrToBool(pointer)	(pointer)
#endif


/*
====================================================================================
These are commonly used to define multi-statement (multi-line) macros in C/C++
(that are going to be used in function-scope).

e.g.

Suppose, we #define the following:

#define FUNC(X) Something1; \
				Something2

If it's used like this...

	if( a == 0 ) 
		FUNC(X);

...then we have a bug:

	'Something1' will be executed only if the condition is true,
	but 'Something2' will ALWAYS be run!

To protect against these errors such can be used:

#define FUNC(X)\
do{\
statement;\
...\
statement;\
}while(0)

====================================================================================
*/
#define mxMACRO_BEGIN	do{
#define mxMACRO_END		}while(0)

//
//	mxCOUNT_OF
// Return the number of elements in a statically sized array.
//   DWORD Buffer[100];
//   RTL_NUMBER_OF(Buffer) == 100
// This is also popularly known as: mxCOUNT_OF, ARRSIZE, mxCOUNT_OF, _countof, NELEM, etc.
// But it can lead to the following error:
// void Test(int C[3])
// {
//   int A[3];
//   int *B = Foo();
//   size_t x = count_of(A); // Ok
//   x = count_of(B); // Error
//   x = count_of(C); // Error
// }
// From "WinNT.h" :
//
// TCountOf is a function that takes a reference to an array of N Ts.
//
// typedef T array_of_T[N];
// typedef array_of_T &reference_to_array_of_T;
//
// TCountOf returns a pointer to an array of N chars.
// We could return a reference instead of a pointer but older compilers do not accept that.
//
// typedef char array_of_char[N];
// typedef array_of_char *pointer_to_array_of_char;
//
// sizeof(array_of_char) == N
// sizeof(*pointer_to_array_of_char) == N
//
// pointer_to_array_of_char TCountOf(reference_to_array_of_T);
//
// We never even call TCountOf, we just take the size of dereferencing its return type.
// We do not even implement TCountOf, we just declare it.
//
// Attempts to pass pointers instead of arrays to this macro result in compile time errors.
// That is the point.
//
extern "C++" // templates cannot be declared to have 'C' linkage
template< typename T, size_t N >	// accepts a reference to an array of N T's 
char (*ArraySizeHelper( /*UNALIGNED*/ T (&)[N] ))[N];	// returns a pointer to an array of N chars

#define mxCOUNT_OF(A)	(sizeof(*ArraySizeHelper(A)))

//
// This does not work with:
//
// void Foo()
// {
//    struct { int x; } y[2];
//    mxCOUNT_OF(y); // illegal use of anonymous local type in template instantiation
// }
//
// You must instead do:
//
// struct Foo1 { int x; };
//
// void Foo()
// {
//    Foo1 y[2];
//    mxCOUNT_OF(y); // ok
// }
//
// OR
//
// void Foo()
// {
//    struct { int x; } y[2];
//    mxCOUNT_OF(y); // ok
// }
//
// OR
//
// void Foo()
// {
//    struct { int x; } y[2];
//    mxCOUNT_OF(y); // ok
// }
//
/*
template< typename T, size_t N >
size_t CountOf( const T (&) [N] )	{ return N; }
*/
/*
	This comes from Havok:

	struct hkCountOfBadArgCheck
	{
		class ArgIsNotAnArray;
		template<typename T> static ArgIsNotAnArray isArrayType(const T*, const T* const*);
		static int isArrayType(const void*, const void*);
	};

		/// Returns the number of elements in the C array.
	#define HK_COUNT_OF(x) ( \
		0 * sizeof( reinterpret_cast<const ::hkCountOfBadArgCheck*>(x) ) + \
		0 * sizeof( ::hkCountOfBadArgCheck::isArrayType((x), &(x)) ) + \
		sizeof(x) / sizeof((x)[0]) ) 
*/

/*
See: http://www.codersnotes.com/notes/easy-preprocessor-defines

Example usage:

#define BIG_ENDIAN	mxPP_ON
#if mxPP_USING( BIG_ENDIAN )
// whatever
#endif
*/
#define mxPP_ON				2-
#define mxPP_OFF			1-
#define mxPP_USING( X )		( (X 0) == 2 )

//
//	SIZE_OF_THIS
//
#define SIZE_OF_THIS	sizeof(*this)

//
//	OFFSET_OF( _struct, member ) - The byte offset of a field in a structure.
//
#define OFFSET_OF( _struct, member )		((long)(long*)&((_struct*)0)->member)

// A check for whether the offset of a member within a structure is as expected
#define OFFSET_EQUALS(CLASS,MEMBER,OFFSET) (OFFSET_OF(CLASS,MEMBER)==OFFSET)

//
//	FIELD_SIZE( _struct, member ) - The size of a field in a structure.
//
#define FIELD_SIZE( _struct, member )		sizeof( ((_struct*)0)->member )

//===========================================================================
//
// mxGET_OUTER( OuterType, OuterMember )
//
// A platform-independent way for a contained class to get a pointer to its
// owner. If you know a class is exclusively used in the context of some
// "outer" class, this is a much more space efficient way to get at the outer
// class than having the inner class store a pointer to it.
//
//	This comes from Source Engine, Valve.
//
//	class COuter
//	{
//		class CInner // Note: this does not need to be a nested class to work
//		{
//			void PrintAddressOfOuter()
//			{
//				printf( "Outer is at 0x%x\n", mxGET_OUTER( COuter, m_Inner ) );
//			}
//		};
//		
//		CInner m_Inner;
//		friend class CInner;
//	};

#define mxGET_OUTER( OuterType, OuterMember ) \
	( ( OuterType * ) ( (char*)this - OFFSET_OF( OuterType, OuterMember ) ) )

#define mxGET_OUTER2( OUTER_TYPE, MEMBER_FIELD_VAR, MEMBER_FIELD_PTR )\
	( (OUTER_TYPE*) ( (char*)(MEMBER_FIELD_PTR) - OFFSET_OF(OUTER_TYPE, MEMBER_FIELD_VAR) ) )

/*
============================================================================
//	mxBASE_OFFSET( CLASS, BASE_KLASS )
//
//	calculates offset of the base class relative to the derived class.
//	
//	NOTE: turned out to be too complex for a compile-time expression when tested on MVC++ 2008.
//	it probably doesn't work on other compilers.
//	
//	Author: this is my own shit (offsets into POD structs, multiple inheritance, ...).
//	
//	Example:
//	
//	struct Base
//	{
//		int baseMember;	// 4 bytes
//		// 4 bytes
//	};
//	struct A
//	{
//	int a_0;	// 4 bytes
//		int a_1[3];	// 12 bytes
//		// 16 bytes
//	};
//	struct B
//	{
//		int b_0;	// 4 bytes
//		// 4 bytes
//	};
//	struct C
//	{
//		int c_0;	// 4 bytes
//		int c_1;	// 4 bytes
//		// 8 bytes
//	};
//	struct Base_A_B : Base, A, B
//	{
//		int		xxx;
//	};
//	struct Base_A_B__C : Base_A_B, C
//	{
//		float	crap[5];
//	};
//	
//	void TestBaseOffsetsMacro()
//	{
//		Base_A_B__C		o;
//	
//		Base_A_B__C* pO = &o;
//
//		A* pA = (A*)pO;
//		const size_t offsetToA = size_t((char*)pA - (char*)pO);	// 4
//	
//		const size_t offset_to_Base = mxBASE_OFFSET( Base_A_B__C, Base );
//		assert( offset_to_Base == 0 );
//	
//		const size_t offset_to_A = mxBASE_OFFSET( Base_A_B__C, A );
//		assert( offset_to_A == 4 );
//	
//		const size_t offset_to_B = mxBASE_OFFSET( Base_A_B__C, B );
//		assert( offset_to_B == 20 );
//	
//		const size_t offset_to_C = mxBASE_OFFSET( Base_A_B__C, C );
//		assert( offset_to_C == 28 );
//	
//		const size_t offset_to_Base_A_B = mxBASE_OFFSET( Base_A_B__C, Base_A_B );
//		assert( offset_to_Base_A_B == 0 );
//	
//		const size_t offset_to_B_in_Base_A_B = mxBASE_OFFSET( Base_A_B, B );
//		assert( offset_to_B_in_Base_A_B == 20 );
//	
//		//mxSTATIC_CHECK( mxBASE_OFFSET( Base_A_B__C, Base ) == 0 );
//	}
============================================================================
*/

//#define mxBASE_OFFSET( CLASS, BASE_KLASS )\
//	int( (char*)(BASE_KLASS*)&(*(CLASS*)16) - (char*)((CLASS*)16)  )

#define mxBASE_OFFSET( CLASS, BASE_KLASS )\
	(size_t)( reinterpret_cast< const char* >( (BASE_KLASS*)&(*(CLASS*)16) ) - reinterpret_cast< const char* >((CLASS*)16) )


//-------------------------------------------------------------

/// Fields that can be accessed by multiple threads simultaneously
/// should use the whole cache line to avoid false sharing.
#define mxPAD_SIZE_TO_CACHELINE( SIZE )\
	(mxCACHE_LINE_SIZE - (SIZE) % mxCACHE_LINE_SIZE)

#define mxPAD_TO_CACHELINE( TYPE )\
	char PP_JOIN(__pad,__LINE__) [ mxPAD_SIZE_TO_CACHELINE(sizeof(TYPE)) ]

#define mxALIGN_BY_CACHELINE	__declspec(align(mxCACHE_LINE_SIZE))



#if mxARCH_TYPE != mxARCH_64BIT
	#define PAD_POINTER_TO_64BIT (4)
#else
	#define PAD_POINTER_TO_64BIT (0)
#endif

//-------------------------------------------------------------
/*
//
// Bit - single-bit bitmask.
//
template< size_t N >
class Bit {
	enum { mask = (1 << N) };
};
*/
#define BIT(n)		(1U << (n))
#define BIT64(n)	(1ULL << (n))

#define BITS_NONE	(0)
#define BITS_ALL	(~0)

#define BITS_IN_BYTE		(8)
#define BYTES_TO_BITS(x)	((x)<<3)
#define BITS_TO_BYTES(x)	(((x)+7)>>3)

#define TEST_BIT( x, bit )		((x) & (bit))
#define SET_BITS( x, bits )		((x) = (x) | (bits))
#define CLEAR_BITS( x, bits )	((x) = (x) & ~(bits))
#define FLIP_BITS( x, bits )	((x) = (x) ^ (bits))

//#define SET_FLAGS( x, bits, cond )	do{ if((cond)) (x) |= (bits); else (x) &= ~(bits); }while(0)
template< typename T, typename U >
inline void SET_FLAGS( T & _destination, U _flags, bool _condition ) {
	if( _condition ) {
		_destination |= _flags;
	} else {
		_destination &= ~_flags;
	}
}

//-------------------------------------------------------------



//
//	TPow2 - Template to compute 2 to the Nth power at compile-time.
//
template< unsigned N >
struct TPow2
{
	//enum { value = 2 * TPow2<N-1>::value };
	//changed enums to longs because of MVC++ warning: C4307: '*' : integral constant overflow.
	static const unsigned long long value = 2 * TPow2<N-1>::value;
};
template<>
struct TPow2<0>
{
	//enum { value = 1 };
	static const unsigned long long  value = 1;
};

//
//	TIsPowerOfTwo
//
template< unsigned long x >
struct TIsPowerOfTwo
{
	enum { value = (((x) & ((x) - 1)) == 0) };
};

//
//	TLog2
//
template< unsigned long NUM >
struct TLog2
{
	enum {
		value = TLog2<NUM / 2>::value + 1
	};
	typedef char NUM_MustBePowerOfTwo[((NUM & (NUM - 1)) == 0 ? 1 : -1)];
};
template<> struct TLog2< 1 >
{
	enum { value = 0 };
};

//-------------------------------------------------------------

#define mxARGB(a,r,g,b)			((unsigned)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define mxRGBA(r,g,b,a)			mxARGB(a,r,g,b)
#define mxXRGB(r,g,b)			mxARGB(0xff,r,g,b)
/// maps floating point channels (0.f to 1.f range) to unsigned 8-bit color values
#define mxCOLORVALUE(r,g,b,a)	mxRGBA((unsigned)((r)*255.f),(unsigned)((g)*255.f),(unsigned)((b)*255.f),(unsigned)((a)*255.f))


#define MCHAR2( a, b )			( a | (b << 8) )
#define MCHAR4( a, b, c, d )	( a | (b << 8) | (c << 16) | (d << 24) )

// for code formatting purposes
#ifdef ___
#	error ___ mustn't be defined!
#endif
#define ___


// Automatically converts static string to multi-byte or Unicode
#if MX_UNICODE
	#define mxTEXT(X)	L##X
#else
	#define mxTEXT(X)	X
#endif

// Converts static string from ANSI char to wide char type.
#define WIDEN2(x)		L ## x
#define WIDEN(x)		WIDEN2(x)


// The "" str part only works if str is an actual string literal.
// If something other than a literal is passed compilation fails.
// see: http://mortoray.com/2014/05/29/wait-free-queueing-and-ultra-low-latency-logging/
#define mxSAFE_LITERAL( str )	"" str

#define __WFILE__		WIDEN(__FILE__)
#define __WFUNCTION__	WIDEN(__FUNCTION__)


#define GET_VERSION_MAJOR(version) (version >> 8)
#define GET_VERSION_MINOR(version) (version & 0xFF)
#define MAKE_VERSION(major, minor) (((major)<<8) + (minor))
#define GET_VERSION(version, major, minor) ( (major) = GET_VERSION_MAJOR(version); (minor) = GET_VERSION_MINOR(version); )


//! Evaluates to the high byte of x. x is expected to be of type U16.
#define HI_BYTE(x) (BYTE)((x) >> 8)

//! Evaluates to the low byte of x. x is expected to be of type U16.
#define LO_BYTE(x) (BYTE)((x) & 0xff)

//! Evaluates to the 16 bit value specified by x and y in little endian order (low, high).
#define LO_HI(x,y) (UINT16)((y) << 8 | (x))


#if defined(smallest) || defined(largest)
#error Shouldn't happen
#endif

#undef smallest
#define smallest(a,b)            (((a) < (b)) ? (a) : (b))

#undef largest
#define largest(a,b)            (((a) > (b)) ? (a) : (b))

//-------------------------------------------------------------

//#define OVERRIDES( BaseClassName )	virtual
//#define OVERRIDEN					virtual

//// Microsoft Visual C++ compiler has a special keyword for this purpose - '__super'.
//#define DECLARE_CLASS( className, baseClassName )	\
//	typedef className THIS_TYPE;	\
//	typedef baseClassName BASE_TYPE;	\
//	typedef baseClassName Super;


/// it says: 'unsure about whether this function should be inlined or not'
#define mxINLINE	inline

//-------------------------------------------------------------
//	Usage:
//		NO_COPY_CONSTRUCTOR( ClassName );
//		NO_ASSIGNMENT( ClassName );
//		NO_COMPARES( ClassName );
//

#define NO_DEFAULT_CONSTRUCTOR( ClassName )\
	protected: ClassName() {}

#define NO_COPY_CONSTRUCTOR( ClassName );	\
	private: ClassName( const ClassName & );	// Do NOT implement!

#define NO_ASSIGNMENT( ClassName );			\
	private: ClassName & operator = ( const ClassName & );	// Do NOT implement!

#define NO_COMPARES( ClassName );			\
	private: ClassName & operator == ( const ClassName & );	// Do NOT implement!

#define PREVENT_COPY( ClassName );			\
	NO_COPY_CONSTRUCTOR( ClassName );		\
	NO_ASSIGNMENT( ClassName );				\
	NO_COMPARES( ClassName );

//-------------------------------------------------------------

//
//	Utility class, you can inherit from this class
//	to disallow the assignment operator and copy construction.
//  Private copy constructor and copy assignment ensure classes derived from
//  class noncopyable cannot be copied.
//
struct NonCopyable {
protected:
	NonCopyable() {}
	~NonCopyable() {}
private:  // emphasize the following members are private
	NonCopyable( const NonCopyable& );
	const NonCopyable& operator=( const NonCopyable& );
};

class NonInstantiable {
private:  // emphasize the following members are private
	NonInstantiable() {}
	~NonInstantiable() {}
	NonInstantiable( const NonInstantiable& );
	const NonInstantiable& operator=( const NonInstantiable& );
};

//-----------------------------------------------------------------------
// Quick const-manipulation macros

// Declare a const and variable version of a function simultaneously.
#define CONST_VAR_FUNCTION(head, body) \
	inline head body \
	inline const head const body

template<class T> inline
T& make_non_const(const T& t)
	{ return const_cast<T&>(t); }

#define using_type(super, type) \
	typedef typename super::type type;


//-------------------------------------------------------------
//	Preprocessor library.
//-------------------------------------------------------------

// Convert the expanded result of a macro to a char string.
// Expands the expression before stringifying it. See:
// http://c-faq.com/ansi/stringize.html
#define TO_STR2(X)		#X

// Creates a string from the given expression ('Stringize','Stringify').
// stringizes even macros
//
#define TO_STR(X)		TO_STR2(X)


#define STRINGIZE_INDIRECT( F, X )	F(X)
#define __LINESTR__					STRINGIZE_INDIRECT(TO_STR, __LINE__)
#define __FILELINEFUNC__			(__FILE__ " " __LINESTR__ " " __FUNCTION__)

//#define __FUNCLINE__				( __FUNCTION__ " " __LINESTR__ )
#define __FUNCLINE__				( __FUNCTION__ " (" __LINESTR__ ")" )



// Join two preprocessor tokens, even when a token is itself a macro (see sec 16.3.1 in the C++ standard).
#define PP_JOIN(A,B) PP_JOIN2(A,B)
#define PP_JOIN2(A,B) PP_JOIN3(A,B)
// PREPROCESSOR: concatenates two strings, even when the strings are macros themselves
#define PP_JOIN3(A,B) A##B

// puts a slash before the given symbol, needed for the IF_DEBUG macro to work
#define mxPT_SLASH( c )		/##c

/*
// Counts the number of arguments in the given variadic macro.
// The C Standard specifies that at least one argument must be passed to the ellipsis,
// to ensure that the macro does not resolve to an expression with a trailing comma.
// Note: ##__VA_ARGS__ eats comma in case of empty arg list
// works for [1 - 16], doesn't work if num args is zero
//
#define VA_NUM_ARGS(...)			VA_NUM_ARGS_IMPL_((__VA_ARGS__, 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1))
#define VA_NUM_ARGS_IMPL_(tuple)	VA_NUM_ARGS_IMPL tuple
#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16, N, ... )	N

// how it works:
//	VA_NUM_ARGS(...)
//	e.g.
//	VA_NUM_ARGS(x1,x2,x3) -> VA_NUM_ARGS_IMPL_( x1,x2,x3, 5,4,3,2,1 )
//	-> VA_NUM_ARGS_IMPL x1,x2,x3, 5,4,3,2,1	// _1,_2,_3,_4,_5, stand for x1,x2,x3, 5,4, => N is 3
//	-> 3

// it could be done simply as
#define MX_VA_NUM_ARGS(...)                        MX_VA_NUM_ARGS_HELPER(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define MX_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
*/

// Unfortunately, even Visual Studio 2010 has this bug
// which treats a __VA_ARGS__ argument as being one single parameter.
// Hence, we have to work around this bug using the following piece of macro magic:

// MX_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro
// unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
// https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
#if _MSC_VER >= 1400
#    define MX_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
#    define MX_VA_NUM_ARGS_REVERSE_SEQUENCE            10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define MX_LEFT_PARENTHESIS (
#    define MX_RIGHT_PARENTHESIS )
#    define MX_VA_NUM_ARGS(...)                        MX_VA_NUM_ARGS_HELPER MX_LEFT_PARENTHESIS __VA_ARGS__, MX_VA_NUM_ARGS_REVERSE_SEQUENCE MX_RIGHT_PARENTHESIS
#else
#    define MX_VA_NUM_ARGS(...)                        MX_VA_NUM_ARGS_HELPER(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#    define MX_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
#endif

// MX_PASS_VA passes __VA_ARGS__ as multiple parameters to another macro, working around the above-mentioned bug
#if _MSC_VER >= 1400
#    define MX_PASS_VA(...)                            MX_LEFT_PARENTHESIS __VA_ARGS__ MX_RIGHT_PARENTHESIS
#else
#    define MX_PASS_VA(...)                            (__VA_ARGS__)
#endif

/* example usage:
int c1 = MX_VA_NUM_ARGS(x);	// 1
int c3 = MX_VA_NUM_ARGS(x,y,z);	// 3
int c10 = MX_VA_NUM_ARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10);	// 10
*/





#define mxGETTER(name, type, var)					\
	type Get##name() const { return var; }

#define mxSETTER(name, type, var)					\
	void Set##name(type v) { var = v; }

#define mxACCESSOR(name, type, var)				\
	mxGETTER(name, type, var)						\
	mxSETTER(name, type, var)


//-------------------------------------------------------------
//	Control statements.
//-------------------------------------------------------------

/// should be used in switch statements to indicate fall-through
#define mxCASE_FALLTHROUGH		;

/// Tells the compiler, that the default case is never executed.
/// In cases where no default is present or appropriate, this causes MSVC to generate
/// as little code as possible, and throw an assertion in debug.
///
#define mxNO_SWITCH_DEFAULT			default: mxUNREACHABLE

#define mxSTATE_CASE_STRING(x)      case x: return #x;

// if the given expression returned 'false' then return 'false'
#define RET_FALSE_IF_NOT(expr)		if(!(expr))	return false;
#define RET_FALSE_IF_NIL(expr)		if( (expr) == nil )	return false;

#define RET_IF_NOT(expr)			if(!(expr))	return;
#define RET_IF_NIL(expr)			if( (expr) == nil )	return;

#define RET_X_IF_NOT(expr, retVal)	if(!(expr))	return (retVal);
#define RET_X_IF_NIL(expr, retVal)	if( (expr) == nil )	return (retVal);

#define RET_NIL_IF_NOT(expr)		if(!(expr))	return nil;
#define RET_NIL_IF_NIL(expr)		if( (expr) == nil )	return nil;


#define chkRET_FALSE_IF_NOT(expr)\
	{\
		const bool bOk = (expr);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return false;\
	}

#define chkRET_FALSE_IF_NIL(expr)\
	{\
		const bool bOk = ((expr) != nil);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return false;\
	}

#define chkRET_IF_NOT(expr)\
	{\
		const bool bOk = (expr);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return;\
	}

#define chkRET_IF_NIL(expr)\
	{\
		const bool bOk = ((expr) != nil);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return;\
	}

#define chkRET_NIL_IF_NOT(expr)\
	{\
		const bool bOk = (expr);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return nil;\
	}

#define chkRET_NIL_IF_NIL(expr)\
	{\
		const bool bOk = ((expr) != nil);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return nil;\
	}

#define chkRET_X_IF_NOT(expr, retVal)\
	{\
		const bool bOk = (expr);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return (retVal);\
	}

#define chkRET_X_IF_NIL(expr, retVal)\
	{\
		const bool bOk = ((expr) != nil);\
		mxASSERT2(bOk, #expr);\
		if( !bOk )	return (retVal);\
	}

//-----------------------------------------------------------------
//		Error-handling macros used throughout the engine.
//-----------------------------------------------------------------

// ToDo: cleanup, remove redundancy/complexity.

#if MX_DEVELOPER
	#define DEVOUT(...)		ptPRINT(__VA_ARGS__)
#else
	#define DEVOUT(...)
#endif


#if MX_DEVELOPER

	#define ptPRINT(...)\
		mxMACRO_BEGIN\
			TbLog( __FILE__, __LINE__, __FUNCTION__, LL_Info, __VA_ARGS__ );\
		mxMACRO_END


	#define ptWARN(...)\
		mxMACRO_BEGIN\
			TbLog( __FILE__, __LINE__, __FUNCTION__, LL_Warn, __VA_ARGS__ );\
		mxMACRO_END


	#define ptERROR(...)\
		mxMACRO_BEGIN\
			if(TbLog( __FILE__, __LINE__, __FUNCTION__, LL_Error, __VA_ARGS__ ) == CF_Break)ptDBG_BREAK;\
		mxMACRO_END


	#define ptFATAL(...)\
		mxMACRO_BEGIN\
			if(TbLog( __FILE__, __LINE__, __FUNCTION__, LL_Fatal, __VA_ARGS__ ) == CF_Break)ptDBG_BREAK;\
		mxMACRO_END

	#define LOG_TRACE(...)\
		mxMACRO_BEGIN\
			TbLog( __FILE__, __LINE__, __FUNCTION__, LL_Trace, __VA_ARGS__ );\
		mxMACRO_END

#else

	#define ptPRINT(...)\
		mxMACRO_BEGIN\
		mxMACRO_END

	#define ptWARN(...)\
		mxMACRO_BEGIN\
		mxMACRO_END

	#define ptERROR(...)\
		mxMACRO_BEGIN\
		mxMACRO_END

	#define ptFATAL(...)\
		mxMACRO_BEGIN\
		mxMACRO_END

	#define LOG_TRACE(...)\
		mxMACRO_BEGIN\
		mxMACRO_END

#endif


#define mxSUCCEDED( X )		( (X) == ALL_OK )
#define mxFAILED( X )		( (X) != ALL_OK )


/// mxDO(X) - evaluates the given expression; if it fails,
/// causes an error and returns the corresponding error code.
#if MX_DEBUG || MX_DEVELOPER
	#define mxDO( X, ... )\
		mxMACRO_BEGIN\
			const ERet __result = (X);\
			if( mxFAILED(__result) )\
			{\
				const char* error_message = EReturnCode_To_Chars( __result );\
				mxASSERT2( !#X, "%s(%d): '%s' failed with '%s'\n", __FILE__, __LINE__, #X, error_message );\
				return __result;\
			}\
		mxMACRO_END
#else
	#define mxDO( X, ... )\
		mxMACRO_BEGIN\
			const ERet __result = (X);\
			if( mxFAILED(__result) )\
			{\
				ptERROR( "An unexpected error occurred. Result code: %d\n", (int)__result );\
				return __result;\
			}\
		mxMACRO_END
#endif

/// mxTRY(X) - evaluates the given expression and simply returns an error code if the expression fails.
#define mxTRY( X )\
	mxMACRO_BEGIN\
		const ERet __result = (X);\
		if( mxFAILED(__result) )\
		{\
			return __result;\
		}\
	mxMACRO_END


//
// Causes a breakpoint exception in debug builds, causes fatal error in release builds.
//
#if MX_DEBUG || MX_DEVELOPER

	#define nwDO_MSG( RESULT_CODE_EXPR, ... )\
		mxMACRO_BEGIN\
			const ERet result_code = (RESULT_CODE_EXPR);\
			if( result_code != ALL_OK ){\
				static int s_assert_behavior = AB_Break;\
				if( s_assert_behavior != AB_Ignore ){\
					s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #RESULT_CODE_EXPR, __VA_ARGS__ );\
					if( s_assert_behavior == AB_Break ){\
						ptBREAK;\
					}\
				}\
				return result_code;\
			}\
		mxMACRO_END

#else

	#define nwDO_MSG( RESULT_CODE_EXPR, ... )\
		mxMACRO_BEGIN\
			const ERet result_code = (RESULT_CODE_EXPR);\
			if( result_code != ALL_OK ){\
				ptERROR( "An unexpected error occurred. Code: %d\n", (int)result_code );\
				return result_code;\
			}\
		mxMACRO_END

#endif


//
// Causes a breakpoint exception in debug builds, causes fatal error in release builds.
//
#if MX_DEBUG || MX_DEVELOPER
	#define mxENSURE( CONDITION, RETURN_CODE, ... )\
		mxMACRO_BEGIN\
			const bool __result = (CONDITION);\
			if( !__result ){\
				static int s_assert_behavior = AB_Break;\
				if( s_assert_behavior != AB_Ignore )\
					if( !__result ){\
						s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #CONDITION, __VA_ARGS__ );\
						if( s_assert_behavior == AB_Break )\
								ptBREAK;\
					}\
				return (RETURN_CODE);\
			}\
		mxMACRO_END
#else
	#define mxENSURE( EXPRESSION, RETURN_CODE, ... )\
		mxMACRO_BEGIN\
			if( !(EXPRESSION) ){\
				ptERROR( "An unexpected error occurred\n" );\
				return (RETURN_CODE);\
			}\
		mxMACRO_END
#endif

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//== for enabling/disabling code in editor/release mode respectively
//
#if MX_EDITOR
	#define EDITOR_CODE(x)	x
#else
	#define EDITOR_CODE(x)
#endif

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//== for enabling/disabling code in editor/release mode respectively
//
#if MX_EDITOR
#	define IF_EDITOR
#else
#	define IF_EDITOR		mxPT_SLASH(/)
#endif // ! MX_DEBUG




//-------------------------------------------------------------
//	Evil hacks.
//-------------------------------------------------------------


#if 0//MX_DEVELOPER && MX_DEBUG

// This can be used to access private and protected variables.

#define private		public
#define protected	public

// messes up with std::
//#define class		struct

//#define const_cast		"const_cast<> is prohibited"

#endif


//-------------------------------------------------------------
//	Virtual member functions.
//-------------------------------------------------------------

/// abstract overridable method
#define PURE_VIRTUAL		=0

//
// ...because i'm always eager to instantiate my classes and test them;
// it's easy to replace these macros with " = 0" (or with "" in derived classes)
// which would hopefully be done once i got a very stable code base
//

/// for abstract methods that return 'void'
#define _PURE_METHOD_STUB			{ Unimplemented; }

/// for abstract methods that should return a pointer
/// (i.e. this should be "#define _PURE_METHOD_STUB_RET_NULL  = 0;")
#define _PURE_METHOD_STUB_RET_NULL	{ Unimplemented; return nil; }


//-------------------------------------------------------------
//	Loops.
//-------------------------------------------------------------

// NOTE: this is evil, don't use!

#define mxINT_LOOP( var, num )		for( INT var = 0; var < (INT)num; var++ )
#define mxINT_LOOP_i( num )			mxINT_LOOP( i, num )
#define mxINT_LOOP_j( num )			mxINT_LOOP( j, num )
#define mxINT_LOOP_k( num )			mxINT_LOOP( k, num )

// NOTE: potential bug - if (INT)num is -1 then (unsigned)num is UINT_MAX (0xFFFF...)
#define mxUINT_LOOP( var, num )		for( unsigned var = 0; var < (unsigned)num; var++ )
#define mxUINT_LOOP_i( num )		mxUINT_LOOP( i, num )
#define mxUINT_LOOP_j( num )		mxUINT_LOOP( j, num )
#define mxUINT_LOOP_k( num )		mxUINT_LOOP( k, num )


/// use for(;;) instead of while(true)
/// because:
/// 1) avoid 'loop on constant expression' warning
/// 2) true can be #defined to false, etc.
///
#define mxLOOP_FOREVER		for(;;)


// TODO: for_each ?


#define elif	else if


//===========================================================================
/*
	macros for initializing arrays statically.

	e.g.
	enum { MAX_SCOPE_DEPTH = 32 };
	const char MyTextWriter::tabs[ MAX_SCOPE_DEPTH ] = { VAL_32X };

	you should define VAL_1X first.
	don't forget to call #undef for preprocessor clean-up.

	initialize leftovers manually if needed.

	Be careful when array size changes!

	Or use something like this:
	const char MyTextWriter::tabs[] = { VAL_32X };
*/

// You should #define VAL_1X yourself!

#define VAL_2X     VAL_1X,  VAL_1X
#define VAL_4X     VAL_2X,  VAL_2X
#define VAL_8X     VAL_4X,  VAL_4X
#define VAL_16X    VAL_8X,  VAL_8X
#define VAL_32X    VAL_16X, VAL_16X
#define VAL_64X    VAL_32X, VAL_32X
#define VAL_128X   VAL_64X, VAL_64X
#define VAL_256X   VAL_128X, VAL_128X
#define VAL_512X   VAL_256X, VAL_256X
#define VAL_1024X  VAL_512X, VAL_512X
#define VAL_2048X  VAL_1024X, VAL_1024X

//===========================================================================

enum HEX_NUMBERS
{
	HEX_0000 = 0x0,
	HEX_0001 = 0x1,
	HEX_0010 = 0x2,
	HEX_0011 = 0x3,
	HEX_0100 = 0x4,
	HEX_0101 = 0x5,
	HEX_0110 = 0x6,
	HEX_0111 = 0x7,
	HEX_1000 = 0x8,
	HEX_1001 = 0x9,
	HEX_1010 = 0xA,
	HEX_1011 = 0xB,
	HEX_1100 = 0xC,
	HEX_1101 = 0xD,
	HEX_1110 = 0xE,
	HEX_1111 = 0xF,
};


/*
	Some hexadecimal constants,
	can be be used as magic numbers
	(search for Hexspeak):

	0x00C0FFEE
	0xBABE
	0xCAFEBABE
	0xD15EA5E
	0xDEADBABE
	0xDEADFA11
	0xDEADFACE
	0xBEEF
	0x0BAD
	0x0BADCODE
	0x0BADF00D
	0xFACEFEED
	0xFEE1DEAD
	0xDEFECA7E

	0x00FACADE
	0xBABE5A55
	0x5ADCA5E5
*/

// used to refer to integer constants by names instead of (obscure) numbers
#define mxKIBIBYTE	(1<<10)
#define mxMEBIBYTE	(1<<20)
#define mxGIBIBYTE	(1<<30)

// these are used for historical reasons
//@todo: make them non-powers of two (e.g. 1000) ?
#define mxKILOBYTE	(1<<10)
#define mxMEGABYTE	(1<<20)
#define mxGIGABYTE	(1<<30)

#define mxKiB(X) ((X) * 1024ULL)
#define mxMiB(X) (mxKiB((X)) * 1024UL)
#define mxGiB(X) (mxMiB((X)) * 1024ULL)
#define mxTiB(X) (mxGiB((X)) * 1024ULL)

//===========================================================================
//
//	Code markers.
//

// Macro to avoid unused parameter warnings (inhibits warnings about unreferenced variables,
// makes sure the specified variable "is used" in some way so that the compiler does not give a "unused parameter" or similar warning).
// This macro may be useful whenever you want to declare a variable, but (by intention) never use it. For example for documentation purposes.
//
#define mxUNUSED( x )		((void)( &x ))
/*
// For preventing compiler warnings "unreferenced formal parameters".
template< typename T >
void UnusedParameter( const T& param )
{ (void)param; }
*/


/// so that all ugly casts can be found with a text editor
#define c_cast(x)		(x)


template< typename FROM, typename TO >
inline TO implicit_cast( FROM const &x )
{
	return x;
}

#define MemZero( Destination, Length )		::memset( (Destination), 0, (Length) )

/*
#define and		&& 
#define and_eq	&=
#define bitor	| 
#define or		||
#define or_eq	|=
#define xor_eq	^= 
#define xor		^ 
#define not		!
#define compl	~ 
#define not_eq	!= 
#define bitand	& 
*/

///	Global variables.
#define global_

///	Static variables (in function or file scope), this is usually used to mark dirty hacks.
#define local_		static

///	Static members of a class shared by all instanced of the class.
#define shared_		static

// access qualifiers of class members
#define public_internal		public
#define protected_internal	protected
#define private_internal	private

#define public_readonly		public
#define protected_readonly	protected
#define private_readonly	private

/// remember: 'const' is a "virus".
#define const_

//#define in_	
//#define out_
//#define inout_

// thread safety
#define mt_safe
#define mt_unsafe

// Use volatile correctness on methods to indicate the thread safety
// of their calls. This is not the same as the pointless effort of 
// declaring data volatile. Here, a class pointer should be declared
// volatile to disable non-volatile method calls the same way declaring
// a class pointer const disables all methods not labelled const. On
// methods, volatile doesn't have any assembly-level memory access 
// fencing implications - it's just a flag that hopefully can move some
// runtime errors of calling unsafe methods to compile time.
#ifndef threadsafe
#define threadsafe volatile
#endif

// Create a different type of cast so that when inspecting usages the 
// user can see when it is intended that the code is casting away thread-safety.
template<typename T, typename U> inline T thread_cast(const U& threadsafeObject) { return const_cast<T>(threadsafeObject); }


// used to mark optional parameters (e.g. pointers which can be null)
#define mxOPTIONAL( typeName )		typeName

// The following macro "NoEmptyFile()" can be put into a file
// in order suppress the MS Visual C++ Linker warning 4221:
// warning LNK4221: no public symbols found; archive member will be inaccessible
// Solution: Use an anonymous namespace:
// Symbols within an anonymous namespace have external linkage,
// so there will be something in the export table.
// This warning occurs on PC and XBOX when a file compiles out completely
// has no externally visible symbols which may be dependent on configuration
// #defines and options.
#define mxNO_EMPTY_FILE	namespace { char NoEmptyFileDummy##__LINE__; }

//-------------------------------------------------------------
//	STATIC STRING MACROS
//-------------------------------------------------------------

// string names are used for faster lookups
#define mxHASH_STR(STATIC_STRING)	GetStaticStringHash(STATIC_STRING)
#define mxHASH_ID(NAME)				GetStaticStringHash(TO_STR(NAME))



//-------------------------------------------------------------
//	Markers (developer notes).
//-------------------------------------------------------------
//
//	Pros: bookmarks are not needed;
//		  easy to "find All References" and no comments needed;
//		  you can #define them to what you like (e.g. #pragma message).
//	Cons: changes to the source file require recompiling;
//		  time & date have to be inserted manually.
//

/// Means there's more to do here, don't forget.
/// https://www.approxion.com/todo-or-not-todo/
/// https://help.semmle.com/wiki/display/CCPPOBJ/TODO+comment
#define mxTODO( message_string )
/// 'soft' 'undone', no breakpoint/exception
#define mxUNDONE

#define mxREFACTOR( message_string )
#define mxREMOVE_OLD_CODE

/// the code is new (usually, to try out a new, 'genious' idea), it's not well tested and prone to bugs
#define mxEXPERIMENTAL

///means there's a known bug here, explain it and optionally give a bug ID.
#define mxBUG( message_string )
#define mxFIXME( message_string )

///KLUDGE - When you've done something ugly say so and explain how you would do it differently next time if you had more time.
#define mxHACK( message_string )
#define mxREMOVE_THIS
#define mxUGLY
///deprecated
#define mxOBSOLETE

/// Tells somebody that the following code is very tricky so don't go changing it without thinking.
#define mxNOTE( message_string )

/// used to mark code that has been nicked from somewhere (for self-education)
#define mxSTOLEN( message_string )

/// Beware of something.
#define mxWARNING_NOTE( message_string )

/// unsafe code (amenable to buffer overruns, etc)
#define mxUNSAFE

/// Sometimes you need to work around a compiler problem. Document it. The problem may go away eventually.
#define mxCOMPILER_PROBLEM( message_string )

/// e.g.: fresh and buggy drivers for graphics card, bugs in new DirectX 11 debug layer
#define mxPLATFORM_PROBLEM( message_string )

/// used to mark code that is not thread-safe
#define mxMT_UNSAFE

/// the code should be optimized
#define mxOPTIMIZE( message_string )

/// the code should be cleaned up
#define mxTIDY_UP_FILE

/// quick hacks that should be removed ('technical debt')
#define mxTEMP

/// long-term hacks
#define mxPERM( message_string )

/// All string constants should be enclosed in mxSTRING()
#define mxSTRING(_str)	(_str)

/// 'to be documented'
#define mxTBD

/// Bibliographical references and notes
#define mxBIBREF(__notes__, ...)

/// used to mark code that works and i don't know yet how it works;
/// right from the book:
/// Enjoy as you do so, as one of the few things more rewarding than programming
/// and seeing a correctly animated, simulated, and rendered scene on a screen
/// is the confidence of understanding how and why everything worked.
/// When something in a 3D system goes wrong (and it always does),
/// the best programmers are never satisfied with "I fixed it, but I'm not sure how";
/// without understanding, there can be no confidence in the solution, and nothing new
/// is learned. Such programmers are driven by the desire to understand what
/// went wrong, how to fix it, and learning from the experience.
#define mxWHY( message_string )
