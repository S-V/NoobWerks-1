#pragma once

//-----------------------------------------------------------------------
//	Remove some Windows-specific macros that might cause name clashes.
//-----------------------------------------------------------------------
#undef BYTE
#undef BOOL
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef DOUBLE
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef mxCDECL
// defined in shlobj.h
#undef NUM_POINTS

#undef LoadImage

#if !MX_DEBUG
	// Disable MSVC STL debug + security features.
	#undef _SECURE_SCL
	#define _SECURE_SCL 0

	#undef _SECURE_SCL_THROWS
	#define _SECURE_SCL_THROWS 0

	#undef _HAS_ITERATOR_DEBUGGING
	#define _HAS_ITERATOR_DEBUGGING 0

	#undef _HAS_EXCEPTIONS
	#define _HAS_EXCEPTIONS 0

	// Inhibit certain compiler warnings.
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif

	#ifndef _CRT_NONSTDC_NO_DEPRECATE
	#define _CRT_NONSTDC_NO_DEPRECATE
	#endif

	#ifndef _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_DEPRECATE
	#endif
#endif // !MX_DEBUG

//-----------------------------------------------------------
//	Check current compiler settings.
//-----------------------------------------------------------
//
// RTTI must be disabled in release builds !
//
#ifdef _CPPRTTI
#	define MX_CPP_RTTI_ENABLED	(1)
#	if MX_DEBUG
		//We need it for dynamic_cast ( checked_cast ).
#	elif !MX_EDITOR
		#error Project settings: RTTI must be disabled in release version!
#	endif // !MX_DEBUG && !MX_EDITOR
#else
#	define MX_CPP_RTTI_ENABLED	(0)
#endif

#if !MX_DEBUG && !MX_EDITOR
#	define dynamic_cast	"Use of dynamic_cast is prohibited!"
#endif //!MX_DEBUG && !MX_EDITOR

//
// Exception handling must be disabled in release builds !
//
#ifdef _CPPUNWIND
#	define MX_EXCEPTIONS_ENABLED	(1)
#	if MX_DEBUG
		// We use exceptions in debug mode.
#	else
//		#error Project settings: Disable exception handling!
#	endif // ! MX_DEBUG
#else
#	define MX_EXCEPTIONS_ENABLED	(0)
#endif

#define NO_THROW	throw()

#ifdef UNICODE
#	define MX_UNICODE	(1)
#else
#	define MX_UNICODE	(0)
#endif

// CHAR_BIT is the number of bits per byte (normally 8).
#ifndef CHAR_BIT
	#define CHAR_BIT (8)
#endif // !CHAR_BIT

//-----------------------------------------------------------
//	Compiler-specific macros.
//-----------------------------------------------------------

// don't inline
#define mxNO_INLINE		__declspec(noinline)

// Force a function call site not to be inlined.
#define mxDONT_INLINE(f)	(((int)(f)+1)?(f):(f))

// NOTE: inlined functions make it harder to debug asm, you may want to turn off inlining in debug mode.

// The C++ standard defines that 'inline' doesn't enforce the function to necessarily be inline.
// It only 'hints' the compiler that it is a good candidate for inline function.
#define mxINLINE			inline

// always inline
#ifndef mxFORCEINLINE
	#define mxFORCEINLINE	__forceinline
#endif

// '__noop' discards everything inside parentheses.
#define mxNOOP			__noop

#define mxDEPRECATED	__declspec( deprecated )

// MVC++ compiler allows non-strings in pragmas.
#define mxPRAGMA( x )	__pragma x

//
//	Function attributes extension
//	(to allow the compiler perform more error checking, inhibit unwanted warnings, perform more optimizations, etc)
//	(e.g. it will be __attribute__(x) under GCC, e.g. __attribute__((format(printf,n,n+1)))).
//
#define mxATTRIBUTE( x )

// pure functions have no side effects; in GCC: __attribute__((pure))
#define mxPURE_FUNC

// enables printf parameter validation on GCC
// (equivalent to MX_ATTRIBUTE( (format(printf,1,2)) ) )
#define mxCHECK_PRINTF_ARGS

// enables vprintf parameter validation on GCC
#define mxCHECK_VPRINTF_ARGS

// enables printf parameter validation on GCC
#define mxCHECK_PRINTF_METHOD_ARGS

// enables vprintf parameter validation on GCC
#define mxCHECK_VPRINTF_METHOD_ARGS

/// Visual Studio's non-standard C++ extensions:

/// allows member functions to be made abstract. uses nonstandard C++ extensions provided by MSVC
#define mxABSTRACT		abstract
 
/// marks member functions as being an override of a base class virtual function. uses nonstandard C++ extensions provided by MSVC
#define mxOVERRIDE			override
/// allows classes and member functions to be made sealed. uses nonstandard C++ extensions provided by MSVC
#define mxFINAL				sealed
#define mxFINAL_OVERRIDE	sealed override

//
//	Calling conventions.
//
#define mxVARARGS	__cdecl
#define mxCDECL		__cdecl
#define mxPASCAL	__stdcall
#define mxVPCALL	__fastcall
#define mxCALL		__cdecl
// tells the compiler not to emit function prologue/epilogue code
#define mxNAKED		__declspec( naked )

//-----------------------------------------------------------------------

// The first two pragmas tell the compiler to do inline recursion as deep as it needs to.
// You can change the depth to any number up to 255.
// The other pragma tells the compiler to inline whenever it feels like it.
// this may improve code involving template metaprogramming with recursion
//
#pragma inline_depth( 255 )
#pragma inline_recursion( on )
#if !MX_DEBUG
#pragma auto_inline( on )
#endif

// Don't initialize the pointer to the vtable in a constructor of an abstract class.
// See: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/_langref_novtable.asp
#define mxNO_VTABLE	__declspec( novtable )

// Tell the compiler what size of a pointer to a member of a forward-declared class it should use.
// e.g.
//	class mxVIRTUAL_INHERITANCE Foo;
//	int Foo::*ptr;
// NOTE: You should try to use single inheritance only (for best performance).
//
#define mxSINGLE_INHERITANCE		__single_inheritance
#define mxMULTIPLE_INHERITANCE		__multiple_inheritance
#define mxVIRTUAL_INHERITANCE		__virtual_inheritance

// Generate safe and optimal code using best-case representation.
// The most general representation is single-inheritance, pointer to member function.
// See: http://msdn.microsoft.com/en-us/library/83cch5a6(v=vs.71).aspx
//	http://www.peousware.com/member-function-pointer-size-in-visual-c/
// Generally, in Visual Studio pointers to member functions will occupy either 4, 8 or 16 bytes
// (for single inheritance, multiple inheritance, and virtual inheritance, respectively).
// See: http://blogs.msdn.com/b/oldnewthing/archive/2004/02/09/70002.aspx
// NOTE: commented out because it messes up with fast delegates.
//#pragma pointers_to_members( best_case, single_inheritance )


// Optimization macros (they should be put right after #pragma).
#if MX_DEBUG
	#define mxENABLE_OPTIMIZATION  optimize("",off)
#else
	#define mxENABLE_OPTIMIZATION  optimize("",on)
#endif

#define mxDISABLE_OPTIMIZATION optimize("",off)

// Assume that the given pointer is not aliased.
#define mxRESTRICT_PTR( pointer )		__restrict pointer

// tells the compiler that the return value (RV) of a function is an object that will not be aliased with any other pointers
#define mxRESTRICT_RV		__declspec(restrict)

// Tell the compiler that the specified condition is assumed to be true.
// 0 can be used to mark unreachable locations after that point in the program
// (e.g. it's used in switch-statements whose default-case can never be reached, resulting in more optimal code).
// Essentially the 'Hint' is that the condition specified is assumed to be true at
// that point in the compilation.  If '0' is passed, then the compiler assumes that
// any subsequent code in the same 'basic block' is unreachable, and thus usually
// removed.
#define mxOPT_HINT( expr )		__assume(( expr ))

// Tell the compiler that the specified expression is unlikely to be true.
// (Hints to branch prediction system.)
//
#define mxLIKELY( expr )	( expr )
#define mxUNLIKELY( expr )	( expr )

/*
noalias means that a function call does not modify or reference visible global state 
and only modifies the memory pointed to directly by pointer parameters (first-level indirections). 
If a function is annotated as noalias, the optimizer can assume that, in addition to the parameters themselves,
only first-level indirections of pointer parameters are referenced or modified inside the function.
The visible global state is the set of all data that is not defined or referenced outside of the compilation scope,
and their address is not taken. The compilation scope is all source files (/LTCG (Link-time Code Generation) builds) or a single source file (non-/LTCG build).
*/
#define mxNO_ALIAS	__declspec( noalias )

// Function doesn't return.
#define mxNO_RETURN	__declspec( noreturn )

#define mxUNROLL

// The purpose of the following global constants is to prevent redundant 
// reloading of the constants when they are referenced by more than one
// separate inline math routine called within the same function.  Declaring
// a constant locally within a routine is sufficient to prevent redundant
// reloads of that constant when that single routine is called multiple
// times in a function, but if the constant is used (and declared) in a 
// separate math routine it would be reloaded.
//
#define mxGLOBAL_CONST		extern const __declspec(selectany)

// In MVC++ explicit storage types for enums can be specified, e.g.
// enum ETest : char
// {};
#define mxDECL_ENUM_STORAGE( ENUM, STORAGE )	enum ENUM : STORAGE

// Thread-local storage specifier
#define mxTHREAD_LOCAL		__declspec( thread )

// Get the alignment of the passed type/variable, in bytes.
//	BUG: there's a bug in MSVC++: http://old.nabble.com/-Format--Asserts-with-non-default-structure-packing-in-1.37-(MSVC)-td21215959.html
//	_alignof does correctly report the alignment of 2-byte aligned objects,
//	but that only the alignment of structures can be specified by the user.
#define mxALIGNOF( X )	__alignof( X )

#define mxPREALIGN( A )		__declspec(align( A ))

#define mxPOSTALIGN( A )	/* Nothing */

#define mxUNALIGNED( X )	__unaligned( X )

// These macros change alignment of the enclosed class/struct.
#define mxSET_PACKING( byteAlignment )		mxPRAGMA( pack( push, byteAlignment ) )
#define mxEND_PACKING						mxPRAGMA( pack( pop ) )

//--------------------------------------------------------------
//		Compile-time messages.
//--------------------------------------------------------------

// Statements like:
//		#pragma message(Reminder "Fix this problem!")
// Which will cause messages like:
//		C:\Source\Project\main.cpp(47): Reminder: Fix this problem!
// to show up during compiles.  Note that you can NOT use the
// words "error" or "warning" in your reminders, since it will
// make the IDE think it should abort execution.  You can double
// click on these messages and jump to the line in question.
#define mxREMINDER				__FILE__ "(" TO_STR(__LINE__) "): "

// Once defined, use like so: 
// #pragma message(Reminder "Fix this problem!")
// This will create output like: 
// C:\Source\Project\main.cpp(47): Reminder: Fix this problem!

// TIP: you can use #pragma with some bogus parameter
// and Visual C++ will issue a warning (C4068: unknown pragma).

// Compile time output. This macro writes message provided as its argument
// into the build window together with source file name and line number at compile time.
//
// Syntax: mxMSG( ... your text here ...)
//
// Sample: the following statement writes the text "Hello world" into the build window:
//         mxMSG( Hello world )
//
#define mxMSG( x )	__pragma(message( __FILE__ "(" TO_STR(__LINE__) "): " #x ))


//--------------------------------------------------------------
//		Modern C++ support.
//--------------------------------------------------------------

//NOTE: the following doesn't work in practice, so we have to determine the C++ 'feature level' here.
// Compilers that conform to the C++0x specification
// will define the macro __cplusplus with the value 201103L.
//Older: __cplusplus is 1
//C++98: __cplusplus is 199711L
//C++11: __cplusplus is 201103L
//C++14: __cplusplus is 201402L


#if _MSC_VER < 1600	// before Microsoft Visual C++ 10.0 ( 2010 )
	#define mxCPP_VERSION mxCPP_VERSION_x03
#elif _MSC_VER < 1900 // before MSVS-14 CTP1
	#define mxCPP_VERSION mxCPP_VERSION_11
#else
	#define mxCPP_VERSION mxCPP_VERSION_14
#endif

//KLUDGE: stock MVC++ 12 doesn't support 'constexpr'
#if mxCPP_VERSION < mxCPP_VERSION_14
	#define constexpr const
#endif

// Microsoft Visual C++ 11.0 (2012)
#if  _MSC_VER < 1700
	// aka 'sealed'
	//NOTE: conflicts with stb_image: stb_image declares a variable named 'final'
	#define final
#endif

#if  _MSC_VER >= 1900
	#define mxSUPPORTS_VARIADIC_TEMPLATES	(1)
#else
	#define mxSUPPORTS_VARIADIC_TEMPLATES	(0)
#endif

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
