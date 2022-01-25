/*
=============================================================================
	File:	Windows_MVCpp.h
	Desc:	Windows-specific code for Microsoft Visual C++ compilers.
	ToDo:	don't include "Windows.h" - it slows down compilation!
=============================================================================
*/
#pragma once

#if (mxPLATFORM != mxPLATFORM_WINDOWS) || (mxCOMPILER != mxCOMPILER_MSVC)
#	error This header is only for Visual C++ on Windows!
#endif

// #define MX_AUTOLINK to automatically include the needed libs (via #pragma lib)
#ifndef MX_AUTOLINK
#define MX_AUTOLINK		(1)
#endif

// Check the compiler and windows version.
#if !defined(_MSC_VER)
#	error Microsoft Visual C++ compiler required!
#endif

#if (_MSC_VER < 1500 )	// MVC++ 2008
#	error A version of Microsoft Visual C++ 9.0 ( 2008 ) or higher is required!
#endif

/*
	Microsoft Visual C++ 12.0 ( 2013 )	-	_MSC_VER == 1800
	Microsoft Visual C++ 11.0 ( 2012 )	-	_MSC_VER == 1700
	Microsoft Visual C++ 10.0 ( 2010 )	-	_MSC_VER == 1600
	Microsoft Visual C++ 9.0 ( 2008 )	-	_MSC_VER == 1500
	Microsoft Visual C++ 8.0 ( 2005 )	-	_MSC_VER == 1400
	Microsoft Visual C++ 7.1 ( 2003 )	-	_MSC_VER == 1310
	Microsoft Visual C++ 7.0 ( 2002 )	-	_MSC_VER == 1300
	Microsoft Visual C++ 6.0 ( 1998 )	-	_MSC_VER == 1200
*/
#if ( _MSC_VER == 1200 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 6.0"	// 1998
#elif ( _MSC_VER == 1300 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 7.0"	// 2002
#elif ( _MSC_VER == 1400 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 8.0"	// 2005
#elif ( _MSC_VER == 1500 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 9.0"	// 2007
#elif ( _MSC_VER == 1600 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 10.0"	// 2010
#elif ( _MSC_VER == 1700 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 11.0"	// 2012
#elif ( _MSC_VER == 1800 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 12.0"	// 2013
#elif ( _MSC_VER == 1900 )
#	define mxCOMPILER_NAME		"Microsoft Visual C++ 14.0"	// 2015
#else
#	pragma message( "Unknown compiler!" )
#endif

//
//	Prevent the compiler from complaining.
//
#if !MX_ENABLE_ALL_COMPILER_WARNINGS
	// Disable (and enable) specific compiler warnings.
	// MSVC compiler is very confusing in that some 4xxx warnings are shown even with warning level 3,
	// and some 4xxx warnings are NOT shown even with warning level 4.
	#pragma warning ( disable: 4345 )	// behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
	#pragma warning ( disable: 4351 )	// new behavior: elements of array 'array' will be default initialized
	#pragma warning ( disable: 4511 )	// copy constructor could not be generated
	#pragma warning ( disable: 4512 )	// assignment operator could not be generated
	#pragma warning ( disable: 4505 )	// unreferenced local function has been removed
	#pragma warning ( disable: 4127 )	// conditional expression is constant
	#pragma warning ( disable: 4100 )	// unreferenced formal parameter
	#pragma warning ( disable: 4245 )	// 'argument': conversion from 'type1' to 'type2', signed/unsigned mismatch
	#pragma warning ( disable: 4244 )	// 'argument': conversion from 'double' to 'float', possible loss of data
	#pragma warning ( disable: 4267 )	// 'argument': conversion from 'size_t' to 'int', possible loss of data
	#pragma warning ( disable: 4311 )	// 'variable' : pointer truncation from 'type1' to 'type2'
	#pragma warning ( disable: 4312 )	// 'type cast' : conversion from 'type1' to 'type2' of greater size
	#pragma warning ( disable: 4018 )	// signed/unsigned mismatch
	#pragma warning ( disable: 4324 )	// structure was padded due to __declspec(align())
	#pragma warning ( disable: 4172 )	// returning address of local variable or temporary
	#pragma warning ( disable: 4512 )	// assignment operator could not be generated
	#pragma warning ( disable: 4995 )	// name was marked as #pragma deprecated
	#pragma warning ( disable: 4996 )	// Deprecation warning: this function or variable may be unsafe.
	#pragma warning ( disable: 4099 )	// type name first seen using 'class' now seen using 'struct'
	#pragma warning ( disable: 4786 )	// the fully-qualified name of the class you are using is too long to fit into the debug information and will be truncated to 255 characters
	#pragma warning ( disable: 4355 )	// 'this' : used in base member initializer list
	#pragma warning ( disable: 4163 )	// function not available as an intrinsic function
	#pragma warning ( disable: 4510 )	// default constructor could not be generated
	#pragma warning ( disable: 4158 )	// assuming #pragma pointers_to_members(full_generality, single_inheritance)
	#pragma warning ( disable: 4610 )	// X can never be instantiated - user defined constructor required
	#pragma warning ( disable: 4098 )	// defaultlib conflicts with use of other libs; use /NODEFAULTLIB:library
	#pragma warning ( disable: 4221 )	// no public symbols found; archive member will be inaccessible
	#if !MX_DEBUG
		#pragma warning ( disable: 4702 )	// unreachable code
		#pragma warning ( disable: 4711 )	// selected for automatic inline expansion
		#pragma warning ( disable: 4505 )	// unreferenced local function has been removed
	#endif //!MX_DEBUG
	// Non-standard extensions that I use
	#pragma warning ( disable: 4482 )	// Nonstandard extension: enum used in qualified name.
	#pragma warning ( disable: 4201 )	// Nonstandard extension used : nameless struct/union.
	#pragma warning ( disable: 4239 )	// Nonstandard extension used : conversion from X to Y.
	#pragma warning ( disable: 4480 )	// nonstandard extension used: specifying underlying type for enum
	#pragma warning ( disable: 4238 )	// nonstandard extension used : class rvalue used as lvalue
	#pragma warning ( disable: 4200 )	// nonstandard extension used : zero-sized array in struct/union
	#pragma warning ( disable: 4481 )	// nonstandard extension used: override specifier 'override'
	// Warnings disabled in builds with C++ code analysis enabled.
	#if !MX_DEBUG
	#pragma warning ( disable: 6387 )	// 'argument X' might be 'Y': this does not adhere to the specification for the function 'F'
	#pragma warning ( disable: 6011 )	// Dereferencing NULL pointer
	#pragma warning ( disable: 6211 )	// Leaking memory due to an exception. Consider using a local catch block to clean up memory
	#endif
#endif // !MX_ENABLE_ALL_COMPILER_WARNINGS

//-----------------------------------------------------------------------
//	Make MSVC more pedantic, this is a recommended pragma list
//	from _Win32_Programming_ by Rector and Newcomer.
//-----------------------------------------------------------------------
#pragma warning(error:4002) /* too many actual parameters for macro */
#pragma warning(error:4003) /* not enough actual parameters for macro */
#pragma warning(1:4010)     /* single-line comment contains line-continuation character */
#pragma warning(error:4013) /* 'function' undefined; assuming extern returning int */
#pragma warning(1:4016)     /* no function return type; using int as default */
#pragma warning(error:4020) /* too many actual parameters */
#pragma warning(error:4021) /* too few actual parameters */
#pragma warning(error:4027) /* function declared without formal parameter list */
#pragma warning(error:4029) /* declared formal parameter list different from definition */
#pragma warning(error:4033) /* 'function' must return a value */
#pragma warning(error:4035) /* 'function' : no return value */
#pragma warning(error:4045) /* array bounds overflow */
#pragma warning(error:4047) /* different levels of indirection */
#pragma warning(error:4049) /* terminating line number emission */
#pragma warning(error:4053) /* An expression of type void was used as an operand */
#pragma warning(error:4071) /* no function prototype given */
#pragma warning(error:4150)//deletion of pointer to incomplete type 'x'; no destructor called

#pragma warning(disable:4244)   /* No possible loss of data warnings: Win32 */
#pragma warning(disable:4267)   /* No possible loss of data warnings: x64 */
#pragma warning(disable:4305) /* No truncation from int to char warnings */
#pragma warning(disable:4146) /* Unary minus operator applied to unsigned type, result still unsigned */

#ifndef __cplusplus
#pragma warning(disable:4018) /* No signed unsigned comparison warnings for C code, to match gcc's behavior */
#endif

#pragma warning(disable:4800) /* Forcing value to bool 'true' or 'false' warnings */

#pragma warning(disable:4251)	// 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable:4275)	// non – DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'

#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning(disable: 4996)	// 'stricmp' was declared deprecated
#pragma warning(disable: 4503)  // decorated name length exceeded, name was truncated
#pragma warning(disable: 6255)  // _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
#pragma warning(disable: 4310)  // cast truncates constant value

// Turn on the following very useful warnings.
//#pragma warning(3: 4264)				// no override available for virtual member function from base 'class'; function is hidden
//#pragma warning(3: 4266)				// no override available for virtual member function from base 'type'; function is hidden

// NOTE: this does not work.
// Mainly because this suppresses warning 4221 for the compiler
// but the warning occurs during the linker phase so this has no effect on eliminating the warning.
// See: http://stackoverflow.com/questions/1822887/what-is-the-best-way-to-eliminate-ms-visual-c-linker-warning-warning-lnk4221
// comment out to identify empty translation units
#pragma warning ( disable: 4221 )	// no public symbols found; archive member will be inaccessible

//-----------------------------------------------------------
//	Include windows-specific headers.
//-----------------------------------------------------------

#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

// 0x0501 - Windows XP, 0x0600 - Windows Vista
#ifndef _WIN32_WINNT		// Allow use of features specific to Windows 7 or later.                   
#define _WIN32_WINNT 0x0700	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

// Exclude rarely-used stuff from Windows headers
// to reduce the size of the header files and speed up compilation.
// Excludes things like cryptography, DDE, RPC, the Windows Shell and Winsock.
// unset this option if it causes problems (e.g. GDI plus and DXUT won't compile)
#if 1
	#define NOMINMAX	// Suppress Window's global min/max macros.
	#define NOGDICAPMASKS
	#define OEMRESOURCE
	#define NOATOM

#if !MX_EDITOR
	#define NOCTLMGR
#endif // MX_EDITOR

	#define NOMEMMGR
	#define NOMETAFILE
	#define NOOPENFILE
	#define NOSERVICE
	#define NOSOUND
	#define NOCOMM
	#define NOKANJI
	#define NOHELP
	#define NOPROFILER
	#define NODEFERWINDOWPOS
	#define NOMCX
	#define NOIME
	#define NORPC
	#define NOPROXYSTUB
	#define NOIMAGE
	#define NOTAPE

	#define WIN32_LEAN_AND_MEAN
	#define WIN32_EXTRA_LEAN
	#define VC_EXTRALEAN
#endif

#ifndef STRICT
	#define STRICT					// Use strict declarations for Windows types
#endif

#include <Windows.h>

// C Run-Time Library Header Files
#include <stdio.h>	// sprintf_s
#include <stdlib.h>	// _splitpath_s
#include <stdarg.h>	// vsnprintf, _vsnwprintf
#include <limits.h>
#include <float.h>
#include <math.h>

#include "ptWindowsMacros.h"

//-----------------------------------------------------------
//	Low-level Debug Utilities.
//-----------------------------------------------------------

#include "ptWindowsDebug.h"

//-----------------------------------------------------------
// Low-level Types.
//-----------------------------------------------------------

//
//	Character types.
//
#ifdef _CHAR_UNSIGNED
#	error "Wrong project settings: The 'char' type must be signed!"
#endif

typedef char	ANSICHAR;		// ANSI character.
typedef wchar_t	UNICODECHAR;	// Unicode character.

// Default character type - this can be either 8-bit wide ANSI or 16-bit wide Unicode character.
#if MX_UNICODE
	typedef UNICODECHAR	TCHAR;
#else
	typedef ANSICHAR	TCHAR;
#endif

//
//	Base low-level integer types.
//
//!< BYTE is defined in Windows.h as unsigned char
typedef unsigned __int8		BYTE;

typedef signed __int8		INT8;	// Signed 8-bit integer [-128..127].
typedef unsigned __int8		UINT8;	// Unsigned 8-bit integer [0..255].

typedef signed __int16		INT16;	// Signed 16-bit integer [-32 768..32 767].
typedef unsigned __int16	UINT16;	// Unsigned 16-bit integer [0..65 535].

typedef signed __int32		INT32;	// Signed 32-bit integer [-2 147 483 648..2 147 483 647].
typedef unsigned __int32	UINT32;	// Unsigned 32-bit integer [0..4 294 967 295].

// 64-bit wide integer types.
typedef signed __int64		INT64;	// Signed 64-bit integer [-9,223,372,036,854,775,808 .. 9,223,372,036,854,775,807].
typedef unsigned __int64	UINT64;	// Unsigned 64-bit integer [0 .. 18,446,744,073,709,551,615].

/*/ 128-bit wide integer types. They are not supported on 32-bit architectures.
typedef signed __int128		INT128;	// Signed 128-bit integer.
typedef unsigned __int128	UINT128;	// Unsigned 128-bit integer.
*/

// An integer type that is guaranteed to be the same size as a pointer.
// size_t is used for counts or ranges which need to span the range of a pointer
// ( e.g.: in memory copying routines. )
// Note: these should be used instead of ints for VERY LARGE arrays as loop counters/indexes.
//typedef size_t			SIZE_T;

//
// The most efficient integer types on this platform.
//
typedef signed int		INT;	// Must be the most efficient signed integer type on this platform ( the signed integer type used by default ).
typedef unsigned int	UINT;	// Must be the most efficient unsigned integer type on this platform ( the unsigned integer type used by default ).

//
//	Base IEEE-754 floating-point types.
//
// FLOAT32 : 32 bit IEEE float ( 1 sign , 8 exponent bits , 23 fraction bits )
// FLOAT64 : 64 bit IEEE float ( 1 sign , 11 exponent bits , 52 fraction bits )
typedef float		FLOAT32;	// Used when 32-bit floats are explicitly required. Range: -1.4023x10^-45__+3.4028x10^+38
typedef double		FLOAT64;	// Used when 64-bit floats are explicitly required. Range: -4.9406x10^-324__+1.7977x10^+308

typedef double	DOUBLE;

// 80-bit extended precision floating point data type.
// Note: this type is not accessible through C/C++. Use asm for that.
//typedef long double	FLOAT80;


// Operating system's page size.
enum { OS_PAGE_SIZE = 4096 };

enum Data_Alignment
{
	// Minimum possible memory alignment, in bytes.
	MINIMUM_ALIGNMENT = 4,
	// Maximum possible memory alignment, in bytes.
	MAXIMUM_ALIGNMENT = 128,
	// Preferred data alignment.
	PREFERRED_ALIGNMENT = 4,
	// The most efficient memory alignment.
	EFFICIENT_ALIGNMENT = 16,
	// Memory allocations are aligned on this boundary by default.
	DEFAULT_MEMORY_ALIGNMENT = EFFICIENT_ALIGNMENT,
};

/// handle to the application instance
extern HINSTANCE getAppInstanceHandle();

/// handle to the current thread
extern HANDLE getCurrentThreadHandle();

//-----------------------------------------------------------
//	Platform-independent application entry point.
//	Put the mxAPPLICATION_ENTRY_POINT macro
//	into one of your source files.
//-----------------------------------------------------------

#ifndef _CONSOLE

	#define mxAPPLICATION_ENTRY_POINT	\
		int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd )	\
		{																									\
			UNREFERENCED_PARAMETER( hPrevInstance );														\
			UNREFERENCED_PARAMETER( lpCmdLine );															\
			mxENSURE(mxInitializeBase());																	\
			int ret = mxAppMain();																			\
			mxENSURE(mxShutdownBase());																		\
			return ret;																						\
		}

#else

	#define mxAPPLICATION_ENTRY_POINT	\
		int _tmain( int argc, _TCHAR* argv[] )																\
		{																									\
			UNREFERENCED_PARAMETER( argc );																	\
			UNREFERENCED_PARAMETER( argv );																	\
			mxENSURE(mxInitializeBase());																	\
			int ret = mxAppMain();																			\
			mxENSURE(mxShutdownBase());																		\
			return ret;																						\
		}

#endif //_CONSOLE
