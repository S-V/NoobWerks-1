/*  see copyright notice in squirrel.h */
#ifndef _SQPCHEADER_H_
#define _SQPCHEADER_H_

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
  // C++11 is supported
#else
	// C++03
	#define nullptr (NULL)
#endif

// for uint32_t, etc.
#include <stdint.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <new>
//squirrel stuff
#include <squirrel.h>
#include "sqobject.h"
#include "sqstate.h"

#endif //_SQPCHEADER_H_
