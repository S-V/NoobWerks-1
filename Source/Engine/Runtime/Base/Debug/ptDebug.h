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
		Exception handling.
=============================================================================
*/
#if MX_EXCEPTIONS_ENABLED
	#define mxTHROW(X)		throw(X)
	#define mxCATCH(X)		catch(X)
	#define mxCATCH_ALL		catch(...)
#else
	#define mxTHROW(X)		ptERROR("Unhandled exception occured. The program will now exit.\n")
	#define mxCATCH(X)		if(0)
	#define mxCATCH_ALL		if(0)
#endif //MX_EXCEPTIONS_ENABLED

/*
=============================================================================
		Debug macros.
=============================================================================
*/

// Everything inside the parentheses will be discarded in release builds.
#if MX_DEBUG
	#define mxDEBUG_CODE( code )		code
#else
	#define mxDEBUG_CODE( code )		mxNOOP
#endif // ! MX_DEBUG

/*
=============================================================================
	IF_DEBUG is a C/C++ comment-out macro.

	The preprocessor must support the '//' character sequence
	as a single line comment.

	Example:
	IF_DEBUG outputDevice << "Generating a random world " << world.Name();
=============================================================================
*/
#if MX_DEBUG
#	define IF_DEBUG		// Debug version - enable code on line.
#else
#	define IF_DEBUG		mxPT_SLASH(/)	// Release version - comment out line.
#endif // ! MX_DEBUG

#if MX_DEVELOPER
#	define IF_DEVELOPER		// Developer version - enable code on line.
#else
#	define IF_DEVELOPER		mxPT_SLASH(/)	// Release version - comment out line.
#endif // ! MX_DEVELOPER

/*
=============================================================================

	checked_cast< TypeTo, TypeFrom >

	Works as a static_cast, except that it would use dynamic_cast 
	to catch programming errors in debug mode.
	Uses fast static_cast in Release build, but checks cast with an mxASSERT() in Debug.

	Example:

	(Both 'rxPointLight' and 'rxSpotLight' inherit 'RrLocalLight'.)

	rxPointLight * newLight = new_one(rxPointLight());

	rxSpotLight * pSpotLight = checked_cast< rxSpotLight*>( newLight );	// <= will break here

=============================================================================
*/
#if MX_DEBUG && MX_CPP_RTTI_ENABLED

	template < class TypeTo, class TypeFrom >
	inline TypeTo checked_cast( TypeFrom ptr )
	{
		if ( ptr )
		{
			TypeTo dtto = dynamic_cast< TypeTo >( ptr );
			mxASSERT( dtto != NULL );
			TypeTo stto = static_cast< TypeTo >( ptr );
			mxASSERT( dtto == stto );
			return stto;
		}
		return NULL;
	}

#else // if !defined( MX_DEBUG )

	template < class TypeTo, class TypeFrom >
	inline TypeTo checked_cast( TypeFrom ptr )
	{
		return static_cast< TypeTo >( ptr );
	}

#endif // ! MX_DEBUG

/*

Other versions:

template <class TypeTo, class TypeFrom>
TypeTo checked_cast(TypeFrom p)
{
	mxASSERT(dynamic_cast<TypeTo>(p));
	return static_cast<TypeTo>(p);
}

// perform a static_cast asserted by a dynamic_cast
template <class Type, class SourceType>
Type static_cast_checked(SourceType item)
{
	mxASSERT(!item || dynamic_cast<Type>(item));
	return static_cast<Type>(item);
}

#if MX_DEBUG
#	define checked_cast    dynamic_cast
#else // if !defined( MX_DEBUG )
#	define checked_cast    static_cast
#endif // ! MX_DEBUG


=== For using on references:

/// perform a static_cast asserted by a dynamic_cast
template <class Type, class SourceType>
Type* static_cast_checked(SourceType *item)
{
	mxASSERT(!item || dynamic_cast<Type*>(item));
	return static_cast<Type*>(item);
}

/// overload for reference
template <class Type, class SourceType>
Type &static_cast_checked(SourceType &item)
{
	mxASSERT(dynamic_cast<Type *>(&item));
	return static_cast<Type&>(item);
}

=== Use it like this:
	Derived d;
	Base* pbase = static_cast_checked<Base>(&d);
	Base& rbase = static_cast_checked<Base>(d);
*/

//------------------------------------------------------------------------
//	Use this to mark unreachable locations,
//	such as the unreachable default case of a switch
//	or code paths that should not be executed
//	(e.g. overriden member function Write() of some class named ReadOnlyFile).
//	Unreachables are removed in release builds.
//------------------------------------------------------------------------

#if MX_DEBUG
	#define  mxUNREACHABLE				mxASSERT2(false, "Unreachable location")
	#define  mxUNREACHABLE2( ... )		mxASSERT2(false, __VA_ARGS__)
#else
//#	define  mxUNREACHABLE				mxOPT_HINT( 0 )
//#	define  mxUNREACHABLE2( ... )		mxOPT_HINT( 0 )
#	define  mxUNREACHABLE				(void)0
#	define  mxUNREACHABLE2( ... )		(void)0
#endif // ! MX_DEBUG

#if MX_DEBUG
	#define  mxDBG_UNREACHABLE				mxASSERT2(false, "Unreachable location")
	#define  mxDBG_UNREACHABLE2( ... )		mxASSERT2(false, __VA_ARGS__)
#else
#	define  mxDBG_UNREACHABLE				(void)0
#	define  mxDBG_UNREACHABLE2( ... )		(void)0
#endif // ! MX_DEBUG

//------------------------------------------------------------------------
//	Use this to mark unimplemented features
//	which are not supported yet and will cause a crash.
//  They are not removed in release builds.
//------------------------------------------------------------------------
#if MX_DEBUG
	#define  Unimplemented				{mxASSERT2(false, "Unimplemented feature");}
	#define  UnimplementedX( message )	{ptERROR("%s(%d): Unimplemented feature in %s", __FILE__, __LINE__, __FUNCTION__ );}
#else
	#define  Unimplemented				ptBREAK
	#define  UnimplementedX( message )	ptBREAK
#endif // ! MX_DEBUG


#if MX_DEBUG

	#define  UNDONE		{mxASSERT2(false, "Unimplemented feature");}

#else
	
	// do nothing in release
	#define  UNDONE		((void)0)
	//#define  UNDONE		(MX_DEVELOPER ? (void)0 : ptBREAK)

#endif



//----------------------------------------------------------------------------------------------------------

//
//	COMPILE_TIME_MSG( msg )
//
//	Writes a message into the output window.
//
//	Usage:
//	#pragma COMPILE_TIME_MSG( Fix this before final release! )
//
#define mxCOMPILE_TIME_MSG( msg )		message( __FILE__ "(" TO_STR(__LINE__) "): " #msg )

//TODO: compile-time warnings
#define mxCOMPILE_TIME_WARNING( x, msg )	mxCOMPILE_TIME_ASSERT((x))

//----------------------------------------------------------------------------------------------------------

#if MX_DEBUG
	#define mxDBG_TRACE_CALL		DBGOUT("%s()",__FUNCTION__)
#else
	#define mxDBG_TRACE_CALL
#endif // MX_DEBUG


#if MX_DEBUG

	#define DBG_DO_INTERVAL(what,milliseconds)\
	{\
		static UINT prevTime = mxGetTimeInMilliseconds();\
		const UINT currTime = mxGetTimeInMilliseconds();\
		if( currTime - prevTime > milliseconds )\
		{\
			what;\
			prevTime = currTime;\
		}\
	}

#else

	#define DBG_DO_INTERVAL(what,milliseconds)

#endif // MX_DEBUG

//
//	DBGOUT - The same as Print(), but works in debug mode only.
//
#if MX_DEBUG
	extern void mxVARARGS DBGOUT_IMPL( const char* _format, ... );
	#define DBGOUT( format,...)	DBGOUT_IMPL( __FILE__ "(" TO_STR(__LINE__) "): " format, ##__VA_ARGS__ )
#else
	#define DBGOUT( format, ... ) __noop
#endif
