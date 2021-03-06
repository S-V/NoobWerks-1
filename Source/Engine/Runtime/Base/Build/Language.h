/*
=============================================================================
	File:	Language.h
	Desc:	Platform-independent C++ defines.
=============================================================================
*/
#pragma once

//----------------------------------------------
//	find out the supported version of C++.
//----------------------------------------------

#ifndef __cplusplus
#	error A C++ compiler is required!
#endif

// Ideally, the currect C++ version should be determined in a platform- and compiler-independent manner.
// But the the following doesn't work in practice:
// Compilers that conform to the C++0x specification
// will define the macro __cplusplus with the value 201103L.
//Older: __cplusplus is 1
//C++98: __cplusplus is 199711L
//C++11: __cplusplus is 201103L
//C++14: __cplusplus is 201402L
#define mxCPP_VERSION_x03	(0)
#define mxCPP_VERSION_11	(1)
#define mxCPP_VERSION_14	(2)
/*
#if __cplusplus == 201402L
	#define CPP_0X	mxCPP_VERSION_14
#elif __cplusplus == 201103L
	#define CPP_0X	mxCPP_VERSION_11
#else
	#define CPP_0X	mxCPP_VERSION_x03
#endif
*/

#if 0

//
// check C++ keywords
// see: http://www.viva64.com/en/b/0146/
//
#if defined(alignas)\
	||defined(alignof)\
	||defined(asm)\
	||defined(auto)\
	||defined(bool)\
	||defined(break)\
	||defined(case)\
	||defined(catch)\
	||defined(char)\
	||defined(char16_t)\
	||defined(char32_t)\
	||defined(class)\
	||defined(const)\
	||defined(const_cast)\
	||defined(constexpr)\
	||defined(continue)\
	||defined(decltype)\
	||defined(default)\
	||defined(delete)\
	||defined(do)\
	||defined(double)\
	||defined(dynamic_cast)\
	||defined(else)\
	||defined(enum)\
	||defined(explicit)\
	||defined(export)\
	||defined(extern)\
	||defined(false)\
	||defined(float)\
	||defined(for)\
	||defined(friend)\
	||defined(goto)\
	||defined(if)\
	||defined(inline)\
	||defined(int)\
	||defined(long)\
	||defined(mutable)\
	||defined(namespace)\
	||defined(new)&&defined(_ENFORCE_BAN_OF_MACRO_NEW)\
	||defined(noexcept)\
	||defined(nullptr)\
	||defined(operator)\
	||defined(private)\
	||defined(protected)\
	||defined(public)\
	||defined(register)\
	||defined(reinterpret_cast)\
	||defined(return)\
	||defined(short)\
	||defined(signed)\
	||defined(sizeof)\
	||defined(static)\
	||defined(static_assert)\
	||defined(static_cast)\
	||defined(struct)\
	||defined(switch)\
	||defined(template)\
	||defined(this)\
	||defined(thread_local)\
	||defined(throw)\
	||defined(true)\
	||defined(try)\
	||defined(typedef)\
	||defined(typeid)\
	||defined(typename)\
	||defined(union)\
	||defined(unsigned)\
	||defined(using)\
	||defined(virtual)\
	||defined(void)\
	||defined(volatile)\
	||defined(wchar_t)\
	||defined(while)

	#error keyword defined before including C++ standard header
#endif /* if any keyword re#defined... */ 

#endif

//#if !CPP_0X
//	#define constexpr const
//#endif // !CPP_0X

// Annoying C++0x stuff.
namespace std { namespace tr1 {}; using namespace tr1; }

// for placement new()
#include <new>
