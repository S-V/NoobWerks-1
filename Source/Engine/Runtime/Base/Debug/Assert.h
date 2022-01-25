/*
=============================================================================
	File:	Debug.h
	Desc:	Code for debug utils, assertions and other very useful things.
	Note:	Some debug utils depend heavily on the platform.
			Disabled passing __FUNCTION__, because it led to bloat with templates, e.g.:
			'TArrayBase<struct GL::InputLayoutD3D11,struct THandleManager<struct GL::InputLayoutD3D11> >::operator []'
	References:
	Assertions in Game Development:
	http://www.itshouldjustworktm.com/?p=155
	ToDo:	clean up
=============================================================================
*/
#pragma once

/*
=============================================================================

		Run-time checks and assertions.

=============================================================================
*/

///	EAssertBehavior - used to specify actions taken on assert failures.
enum EAssertBehavior
{
	AB_Break = 0,		//!< trigger a breakpoint
	AB_Ignore = 1,		//!< continue and don't report this assert from now on
	AB_Continue = 2,	//!< continue execution, simply return from the assertion failure
	AB_FatalError = 3,	//!< the game is over, abort the mission!
};

/// user assertion failure handler should contain user logic (wrapped in your favourite GUI) and writing to log;
/// NOTE: 'message' is optional, can be null
typedef EAssertBehavior (*UserAssertCallback)(
	const char* _file, int _line, const char* _function,
	const char* _expression,
	const char* _format, va_list _args
);

//! Set a user-defined assertion-failure callback.
/** This function registers a user-defined routine to call in the event of an
* assertion failure.
* \param cb A callback routine to call that is of type UserAssertCallback.
* \return Returns the previous callback, if one was registered.
* \note The user-defined callback should return 0 to have the program simply return
* from the assertion failure and not process it any further,
* nonzero to continue with assertion processing for the host OS.
*/
extern UserAssertCallback SetUserAssertCallback( UserAssertCallback cb );

namespace Debug
{
	namespace Assertions
	{
		void SetAllAssertionsEnabled( bool _enable );
	}//namespace Assertions
}//namespace Debug

// internal function;
// returns assertion behaviour (e.g. whether the assertion should trigger a breakpoint or silently ignored)
//
extern int ZZ_OnAssertionFailed(
	const char* _file, int _line, const char* _function,
	const char* expression,
	const char* message, ...
);

/*
=============================================================================
	mxASSERT asserts that the statement x is true, and continue processing.

	If the expression is true, continue processing.

	If the expression is false, trigger a debug breakpoint.

	These asserts are only present in DEBUG builds.

	Examples of usage:
		mxASSERT( a > b );
		mxASSERT2( x == y, "X must be equal to Y!" );
		mxASSERT2( size > 16, "Expected at least 16 bytes, but got %d!", size );

	NOTE: Asserts should not have any side effects.

	In debug mode the compiler will generate code for asserts.
	When using MVC++, asserts trigger a break point interrupt inside a macro.
	This way the program brakes on the line of the assert.
=============================================================================
*/

#if MX_DEBUG || MX_DEVELOPER

	#define mxASSERT( X )\
		mxMACRO_BEGIN\
			static int s_assert_behavior = AB_Break;\
			if( s_assert_behavior != AB_Ignore )\
				if( !(X) ){\
					s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #X, "" );\
					if( s_assert_behavior == AB_Break )\
						ptBREAK;\
				}\
		mxMACRO_END

	#define mxASSERT2( X, ... )\
		mxMACRO_BEGIN\
			static int s_assert_behavior = AB_Break;\
			if( s_assert_behavior != AB_Ignore )\
				if( !(X) ){\
					s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #X, __VA_ARGS__ );\
					if( s_assert_behavior == AB_Break )\
						ptBREAK;\
				}\
		mxMACRO_END

	/// EXEC_ON_BREAK - A small piece of code to execute when the breakpoint is triggered
	/// (can be useful to print out variable values, etc.).
	#define mxASSERT3( EXPR, EXEC_ON_BREAK )\
		mxMACRO_BEGIN\
			static int s_assert_behavior = AB_Break;\
			if( s_assert_behavior != AB_Ignore )\
				if( !(EXPR) ){\
					s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #EXPR, "" );\
					if( s_assert_behavior == AB_Break ) {\
						EXEC_ON_BREAK;\
						ptBREAK;\
					}\
				}\
		mxMACRO_END

#else // ! MX_DEBUG

	#define mxASSERT( X )						mxNOOP;(void)(X)
	#define mxASSERT2( X, ... )					mxNOOP;(void)(X)
	#define mxASSERT3( EXPR, EXEC_ON_BREAK )	mxNOOP;(void)(EXPR)

#endif // ! MX_DEBUG


#define mxASSERT_PTR( ptr )			mxASSERT( mxIsValidHeapPointer((ptr)) )

/*
=============================================================================
	CHK(x) is always evaluated, returns the (boolean) value of the passed (logical) expression.

	e.g.:
		bool	b;

		b = CHK(5>3);
		b = CHK(5>1);
		b = CHK(5>1) && CHK(4<1);	// <- will break on this line (4<1)
		b = CHK(5>9);				// <- and here

	Real-life example:
		bool Mesh::isOk() const
		{
			return CHK(mesh != null)
				&& CHK(numBatches > 0)
				&& CHK(batches != null)
				;
		}
=============================================================================
*/
#if MX_DEBUG || MX_DEVELOPER
	#define CHK( expr )		( (expr) ? true : (ptDBG_BREAK,false) )
#else
	#define CHK( expr )		(expr)
#endif // ! MX_DEBUG


/*
The verify(x) macro just returns true or false in release mode, but breaks
in debug mode.  That way the code can take a non-fatal path in release mode
if something that's not supposed to happen happens.

if ( !verify(game) ) {
	// This should never happen!
	return;
}

(this is taken from Quake 4 SDK)
*/

#if 0
#if MX_DEBUG
	#define mxVERIFY( X )	((X) ? true :\
									((ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #X, "" )==AB_Break) ? ptBREAK : false))
#else
	#define mxVERIFY( X )	((X))
#endif
#endif

#if 0
//
// FullAsserts are never removed in release builds, they slow down a lot.
//
#define AlwaysAssert( expr )		\
	mxMACRO_BEGIN					\
	static bool bIgnoreAlways = 0;	\
	if(!bIgnoreAlways)				\
		if(!(expr))					\
			if(ZZ_OnAssertionFailed( #expr, __FILE__, __FUNCTION__, __LINE__, &bIgnoreAlways ))ptDBG_BREAK;\
	mxMACRO_END


#define AlwaysAssertX( expr, message )	\
	mxMACRO_BEGIN						\
	static bool bIgnoreAlways = 0;		\
	if(!bIgnoreAlways)					\
		if(!(expr))						\
			if(ZZ_OnAssertionFailed( #expr, __FILE__, __FUNCTION__, __LINE__, &bIgnoreAlways, message ))ptBREAK;\
	mxMACRO_END
#endif



/*
another version:
#define AlwaysAssert( expr )	\
	do { if(!(expr)) ZZ_OnAssertionFailed( #expr, __FILE__, __FUNCTION__, __LINE__ ); } while(0)

#define AlwaysAssertX( expr, message )	\
	do { if(!(expr)) ZZ_OnAssertionFailed( message, #expr, __FILE__, __FUNCTION__, __LINE__ ); } while(0)
*/

/* old code:
#define AlwaysAssert( expr )	\
	(void)( (expr) ? 1 : (ZZ_OnAssertionFailed( #expr, __FILE__, __FUNCTION__, __LINE__ ) ))

#define AlwaysAssertX( expr, message )	\
	(void)( (expr) ? 1 : (ZZ_OnAssertionFailed( message, #expr, __FILE__, __FUNCTION__, __LINE__ ) ))
*/

#define mxASSUME( X )		{ mxASSERT((X)); mxOPT_HINT(X); }


/*
=============================================================================

		Compile-time checks and assertions.

		NOTE: expressions to be checked must be compile-time constants!
=============================================================================
*/

/*
============================================================================
	StaticAssert( expression )
	STATIC_ASSERT2( expression, message )

	Fires at compile-time !

	Usage:
		StaticAssert( sizeof(*void) == 4, size_of_void_must_be_4_bytes );
============================================================================
*/


	#define mxSTATIC_ASSERT( expression )\
		typedef char PP_JOIN(CHECK_LINE_, __LINE__)	[ (expression) ? 1 : -1 ];\

	//// This macro has no runtime side affects as it just defines an enum
	//// whose name depends on the current line,
	//// and whose value will give a divide by zero error at compile time if the assertion is false.
	//// removed because of warning C4804: '/' : unsafe use of type 'bool' in operation
	////#define mxSTATIC_ASSERT( expression )\
	////	enum { PP_JOIN(CHECK_LINE_, __LINE__) = 1/(!!(expression)) }


	//#define mxSTATIC_ASSERT( expression )	switch(0){case 0:case (expression):;}
    


#define mxSTATIC_ASSERT2( expression, message )		\
	struct ERROR_##message {						\
		ERROR_##message() {							\
			int _##message[ (expression) ? 1 : -1 ];\
			(void)_##message;/* inhibit warning: unreferenced local variable */\
		}											\
	}


#if mxCPP_VERSION <= mxCPP_VERSION_x03
	// _MSC_VER < 1600
	#define static_assert( expression, message )		switch(0){case 0:case (expression):;}
#else
	//
#endif // CPP_0X


/*
============================================================================
	STATIC_CHECK( expression )
	STATIC_CHECK2( expression, message )

	Fires at link-time !
	( unresolved external symbol void CompileTimeChecker<0>(void); )

	Usage:
		STATIC_CHECK2( sizeof(*void) == 4, size_of_void_must_be_4_bytes );

	Note: should only be used in source files, not header files.
============================================================================
*/
namespace Debug
{
	// We create a specialization for true, but not for false.
	template< bool > struct CompileTimeChecker;
	template<> struct CompileTimeChecker< true > {
		CompileTimeChecker( ... );
	};
}//end of namespace Debug

#define mxSTATIC_CHECK( expr )\
	Debug::CompileTimeChecker < (expr) != false > ();

#define mxSTATIC_CHECK2( expr, msg )\
	class ERROR_##msg {\
		Debug::CompileTimeChecker< (expr) != false > $;\
	}

/*
=================================================================
	DELAYED_ASSERT( expression );

	Valid only in function scope !

	Fires at link-time !
	( unresolved external symbol void StaticAssert<0>(void); )

	Usage example:
			void  Foo()
			{
				DELAYED_ASSERT( a == b );

				// ....
			}
=================================================================
* /
namespace debug {

	template< bool > void	StaticAssert();
	template<> inline void	StaticAssert< true >() {}

}//end of namespace debug

#define DELAYED_ASSERT( expression )					\
		::debug::StaticAssert< expression != false >();	\
*/

//----------------------------------------------------------------------------------------------------------
//	These can be very useful compile-time assertions :
/* COMPILE_TIME_ASSERT is for enforcing boolean/integral conditions at compile time.
   Since it is purely a compile-time mechanism that generates no code, the check
   is left in even if _DEBUG is not defined. */
//----------------------------------------------------------------------------------------------------------

// This one is valid in function scope only!
#define mxCOMPILE_TIME_ASSERT( x )				{ typedef int ZZ_compile_time_assert_failed[ (x) ? 1 : -1 ]; }

// This one is valid in function scope only!
#define mxCOMPILE_TIME_ASSERT2( x )				switch(0) { case 0: case !!(x) : ; }

// This one is valid in file and function scopes.
#define mxFILE_SCOPED_COMPILE_TIME_ASSERT( x )	extern int ZZ_compile_time_assert_failed[ (x) ? 1 : -1 ]

#define ASSERT_SIZEOF( type, size )				mxSTATIC_ASSERT( sizeof( type ) == size )
#define ASSERT_OFFSETOF( type, field, offset )	mxSTATIC_ASSERT( offsetof( type, field ) == offset )
#define ASSERT_SIZEOF_8_BYTE_MULTIPLE( type )	mxSTATIC_ASSERT( ( sizeof( type ) & 8 ) == 0 )
#define ASSERT_SIZEOF_16_BYTE_MULTIPLE( type )	mxSTATIC_ASSERT( ( sizeof( type ) & 15 ) == 0 )
#define ASSERT_SIZEOF_32_BYTE_MULTIPLE( type )	mxSTATIC_ASSERT( ( sizeof( type ) & 31 ) == 0 )
#define ASSERT_SIZEOF_ARE_EQUAL( type1, type2 )	mxSTATIC_ASSERT( sizeof( type1 ) == sizeof( type2 ) )

#define mxSTATIC_ASSERT_ISPOW2( X )				mxSTATIC_ASSERT( TIsPowerOfTwo< (X) >::value )

/*
============================================================================
	CHECK_STORAGE( var, required_range )

	// bad explanation, look at the example below
	var - variable (e.g. 'c', where 'c' is of type 'char')
	required_range - [0..max_alowed_value] (e.g. 256)

	Fires at compile-time !

	Usage:
		unsigned char c;
		enum { Required_Range = 257 };	// 'c' must hold up to 257 different values without overflow
		CHECK_STORAGE( c, Required_Range ); // <- error, 'c' can only hold 256 different values

	Multiple checks should be enclosed into brackets:
		{ CHECK_STORAGE( var1, range1 ); }
		{ CHECK_STORAGE( var2, range2 ); }
		etc.
============================================================================
*/

namespace debug
{
	template< unsigned BytesInVariable, unsigned long RequiredRange >
	struct TStorageChecker
	{
		enum {
			bEnoughStorage =  TPow2< BYTES_TO_BITS(BytesInVariable) >::value >= RequiredRange
		};
		//static const bool bEnoughStorage = ( TPow2< BYTES_TO_BITS(BytesInVariable) >::value >= RequiredRange );
	};

	template< unsigned BitsInVariable, unsigned long RequiredRange >
	struct TStorageCheckerBits
	{
		enum {
			bEnoughStorage =  TPow2< BitsInVariable >::value >= RequiredRange
		};
	};

}//end of namespace debug


#define CHECK_STORAGE( size_of_var, required_range )\
	class PP_JOIN(StorageChecker_,__LINE__) {\
		int a[ debug::TStorageChecker< (size_of_var), (required_range) >::bEnoughStorage ? 1 : -1 ];\
	}

#define CHECK_STORAGE_BITS( size_of_var_in_bits, max_value )\
	class PP_JOIN(StorageChecker_,__LINE__) {\
		int a[ debug::TStorageCheckerBits< (size_of_var_in_bits), (max_value) >::bEnoughStorage ? 1 : -1 ];\
	}
