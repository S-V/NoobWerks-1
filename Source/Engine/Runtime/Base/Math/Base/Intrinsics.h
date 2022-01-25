//
#pragma once

// these Windows macros conflict with std::numeric_limits<>
#undef min
#undef max

// gcc and icc defines __SSE3__, ...
// there is no way to know about this on msvc. You can define MM_VECTORIZE_SSE* if you
// want to force the use of those instructions with msvc.

#define MM_NO_INTRINSICS	(0)



#if !MM_NO_INTRINSICS


/// Use SSE4.2 instructions?
#define __SSE4_2__

/// Use SSE4.1 instructions?
#define __SSE4_1__

///
#define __SSSE3__


/*
immintrin.h - AVX 256 bit SIMD instruction set. (Defines __m256.)
wmmintrin.h - AVX AES instructions.
nmmintrin.h - The Core 2 Duo 4.2 SIMD instruction set.
smmintrin.h - The Core 2 Duo 4.1 SIMD instruction set.
tmmintrin.h - SSSE 3 (Supplemental Streaming SIMD Extensions 3) aka SSE 3.1 SIMD instruction set.
pmmintrin.h - SSE 3 SIMD instruction set.
emmintrin.h - SSE 2 SIMD instruction set.
xmmintrin.h - SSE(1) SIMD instruction set. (Defines __m128.)
mmintrin.h - MMX instruction set. (Defines __m64.)

SSE: Pentium III [1999]

SSE2 [2001]: __m128
Intel Pentium 4 - 2001
AMD Opteron/Athlon - 2003

SSE3 [2004]: floating point horizontal_add.

SSSE3 [2006]: permute, blend and lookup functions, integer horizontal_add, integer abs.

SSE4.1 [2007]: select, blend, horizontal_and, horizontal_or, integer max/min, integer multiply (32 and 64 bit),
integer divide (32 bit), 64-bit integer compare (==, !=), floating point round, truncate, floor, ceil.
Intel Penryn - 2007
AMD Bulldozer - Q4 2011

SSE4.2 [2008]: 64-bit integer compare (>, >=, <, <=). 64 bit integer max, min.

AVX (Advanced Vector Extensions Instructions) [2011]: all operations on 256-bit floating point vectors.
Intel Sandy Bridge - Q1 2011
AMD Bulldozer - Q4 2011

FMA3 [2012]: floating point code containing multiplication followed by addition.
Intel Haswell - Q2 2013
AMD Piledriver - 2012

AVX2 [2013]: all operations on 256-bit integer vectors; gather.
Intel Haswell - Q2 2013
AMD Carrizo - Q2 2015

AVX512f [2016]: all operations on 512-bit integer and floating point vectors.
*/
#include <xmmintrin.h>	// SSE 1, __m128
#include <emmintrin.h>	// SSE 2, __m128i
#include <pmmintrin.h>	// SSE 3
#if defined(__SSSE3__)
#	include <tmmintrin.h>
#endif
#if defined(__SSE4_1__)
#	include <smmintrin.h>	// SSE 4.1 (Intel(R) Core(TM) 2 Duo, 2007)
#endif
#if defined(__SSE4_2__)
#	include <nmmintrin.h>	// SSE 4.2
#endif
#if defined(__AVX__) || defined(__AVX2__)	// Advanced Vector Extensions Instructions
#	include <immintrin.h>
#endif

#endif // !MM_NO_INTRINSICS


/*
=======================================================================
	DEFINES
=======================================================================
*/

#define mmINLINE __forceinline

#define mmCACHE_LINE_SIZE  64

#define mmSELECT_0         0x00000000
#define mmSELECT_1         0xFFFFFFFF

#define mmPERMUTE_0X       0x00010203
#define mmPERMUTE_0Y       0x04050607
#define mmPERMUTE_0Z       0x08090A0B
#define mmPERMUTE_0W       0x0C0D0E0F
#define mmPERMUTE_1X       0x10111213
#define mmPERMUTE_1Y       0x14151617
#define mmPERMUTE_1Z       0x18191A1B
#define mmPERMUTE_1W       0x1C1D1E1F

// Stolen from XMISNAN and XMISINF
//#if MM_NO_INTRINSICS
#define mmIS_NAN(x)  ((*(UINT*)&(x) & 0x7F800000) == 0x7F800000 && (*(UINT*)&(x) & 0x7FFFFF) != 0)
#define mmIS_INF(x)  ((*(UINT*)&(x) & 0x7FFFFFFF) == 0x7F800000)
//#endif

/// At link time, if multiple definitions of a COMDAT are seen, the linker picks one and discards the rest.
#define mmGLOBALCONST extern const __declspec(selectany)
